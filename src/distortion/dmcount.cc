/**
 * @author George Foster & Samuel Larkin
 * @file dmtrain
 * @brief Make counts for a Moishe-style distortion model
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */
#include <boost/optional/optional.hpp>
#include <iostream>
#include <fstream>
#include <argProcessor.h>
#include <exception_dump.h>
#include <file_utils.h>
#include <ibm.h>
#include <hmm_aligner.h>
#include <phrase_table.h>
#include "word_align.h"
#include "dmstruct.h"
#include "printCopyright.h"
#include "phrase_pair_extractor.h"
#include "distortion_algorithm.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
dmcount [options] ibm_lang2_given_lang1 ibm_lang1_given_lang2 \n\
                  file1_lang1 file1_lang2 ... fileN_lang1 fileN_lang2\n\
\n\
Count reordering events for a lexicalized distortion model over a set of\n\
line-aligned parallel files, using given IBM/HMM models to perform word\n\
alignment for phrase extraction. Lang1 is assumed to be the source unless the\n\
-r switch is supplied. The output distortion counts are written to stdout, in\n\
the format:  'l1-phrase ||| l2-phrase ||| pm ps pd nm ns nd', where the 'p'\n\
counts pertain to the previous target phrase, and the 'n' counts pertain to the\n\
next target phrase; and 'm', 's', and 'd' designate monotonic, swap, and\n\
discontinuous orientations.\n\
\n\
Options:\n\
\n\
-h     Display this help message and quit.\n\
-H     List available word-alignment methods and quit.\n\
-v     Write progress reports to cerr.\n\
-r     Reverse the roles of the languages: assume lang2 is the source. This is\n\
       equivalent to swapping the positions of all language-dependent arguments.\n\
-a     Word-alignment method and optional args. Use -H for list of methods.\n\
       Multiple methods may be specified by using -a repeatedly. [IBMOchAligner]\n\
-w     Add <nw> best IBM1 translations for src and tgt words that occur in the\n\
       given files but don't have translations in phrase table. Each word pair is\n\
       added with 0 counts, so non-zero smoothing parameters should be used in\n\
       the subsequent dmestm step.\n\
-m     Maximum phrase length. <max> can be in the form 'len1,len2' to specify\n\
       lengths for each language, or 'len' to use one length for both\n\
       languages. 0 means no limit. [4]\n\
-min   Minimum phrase length. <min> can be in the form 'len1,len2' to specify\n\
       lengths for each language, or 'len' to use one length for both\n\
       languages. Has to be at least 1. [1]\n\
-d     Max permissible difference in number of words between source and\n\
       target phrases. [4]\n\
-ibm   Type of ibm models given to the -ibm_* switches: 1, 2, or hmm. This\n\
       may be used to force an HMM ttable to be used as an IBM1, for example.\n\
       [determine type of model automatically from filename]\n\
-lc1   Do lowercase mapping of lang 1 words to match IBM/HMM models, using\n\
       locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
-lc2   Do lowercase mapping of lang 2 words to match IBM/HMM models, using\n\
       locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
-giza  IBM-style alignments are to be read from files in GIZA++ format,\n\
       rather than computed at run-time; corresponding alignment files \n\
       should be specified after each pair of text files, like this: \n\
       fileN_lang1 fileN_lang2 align_1_to_2 align_2_to_1...\n\
       Notes:\n\
        - you still need to provide IBM models as arguments\n\
        - this currently only works with IBMOchAligner\n\
        - this won't work if you specify more than one aligner\n\
-ext   SRI-style alignments are to be read from files in SRI format,\n\
       rather than computed at run-time; corresponding alignment files \n\
       should be specified after each pair of text files, like this: \n\
       fileN_lang1 fileN_lang2 align_1_to_2...\n\
       Notes:\n\
        - this replace the hack: -a 'ExternalAligner model' -ibm 1 /dev/null /dev/null\n\
        - you DON'T need to provide IBM models as arguments\n\
        - this implies the ExternalAligner\n\
        - this won't work if you specify more than one aligner\n\
-hier  Extract hierarchical LDM counts rather than word-based LDM counts.\n\
\n\
HMM only options:\n\
\n\
       Some HMM parameters can be modified post loading. See gen_phrase_tables -h\n\
       for details.\n\
";

