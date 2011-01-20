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
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <iterator>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
merge_counts [-v][-d] [outfile [infile]]\n\
\n\
Merge multi-counts files where the counts are separated by \" ||| \" in a line.\n\
\n\
IMPORTANT:\n\
- input files must be LC_ALL=C sorted.\n\
\n\
Options:\n\
\n\
-d  Write debugging info. [don't]\n\
-v  Write progress reports to cerr. [don't]\n\
";

// globals
const char* PHRASE_TABLE_SEP = " ||| ";

static bool bDebug = false;
static bool verbose = false;
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
         
         void print(ostream& out) const {
            out << prefix;
            int i;
            for (i = 0; i < int(counts.size()) - 1; i++) {
               out << counts[i] << " ";
            }
            out << counts[i] << suffix << endl;                
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
            iSafeMagicStream   input;  ///< file stream
            Datum*             _data;  ///< Datum
            string             buffer; ///< line buffer

         private:
            Stream(const Stream&);
            Stream& operator=(const Stream&);

         public:
            /// Default constructor
            Stream(const string& file)
            : file(file)
            , input(file)
            , _data(new Datum)
            {
               get();
            }

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
                  const string::size_type pos = buffer.rfind(PHRASE_TABLE_SEP);
                  if (pos == string::npos)
                     error(ETFatal, "Invalid entry %s", buffer.c_str());
                     
                  _data->prefix = buffer.substr(0, pos+sep_len);  // Keep the PHRASE_TABLE_SEP
                  
                  vector<string> allcounts;
                  split(buffer.substr(pos+sep_len), allcounts);
                  
                  _data->counts.clear(); 
                  
                  Uint intcount;
                  for (int j =0; j < int(allcounts.size()); j++){            
                  
                     if (!convT(allcounts[j].c_str(), intcount))
                        error(ETWarn, "Count is not a number %s", allcounts[j].c_str());
                     else
                        _data->counts.push_back(intcount);
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
            queue.push(new Stream(infiles[i]));
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
         swap(total, last_datum_read);  // Initialzes total with the smallest value
         // NOTE: current aka last_datum_read is not empty after this loop.
         while ((current = get()) != NULL && *(current) == *(total)) {
            //total->count += current->count;
                for (int i = 0; i < int(current->counts.size()); i++) { //boxing
                   total->counts[i] += current->counts[i]; //boxing
                } 
         }

         return *total;
      }
};



// main
int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   fprintf(stderr, "Attempting to merge :\n");
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
   const char* switches[] = {"v", "d"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("d", bDebug);

   arg_reader.testAndSet(0, "outfile", outfile);
   arg_reader.getVars(1, infiles);

   // Check validity of params
   if (outfile.empty())
      error(ETFatal, "Invalid output file");
   if (infiles.empty())
      error(ETFatal, "You must provide at least one file to merge");
}