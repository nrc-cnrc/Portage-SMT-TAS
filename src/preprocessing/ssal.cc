/**
 * @author George Foster
 * @file ssal.cc
 * @brief Simple sentence aligner.
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <cfloat>
#include <cmath>
#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"
#include "ibm.h"
#include "casemap_strings.h"
#include <memory> // for auto_ptr

using namespace Portage;

/// Program ssal (Simple sentence aligner) usage
static char help_message[] = "\n\
ssal [-vH][-hm mark][-m1 m][-m2 m][-w w][-ibm_1g2 model][-ibm_2g1 model]\n\
     [-l][-lc locale][-f][-fm][-a alfile][-i idfile]\n\
     file1 file2\n\
\n\
Sentence-align file1 with file2, writing the results to file1.al and file2.al.\n\
Input files should have one segment per line, and output files will be\n\
line-aligned. Known correspondence points can be indicated using hard markup\n\
specified via the -hm flag. Suspected correspondence points can be indicated\n\
using soft markup: any identical lines, which will be considered very likely\n\
to align with each other. Both kinds of markup can be optionally filtered from\n\
the output.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-H  Print some guidelines on multi-pass alignment, then quit.\n\
-hm Interpret <mark>, when alone on a line, as hard markup: align sentence\n\
    pairs between consecutive marks, never across them. It is an error for one\n\
    input file to contain more hard marks than the other. By default, <mark>\n\
    pairs are written to all output files, but this can be changed with -fm.\n\
-m1 Set maximum number of file1 segments per alignment to m [3].\n\
-m2 Set maximum number of file2 segments per alignment to m [3].\n\
-w  Set weight on segment match feature to w [0.4 if -ibm* given, 10 else].\n\
-ibm_{1g2,2g1} Load p(L1|L2) and p(L2|L1) ttables from <model> files, and use\n\
    IBM scores instead of standard length-match scoring function. These options\n\
    must always be given together. They require that file1/file2 be tokenized.\n\
-lc Map input to lowercase according to <locale> before scoring with IBM\n\
    models.  (Compilation with ICU is required to use UTF-8 locales.)\n\
-l  Shortcut for -lc en_US.UTF-8 (requires compilation with ICU).\n\
-f  Filter out alignment pairs in which at least one member is markup (first\n\
    and last non-white characters are < and > respectively). This does not\n\
    affect hard markup specified by -hm.\n\
-fm Filter out hard markup if -hm is given [include <mark> pairs from input].\n\
-a  Write alignment info to <alfile>. Format is: b1-e1 b2-e2 n1-n2 s,\n\
    indicating that line numbers [b1,e1) from file1 align to lines [b2,e2)\n\
    from file2, with alignment pattern n1-n2, and score s (the total alignment\n\
    score is the sum over all sentences).\n\
-i  Read one id line from <idfile> for each region identified by hard markup,\n\
    and write one copy of the line to <idfile>.al for each sentence pair in the\n\
    region after alignment (<idfile>.al will be line-aligned to the other .al\n\
    files). NB: <idfile> is expected to contain exactly one line for each hard\n\
    region, even empty ones. This means, for instance, that it must contain 1\n\
    line if file1/file2 are empty. Due to the line-alignment requirement for\n\
    output, however, id lines for empty regions are not written to <idfile>.al.\n\
";

static char alt_help[] = "\n\
Here is a suggested multi-pass procedure for aligning raw text:\n\
\n\
1) Identify paragraphs or similar structure in the text, using cues such as\n\
   blank lines. Put both source and target texts in one-paragraph-per-line\n\
   format, not necessarily tokenized. Put any parallel anchors that are\n\
   identical across both texts alone on a line.\n\
\n\
2) Align paragraphs using ssal in length-based mode (no -ibm* options), eg:\n\
\n\
   ssal en.para fr.para\n\
\n\
3) Tokenize and sentence-split the results from 2, marking aligned paragraph\n\
   boundaries with blank lines, eg:\n\
\n\
   utokenize.pl -ss -paraline -p -lang=en en.para.al | head -n -1 > en.sent\n\
   utokenize.pl -ss -paraline -p -lang=fr fr.para.al | head -n -1 > fr.sent\n\
\n\
4) Align sentences using ssal in length-based mode, treating paragraph\n\
   boundaries as hard constraints, eg:\n\
\n\
   ssal -hm \"\" -fm -a al.len en.sent fr.sent \n\
\n\
5) Train HMM models on the sentence alignments mapped to lowercase, eg:\n\
\n\
   utf8_casemap en.sent.al en.sent.lc\n\
   utf8_casemap fr.sent.al fr.sent.lc\n\
   train_ibm -hmm -bin hmm.fr_given_en.bin en.sent.lc fr.sent.lc\n\
   train_ibm -hmm -bin hmm.en_given_fr.bin fr.sent.lc en.sent.lc\n\
\n\
6) Re-align using HMM model, eg:\n\
\n\
   mv en.sent.al{,.len}\n\
   mv fr.sent.al{,.len}\n\
   ssal -hm \"\" -fm -l -a al.ibm \\\n\
        -ibm_1g2 hmm.en_given_fr.bin -ibm_2g1 hmm.fr_given_en.bin \\\n\
        en.sent fr.sent\n\
\n\
7) Optionally, compare length-based and ibm-based alignments, eg:\n\
\n\
   al-diff.py -m \"\" al.len al.ibm en.sent fr.sent | less\n\
\n\
Steps 5 and 6 usually improve alignment quality, but this depends on many\n\
factors, such as the size of the corpus. For large corpora, the HMM model\n\
training should be done in parallel using cat.sh.  Variants on this procedure\n\
involve tuning ssal's -w parameter using al-diff.py, and training the HMM on\n\
only high-confidence len-based alignments from step 4.\n\
";

// globals

static bool verbose = false;
static bool filt = false;
static bool filt_mark = false;
static string filename1, filename2;
static string alfilename;
static string idfilename;
static Uint maxsegs1 = 3;      ///< max # file1 segs that can participate in an alignment
static Uint maxsegs2 = 3;      ///< max # file2 segs that can participate in an alignment
static bool len_mismatch_set = false;
static double len_mismatch_wt = 10.0;
static const char* mark = NULL; // optional hard marker
static string ibm_1g2_filename;
static string ibm_2g1_filename;
static bool lowercase = false;
static string lc_locale;
const string default_locale = "en_US.UTF-8";
static IBM1 *ibm_1g2, *ibm_2g1;
static auto_ptr<CaseMapStrings> mapr;

static void getArgs(int argc, char* argv[]);
static double alignScore(const vector<string>& lines1, const vector<string>& lines2,
                         Uint beg1, Uint end1, Uint beg2, Uint end2,
                         Uint totlen1, Uint totlen2);
static bool keep(const vector<string>& lines1, const vector<string>& lines2,
                 Uint beg1, Uint end1, Uint beg2, Uint end2);
static bool read(istream& file, vector<string>& lines);
static void align(const vector<string>& lines1, const vector<string>& lines2,
                  vector<double> &score_matrix, vector<Uint>& backlink_matrix,
                  vector<Uint>& connections);
static void write(Uint region_num,
                  const vector<string>& lines1, const vector<string>& lines2,
                  const vector<double> &score_matrix, const vector<Uint>& backlink_matrix,
                  vector<Uint>& connections,
                  ostream& file1, ostream& file2, ostream* alfile,
                  const string& idline, ostream* oidfile);

// main

int main(int argc, char* argv[])
{
   printCopyright(2006, "ssal");
   getArgs(argc, argv);

   // read and init

   iSafeMagicStream file1(filename1);
   iSafeMagicStream file2(filename2);
   iSafeMagicStream* idfile = idfilename != "" ?
      new iSafeMagicStream(idfilename) : NULL;


   oSafeMagicStream ofile1(filename1 + ".al");
   oSafeMagicStream ofile2(filename2 + ".al");
   oSafeMagicStream* alfile = alfilename != "" ?
      new oSafeMagicStream(alfilename) : NULL;
   oSafeMagicStream* oidfile = idfilename != "" ?
      new oSafeMagicStream(idfilename + ".al") : NULL;

   if (ibm_1g2_filename != "") ibm_1g2 = new IBM1(ibm_1g2_filename);
   if (ibm_2g1_filename != "") ibm_2g1 = new IBM1(ibm_2g1_filename);

   if (ibm_1g2 && !len_mismatch_set) len_mismatch_wt = 0.4;

   // align and write output

   vector<double> score_matrix;
   vector<Uint> backlink_matrix;
   vector<Uint> connections;
   vector<string> lines1, lines2;
   string idline;
   Uint region_num = 0;
   while (true) {
      bool s1 = read(file1, lines1);
      bool s2 = read(file2, lines2);
      bool si = idfile ? getline(*idfile, idline) : true;
      if (s1 && s2 && si) {
         ++region_num;
         if (verbose) cerr << "aligning region " << region_num << endl;
         align(lines1, lines2, score_matrix, backlink_matrix, connections);
         write(region_num, lines1, lines2, score_matrix, backlink_matrix,
               connections, ofile1, ofile2, alfile, idline, oidfile);
      } else if (s1 == false && s2 == false && (idfile == NULL || si == false)) {
         break;  // end of file
      } else {   // asymmetric eof
         string shortones;
         if (!s1) shortones += filename1 + " ";
         if (!s2) shortones += filename2 + " ";
         if (!si) shortones += idfilename;
         error(ETFatal, "file(s) end prematurely: %s", shortones.c_str());
         break;
      }
   }

   // cleanup (safe as is, without guard, even when pointers are NULL)
   delete alfile;
   delete idfile;
   delete oidfile;
}

/**
 * Parses the command line arguments.
 * @param argc  number of command line arguments
 * @param argv  vector of command line arguments
 */