static const string alt_help = WordAlignerFactory::help();

// Switches
const char* const switches[] = {
   "v", "r", "s", "a:", "w:", "m:", "min:", "d:", "ibm:",
   "lc1:", "lc2:", "giza", "hier",
   "p0:", "up0:", "alpha:", "lambda:", "max-jump:",
   "anchor", "noanchor", "end-dist", "noend-dist",
   "p0_2:", "up0_2:", "alpha_2:", "lambda_2:", "max-jump_2:",
   "anchor_2", "noanchor_2", "end-dist_2", "noend-dist_2",
   "ext",
};

static bool rev = false;
static Uint add_word_translations = 0;
static string lc1;
static string lc2;
static bool giza_alignment = false;
static bool hierarchical = false;
static bool externalAlignerMode = false;

// HMM post-load parameters (intentionally left uninitialized).

// Main arguments

static vector<string> textfiles;

// Most parameters are in now ppe, with their defaults set in
// PhrasePairExtractor::PhrasePairExtractor() (see phrase_pair_extractor.h).
static PhrasePairExtractor ppe;

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

            string ibmtype;
            string max_phrase_string;
            string min_phrase_string;
            vector<string> verboses;

            mp_arg_reader->testAndSet("ext", externalAlignerMode);
            if (externalAlignerMode) {
               ppe.ibm_num = 0;
            }

            mp_arg_reader->testAndSet("v", verboses);
            mp_arg_reader->testAndSet("r", rev);
            mp_arg_reader->testAndSet("a", ppe.align_methods);
            mp_arg_reader->testAndSet("w", ppe.add_word_translations);
            mp_arg_reader->testAndSet("m", ppe.max_phrase_string);
            mp_arg_reader->testAndSet("min", ppe.min_phrase_string);
            mp_arg_reader->testAndSet("d", ppe.max_phraselen_diff);
            mp_arg_reader->testAndSet("ibm", ibmtype);
            mp_arg_reader->testAndSet("lc1", lc1);
            mp_arg_reader->testAndSet("lc2", lc2);
            mp_arg_reader->testAndSet("giza", giza_alignment);
            mp_arg_reader->testAndSet("hier", hierarchical);

            mp_arg_reader->testAndSet("p0", ppe.p0);
            mp_arg_reader->testAndSet("up0", ppe.up0);
            mp_arg_reader->testAndSet("alpha", ppe.alpha);
            mp_arg_reader->testAndSet("lambda", ppe.lambda);
            mp_arg_reader->testAndSet("max-jump", ppe.max_jump);
            mp_arg_reader->testAndSetOrReset("anchor", "noanchor", ppe.anchor);
            mp_arg_reader->testAndSetOrReset("end-dist", "noend-dist", ppe.end_dist);

            mp_arg_reader->testAndSet("p0_2", ppe.p0_2);
            mp_arg_reader->testAndSet("up0_2", ppe.up0_2);
            mp_arg_reader->testAndSet("alpha_2", ppe.alpha_2);
            mp_arg_reader->testAndSet("lambda_2", ppe.lambda_2);
            mp_arg_reader->testAndSet("max-jump_2", ppe.max_jump_2);
            mp_arg_reader->testAndSetOrReset("anchor_2", "noanchor_2", ppe.anchor_2);
            mp_arg_reader->testAndSetOrReset("end-dist_2", "noend-dist_2", ppe.end_dist_2);

            ppe.verbose = verboses.size();

            if (giza_alignment)
               error(ETFatal, "GIZA alignment reading not yet implemented");

            if (externalAlignerMode and !add_word_translations) {
               mp_arg_reader->getVars(0, textfiles);
               ibmtype = "0";
            }
            else {
               ppe.model1 = mp_arg_reader->getVar(0);
               ppe.model2 = mp_arg_reader->getVar(1);
               mp_arg_reader->getVars(2, textfiles);
            }

            if (ibmtype == "") {
               if (check_if_exists(HMMAligner::distParamFileName(ppe.model1)) &&
                     check_if_exists(HMMAligner::distParamFileName(ppe.model2)))
                  ibmtype = "hmm";
               else if (check_if_exists(IBM2::posParamFileName(ppe.model1)) &&
                     check_if_exists(IBM2::posParamFileName(ppe.model2)))
                  ibmtype = "2";
               else
                  ibmtype = "1";
            }

            if (ibmtype == "hmm")
               ppe.use_hmm = true;
            else if (ibmtype == "0")
               ppe.ibm_num = 0;
            else if (ibmtype == "1")
               ppe.ibm_num = 1;
            else if (ibmtype == "2")
               ppe.ibm_num = 2;
            else
               error(ETFatal, "Unknown ibmtype.");
               

            if (rev) {
               swap(ppe.max_phrase_len1, ppe.max_phrase_len2);
               swap(ppe.min_phrase_len1, ppe.min_phrase_len2);
               swap(lc1, lc2);
               swap(ppe.p0, ppe.p0_2);
               swap(ppe.up0, ppe.up0_2);
               swap(ppe.alpha, ppe.alpha_2);
               swap(ppe.lambda, ppe.lambda_2);
               swap(ppe.anchor, ppe.anchor_2);
               swap(ppe.end_dist, ppe.end_dist_2);
               swap(ppe.max_jump, ppe.max_jump_2);
               swap(ppe.model1, ppe.model2);
               if (externalAlignerMode)
                  error(ETFatal, "reverse mode not supported with -ext");
               else
                  for (Uint i = 0; i+1 < textfiles.size(); i += 2)
                     swap(textfiles[i], textfiles[i+1]);
            }

            ppe.checkArgs();
         }
   };
};


