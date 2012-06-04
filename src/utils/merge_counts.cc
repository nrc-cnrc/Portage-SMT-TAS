/**
 * @author Samuel Larkin
 * @file merge_counts.cc
 * @brief Merge sorted files by tallying counts.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include "printCopyright.h"
#include "file_utils.h"
#include "arg_reader.h"
#include "merge_stream.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
merge_counts [-v][-d] OUTFILE INFILE(S)\n\
\n\
Merge count files.  The count must be at the end of each line, seperated by at\n\
least one space.  Input files must be LC_ALL=C sorted.\n\
\n\
Options:\n\
\n\
-t  Use a tab character instead of a space as delimiter.\n\
-d  Write debugging info. [don't]\n\
-v  Write progress reports to cerr. [don't]\n\
";

// globals

static bool bDebug = false;
static bool verbose = false;
static char delimiter = ' ';
static vector<string> infiles;
static string outfile("-");

static void getArgs(int argc, char* argv[]);


/**
 * Bundles the phrase and its counts.
 */
struct Datum {
   string key;   ///< phrase
   Uint   count;    ///< phrase's count

   /// Default constructor.
   Datum()
   : count(0xBAD)
   {}

   /// Stream need the key to validate the order of it underlying input file.
   const string& getKey() const { return key; }

   bool parse(const string& buffer, Uint pId) {
      const string::size_type pos = buffer.rfind(delimiter);
      if (pos == string::npos)
         error(ETFatal, "Invalid entry %s", buffer.c_str());

      // Check if the stream is LC_ALL=C sorted on the fly.
      key = buffer.substr(0, pos+1);

      if (!convT(buffer.substr(pos+1).c_str(), count))
         error(ETWarn, "Count is not a number: %s", buffer.substr(pos).c_str());

      return true;
   }

   void print(ostream& out) const {
      out << key << count << endl;
   }

   void debug(const char* const msg) const {
      if (bDebug) {
         cerr << msg << "\t";
         print(cerr);
      }
   }
   Datum& operator+=(const Datum& other) {
      count += other.count;
      return *this;
   }
   bool operator==(const Datum& other) const {
      return key == other.key;
   }
   bool operator<(const Datum& other) const {
      return key < other.key;
   }
};



// main
int main(int argc, char* argv[])
{
   printCopyright(2008, "merge_counts");
   getArgs(argc, argv);

   cerr << "Merging: " << endl << join(infiles) << endl;

   mergeStream<Datum>  ms(infiles);
   oSafeMagicStream    out(outfile);

   while (!ms.eof()) {
      ms.next().print(out);
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "d", "t"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("d", bDebug);
   bool tab_delim(false);
   arg_reader.testAndSet("t", tab_delim);
   if ( tab_delim ) delimiter = '\t';

   arg_reader.testAndSet(0, "outfile", outfile);
   arg_reader.getVars(1, infiles);

   // Check validity of params
   if (outfile.empty())
      error(ETFatal, "Invalid output file");
   if (infiles.empty())
      error(ETFatal, "You must provide at least of file to merge");
}
