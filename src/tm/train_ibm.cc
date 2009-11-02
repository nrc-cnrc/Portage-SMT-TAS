/**
 * @author George Foster
 * @file train_ibm.cc 
 * @brief Program that trains IBM models 1 and 2.
 *
 * TODO: add ppx-based break condition
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005-2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005-2008, Her Majesty in Right of Canada
 */

#include <file_utils.h>
#include <unistd.h>
#include <arg_reader.h>
#include <printCopyright.h>
#include "tm_io.h"
#include "ibm.h"
#include "hmm_aligner.h"

using namespace Portage;
using namespace std;

static const char help_message[] = "\n\
train_ibm [options] MODEL FILE1_LANG1 FILE1_LANG2 ... FILEn_LANG1 FILEn_LANG2\n\
\n\
  Train IBM1 and IBM2/HMM models for p(LANG2|LANG1) from the given list of\n\
  line-aligned files.  Write results to file MODEL (ttable) and MODEL.pos\n\
  (IBM2 position parameters) or MODEL.dist (HMM jump parameters).\n\
\n\
Options:\n\
\n\
  -v       Write progress reports to cerr.\n\
  -r       Do a pairwise reversal of the input file list, turning it into:\n\
           file1_lang2 file1_lang1 ... fileN_lang2 fileN_lang1\n\
  -n1 N1   Run N1 IBM1 iterations [5]\n\
  -n2 N2   Run N2 IBM2/HMM iterations [5]\n\
  -bin     Write all TTables (IBM1 part of all models) in binary format [don't]\n\
           Note: all programs that read TTables support binary TTables\n\
  -hmm     Train an HMM model rather than an IBM2 model\n\
           [train an IBM2 model, unless INIT_MODEL is an HMM model]\n\
  -speed S For initial pass: 1 (slower, smaller) or 2 (faster, bigger) [2]\n\
  -i INIT_MODEL Initialize the TTable from INIT_MODEL, rather than compiling it\n\
                from corpus.  Only the word pairs in INIT_MODEL will be\n\
                considered as potential translations, and their starting probs\n\
                will come from INIT_MODEL.  If INIT_MODEL.pos/.dist exists,\n\
                IBM2/HMM parameters will be read in, otherwise they will be\n\
                initialized from the command line.\n\
  -s IBM1_MODEL Save IBM1 model to IBM1_MODEL before IBM2/HMM training starts\n\
  -p THRESH     Prune IBM1 probs < THRESH after each IBM1 iteration [1e-06]\n\
  -p2 THRESH2   Prune lexical probs < THRESH2 after each IBM2/HMM iter [1e-10]\n\
  -beg BEGLINE  First line to process in each file [1]\n\
  -end ENDLINE  Last line to process in each file [0 = final]\n\
  -mod REM:DIV  Process only lines with (line_no\%DIV==REM) in each file. [0:1]\n\
  -final-cleanup Delete models before exiting (slow; use for leak detection)\n\
  -max-len MAX  Truncate sentenences longer than MAX tokens; 0 = no limit [0]\n\
\n\
IBM2-only parameters:\n\
  -slen SLEN      Max source length for standard IBM2 pos table [50]\n\
  -tlen TLEN      Max target length for standard IBM2 pos table [50]\n\
  -bksize BKSIZE  Size of IBM2 backoff pos table [50] \n\
\n\
HMM mode selection:\n\
  -mimic HMMMODE  Use canned HMM parameter sets to reproduce a specific paper.\n\
           Combine with advanced options below for further tweaking.\n\
           Any option not mentioned in the description here keeps its default\n\
           value described under the advanced HMM parameter section.\n\
           -mimic implies -hmm.  Supported HMMMODE values:\n\
    default  Just -hmm - supported so a script is allowed to always use -mimic.\n\
    och    Reset defaults as described in Och+Ney (CL, 2003): -p0 0.2 -up0 0.\n\
    liang  Mimic Liang et al (HLT-2006):\n\
           -up0 1 -p0 0 -max-jump 5 -end-dist -anchor -alpha 0.\n\
    he-baseline  Mimic the baseline system described in He (ACL-WMT07):\n\
           -up0 0 -p0 0.05 -max-jump 7 -end-dist -anchor\n\
           Use with -word-classes-l1 to properly mimic this paper.\n\
           Note: He tunes p0 and alpha on held-out data but doesn't say what\n\
           values he obtained (0.05 is a guess that's not too unreasonable).\n\
    he-baseline-mod  Same as he-baseline, but with the Liang style -up0 1 -p0 0\n\
           setting.\n\
    he-lex Mimic He's lexicalized MAP Bayesian learning model (He, ACL-WMT07):\n\
           -map-tau 100 -p0 0.05 -up0 0 -max-jump 7 -end-dist -anchor\n\
\n\
Advanced HMM parameters (overrides what the -mimic option sets):\n\
  -p0 P0     Transition probability to a null alignment (Och uses .2) [0.0]\n\
  -up0 UP0   Add UP0/(I+1) to P0 where I is the length of the lang1 sentence\n\
             (Liang et al, 2006, use -up0 1.0) [1.0]\n\
  -max-jump MAX_JUMP  Bin all jumps of size MAX_JUMP or higher (a la Liang et\n\
             al, 2006) [0, i.e., no max jump]\n\
  -end-dist  Use a distinct distribution for jumps from the start and to the\n\
             end positions (a la Liang et al, 2006) (implies -anchor) [don't]\n\
  -noend-dist Disables -end-dist (only has an effect if -mimic sets -end-dist)\n\
  -anchor    Anchor alignments to end of sequence [don't]\n\
  -noanchor  Disables -anchor (only has an effect if -mimic sets -anchor)\n\
  -word-classes-l1 L1WC  Read word classes from file L1WC and train an\n\
             HMM model conditioning its jump parameters on the word classes (a\n\
             la Och+Ney, 2000 and 2003) [don't use word classes]\n\
  -alpha ALPHA    Alpha smoothe jump parms (a la Och+Ney, 2003) [0.01]\n\
  -lambda LAMBDA  +Lambda smoothe jump parms (Lidstone smoothing) [0.0]\n\
    Note:  Alpha and lambda smoothing can be disabled by setting their values\n\
           to 0.0. They may also be used together: LAMBDA is added to counts\n\
           before normalizing; ALPHA controls interpolation after normalizing.\n\
  -map-tau TAU  Train an HMM model using lexicalized MAP Bayesian learning a la\n\
             He (ACL-WMT07).  TAU is the weight of the prior, with reasonable\n\
             values around 100-1000 according to He's paper.  The prior can be\n\
             any other type of HMM model; it will be created according to the\n\
             other HMM parameters above.  [0, which means not to use MAP]\n\
  -lex-prune-ratio RATIO  When doing lexicalized MAP learning, words with a\n\
             total jump count < TAU * RATIO will use the prior model alone.\n\
             Only meaningful if TAU > 0. [0.1]\n\
\n\
Options for symmetrized training:\n\
  -symmetrized METHOD   Perform symmetrized training []. METHOD can be:\n\
    indep - train non-symmetrized models in both directions at the same time.\n\
    liang - fully symmetrized counting following Liang et al (HLT-2006).\n\
    liang-variant - variant implementation of liang - see tech report.\n\
  -rev-i INIT_REV_MODEL Initial reverse model [inferred from INIT_MODEL]\n\
  -rev-s REV_IBM1_MODEL Output reverse IBM1 model [inferred from IBM1_MODEL]\n\
  -rev-model REV_MODEL  Output reverse model [inferred from REV_MODEL]\n\
  -word-classes-l2 L2WC Word classes for the reverse model []\n\
\n\
TTable conversions between Portage binary format and Giza++ format:\n\
  train_ibm -frombin BIN_TTABLE_FILE TEXT_TTABLE_FILE\n\
  train_ibm -tobin TEXT_TTABLE_FILE BIN_TTABLE_FILE\n\
\n\
Options for parallel training:\n\
  -count-only  Do only the counting part of one iteration, writing counts\n\
               (not a full model) into MODEL, and reverse direction counts into\n\
               REV_MODEL if doing symmetrized training.  -i is required.\n\
  -est-only    Do only the estimation part of one iteration.  In this case,\n\
               count files produced with -count-only should be provided after\n\
               MODEL, instead of parallel files.  -i is required and\n\
               INIT_MODEL should be the same as was given with -count-only.\n\
               With -symmetrized, only the count files for MODEL are listed,\n\
               and they must use standard l2_given_l1 names.  The count files\n\
               for REV_MODEL will be inferred, and must be named the same but\n\
               with l1_given_l2 instead of l2_given_l1.\n\
  In both modes, the type of model is deduced from INIT_MODEL, or the first\n\
  count file, respectively, unless -ibm1, -ibm2 or -hmm is also specified.\n\
";

