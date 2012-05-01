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

#include "file_utils.h"
#include "arg_reader.h"
#include "vector_map.h"
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <iterator>

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
";

// globals
const char* PHRASE_TABLE_SEP = " ||| ";

static bool bDebug = false;
static bool verbose = false;
static bool process_alignments = false;
static bool top_alignment = false;
static vector<string> infiles;
static string outfile("-");

static void getArgs(int argc, char* argv[]);


/**
 * Keeps track of multiple streams and only allows access to the smallest
 * stream which is the stream with the smallest phrase in ascii order.
 */
class mergeStream
{
   public:
      /**
       * Bundles the phrase and its counts.
       */
      struct Datum {
         string prefix;   ///< phrase
         string suffix;   ///< phrase
         vector<Uint> counts;    ///< phrase's counts
         vector_map<string,Uint> alignments; ///< phrase's alignments

         void print(ostream& out) const {
            out << prefix;
            int i;
            for (i = 0; i < int(counts.size()) - 1; i++) {
               out << counts[i] << " ";
            }
            out << counts[i] << suffix;
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

         void debug(const char* const msg) const {
            if (bDebug) {
               cerr << msg << "\t";
               print(cerr);
            }
         }
         bool operator==(const Datum& other) const {
            return prefix == other.prefix && suffix == other.suffix;
         }
         bool operator<(const Datum& other) const {
            return string(prefix + suffix) < string(other.prefix + other.suffix);
         }
      };

   private:
      /**
       * Reads, parses and track each datum from a file.
       */
      class Stream {
         private:
            const string       file;   ///< Filename
            Uint               lineno; ///< Current line number
            iSafeMagicStream   input;  ///< file stream
            Datum*             _data;  ///< Datum
            string             buffer; ///< line buffer
            /// We need a placeholder to keep track of the entries to check if
            /// the stream is LC_ALL=C sorted.
            string             prev_entry;

         private:
            Stream(const Stream&); ///< Noncopyable
            Stream& operator=(const Stream&); ///< Noncopyable

         public:
            /// Default constructor
            Stream(const string& file)
            : file(file)
            , lineno(0)
            , input(file)
            , _data(new Datum)
            {
               // Triggers reading the first record which is required to have
               // the stream ordered.
               get();
            }

            /// Destructor.
            ~Stream() { delete _data; }

            bool eof() const { return input.eof(); }

            /**
             * Swaps this stream's buffer with spare.
             * This stream will now use spare internally.
             * This allows use to minimize memory allocation/copy.
             * @param spare a datum buffer to give to this stream.
             */
            void swap_buffer(Datum*& spare) {
                assert(spare != NULL);
                swap(_data, spare);
            }

