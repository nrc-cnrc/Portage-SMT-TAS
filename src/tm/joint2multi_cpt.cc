/**
 * @author George Foster
 * @file joint2multi_cpt.cc 
 * @brief Convert multiple jpts into multi-column cpt(s).
 *
 * COMMENTS:
 *
 * Since the PhraseTable class is really designed for scalar operations only,
 * the frequencies for all jpts are maintained externally here. The jpts are
 * read in to a single PhraseTable one after another, so that it contains the
 * union of all their phrase pairs. Then each jpt is smoothed in sequence:
 * its frequencies are inserted into the PhraseTable, with zeroes for phrase
 * pairs that come from other jpts, then the normal PhraseTable smoothing
 * algorithm is is run.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008 Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008 Her Majesty in Right of Canada
 */

#include <unistd.h>
#include <iostream>
#include <arg_reader.h>
#include "phrase_table.h"
#include "phrase_smoother_cc.h"
#include "phrase_table_writer.h"
#include "hmm_aligner.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
joint2multi_cpt [options] jpt1 .. jptn\n\
\n\
Convert joint phrase tables jpt1..jptn into a multi-column conditional phrase\n\
table, with conditional probability estimates based on selected jpts & smoothing\n\
methods. Any <jpti> argument may alternatively be a directory, in which case all\n\
jpts <jpti>/jpt.* will be concatenated (summing counts), so the directory plays\n\
the same role as a single jpt in the argument list. Output is written to the\n\
file(s) specified by the -o and -dir options.  Columns in the output cpt are:\n\
\n\
   RG RJ1..RJn RL   FG FJ1..FJn FL   [AG AJ1..AJn AL]\n\
\n\
where RG is 0 or more columns of 'reverse' (src given tgt) probability estimates\n\
based on global frequencies (summed over all input jpts), RJi is 0 or more\n\
columns of reverse probability estimates based on frequencies in the ith jpt,\n\
and RL is 0 or columns of reverse 'lexical' probability estimates.  The 'F'\n\
columns are 'forward' estimates in the same order as the 'R' ones.  The contents\n\
of the columns are determined by the arguments to the -s switch.  Note that\n\
'lexical' estimates always come last, regardless of the order in which they are\n\
specified, so it is good practice to list them last. The A* estimates are\n\
optional adirectional ('4th column') probabilities specified by -a switches\n\
using the same syntax as -s. In the A* columns, reverse and forward estimates\n\
from each smoother are written consecutively, and symmetrical smoothers write\n\
only one value.\n\
\n\
Use this program instead of joint2cond_phrase_tables when you have many jpts,\n\
and you want to make smoothed estimates from each jpt for all phrase pairs\n\
contained in the union of all jpts.\n\
\n\
Options:\n\
\n\
-H        List available smoothing methods and quit.\n\
-v        Write progress reports to cerr.\n\
-i        Counts are integers [counts are floating point]\n\
-1 l1     Name of language 1 (one in left column of jpts) [en]\n\
-2 l2     Name of language 2 (one in right column of jpts) [fr]\n\
-prune1   Prune so that each language1 phrase has at most n translations. Based\n\
          on summed joint freqs; done right after reading in all tables.\n\
-prune1w  Same as prune1, but multiply n by the number of words in the current\n\
          source phrase.  When using both -prune1 and -prune1w, keep n + nw*len\n\
          tranlations for a source phrase of len words.\n\
-s sm     Smoothing method for conditional probs. Use -H for list of methods.\n\
          Multiple methods may be specified by using -s repeatedly. By default,\n\
          each listed smoother will be applied to global frequencies and to\n\
          frequencies from each individual jpt, resulting in (n+1)*2 probability\n\
          columns. This may be limited by preceding each smoother with a list of\n\
          the columns to which it should be applied. Eg, '-s 0,2-4,6:RFSmoother'\n\
          means to make RF estimates from global frequencies (id 0) as well as \n\
          from jpts 2, 3, 4, and 6. [RFSmoother]\n\
-a sm     Smoothers for adirectional (4th column) output; syntax is same as -s.\n\
-0 cols   Input jpts to sum over when establishing global frequencies. These are\n\
          specified in the same format as the lists of jpts for smoothers in -s,\n\
          except 0 is not legal. NB: jpts may be duplicated in the list. [1-n]\n\
-max0     Take the max instead of the sum when establishing global frequencies.\n\
-eps0 e   Epsilon value to use for missing phrase pairs when establishing global\n\
          frequencies. For example, if there are three input jpts, then a phrase\n\
          pair that occurs in only one of them, with frequency f, will be\n\
          assigned a global frequency of f + 2e.  [0]\n\
-ibm_l2_given_l1 m  Name of IBM model for language 2 given language 1. Repeat the\n\
          switch to specify multiple models. [none]\n\
-ibm_l1_given_l2 m  Name of IBM model for language 1 given language 2. Repeat the\n\
          switch to specify multiple models. [none]\n\
-ibm n    Type of ibm models given to the -ibm_* switches: 1, 2, or hmm. This\n\
          may be used to force an HMM ttable to be used as an IBM1, for example.\n\
          [determine type of model automatically from filename]\n\
-lc1 loc  Do lowercase mapping of lang 1 words to match IBM/HMM models, using\n\
          locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
-lc2 loc  Do lowercase mapping of lang 2 words to match IBM/HMM models, using\n\
          locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
-o cpt    Set base name for output tables [cpt]\n\
-w cols   Write only the columns in <cols> to <cpt>, where <cols> is a list of\n\
          1-based indexes, eg '1,2,4,6', into the vector of output probability\n\
          columns that would normally get written by -s. NB: columns are written\n\
          in the order they appear on <cols>. [write all]\n\
-dir d    Direction for output tables(s). One of: 'fwd' = output <cpt>.<l1>2<l2>\n\
          for translating from <l1> to <l2>; 'rev' = output <cpt>.<l2>2<l1>; or\n\
          'both' = output both. [fwd]\n\
-write-al A  Write alignment information for all phrase pairs having alignments\n\
          in input jpts, after summing frequencies across jpts. If A is 'top',\n\
          write only the most frequent alignment without its frequency; if 'keep',\n\
          write all alignments with summed frequencies; if 'none', write nothing.\n\
-write-count Write the total joint count for each phrase pair in c=<cnt> format.\n\
-nofwd    Write only 'reverse' estimates, not 'forward' ones.\n\
-z        Compress the output files [don't]\n\
-force    Overwrite any existing files [don't]\n\
";

// globals

static bool verbose = false;
static bool int_counts = false;
static bool nofwd = false;
static string lang1("en");
static string lang2("fr");
static Uint prune1 = 0;
static Uint prune1w = 0;
static vector<string> smoothing_methods;
static vector<string> adir_smoothing_methods;
static string jpts0;
static bool max0 = false;
static double eps0 = 0;
static vector<string> ibm_l2_given_l1;
static vector<string> ibm_l1_given_l2;
static string ibmtype;
static string lc1;
static string lc2;
static string name("cpt");
static vector<Uint> output_cols;
static string output_drn("fwd");
static Uint write_al = 0;
static bool write_count = false;
static bool compress_output = false;
static bool force = false;
static vector<string> input_jpt_files;

static string extension(".gz");

static void getArgs(int argc, char* argv[]);
static void checkOutputFile(const string& filename);
static string addExtension(string fname);
template<class T> void doEverything(const char* prog_name);

// Value (frequency) and index pair. PhraseTable maps each phrase pair to an
// object of this type.

template<class T> struct ValAndIndex {
   T val;
   Uint index;

   ValAndIndex(): val(0), index(-1) {}
   ValAndIndex(T val) : val(val), index(-1) {}

   // treat a val,index pair as a val for all intents and purposes, except the
   // explicit manipulations performed by this program

   operator T() const {
      return val;
   }
   operator T&() {
      return val;
   }
};

template <class StringType, class T> bool conv(StringType s, ValAndIndex<T>& vi)
{
   return conv(s, vi.val);
}

/**
 * Parse a range spec, eg "0,2-4,6". Put resulting list of columns into <jpts>,
 * eg 0,2,3,4,6.
 */
void parseRangeSpec(const string& spec, Uint num_jpts, vector<Uint>& jpts)
{
   vector<string> toks;
   split(spec, toks, ",");
   for (Uint i = 0; i < toks.size(); ++i) {
      if ((last(toks[i]) == '-') || (toks[i].length() && (toks[i][0] == '-')))
         error(ETFatal, "range can't begin or end with '-' in range spec: %s",
               toks[i].c_str());
      vector<string> ntoks;
      split(toks[i], ntoks, "-");
      if (ntoks.size() != 1 && ntoks.size() != 2)
         error(ETFatal, "bad format for range specification: %s", spec.c_str());
      Uint lo = conv<Uint>(ntoks[0]);
      Uint hi = conv<Uint>(ntoks[ntoks.size()-1]);
      if (lo > hi)
         error(ETFatal, "empty range in range spec: %s", toks[i].c_str());
      if (hi >= num_jpts) {
         error(ETWarn, "jpt range limit too large: %d - setting to %d",
	                hi, num_jpts-1);
         hi = num_jpts-1;
      }
      for (Uint j = lo; j < hi+1; ++j) {
         jpts.push_back(j);
      }
   }
}

/**
 * Parse a smoothing specification or die on error.
 * @param spec the specification, eg "0,2-4,6:RFSmoother [args]"
 * @param num_jpts 1 plus the number of jpts: jpt indexes in the "0,2-4,6" part must
 * be < this (NB: 0 is the index reserved for the global estimates).
 * @param smoother on return, the "RFSmoother [args]" part of the original spec
 * @param jpts on return, a list like 0,2,3,4,6 - ie, just an expansion into a
 * list of jpt indexes, not sorted.
 */
template<class T>
void parseSmoothingSpec(const string& spec, Uint num_jpts, string& smoother,
                        vector<Uint>& jpts) 
{
   jpts.clear();
   if (spec.find_first_of("0123456789") == 0 && spec.find(":") != string::npos) {
      vector<string> toks;
      split(spec, toks, ":", 2);
      smoother = toks[1];
      parseRangeSpec(toks[0], num_jpts, jpts);
   } else
      smoother = spec;
   
   if (PhraseSmootherFactory<T>::usesCounts(smoother)) {
      if (jpts.size() == 0)
         for (Uint i = 0; i < num_jpts; ++i)
            jpts.push_back(i);
   } else
      if (jpts.size())
         error(ETFatal, "smoother %s doesn't use counts; it can't be applied to specific jpts", 
               smoother.c_str());
}

/**
 * Filter vector of output probs to retain only the ones specified in output_cols
 */
void filterOutputCols(vector<double>& outprobs)
{
   if (output_cols.size() == 0)
      return;
   vector<double> outpr(outprobs);
   outprobs.clear();
   for (Uint i = 0; i < output_cols.size(); ++i) {
      if (output_cols[i] >= outpr.size())
         error(ETFatal, "-w output index too large: %d", output_cols[i]+1);
      outprobs.push_back(outpr[output_cols[i]]);
   }
}


// main

int main(int argc, char* argv[])
{
   printCopyright(2008, "joint2multi_cpt");
   getArgs(argc, argv);

   if (int_counts)
      doEverything<Uint>(argv[0]);
   else
      doEverything<float>(argv[0]);
}

