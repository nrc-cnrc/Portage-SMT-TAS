/**
 * @author George Foster, Eric Joanis
 * @file gen_phrase_tables.cc
 * @brief Program that generates phrase translation tables from IBM models and
 * a set of line-aligned files.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */


#include <argProcessor.h>
#include <exception_dump.h>
#include <printCopyright.h>
#include "tm_io.h"
#include "ibm.h"
#include "hmm_aligner.h"
#include "phrase_table.h"
#include "word_align.h"
#include "phrase_smoother.h"
#include "phrase_smoother_cc.h"
#include "phrase_table_writer.h"

using namespace Portage;

static char help_message[] = "\n\
gen_phrase_tables [-hHvijz][-a 'meth args'][-s 'meth args'][-w nw]\n\
                  [-wf file][-wfvoc voc][-addsw][-twist]\n\
                  [-prune1 n][-prune1w nw][-m max][-min min][-d ldiff][-ali]\n\
                  [-ibm n][-hmm][-p0 p0][-up0 up0][-alpha a][-lambda l]\n\
                  [-[no]anchor][-max-jump max_j][-[no]end-dist]\n\
                  [-lc1 loc][-lc2 loc]\n\
                  [-1 lang1][-2 lang2][-o name][-f1 freqs1][-f2 freqs2]\n\
                  [-multipr d][-giza]\n\
                  ibm-model_lang2_given_lang1 ibm-model_lang1_given_lang2 \n\
                  file1_lang1 file1_lang2 ... fileN_lang1 fileN_lang2\n\
\n\
Generate phrase translation tables from IBM or HMM models and a set of\n\
line-aligned files. The models should be for p(lang2|lang1) and p(lang1|lang2)\n\
respectively: <model1> should contain entries of the form 'lang1 lang2 prob',\n\
and <model2> the reverse.\n\
\n\
Options:\n\
\n\
-h     Display this help message and quit.\n\
-H     List available word-alignment and smoothing methods and quit.\n\
-v     Write progress reports to cerr. Use -vv to get more (-vs for smoothing).\n\
-z     Add .gz to all generated file names (and compress those files).\n\
-prune1  Prune so that each language1 phrase has at most n translations. This\n\
       is based on joint frequencies, and is done before smoothing.\n\
-prune1w  Same as prune1, but multiply nw by the number of words in the current\n\
       source phrase.  When using both -prune1 and -prune1w, keep n + nw*len\n\
       tranlations for a source phrase of len words.\n\
-a     Word-alignment method and optional args. Use -H for list of methods.\n\
       Multiple methods may be specified by using -a repeatedly. [IBMOchAligner]\n\
-s     Smoothing method for conditional probs. Use -H for list of methods.\n\
       Multiple methods may be specified by using -s repeatedly, but these are\n\
       only useful if -multipr output is selected. [RFSmoother]\n\
-w     Add <nw> best IBM1 translations for src and tgt words that occur in the\n\
       given files but don't have translations in phrase table [don't].\n\
-wf    If -w specified, write the corresponding entries to <file>.[12] instead\n\
       of to the output phrasetable. This writes slightly more entries than\n\
       would actually get added to the phrasetable, due to a better algorithm.\n\
       It has no effect if -i is given.\n\
-wfvoc With -w, use vocabulary files <voc>.1 and <voc>.2 to filter IBM1\n\
       translations instead of vocabularies compiled from the input files.\n\
-addsw Add single-word phrase pairs for each alignment link [don't] \n\
-m     Maximum phrase length. <max> can be in the form 'len1,len2' to specify\n\
       lengths for each language, or 'len' to use one length for both\n\
       languages. 0 means no limit. [4,4]\n\
-min   Minimum phrase length. <min> can be in the form 'len1,len2' to specify\n\
       lengths for each language, or 'len' to use one length for both\n\
       languages. Has to be at least 1. [1,1]\n\
-d     Max permissible difference in number of words between source and\n\
       target phrases. [4]\n\
-ali   Allow phrase pairs consisting only of unaligned words in each language\n\
       [don't]\n\
-ibm   Use IBM model <n>: 1 or 2\n\
-hmm   Use an HMM model instead of an IBM model (only works with IBMOchAligner).\n\
       [if, for both models, <model>.pos doesn't exist and <model>.dist does,\n\
       -hmm is assumed, otherwise -ibm 2 is the default]\n\
-twist With IBM1, assume one language has reverse word order.\n\
       No effect with IBM2 or HMM.\n\
-lc1   Do lowercase mapping of lang 1 words to match IBM/HMM models, using\n\
       locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
       (Compilation with ICU is required to use UTF-8 locales.)\n\