            void get() {
               const Uint sep_len = strlen(PHRASE_TABLE_SEP);

               if (getline(input, buffer)) {
                  ++lineno;
                  const string::size_type pos = buffer.rfind(PHRASE_TABLE_SEP);
                  if (pos == string::npos)
                     error(ETFatal, "Invalid entry %s", buffer.c_str());

                  // Check if the stream is LC_ALL=C sorted on the fly.
                  _data->prefix = buffer.substr(0, pos+sep_len);  // Keep the PHRASE_TABLE_SEP
                  if (prev_entry > _data->prefix) {
                     error(ETFatal, "%s is not LC_ALL=C sorted at line %u:\n%s\n%s",
                           file.c_str(), lineno, prev_entry.c_str(), _data->prefix.c_str());
                  }

                  prev_entry = _data->prefix;

                  vector<string> allcounts;
                  split(buffer.substr(pos+sep_len), allcounts);

                  _data->counts.clear();
                  _data->alignments.clear();

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
                              _data->alignments[all_alignments[i]] += 1;
                           } else {
                              if ( !convT(all_alignments[i].substr(colon_pos+1).c_str(), intcount) )
                                 error(ETWarn, "Count is not a number %s in %s",
                                       all_alignments[i].substr(colon_pos+1).c_str(),
                                       buffer.c_str());
                              else
                                 _data->alignments[all_alignments[i].substr(0,colon_pos)] += intcount;
                           }
                        }
                     } else if (!convT(allcounts[j].c_str(), intcount)) {
                        error(ETWarn, "Count is not a number %s", allcounts[j].c_str());
                     } else {
                        _data->counts.push_back(intcount);
                     }
                  }
               }
            }

            bool operator<(const Stream& other) const {
               // Make sure the exhausted streams are at the end of the queue
               if (other.eof()) return true;
               if (eof()) return false;
               return *(_data) < *(other._data);
            }

            struct bestThan {
               bool operator()(const Stream* const a, const Stream* const b) {
                  // false when equal
                  return !(*a < *b);
               }
            };
      };

      /// Keeps track of the next "smallest" stream to get datum from.
      priority_queue<Stream*, vector<Stream*>, Stream::bestThan> queue;

      /// Last datum read which also acts has a buffered input.
      Datum* last_datum_read;
      /// Used to tally counts for a particular source.  Spares some allocations.
      Datum* total;
      /// Indicates if there are datums to process in the stream.
      bool finished;

   private:
      /// Deactivated copy constructor.
      mergeStream(const mergeStream&);
      /// Deactivated assignment operator.
      mergeStream& operator=(const mergeStream&);

   public:
      /**
       * Default constructor.
       * @param infiles  list of filenames of stream to process.
       */
      mergeStream(const vector<string>& infiles)
      : last_datum_read(new Datum)
      , total(new Datum)
      , finished(false)
      {
         for (Uint i(0); i<infiles.size(); ++i) {
            Stream* s = new Stream(infiles[i]);
            if ( s->eof() ) {
               cerr << "Ignoring empty input file: " << infiles[i] << endl;
               delete s;
            } else {
               queue.push(s);
            }
         }

         last_datum_read = get();
      }

      /// Destructor.
      ~mergeStream() {
         while (!queue.empty()) {
            Stream* smallest = queue.top();
            queue.pop();
            delete smallest;
         }
         assert(queue.empty());
         delete last_datum_read;
         delete total;
      }

      /**
       * Signals that there is nothing less in the stream(s).
       * The end only happens when all the streams have reached eof and that
       * the buffered datum have been processed.
       * @return Returns true if there is no more datum in the stream.
       */
      bool eof() const { return finished; }

      /**
       * Reads one datum from the "smallest" stream.
       * @return Returns the the datum from the smallest stream or NULL if the
       *         queue is empty which doesn't mean there is no datum left in memory.
       */
      Datum* get() {
         if (queue.empty()) {
            // Deferred eof since we always have one datum in memory.
            finished = true;
            return NULL;
         }

         // Remove eof stream from priority queue
         Stream* smallest = queue.top();
         queue.pop();

         // Make the datum from the smallest stream the last datum read.
         smallest->swap_buffer(last_datum_read);

         // Update the stream by reading the next entry.
         smallest->get();
         if (!smallest->eof()) {
            queue.push(smallest);
         }
         else {
            delete smallest;
         }

         return last_datum_read;
      }

      /**
       * Tallies the results for the next entry.
       * @return Returns the tallied result for the next entry.
       */
      Datum next() {
         Datum* current;
         swap(total, last_datum_read);  // Initializes total with the smallest value
         // NOTE: current aka last_datum_read is not empty after this loop.
         while ((current = get()) != NULL && *(current) == *(total)) {
            for (int i = 0; i < int(current->counts.size()); i++) { //boxing
               total->counts[i] += current->counts[i]; //boxing
            }
            if ( process_alignments )
               total->alignments += current->alignments;
         }

         return *total;
      }
};



// main
int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   fprintf(stderr, "Merging :\n");
   copy(infiles.begin(), infiles.end(), ostream_iterator<string>(cerr, " "));
   cerr << endl;


   mergeStream       ms(infiles);
   oSafeMagicStream  out(outfile);

   while (!ms.eof()) {
      ms.next().print(out);
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "d", "a", "top-a"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("d", bDebug);
   arg_reader.testAndSet("a", process_alignments);
   arg_reader.testAndSet("top-a", top_alignment);
   if ( top_alignment ) process_alignments = true;

   arg_reader.testAndSet(0, "outfile", outfile);
   arg_reader.getVars(1, infiles);

   // Check validity of params
   if (outfile.empty())
      error(ETFatal, "Invalid output file");
   if (infiles.empty())
      error(ETFatal, "You must provide at least one file to merge");
}