// globals

// IMPORTANT: if you modify the parameters and switches of this program, you
// should update cat.sh accordingly, since it must support all the options
// train_ibm recognizes!
static const char* switches[] = {
   "v", "r", "-m", "n1:", "n2:", "i:", "s:", "p:", "p2:",
   "slen:", "tlen:", "bksize:", "speed:", "bin",
   "beg:", "end:", "hmm", "alpha:", "lambda:", "p0:", "anchor", "noanchor",
   "up0:", "max-jump:", "end-dist", "noend-dist",
   "word-classes-l1:", "word-classes-l2:", "max-len",
   "liang", "mimic:", "final-cleanup",
   "tobin:", "frombin:", "count-only", "est-only",
   "ibm1", "ibm2", "mod:",
   "rev-i:", "rev-s:", "rev-model:", "symmetrized:",
   "map-tau:", "lex-prune-ratio:",
   "debug-options"
};
static ArgReader arg_reader(ARRAY_SIZE(switches), switches,
                            1, -1, help_message, "-h", true);

static bool verbose = false;
static bool reverse_dir = false;
static Uint num_iters1 = 5;
static Uint num_iters2 = 5;
static string init_model;
static string ibm1_model;
static string model;
static double pruning_thresh = 1e-06;
static double pruning_thresh2 = 1e-10; 
static Uint max_len1 = 0; // max sent length during ibm1 training
static Uint max_len2 = 0; // max sent length during ibm2/hmm training
static Uint slen = 50;
static Uint tlen = 50;
static Uint bksize = 50;
static Uint speed = 2;
static Uint begline = 1;
static Uint endline = 0;
static bool bin_ttables = false;
static bool do_hmm = false;
static double alpha = 0.01;
static double lambda = 0.0;
static double p0 = 0.0;
static double up0 = 1.0;
static bool anchor = false;
static Uint max_jump = 0;
static bool end_dist = false;
static string word_classes_file_lang1;
static string word_classes_file_lang2;
static bool liang = false;
static string mimic;
static string tobin;
static string frombin;
static bool count_only = false;
static bool est_only = false;
static bool do_ibm1 = false;
static bool do_ibm2 = false;
static string modulo;
static Uint mod_divisor = 1;
static Uint mod_remainder = 0;
static string rev_init_model;
static string rev_ibm1_model;
static string rev_model;
static string symmetrized_method;
static bool symmetrized = false;
static double map_tau = 0.0;
static double lex_prune_ratio = 0.1;
static bool final_cleanup = false;
static bool debug_options = false;
static void getArgs(int argc, char* argv[]);

