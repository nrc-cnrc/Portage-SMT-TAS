/**
 * @author George Foster, Eric Joanis, Michel Simard
 * @file align-words.cc 
 * @brief Program that aligns words in a set of line-aligned files using IBM
 * models.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <argProcessor.h>
#include <str_utils.h>
#include <exception_dump.h>
#include <file_utils.h>
#include <printCopyright.h>
#include "ibm.h"
#include "hmm_aligner.h"
#include "word_align.h"
#include "word_align_io.h"

using namespace Portage;

static char help_message[] = "\n\
align-words [-hHvni][-o 'format'][-a 'meth args'][-ibm n][-hmm][-twist][-giza]\n\
            [-post] ibm-model_lang2_given_lang1 ibm-model_lang1_given_lang2\n\
            file1_lang1 file1_lang2 ... fileN_lang1 fileN_lang2\n\
\n\
  Align words in a set of line-aligned files using IBM or HMM models. The models\n\
  should be for p(lang2|lang1) and p(lang1|lang2) respectively (see train_ibm);\n\
  <model1> should contain entries of the form 'lang1 lang2 prob', and <model2>\n\
  the reverse. Either model argument can be \"no-model\", in which case no model\n\
  will be loaded; this may be useful with an alignment method that does not\n\
  require models or that requires only one.\n\
\n\
  Output is written to stdout. The format is determined by the output selection\n\
  options (see below).\n\
\n\
Note:\n\
\n\
  To get probabilities per sentence pairs according to an individual word-\n\
  model, it may be easier to use gen_feature_values instead.  E.g.:\n\
    gen_feature_values IBM2SrcGivenTgt ibm2.src_given_tgt src.al tgt.al\n\
  See gen_feature_values -h for details and rescore_train -H for the list of\n\
  available feature functions.\n\
\n\
Options:\n\
\n\
-h     Display this help message and quit.\n\
-H     List available word-alignment methods and quit.\n\
-v     Write progress reports to cerr. Use -vv to get more.\n\
-i     Ignore case (actually: lowercase everything).\n\
-o     Specify output format, one of: \n\
       "WORD_ALIGNMENT_WRITER_FORMATS" [aachen]\n\
       Use -H for documentation of each format.\n\
-a     Word-alignment method and optional args.\n\
       Use -H for the list of methods with documentation.  [IBMOchAligner]\n\
-post  Also print link posterior probabilities according to each model.\n\
-ibm   Use IBM model <n>: 1 or 2\n\
-hmm   Use an HMM model instead of an IBM model (only works with IBMOchAligner).\n\
       [if, for both models, <model>.pos doesn't exist and <model>.dist does,\n\
       -hmm is assumed, otherwise -ibm 2 is the default]\n\
-twist With IBM1, assume one language has reverse word order.\n\
       No effect with IBM2 or HMM.\n\
-giza  IBM-style alignments are to be read from files in GIZA++ format,\n\
       rather than computed at run-time; corresponding alignment files \n\
       should be specified after each pair of text files, like this: \n\
       fileN_lang1 fileN_lang2 align_1_to_2 align_2_to_1...\n\
       Notes:\n\
        - this currently only works with IBMOchAligner\n\
        - you normally still need to provide IBM models as arguments,\n\
          unless you use either of these tricks: specify models called\n\
          \"no-model\", or use the \"-ibm 0\", in which case the program\n\
          does not expect the 2 model arguments.\n\
\n\
HMM only options:\n\
       By default, all HMM parameters are read from the model file. However,\n\
       these options can be used to override the values in the model file:\n\
          -p0 -up0 -alpha -lambda -anchor -end-dist -max-jump\n\
       Boolean options -anchor and -end-dist can be reset using -no<option>.\n\
       A parallel set of options -p0_2, -up0_2, etc, applies to the \n\
       lang1_given_lang2 models. If these options are not present, the values\n\
       for the original set are used for HMMs in both directions.\n\
       See train_ibm -h for documentation of these HMM parameters.\n\
";

// globals

typedef PhraseTableGen<Uint> PhraseTable;

static const char* const switches[] = {
   "v", "vv", "i", "z", "a:", "o:", "hmm", "ibm:", "twist", "giza", "post",
   "p0:", "up0:", "alpha:", "lambda:", "max-jump:",
   "anchor", "noanchor", "end-dist", "noend-dist",
   "p0_2:", "up0_2:", "alpha_2:", "lambda_2:", "max-jump_2:",
   "anchor_2", "noanchor_2", "end-dist_2", "noend-dist_2",
};

static Uint verbose = 0;
static bool lowercase = false;
static string align_method;
static bool twist = false;
static bool giza_alignment = false;
static bool posteriors = false;
static string model1, model2;
static Uint ibm_num = 42; // 42 means not initialized - ARG will set its value
static bool use_hmm = false;
static bool compress_output = false;
static Uint first_file_arg = 2;
static string output_format = "aachen";

// The optional<T> variables are intentionally left uninitialized.
static optional<double> p0;
static optional<double> up0;
static optional<double> alpha;
static optional<double> lambda;
static optional<bool> anchor;
static optional<bool> end_dist;
static optional<Uint> max_jump;

static optional<double> p0_2;
static optional<double> up0_2;
static optional<double> alpha_2;
static optional<double> lambda_2;
static optional<bool> anchor_2;
static optional<bool> end_dist_2;
static optional<Uint> max_jump_2;


static inline char ToLower(char c) { return tolower(c); }

// arg processing

/// align words namespace.
/// Prevents global namespace polution in doxygen.
namespace alignWords {
/// Specific argument processing class for align-words program.
class ARG : public argProcessor {
private:
    Logging::logger m_logger;

public:
   /**
    * Default constructor.
    * @param argc  same as the main argc
    * @param argv  same as the main argv
    * @param alt_help alternate help message
    */
   ARG(const int argc, const char* const argv[], const char* alt_help) : 
      argProcessor(ARRAY_SIZE(switches), switches, 1, -1, help_message, "-h", false, alt_help, "-H"),
      m_logger(Logging::getLogger("verbose.main.arg"))
   {
      argProcessor::processArgs(argc, argv);
   }

   /// See argProcessor::processArgs()
   virtual void processArgs() {
      LOG_INFO(m_logger, "Processing arguments");

      string max_phrase_string;

      if (mp_arg_reader->getSwitch("v")) {verbose = 1;}
      if (mp_arg_reader->getSwitch("vv")) verbose = 2;
      
      mp_arg_reader->testAndSet("i", lowercase);
      mp_arg_reader->testAndSet("a", align_method);
      mp_arg_reader->testAndSet("z", compress_output);
      mp_arg_reader->testAndSet("o", output_format);
      mp_arg_reader->testAndSet("ibm", ibm_num);
      mp_arg_reader->testAndSet("hmm", use_hmm);
      mp_arg_reader->testAndSet("twist", twist);
      mp_arg_reader->testAndSet("giza", giza_alignment);
      mp_arg_reader->testAndSet("post", posteriors);

      mp_arg_reader->testAndSet("p0", p0);
      mp_arg_reader->testAndSet("up0", up0);
      mp_arg_reader->testAndSet("alpha", alpha);
      mp_arg_reader->testAndSet("lambda", lambda);
      mp_arg_reader->testAndSet("max-jump", max_jump);
      mp_arg_reader->testAndSetOrReset("anchor", "noanchor", anchor);
      mp_arg_reader->testAndSetOrReset("end-dist", "noend-dist", end_dist);

      mp_arg_reader->testAndSet("p0_2", p0_2);
      mp_arg_reader->testAndSet("up0_2", up0_2);
      mp_arg_reader->testAndSet("alpha_2", alpha_2);
      mp_arg_reader->testAndSet("lambda_2", lambda_2);
      mp_arg_reader->testAndSet("max-jump_2", max_jump_2);
      mp_arg_reader->testAndSetOrReset("anchor_2", "noanchor_2", anchor_2);
      mp_arg_reader->testAndSetOrReset("end-dist_2", "noend-dist_2", end_dist_2);

      // initialize *_2 parameters from defaults if not explicitly set
      if (!p0_2) p0_2 = p0;
      if (!up0_2) up0_2 = up0;
      if (!alpha_2) alpha_2 = alpha;
      if (!lambda_2) lambda_2 = lambda;
      if (!max_jump_2) max_jump_2 = max_jump;
      if (!anchor_2) anchor_2 = anchor;
      if (!end_dist_2) end_dist_2 = end_dist;

      if (ibm_num == 0) {
         if (!giza_alignment)
            error(ETFatal, "Can't use -ibm=0 trick unless -giza is used");
         first_file_arg = 0;
      } else {
         mp_arg_reader->testAndSet(0, "model1", model1);
         mp_arg_reader->testAndSet(1, "model2", model2);
         if ( ibm_num == 42 && !use_hmm ) {
            // neither -hmm nor -ibm specified; default is IBM2 if .pos files
            // exist, or else HMM if .dist files exist, or error otherwise: we
            // never assume IBM1, because it is so seldom used, it's probably
            // an error; we want the user to assert its use explicitly.
            if ( check_if_exists(IBM2::posParamFileName(model1)) &&
                 check_if_exists(IBM2::posParamFileName(model2)) )
               ibm_num = 2;
            else if ( check_if_exists(HMMAligner::distParamFileName(model1)) &&
                      check_if_exists(HMMAligner::distParamFileName(model2)) )
               use_hmm = true;
            else
               error(ETFatal, "Models are neither IBM2 nor HMM, specify -ibm N or -hmm explicitly.");
         }
      }
   }
};
}; // ends namespace alignWords.


