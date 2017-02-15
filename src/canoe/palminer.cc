/**
 * @author George Foster
 * @file palminer.cc
 * @brief Mine phrase alignments for decoder features.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

// TODO (includes SparseModel-specific TODOs):
// - Will decoder rules work ok?
// - Distinguish singleton phrases (left out in L1O) somehow (if they occur?).
// - Various possibilities for online learning (even during forced decoding).
// - Dynamic pruning during learning: eg, chop feature set in half if it gets
//   bigger than a certain size.
// - Precision for weights stored in models and in configtool's output.


#include <iostream>
#include <fstream>
#include "logging.h"
#include "file_utils.h"
#include "arg_reader.h"
#include "parse_pal.h"
#include "phrase_table.h"
#include "phrasetable.h"
#include "config_io.h"
#include "basicmodel.h"
#include "inputparser.h"
#include "phrasefinder.h"
#include "sparsemodel.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
palminer [options] pref1 [pref2...]\n\
palminer -pop -m model\n\
\n\
Mine phrase-aligned data for features specified by a set of event templates\n\
(given by the mandatory -m argument below). Features can be either atomic events\n\
or automatically-learned conjunctions (invoked with -c below). Each feature is\n\
assigned a weight based on the examples it participates in. If a decoder model\n\
is specified with -f, weights are learned discriminatively using the perceptron\n\
algorithm; in dynamic mode (-d), the learned features participate in the choice\n\
of a target hypothesis for weight update, otherwise only the base decoder model\n\
chooses. If no decoder model is specified, feature weights are just normalized\n\
frequencies. The final set of features is optionally pruned, then saved to disk\n\
as a SparseModel (see -s).\n\
\n\
Each <pref> in the argument list must correspond to a source, output,\n\
phrase-alignment triple <pref>{.lc,.out,.pal}.  The out and pal files may\n\
contain n-best lists; this condition is detected automatically. See palview -h\n\
for more info about n-best formats.\n\
\n\
Options:\n\
\n\
  -v     Write progress reports to cerr (use multiple times for more).\n\
  -wsf   Write the list of source files to stdout and exit.\n\
  -pop   Populate the model with atomic features prior to learning. This\n\
         option may be used without specifying any <pref> arguments.\n\
  -eval  Don't learn, just evaluate the classification performance of a\n\
         previously-trained model specified by -m.\n\
  -m m   Read sparse model structure file <m>. Use -H for model syntax. [model]\n\
  -f f   Read decoder model from canoe config file <f>. [use none]\n\
  -mod i:d Read only every ith source sentence out of every d sentences. [0:1].\n\
  -n n   Read at most <n> alignments per source sentence, no matter how many\n\
         are available in <pal> (0 = read all) [1].\n\
  -e pfs Colon-separated list of extensions to append to each <pref> to obtain\n\
         names of source, output, and pal files [.lc:.out:.pal]\n\
  -r rx  Extension of canoe-input ('rule') file. [.lc]\n\
  -u     Learn from reference phrase pairs not covered by current phrase table.\n\
         NB: this won't work for some features, such as LexUnal*, that require\n\
         extra info from the phrasetable. [skip these examples if -f is given]\n\
  -mc    Match coverage: if -f specified, consider only competing phrase pairs\n\
         that have the same src length as the forced-decoding reference pair.\n\
  -nf    If -f given, don't include future score when evaluating phrase pairs.\n\
  -c     Learn conjunctions [consider only atomic features].\n\
  -d     Dynamic: include current SparseModel score when choosing current model-best\n\
         hypothesis. Only makes sense with -f.\n\
  -maf f If learning conjunctions, the maximum number of features that may be\n\
         active in a given context in order for new conjunctions to be acquired.\n\
         Larger values lead to slower performance [2000].\n\
  -mff f If learning conjunctions, the minimum feature frequency (not \n\
         weight) required for a feature to be considered for conjunction. [2]\n\
  -z     Don't acquire zero-weight features.\n\
  -p n   Prune sparse model to n features before saving [0 = all]\n\
  -na    Don't average weights across all examples before saving.\n\
  -np    Don't prune inactive feature templates when saving model.\n\
  -s fp  Save sparse model to files beginning with prefix pf [-m argument].\n\
";

// globals

static Uint verbose = 0;
static bool write_source_files = false;
static bool populate = false;
static bool eval = false;
static string modelfile = "model";
static string cfg = "";
static Uint mod_i = 0;
static Uint mod_d = 1;
static Uint n = 1;
static string src_ext = ".lc";
static string out_ext = ".out";
static string pal_ext = ".pal";
static string rul_ext = ".lc";
static bool use_nonfound_phrases = false;
static bool match_coverage = false;
static bool no_future_score = false;
static bool learn_conjunctions = false;
static bool learn_dynamically = false;
static Uint max_features_for_conjoin = 2000;
static Uint min_freq_for_conjoin = 2;
static bool prune0 = false;
static Uint prune = 0;
static bool no_averaging = false;
static bool no_prune_templates = false;
static string sparseout;
static vector<string> prefs;

static void getArgs(int argc, char* argv[]);
static BasicModelGenerator* loadModel(CanoeConfig& c);
static void PhrasePair2PhraseInfo(const PhrasePair& pp, 
                                  const vector<string>& hyp, 
                                  Voc* voc, PhraseInfo& pi);

static PartialTranslation* initPartialTranslation(Uint srclen);
static void deletePartialTranslations(const PartialTranslation* trans);

struct LearningStats {
   Uint ref_found;
   Uint alternatives;
   Uint bm_correct;
   Uint sm_correct;
   Uint cmb_correct;
   LearningStats() : 
      ref_found(0), alternatives(0), bm_correct(0), 
      sm_correct(0), cmb_correct(0) {}
};
static PhraseInfo *findModelBestHyp(BasicModel* bm,
                                    SparseModel* sm,
                                    double& bm_weight,
                                    PhraseFinder* finder,
                                    PartialTranslation* trans, 
                                    PhraseInfo* ref,
                                    PhraseInfo* &ref_found,
                                    LearningStats& stats);

static string pc(Uint p, Uint n)
{
   ostringstream oss;
   oss.precision(3.2);
   oss << " (" << p * 100.0 / n << "%)";
   return oss.str();
}

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);
   Logging::init();

   // initialize

   CanoeConfig c;
   if (cfg != "") {
      c.configFile = cfg;
      c.read(c.configFile.c_str());
      c.check();
      c.verbosity = verbose;
      PhraseTable::log_almost_0 = c.phraseTableLogZero;
      if (c.bLoadBalancing)
         error(ETFatal, "palminer doesn't support load balancing / numbered source sentences");
   } else {
      c.verbosity = 0;
      c.loadFirst = true;  // don't create big src->src phrasetable from input
   }
   BasicModelGenerator* gen = loadModel(c); // reads source sentences if called for
   SparseModel* mp = eval ?
      new SparseModel(modelfile, DirName(cfg), verbose, gen->getOpenVoc(),
                      false, false, false, false) :
      new SparseModel(modelfile, verbose, 
                      learn_conjunctions, max_features_for_conjoin, 
                      min_freq_for_conjoin, prune0, gen->getOpenVoc());
   SparseModel& m = *mp;
   if (populate) m.populate();

   double bm_weight = 1.0, bm_weight_sum = 0.0;
   m.learnInit();
   if (verbose) cerr << m.describeModelStructure() << endl;
   if (verbose > 1) cerr << m.describeActiveEventTemplateImplications() << endl;
   Uint num_examples = 0;
   Uint srcline = 0;
   LearningStats lstats;
    
   // process each phrase-aligned file triple in turn

   for (Uint i = 0; i < prefs.size(); ++i) {

      // setup for reading pal files

      iSafeMagicStream canoe_src(prefs[i]+rul_ext);
      InputParser canoe_reader(canoe_src);
      PalReader pal_reader(prefs[i]+src_ext, prefs[i]+out_ext, prefs[i]+pal_ext, 
                           verbose);
      PSrcSent nss;
      string src, out, pal;
      vector<string> srctoks, outtoks;
      vector<PhrasePair> pps;
      vector<PhraseInfo> pis;
      Uint palline = 0;
      BasicModel* bm = NULL;           // decoder model
      PhraseFinder* finder = NULL;     // find viable phrase pairs in context

      // process each sentence triple

      while (pal_reader.readNext(src, out, pal)) {

         if (verbose && srcline % 10 == 0) cerr << ".";

         // read and parse
         ++palline;
         if (n != 0 && pal_reader.pos() > n)
            continue;
         if (pal_reader.pos() == 1) { // new source sentence
            if (bm) { delete finder; delete bm; bm = NULL;}
            ++srcline;
            if (!(nss = canoe_reader.getMarkedSent()))
               error(ETFatal, "rule file %s is too short", (prefs[i]+rul_ext).c_str());
            m.newSrcSent(*nss);
            splitZ(src, srctoks);
            if (srctoks != nss->src_sent) {
               error(ETFatal, "source sent %d differs between %s and %s versions of %s",
                     srcline, src_ext.c_str(), rul_ext.c_str(), prefs[i].c_str());
            }
            if (cfg != "" && (srcline-1) % mod_d == mod_i) {
               bm = gen->createModel(*nss);
               finder = new RangePhraseFinder(bm->getPhraseInfo(), *bm);
            }
         }
         if ((srcline-1) % mod_d != mod_i) continue;
         if (!parsePhraseAlign(pal, pps))
            error(ETFatal, "bad format in pal file at line %d", palline);
         if (pps.empty()) continue;
         sortByTarget(pps.begin(), pps.end());
         splitZ(out, outtoks);

         // step through phrase pairs like the decoder does, and learn from each

         pis.resize(pps.size());
         PartialTranslation* trans = initPartialTranslation(nss->src_sent.size());
         for (Uint i = 0; i < pps.size(); ++i) {
            if (verbose > 1) cerr << endl;
            PhrasePair2PhraseInfo(pps[i], outtoks, gen->getOpenVoc(), pis[i]);
            PhraseInfo* ref_from_finder = NULL;
            PhraseInfo* mbest_pi = bm ? 
               findModelBestHyp(bm, learn_dynamically ? &m : NULL, bm_weight, 
                                finder, trans, &pis[i], ref_from_finder, lstats) :
               NULL;
            bm_weight_sum += bm_weight;
            PartialTranslation* ref = 
               new PartialTranslation(trans, ref_from_finder ? 
                                      ref_from_finder : &pis[i]);
            PartialTranslation* mbest = NULL;
            if (mbest_pi == &pis[i]) mbest = ref;
            else if (mbest_pi) mbest = new PartialTranslation(trans, mbest_pi);
            if ((bm == NULL || ref_from_finder || use_nonfound_phrases) && !eval)
               m.learn(ref, mbest);
            if (mbest != ref) delete mbest;
            trans = ref;
            ++num_examples;
         }
         deletePartialTranslations(trans);
      }
   }
   if (verbose) cerr << endl;

   if (!eval) {
      if (!no_averaging) m.averageWeights();
      bm_weight = num_examples ? bm_weight_sum / num_examples : 1.0;
      m.scaleWeights(1.0 / bm_weight); // no op unless learn_dynamically set
      if (prune) m.prune(prune);
      m.save(sparseout, !no_prune_templates);
   }

   if (verbose) {
      cerr << endl;
      cerr << "SparseModel contains " 
           << m.numFeatures() << " features and " 
           << m.numEvents() << " events" << endl;
      cerr << "Learned from " << num_examples << " examples" << endl;
      if (cfg != "") {
         cerr << "   reference found for " << lstats.ref_found << pc(lstats.ref_found, num_examples) << endl;
         cerr << "   base model correct for " << lstats.bm_correct << pc(lstats.bm_correct, num_examples) << endl;
         cerr << "   sparse model correct for " << lstats.sm_correct << pc(lstats.sm_correct, num_examples) << endl;
         cerr << "   combined model correct for " << lstats.cmb_correct << pc(lstats.cmb_correct, num_examples) << endl;
         cerr << "   average number of choices = " << lstats.alternatives / double(num_examples) << endl;
         cerr << "   average weight on base model = " << bm_weight << endl;
      }
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "wsf", "pop", "eval", "c", "d", "np", "na", "f:", 
                             "u", "mc", "nf", "m:", "maf:", "mff:", "z", "p:", 
                             "s:", "mod:", "n:", "e:", "r:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, -1, help_message, 
                        "-h", false,
                        SparseModel::describeAllEventTemplates().c_str(), "-H");
   arg_reader.read(argc-1, argv+1);

   vector<string> verboses;
   arg_reader.testAndSet("v", verboses);
   verbose = verboses.size();
   
   string modstring;
   arg_reader.testAndSet("wsf", write_source_files);
   arg_reader.testAndSet("pop", populate);
   arg_reader.testAndSet("eval", eval);
   arg_reader.testAndSet("c", learn_conjunctions);
   arg_reader.testAndSet("d", learn_dynamically);
   arg_reader.testAndSet("na", no_averaging);
   arg_reader.testAndSet("np", no_prune_templates);
   arg_reader.testAndSet("f", cfg);
   arg_reader.testAndSet("u", use_nonfound_phrases);
   arg_reader.testAndSet("mc", match_coverage);
   arg_reader.testAndSet("nf", no_future_score);
   arg_reader.testAndSet("m", modelfile);
   arg_reader.testAndSet("maf", max_features_for_conjoin);
   arg_reader.testAndSet("mff", min_freq_for_conjoin);
   arg_reader.testAndSet("z", prune0);
   arg_reader.testAndSet("p", prune);
   arg_reader.testAndSet("s", sparseout);
   arg_reader.testAndSet("mod", modstring);
   arg_reader.testAndSet("n", n);

   if (sparseout == "") sparseout = modelfile;
   if (modstring != "") {
      vector<Uint> toks;
      if (split(modstring, toks, ":") != 2)
         error(ETFatal, "-mod argument must be of the form i:d");
      mod_i = toks[0];
      mod_d = toks[1];
      if (mod_d <= mod_i)
         error(ETFatal, "in -mod i:d argument, i must be < d");
   }

   if (arg_reader.getSwitch("e")) {
      src_ext = out_ext = pal_ext = "";
      string exts;
      arg_reader.testAndSet("e", exts);
      vector<string> exts_toks;
      split(exts, exts_toks, ":");
      if (exts_toks.size() >= 1) src_ext = exts_toks[0];
      else error(ETWarn, "no src extension supplied with -e - using none");
      if (exts_toks.size() >= 2) out_ext = exts_toks[1];
      else error(ETWarn, "no out extension supplied with -e - using none");
      if (exts_toks.size() >= 3) pal_ext = exts_toks[2];
      else error(ETWarn, "no pal extension supplied with -e - using none");
   }
   arg_reader.testAndSet("r", rul_ext);

   arg_reader.getVars(0, prefs);
   
   if (write_source_files) {
      for (Uint i = 0; i < prefs.size(); ++i)
         cout << prefs[i] << src_ext << endl;
      exit(0);
   }
}

/**
 * Load model(s), optionally reading the source file(s) first and filtering on
 * contents. 
 * @param c config object that specifies the models
 * @return BMG with loaded models
 */