static void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "f", "fm", "hm:", "a:", "i:", "m1:", "m2:",
                             "w:", "ibm_1g2:", "ibm_2g1:", "l", "lc:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 2, help_message, "-h",
                        false, alt_help, "-H");
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("f", filt);
   arg_reader.testAndSet("fm", filt_mark);

   static string markstring;
   if (arg_reader.getSwitch("hm", &markstring))
      mark = markstring.c_str();

   arg_reader.testAndSet("a", alfilename);
   arg_reader.testAndSet("i", idfilename);
   arg_reader.testAndSet("m1", maxsegs1);
   arg_reader.testAndSet("m2", maxsegs2);
   len_mismatch_set = arg_reader.getSwitch("w");
   arg_reader.testAndSet("w", len_mismatch_wt);
   arg_reader.testAndSet("ibm_1g2", ibm_1g2_filename);
   arg_reader.testAndSet("ibm_2g1", ibm_2g1_filename);
   arg_reader.testAndSet("l", lowercase);
   arg_reader.testAndSet("lc", lc_locale);
   if ( lowercase ) {
      if ( !lc_locale.empty() && lc_locale != default_locale )
         error(ETWarn, "Specified conflicting options -l and -lc %s; using locale %s for lowercasing.",
               lc_locale.c_str(), lc_locale.c_str());
      else
         lc_locale = default_locale;
   }

   if ( !lc_locale.empty() ) {
      lowercase = true;
      mapr.reset(new CaseMapStrings(lc_locale.c_str()));
   }

   if ((ibm_1g2_filename != "" && ibm_2g1_filename == "") ||
       (ibm_1g2_filename == "" && ibm_2g1_filename != ""))
      error(ETFatal, "IBM models in both directions must be specified");

   arg_reader.testAndSet(0, "file1", filename1);
   arg_reader.testAndSet(1, "file2", filename2);
}


