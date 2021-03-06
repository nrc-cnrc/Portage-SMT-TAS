/**
 * @author George Foster, with reduced memory usage mods by Darlene Stewart
 * @file joint2cond_phrase_tables.cc
 * @brief Program that converts joint-frequency phrase table.
 *
 *
 * COMMENTS:
 *
 * Convert a joint-frequency phrase table (as output by gen_phrase_tables -i or -j)
 * into 2 conditional probability tables (as required by canoe).
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <unistd.h>
#include <iostream>
#include "exception_dump.h"
#include "arg_reader.h"
#include <printCopyright.h>
#include "phrase_table.h"
#include "phrase_smoother.h"
#include "phrase_smoother_cc.h"
#include "phrase_table_writer.h"
#include "hmm_aligner.h"
#include "logging.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
joint2cond_phrase_tables [-Hvijz][-[no-]sort][-1 l1][-2 l2][-o name][-s 'meth args']\n\
                         [-ibm n][-hmm][-ibm_l2_given_l1 m][-ibm_l1_given_l2 m]\n\
                         [-prune1 n][-prune1w nw][-multipr d]\n\
                         [-lc1 loc][-lc2 loc] [-[no-]reduce-mem][jtable]\n\
\n\
Convert joint-frequency phrase table <jtable> (stdin if no <jtable> parameter\n\
given) into two standard conditional-probability phrase tables\n\
<name>.<l1>_given_<l2> and <name>.<l2>_given_<l1>. <jtable> can be one or more\n\
individual joint tables cat'd together. Note that the ibm-model parameters are\n\
required only for certain smoothing schemes.\n\
\n\
Options:\n\
\n\
-H    List available smoothing methods and quit.\n\
-v    Write progress reports to cerr.\n\
-i    Counts are integers [counts are floating point]\n\
-max m Ignore phrase pairs where either side is longer than m tokens when\n\
      reading input. [0 = no limit]\n\
-x    Swap left/right phrases when reading them in. Not compatible with joint\n\
      tables containing alignment info. This option is meant for use with the\n\
      -prune* options, when the source language is language2 (right column).\n\
-prune1  Prune so that each language1 phrase has at most n translations. This is\n\
      based on joint frequencies, and is done right after reading in the table.\n\
-prune1w  Same as prune1, but multiply nw by the number of words in the current\n\
      source phrase.  When using both -prune1 and -prune1w, keep n + nw*len\n\
      tranlations for a source phrase of len words.\n\
-j    Write global joint frequency phrase table to stdout (useful for combining\n\
      multiple tables cat'd to <jtable>).\n\
-z    Compress the output files[don't]\n\
-1    Name of language 1 (one in left column of <jtable>) [en]\n\
-2    Name of language 2 (one in right column of <jtable>) [fr]\n\
-o    Set base name for output tables [phrases]\n\
-s    Smoothing method for conditional probs. Use -H for list of methods.\n\
      Multiple methods may be specified by using -s repeatedly, but these are\n\
      only useful if -multipr output is selected. [RFSmoother]\n\
-[no-]sort Sort/don't sort phrase tables on the source phrase. [sort]\n\
-ibm  Use IBM model <n> for lexical smoothing: 1 or 2\n\
-hmm  Use HMM model for lexical smoothing\n\
      [if, for both models provided with -ibm_l2_given_l1 and -ibm_l1_given_l2,\n\
      <model>.pos exists, then -ibm 2 is assumed; else if <model>.dist exists,\n\
      then -hmm is assumed; otherwise -ibm 1 is assumed]\n\
-ibm_l2_given_l1  Name of IBM model for language 2 given language 1 [none]\n\
-ibm_l1_given_l2  Name of IBM model for language 1 given language 2 [none]\n\
-lc1  Do lowercase mapping of lang 1 words to match IBM/HMM models, using\n\
      locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
      (Compilation with ICU is required to use UTF-8 locales.)\n\
-lc2  Do lowercase mapping of lang 2 words to match IBM/HMM models, using\n\
      locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
-multipr d  Write text phrase table(s) with multiple probabilities: for each\n\
            phrase pair, one or more 'backward' probabilities followed by one\n\
            or more 'forward' probabilities (more than one when multiple\n\
            smoothing methods are selected). d may be one of 'fwd', 'rev', or\n\
            'both', to select: output <name>.<lang1>2<lang2>, the reverse, or\n\
            both directions.\n\
-write-al A Show alignment information with all phrase pairs written in\n\
            multi-prob and joint frequency formats.  Alignments must be present\n\
            in <jtable>.  Alignments are reversed in reversed multi-prob\n\
            tables.  If A is \"top\", the most frequent alignment is written in\n\
            the \"green\" format surrounded by a=(<alignment>), just before the\n\
            scores or counts.  If A is \"keep\" (or \"all\"), the alignment\n\
            information found in <jtable> is copied through as is.  \"none\" is\n\
            the same as not specifying -write-al.  [none]\n\
-write-count Show the joint count in the 3rd column of multi-prob phrase tables\n\
             in the format c=<count>.  [don't]\n\
-write-smoother-state Show the internal state of the smoothers for each phrase\n\
                      pair.  Use -H to see what are the internal states. [don't]\n\
-[no-]reduce-mem  Reduce/don't reduce memory usage. Memory reduction is\n\
                  achieved by not keeping the phrase tables entirely in memory.\n\
                  This requires reading the jpt file multiple times, once for\n\
                  each iteration over the phrase table. [don't reduce memory]\n\
                  Note: Concatenation of jpts is not permitted with the\n\
                  reduced memory option, and the jpt cannot be read from stdin.\n\
                  Use merge_counts to tally counts from multiple jpts before\n\
                  using this option, or else the output will be incorrect.\n\
                  When combined with -prune1[w], all phrase pairs for a given\n\
                  lang1 phrase must also occur consecutively.\n\
";

// globals

static bool verbose = false;
static bool int_counts = false;
static bool joint = false;
static Uint max_len_on_read = 0;
static bool swap_on_read = false;
static Uint prune1 = 0;
static Uint prune1w = 0;
static string lang1("en");
static string lang2("fr");
static string name("phrases");
static vector<string> smoothing_methods;
static Uint ibm_num = 42; // 42 means uninitialized-getArgs will set its value.
static bool use_hmm = false;
static string ibm_l2_given_l1;
static string ibm_l1_given_l2;
static string lc1;
static string lc2;
static string multipr_output = "";
static string in_file;
static bool compress_output = false;
static const string extension(".gz");
static bool sorted(true);
static bool reduce_memory(false);
static string store_alignment_option = "";
static Uint display_alignments = 0; // 0=none, 1=top, 2=all/keep
static bool write_count(false);
static bool write_smoother_state(false);

static void getArgs(int argc, const char* const argv[]);

template<class T>
static void doEverything(const char* prog_name);

// main

int MAIN(argc,argv)
{
   printCopyright(2005, "joint2cond_phrase_tables");
   Logging::init();
   getArgs(argc, argv);

   if (int_counts)
      doEverything<Uint>(argv[0]);
   else
      doEverything<float>(argv[0]);
}
END_MAIN

static string makeFinalFileName(string orignal_filename)
{
   if (compress_output) orignal_filename += extension;
   return orignal_filename;
}

static void open_output_file(oMagicStream& ofs, string& filename) {
   if (verbose) cerr << "Writing " << filename << endl;
   ofs.open(filename);
   if (ofs.fail())
      error(ETFatal, "Unable to open %s for writing", filename.c_str());
}

static void open_mp_file(oMagicStream& ofs, string& name, string& lang1, string& lang2) {
   string filename = makeFinalFileName(name + "." + lang1 + "2" + lang2);
   if (sorted) {
      filename = " > " + filename;
      if (compress_output) filename = "| gzip" + filename;
      filename = "| LC_ALL=C TMPDIR=. sort " + filename;
   }
   open_output_file(ofs, filename);
}

template<class T>
static void doEverything(const char* prog_name)
{
   time_t start_time(time(NULL));
   PhraseTableGen<T> pt;
   pt.readJointTable(in_file, reduce_memory, swap_on_read, max_len_on_read);

   if (verbose && !reduce_memory) {
      cerr << "read joint table: "
           << pt.numLang1Phrases() << " " << lang1 << " phrases, "
           << pt.numLang2Phrases() << " " << lang2 << " phrases" << endl;
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

   if (joint)
      pt.dump_joint_freqs(cout, 0, false, false, display_alignments);

   IBM1* ibm_1 = NULL;
   IBM1* ibm_2 = NULL;
   if ( ibm_l2_given_l1 != "" || ibm_l1_given_l2 != "" ) {
      if (use_hmm) {
         if (verbose) cerr << "Loading HMM models" << endl;
         if (ibm_l2_given_l1 != "") ibm_1 = new HMMAligner(ibm_l2_given_l1);
         if (ibm_l1_given_l2 != "") ibm_2 = new HMMAligner(ibm_l1_given_l2);
      } else if (ibm_num == 1) {
         if (verbose) cerr << "Loading IBM1 models" << endl;
         if (ibm_l2_given_l1 != "") ibm_1 = new IBM1(ibm_l2_given_l1);
         if (ibm_l1_given_l2 != "") ibm_2 = new IBM1(ibm_l1_given_l2);
      } else if (ibm_num == 2) {
         if (verbose) cerr << "Loading IBM2 models" << endl;
         if (ibm_l2_given_l1 != "") ibm_1 = new IBM2(ibm_l2_given_l1);
         if (ibm_l1_given_l2 != "") ibm_2 = new IBM2(ibm_l1_given_l2);
      } else
         error(ETFatal, "Invalid option: -ibm %d", ibm_num);
   }

   CaseMapStrings cms1(lc1.c_str());
   CaseMapStrings cms2(lc2.c_str());
   if (lc1 != "" && ibm_1 && ibm_2) {
      ibm_1->getTTable().setSrcCaseMapping(&cms1);
      ibm_2->getTTable().setTgtCaseMapping(&cms1);
   }
   if (lc2 != "" && ibm_1 && ibm_2) {
      ibm_1->getTTable().setTgtCaseMapping(&cms2);
      ibm_2->getTTable().setSrcCaseMapping(&cms2);
   }

   PhraseSmootherFactory<T> smoother_factory(&pt, ibm_1, ibm_2, verbose);
   vector< PhraseSmoother<T>* > smoothers;
   smoother_factory.createSmoothersAndTally(smoothers, smoothing_methods);

   if (verbose && reduce_memory) {
      cerr << "read joint table: "
           << pt.numLang1Phrases() << " " << lang1 << " phrases, "
           << pt.numLang2Phrases() << " " << lang2 << " phrases" << endl;
   }
   if (verbose)
      cerr << "Read joint table and ran smoothers in " << (time(NULL) - start_time) << " seconds" << endl;

   oMagicStream mp_fwd_ofs, mp_rev_ofs;
   bool fwd = multipr_output == "fwd" || multipr_output == "both";
   if (fwd) {
      open_mp_file(mp_fwd_ofs, name, lang1, lang2);
   }
   bool rev = multipr_output == "rev" || multipr_output == "both";
   if (rev) {
      open_mp_file(mp_rev_ofs, name, lang2, lang1);
   }

   Uint total = 0;
   Uint mp_fwd_non_zero = 0,  mp_rev_non_zero = 0;
   start_time = time(NULL);
   for (typename PhraseTableGen<T>::iterator it(pt.begin()); it != pt.end(); ++it) {
      if (fwd)
         if (dumpMultiProb(mp_fwd_ofs, 1, it, smoothers, display_alignments, write_count, write_smoother_state, verbose))
            ++mp_fwd_non_zero;
      if (rev)
         if (dumpMultiProb(mp_rev_ofs, 2, it, smoothers, display_alignments, write_count, write_smoother_state, verbose))
            ++mp_rev_non_zero;
      ++total;
   }
   if (verbose)
      cerr << "Wrote files in " << (time(NULL) - start_time) << " seconds" << endl;
   if (fwd) {
      mp_fwd_ofs.flush();
      if (verbose)
         cerr << "dumped fwd multi-prob distn: "
              << mp_fwd_non_zero << " non-zero prob phrase pairs of "
              << total << " total phrase pairs" << endl;
   }
   if (rev) {
      mp_rev_ofs.flush();
      if (verbose)
         cerr << "dumped rev multi-prob distn: "
              << mp_rev_non_zero << " non-zero prob phrase pairs of "
              << total << " total phrase pairs" << endl;
   }
}

// arg processing

static void getArgs(int argc, const char* const argv[])
{
   const string alt_help = PhraseSmootherFactory<Uint>::help();
   const char* switches[] = {
      "v", "i", "j", "z", "max:", "x", "prune1:", "prune1w:", "s:", "1:", "2:", "o:", "force",
      "ibm:", "hmm", "ibm_l1_given_l2:", "ibm_l2_given_l1:",
      "lc1:", "lc2:",
      "write-al:", "write-count", "write-smoother-state",
      "tmtext", "multipr:", "sort", "no-sort",
      "reduce-mem", "no-reduce-mem"
   };

   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 1, help_message,
                        "-h", true, alt_help.c_str(), "-H");
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("i", int_counts);
   arg_reader.testAndSet("j", joint);
   arg_reader.testAndSet("max", max_len_on_read);
   arg_reader.testAndSet("x", swap_on_read);
   arg_reader.testAndSet("prune1", prune1);
   arg_reader.testAndSet("prune1w", prune1w);
   arg_reader.testAndSet("z", compress_output);
   arg_reader.testAndSet("1", lang1);
   arg_reader.testAndSet("2", lang2);
   arg_reader.testAndSet("o", name);
   arg_reader.testAndSet("s", smoothing_methods);
   arg_reader.testAndSet("ibm", ibm_num);
   arg_reader.testAndSet("hmm", use_hmm);
   arg_reader.testAndSet("ibm_l2_given_l1", ibm_l2_given_l1);
   arg_reader.testAndSet("ibm_l1_given_l2", ibm_l1_given_l2);
   arg_reader.testAndSet("lc1", lc1);
   arg_reader.testAndSet("lc2", lc2);
   arg_reader.testAndSet("multipr", multipr_output);
   bool force(false);
   arg_reader.testAndSet("force", force);
   if (force) error(ETWarn, "ignoring obsolete -force option - existing files are now always overwritten");
   arg_reader.testAndSetOrReset("sort", "no-sort", sorted);
   if (sorted && multipr_output != "") cerr << "Producing sorted cpt." << endl;
   arg_reader.testAndSetOrReset("reduce-mem", "no-reduce-mem", reduce_memory);
   arg_reader.testAndSet("write-al", store_alignment_option);
   arg_reader.testAndSet("write-count", write_count);
   arg_reader.testAndSet("write-smoother-state", write_smoother_state);

   arg_reader.testAndSet(0, "jtable", in_file);
   if (in_file.empty()) in_file = "-";

   if (arg_reader.getSwitch("tmtext"))
      error(ETFatal, "-tmtext is obsolete");

   if ( (ibm_l2_given_l1 != "" || ibm_l1_given_l2 != "") &&
        ibm_num == 42 && !use_hmm ) {
      // neither -hmm nor -ibm specified; default is IBM2 if .pos files
      // exist, or HMM if .dist files exist, else IBM1
      if ( check_if_exists(IBM2::posParamFileName(ibm_l2_given_l1)) &&
           check_if_exists(IBM2::posParamFileName(ibm_l1_given_l2)) )
         ibm_num = 2;
      else if ( check_if_exists(HMMAligner::distParamFileName(ibm_l2_given_l1)) &&
                check_if_exists(HMMAligner::distParamFileName(ibm_l1_given_l2)) )
         use_hmm = true;
      else
         ibm_num = 1;
   }

   if (smoothing_methods.empty())
      smoothing_methods.push_back("RFSmoother");

   if (!joint && multipr_output.empty())
      error(ETFatal, "No output requested");

   if (multipr_output != "" && multipr_output != "fwd" && multipr_output != "rev" &&
       multipr_output != "both")
      error(ETFatal, "Unknown value for -multipr switch: %s", multipr_output.c_str());

   if (smoothing_methods.size() > 1 && multipr_output.empty()) {
      error(ETWarn, "Multiple smoothing methods are only used with -multipr output - ignoring all but %s",
            smoothing_methods[0].c_str());
      smoothing_methods.resize(1);
   }

   if (reduce_memory && (in_file == "-")) {
      error(ETWarn, "Cannot reduce memory when reading jtable from stdin. "
            "Phrase tables will be kept entirely in memory.  "
            "(Ignoring -reduce-mem option.)");
      reduce_memory = false;
   }

   if ((prune1||prune1w) && (multipr_output == "rev" || multipr_output == "both"))
      error(ETFatal, "prune1(w) is not valid with -multipr rev or -multipr both.");

   if (!store_alignment_option.empty()) {
      if ( store_alignment_option == "top" ) {
         // display_alignments==1 means display only top one, without freq
         display_alignments = 1;
      } else if ( store_alignment_option == "keep" || store_alignment_option == "all" ) {
         // display_alignments==2 means display all alignments with freq.  In
         // joint2cond_phrase_tables, this means preserving whatever was in jtable.
         display_alignments = 2;
      } else if ( store_alignment_option == "none" ) {
         display_alignments = 0;
      } else {
         error(ETFatal, "Invalid -write-al value: %s; expected top or keep.",
               store_alignment_option.c_str());
      }
   }

   if (display_alignments && multipr_output.empty() && !joint)
      error(ETFatal, "-write-al requires -multipr or -j");

   if (write_count && multipr_output.empty())
      error(ETFatal, "-write-count requires -multipr");
}