BasicModelGenerator* loadModel(CanoeConfig& c)
{
   // read all source files and parse markup, if called for

   VectorPSrcSent sents;
   if (!c.loadFirst) {
      if (verbose) cerr << "Filtering on input sentences." << endl;
      PSrcSent nss;
      Uint srcline = 0;
      for (Uint i = 0; i < prefs.size(); ++i) {
         iSafeMagicStream input(prefs[i]+rul_ext);
         InputParser reader(input);
         while (nss = reader.getMarkedSent()) {
            if (srcline % mod_d == mod_i)
               sents.push_back(nss);
            ++srcline;
         }
      }
      if (verbose) cerr << "Input read for filtering." << endl;
   }
   // read models, filtering for current source files if they were read

   time_t start;
   time(&start);
   BasicModelGenerator *gen = BasicModelGenerator::create(c, &sents);
   if (verbose)
      cerr << "Loaded data structures in " << difftime(time(NULL), start)
           << " seconds." << endl;
   if (c.verbosity >= 1)
      cerr << endl << "Features of the log-linear model used, in order:"
           << endl << gen->describeModel() << endl;

   return gen;
}

/**
 * PhrasePair -> PhraseInfo
 */
void PhrasePair2PhraseInfo(const PhrasePair& pp, 
                           const vector<string>& hyp, 
                           Voc* voc,
                           PhraseInfo& pi)
{
   pi.src_words.start = pp.src_pos.first;
   pi.src_words.end = pp.src_pos.second;
   pi.phrase.clear();
   for (vector<string>::const_iterator p = hyp.begin() + pp.tgt_pos.first; 
        p != hyp.begin() + pp.tgt_pos.second; ++p)
      pi.phrase.push_back(voc->add(p->c_str()));
   
   pi.phrase_trans_prob = 0.0;
}

/**
 * Make an initial PartialTranslation, representing the starting context, with
 * nothing translated yet. 
 */
static PartialTranslation* initPartialTranslation(Uint srclen)
{
   PartialTranslation* trans = new PartialTranslation(srclen);
   return trans;
}

/**
 * Delete a chain of PartialTranslations, created by initPartialTranslation()
 * and subsequent calls to the appending constructor.
 */
static void deletePartialTranslations(const PartialTranslation* trans)
{
   while (trans) {
      const PartialTranslation* prev = trans->back;
      delete trans;
      trans = prev;
   }
}

/**
 * All we care about when comparing PhraseInfos for findModelBestHyp() is
 * whether src and tgt positions match.
 */
static bool operator==(const PhraseInfo& pi1, const PhraseInfo& pi2) 
{
   return pi1.src_words == pi2.src_words && pi1.phrase == pi2.phrase;
}

/**
 * Find the best PhraseInfo option in the current context according to a
 * weighted combination of the current base model and sparse model.
 * @param bm current base model
 * @param sm SparseModel, NULL for none
 * @param bm_weight weight on base model, ignored if sm == NULL, updated else
 * @param finder used to generate competing PhraseInfo pairs
 * @param trans context to which PhraseInfo pairs are appended
 * @param ref reference PhraseInfo pair from forced decoding
 * @param ref_from_finder PhraseInfo pair from finder than matches ref, or NULL
 * if no such
 * @param stats updated for this example
 * @return the best PhraseInfo pair according to the model (this may be the
 * same as ref)
 */
