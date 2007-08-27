/**
 * @author George Foster
 * @file ssal.cc Simple sentence aligner
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <cfloat>
#include <cmath>
#include <file_utils.h>
#include <arg_reader.h>
#include <printCopyright.h>

using namespace Portage;

/// Program ssal (Simple sentence aligner) usage
static char help_message[] = "\n\
ssal [-vf][-a alfile] file1 file2\n\
\n\
Sentence-align file1 with file2, writing the results to file1.al and file2.al.\n\
Input files should have one segment per line, and output files are line-aligned.\n\
No further further assumptions are made about the input, although an attempt is\n\
made to align lines that are identical across file1 and file2.\n\
\n\
Options:\n\
\n\
-v  Write progress reports to cerr.\n\
-a  Write alignment info to <alfile>. Format is: b1-e1 b2-e2 n1-n2 s, indicating\n\
    that line numbers [b1,e1) from file1 align to lines [b2,e2) from file2, with\n\
    alignment pattern n1-n2, and score s (total alignment score is sum over all\n\
    sentences).\n\
-f  Filter out alignment pairs in which at least one member is markup (first\n\
    and last non-white characters are < and > respectively).\n\
";

// globals

static bool verbose = false;
static bool filt = false;
static string filename1, filename2;
static string alfilename;

/**
 * Parses the command line arguments.
 * @param argc  number of command line arguments
 * @param argv  vector of command line arguments
 */
static void getArgs(int argc, char* argv[]);

static Uint maxsegs1 = 3;      ///< max # file1 segs that can participate in an alignment
static Uint maxsegs2 = 3;      ///< max # file2 segs that can participate in an alignment

/**
 * Calulate a log-domain score for aligning the text in lines1 from [beg1,end1)
 * with that in lines2 from [beg2,end2).
 * @param lines1    text # 1
 * @param lines2    text # 2   
 * @param beg1      start index for lines1
 * @param end1      end+1 index for lines1
 * @param beg2      start index for lines2
 * @param end2      end+1 index for lines2
 * @param lenratio  num chars in lines1 / num chars in lines2
 * @return  score
 */
static double alignScore(const vector<string>& lines1, const vector<string>& lines2, 
                         Uint beg1, Uint end1, Uint beg2, Uint end2, double lenratio);


/**
 * If we're filtering output (-f), return true if we should keep the current
 * aligned pair. This checks for pairs whose members begin and end with
 * < and > respectively.
 */
static bool keep(const vector<string>& lines1, const vector<string>& lines2, 
                 Uint beg1, Uint end1, Uint beg2, Uint end2)
{
   static const char* const white = " \r\t\n";

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
      (b1 != string::npos && lines1[beg1][b1] == '<' && 
       e1 != string::npos && lines1[end1-1][e1] == '>' ||
       b2 != string::npos && lines2[beg2][b2] == '<' && 
       e2 != string::npos && lines2[end2-1][e2] == '>');
}

// main

int main(int argc, char* argv[])
{
   printCopyright(2006, "ssal");
   getArgs(argc, argv);

   // read and init

   IMagicStream file1(filename1);
   IMagicStream file2(filename2);

   vector<string> lines1, lines2;
   string line;
   Uint totlen1 = 0, totlen2 = 0;
   while (getline(file1, line)) {
      totlen1 += line.length();
      lines1.push_back(line);
   }
   while (getline(file2, line)) {
      totlen2 += line.length();
      lines2.push_back(line);
   }
   double lenratio = totlen1 / (double) totlen2;
   const Uint dim2 = lines2.size()+1;

   double (*score_matrix)[dim2] = (double(*)[dim2]) new double[(lines1.size()+1) * dim2];
   Uint (*backlink_matrix)[dim2] = (Uint(*)[dim2]) new Uint[(lines1.size()+1) * dim2];

   score_matrix[0][0] = 0.0;    // no cost starting point

   // dynamic programming

   if (verbose) cerr << "aligning " << filename1 << " and " << filename2 << endl;
   Uint size = (lines1.size()+1) * dim2;
   Uint it = 0;
   for (Uint i = 0; i <= lines1.size(); ++i) {
      for (Uint j = 0; j <= lines2.size(); ++j) {
         if (verbose && 100 * it / size != 100 * ++it / size) cerr << '.';
         if (i != 0 || j != 0) score_matrix[i][j] = -DBL_MAX;
         for (Uint ibeg = (i > maxsegs1 ? i-maxsegs1 : 0); ibeg <= i; ++ibeg) {
            for (Uint jbeg = (j > maxsegs2 ? j-maxsegs2 : 0); jbeg <= j; ++jbeg) {
               if (jbeg == j && ibeg == i) continue;
               double score = score_matrix[ibeg][jbeg] + 
                  alignScore(lines1, lines2, ibeg, i, jbeg, j, lenratio);
               if (score > score_matrix[i][j]) {
                  score_matrix[i][j] = score;
                  backlink_matrix[i][j] = ibeg * dim2 + jbeg;
               }
            }
         }
      }
   }
   
   // backtrack

   if (verbose) cerr << "\nbacktracking" << endl;
   vector<Uint> connections;
   Uint pos = lines1.size() * dim2 + lines2.size();
   while (pos > 0) {
      connections.push_back(pos);
      pos = backlink_matrix[pos / dim2][pos % dim2];
   }

   // output

   if (verbose) cerr << "writing" << (filt ? " filtered " : " ") << "output" << endl;
   OMagicStream ofile1(filename1 + ".al");
   OMagicStream ofile2(filename2 + ".al");
   OMagicStream* alfile = alfilename != "" ? new OMagicStream(alfilename) : NULL;

   Uint ibeg = 0, jbeg = 0;
   double begscore = 0.0;
   while (!connections.empty()) {
      Uint i = connections.back() / dim2;
      Uint j = connections.back() % dim2;
      if (!filt || keep(lines1, lines2, ibeg, i, jbeg, j)) {
         if (alfile)
            (*alfile) << ibeg << "-" << i << " " << jbeg << "-" << j << " "
                      << i-ibeg << "-" << j-jbeg << " "
                      << score_matrix[i][j] - begscore << endl;
         while (ibeg < i) ofile1 << lines1[ibeg++] << (ibeg == i ? "" : " ");
         while (jbeg < j) ofile2 << lines2[jbeg++] << (jbeg == j ? "" : " ");
         ofile1 << endl;
         ofile2 << endl;
      } else {
         ibeg = i;
         jbeg = j;
      }
      begscore = score_matrix[i][j];
      connections.pop_back();
   }

   if (alfile) delete alfile;
}

static double alignScore(const vector<string>& lines1, const vector<string>& lines2, 
                         Uint beg1, Uint end1, Uint beg2, Uint end2,
                         double lenratio) 
{
   Uint n1 = end1 - beg1, n2 = end2 - beg2;
   if (n1 == 1 && n2 == 1 && lines1[beg1] == lines2[beg2])
      return 0.0;    // special case: single exact match.

   Uint len1 = 0, len2 = 0;
   for (Uint i = beg1; i < end1; ++i) len1 += lines1[i].length();
   for (Uint i = beg2; i < end2; ++i) len2 += lines2[i].length();
   double z = len1 / (double) (len2 + 0.001) - lenratio;

   if (n1 > n2) swap(n1,n2);
   double prior = (n1 == 0 ? 1 : n1 * n1) + n2-n1;

   return - z * z - prior;
}

// arg processing

static void getArgs(int argc, char* argv[])
{
   const char* const switches[] = {"v", "f", "a:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 2, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("f", filt);
   arg_reader.testAndSet("a", alfilename);
   
   arg_reader.testAndSet(0, "file1", filename1);
   arg_reader.testAndSet(1, "file2", filename2);
}