using namespace genPhraseTable;


template<class ALGO>
void doEverything(ARG& args) {
   ppe.loadModels(!externalAlignerMode);

   if (add_word_translations && (ppe.ibm_1 == NULL || ppe.ibm_2 == NULL)) {
      error(ETFatal, "You must provide IBM/HMM model when using -w.");
   }

   CaseMapStrings cms1(lc1.c_str());
   CaseMapStrings cms2(lc2.c_str());
   if (lc1 != "") {
      ppe.ibm_1->getTTable().setSrcCaseMapping(&cms1);
      ppe.ibm_2->getTTable().setTgtCaseMapping(&cms1);
   }
   if (lc2 != "") {
      ppe.ibm_1->getTTable().setTgtCaseMapping(&cms2);
      ppe.ibm_2->getTTable().setSrcCaseMapping(&cms2);
   }

   PhraseTableGen<DistortionCount> pt;
   Voc word_voc_1, word_voc_2;
   ALGO algo;

   for (Uint fno = 0; fno+1 < textfiles.size(); fno += 2) {

      const string file1 = textfiles[fno];
      const string file2 = textfiles[fno+1];
      if (externalAlignerMode) {
         string alfile;
         fno+=1;
         if (fno+1 >= args.numVars())
            error(ETFatal, "Missing arguments: alignment files");
         args.testAndSet(fno+1, "alfile", alfile);
         if (ppe.verbose)
            cerr << "reading aligment files " << alfile << endl;

         IBM1* model = NULL;
         if (ppe.aligner_factory) delete ppe.aligner_factory;
         ppe.aligner_factory = new WordAlignerFactory(
               model, model, ppe.verbose, ppe.twist, ppe.add_single_word_phrases);

         ppe.align_methods.clear();
         ppe.align_methods.push_back("ExternalAligner " + alfile);

         ppe.aligners.clear();
         ppe.aligners.push_back(ppe.aligner_factory->createAligner("ExternalAligner", alfile));
      }

      ppe.alignFilePair(file1,
	    file2,
	    pt,
	    algo,
	    word_voc_1,
	    word_voc_2);

   }

   if (add_word_translations && ppe.ibm_1 && ppe.ibm_2) {
      if (ppe.verbose) cerr << "ADDING IBM1 translations for untranslated words:" << endl;
      ppe.add_ibm1_translations(1, pt, word_voc_1, word_voc_2);
      ppe.add_ibm1_translations(2, pt, word_voc_2, word_voc_1);
   }

   pt.dump_joint_freqs(cout);
}

// main

int MAIN(argc, argv)
{
   printCopyright(2009, "dmcount");
   ARG args(argc, argv, alt_help.c_str());

   if (hierarchical)
      doEverything<hier_ldm_count>(args);
   else
      doEverything<word_ldm_count>(args);
}
END_MAIN

