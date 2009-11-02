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
methods. Output is written to the file(s) specified by the -o and -dir options.\n\
Columns in the output cpt are:\n\
\n\
   RG RJ1..RJn RL   FG FJ1..FJn FL\n\
\n\
where RG is 0 or more columns of 'reverse' (src given tgt) probability estimates\n\
based on global frequencies (summed over all input jpts), RJi is 0 or more\n\
columns of reverse probability estimates based on frequencies in the ith jpt,\n\
and RL is 0 or columns of reverse 'lexical' probability estimates. The 'F'\n\
columns are 'forward' estimates in the same order as the 'R' ones.  The contents\n\
of the columns are determined by the arguments to the -s switch.  Note that\n\
'lexical' estimates always come last, regardless of the order in which they are\n\
specified, so it is good practice to list them last.\n\
\n\
Use this program instead of joint2cond_phrase_tables when you have many jpts,\n\
and you want to make smoothed estimates from each jpt for all phrase pairs\n\
contained in the union of all jpts.\n\
\n\
Note: estimates are based on global frequency (sum over all jpts) or individual\n\
jpt frequencies - to base estimates on subsets of jpts, combine them ahead of\n\
time using joint2cond_phrase_table -j, for instance.\n\
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
          source phrase. Only one of prune1 or prune1w may be used.\n\
-s sm     Smoothing method for conditional probs. Use -H for list of methods.\n\
          Multiple methods may be specified by using -s repeatedly. By default,\n\
          each listed smoother will be applied to global frequencies and to\n\
          frequencies from each individual jpt, resulting in (n+1)*2 probability\n\
          columns. This may be limited by preceding each smoother with a list of\n\
          the columns to which it should be applied. Eg, '-s 0,2-4,6:RFSmoother'\n\
          means to make RF estimates from global frequencies (id 0) as well as \n\
          from jpts 2, 3, 4, and 6. [RFSmoother]\n\
-0 cols   Input jpts to sum over when establishing global frequencies. These are\n\
          specified in the same format as the lists of jpts for smoothers in -s,\n\
          except 0 is not legal. NB: jpts may be duplicated in the list. [1-n]\n\
-eps0 e   Epsilon value to use for missing phrase pairs when establishing global\n\
          frequencies. For example, if there are three input jpts, then a phrase\n\
          pair that occurs in only one of them, with frequency f, will be\n\
          assigned a global frequency of f + 2e.  [0]\n\
-ibm_l2_given_l1 m  Name of IBM model for language 2 given language 1 [none]\n\
-ibm_l1_given_l2 m  Name of IBM model for language 1 given language 2 [none]\n\
-ibm n    Type of ibm models given to the -ibm_* switches: 1, 2, or hmm. This\n\
          may be used to force an HMM ttable to be used as an IBM1, for example.\n\
          [determine type of model automatically from filename]\n\
-lc1 loc  Do lowercase mapping of lang 1 words to match IBM/HMM models, using\n\
          locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
-lc2 loc  Do lowercase mapping of lang 2 words to match IBM/HMM models, using\n\
          locale <loc>, eg: C, en_US.UTF-8, fr_CA.88591 [don't map]\n\
-o cpt    Set base name for output tables [cpt]\n\
-dir d    Direction for output tables(s). One of: 'fwd' = output <cpt>.<l1>2<l2>\n\
          for translating from <l1> to <l2>; 'rev' = output <cpt>.<l2>2<l1>; or\n\
          'both' = output both. [fwd]\n\
-z        Compress the output files [don't]\n\
-force    Overwrite any existing files [don't]\n\
";

// globals

static bool verbose = false;
static bool int_counts = false;
static string lang1("en");
static string lang2("fr");
static Uint prune1 = 0;
static Uint prune1w = 0;
static vector<string> smoothing_methods;
static string jpts0;
static double eps0 = 0;
static string ibm_l2_given_l1;
static string ibm_l1_given_l2;
static string ibmtype;
static string lc1;
static string lc2;
static string name("cpt");
static string output_drn("fwd");
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

   // tread a val,index pair as a val for all intents and purposes, except the
   // explicit manipulations performed by this program

   operator T() const {
      return val;
   }
   operator T&() {
      return val;
   }
};