template<class T>
void doEverything(const char* prog_name)
{
   // initial checks

   if (output_drn == "fwd" || output_drn == "both")
      checkOutputFile(addExtension(name + "." + lang1 + "2" + lang2));
   if (output_drn == "rev" || output_drn == "both")
      checkOutputFile(addExtension(name + "." + lang2 + "2" + lang1));

   for (vector<string>::iterator p = input_jpt_files.begin(); 
	p != input_jpt_files.end(); ++p)
      error_unless_exists(*p, true, "jpt");

   // Analyze the list of smoothers specified using -s/-a, separate into ones
   // that use counts and ones that don't, and create a list of the counting
   // smoothers to be applied to each jpt, and to global counts (index 0)

   vector<string> counting_smoothing_methods; // need jpt counts
   vector<string> noncount_smoothing_methods; // eg, IBM estimates
   // column index (0.. num input jpts+1) -> list of indexes into counting_smoothing_methods
   vector< vector<Uint> > column_smoothers(input_jpt_files.size()+1);
   for (Uint i = 0; i < smoothing_methods.size(); ++i) {
      string smoother;
      vector<Uint> jcols;
      parseSmoothingSpec<T>(smoothing_methods[i], column_smoothers.size(), smoother, jcols);
      if (PhraseSmootherFactory<T>::usesCounts(smoother)) {
         for (Uint j = 0; j < jcols.size(); ++j) 
            column_smoothers[jcols[j]].push_back(counting_smoothing_methods.size());
         counting_smoothing_methods.push_back(smoother);
      } else
         noncount_smoothing_methods.push_back(smoother);
   }
   // a copy of the above block, for -a.
   vector<string> adir_counting_smoothing_methods; // need jpt counts
   vector<string> adir_noncount_smoothing_methods; // eg, IBM estimates
   // column index (0.. num input jpts+1) -> list of indexes into adir_counting_smoothing_methods
   vector< vector<Uint> > adir_column_smoothers(input_jpt_files.size()+1);
   for (Uint i = 0; i < adir_smoothing_methods.size(); ++i) {
      string smoother;
      vector<Uint> jcols;
      parseSmoothingSpec<T>(adir_smoothing_methods[i], adir_column_smoothers.size(), smoother, jcols);
      if (PhraseSmootherFactory<T>::usesCounts(smoother)) {
         for (Uint j = 0; j < jcols.size(); ++j) 
            adir_column_smoothers[jcols[j]].push_back(adir_counting_smoothing_methods.size());
         adir_counting_smoothing_methods.push_back(smoother);
      } else
         adir_noncount_smoothing_methods.push_back(smoother);
   }
   if (verbose) {
      Uint col = 0;
      cerr << "output probability columns:" << endl;
      for (Uint d = 0; d < 2; ++d) {
         for (Uint i = 0; i < column_smoothers.size(); ++i) {
            for (Uint j = 0; j < column_smoothers[i].size(); ++j) {
               cerr << "column " << ++col << ": jpt " << i << " "
                    << (i == 0 ? "global" : input_jpt_files[i-1].c_str()) << " "
                    << counting_smoothing_methods[column_smoothers[i][j]]
                    << (d == 0 ? " (reverse)" : " (forward)") << endl;
            }
         }
         for (Uint i = 0; i < noncount_smoothing_methods.size(); ++i) {
            cerr << "column " << ++col << ": " << noncount_smoothing_methods[i]
                 << (d == 0 ? " (reverse)" : " (forward)") << endl;
         }
      }
      col = 0;
      for (Uint i = 0; i < adir_column_smoothers.size(); ++i) {
         for (Uint j = 0; j < adir_column_smoothers[i].size(); ++j) {
            bool symm = PhraseSmootherFactory<T>::isSymmetrical(
               adir_counting_smoothing_methods[adir_column_smoothers[i][j]]);
            for (Uint d = 0; d < 2; ++d) {
               cerr << "4thcolumn " << ++col << ": jpt " << i << " "
                    << (i == 0 ? "global" : input_jpt_files[i-1].c_str()) << " "
                    << adir_counting_smoothing_methods[adir_column_smoothers[i][j]]
                    << (d == 0 ? (symm ? " (symmetrical)" : " (reverse)") : " (forward)")
                    << endl;
               if (symm) break;
            }
         }
      }
      for (Uint i = 0; i < adir_noncount_smoothing_methods.size(); ++i) {
         if (PhraseSmootherFactory<T>::isSymmetrical(adir_noncount_smoothing_methods[i]))
            cerr << "4thcolumn " << ++col << ": " << noncount_smoothing_methods[i] << " (symmetrical)" << endl;
         else {
            cerr << "4thcolumn " << ++col << ": " << noncount_smoothing_methods[i] << " (reverse)" << endl;
            cerr << "4thcolumn " << ++col << ": " << noncount_smoothing_methods[i] << " (forward)" << endl;
         }
      }
      cerr << endl;
   }

   // read contents of all jpts into pt, storing the freqs of the ith jpt in
   // jointfreqs[i+1], indexed by the index member of the phrases' ValIndex
   // objects

   PhraseTableGen< ValAndIndex<T> > pt;
   vector< vector<T> > jointfreqs(input_jpt_files.size()+1);
   Uint num_phrases = 0;

   for (Uint i = 0; i < input_jpt_files.size(); ++i) {

      // read ith table into pt
      if (verbose) cerr << "reading " << input_jpt_files[i] << "... ";
      if (is_directory(input_jpt_files[i])) {
         if (verbose) cerr << "(concatenating all jpt.* in directory) " << endl;
         string cmd = "zcat " + input_jpt_files[i] + "/jpt.*|" ;
         pt.readJointTable(cmd); 
      } else
         pt.readJointTable(input_jpt_files[i]);

      // save freqs in jointfreqs[i+1], assign indexes to phrases from this jpt
      // that weren't already in pt, and zero freqs in pt for next iter 
      jointfreqs[i+1].resize(num_phrases, 0);
      for (typename PhraseTableGen< ValAndIndex<T> >::iterator it = pt.begin(); 
	   it != pt.end(); ++it) {
         ValAndIndex<T>& vi = it.getJointFreqRef();
         if (vi.index == Uint(-1)) { // new phrase pair
            vi.index = jointfreqs[i+1].size();
            jointfreqs[i+1].push_back(vi.val);
         } else
            jointfreqs[i+1][vi.index] = vi.val;
         vi.val = 0.0;
      }
      if (verbose)
         cerr << jointfreqs[i+1].size() - num_phrases 
	      << " new phrase pairs read" << endl;
      num_phrases = jointfreqs[i+1].size();
   }

   // fill in column 0 with global counts

   vector<Uint> jpt0_list;
   if (jpts0 != "")
      parseRangeSpec(jpts0, input_jpt_files.size()+1, jpt0_list);
   else
      for (Uint i = 1; i <= input_jpt_files.size(); ++i)
         jpt0_list.push_back(i);

   if (verbose)
      cerr << (max0 ? "maxing" : "summing") << " counts for column 0, from jpts: ";
   for (Uint i = 0; i < jpt0_list.size(); ++i) {
      if (jpt0_list[i] == 0)
         error(ETFatal, "0 is not a legal value in the argument to -0");
      if (verbose) cerr << input_jpt_files[jpt0_list[i]-1] << " ";
   }
   if (verbose) cerr << endl;

   jointfreqs[0].resize(num_phrases, 0);
   for (Uint i = 0; i < num_phrases; ++i)
      for (Uint k = 0; k < jpt0_list.size(); ++k) {
         Uint j = jpt0_list[k];
         T freq = i < jointfreqs[j].size() ? jointfreqs[j][i] : 0;
         if (eps0 && freq == 0) freq = eps0;
         if (max0) 
            jointfreqs[0][i] = max(jointfreqs[0][i], freq);
         else 
            jointfreqs[0][i] += freq;
      }

   // prune the whole table using global freqs

   if (prune1 || prune1w) {
      if (verbose) {
         cerr << "pruning to best ";
         if (prune1)            cerr << prune1;
         if (prune1 && prune1w) cerr << "+";
         if (prune1w)           cerr << prune1w << "*numwords";
         cerr << " translations, using total freqs" << endl;
      }

      for (typename PhraseTableGen< ValAndIndex<T> >::iterator it = pt.begin(); 
	   it != pt.end(); ++it) {
         ValAndIndex<T>& vi = it.getJointFreqRef();
         vi.val = jointfreqs[0][vi.index];
      }
      pt.pruneLang2GivenLang1(prune1, prune1w);
   }

   // create IBM models if needed, and set up optional casemapping

   vector<IBM1*> ibm_1s;
   vector<IBM1*> ibm_2s;
   CaseMapStrings cms1(lc1.c_str());
   CaseMapStrings cms2(lc2.c_str());
   for (Uint i = 0; i < ibm_l1_given_l2.size(); ++i) {
      assert(ibm_l2_given_l1.size() > i); // checked in getArgs()
      string namepair = ibm_l2_given_l1[i] + " + " + ibm_l1_given_l2[i];
      ibm_1s.push_back(NULL); ibm_2s.push_back(NULL);
      IBM1::createIBMModelPair(ibm_1s.back(), ibm_2s.back(),
         ibm_l2_given_l1[i], ibm_l1_given_l2[i], ibmtype, verbose);
      assert(ibm_1s.back()); assert(ibm_2s.back());
      if (lc1 != "") {
         ibm_1s[i]->getTTable().setSrcCaseMapping(&cms1);
         ibm_2s[i]->getTTable().setTgtCaseMapping(&cms1);
      }
      if (lc2 != "") {
         ibm_1s[i]->getTTable().setTgtCaseMapping(&cms2);
         ibm_2s[i]->getTTable().setSrcCaseMapping(&cms2);
      }
   }
   assert(ibm_1s.size() == ibm_2s.size());
 
   // Make the counting smoothers. NB: this assumes that each smoother needs
   // access to the appropriate frequencies (for the ith jpt) when it is
   // created, but that the SmootherFactory itself doesn't look at frequencies
   // at all.

   PhraseSmootherFactory< ValAndIndex<T> >
      smoother_factory(&pt, ibm_1s, ibm_2s, verbose);
   typedef PhraseSmoother< ValAndIndex<T> > Smoother;

   vector< vector<Smoother*> > smoothers(jointfreqs.size());
   vector< vector<Smoother*> > adir_smoothers(jointfreqs.size());
   for (Uint i = 0; i < jointfreqs.size(); ++i) { // for each freq column
      // inject column's contents into pt
      for (typename PhraseTableGen< ValAndIndex<T> >::iterator it = pt.begin(); 
	   it != pt.end(); ++it) {
         ValAndIndex<T>& vi = it.getJointFreqRef();
	 vi.val = vi.index < jointfreqs[i].size() ? jointfreqs[i][vi.index] : 0;
      }
      // create all required smoothers for this column
      for (Uint j = 0; j < column_smoothers[i].size(); ++j) {
         string& sm = counting_smoothing_methods[column_smoothers[i][j]];
         smoothers[i].push_back(smoother_factory.createSmootherAndTally(sm));
      }
      for (Uint j = 0; j < adir_column_smoothers[i].size(); ++j) {
         string& sm = adir_counting_smoothing_methods[adir_column_smoothers[i][j]];
         adir_smoothers[i].push_back(smoother_factory.createSmootherAndTally(sm));
      }

   }
   if (verbose && 
       (counting_smoothing_methods.size() || adir_counting_smoothing_methods.size()))
      cerr << "created count-dependent smoother(s)" << endl;

   // Now make the smoothers that don't depend on joint counts.
   vector<Smoother*> noncount_smoothers(noncount_smoothing_methods.size());
   smoother_factory.createSmoothersAndTally(noncount_smoothers, 
                                            noncount_smoothing_methods);

   vector<Smoother*> adir_noncount_smoothers(adir_noncount_smoothing_methods.size());
   smoother_factory.createSmoothersAndTally(adir_noncount_smoothers, 
                                            adir_noncount_smoothing_methods);

   if (verbose && 
       (noncount_smoothing_methods.size() || adir_noncount_smoothing_methods.size()))
      cerr << "created count-independent smoother(s)" << endl;

   // Write output. NB: this assumes that each smoother looks only at the
   // current iterator (plus its own previously-stored information) when
   // estimating probabilities for the current phrase pair - ie, it doesn't
   // look at the rest of the table. This is consistent with the
   // PhraseSmoother interface, but not really enforceable.

   ostream* out_fwd = NULL;
   ostream* out_rev = NULL;
   if (output_drn == "fwd" || output_drn == "both") {
      string filename = addExtension(name + "." + lang1 + "2" + lang2);
      if (verbose) cerr << "writing to " << filename << endl;
      out_fwd = new oSafeMagicStream(filename);
      //out_fwd->precision(9);
   }
   if (output_drn == "rev" || output_drn == "both") {
      string filename = addExtension(name + "." + lang2 + "2" + lang1);
      if (verbose) cerr << "writing to " << filename << endl;
      out_rev = new oSafeMagicStream(filename);
      //out_rev->precision(9);
   }

   vector<double> vals_fwd;
   vector<double> vals_rev;
   vector<double> vals;
   vector<double> adir_vals;
   string p1, p2;
   Uint total = 0;
   for (typename PhraseTableGen< ValAndIndex<T> >::iterator it = pt.begin(); 
	it != pt.end(); ++it) {
      vals_fwd.clear();
      vals_rev.clear();
      adir_vals.clear();
      it.getPhrase(1, p1);
      it.getPhrase(2, p2);
      ValAndIndex<T>& vi = it.getJointFreqRef();
      for (Uint i = 0; i < jointfreqs.size(); ++i) {
	 vi.val = vi.index < jointfreqs[i].size() ? jointfreqs[i][vi.index] : 0;
	 for (Uint j = 0; j < smoothers[i].size(); ++j) {
	    vals_rev.push_back(smoothers[i][j]->probLang1GivenLang2(it));
	    vals_fwd.push_back(smoothers[i][j]->probLang2GivenLang1(it));
	 }
	 for (Uint j = 0; j < adir_smoothers[i].size(); ++j) {
	    adir_vals.push_back(adir_smoothers[i][j]->probLang1GivenLang2(it));
            if (!adir_smoothers[i][j]->isSymmetrical())
               adir_vals.push_back(adir_smoothers[i][j]->probLang2GivenLang1(it));
         }
      }
      for (Uint i = 0; i < noncount_smoothers.size(); ++i) {
         vals_rev.push_back(noncount_smoothers[i]->probLang1GivenLang2(it));
         vals_fwd.push_back(noncount_smoothers[i]->probLang2GivenLang1(it));
      }
      for (Uint i = 0; i < adir_noncount_smoothers.size(); ++i) {
         adir_vals.push_back(adir_noncount_smoothers[i]->probLang1GivenLang2(it));
         if (!adir_noncount_smoothers[i]->isSymmetrical()) 
            adir_vals.push_back(adir_noncount_smoothers[i]->probLang2GivenLang1(it));
      }

      double jfreq = vi.index < jointfreqs[0].size() ? jointfreqs[0][vi.index] : 0;
      string align_str;
      if (write_al) 
         it.getAlignmentString(align_str, false, write_al == 1);

      if (out_fwd) {
	 vals = vals_rev;
	 if (!nofwd) vals.insert(vals.end(), vals_fwd.begin(), vals_fwd.end());
         filterOutputCols(vals);
	 PhraseTableBase::writePhrasePair(*out_fwd, p1.c_str(), p2.c_str(), 
                                          write_al ? align_str.c_str() : NULL, 
                                          vals, write_count, jfreq, &adir_vals);
      }
      if (out_rev) {
         if (nofwd) {
            vals = vals_rev;
         } else {
            vals = vals_fwd;
            vals.insert(vals.end(), vals_rev.begin(), vals_rev.end());
         }
         if (write_al) it.getAlignmentString(align_str, true, write_al == 1);
         filterOutputCols(vals);
	 PhraseTableBase::writePhrasePair(*out_rev, p2.c_str(), p1.c_str(), 
                                          write_al ? align_str.c_str() : NULL, 
                                          vals, write_count, jfreq, &adir_vals);
      }
      ++total;
   }
   if (verbose) {
      cerr << "wrote multi-prob distn(s): "
           << total << " phrase pairs" << endl;
   }

   if (out_fwd) delete out_fwd;
   if (out_rev) delete out_rev;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const string alt_help = PhraseSmootherFactory<Uint>::help();
   const char* switches[] = {
      "v", "i", "1:", "2:", "prune1:", "prune1w:", "s:", "a:", "0:", "max0", "eps0:",
      "ibm_l1_given_l2:", "ibm_l2_given_l1:", "ibm:", "lc1:", "lc2:", 
      "o:", "w:", "dir:", "write-al:", "write-count", "nofwd", "z", "force"
   };
   string output_cols_string, write_al_str;

   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, -1, help_message,
                        "-h", true, alt_help.c_str(), "-H");
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("i", int_counts);
   arg_reader.testAndSet("1", lang1);
   arg_reader.testAndSet("2", lang2);
   arg_reader.testAndSet("prune1", prune1);
   arg_reader.testAndSet("prune1w", prune1w);
   arg_reader.testAndSet("s", smoothing_methods);
   arg_reader.testAndSet("a", adir_smoothing_methods);
   arg_reader.testAndSet("0", jpts0);
   arg_reader.testAndSet("max0", max0);
   arg_reader.testAndSet("eps0", eps0);
   arg_reader.testAndSet("ibm_l2_given_l1", ibm_l2_given_l1);
   arg_reader.testAndSet("ibm_l1_given_l2", ibm_l1_given_l2);
   arg_reader.testAndSet("ibm", ibmtype);
   arg_reader.testAndSet("lc1", lc1);
   arg_reader.testAndSet("lc2", lc2);
   arg_reader.testAndSet("o", name);
   arg_reader.testAndSet("w", output_cols_string);
   arg_reader.testAndSet("dir", output_drn);
   arg_reader.testAndSet("write-al", write_al_str);
   arg_reader.testAndSet("write-count", write_count);
   arg_reader.testAndSet("nofwd", nofwd);
   arg_reader.testAndSet("z", compress_output);
   arg_reader.testAndSet("force", force);

   arg_reader.getVars(0, input_jpt_files);

   if (smoothing_methods.empty())
      smoothing_methods.push_back("RFSmoother");

   if (ibmtype != "" && ibmtype != "1" && ibmtype != "2" && ibmtype != "hmm")
      error(ETFatal, "Bad value for -ibm switch: %s", ibmtype.c_str());

   if (ibm_l1_given_l2.size() != ibm_l2_given_l1.size())
      error(ETFatal, "IBM models in both directions must be paired.");

   if (output_drn != "fwd" && output_drn != "rev" &&  output_drn != "both")
      error(ETFatal, "Bad value for -dir switch: %s", output_drn.c_str());

   if (output_cols_string != "") {
      split(output_cols_string, output_cols, " ,");
      for (Uint i = 0; i < output_cols.size(); ++i) {
         if (output_cols[i] == 0)
            error(ETFatal, "-w output columns must be > 0");
         --output_cols[i];
      }
   }
   if (write_al_str != "") {
      if (write_al_str == "none") write_al = 0;
      else if (write_al_str == "top") write_al = 1;
      else if (write_al_str == "all" || write_al_str == "keep") write_al = 2;
      else
         error(ETFatal, "Bad value for -write-al switch: %s", 
               write_al_str.c_str());
   }
}

static void checkOutputFile(const string& filename) 
{
   if (force)
      delete_if_exists(filename.c_str(),
                       "File %s exists - deleting and recreating");
   else
      error_if_exists(filename.c_str(),
                      "File %s exists, quitting - use -force to overwrite");
}

static string addExtension(string fname)
{
   if (compress_output) fname += extension;
   return fname;
}