-lc2   Do lowercase mapping of lang 2 words to match IBM/HMM models, using\n\
       locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
-1     Name of language 1 (one in left column in model1) [en]\n\
-2     Name of language 2 (one in right col of model1) [fr]\n\
-o     The base name of the generated tables [phrases]\n\
-giza  IBM-style alignments are to be read from files in GIZA++ format,\n\
       rather than computed at run-time; corresponding alignment files \n\
       should be specified after each pair of text files, like this: \n\
       fileN_lang1 fileN_lang2 align_1_to_2 align_2_to_1...\n\
       Notes:\n\
        - you still need to provide IBM models as arguments\n\
        - this currently only works with IBMOchAligner\n\
        - this won't work if you specify more than one aligner\n\
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
\n\
Output selection options (specify as many as you need):\n\
\n\
-multipr d  Write text phrase table(s) with multiple probabilities: for each\n\
            phrase pair, one or more 'backward' probabilities followed by one\n\
            or more 'forward' probabilities (more than one when multiple\n\
            smoothing methods are selected). d may be one of 'fwd', 'rev', or\n\
            'both', to select: output <name>.<lang1>2<lang2>, the reverse, or\n\
            both directions.\n\
-i          Write individual joint frequency phrase tables for each file pair\n\
            in the corpus.  The table for the ith pair is named <name>-i.pt\n\
            (where i is a 4-digit number).  This option suppresses and is\n\
            incompatible with all other output formats.\n\
-j          Write global joint frequency phrase table to stdout.\n\
-f1 freqs1  Write language 1 phrases and their frequencies to file freqs1.\n\
-f2 freqs2  Write language 2 phrases and their frequencies to file freqs2.\n\
";

// globals

typedef PhraseTableGen<Uint> PhraseTable;

static const char* const switches[] = {
   "v", "vv", "vs", "i", "j", "z", "prune1:", "prune1w:", "a:", "s:",
   "m:", "min:", "d:", "ali", "w:", "wf:", "wfvoc:", "1:", "2:", "ibm:",
   "hmm", "p0:", "up0:", "alpha:", "lambda:", "max-jump:",
   "anchor", "noanchor", "end-dist", "noend-dist",
   "p0_2:", "up0_2:", "alpha_2:", "lambda_2:", "max-jump_2:",
   "anchor_2", "noanchor_2", "end-dist_2", "noend-dist_2",
   "twist", "addsw", "o:", "f1:", "f2:",
   "lc1:", "lc2:",
   "num-file-args", // hidden option for gen-jpt-parallel.sh
   "file-args", // hidden option for gen-jpt-parallel.sh
   "multipr:", "tmtext", "giza"
};

static Uint verbose = 0;
static Uint smoothing_verbose = 0; // ugly ugly ugly ugly ugly ugly ugly ugly
static Uint prune1 = 0;
static Uint prune1w = 0;
static vector<string> align_methods;
static vector<string> smoothing_methods;
static bool indiv_tables = false;
static bool twist = false;
static bool add_single_word_phrases = false;
static bool joint = false;
static bool giza_alignment = false;
static Uint add_word_translations = 0;
static string add_word_trans_file = "";
static string add_word_trans_voc = "";
static Uint max_phrase_len1 = 4;
static Uint max_phrase_len2 = 4;
static Uint max_phraselen_diff = 4;
static Uint min_phrase_len1 = 1;
static Uint min_phrase_len2 = 1;
static bool allow_linkless_pairs = false;
static string model1, model2;
static string lang1("en");
static string lang2("fr");
static string lc1;
static string lc2;
static string name("phrases");
static string freqs1;
static string freqs2;
static Uint ibm_num = 42; // 42 means uninitialized - ARG will set its value.
static bool use_hmm = false;
static string multipr_output = "";
static bool compress_output = false;
static Uint first_file_arg = 2;

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

static void add_ibm1_translations(Uint lang, const TTable& tt, PhraseTable& pt,
				  Voc& src_word_voc, Voc& tgt_word_voc,
                                  ostream* os);


// arg processing