template <class T> bool conv(const string& s, ValAndIndex<T>& vi)
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
      if (!check_if_exists(*p))
	 error(ETFatal, "can't read jpt %s", p->c_str());

   if ( (ibm_l2_given_l1 != "" || ibm_l1_given_l2 != "")  && ibmtype == "") {
      if (check_if_exists(HMMAligner::distParamFileName(ibm_l2_given_l1)) &&
	  check_if_exists(HMMAligner::distParamFileName(ibm_l1_given_l2)))
         ibmtype = "hmm";
      else if (check_if_exists(IBM2::posParamFileName(ibm_l2_given_l1)) &&
	       check_if_exists(IBM2::posParamFileName(ibm_l1_given_l2)))
         ibmtype = "2";
      else
	 ibmtype = "1";
   }

   // Analyze the list of smoothers specified using -s, separate into ones
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
      pt.readJointTable(input_jpt_files[i]);

      // save freqs in jointfreqs[i+1], assign indexes to phrases from this jpt
      // that weren't already in pt, and zero freqs in pt for next iter 
      jointfreqs[i+1].resize(num_phrases);
      for (typename PhraseTableGen< ValAndIndex<T> >::iterator it = pt.begin(); 
	   !it.equal(pt.end()); it.incr()) {
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

   if (verbose) cerr << "summing counts for column 0, from jpts: ";
   for (Uint i = 0; i < jpt0_list.size(); ++i) {
      if (jpt0_list[i] == 0)
         error(ETFatal, "0 is not a legal value in the argument to -0");
      if (verbose) cerr << input_jpt_files[jpt0_list[i]-1] << " ";
   }
   if (verbose) cerr << endl;

   jointfreqs[0].resize(num_phrases);
   for (Uint i = 0; i < num_phrases; ++i)
      for (Uint k = 0; k < jpt0_list.size(); ++k) {
         Uint j = jpt0_list[k];
         if (eps0) {
            if (i >= jointfreqs[j].size() || jointfreqs[j][i] == 0)
               jointfreqs[0][i] += eps0;
            else
               jointfreqs[0][i] += jointfreqs[j][i];
         } else
            if (i < jointfreqs[j].size())
               jointfreqs[0][i] += jointfreqs[j][i];
      }

   // prune the whole table using global freqs

   if (prune1 || prune1w) {
      if (verbose)
         cerr << "pruning to best " << (prune1 ? prune1 : prune1w) 
              << " translations" << (prune1w ? " per word" : "") 
              << ", using total freqs" << endl;
      for (typename PhraseTableGen< ValAndIndex<T> >::iterator it = pt.begin(); 
	   !it.equal(pt.end()); it.incr()) {
         ValAndIndex<T>& vi = it.getJointFreqRef();
         vi.val = jointfreqs[0][vi.index];
      }
      pt.pruneLang2GivenLang1(prune1 ? prune1 : prune1w, prune1w != 0);
   }

   // create IBM models if needed, and set up optional casemapping

   IBM1* ibm_1 = NULL;
   IBM1* ibm_2 = NULL;
   if ( ibm_l2_given_l1 != "" && ibm_l1_given_l2 != "" ) {
      if (ibmtype == "hmm") {
         if (verbose) cerr << "loading HMM models" << endl;
         if (ibm_l2_given_l1 != "") ibm_1 = new HMMAligner(ibm_l2_given_l1);
         if (ibm_l1_given_l2 != "") ibm_2 = new HMMAligner(ibm_l1_given_l2);
      } else if (ibmtype == "1") {
         if (verbose) cerr << "loading IBM1 models" << endl;
         if (ibm_l2_given_l1 != "") ibm_1 = new IBM1(ibm_l2_given_l1);
         if (ibm_l1_given_l2 != "") ibm_2 = new IBM1(ibm_l1_given_l2);
      } else if (ibmtype == "2") {
         if (verbose) cerr << "loading IBM2 models" << endl;
         if (ibm_l2_given_l1 != "") ibm_1 = new IBM2(ibm_l2_given_l1);
         if (ibm_l1_given_l2 != "") ibm_2 = new IBM2(ibm_l1_given_l2);
      }
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

   // Make the counting smoothers. NB: this assumes that each smoother needs
   // access to the appropriate frequencies (for the ith jpt) when it is
   // created, but that the SmootherFactory itself doesn't look at frequencies
   // at all.

   PhraseSmootherFactory< ValAndIndex<T> > smoother_factory(&pt, ibm_1, ibm_2, verbose);
   typedef PhraseSmoother< ValAndIndex<T> > Smoother;

   vector< vector<Smoother*> > smoothers(jointfreqs.size());
   for (Uint i = 0; i < jointfreqs.size(); ++i) { // for each freq column

      // inject column's contents into pt
      for (typename PhraseTableGen< ValAndIndex<T> >::iterator it = pt.begin(); 
	   !it.equal(pt.end()); it.incr()) {
         ValAndIndex<T>& vi = it.getJointFreqRef();
	 vi.val = vi.index < jointfreqs[i].size() ? jointfreqs[i][vi.index] : 0;
      }
      // create all required smoothers for this column
      for (Uint j = 0; j < column_smoothers[i].size(); ++j) {
         string& sm = counting_smoothing_methods[column_smoothers[i][j]];
         smoothers[i].push_back(smoother_factory.createSmoother(sm));
      }
   }
   if (verbose && counting_smoothing_methods.size()) 
      cerr << "created count-dependent smoother(s)" << endl;

   // Now make the smoothers that don't depend on joint counts.
   vector<Smoother*> noncount_smoothers(noncount_smoothing_methods.size());
   for (Uint i = 0; i < noncount_smoothing_methods.size(); ++i)
      noncount_smoothers[i] = 
         smoother_factory.createSmoother(noncount_smoothing_methods[i]);

   if (verbose && noncount_smoothing_methods.size()) 
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
      out_fwd->precision(9);
   }
   if (output_drn == "rev" || output_drn == "both") {
      string filename = addExtension(name + "." + lang2 + "2" + lang1);
      if (verbose) cerr << "writing to " << filename << endl;
      out_rev = new oSafeMagicStream(filename);
      out_rev->precision(9);
   }

   vector<double> vals_fwd;
   vector<double> vals_rev;
   vector<double> vals;
   string p1, p2;
   Uint total = 0;
   for (typename PhraseTableGen< ValAndIndex<T> >::iterator it = pt.begin(); 
	!it.equal(pt.end()); it.incr()) {
      vals_fwd.clear();
      vals_rev.clear();
      it.getPhrase(1, p1);
      it.getPhrase(2, p2);
      ValAndIndex<T>& vi = it.getJointFreqRef();
      for (Uint i = 0; i < jointfreqs.size(); ++i) {
	 vi.val = vi.index < jointfreqs[i].size() ? jointfreqs[i][vi.index] : 0;
	 for (Uint j = 0; j < smoothers[i].size(); ++j) {
	    vals_rev.push_back(smoothers[i][j]->probLang1GivenLang2(it));
	    vals_fwd.push_back(smoothers[i][j]->probLang2GivenLang1(it));
	 }
      }
      for (Uint i = 0; i < noncount_smoothers.size(); ++i) {
         vals_rev.push_back(noncount_smoothers[i]->probLang1GivenLang2(it));
         vals_fwd.push_back(noncount_smoothers[i]->probLang2GivenLang1(it));
      }

      if (out_fwd) {
	 vals = vals_rev;
	 vals.insert(vals.end(), vals_fwd.begin(), vals_fwd.end());
	 PhraseTableBase::writePhrasePair(*out_fwd, p1.c_str(), p2.c_str(), vals);
      }
      if (out_rev) {
	 vals = vals_fwd;
	 vals.insert(vals.end(), vals_rev.begin(), vals_rev.end());
	 PhraseTableBase::writePhrasePair(*out_rev, p2.c_str(), p1.c_str(), vals);
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
      "v", "i", "1:", "2:", "prune1:", "prune1w:", "s:", "0:", "eps0:",
      "ibm_l1_given_l2:", "ibm_l2_given_l1:", "ibm:", "lc1:", "lc2:", 
      "o:", "dir:", "z", "force"
   };

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
   arg_reader.testAndSet("0", jpts0);
   arg_reader.testAndSet("eps0", eps0);
   arg_reader.testAndSet("ibm_l2_given_l1", ibm_l2_given_l1);
   arg_reader.testAndSet("ibm_l1_given_l2", ibm_l1_given_l2);
   arg_reader.testAndSet("ibm", ibmtype);
   arg_reader.testAndSet("lc1", lc1);
   arg_reader.testAndSet("lc2", lc2);
   arg_reader.testAndSet("o", name);
   arg_reader.testAndSet("dir", output_drn);
   arg_reader.testAndSet("z", compress_output);
   arg_reader.testAndSet("force", force);

   arg_reader.getVars(0, input_jpt_files);

   if (smoothing_methods.empty())
      smoothing_methods.push_back("RFSmoother");

   if (prune1 && prune1w)
      error(ETFatal, "only one of -prune1 and -prune1w may be specified");

   if (ibmtype != "" && ibmtype != "1" && ibmtype != "2" && ibmtype != "hmm")
      error(ETFatal, "Bad value for -ibm switch: %s", ibmtype.c_str());

   if ((ibm_l1_given_l2 != "" || ibm_l2_given_l1 != "") && 
       (ibm_l1_given_l2 == "" || ibm_l2_given_l1 == ""))
      error(ETFatal, "IBM models in both directions must be supplied, or none at all");
      
   if (output_drn != "fwd" && output_drn != "rev" &&  output_drn != "both")
      error(ETFatal, "Bad value for -dir switch: %s", output_drn.c_str());
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