static PhraseInfo *findModelBestHyp(BasicModel* bm,
                                    SparseModel* sm,
                                    double& bm_weight,
                                    PhraseFinder* finder,
                                    PartialTranslation* trans, 
                                    PhraseInfo* ref,
                                    PhraseInfo* &ref_from_finder,
                                    LearningStats& stats)
{
   Uint v = 0;
   if (sm) swap(sm->verbose, v); // turn off very verbose during scoring
   
   // get potential phrase pairs in context
   vector<PhraseInfo *> phrases;
   finder->findPhrases(phrases, *trans);
   Uint n = phrases.size();
   stats.alternatives += n;

   // score pairs and find the best, recording preferences of each model
   Uint slen = ref->src_words.end - ref->src_words.start;
   Uint ref_i = n, cmb_i = n, bm_i = n, sm_i = n; // index of ref and model prefs
   double cmb_ref = 0.0, bm_ref = 0.0, sm_ref = 0.0;  // scores assigned to reference
   double cmb_tgt = 0.0, bm_tgt = 0.0, sm_tgt = 0.0;  // scores assigned to target
   double bm_best = 0.0, sm_best = 0.0;  // each sub-model's preference
   for (Uint i = 0; i < phrases.size(); ++i) {
      if (match_coverage && 
          (phrases[i]->src_words.end - phrases[i]->src_words.start != slen))
         continue;   // only consider same-length competitors
      PartialTranslation ntrans(trans, phrases[i]); // new complete hyp to score
      double bm_s = bm->scoreTranslation(ntrans, verbose) + 
         (no_future_score ? 0.0 : bm->computeFutureScore(ntrans));
      double sm_s = sm ? sm->score(ntrans) : 0.0;
      double cmb_s = bm_s * bm_weight + sm_s;
      if (cmb_i == n || cmb_s > cmb_tgt) 
         {cmb_i = i; cmb_tgt = cmb_s; bm_tgt = bm_s; sm_tgt = sm_s;}
      if (bm_i == n || bm_s > bm_best) {bm_i = i; bm_best = bm_s;}
      if (sm_i == n || sm_s > sm_best) {sm_i = i; sm_best = sm_s;}
      if (ref_i == n && *ref == *phrases[i]) {
         ref_i = i; cmb_ref = cmb_s; bm_ref = bm_s; sm_ref = sm_s;
      }
   }
   // stats and verbose
   ref_from_finder = (ref_i != n) ? phrases[ref_i] : NULL;
   if (ref_from_finder) ++stats.ref_found;
   if (cmb_i == ref_i) ++stats.cmb_correct;
   if (bm_i == ref_i) ++stats.bm_correct;
   if (sm && sm_i == ref_i) ++stats.sm_correct;
   if (verbose > 1) {
      cerr << "learning from " << phrases.size() << " candidates:" << endl;
      cerr << "   ref ";
      if (!ref_from_finder)
         cerr << "not found" << endl;
      else
         cerr << "at " << ref_i << ", cmb score=" << cmb_ref << ", base score=" 
              << bm_ref << ", sparse score=" << sm_ref << endl;
      cerr << "   tgt at " << cmb_i << ", cmb score=" << cmb_tgt << ", base score=" 
         << bm_tgt << ", sparse score=" << sm_tgt << endl;
      cerr << "   base best at " << bm_i << ", score = " << bm_best << endl;
      cerr << "   sparse best at " << sm_i << ", score = " << sm_best << endl;
      cerr << "   base weight was " << bm_weight << endl;
   }
   // perceptron update for bm score, if appropriate
   if (!eval && sm && ref_from_finder) bm_weight += bm_ref - bm_tgt;
   
   if (sm) sm->verbose = v;

   return cmb_i == ref_i || cmb_i == phrases.size() ?  ref : phrases[cmb_i];
}
