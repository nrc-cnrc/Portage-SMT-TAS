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
#include "banded_matrix.h"
#include <memory> // for auto_ptr
#include <limits>  // numeric_limits::max
#include <utility>  // pair

using namespace Portage;

/// Program ssal (Simple sentence aligner) usage
static char help_message[] = "\n\
ssal [-vH][-hm mark][-p][-m1 m][-m2 m][-w w][-ibm_1g2 model][-ibm_2g1 model]\n\
     [-l][-lc locale][-f][-fm][-a alfile][-rel_a][-i idfile]\n\
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
-d  Write debugging info to cerr.\n\
-x  Write alignment matrix to trace what happened.\n\
-H  Print some guidelines on multi-pass alignment, then quit.\n\
-hm Interpret <mark>, when alone on a line, as hard markup: align sentence\n\
    pairs between consecutive marks, never across them. It is an error for one\n\
    input file to contain more hard marks than the other. By default, <mark>\n\
    pairs are written to all output files, but this can be changed with -fm.\n\
-p  Paragraph mode: two consecutive newlines form a hard markup. Intended to\n\
    process the output of utokenize.pl -ss -p. Different from -hm \"\" in that\n\
    an empty paragraph is still treated as just one paragraph, not two.\n\
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
    (With -p, remove the second newline marking the paragraph boundary.)\n\
-a  Write alignment info to <alfile>. Format is: b1-e1 b2-e2 n1-n2 s,\n\
    indicating that line numbers [b1,e1) from file1 align to lines [b2,e2)\n\
    from file2, with alignment pattern n1-n2, and score s (the total alignment\n\
    score is the sum over all sentences).\n\
-rel_a In <alfile>, by default, b1, e1, b2, e2 are lines numbers in the file.\n\
    With -rel_a, make them relative to the last hard marker.\n\
-i  Read one id line from <idfile> for each region identified by hard markup,\n\
    and write one copy of the line to <idfile>.al for each sentence pair in the\n\
    region after alignment (<idfile>.al will be line-aligned to the other .al\n\
    files). NB: <idfile> is expected to contain exactly one line for each hard\n\
    region, even empty ones. This means, for instance, that it must contain 1\n\
    line if file1/file2 are empty. Due to the line-alignment requirement for\n\
    output, however, id lines for empty regions are not written to <idfile>.al.\n\
-o1 output filename for file 1 [basename(file1).al].\n\
-o2 output filename for file 2 [basename(file2).al].\n\
-ne Replace empty alignment with the tag <EMPTY> [no]\n\
-b  Beam width [0.0, 1.0) which is the number of line over and under the diagonal [0.005].\n\
-g  Growth factor (1.0, inf) by which ssal should increase the beam width [1.5].\n\
-mw Minimum beam width in number of lines [5].\n\
-c  How close, in number of lines, we can get from the edge of the explored\n\
    band without triggering a growth of the band.[2]\n\
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
   boundaries as hard constraints. Note: use -p rather than -hm \"\", in order\n\
   to process empty paragraphs correctly (eg: 0-1 alignments from step 2), eg:\n\
\n\
   ssal -p -fm -a al.len en.sent fr.sent\n\
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
   ssal -p -fm -l -a al.ibm \\\n\
        -ibm_1g2 hmm.en_given_fr.bin -ibm_2g1 hmm.fr_given_en.bin \\\n\
        en.sent fr.sent\n\
\n\
   (Or, even better, redo steps 2-4 using -ibm_*, to realign paragraphs too.)\n\
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
\n\
8) Optionally, you can reconstruct the original, non-tokenized parallel corpus:\n\
\n\
   At step 3, sentence split and tokenize the corpus in two separate steps:\n\
      utokenize.pl -notok -ss -paraline -p -lang=LL LL.para.al | head -n -1 > LL.sent\n\
      utokenize.pl -noss -lang=LL LL.sent > LL.sent.tok\n\
   Run steps 4, 5 and 6 on LL.sent.tok instead of LL.sent.\n\
   Apply the alignments from al.ibm on the original sentences:\n\
      select-lines.py -a 1 al.ibm en.sent > en.sent.al\n\
      select-lines.py -a 2 al.ibm fr.sent > fr.sent.al\n\
";

/**
 * A class to automatically set double's default initial value to -inf.
 */
class Double
{
   public :
      Double() : x(-numeric_limits<double>::infinity()) {}
      Double(double x) : x(x) {}

      operator double const &() const { return x ; }
      operator double&() { return x ; }

      Double& operator = (double v) {
         x = v;
         return *this;
      }

   private :
      double x;
};

typedef pair<Uint, Uint>      Coord;
typedef BandedMatrix<Double>  ScoreMatrix;
typedef BandedMatrix<Coord>   BacklinkMatrix;

namespace std {
   /**
    * Writes a Coord object to an output stream.
    */
   ostream& operator<<(ostream& s, const Coord& c) {
      s << "(" << c.first << ", " << c.second << ")";
      return s;
   }
};

// globals

static bool debug = false;
static bool verbose = false;
static bool showMatrix = false;
static bool filt = false;
static bool filt_mark = false;
static string filename1, filename2;
static string ofilename1, ofilename2;
static string alfilename;
static bool al_relative = false;
static string idfilename;
static string oidfilename;
static Uint maxsegs1 = 3;      ///< max # file1 segs that can participate in an alignment
static Uint maxsegs2 = 3;      ///< max # file2 segs that can participate in an alignment
static bool len_mismatch_set = false;
static double len_mismatch_wt = 10.0;
static const char* mark = NULL; // optional hard marker
static bool paragraph = false;
static string ibm_1g2_filename;
static string ibm_2g1_filename;
static bool lowercase = false;
static string lc_locale;
const string default_locale = "en_US.UTF-8";
static IBM1 *ibm_1g2 = NULL;
static IBM1 *ibm_2g1 = NULL;
static auto_ptr<CaseMapStrings> mapr;
static bool noEmpty = false;

static float beamWidth = 0.005f;
static double growthFactor = 1.5;
static Uint minimumWidth = 100;
static Uint closeness = 10;



static void getArgs(int argc, char* argv[]);
static double alignScore(const vector<string>& lines1, const vector<string>& lines2,
                         Uint beg1, Uint end1, Uint beg2, Uint end2,
                         Uint totlen1, Uint totlen2);
static bool keep(const vector<string>& lines1, const vector<string>& lines2,
                 Uint beg1, Uint end1, Uint beg2, Uint end2);
static bool read(istream& file, vector<string>& lines);
static bool align(const vector<string>& lines1, const vector<string>& lines2,
                  ScoreMatrix& score_matrix, BacklinkMatrix& backlink_matrix,
                  vector<Coord>& connections, bool reversed);
static void write(Uint region_num,
                  const vector<string>& lines1, const vector<string>& lines2,
                  Uint s1_offset, Uint s2_offset,
                  const ScoreMatrix& score_matrix,
                  vector<Coord>& connections,
                  ostream& file1, ostream& file2, ostream* alfile,
                  const string& idline, ostream* oidfile,
                  bool reversed);

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


   oSafeMagicStream ofile1(ofilename1);
   oSafeMagicStream ofile2(ofilename2);
   oSafeMagicStream* alfile = alfilename.empty() ? NULL :
      new oSafeMagicStream(alfilename);
   oSafeMagicStream* oidfile = oidfilename.empty() ? NULL :
      new oSafeMagicStream(oidfilename);

   if (ibm_1g2_filename != "") ibm_1g2 = new IBM1(ibm_1g2_filename);
   if (ibm_2g1_filename != "") ibm_2g1 = new IBM1(ibm_2g1_filename);

   if (ibm_1g2 && !len_mismatch_set) len_mismatch_wt = 0.4;

   // align and write output

   ScoreMatrix score_matrix;
   BacklinkMatrix backlink_matrix;
   vector<Coord> connections;
   vector<string> lines1, lines2;
   string idline;
   Uint region_num = 0;

   // lines processed before current block, when using hard markers
   Uint s1_offset = 0;
   Uint s2_offset = 0;

   while (true) {
      bool s1 = read(file1, lines1);
      bool s2 = read(file2, lines2);
      bool si = idfile ? getline(*idfile, idline) : true;
      if (s1 && s2 && si) {
         ++region_num;
         if (verbose) cerr << "\naligning region " << region_num << endl;

         // We need the bigger set of segments to be the rows for the BandedMatrix.
         const bool reversed = lines1.size() < lines2.size();
         //cerr << (reversed ? "" : "NOT ") << "reversed" << endl;  // SAM DEBUGGING
         const vector<string>& rlines1 = !reversed ? lines1 : lines2;
         const vector<string>& rlines2 = !reversed ? lines2 : lines1;

         double growth = 1.0;
         Uint numGrowth = 0;
         do {
//            // Make sure that we will not overflow when storing the backlink.
//            if (static_cast<Uint64>(rlines1.size()+1) * static_cast<Uint64>(rlines2.size()+1) > numeric_limits<Uint>::max())
//               error(ETFatal, "Overflow of matrix sizes.  Cannot safely create matrix (%d, %d).", rlines1.size()+1, rlines2.size()+1);

            const Uint width = growth*max(Uint(beamWidth*rlines2.size()), minimumWidth);
            if (growth > 1.0) {
               cerr << "Widening the beam to width " << width << " for the " << numGrowth << " times";
               if (idfile) cerr << " for id " << idline;
               cerr << endl;
            }

            score_matrix.resize(rlines1.size()+1, rlines2.size()+1, width);
            backlink_matrix.resize(rlines1.size()+1, rlines2.size()+1, width);

            if (debug) {
               cerr << "growth: " << growth << endl;  // SAM DEBUGGING
               cerr << "width: " << width << endl;  // SAM DEBUGGING
            }
            growth *= growthFactor;
            ++numGrowth;
         } while (!align(rlines1, rlines2, score_matrix, backlink_matrix, connections, reversed));

         write(region_num, lines1, lines2, s1_offset, s2_offset, score_matrix,
               connections, ofile1, ofile2, alfile, idline, oidfile, reversed);

         s1_offset += lines1.size()+1;
         s2_offset += lines2.size()+1;
      }
      else if (s1 == false && s2 == false && (idfile == NULL || si == false)) {
         break;  // end of file
      }
      else {   // asymmetric eof
         string shortones;
         if (!s1) shortones += filename1 + " ";
         if (!s2) shortones += filename2 + " ";
         if (!si) shortones += idfilename;
         error(ETFatal, "file(s) end prematurely: %s", shortones.c_str());
         break;
      }
   }

   if (mark)
      cerr << "Aligned " << region_num << " regions totalling "
           << (s1_offset-1) << "/" << (s2_offset-1) << " lines." << endl;

   // cleanup (safe as is, without guard, even when pointers are NULL)
   delete alfile;
   delete idfile;
   delete oidfile;
   delete ibm_1g2;
   delete ibm_2g1;
}


/**
 * Change a filename to its align form.
 * @param file  original filename to convert to align filename.
 * @return  alignment filename.
 */
string getAlignFilename(const string& file) {
   // repalce .arc.gz with .al.gz
   return file.empty() ? "" : addExtension(extractFilename(file), ".al");
}


/**
 * Parses the command line arguments.
 * @param argc  number of command line arguments
 * @param argv  vector of command line arguments
 */
static void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"d", "v", "f", "fm", "hm:", "p", "a:", "rel_a", "i:", "m1:", "m2:",
                             "w:", "ibm_1g2:", "ibm_2g1:", "l", "lc:",
                             "o1:", "o2:", "oid:", "ne", "x",
                             "b:", "g:", "c:", "mw:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 2, help_message, "-h",
                        false, alt_help, "-H");
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("v", debug);
   arg_reader.testAndSet("x", showMatrix);
   arg_reader.testAndSet("f", filt);
   arg_reader.testAndSet("fm", filt_mark);

   arg_reader.testAndSet("d", debug);

   // Search speed optimization related values.
   arg_reader.testAndSet("b", beamWidth);
   arg_reader.testAndSet("g", growthFactor);
   arg_reader.testAndSet("mw", minimumWidth);
   arg_reader.testAndSet("c", closeness);
   // Valid cases:
   // - beamwidth = 0% && minimumWidth > 0
   // - minimumWidth = 0 && beamWidth > 0
   if (minimumWidth == 0 && beamWidth == 0)
      error(ETFatal, "Beam width and minimum width must be greater than 0!");
   // 0 <= beamWidth < 1.0
   if (0.0 > beamWidth || beamWidth >= 1.0)
      error(ETFatal, "Beam width's value (%f) must be [0.0, 1.0)", beamWidth);
   // 0 < closeness < beamWidth
   if (closeness >= minimumWidth)
      error(ETFatal, "Beam's minimum width (%d) must be greater than the closeness factor (%d)\n 0 < %d < %d", minimumWidth, closeness, closeness, minimumWidth);
   // 1.0 < growthFactor
   if (growthFactor <= 1.0)
      error(ETFatal, "The growth factor (%f) must always be greater than 1!", growthFactor);

   static string markstring;
   if (arg_reader.getSwitch("hm", &markstring))
      mark = markstring.c_str();
   arg_reader.testAndSet("p", paragraph);
   if (paragraph) {
      if (mark)
         error(ETFatal, "Conflicting -p and -hm both specified. Remove one of them.");
      static const char empty[] = "";
      mark = empty;
   }

   arg_reader.testAndSet("a", alfilename);
   arg_reader.testAndSet("rel_a", al_relative);
   arg_reader.testAndSet("i", idfilename);
   arg_reader.testAndSet("m1", maxsegs1);
   arg_reader.testAndSet("m2", maxsegs2);
   len_mismatch_set = arg_reader.getSwitch("w");
   arg_reader.testAndSet("w", len_mismatch_wt);
   arg_reader.testAndSet("ibm_1g2", ibm_1g2_filename);
   arg_reader.testAndSet("ibm_2g1", ibm_2g1_filename);
   arg_reader.testAndSet("l", lowercase);
   arg_reader.testAndSet("lc", lc_locale);
   noEmpty = arg_reader.getSwitch("ne");
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

   ofilename1 = getAlignFilename(filename1);
   arg_reader.testAndSet("o1", ofilename1);
   ofilename2 = getAlignFilename(filename2);
   arg_reader.testAndSet("o2", ofilename2);
   oidfilename = getAlignFilename(idfilename);
   arg_reader.testAndSet("oid", oidfilename);
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
      assert(i<lines2.size());   // SAM DEBUGGING
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
      const double n = max(max(len1,len2), max(totlen1,totlen2)); // largest component
      const double l1 = len1/n, l2 = len2/n, tl1 = totlen1/n, tl2 = totlen2/n;
      const double z = sqrt(l1*l1 + l2*l2) * sqrt(tl1*tl1 + tl2*tl2);
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
      if (mark && line == mark && (!paragraph || !lines.empty()))
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
 * @return true if the alignemnt didn't come to close to an edge.
 */
bool align(const vector<string>& lines1, const vector<string>& lines2,
           ScoreMatrix& score_matrix, BacklinkMatrix& backlink_matrix,
           vector<Coord>& connections, bool reversed)
{
   Uint totlen1 = 0;
   for (vector<string>::const_iterator p = lines1.begin(); p != lines1.end(); ++p)
      totlen1 += p->length();
   Uint totlen2 = 0;
   for (vector<string>::const_iterator p = lines2.begin(); p != lines2.end(); ++p)
      totlen2 += p->length();

   score_matrix.set(0, 0, 0.0);    // no cost starting point

   // dynamic programming
   for (Uint i = 0; i <= lines1.size(); ++i) {
      for (Uint j = score_matrix.minRange(i); j < score_matrix.maxRange(i); ++j) {
         if (i != 0 || j != 0) score_matrix.set(i, j, -DBL_MAX);
         if (showMatrix) cerr << "i=" << i << " j=" << j;
         for (Uint ibeg = i > maxsegs1 ? i-maxsegs1 : 0; ibeg <= i; ++ibeg) {
            for (Uint jbeg = j > maxsegs2 ? j-maxsegs2 : 0; jbeg <= j; ++jbeg) {
               if (jbeg == j && ibeg == i) continue;
               const double score = score_matrix.get(ibeg, jbeg) +
                  (reversed
                  ? alignScore(lines2, lines1, jbeg, j, ibeg, i, totlen2, totlen1)
                  : alignScore(lines1, lines2, ibeg, i, jbeg, j, totlen1, totlen2));
               if (showMatrix) cerr << " (m=" << ibeg << " n=" << jbeg << " s=" << score << ")";
               if (score > score_matrix.get(i, j)) {
                  score_matrix.set(i, j, score);
                  backlink_matrix.set(i, j, Coord(ibeg, jbeg));
               }
            }
         }
         if (showMatrix) {
            const Coord backIndex = backlink_matrix.get(i, j);
            const Uint m =  backIndex.first;
            const Uint n =  backIndex.second;
            cerr << " bs: " << score_matrix.get(i, j) << " bl: " << m << "," << n << endl;
         }
      }
   }

   if (debug) {
      if (!reversed) {
         score_matrix.print(cerr);
      }
      else {
         const streamsize old = cerr.precision();
         cerr.precision(5);
         cerr << "size1:" << score_matrix.size1() << "; size2:" << score_matrix.size2() << "; m:" << score_matrix.getM() << "; b:" << score_matrix.getB();
         cerr << "; data.size:" << score_matrix.getDataSize() << "/" << score_matrix.size1()*score_matrix.size2() << endl;
         //for (Uint i(0); i<index.size(); ++i) cerr << index[i] << endl;  // SAM DEBUGGING
         for (Uint i(0); i<score_matrix.size2(); ++i) {
            cerr << score_matrix.get(0, i);
            for (Uint j(1); j<score_matrix.size1(); ++j) {
               cerr << "\t" << score_matrix.get(j, i);
            }
            cerr << endl;
         }
         cerr.precision(old);  // Reset the precision to what it was.
      }
      backlink_matrix.print(cerr);
   }

   // backtrack
   connections.clear();
   Coord pos(lines1.size(), lines2.size());
   while (pos.first > 0 && pos.second > 0) {
      connections.push_back(pos);

      const Uint i = pos.first;
      const Uint j = pos.second;

      if (debug) cerr << "[" << pos.first << ", " << pos.second << "]\t";

      // i & j must always be in range or else there is a problem with the
      // alignment algorithn or the BandedMatrix class.
      if (!score_matrix.inRange(i, j)) {
         if (debug) {
            score_matrix.print(cerr);
            backlink_matrix.print(cerr);
         }
         error(ETFatal, "Something went horribly wrong with the alignment process (%d, %d).", i, j);
      }

      // We went outside the the band, this iteration is no longer valid.  We
      // must widen the band and restart aligning.
      if (score_matrix.get(i, j) == -numeric_limits<double>::infinity()) {
         if (score_matrix.inRange(i,j)) cerr << "says in range" << endl;
         // SAM DEBUGGING START
         if (debug) {
            cerr << "We went outside the band." << endl;
            cerr << score_matrix.minRange(i) << ", " << score_matrix.maxRange(i) << endl;
            cerr << "Hitting a wall at " << i << ", " << j << endl;  // SAM DEBUGGING
            cerr << score_matrix.get(i, j) << endl;
            cerr << backlink_matrix.get(i, j) << endl;
         }
         // SAM DEBUGGING END
         return false;
      }

      // We've walked to close to the edge we should widen the band and restart aligning.
      // NOTE: if there is no room to grown the beam, we keep on truckin.
      // TODO: check that the type of the operands are not going to cause problems in the calculation.
      if (score_matrix.getB()+1 < score_matrix.size2()  // Still room to grow the beam?
          // are we too close to the edge?
          && score_matrix.getB() - closeness < Uint(abs(int(j)-int(score_matrix.diag(i))))) {
         // SAM DEBUGGING START
         if (debug) {
            cerr << endl;
            cerr << "We went too close to the edge." << endl;
            cerr << "i: " << i << " j: " << j << endl;
            cerr << "B: " << score_matrix.getB() << endl;
            cerr << "closeness: " << closeness << endl;
            cerr << "diag: " << score_matrix.diag(i) << endl;
            cerr << "|j-diag(i)| = " << abs(int(j)-int(score_matrix.diag(i))) << endl;
            cerr << "b-|j-diag(i)| = " << score_matrix.getB() - abs(int(j)-int(score_matrix.diag(i))) << endl;
         }
         // SAM DEBUGGING END
         return false;
      }

      pos = backlink_matrix.get(i, j);
   }

   // For completeness sake.
   if (debug) cerr << "[" << pos.first << ", " << pos.second << "]\t";

   return true;
}


/**
 * Write an alignment.
 * @param region_num index, for writing separators
 * See align() for interpretation of next 5 parameters. The
 * connections list is emptied as alignments are written.
 * @param ofile1 destination for aligned lines1
 * @param ofile2 destination for aligned lines2
 * @param s1_offset offset for lines1 line numbers in their file
 * @param s2_offset offset for lines2 line numbers in their file
 * @param alfile destination for alignment info
 * @param idline id line for this region
 * @param oidfile sentence-aligned output id file
 */
void write(Uint region_num,
           const vector<string>& lines1, const vector<string>& lines2,
           Uint s1_offset, Uint s2_offset,
           const ScoreMatrix& score_matrix,
           vector<Coord>& connections,
           ostream& ofile1, ostream& ofile2, ostream* alfile,
           const string& idline, ostream* oidfile,
           bool reversed)
{
   if (region_num > 1 and mark and !filt_mark) {
      ofile1 << mark << endl;
      ofile2 << mark << endl;
      if (alfile)
         (*alfile) << mark << endl;
      if (oidfile)
         (*oidfile) << mark << endl;
   }

   // Zero-out the file offsets when using -rel_a, so line numbers are printed
   // with respect to the current block rather than the whole file
   if (al_relative)
      s1_offset = s2_offset = 0;

   Uint ibeg = 0, jbeg = 0;
   double begscore = 0.0;
   while (!connections.empty()) {
      const Uint i = reversed ? connections.back().second : connections.back().first;
      const Uint j = reversed ? connections.back().first  : connections.back().second;
      const double score = reversed ? score_matrix.get(j, i) : score_matrix.get(i, j);
      if (!filt || keep(lines1, lines2, ibeg, i, jbeg, j)) {
         if (alfile)
            (*alfile) << ibeg+s1_offset << "-" << i+s1_offset << " "
                      << jbeg+s2_offset << "-" << j+s2_offset << " "
                      << i-ibeg << "-" << j-jbeg << " "
                      << score - begscore << endl;
         if (oidfile)
            (*oidfile) << idline << endl;

         if (noEmpty && ibeg == i) ofile1 << "<EMPTY>";
         while (ibeg < i) {
            ofile1 << lines1[ibeg++];
            ofile1 << (ibeg == i ? "" : " ");
         }

         if (noEmpty && jbeg == j) ofile2 << "<EMPTY>";
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
      begscore = score;
      connections.pop_back();
   }
}