/// gen_phrase_table namespace.
/// Prevents global namespace polution in doxygen.
namespace genPhraseTable {

/// Specific argument processing class for gen_phrase_table program.
class ARG : public argProcessor {
private:
   Logging::logger m_logger;

public:
   /**
    * Default constructor.
    * @param argc  same as the main argc
    * @param argv  same as the main argv
    * @param alt_help  alternate help message
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
      string min_phrase_string;

      if (mp_arg_reader->getSwitch("v")) {verbose = 1; smoothing_verbose = 1;}
      if (mp_arg_reader->getSwitch("vv")) verbose = 2;
      if (mp_arg_reader->getSwitch("vs")) smoothing_verbose = 2;

      mp_arg_reader->testAndSet("a", align_methods);
      mp_arg_reader->testAndSet("s", smoothing_methods);
      mp_arg_reader->testAndSet("prune1", prune1);
      mp_arg_reader->testAndSet("prune1w", prune1w);
      mp_arg_reader->testAndSet("i", indiv_tables);
      mp_arg_reader->testAndSet("j", joint);
      mp_arg_reader->testAndSet("z", compress_output);
      mp_arg_reader->testAndSet("m", max_phrase_string);
      mp_arg_reader->testAndSet("min", min_phrase_string);
      mp_arg_reader->testAndSet("d", max_phraselen_diff);
      mp_arg_reader->testAndSet("ali", allow_linkless_pairs);
      mp_arg_reader->testAndSet("w", add_word_translations);
      mp_arg_reader->testAndSet("wf", add_word_trans_file);
      mp_arg_reader->testAndSet("wfvoc", add_word_trans_voc);
      mp_arg_reader->testAndSet("1", lang1);
      mp_arg_reader->testAndSet("2", lang2);
      mp_arg_reader->testAndSet("lc1", lc1);
      mp_arg_reader->testAndSet("lc2", lc2);
      mp_arg_reader->testAndSet("ibm", ibm_num);
      mp_arg_reader->testAndSet("hmm", use_hmm);

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

      mp_arg_reader->testAndSet("twist", twist);
      mp_arg_reader->testAndSet("addsw", add_single_word_phrases);
      mp_arg_reader->testAndSet("giza", giza_alignment);
      mp_arg_reader->testAndSet("o", name);
      mp_arg_reader->testAndSet("f1", freqs1);
      mp_arg_reader->testAndSet("f2", freqs2);
      mp_arg_reader->testAndSet("multipr", multipr_output);

      if (mp_arg_reader->getSwitch("tmtext"))
         error(ETFatal, "-tmtext is obsolete");

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

      if (max_phrase_string.length()) {
         vector<Uint> max_phrase_len;
         if (!split(max_phrase_string, max_phrase_len, ",") ||
             max_phrase_len.empty() || max_phrase_len.size() > 2)
            error(ETFatal, "bad argument for -m switch");
         max_phrase_len1 = max_phrase_len[0];
         max_phrase_len2 = max_phrase_len.size() == 2 ? max_phrase_len[1] : max_phrase_len[0];
      }

      if (min_phrase_string.length()) {
         vector<Uint> min_phrase_len;
         if (!split(min_phrase_string, min_phrase_len, ",") ||
             min_phrase_len.empty() || min_phrase_len.size() > 2)
            error(ETFatal, "bad argument for -min switch");
         min_phrase_len1 = min_phrase_len[0];
         min_phrase_len2 = min_phrase_len.size() == 2 ? min_phrase_len[1] : min_phrase_len[0];
      }

      if (multipr_output != "" && multipr_output != "fwd" && multipr_output != "rev" &&
          multipr_output != "both")
         error(ETFatal, "Unknown value for -multipr switch: %s", multipr_output.c_str());

      if (smoothing_methods.size() > 1 && multipr_output.empty()) {
         error(ETWarn, "Multiple smoothing methods are only used with -multipr output - ignoring all but %s",
               smoothing_methods[0].c_str());
         smoothing_methods.resize(1);
      }

      if (giza_alignment && align_methods.size() > 1)
        error(ETFatal, "Can't use -giza with multiple alignment methods");

      if (mp_arg_reader->getSwitch("file-args")) {
         vector<string> corpora;
         mp_arg_reader->getVars(first_file_arg, corpora);
         copy(corpora.begin(), corpora.end(), ostream_iterator<string>(cout, " "));
         cout << endl;
         exit(0);
      }

      if (mp_arg_reader->getSwitch("num-file-args")) {
         cout << (mp_arg_reader->numVars() - first_file_arg) << endl;
         exit(0);
      }

      if (!indiv_tables && !joint && multipr_output.empty())
         error(ETFatal, "No output requested");
   }
};

template <class T>
std::ostream& operator<<(std::ostream& os, const optional<T>& val) {
   if ( val )
      os << *val;
   else
      os << "default";
   return os;
}

}; // ends namespace genPhraseTable

using namespace genPhraseTable;

void doEverything(const char* prog_name, ARG& args);

int MAIN(argc, argv)
{
   printCopyright(2004, "gen_phrase_tables");
   static string alt_help =
      "--- word aligners ---\n" + WordAlignerFactory::help() +
      "\n--- phrase smoothers ---\n" + PhraseSmootherFactory<Uint>::help();
   ARG args(argc, argv, alt_help.c_str());
   doEverything(argv[0], args);
}
END_MAIN

void doEverything(const char* prog_name, ARG& args)
{
   string z_ext(compress_output ? ".gz" : "");

   if ((indiv_tables || joint) && lang1 >= lang2)
      error(ETWarn, "%s\n%s\n%s",
         "violating standard convention for joint phrasetables: language with",
         "lexicographically earlier name goes in left column. Fix by giving",
         "this language as the -1 argument.");

   if (align_methods.empty())
      align_methods.push_back("IBMOchAligner");
   if (smoothing_methods.empty())
      smoothing_methods.push_back("RFSmoother");

   if (max_phrase_len1 == 0) max_phrase_len1 = 10000000;
   if (max_phrase_len2 == 0) max_phrase_len2 = 10000000;
   if (min_phrase_len1 == 0) {
      min_phrase_len1 = 1;
      cerr << "minimal phrase length has to be at least 1 -> changing this!" << endl;
   }
   if (min_phrase_len2 == 0) {
      min_phrase_len2 = 1;
      cerr << "minimal phrase length has to be at least 1 -> changing this!" << endl;
   }
   if ( min_phrase_len1 > max_phrase_len1 || min_phrase_len2 > max_phrase_len2 ) {
      cerr << "Minimal phrase length is greater than the maximal one!" << endl
           << "lang1 : " << min_phrase_len1 << " " << max_phrase_len1 << endl
           << "lang2 : " << min_phrase_len2 << " " << max_phrase_len2 << endl;
      exit(1);
   }


   IBM1* ibm_1 = NULL;
   IBM1* ibm_2 = NULL;

   if (ibm_num == 0) {
      if (verbose) cerr << "**Not** loading IBM models" << endl;
   } else {
      if (use_hmm) {
         if (verbose) cerr << "Loading HMM models" << endl;
         ibm_1 = new HMMAligner(model1, p0, up0, alpha, lambda, anchor,
                                end_dist, max_jump);
         ibm_2 = new HMMAligner(model2, p0_2, up0_2, alpha_2, lambda_2, anchor_2,
                                end_dist_2, max_jump_2);
      } else if (ibm_num == 1) {
         if (verbose) cerr << "Loading IBM1 models" << endl;
         ibm_1 = new IBM1(model1);
         ibm_2 = new IBM1(model2);
      } else if (ibm_num == 2) {
         if (verbose) cerr << "Loading IBM2 models" << endl;
         ibm_1 = new IBM2(model1);
         ibm_2 = new IBM2(model2);
      } else
         error(ETFatal, "Invalid option: -ibm %d", ibm_num);
      if (verbose) cerr << "models loaded" << endl;
   }

   CaseMapStrings cms1(lc1.c_str());
   CaseMapStrings cms2(lc2.c_str());
   if (lc1 != "" && ibm_num != 0) {
      ibm_1->getTTable().setSrcCaseMapping(&cms1);
      ibm_2->getTTable().setTgtCaseMapping(&cms1);
   }
   if (lc2 != "" && ibm_num != 0) {
      ibm_1->getTTable().setTgtCaseMapping(&cms2);
      ibm_2->getTTable().setSrcCaseMapping(&cms2);
   }

   WordAlignerFactory* aligner_factory = 0;
   vector<WordAligner*> aligners;

   if (!giza_alignment) {
     aligner_factory = new WordAlignerFactory(ibm_1, ibm_2, verbose, twist,
                                              add_single_word_phrases, allow_linkless_pairs);
     for (Uint i = 0; i < align_methods.size(); ++i)
       aligners.push_back(aligner_factory->createAligner(align_methods[i]));
   }

   Voc extern_word_voc_1, extern_word_voc_2;
   if (add_word_trans_voc != "") {
      iMagicStream is1(add_word_trans_voc + ".1");
      extern_word_voc_1.read(is1);
      iMagicStream is2(add_word_trans_voc + ".2");
      extern_word_voc_2.read(is2);
   }

   PhraseTable pt;
   Voc word_voc_1, word_voc_2;

   string in_f1, in_f2;
   string alfile1, alfile2;
   Uint fpair = 0;

   GizaAlignmentFile* al_1 = NULL;
   GizaAlignmentFile* al_2 = NULL;

   WordAlignerStats stats;

   for (Uint arg = first_file_arg; arg+1 < args.numVars(); arg += 2) {

      string file1 = args.getVar(arg), file2 = args.getVar(arg+1);
      if (verbose)
         cerr << "reading " << file1 << "/" << file2 << endl;

      args.testAndSet(arg, "file1", in_f1);
      args.testAndSet(arg+1, "file2", in_f2);
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
         aligner_factory = new WordAlignerFactory(al_1, al_2, verbose, twist, add_single_word_phrases);

         aligners.clear();
         for (Uint i = 0; i < align_methods.size(); ++i)
            aligners.push_back(aligner_factory->createAligner(align_methods[i]));
      }

      Uint line_no = 0;
      string line1, line2;
      vector<string> toks1, toks2;
      vector< vector<Uint> > sets1;

      while (getline(in1, line1)) {
         if (!getline(in2, line2)) {
            error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
                  file1.c_str(), file2.c_str());
            break;
         }
         ++line_no;

         if (verbose > 1) cerr << "--- " << line_no << " ---" << endl;

         toks1.clear(); toks2.clear();
         split(line1, toks1);
         split(line2, toks2);

         for (Uint i = 0; i < toks1.size(); ++i)
            word_voc_1.add(toks1[i].c_str());
         for (Uint i = 0; i < toks2.size(); ++i)
            word_voc_2.add(toks2[i].c_str());

         if (verbose > 1)
            cerr << line1 << endl << line2 << endl;

         for (Uint i = 0; i < aligners.size(); ++i) {

            if (verbose > 1) cerr << "---" << align_methods[i] << "---" << endl;
            aligners[i]->align(toks1, toks2, sets1);
            if (verbose) stats.tally(sets1, toks1.size(), toks2.size());

            if (verbose > 1) {
               cerr << "---" << endl;
               aligner_factory->showAlignment(toks1, toks2, sets1);
               cerr << "---" << endl;
            }
            aligner_factory->addPhrases(toks1, toks2, sets1,
                                        max_phrase_len1, max_phrase_len2,
                                        max_phraselen_diff,
                                        min_phrase_len1, min_phrase_len2,
                                        pt);
         }
         if (verbose > 1) cerr << endl; // end of block
         if (verbose == 1 && line_no % 1000 == 0)
            cerr << "line: " << line_no << endl;
      }

      if (getline(in2, line2))
         error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
               file1.c_str(), file2.c_str());

      if (indiv_tables) {

         if (add_word_translations && ibm_1 && ibm_2) {
            if (verbose) cerr << "ADDING IBM1 translations for untranslated words:" << endl;
            add_ibm1_translations(1, ibm_1->getTTable(), pt, word_voc_1,
                                  add_word_trans_voc == "" ? word_voc_2 : extern_word_voc_2, NULL);
            add_ibm1_translations(2, ibm_2->getTTable(), pt, word_voc_2,
                                  add_word_trans_voc == "" ? word_voc_1 : extern_word_voc_1, NULL);
            word_voc_1.clear();
            word_voc_2.clear();
         }

         ostringstream fname;
         fname << name << "-" << std::setw(4) << setfill('0') << fpair
               << ".pt" << z_ext;
         oSafeMagicStream ofs(fname.str());
         pt.dump_joint_freqs(ofs);
         pt.clear();
      }

      ++fpair;
   }

   pt.remap_psep();

   if (verbose) stats.display(lang1, lang2, cerr);

   if (add_word_translations && ibm_1 && ibm_2) {
      ostream* os1 = NULL;
      ostream* os2 = NULL;
      if (add_word_trans_file != "") {
         os1 = new oSafeMagicStream(add_word_trans_file + ".1");
         os2 = new oSafeMagicStream(add_word_trans_file + ".2");
      }
      if (verbose) {
         cerr << "ADDING IBM1 translations for untranslated words";
         if (os1) cerr << " (writing to " << add_word_trans_file << ".[12])";
         cerr << endl;
      }
      add_ibm1_translations(1, ibm_1->getTTable(), pt, word_voc_1,
                            add_word_trans_voc == "" ? word_voc_2 : extern_word_voc_2, os1);
      add_ibm1_translations(2, ibm_2->getTTable(), pt, word_voc_2,
                            add_word_trans_voc == "" ? word_voc_1 : extern_word_voc_1, os2);
      if (os1) delete os1;
      if (os2) delete os2;
   }

   if (prune1 || prune1w) {
      if (verbose) {
         cerr << "pruning to best ";
         if (prune1)            cerr << prune1;
         if (prune1 && prune1w) cerr << "+";
         if (prune1w)           cerr << prune1w << "*numwords";
         cerr << " translations" << endl;
      }
      pt.pruneLang2GivenLang1(prune1, prune1w);
   }

   if ( !indiv_tables ) {
      if ( multipr_output != "" ) {
         if (verbose) cerr << "smoothing:" << endl;

         PhraseSmootherFactory<Uint> smoother_factory(&pt, ibm_1, ibm_2, smoothing_verbose);
         vector< PhraseSmoother<Uint>* > smoothers;
         smoother_factory.createSmoothersAndTally(smoothers, smoothing_methods);

         if (multipr_output == "fwd" || multipr_output == "both") {
            string filename = name + "." + lang1 + "2" + lang2 + z_ext;
            if (verbose) cerr << "Writing " << filename << endl;
            oSafeMagicStream ofs(filename);
            dumpMultiProb(ofs, 1, pt, smoothers, verbose);
         }
         if (multipr_output == "rev" || multipr_output == "both") {
            string filename = name + "." + lang2 + "2" + lang1 + z_ext;
            if (verbose) cerr << "Writing " << filename << endl;
            oSafeMagicStream ofs(filename);
            dumpMultiProb(ofs, 2, pt, smoothers, verbose);
         }
      }
      if (joint)
         pt.dump_joint_freqs(cout);
      if (freqs1 != "") {
         oSafeMagicStream ofs(freqs1);
         pt.dump_freqs_lang1(ofs);
      }
      if (freqs2 != "") {
         oSafeMagicStream ofs(freqs2);
         pt.dump_freqs_lang2(ofs);
      }
   }

   if (verbose) cerr << "done" << endl;

}

static const string& remap(const string& s) {
   if (s == PhraseTableBase::psep)
      return PhraseTableBase::psep_replacement;
   else
      return s;
}

// Lang is source language for tt: 1 or 2.
// If os is non-NULL, the pairs get written to os instead of inserted into the
// phrasetable.

void add_ibm1_translations(Uint lang, const TTable& tt, PhraseTable& pt,
                           Voc& src_word_voc, Voc& tgt_word_voc,
                           ostream* os)
{
   vector<string> words, trans;
   vector<float> probs;

   tt.getSourceVoc(words);
   for (vector<string>::const_iterator p = words.begin(); p != words.end(); ++p) {
      bool in_phrase_voc = lang == 1 ? pt.inVoc1(*p) : pt.inVoc2(*p);
      if (!in_phrase_voc && src_word_voc.index(p->c_str()) != src_word_voc.size()) {
         tt.getSourceDistnByDecrProb(*p, trans, probs);
         Uint num_added = 0;
         for (Uint i = 0; num_added < add_word_translations && i < trans.size(); ++i) {
            if (tgt_word_voc.index(trans[i].c_str()) == tgt_word_voc.size()) {
               continue;
            }
            if (lang == 1) {
               if (os) (*os) << remap(*p) << " ||| " << remap(trans[i]) << " ||| " << 1 << endl;
               else pt.addPhrasePair(p, p+1, trans.begin()+i, trans.begin()+i+1);
            } else {
               if (os) (*os) << remap(trans[i]) << " ||| " << remap(*p) << " ||| " << 1 << endl;
               else pt.addPhrasePair(trans.begin()+i, trans.begin()+i+1, p, p+1);
            }
            ++num_added;
            if (verbose > 1) cerr << *p << "/" << trans[i] << endl;
         }
      }
   }
}

// vim:sw=3:
