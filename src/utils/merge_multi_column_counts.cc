/**
 * @author Boxing Chen
 * @file merge_multi_column_counts.cc
 * @brief Merge sorted files by multi-counts which separated by " ||| ".
 *
 * modified based on Samuel Larkin's merge_counts.cc
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include "arg_reader.h"
#include "vector_map.h"
//#include "file_utils.h"
#include "merge_stream.h"
#include "printCopyright.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iterator>  // ostream_iterator

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
merge_multi_column_counts [-v][-d] OUTFILE INFILE(S)\n\
\n\
Merge multi-counts files.  Counts are at the end of each line, separated by\n\
\" ||| \" from the rest of the line.  Everything up to and including the last\n\
separator found is used as the merge key.  Input files must be LC_ALL=C sorted.\n\
\n\
Options:\n\
\n\
-d  Write debugging info. [don't]\n\
-v  Write progress reports to cerr. [don't]\n\
-a  Expect alignments with counts instead of multiple columns of counts\n\
    In this case, the result is the multi-set union of the alignments.\n\
    The format is a=<align>:<count>(;<align>:<count>)*.\n\
-top-a  Same as -a, but prints only the globally most frequent alignment,\n\
        without its count.  Ties are resolved arbitrarily.\n\
-fillup retains the first value encountered for a given key.\n\
";

// globals
const char* const PHRASE_TABLE_SEP = " ||| ";
const Uint sep_len = strlen(PHRASE_TABLE_SEP);


static bool bDebug = false;
static bool verbose = false;
static bool process_alignments = false;
static bool top_alignment = false;
static bool fillup = false;
static vector<string> infiles;
static string outfile("-");

static void getArgs(int argc, char* argv[]);

/**
 * Bundles the phrase and its counts.
 */
struct Datum {
   string key;       ///< phrase

   vector<Uint> counts;    ///< phrase's counts
   vector_map<string,Uint> alignments; ///< phrase's alignments
   Uint  stream_positional_id;   ///< This Datum is from what positional stream?

   Datum()
   : stream_positional_id(0)
   { }

   const string& getKey() const { return key; }

   void print(ostream& out) const {
      out << key;
      int i;
      for (i = 0; i < int(counts.size()) - 1; i++) {
         out << counts[i] << " ";
      }
      out << counts[i];
      if ( process_alignments && !alignments.empty() ) {
         out << " a=";
         if ( top_alignment ) {
            out << alignments.max()->first;
         } else {
            for ( vector_map<string,Uint>::const_iterator it(alignments.begin()), end(alignments.end());
                  it != end; ) {
               out << it->first;
               if ( it->second != 1 || alignments.size() != 1 )
                  out << ":" << it->second;
               if ( ++it != end ) out << ";";
            }
         }
      }
      out << endl;
   }

   bool parse(const string& buffer, Uint pId) {
      stream_positional_id = pId;

      const string::size_type pos = buffer.rfind(PHRASE_TABLE_SEP);
      if (pos == string::npos)
         error(ETFatal, "Invalid entry %s", buffer.c_str());

      // Check if the stream is LC_ALL=C sorted on the fly.
      key = buffer.substr(0, pos+sep_len);  // Keep the PHRASE_TABLE_SEP

      vector<string> allcounts;
      split(buffer.substr(pos+sep_len), allcounts);

      counts.clear();
      alignments.clear();

      Uint intcount;
      for (int j =0; j < int(allcounts.size()); j++){

         if (process_alignments && allcounts[j].compare(0,2,"a=") == 0) {
            vector<string> all_alignments;
            split(allcounts[j].substr(2), all_alignments, ";");
            for ( Uint i = 0; i < all_alignments.size(); ++i ) {
               string::size_type colon_pos = all_alignments[i].find(':');
               if ( colon_pos == string::npos ) {
                  static bool warning_displayed = false;
                  if ( !warning_displayed ) {
                     warning_displayed = true;
                     error(ETWarn, "alignments without counts are treated as 1's");
                  }
                  alignments[all_alignments[i]] += 1;
               }
               else {
                  if ( !convT(all_alignments[i].substr(colon_pos+1).c_str(), intcount) )
                     error(ETWarn, "Count is not a number %s in %s",
                           all_alignments[i].substr(colon_pos+1).c_str(),
                           buffer.c_str());
                  else
                     alignments[all_alignments[i].substr(0,colon_pos)] += intcount;
               }
            }
         }
         else if (!convT(allcounts[j].c_str(), intcount)) {
            error(ETWarn, "Count is not a number %s", allcounts[j].c_str());
         }
         else {
            counts.push_back(intcount);
         }
      }

      return true;
   }

   void debug(const char* const msg) const {
      if (bDebug) {
         cerr << msg << "\t";
         print(cerr);
      }
   }
   /**
    * Combines two keys.
    */
   Datum& operator+=(const Datum& other) {
      assert(key == other.key);
      assert(counts.size() == other.counts.size());
      if (fillup) {
         if (stream_positional_id > other.stream_positional_id) {
            *this = other;
         }
      }
      else {
         for (Uint i = 0; i < other.counts.size(); ++i) {
            counts[i] += other.counts[i];
         }
         if ( process_alignments )
            alignments += other.alignments;
      }

      return *this;
   }
   /**
    * Two datum are equal if they have the same key.
    */
   bool operator==(const Datum& other) const {
      return key == other.key;
   }
   /**
    * Orders datum according to their key.
    */
   bool operator<(const Datum& other) const {
      return key < other.key;
   }
};



// main
int main(int argc, char* argv[])
{
   printCopyright(2009, "merge_multi_column_counts");
   getArgs(argc, argv);

   fprintf(stderr, "Merging :\n");
   copy(infiles.begin(), infiles.end(), ostream_iterator<string>(cerr, " "));
   cerr << endl;


   mergeStream<Datum>  ms(infiles);
   oSafeMagicStream  out(outfile);

   while (!ms.eof()) {
      ms.next().print(out);
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "d", "a", "top-a", "fillup"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("d", bDebug);
   arg_reader.testAndSet("a", process_alignments);
   arg_reader.testAndSet("top-a", top_alignment);
   arg_reader.testAndSet("fillup", fillup);
   if ( top_alignment ) process_alignments = true;

   arg_reader.testAndSet(0, "outfile", outfile);
   arg_reader.getVars(1, infiles);

   // Check validity of params
   if (outfile.empty())
      error(ETFatal, "Invalid output file");
   if (infiles.empty())
      error(ETFatal, "You must provide at least one file to merge");
}