/**
 * If we're filtering output (-f), return true if we should keep the current
 * aligned pair. This checks for pairs whose members begin and end with
 * < and > respectively.
 */
static bool keep(const vector<string>& lines1, const vector<string>& lines2,
                 Uint beg1, Uint end1, Uint beg2, Uint end2)
{
   static const char* white = " \r\t\n";

   string::size_type b1 = string::npos;
   string::size_type e1 = string::npos;
   string::size_type b2 = string::npos;
   string::size_type e2 = string::npos;

   if (beg1 != end1) {
      b1 = lines1[beg1].find_first_not_of(white);
      e1 = lines1[end1-1].find_last_not_of(white);
   }
   if (beg2 != end2) {
      b2 = lines2[beg2].find_first_not_of(white);
      e2 = lines2[end2-1].find_last_not_of(white);
   }

   return !
      ( ( b1 != string::npos && lines1[beg1][b1] == '<' &&
          e1 != string::npos && lines1[end1-1][e1] == '>' ) ||
        ( b2 != string::npos && lines2[beg2][b2] == '<' &&
          e2 != string::npos && lines2[end2-1][e2] == '>' ) );
}

/**
 * Calulate a log-domain score for aligning the text in lines1 from [beg1,end1)
 * with that in lines2 from [beg2,end2).
 * @param lines1    text # 1
 * @param lines2    text # 2
 * @param beg1      start index for lines1
 * @param end1      end+1 index for lines1
 * @param beg2      start index for lines2
 * @param end2      end+1 index for lines2
 * @param totlen1   num chars in lines1
 * @param totlen2   num chars in lines2
 * @return  score
 */