static void truncate(vector<string>& toks, Uint max_len) {
   if ( max_len && toks.size() > max_len ) {
      error(ETWarn, "Truncating long sentence from %u tokens to max: %u",
            toks.size(), max_len);
      toks.resize(max_len);
   }
}

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "train_ibm");
   getArgs(argc, argv);

   string model_parms = do_hmm ? HMMAligner::distParamFileName(model)
                             : IBM2::posParamFileName(model);
   string init_model_parms = do_hmm ? HMMAligner::distParamFileName(init_model)
                                  : IBM2::posParamFileName(init_model);

   if (check_if_exists(model))
      error(ETFatal, "ttable file <%s> exists - won't overwrite", model.c_str());
   if (check_if_exists(model_parms))
      error(ETFatal, "file <%s> exists - won't overwrite", model_parms.c_str());

   IBM1* aligner(NULL);
   bool init_model_is_ibm1(true);
   //HMMAligner* ibm2;
   if ( do_hmm ) {
      const char* const word_classes_file_lang1_cstr = 
         word_classes_file_lang1.empty() ? NULL
            : word_classes_file_lang1.c_str();
      if (init_model == "")        // completely new model
         aligner = new HMMAligner(p0, up0, alpha, lambda,
                                  anchor, end_dist, max_jump,
                                  word_classes_file_lang1_cstr,
                                  map_tau, map_tau*lex_prune_ratio);
      else if (!check_if_exists(init_model_parms)) // read IBM1 but not HMM
         aligner = new HMMAligner(init_model, "",
                                  p0, up0, alpha, lambda,
                                  anchor, end_dist, max_jump,
                                  word_classes_file_lang1_cstr,
                                  map_tau, map_tau*lex_prune_ratio);
      else {                       // read both IBM1 and HMM
         aligner = new HMMAligner(init_model);
         init_model_is_ibm1 = false;
      }
   } else {
      if (init_model == "")        // completely new model
         aligner = new IBM2(slen, tlen, bksize);
      else if (!check_if_exists(init_model_parms)) // read IBM1 but not IBM2
         aligner = new IBM2(init_model, 0, slen, tlen, bksize);
      else {                       // read both IBM1 and IBM2
         aligner = new IBM2(init_model);
         init_model_is_ibm1 = false;
      }
   }

   aligner->useImplicitNulls = false;
   aligner->getTTable().setSpeed(speed);

   IBM1* rev_aligner(NULL);
   if ( symmetrized ) {
      string rev_model_parms =
         do_hmm ? HMMAligner::distParamFileName(rev_model)
                : IBM2::posParamFileName(rev_model);
      string rev_init_model_parms =
         do_hmm ? HMMAligner::distParamFileName(rev_init_model)
                : IBM2::posParamFileName(rev_init_model);

      if (check_if_exists(rev_model))
         error(ETFatal, "ttable file <%s> exists - won't overwrite", rev_model.c_str());
      if (check_if_exists(rev_model_parms))
         error(ETFatal, "file <%s> exists - won't overwrite", rev_model_parms.c_str());

      bool rev_init_model_is_ibm1(true);
      if ( do_hmm ) {
         const char* const word_classes_file_lang2_cstr = 
            word_classes_file_lang2.empty() ? NULL
               : word_classes_file_lang2.c_str();
         if (rev_init_model == "")    // completely new model
            rev_aligner = new HMMAligner(p0, up0, alpha, lambda,
                                         anchor, end_dist, max_jump,
                                         word_classes_file_lang2_cstr,
                                         map_tau, map_tau*lex_prune_ratio);
         else if (!check_if_exists(rev_init_model_parms)) // read IBM1 only
            rev_aligner = new HMMAligner(rev_init_model, "",
                                         p0, up0, alpha, lambda,
                                         anchor, end_dist, max_jump,
                                         word_classes_file_lang2_cstr,
                                         map_tau, map_tau*lex_prune_ratio);
         else {                       // read both IBM1 and HMM
            rev_aligner = new HMMAligner(rev_init_model);
            rev_init_model_is_ibm1 = false;
         }
         if ( symmetrized_method == "liang-variant" ) {
            dynamic_cast<HMMAligner*>(aligner)->useLiangSymVariant = true;
            dynamic_cast<HMMAligner*>(rev_aligner)->useLiangSymVariant = true;
         }
      } else {
         if (rev_init_model == "")    // completely new model
            rev_aligner = new IBM2(slen, tlen, bksize);
         else if (!check_if_exists(rev_init_model_parms)) // read IBM1 only
            rev_aligner = new IBM2(rev_init_model, 0, slen, tlen, bksize);
         else {                       // read both IBM1 and IBM2
            rev_aligner = new IBM2(rev_init_model);
            rev_init_model_is_ibm1 = false;
         }
      }
      if ( init_model_is_ibm1 != rev_init_model_is_ibm1 )
         error(ETFatal, "Different types of models given with -i and -rev-i");

      rev_aligner->useImplicitNulls = false;
      rev_aligner->getTTable().setSpeed(speed);
   }

   if (verbose) {
      cerr << "initializing from ";
      if ( init_model.empty() )
         cerr << "corpus";
      else {
         cerr << init_model;
         if ( symmetrized && !rev_init_model.empty() )
            cerr << "/" << rev_init_model;
      }
      cerr << ":" << endl;
   }

   string in_f1, in_f2;

   bool line_count_calculated(false);
   Uint line_count = 0; // will be initialized in iter 0
   Uint line_modulo = 1000000; // will be re-initialinzed in iter 0

   if ( est_only ) {
      // Counts were done before, just add them and do the estimation phase
      if ( do_ibm1 ) {
         aligner->IBM1::initCounts();
         if ( symmetrized ) rev_aligner->IBM1::initCounts();
      } else {
         aligner->initCounts();
         if ( symmetrized ) rev_aligner->initCounts();
      }

      for (Uint arg = 1; arg < arg_reader.numVars(); ++arg) {
         const string counts_file = arg_reader.getVar(arg);
         if (verbose)
            cerr << "Adding counts from " << counts_file << endl;
         if ( do_ibm1 )
            aligner->IBM1::readAddBinCounts(counts_file);
         else
            aligner->readAddBinCounts(counts_file);

         if (symmetrized) {
            const string rev_counts_file = swap_languages(counts_file, "_given_");
            if ( rev_counts_file.empty() )
               error(ETFatal, "Can't reverse language names in %s",
                     counts_file.c_str());
            if (verbose)
               cerr << "Adding reverse counts from " << rev_counts_file << endl;
            if ( do_ibm1 )
               rev_aligner->IBM1::readAddBinCounts(rev_counts_file);
            else
               rev_aligner->readAddBinCounts(rev_counts_file);
         }
      }

      pair<double,Uint> r = do_ibm1 ?
         aligner->IBM1::estimate(pruning_thresh, pruning_thresh/1000) :
         aligner->estimate(pruning_thresh2, pruning_thresh2*10);

      if (verbose)
         cerr << "parallel iter"
              << (do_ibm1 ? " (IBM1)" : ( do_hmm ? " (HMM)" : " (IBM2)") )
              << ": prev ppx = " << r.first
              << ", size = " << r.second << " word pairs."
              << endl;
      if ( do_ibm1 )
         aligner->IBM1::write(model, bin_ttables);
      else
         aligner->write(model, bin_ttables);

      if ( symmetrized ) {
         pair<double,Uint> rev_r = do_ibm1 ?
            rev_aligner->IBM1::estimate(pruning_thresh, pruning_thresh/1000) :
            rev_aligner->estimate(pruning_thresh2, pruning_thresh2*10);
         if (verbose)
            cerr << "rev parallel iter"
                 << (do_ibm1 ? " (IBM1)" : ( do_hmm ? " (HMM)" : " (IBM2)") )
                 << ": prev ppx = " << rev_r.first
                 << ", size = " << rev_r.second << " word pairs."
                 << endl;
         if ( do_ibm1 )
            rev_aligner->IBM1::write(rev_model, bin_ttables);
         else
            rev_aligner->write(rev_model, bin_ttables);
      }

      return 0;
   }

   for (Uint iter = 0; iter <= num_iters1 + num_iters2; ++iter) {

      if (iter == 0 && !init_model.empty())
         continue; // skip ttable init if there is an input model

      const time_t start_time = time(NULL);

      if (iter == 1)
         if (verbose) cerr << "beginning training:" << endl;

      if (iter > 0) {
         if ( iter <= num_iters1 ) {
            aligner->IBM1::initCounts();
            if ( symmetrized ) rev_aligner->IBM1::initCounts();
         } else {
            aligner->initCounts();
            if ( symmetrized ) rev_aligner->initCounts();
         }
      }

      Uint global_lineno = 0;
      Uint lines_processed = 0;
      for (Uint arg = 1; arg+1 < arg_reader.numVars(); arg += 2) {

         Uint a1 = reverse_dir ? 1 : 0, a2 = reverse_dir ? 0 : 1;
         string file1 = arg_reader.getVar(arg+a1),
                file2 = arg_reader.getVar(arg+a2);
         if (verbose)
            cerr << "reading " << file1 << "/" << file2;
         arg_reader.testAndSet(arg+a1, "file1", in_f1);
         arg_reader.testAndSet(arg+a2, "file2", in_f2);
         iSafeMagicStream in1(in_f1);
         iSafeMagicStream in2(in_f2);

         string line1, line2;
         vector<string> toks1, toks2;

         Uint lineno = 0;
         while (getline(in1, line1)) {
            if (!getline(in2, line2)) {
               error(ETFatal, "Line counts differ in file pair %s/%s. Aborting",
                     file1.c_str(), file2.c_str());
               break;
            }
            ++lineno; ++global_lineno;
            if ( verbose && iter != 0 && global_lineno % line_modulo == 0 )
               cerr << ".";

            if (lineno < begline) continue;
            if (endline != 0 && lineno > endline) break;
            if (global_lineno % mod_divisor != mod_remainder) continue;

            ++lines_processed;
            toks1.clear(); split(line1, toks1);
            toks2.clear(); split(line2, toks2);

            if (iter == 0) {
               aligner->add(toks1, toks2, true);
               if ( symmetrized )
                  rev_aligner->add(toks2, toks1, true);
            } else if (iter <= num_iters1) {
               truncate(toks1, max_len1);
               truncate(toks2, max_len1);
               if ( symmetrized && isPrefix("liang", symmetrized_method) ) {
                  aligner->IBM1::count_symmetrized(toks1, toks2, true,
                                                   rev_aligner);
               } else {
                  aligner->IBM1::count(toks1, toks2, true);
                  if ( symmetrized )
                     rev_aligner->IBM1::count(toks2, toks1, true);
               }
            } else {
               truncate(toks1, max_len2);
               truncate(toks2, max_len2);
               if ( symmetrized && isPrefix("liang", symmetrized_method) ) {
                  aligner->count_symmetrized(toks1, toks2, true, rev_aligner);
               } else {
                  aligner->count(toks1, toks2, true);
                  if ( symmetrized )
                     rev_aligner->count(toks2, toks1, true);
               }
            }

            if (verbose && lineno % 100000 == 0)
               cerr << "line " << lineno << " in " << file1 << "/"
                    << file2 << endl;
         }
         if ( !line_count_calculated ) line_count += lineno;
         if ( verbose ) cerr << endl;

         if (endline == 0 && getline(in2, line2))
            error(ETFatal, "Line counts differ in file pair %s/%s. Aborting",
                  file1.c_str(), file2.c_str());
      }
      if ( !line_count_calculated ) {
         line_count_calculated = true;
         line_modulo = 1 + line_count / 20;
      }
      cerr << "Lines processed: " << lines_processed << endl;
      if ( count_only ) {
         if ( do_ibm1 ) {
            aligner->IBM1::writeBinCounts(model);
            if ( symmetrized ) rev_aligner->IBM1::writeBinCounts(rev_model);
         } else {
            aligner->writeBinCounts(model);
            if ( symmetrized ) rev_aligner->writeBinCounts(rev_model);
         }
         return 0;
      }
      if (iter == 0) {
         aligner->compile();
         if ( symmetrized ) rev_aligner->compile();
         // BIN testing
         //string bin_file(model + string("iter") + char('A' + iter) + ".gz");
         //aligner->getTTable().test_read_write_bin(bin_file);
      } else {
         // read/write_bin_count testing
         //string count_file(model + "iter" + char('A' + iter) + "-counts.gz");
         //aligner->testReadWriteBinCounts(count_file);

         pair<double,Uint> r = iter <= num_iters1 ?
            aligner->IBM1::estimate(pruning_thresh, pruning_thresh/1000) :
            aligner->estimate(pruning_thresh2, pruning_thresh2*10);
         if (verbose)
            cerr << "iter " << iter
                 << (iter <= num_iters1
                      ? " (IBM1)"
                      : ( do_hmm ? " (HMM)" : " (IBM2)") )
                 << ": prev ppx = " << r.first
                 << ", size = " << r.second << " word pairs"
                 << "; took " << (time(NULL) - start_time) << " second(s)."
                 << endl;
         if (ibm1_model != "" && iter == num_iters1)
            aligner->IBM1::write(ibm1_model, bin_ttables);

         if ( symmetrized ) {
            pair<double,Uint> rev_r = iter <= num_iters1 ?
               rev_aligner->IBM1::estimate(pruning_thresh, pruning_thresh/1000) :
               rev_aligner->estimate(pruning_thresh2, pruning_thresh2*10);
            if (verbose)
               cerr << "rev  " << iter
                    << (iter <= num_iters1
                         ? " (IBM1)"
                         : ( do_hmm ? " (HMM)" : " (IBM2)") )
                    << ": prev ppx = " << rev_r.first
                    << ", size = " << rev_r.second << " word pairs"
                    << "; took " << (time(NULL) - start_time) << " second(s)."
                    << endl;
            if (rev_ibm1_model != "" && iter == num_iters1)
               rev_aligner->IBM1::write(rev_ibm1_model, bin_ttables);
         }

         // BIN testing
         //string bin_file(model + string("iter") + char('A' + iter) + ".gz");
         //aligner->getTTable().test_read_write_bin(bin_file);

         // test for break condition here
      }
   }
   if ( num_iters2 > 0 || !init_model_is_ibm1 ) {
      aligner->write(model, bin_ttables);
      if ( symmetrized ) rev_aligner->write(rev_model, bin_ttables);
   } else {
      aligner->IBM1::write(model, bin_ttables);
      if ( symmetrized ) rev_aligner->IBM1::write(rev_model, bin_ttables);
   }

   // Clean up - we typically skip this step as an optimization, since the OS
   // cleans things up much faster, and just as effectively on Linux hosts.
   if ( final_cleanup ) {
      time_t start = time(NULL);
      delete aligner;
      delete rev_aligner;
      cerr << "Spent " << (time(NULL)-start) << " seconds cleaning up";
   }
}


// arg processing

void getArgs(int argc, char* argv[])
{
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("mimic", mimic);
   arg_reader.testAndSet("liang", liang);
   if ( liang ) {
      error(ETWarn, "The -liang option has been replaced by -mimic liang. "
            "Please update your scripts.");
      if ( !mimic.empty() ) {
         error(ETFatal, "Combining the obsolete -liang with the new -mimic "
               "is not allowed.");
      } else {
         error(ETWarn, "Proceeding as if you had typed -mimic liang.");
         mimic = "liang";
      }
   }
   if ( !mimic.empty() ) {
      do_hmm = true;
      if ( mimic == "och" ) {
         up0 = 0.0;
         p0 = 0.2;
      } else if ( mimic == "liang" ) {
         up0 = 1.0;
         p0 = 0.0;
         max_jump = 5;
         end_dist = true;
         anchor = true;
         alpha = 0.0;
      } else if ( mimic == "he-baseline" ) {
         up0 = 0.0;
         p0 = 0.05;
         max_jump = 7;
         end_dist = true;
         anchor = true;
      } else if ( mimic == "he-baseline-mod" ) {
         up0 = 1.0;
         p0 = 0.0;
         max_jump = 7;
         end_dist = true;
         anchor = true;
      } else if ( mimic == "he-lex" ) {
         up0 = 0.0;
         p0 = 0.05;
         max_jump = 7;
         end_dist = true;
         anchor = true;
         map_tau = 100;
      } else if ( mimic == "default" ) {
         // do nothing
      } else
         error(ETFatal, "Unknown -mimic value: %s", mimic.c_str());
   }

   arg_reader.testAndSet("hmm", do_hmm);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("r", reverse_dir);
   arg_reader.testAndSet("i", init_model);
   arg_reader.testAndSet("s", ibm1_model);
   arg_reader.testAndSet("n1", num_iters1);
   arg_reader.testAndSet("n2", num_iters2);
   arg_reader.testAndSet("p", pruning_thresh);
   arg_reader.testAndSet("p2", pruning_thresh2);
   arg_reader.testAndSet("slen", slen);
   arg_reader.testAndSet("tlen", tlen);
   arg_reader.testAndSet("bksize", bksize);
   arg_reader.testAndSet("speed", speed);
   arg_reader.testAndSet("beg", begline);
   arg_reader.testAndSet("end", endline);
   arg_reader.testAndSet("bin", bin_ttables);
   arg_reader.testAndSet("max-len", max_len1);
   arg_reader.testAndSet("max-len", max_len2);
   arg_reader.testAndSet("alpha", alpha);
   arg_reader.testAndSet("lambda", lambda);
   arg_reader.testAndSet("p0", p0);
   arg_reader.testAndSet("up0", up0);
   arg_reader.testAndSet("max-jump", max_jump);
   arg_reader.testAndSetOrReset("end-dist", "noend-dist", end_dist);
   arg_reader.testAndSetOrReset("anchor", "noanchor", anchor);
   arg_reader.testAndSet("word-classes-l1", word_classes_file_lang1);
   arg_reader.testAndSet("word-classes-l2", word_classes_file_lang2);
   arg_reader.testAndSet("tobin", tobin);
   arg_reader.testAndSet("frombin", frombin);
   arg_reader.testAndSet("ibm1", do_ibm1);
   arg_reader.testAndSet("ibm2", do_ibm2);
   arg_reader.testAndSet("count-only", count_only);
   arg_reader.testAndSet("est-only", est_only);
   arg_reader.testAndSet("mod", modulo);
   arg_reader.testAndSet("rev-i", rev_init_model);
   arg_reader.testAndSet("rev-s", rev_ibm1_model);
   arg_reader.testAndSet("rev-model", rev_model);
   arg_reader.testAndSet("symmetrized", symmetrized_method);
   arg_reader.testAndSet("map-tau", map_tau);
   arg_reader.testAndSet("lex-prune-ratio", lex_prune_ratio);
   arg_reader.testAndSet("final-cleanup", final_cleanup);
   arg_reader.testAndSet("debug-options", debug_options);

   arg_reader.testAndSet(0, "model", model);

   if ( end_dist ) anchor = true;
   if ( lambda < 0 || alpha < 0 || pruning_thresh < 0 || pruning_thresh2 < 0 )
      error(ETFatal, "Options -p, -p2, -lambda, and -alpha can't take "
                     "negative arguments.");
   if ( map_tau < 0.0 || lex_prune_ratio < 0.0 )
      error(ETFatal, "Options -map-tau and -lex-prune-raio can't take "
                     "negative arguments.");

   if ( (mimic == "he-baseline" || mimic == "he-baseline-mod") &&
        word_classes_file_lang1.empty() )
      error(ETWarn, "Using He baseline without word classes.  Did you forget the -word-classes-l1 switch?");

   //if ( p0 >= .2 && up0 >= 1 )
   //   error(ETFatal, "Combination of -p0 %f with -up0 %f is too high.",
   //         p0, up0);

   // for bin <-> text model conversions, we use a side effect of the main
   // loop, which will load a model and rewrite it if n1 = n2 = 0 and an
   // init_model is provided.
   if ( tobin != "" ) {
      init_model = tobin;
      num_iters1 = num_iters2 = 0;
      bin_ttables = true;
   } else if ( frombin != "" ) {
      init_model = frombin;
      num_iters1 = num_iters2 = 0;
      bin_ttables = false;
   }

   if ( count_only ) {
      if ( init_model == "" )
         error(ETFatal, "-i init_model is required with -count-only");
      if (!do_hmm && !do_ibm1 && !do_ibm2) {
         if ( check_if_exists(HMMAligner::distParamFileName(init_model)) )
            do_hmm = true;
         else if ( check_if_exists(IBM2::posParamFileName(init_model)) )
            do_ibm2 = true;
         else
            do_ibm1 = true;
      }
      if ( do_hmm || do_ibm2 ) {
         num_iters1 = 0;
         num_iters2 = 1;
      } else {
         num_iters1 = 1;
         num_iters2 = 0;
      }
   } else if ( est_only ) {
      if ( init_model == "" )
         error(ETFatal, "-i init_model is required with -est-only");
      if ( arg_reader.numVars() < 2 )
         error(ETFatal, "at least one count file is required with -est-only");
      if (!do_hmm && !do_ibm1 && !do_ibm2) {
         string first_count_file = arg_reader.getVar(1);
         if ( check_if_exists(addExtension(first_count_file, ".hmm")) )
            do_hmm = true;
         else if ( check_if_exists(addExtension(first_count_file, ".ibm2")) )
            do_ibm2 = true;
         else
            do_ibm1 = true;
      }
   }

   if ( !symmetrized_method.empty() ) {
      symmetrized = true;
      if ( symmetrized_method != "zens" && symmetrized_method != "liang" &&
           symmetrized_method != "liang-variant" &&
           symmetrized_method != "indep" )
         error(ETFatal, "unknown symmetrization method: %s; expected "
                        "indep, zens, liang or liang-variant",
                        symmetrized_method.c_str());
      string l1, l2;
      if ( !init_model.empty() && rev_init_model.empty() ) {
         rev_init_model = swap_languages(init_model, "_given_", &l1, &l2);
         if ( rev_init_model.empty() )
            error(ETFatal, "Can't infer -rev-i from non-standard -i: %s", 
                           init_model.c_str());
      }
      if ( init_model.empty() && !rev_init_model.empty() )
         error(ETFatal, "-i is required when -rev-i is specified.");
      if ( !ibm1_model.empty() && rev_ibm1_model.empty() ) {
         string s_l1, s_l2;
         rev_ibm1_model = swap_languages(ibm1_model, "_given_", &s_l1, &s_l2);
         if ( rev_ibm1_model.empty() )
            error(ETFatal, "Can't infer -rev-s from non-standard -s: %s",
                           ibm1_model.c_str());
         if ( !l1.empty() && (l1 != s_l1 || l2 != s_l2) )
            error(ETFatal, "Language codes not consistent between -s and -i "
                  "(%s_given_%s != %s_given_%s), specify -rev-s or -rev-i "
                  "explicitly",
                  s_l1.c_str(), s_l2.c_str(), l1.c_str(), l2.c_str());
         l1 = s_l1;
         l2 = s_l2;
      }
      if ( ibm1_model.empty() && !rev_ibm1_model.empty() )
         error(ETFatal, "-s is required when -rev-s is specified.");
      if ( rev_model.empty() ) {
         string m_l1, m_l2;
         rev_model = swap_languages(model, "_given_", &m_l1, &m_l2);
         if ( rev_model.empty() )
            error(ETFatal, "Can't infer -rev-model from non-standard model: %s",
                           model.c_str());
         if ( !l1.empty() && (l1 != m_l1 || l2 != m_l2) )
            error(ETFatal, "Language codes not consistent between -s, -i "
                  "and/or MODEL (%s_given_%s != %s_given_%s), specify -rev-s, "
                  "-rev-i and/or -rev-model explicitly",
                  l1.c_str(), l2.c_str(), m_l1.c_str(), m_l2.c_str());
      }
      if ( (!word_classes_file_lang1.empty()) != (!word_classes_file_lang2.empty()) )
         error(ETFatal, "In symmetrized training, specify both -word-classes-l1"
                        " and -word-classes-l2 or neither.");
   }

   if ( init_model != "" )
      if (check_if_exists(HMMAligner::distParamFileName(init_model)))
         do_hmm = true;

   if ( modulo != "" ) {
      vector<Uint> tokens;
      bool success = split(modulo, tokens, ":", 2);
      if ( !success || tokens.size() != 2 ||
           tokens[0] > tokens[1] || tokens[1] == 0 )
         error(ETFatal, "invalid modulo specification: %s", modulo.c_str());
      mod_remainder = tokens[0];
      mod_divisor = tokens[1];
      if ( mod_divisor == mod_remainder ) mod_remainder = 0;
   }

   if ( debug_options ) {
      cerr << "verbose \t"<< verbose << nf_endl;
      cerr << "reverse_dir\t"<< reverse_dir << nf_endl;
      cerr << "num_iters1\t"<< num_iters1 << nf_endl;
      cerr << "num_iters2\t"<< num_iters2 << nf_endl;
      cerr << "init_model\t"<< init_model << nf_endl;
      cerr << "ibm1_model\t"<< ibm1_model << nf_endl;
      cerr << "model   \t"<< model << nf_endl;
      cerr << "pruning_thresh\t"<< pruning_thresh << nf_endl;
      cerr << "pruning_thresh2\t"<< pruning_thresh2 << nf_endl;
      cerr << "max_len1\t"<< max_len1 << nf_endl;
      cerr << "max_len2\t"<< max_len2 << nf_endl;
      cerr << "slen    \t"<< slen << nf_endl;
      cerr << "tlen    \t"<< tlen << nf_endl;
      cerr << "bksize  \t"<< bksize << nf_endl;
      cerr << "speed   \t"<< speed << nf_endl;
      cerr << "begline \t"<< begline << nf_endl;
      cerr << "nf_endline \t"<< endline << endl;
      cerr << "bin_ttables\t"<< bin_ttables << nf_endl;
      cerr << "do_hmm  \t"<< do_hmm << nf_endl;
      cerr << "alpha   \t"<< alpha << nf_endl;
      cerr << "lambda  \t"<< lambda << nf_endl;
      cerr << "p0      \t"<< p0 << nf_endl;
      cerr << "up0     \t"<< up0 << nf_endl;
      cerr << "anchor  \t"<< anchor << nf_endl;
      cerr << "max_jump\t"<< max_jump << nf_endl;
      cerr << "end_dist\t"<< end_dist << nf_endl;
      cerr << "word_classes_file_lang1\t"<< word_classes_file_lang1 << nf_endl;
      cerr << "word_classes_file_lang2\t"<< word_classes_file_lang2 << nf_endl;
      cerr << "liang   \t"<< liang << nf_endl;
      cerr << "mimic   \t"<< mimic << nf_endl;
      cerr << "tobin   \t"<< tobin << nf_endl;
      cerr << "frombin \t"<< frombin << nf_endl;
      cerr << "count_only\t"<< count_only << nf_endl;
      cerr << "est_only\t"<< est_only << nf_endl;
      cerr << "do_ibm1 \t"<< do_ibm1 << nf_endl;
      cerr << "do_ibm2 \t"<< do_ibm2 << nf_endl;
      cerr << "modulo  \t"<< modulo << nf_endl;
      cerr << "mod_divisor\t"<< mod_divisor << nf_endl;
      cerr << "mod_remainder\t"<< mod_remainder << nf_endl;
      cerr << "rev_init_model\t"<< rev_init_model << nf_endl;
      cerr << "rev_ibm1_model\t"<< rev_ibm1_model << nf_endl;
      cerr << "rev_model\t"<< rev_model << nf_endl;
      cerr << "symmetrized_method\t"<< symmetrized_method << nf_endl;
      cerr << "symmetrized\t"<< symmetrized << nf_endl;
      cerr << "map_tau\t"<< map_tau << nf_endl;
      cerr << "lex_prune_ratio\t"<< lex_prune_ratio << nf_endl;
      cerr << "final_cleanup\t"<< final_cleanup << nf_endl;
      cerr << "debug_options\t"<< debug_options << endl;
      exit(1);
   }
}
