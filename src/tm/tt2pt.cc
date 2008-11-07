/**
 * @author George Foster
 * @file tt2pt.cc  Program that generates a joint phrase table from a pair of
 * ibm ttables and corresponding vocabulary files.
 * 
 * 
 * COMMENTS: 
 *
 * Convert two IBM model ttables (in each direction) to phrase tables. 
 * 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <arg_reader.h>
#include <ttable.h>
#include <phrase_table.h>
#include <printCopyright.h>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
tt2pt [-v][-w1 wt1][-u unkprob][-t thresh] voc1 voc2 tt_2_given_1 tt_1_given_2\n\
\n\
Generate a joint phrase table from a pair of ibm ttables and corresponding\n\
vocabulary files. The vocabulary files must contain one space-separated\n\
word/count pair per line (ie the format produced by get_voc); normally these\n\
will be generated from the same corpus used to train the ibm models. As their\n\
names indicate, the ibm models must be in opposite directions: first\n\
p(lang2|lang1) (ie, having the language of voc1 in the left column), then the\n\
opposite. One joint distribution is constructed from each tt/voc pair, then the\n\
two joint distributions are interpolated to form the final joint phrase table. \n\
Output is written to stdout, with lang1 in the left column.\n\
\n\
Options:\n\
\n\
-v   Write progress reports to cerr.\n\
-w1  Weight on the joint distribution p(lang1)p(lang2|lang1). The weight on\n\
     p(lang2)p(lang1|lang2) is 1-w1. [0.5]\n\
-u   Unigram probability to use for words that are in a ttable but not in the\n\
     corresponding voc file. [1e-06]\n\
-t   Filter all phrase pairs whose joint probability is < thresh [1e-07]\n\
";

// globals

static bool verbose = false;
static string vocfile1, vocfile2, ttfile_2_given_1, ttfile_1_given_2;
static double wt1 = 0.5;
static double unk_prob = 1e-06;
static double thresh = 1e-07;

static void getArgs(int argc, char* argv[]);

void addJointDistn(_CountingVoc<double>& voc, TTable& tt, double wt, bool voc_is_lang1, 
		   PhraseTableGen<float>& pt);

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "tt2pt");
   getArgs(argc, argv);

   if (verbose) cerr << "reading voc files" << endl;
   _CountingVoc<double> voc1, voc2;
   voc1.read(vocfile1);
   voc1.normalize();
   voc2.read(vocfile2);
   voc2.normalize();

   if (verbose) cerr << "reading ttables" << endl;
   TTable tt_2_given_1(ttfile_2_given_1);
   TTable tt_1_given_2(ttfile_1_given_2);

   PhraseTableGen<float> pt;

   if (verbose) cerr << "calculating joint distributions" << endl;
   addJointDistn(voc1, tt_2_given_1, wt1, true, pt);
   addJointDistn(voc2, tt_1_given_2, 1.0-wt1, false, pt);

   string filename;
      
   if (verbose) cerr << "writing joint phrase table" << endl;
   pt.dump_joint_freqs(cout, thresh);
}

void addJointDistn(_CountingVoc<double>& voc, TTable& tt, double wt, bool voc_is_lang1, PhraseTableGen<float>& pt)
{
   vector<string> tword(1);
   vector<string> sv;
   tt.getSourceVoc(sv);

   for (Uint i = 0; i < sv.size(); ++i) {
      const TTable::SrcDistn& distn = tt.getSourceDistn(sv[i]);
      double marginal = voc.freq(sv[i].c_str());
      if (marginal == 0) marginal = unk_prob;

      for (Uint j = 0; j < distn.size(); ++j) {
	 float jointprob = marginal * distn[j].second * wt;
	 tword[0] = tt.targetWord(distn[j].first);

	 if (voc_is_lang1)
	    pt.addPhrasePair(sv.begin()+i, sv.begin()+i+1, tword.begin(), tword.end(), jointprob);
	 else
	    pt.addPhrasePair(tword.begin(), tword.end(), sv.begin()+i, sv.begin()+i+1, jointprob);
      }
   }
}


// arg processing

void getArgs(int argc, char* argv[])
{
   const char* const switches[] = {"v", "t:", "wt1:", "u:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 4, 4, help_message, "-h", true);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("wt1", wt1);
   arg_reader.testAndSet("u", unk_prob);
   arg_reader.testAndSet("t", thresh);

   arg_reader.testAndSet(0, "voc1", vocfile1);
   arg_reader.testAndSet(1, "voc2", vocfile2);
   arg_reader.testAndSet(2, "tt_2_given_1", ttfile_2_given_1);
   arg_reader.testAndSet(3, "tt_1_given_2", ttfile_1_given_2);

}   