static double alignScore(const vector<string>& lines1, const vector<string>& lines2,
                         Uint beg1, Uint end1, Uint beg2, Uint end2,
                         Uint totlen1, Uint totlen2)
{
   static vector<string> toks1, toks2;
   toks1.clear();
   toks2.clear();

   Uint n1 = end1 - beg1, n2 = end2 - beg2;
   if (n1 == 1 && n2 == 1 && lines1[beg1] == lines2[beg2])
      return 0.0;    // special case: single exact match.

   if (n1 > n2) swap(n1,n2);
   const double prior = (n1 == 0 ? 1 : n1 * n1) + n2-n1;

   Uint len1 = 0, len2 = 0;
   for (Uint i = beg1; i < end1; ++i) {
      len1 += lines1[i].length();
      if (ibm_1g2) split(lines1[i], toks1);
   }
   for (Uint i = beg2; i < end2; ++i) {
      len2 += lines2[i].length();
      if (ibm_1g2) split(lines2[i], toks2);
   }
   if (lowercase) {
      for (Uint i = 0; i < toks1.size(); ++i)
         mapr->toLower(toks1[i], toks1[i]);
      for (Uint j = 0; j < toks2.size(); ++j)
         mapr->toLower(toks2[j], toks2[j]);
   }

   double match_score = 0.0;
   if (!ibm_1g2) {
      double n = max(max(len1,len2), max(totlen1,totlen2)); // largest component
      double l1 = len1/n, l2 = len2/n, tl1 = totlen1/n, tl2 = totlen2/n;
      double z = sqrt(l1*l1 + l2*l2) * sqrt(tl1*tl1 + tl2*tl2);
      match_score = log(l1*tl1 + l2*tl2) - log(z);
   } else {
      match_score = ibm_1g2->logpr(toks2, toks1) + ibm_2g1->logpr(toks1, toks2);
   }
   return len_mismatch_wt * match_score - prior;
}

/**
 * Read next section (up to next mark, if supplied) from file.
 * @param file
 * @param lines place to put lines read
 * @return true as long as a valid section was read (even an empty one)
 */
bool read(istream& file, vector<string>& lines)
{
   if (file.eof()) return false;
   lines.clear();
   string line;
   while (getline(file, line))
      if (mark && line == mark)
         break;
      else
         lines.push_back(line);
   return true;
}

/**
 * Align two regions of text.
 * @param lines1 lines in the 1st region
 * @param lines2 lines in the 2nd region
 * @param set to s1+1 x s2+1 matrix of cumulative best scores for each
 * alignment position.
 * @param set to s1+1 x s2+1 matrix of best backlinks from each alignment
 * position.
 * @param connections set to list of indexes into matrix variables of positions
 * on best global alignment.
 */
void align(const vector<string>& lines1, const vector<string>& lines2,
           vector<double> &score_matrix, vector<Uint>& backlink_matrix,
           vector<Uint>& connections)
{
   Uint totlen1 = 0;
   for (vector<string>::const_iterator p = lines1.begin(); p != lines1.end(); ++p)
      totlen1 += p->length();
   Uint totlen2 = 0;
   for (vector<string>::const_iterator p = lines2.begin(); p != lines2.end(); ++p)
      totlen2 += p->length();

   const Uint dim2 = lines2.size()+1;
   Uint size = (lines1.size()+1) * dim2;
   if (score_matrix.size() < size) {
      score_matrix.resize(size);
      backlink_matrix.resize(size);
   }
// Workaround to avoid warning "error: statement has no effect"
// with gcc 4.5.0 on OpenSuSE 11.3 (JHJ)
   typedef double array_size_dim2_of_doubles[dim2];
   typedef Uint   array_size_dim2_of_Uint[dim2];
   array_size_dim2_of_doubles *
      score_mat = (array_size_dim2_of_doubles *) &score_matrix[0];
   array_size_dim2_of_Uint *
      backlink_mat = (array_size_dim2_of_Uint *) &backlink_matrix[0];
// replaces:
//   double (*score_mat)[dim2] = (double(*)[dim2]) &score_matrix[0];
//   Uint (*backlink_mat)[dim2] = (Uint(*)[dim2]) &backlink_matrix[0];

   score_mat[0][0] = 0.0;    // no cost starting point

   // dynamic programming

   for (Uint i = 0; i <= lines1.size(); ++i) {
      for (Uint j = 0; j <= lines2.size(); ++j) {
         if (i != 0 || j != 0) score_mat[i][j] = -DBL_MAX;
         for (Uint ibeg = i > maxsegs1 ? i-maxsegs1 : 0; ibeg <= i; ++ibeg) {
            for (Uint jbeg = j > maxsegs2 ? j-maxsegs2 : 0; jbeg <= j; ++jbeg) {
               if (jbeg == j && ibeg == i) continue;
               double score = score_mat[ibeg][jbeg] +
                  alignScore(lines1, lines2, ibeg, i, jbeg, j, totlen1, totlen2);
               if (score > score_mat[i][j]) {
                  score_mat[i][j] = score;
                  backlink_mat[i][j] = ibeg * dim2 + jbeg;
               }
            }
         }
      }
   }

   // backtrack

   connections.clear();
   Uint pos = lines1.size() * dim2 + lines2.size();
   while (pos > 0) {
      connections.push_back(pos);
      pos = backlink_mat[pos / dim2][pos % dim2];
   }
}

/**
 * Write an alignment.
 * @param region_num index, for writing separators
 * See align() for interpretation of next 5 parameters. The
 * connections list is emptied as alignments are written.
 * @param ofile1 destination for aligned lines1
 * @param ofile2 destination for aligned lines2
 * @param alfile destination for alignment info
 * @param idline id line for this region
 * @param oidfile sentence-aligned output id file
 */
void write(Uint region_num,
           const vector<string>& lines1, const vector<string>& lines2,
           const vector<double> &score_matrix, const vector<Uint>& backlink_matrix,
           vector<Uint>& connections,
           ostream& ofile1, ostream& ofile2, ostream* alfile,
           const string& idline, ostream* oidfile)
{
   const Uint dim2 = lines2.size()+1;
// Workaround to avoid warning "error: statement has no effect"
// with gcc 4.5 (JHJ)
   typedef double array_size_dim2_of_doubles[dim2];
   array_size_dim2_of_doubles *
      score_mat = (array_size_dim2_of_doubles *) &score_matrix[0];
//   double (*score_mat)[dim2] = (double(*)[dim2]) &score_matrix[0];

   if (region_num > 1 and mark and !filt_mark) {
      ofile1 << mark << endl;
      ofile2 << mark << endl;
      if (alfile)
         (*alfile) << mark << endl;
      if (oidfile)
         (*oidfile) << mark << endl;
   }

   Uint ibeg = 0, jbeg = 0;
   double begscore = 0.0;
   while (!connections.empty()) {
      Uint i = connections.back() / dim2;
      Uint j = connections.back() % dim2;
      if (!filt || keep(lines1, lines2, ibeg, i, jbeg, j)) {
         if (alfile)
            (*alfile) << ibeg << "-" << i << " " << jbeg << "-" << j << " "
                      << i-ibeg << "-" << j-jbeg << " "
                      << score_mat[i][j] - begscore << endl;
         if (oidfile)
            (*oidfile) << idline << endl;
         while (ibeg < i) {
            ofile1 << lines1[ibeg++];
            ofile1 << (ibeg == i ? "" : " ");
         }
         while (jbeg < j) {
            ofile2 << lines2[jbeg++];
            ofile2 << (jbeg == j ? "" : " ");
         }
         ofile1 << endl;
         ofile2 << endl;
      } else {
         ibeg = i;
         jbeg = j;
      }
      begscore = score_mat[i][j];
      connections.pop_back();
   }
}
