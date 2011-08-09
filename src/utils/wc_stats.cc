// $Id$
/**
 * @author Samuel Larkin
 * @file wc_stats.cc 
 * @brief Like wc but we more info.
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include "file_utils.h"
#include "arg_reader.h"
#include "gfstats.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
wc_stats [options] INFILES\n\
\n\
  Like word count (wc) plus length of shortest and longest line, mean and sdev.\n\
\n\
Options:\n\
\n\
  -v    Write per line stat.\n\
";

// globals

static bool verbose = false;
static vector<string> infiles;
static void getArgs(int argc, char* argv[]);
static const char* header = "#Lines\t#Words\t#Char\tshortest L\tlongest L\tmean(W/L)\tsdev(W/L)\tfilename";



/**
 * Parses and cumulates stats on sentences.
 */
class countStats {
   // NOTE: s = square root of[(sum of Xsquared -((sum of X)*(sum of X)/N))/(N-1)]
   // Std Dev = sqrt((SUM[x^2] - SUM[x]^2)/n / (n-1))
   private:
      unsigned long long characters;   ///< Number of characters in the input.
      Uint shortest_line;   ///< Length of the shortest line in the input.
      Uint longest_line;   ///< Length of the longest line in the input.

      /// Object to keep track of counts in order to calculate mean & sdev.
      struct Counts {
         // The following are used to calculate the mean & sdev.
         unsigned long long sumX;         ///< Accumulator of X.
         unsigned long long sumXsquare;   ///< Accumulator of X square.
         unsigned long long n;            ///< Accumulator of number of samples.
         /**
          * Default constructor.
          */
         Counts() 
         : sumX(0)
         , sumXsquare(0)
         , n(0)
         { }
         /**
          * Add a data point to the stats pool.
          * @param X  a data point.
          */
         void add(unsigned long long X) {
            sumX       += X;
            sumXsquare += X * X;
            ++n;
         }
         /**
          * Combines/cumulates two Counts.
          * @param other  counts to cumulate.
          */
         void add(const Counts& other) {
            sumX       += other.sumX;
            sumXsquare += other.sumXsquare;
            n          += other.n;
         }

         /**
          * Reset the statistic accumulator to 0.
          */
         void clear() {
            sumX = sumXsquare = n = 0;
         }
         /**
          * Calculate the mean.
          * @return mean.
          */
         double mean() const {
            return _mean(sumX, n);
         }
         /**
          * Calculate the standard deviation.
          * @return standard deviation.
          */
         double sdev() const {
            return _sdev(sumX, sumXsquare, n);
         }
      };

      Counts tokenPerLine;  ///< Keeps track of number of tokens per line.
      Counts tokenPerDoc;   ///< Keeps track of number of tokens per document.
      Counts linePerDoc;    ///< Keeps track of number of lines per document.

   private:
      vector<char> tokens;  ///< The current token indicators in that current line.

   public:
      /// Placeholder for verbose mode where we print stats per line.
      struct perLineStats {
         perLineStats(Uint lineNumber = 0, Uint numTokens = 0, Uint lineSize = 0)
         : lineNumber(lineNumber)
         , numTokens(numTokens)
         , lineSize(lineSize)
         {}

         /**
          * Reset the counts to zero.
          */
         void clear() {
            lineNumber = 0;
            numTokens  = 0;
            lineSize   = 0;
         }

         Uint lineNumber;   ///< Line number.
         Uint numTokens;    ///< number of tokens on that line.
         Uint lineSize;     ///< number of charaters on that line.
      };

   public:
      /// Default constructor.
      countStats()
      : characters(0)
      , shortest_line(-1) // OK to use -1, casts to max unsigned value.
      , longest_line(0)
      {}

      /**
       * Reset the counts to zero.
       */
      void clear() {
         characters = 0;
         shortest_line = -1;
         longest_line = 0;
         tokenPerLine.clear();
         tokenPerDoc.clear();
         linePerDoc.clear();
      }

      /// DummyConverter for calling split to simply count tokens.
      struct DummyConverter {
         void operator()(const char* src, char& dest) { dest = 1; }
      };

      /**
       * Calculates and cumulates stats on a line.
       * @param line  line to analyse.
       * @return a structure wieht the line number, number of tokens and characters.
       */
      perLineStats cumulate(const string& line) {
         // Number or tokens in that line.
         tokens.clear();
         // Optimization: since we only count tokens, but don't inspect them,
         // convert them to dummy indicators instead of real strings.  This
         // change made the program run about 2.5x faster (i.e., a typical run
         // takes 40% of the time it used to take).
         const Uint num_token = split(line.c_str(), tokens, DummyConverter());
         shortest_line = min(shortest_line, num_token);
         longest_line  = max(longest_line, num_token);

         tokenPerLine.add(num_token);

         // Number of characers in that line.
         const Uint line_size = line.size() + 1;
         characters += line_size;

         return perLineStats(tokenPerLine.n, num_token, line_size);
      }

      /**
       * Cumulates two countStats objects.
       * @param other  the other stats to cumulate.
       * @return the augmented stats.
       */
      countStats& operator+=(const countStats& other) {
         characters += other.characters;

         shortest_line = min(shortest_line, other.shortest_line);
         longest_line  = max(longest_line, other.longest_line);

         tokenPerLine.add(other.tokenPerLine);
         tokenPerDoc.add(other.tokenPerLine.sumX);
         linePerDoc.add(other.tokenPerLine.n);

         return *this;
      }

      /**
       * Displays the overall stats.
       * @param filename  stats apply to filename.
       */
      void fullPrint(const string& filename) const {
         const double v_mean = tokenPerLine.mean();
         const double v_sdev = tokenPerLine.sdev();
         printf("L:%llu\tW:%llu\tC:%llu\ts:%u\tl:%u\tm:%2.2f\tsdev:%2.2f\t%s\n",
           tokenPerLine.n, tokenPerLine.sumX, characters, shortest_line, longest_line, v_mean, v_sdev, filename.c_str());

         // If we are a global stat object for all documents.
         if (linePerDoc.n > 0) {
            cout << "\n";
            printf("%2.2f words per doc with sdev of %2.2f\n", tokenPerDoc.mean(), tokenPerDoc.sdev());
            printf("%2.2f lines per doc with sdev of %2.2f\n", linePerDoc.mean(), linePerDoc.sdev());
            cout << linePerDoc.n << " documents in total" << endl;
         }
      }
};

// main

int main(int argc, char* argv[])
{
   printCopyright(2008, "wc_stats");
   getArgs(argc, argv);

   if (!infiles.empty() && !verbose) {
      cout << header << endl;
   }

   countStats allDocStats;
   string line;   // The current line.
   countStats docStats;
   countStats::perLineStats lineStats;
   for (Uint i(0); i<infiles.size(); ++i) {
      iSafeMagicStream istr(infiles[i]);

      line.clear();
      docStats.clear();
      lineStats.clear();
      while (getline(istr, line)) {
         lineStats = docStats.cumulate(line);

         if (verbose) {
            printf("%d\t%d\t%d\t%s\n", lineStats.lineNumber, lineStats.numTokens, lineStats.lineSize, line.c_str());
         }
      }

      // Print out stats for the current file.
      if (verbose) {
         cout << header << endl;
      }

      allDocStats += docStats;
      docStats.fullPrint(infiles[i].c_str());
   }
   
   printf("\n");
   allDocStats.fullPrint("TOTAL");
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);

   arg_reader.getVars(0, infiles);
   if (infiles.empty()) infiles.push_back("-");
}