using namespace alignWords;
int MAIN(argc, argv)
{
   printCopyright(2005, "align-words");
   static string alt_help = 
      WordAlignmentWriter::help() +
      "--- Word aligners ---\n\n" + WordAlignerFactory::help();
   ARG args(argc, argv, alt_help.c_str());
   string z_ext(compress_output ? ".gz" : "");

   if (align_method.empty())
     align_method = "IBMOchAligner";

   IBM1* ibm_1=0;
   IBM1* ibm_2=0;

   if (ibm_num == 0) {
     if (verbose) cerr << "**Not** loading IBM models" << endl;
   } else {
     if (use_hmm) {
       if (verbose) cerr << "Loading HMM models" << endl;
       if (model1 != "no-model") 
          ibm_1 = new HMMAligner(model1, p0, up0, alpha, lambda, anchor, end_dist, max_jump);
       if (model2 != "no-model") 
          ibm_2 = new HMMAligner(model2, p0_2, up0_2, alpha_2, lambda_2, anchor_2, end_dist_2, max_jump_2);
     } else if (ibm_num == 1) {
       if (verbose) cerr << "Loading IBM1 models" << endl;
       if (model1 != "no-model") ibm_1 = new IBM1(model1);
       if (model2 != "no-model") ibm_2 = new IBM1(model2);
     } else if (ibm_num == 2) {
       if (verbose) cerr << "Loading IBM2 models" << endl;
       if (model1 != "no-model") ibm_1 = new IBM2(model1);
       if (model2 != "no-model") ibm_2 = new IBM2(model2);
     } else assert(false);
     if (verbose) cerr << "models loaded" << endl;
   }

   WordAlignerFactory* aligner_factory = NULL;
   WordAligner* aligner = NULL;

   if (!giza_alignment) {
     aligner_factory =
       new WordAlignerFactory(ibm_1, ibm_2, verbose, twist, false);
     aligner = aligner_factory->createAligner(align_method);
     assert(aligner);
   }

   string in_f1, in_f2;
   string alfile1, alfile2;
   Uint fpair = 0;

   GizaAlignmentFile* al_1 = NULL;
   GizaAlignmentFile* al_2 = NULL;

   WordAlignerStats stats;

   WordAlignmentWriter *print = WordAlignmentWriter::create(output_format);
   assert(print != NULL);

   for (Uint arg = first_file_arg; arg+1 < args.numVars(); arg += 2) {

      args.testAndSet(arg, "file1", in_f1);
      args.testAndSet(arg+1, "file2", in_f2);
      if (verbose)
         cerr << "reading " << in_f1 << "/" << in_f2 << endl;
      iSafeMagicStream in1(in_f1);
      iSafeMagicStream in2(in_f2);

      if (giza_alignment) {
        arg+=2;
        if (arg+1 >= args.numVars())
          error(ETFatal, "Missing arguments: alignment files");
        args.testAndSet(arg, "alfile1", alfile1);
        args.testAndSet(arg+1, "alfile2", alfile2);
        if (verbose) 
          cerr << "reading aligment files " << alfile1 << "/" << alfile2 << endl;
        if (al_1) delete al_1;
        al_1 = new GizaAlignmentFile(alfile1);
        if (al_2) delete al_2;
        al_2 = new GizaAlignmentFile(alfile2);
        if (aligner_factory) delete aligner_factory;
        aligner_factory = new WordAlignerFactory(al_1, al_2, verbose, twist, false);

        if (aligner) delete aligner;
        aligner = aligner_factory->createAligner(align_method);
        assert(aligner);
      }

      Uint line_no = 0;
      string line1, line2;
      vector<string> toks1, toks2;
      vector< vector<Uint> > sets1;

      while (getline(in1, line1)) {
         if (!getline(in2, line2)) {
            error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
                  in_f1.c_str(), in_f2.c_str());
            break;
         }
         ++line_no;

         if (verbose > 1) cerr << "--- " << line_no << " ---" << endl;

         if (lowercase) {
           transform(line1.begin(), line1.end(), line1.begin(), ToLower);
           transform(line2.begin(), line2.end(), line2.begin(), ToLower);
         }

         toks1.clear(); toks2.clear();
         split(line1, toks1);
         split(line2, toks2);

         if (verbose > 1)
            cerr << line1 << endl << line2 << endl;

         sets1.clear();
         aligner->align(toks1, toks2, sets1);
         if (verbose) stats.tally(sets1, toks1.size(), toks2.size());

         if (verbose > 1) {
            cerr << "---" << endl;
            aligner_factory->showAlignment(toks1, toks2, sets1);
            cerr << "---" << endl;
         }

         (*print)(cout, toks1, toks2, sets1);
         cout.flush();

         if (posteriors) {
            vector<vector<double> > postProbs;
            if (ibm_1) {
               ibm_1->linkPosteriors(toks1, toks2, postProbs);
               assert(postProbs.size() == toks2.size());
               if ( !postProbs.empty() )
                  assert(postProbs[0].size() == toks1.size() + 1);
               cout << "Posterior probs for " << model1 << endl;
               for ( Uint i = 0; i < toks2.size(); ++i )
                  printf("%-8s ", toks2[i].substr(0,8).c_str());
               cout << endl;
               for ( Uint i = 0; i <= toks1.size(); ++i ) {
                  for ( Uint j = 0; j < toks2.size(); ++j )
                     printf((postProbs[j][i] >= 0.0001 ? "%7.5f  " : "%7.2g  "),
                            postProbs[j][i]);
                  cout << (i == toks1.size() ? "NULL" : toks1[i]) << endl;
               }
               cout << endl;
            }
            if (ibm_2) {
               ibm_2->linkPosteriors(toks2, toks1, postProbs);
               assert(postProbs.size() == toks1.size());
               if ( !postProbs.empty() )
               cout << "Posterior probs for " << model2 << endl;
               for ( Uint i = 0; i < toks2.size(); ++i )
                  printf("%-8s ", toks2[i].substr(0,8).c_str());
               cout << "NULL" << endl;
               for ( Uint i = 0; i < toks1.size(); ++i ) {
                  assert(postProbs[i].size() == toks2.size() + 1);
                  for ( Uint j = 0; j <= toks2.size(); ++j )
                     printf((postProbs[i][j] >= 0.0001 ? "%7.5f  " : "%7.2g  "),
                            postProbs[i][j]);
                  cout << toks1[i] << endl;
               }
               cout << endl;
            }
            //cout << "\x0C";
         }

         if (verbose > 1) cerr << endl; // end of block
         if (verbose == 1 && line_no % 1000 == 0)
            cerr << "line: " << line_no << endl;
      }

      if (getline(in2, line2))
         error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
               in_f1.c_str(), in_f2.c_str());

      ++fpair;
   }

   if (verbose) cerr << "done" << endl;
   if (verbose) stats.display();

   // Clean up - we typically skip this step as an optimization, since the OS
   // cleans things up much faster, and just as effectively on Linux hosts.
   #ifdef __KLOCWORK__
      time_t start = time(NULL);
      delete aligner;
      delete aligner_factory;
      delete al_1;
      delete al_2;
      delete print;
      delete ibm_1;
      delete ibm_2;
      cerr << "Spent " << (time(NULL)-start) << " seconds cleaning up";
   #endif

}
END_MAIN

