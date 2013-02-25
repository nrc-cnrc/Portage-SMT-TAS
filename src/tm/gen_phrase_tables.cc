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
#include "ibm.h"
#include "hmm_aligner.h"
#include "phrase_table.h"
#include "word_align.h"
#include "phrase_smoother.h"
#include "phrase_smoother_cc.h"
#include "phrase_table_writer.h"
#include "phrase_pair_extractor.h"

using namespace Portage;

static char help_message[] = "\n\
gen_phrase_tables [Options]\n\
                  ibm-model_lang2_given_lang1 ibm-model_lang1_given_lang2\n\
                  file1_lang1 file1_lang2 [ ... fileN_lang1 fileN_lang2]\n\
\n\
gen_phrase_tables -giza -ibm 0 [Options]\n\
                  file1_lang1 file1_lang2 align1_1_to_2 align1_2_to_1\n\
                  [ ... fileN_lang1 fileN_lang2 alignN_1_to_2 alignN_2_to_1]\n\
\n\
gen_phrase_tables -ext [Options]\n\
                  file1_lang1 file1_lang2 align1_1_to_2\n\
                  [ ... fileN_lang1 fileN_lang2 alignN_1_to_2]\n\
\n\
Generate phrase translation tables from either:\n\
\n\
1) default: IBM models and set of line-aligned text files. The models should be\n\
   for p(lang2|lang1) and p(lang1|lang2) respectively: <model1> should contain\n\
   entries of the form 'lang1 lang2 prob', and <model2> the reverse.\n\
2) -giza: a set of line-aligned text files, each with a pair of giza-format\n\
   bi-directional word-alignment files (see -giza below for details).\n\
3) -ext: a set of line-aligned text files, each with a symmetrized sri-format\n\
   alignment file (see -ext below for details).\n\
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
-addsw Add single-word phrase pairs for each alignment link [don't]\n\
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
-giza  IBM-style alignments are to be read from files in the old GIZA++ format,\n\
       rather than computed at run-time.  Use align-words -giza2 and then\n\
       gen_phrase_table -ext if you have files in the new giza format.\n\
       Corresponding alignment files should be specified after each pair of\n\
       text files:\n\
       ... fileN_lang1 fileN_lang2 alignN_1_to_2 alignN_2_to_1 ...\n\
       Notes:\n\
        - you must specify -ibm 0 or provide IBM models as arguments,\n\
        - only works with IBMOchAligner,\n\
        - you can only specify one aligner.\n\
-ext   SRI-style alignments are to be read from files in SRI format, e.g., as\n\
       produced by align-words -o sri, rather than computed at run-time;\n\
       corresponding symmetrized alignment files should be specified after each\n\
       pair of text files:\n\
       ... fileN_lang1 fileN_lang2 alignN_1_to_2 ...\n\
       Notes:\n\
        - this replace the hack: -a 'ExternalAligner align' -ibm 1 /dev/null /dev/null,\n\
        - IBM models should not be provided as arguments,\n\
        - implies -a ExternalAligner; you cannot use -a to change this.\n\
\n\
HMM only options:\n\
       By default, all HMM parameters are read from the model file. However,\n\
       these options can be used to override the values in the model file:\n\
          -p0 -up0 -alpha -lambda -anchor -end-dist -max-jump\n\
       Boolean options -anchor and -end-dist can be reset using -no<option>.\n\
       A parallel set of options -p0_2, -up0_2, etc, applies to the\n\
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
-write-al A Show alignment information in multi-prob and joint frequency phrase\n\
            tables.  If A is \"top\", the most frequent alignment is written in\n\
            the \"green\" format preceeded by \"a=\", at the end of the line.\n\
            If A is \"all\", all observed alignments are shown with counts.\n\
            \"none\" is the same as not specifying -write-al.  [none]\n\
-write-count Show the joint count in the 3rd column of multi-prob phrase tables\n\
            in the format c=<count>.  [don't]\n\
-f1 freqs1  Write language 1 phrases and their frequencies to file freqs1.\n\
-f2 freqs2  Write language 2 phrases and their frequencies to file freqs2.\n\
";

// globals

static const char* const switches[] = {
   "v", "vv", "vs", "i", "j", "z", "prune1:", "prune1w:", "a:", "s:",
   "m:", "min:", "d:", "ali", "w:", "wf:", "wfvoc:", "1:", "2:", "ibm:",
   "hmm", "p0:", "up0:", "alpha:", "lambda:", "max-jump:",
   "anchor", "noanchor", "end-dist", "noend-dist",
   "p0_2:", "up0_2:", "alpha_2:", "lambda_2:", "max-jump_2:",
   "anchor_2", "noanchor_2", "end-dist_2", "noend-dist_2",
   "twist", "addsw", "o:", "f1:", "f2:",
   "write-al:", "write-count",
   "lc1:", "lc2:",
   "num-file-args", // hidden option for gen-jpt-parallel.sh
   "file-args", // hidden option for gen-jpt-parallel.sh
   "multipr:", "tmtext", "giza", "ext"
};

static Uint smoothing_verbose = 0; // ugly ugly ugly ugly ugly ugly ugly ugly
static Uint prune1 = 0;
static Uint prune1w = 0;
static vector<string> smoothing_methods;
static bool indiv_tables = false;
static bool joint = false;
static bool giza_alignment = false;
static string add_word_trans_file = "";
static string add_word_trans_voc = "";
static string lang1("en");
static string lang2("fr");
static string lc1;
static string lc2;
static string name("phrases");
static string freqs1;
static string freqs2;
static string multipr_output = "";
static string store_alignment_option = "";
static bool write_count = false;
static bool compress_output = false;
static Uint first_file_arg = 2;
static bool externalAlignerMode = false;

// Most parameters are in now ppe, with their defaults set in
// PhrasePairExtractor::PhrasePairExtractor() (see phrase_pair_extractor.h).
static PhrasePairExtractor ppe;

// arg processing

/// gen_phrase_table namespace.
/// Prevents global namespace polution in doxygen.
namespace genPhraseTable {

/// Specific argument processing class for gen_phrase_table program.
class ARG : public argProcessor {
public:
   /**
    * Default constructor.
    * @param argc  same as the main argc
    * @param argv  same as the main argv
    * @param alt_help  alternate help message
    */
   ARG(const int argc, const char* const argv[], const char* alt_help) :
      argProcessor(ARRAY_SIZE(switches), switches, 1, -1, help_message, "-h", false, alt_help, "-H")
   {
      argProcessor::processArgs(argc, argv);
   }

   /// See argProcessor::processArgs()
   virtual void processArgs() {
      if (mp_arg_reader->getSwitch("v")) {ppe.verbose = 1; smoothing_verbose = 1;}
      if (mp_arg_reader->getSwitch("vv")) ppe.verbose = 2;
      if (mp_arg_reader->getSwitch("vs")) smoothing_verbose = 2;

      mp_arg_reader->testAndSet("a", ppe.align_methods);

      mp_arg_reader->testAndSet("ext", externalAlignerMode);
      if (externalAlignerMode) {
         if (!ppe.align_methods.empty())
            error(ETFatal, "-a and -ext cannot be used together.");
	 // When using the external Aligner Mode, we don't require word alignment models.
         ppe.ibm_num = 0;
         first_file_arg = 0;
         ppe.align_methods.clear();
         ppe.align_methods.push_back("ExternalAligner");
      }

      mp_arg_reader->testAndSet("s", smoothing_methods);
      mp_arg_reader->testAndSet("prune1", prune1);
      mp_arg_reader->testAndSet("prune1w", prune1w);
      mp_arg_reader->testAndSet("i", indiv_tables);
      mp_arg_reader->testAndSet("j", joint);
      mp_arg_reader->testAndSet("z", compress_output);
      mp_arg_reader->testAndSet("m", ppe.max_phrase_string);
      mp_arg_reader->testAndSet("min", ppe.min_phrase_string);
      mp_arg_reader->testAndSet("d", ppe.max_phraselen_diff);
      mp_arg_reader->testAndSet("ali", ppe.allow_linkless_pairs);
      mp_arg_reader->testAndSet("w", ppe.add_word_translations);
      mp_arg_reader->testAndSet("wf", add_word_trans_file);
      mp_arg_reader->testAndSet("wfvoc", add_word_trans_voc);
      mp_arg_reader->testAndSet("1", lang1);
      mp_arg_reader->testAndSet("2", lang2);
      mp_arg_reader->testAndSet("lc1", lc1);
      mp_arg_reader->testAndSet("lc2", lc2);
      mp_arg_reader->testAndSet("ibm", ppe.ibm_num);
      mp_arg_reader->testAndSet("hmm", ppe.use_hmm);

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

      mp_arg_reader->testAndSet("twist", ppe.twist);
      mp_arg_reader->testAndSet("addsw", ppe.add_single_word_phrases);
      mp_arg_reader->testAndSet("giza", giza_alignment);
      mp_arg_reader->testAndSet("o", name);
      mp_arg_reader->testAndSet("f1", freqs1);
      mp_arg_reader->testAndSet("f2", freqs2);
      mp_arg_reader->testAndSet("multipr", multipr_output);
      mp_arg_reader->testAndSet("write-al", store_alignment_option);
      mp_arg_reader->testAndSet("write-count", write_count);

      if (mp_arg_reader->getSwitch("tmtext"))
         error(ETFatal, "-tmtext is obsolete");

      if (ppe.ibm_num == 0 and !ppe.use_hmm) {
         if (!giza_alignment && !externalAlignerMode)
            error(ETFatal, "Can't use -ibm=0 trick unless -giza or -ext is used");
         first_file_arg = 0;
      }
      else {
         first_file_arg = 2;
         mp_arg_reader->testAndSet(0, "model1", ppe.model1);
         mp_arg_reader->testAndSet(1, "model2", ppe.model2);
      }

      // We must have word alginment models to be able to add word translations.
      if (ppe.add_word_translations > 0) {
         first_file_arg = 2;
         mp_arg_reader->testAndSet(0, "model1", ppe.model1);
         mp_arg_reader->testAndSet(1, "model2", ppe.model2);
	 // Let's automatically figure out the model's type.
         // NOTE 42 is the uninitialized value.
	 if (ppe.ibm_num == 0) ppe.ibm_num = 42;
      }

      ppe.checkArgs();

      if (multipr_output != "" && multipr_output != "fwd" && multipr_output != "rev" &&
          multipr_output != "both")
         error(ETFatal, "Unknown value for -multipr switch: %s", multipr_output.c_str());

      if (smoothing_methods.size() > 1 && multipr_output.empty()) {
         error(ETWarn, "Multiple smoothing methods are only used with -multipr output - ignoring all but %s",
               smoothing_methods[0].c_str());
         smoothing_methods.resize(1);
      }

      if (giza_alignment && ppe.align_methods.size() > 1)
        error(ETFatal, "Can't use -giza with multiple alignment methods");

      if (externalAlignerMode) {
         if (ppe.align_methods.size() > 1)
            error(ETFatal, "Can't use -ext with multiple alignment methods!");
         ppe.align_methods.clear();
      }

      if (ppe.add_word_translations > 0 and (ppe.model1.empty() || ppe.model2.empty())) {
         error(ETFatal, "You need to provide IBM or HMM when using -w!");
      }

      if (!store_alignment_option.empty()) {
         if ( store_alignment_option == "top" ) {
            // display_alignments==1 means display only top one, without freq
            ppe.display_alignments = 1;
         } else if ( store_alignment_option == "all" ) {
            // display_alignments==2 means display all alignments with freq
            ppe.display_alignments = 2;
         } else if ( store_alignment_option == "none" ) {
            ppe.display_alignments = 0;
         } else {
            error(ETFatal, "Invalid -write-al value: %s; expected top or all.",
                  store_alignment_option.c_str());
         }
      }

      if (ppe.display_alignments && multipr_output.empty() && !joint)
         error(ETFatal, "-write-al requires -multipr or -j");

      if (write_count && multipr_output.empty())
         error(ETFatal, "-write-count requires -multipr");

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

/**
 * A functor to extract phrase pairs once a sentence pair is aligned by the
 * phrase pair extractor's alignFilePair method.
 */
struct ExtractPhrasePairs {
   WordAlignerStats* stats;
   ExtractPhrasePairs(WordAlignerStats* stats)
      : stats(stats)
   {}
   void operator()(
	 const vector<string>& toks1,
	 const vector<string>& toks2,
	 vector< vector<Uint> >& sets1,
	 PhraseTableUint& pt,
	 PhrasePairExtractor& ppe) {
      if (stats) stats->tally(sets1, toks1.size(), toks2.size());
      assert(ppe.aligner_factory);
      ppe.aligner_factory->addPhrases(toks1, toks2, sets1,
                                  ppe.max_phrase_len1, ppe.max_phrase_len2,
                                  ppe.max_phraselen_diff,
                                  ppe.min_phrase_len1, ppe.min_phrase_len2,
                                  pt.getPhraseAdder(toks1, toks2, 1),
                                  ppe.display_alignments);
   }

};

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
   const string z_ext(compress_output ? ".gz" : "");

   if ((indiv_tables || joint) && lang1 >= lang2)
      error(ETWarn, "%s\n%s\n%s",
         "violating standard convention for joint phrasetables: language with",
         "lexicographically earlier name goes in left column. Fix by giving",
         "this language as the -1 argument.");

   if (smoothing_methods.empty())
      smoothing_methods.push_back("RFSmoother");

   if (ppe.ibm_num == 0 and !ppe.use_hmm) {
      if (ppe.verbose) cerr << "**Not** loading IBM models" << endl;
   } else {
      ppe.loadModels();
   }

   if (ppe.verbose > 1) ppe.dumpParameters();

   CaseMapStrings cms1(lc1.c_str());
   CaseMapStrings cms2(lc2.c_str());
   if (lc1 != "" && ppe.ibm_num != 0) {
      ppe.ibm_1->getTTable().setSrcCaseMapping(&cms1);
      ppe.ibm_2->getTTable().setTgtCaseMapping(&cms1);
   }
   if (lc2 != "" && ppe.ibm_num != 0) {
      ppe.ibm_1->getTTable().setTgtCaseMapping(&cms2);
      ppe.ibm_2->getTTable().setSrcCaseMapping(&cms2);
   }

   Voc extern_word_voc_1, extern_word_voc_2;
   if (add_word_trans_voc != "") {
      iMagicStream is1(add_word_trans_voc + ".1");
      extern_word_voc_1.read(is1);
      iMagicStream is2(add_word_trans_voc + ".2");
      extern_word_voc_2.read(is2);
   }

   PhraseTableUint pt;
   Voc word_voc_1, word_voc_2;

   string alfile1, alfile2;
   Uint fpair = 0;

   GizaAlignmentFile* al_1 = NULL;
   GizaAlignmentFile* al_2 = NULL;

   WordAlignerStats stats;
   WordAlignerStats* p_stats = ppe.verbose ? &stats : NULL;
   ExtractPhrasePairs algo(p_stats);

   for (Uint arg = first_file_arg; arg+1 < args.numVars(); arg += 2) {

      const string file1 = args.getVar(arg);
      const string file2 = args.getVar(arg+1);

      if (externalAlignerMode) {
         arg+=1;
         if (arg+1 >= args.numVars())
            error(ETFatal, "Missing arguments: alignment files");
         args.testAndSet(arg+1, "alfile1", alfile1);
         if (ppe.verbose)
            cerr << "reading aligment files " << alfile1 << endl;

         IBM1* model = NULL;
         if (ppe.aligner_factory) delete ppe.aligner_factory;
         ppe.aligner_factory = new WordAlignerFactory(
               model, model, ppe.verbose, ppe.twist, ppe.add_single_word_phrases);

         ppe.align_methods.clear();
         ppe.align_methods.push_back("ExternalAligner " + alfile1);

         ppe.aligners.clear();
         ppe.aligners.push_back(ppe.aligner_factory->createAligner("ExternalAligner", alfile1));
      }

      if (giza_alignment) {
         arg+=2;
         if (arg+1 >= args.numVars())
            error(ETFatal, "Missing arguments: alignment files");
         args.testAndSet(arg, "alfile1", alfile1);
         args.testAndSet(arg+1, "alfile2", alfile2);
         if (ppe.verbose)
            cerr << "reading aligment files " << alfile1 << "/" << alfile2 << endl;
         if (al_1) delete al_1;
         al_1 = new GizaAlignmentFile(alfile1);
         if (al_2) delete al_2;
         al_2 = new GizaAlignmentFile(alfile2);
         if (ppe.aligner_factory) delete ppe.aligner_factory;
         ppe.aligner_factory = new WordAlignerFactory(
               al_1, al_2, ppe.verbose, ppe.twist, ppe.add_single_word_phrases);

         ppe.aligners.clear();
         for (Uint i = 0; i < ppe.align_methods.size(); ++i)
            ppe.aligners.push_back(ppe.aligner_factory->createAligner(ppe.align_methods[i]));
      }

      ppe.alignFilePair(file1,
	    file2,
	    pt,
	    algo,
	    word_voc_1,
	    word_voc_2);


      if (indiv_tables) {

         if (ppe.add_word_translations && ppe.ibm_1 && ppe.ibm_2) {
            if (ppe.verbose) cerr << "ADDING IBM1 translations for untranslated words in lang 1:" << endl;
            ppe.add_ibm1_translations(1, pt, word_voc_1, add_word_trans_voc == "" ? word_voc_2 : extern_word_voc_2);
            if (ppe.verbose) cerr << "ADDING IBM1 translations for untranslated words in lang 2:" << endl;
            ppe.add_ibm1_translations(2, pt, word_voc_2, add_word_trans_voc == "" ? word_voc_1 : extern_word_voc_1);
            word_voc_1.clear();
            word_voc_2.clear();
         }

         ostringstream fname;
         fname << name << "-" << std::setw(4) << setfill('0') << fpair
               << ".pt" << z_ext;
         oSafeMagicStream ofs(fname.str());
         pt.dump_joint_freqs(ofs, 0, false, false, ppe.display_alignments);
         pt.clear();
      }

      ++fpair;
   }

   pt.remap_psep();

   if (ppe.verbose) stats.display(lang1, lang2, cerr);

   if (ppe.add_word_translations && ppe.ibm_1 && ppe.ibm_2) {
      ostream* os1 = NULL;
      ostream* os2 = NULL;
      if (add_word_trans_file != "") {
         os1 = new oSafeMagicStream(add_word_trans_file + ".1");
         os2 = new oSafeMagicStream(add_word_trans_file + ".2");
      }
      if (ppe.verbose) {
         cerr << "ADDING IBM1 translations for untranslated words in lang 1:";
         if (os1) cerr << " (writing to " << add_word_trans_file << ".1)";
         cerr << endl;
      }
      ppe.add_ibm1_translations(1, pt, word_voc_1,
                            add_word_trans_voc == "" ? word_voc_2 : extern_word_voc_2, os1);
      if (ppe.verbose) {
         cerr << "ADDING IBM1 translations for untranslated words in lang 2:";
         if (os1) cerr << " (writing to " << add_word_trans_file << ".2)";
         cerr << endl;
      }
      ppe.add_ibm1_translations(2, pt, word_voc_2,
                            add_word_trans_voc == "" ? word_voc_1 : extern_word_voc_1, os2);
      if (os1) delete os1;
      if (os2) delete os2;
   }

   if (prune1 || prune1w) {
      if (ppe.verbose) {
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
         if (ppe.verbose) cerr << "smoothing:" << endl;

         PhraseSmootherFactory<Uint> smoother_factory(&pt, ppe.ibm_1, ppe.ibm_2, smoothing_verbose);
         vector< PhraseSmoother<Uint>* > smoothers;
         smoother_factory.createSmoothersAndTally(smoothers, smoothing_methods);

         if (multipr_output == "fwd" || multipr_output == "both") {
            string filename = name + "." + lang1 + "2" + lang2 + z_ext;
            if (ppe.verbose) cerr << "Writing " << filename << endl;
            oSafeMagicStream ofs(filename);
            dumpMultiProb(ofs, 1, pt, smoothers, ppe.display_alignments, write_count, ppe.verbose);
         }
         if (multipr_output == "rev" || multipr_output == "both") {
            string filename = name + "." + lang2 + "2" + lang1 + z_ext;
            if (ppe.verbose) cerr << "Writing " << filename << endl;
            oSafeMagicStream ofs(filename);
            dumpMultiProb(ofs, 2, pt, smoothers, ppe.display_alignments, write_count, ppe.verbose);
         }
      }
      if (joint)
         pt.dump_joint_freqs(cout, 0, false, false, ppe.display_alignments);
      if (freqs1 != "") {
         oSafeMagicStream ofs(freqs1);
         pt.dump_freqs_lang1(ofs);
      }
      if (freqs2 != "") {
         oSafeMagicStream ofs(freqs2);
         pt.dump_freqs_lang2(ofs);
      }
   }

   if (ppe.verbose) cerr << "done" << endl;

}

// vim:sw=3:
