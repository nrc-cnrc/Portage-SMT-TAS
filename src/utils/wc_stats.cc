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
static const char* header = "#Lines\t#Words\t#Char\tshortest line\tlongest line\tmean\tsdev\tfilename";

/**
 * Parses and cumulates stats on sentences.
 */
class countStats {
   private:
      Uint lines;   ///< Number of lines in the input.
      Uint words;   ///< Number of words in the input.
      unsigned long long characters;   ///< Number of characters in the input.
      Uint shortest_line;   ///< Length of the shortest line in the input.
      Uint longest_line;   ///< Length of the longest line in the input.
      vector<Uint> acc;   ///< Accumulator of line length to calculate mean & sdev.

   private:
      vector<string> tokens;    ///< The current tokens in that current line.

   public:
      /// Placeholder for verbose mode where we print stats per line.
      struct perLineStats {
         perLineStats(Uint lineNumber = 0, Uint numTokens = 0, Uint lineSize = 0)
         : lineNumber(lineNumber)
         , numTokens(numTokens)
         , lineSize(lineSize)
         {}

         Uint lineNumber;   ///< Line number.
         Uint numTokens;    ///< number of tokens on that line.
         Uint lineSize;     ///< number of charaters on that line.
      };

   public:
      /// Default constructor.
      countStats()
      : lines(0)
      , words(0)
      , characters(0)
      , shortest_line(0xFFFFFFFF)
      , longest_line(0)
      {}

      /**
       * Calculates and cumulates stats on a line.
       * @param line  line to analyse.
       * @return a structure wieht the line number, number of tokens and characters.
       */
      perLineStats cumulate(const string& line) {
         // Number of lines.
         ++lines;

         // Number or tokens in that line.
         tokens.clear();
         split(line, tokens);
         const Uint num_token = tokens.size();
         words += num_token;
         shortest_line = min(shortest_line, num_token);
         longest_line  = max(longest_line, num_token);
         acc.push_back(num_token);

         // Number of characers in that line.
         const Uint line_size = line.size() + 1;
         characters += line_size;

         return perLineStats(lines, num_token, line_size);
      }

      /**
       * Cumulates two countStats objects.
       * @param other  the other stats to cumulate.
       * @return the augmented stats.
       */
      countStats& operator+=(const countStats& other) {
         lines += other.lines;
         words += other.words;
         characters += other.characters;

         shortest_line = min(shortest_line, other.shortest_line);
         longest_line  = max(longest_line, other.longest_line);
         acc.reserve(acc.size() + other.acc.size());
         acc.insert(acc.end(), other.acc.begin(), other.acc.end());

         return *this;
      }

      /**
       * Displays the overall stats.
       * @param filename  stats apply to filename.
       */
      void fullPrint(const string& filename) const {
         const double v_mean = mean(acc.begin(), acc.end());
         const double v_sdev = Portage::sdev(acc.begin(), acc.end(), v_mean);
         printf("L:%d\tW:%d\tC:%llu\ts:%d\tl:%d\tm:%2.2f\tsdev:%2.2f\t%s\n",
           lines, words, characters, shortest_line, longest_line, v_mean, v_sdev, filename.c_str());
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
   for (Uint i(0); i<infiles.size(); ++i) {
      iSafeMagicStream istr(infiles[i]);

      string line;   // The current line.
      countStats docStats;
      countStats::perLineStats lineStats;
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
