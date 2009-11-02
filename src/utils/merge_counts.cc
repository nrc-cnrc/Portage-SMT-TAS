/**
 * @author Samuel Larkin
 * @file merge_counts.cc
 * @brief Merge sorted files by talling counts.
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
Merge count files where the count is separated by at least one space and is the\n\
last item on a line.\n\
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
         Uint   count;    ///< phrase's count

         /// Default constructor.
         Datum()
         : count(0xBAD)
         {}

         void print(ostream& out) const {
            out << prefix << count << endl;
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
            return prefix == other.prefix;
         }
         bool operator<(const Datum& other) const {
            return prefix < other.prefix;
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
            Stream(const Stream&); ///< Noncopyable
            Stream& operator=(const Stream&); ///< Noncopyable

         public:
            /// Default constructor
            Stream(const string& file)
            : file(file)
            , input(file)
            , _data(NULL)
            {
               // Triggers writing the first record which is required to have
               // the stream ordered.
               get();
            }

            /// Destructor.
            ~Stream() { delete _data; }

            bool eof() const { return _data == NULL; }

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

            Datum* get() {
               if (getline(input, buffer)) {
                  const string::size_type pos = buffer.rfind(' ');
                  if (pos == string::npos) error(ETFatal, "Invalid entry %s", buffer.c_str());
                  if (_data == NULL) _data = new Datum;
                  _data->prefix = buffer.substr(0, pos+1);  // Keep the space
                  if (!convT(buffer.substr(pos).c_str(), _data->count))
                     error(ETWarn, "Count is not a number %s", buffer.c_str());
               }
               else {
                  delete _data;
                  _data = NULL;
               }

               return _data;
            }

            bool operator<(const Stream& other) const {
               // Make sure the exhausted streams are at the end of the queue
               if (other.eof()) return true;
               if (eof()) return false;
               // At this point, _data and other._data have been verified not NULL.
               return *(_data) < *(other._data);
            }

            struct bestThan {
               bool operator()(const Stream* const a, const Stream* const b) {
                  assert(a != NULL);
                  assert(b != NULL);
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
      {
         for (Uint i(0); i<infiles.size(); ++i) {
            // Create a Stream with one input file.
            Stream* s = new Stream(infiles[i]);
            assert(s != NULL);
            // Make sure there is at least one entry or else the is no point in
            // adding it to the queue.
            if (!s->eof()) {
               queue.push(s);
            }
            else {
               // This stream was empty, delete it.
               delete s;
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
      bool eof() const { 
         // We are at the end when there is no more datum.
         return last_datum_read == NULL; 
      }

      /**
       * Reads one datum from the "smallest" stream.
       * @return Returns the the datum from the smallest stream or NULL if the
       *         queue is empty which doesn't mean there is no datum left in memory.
       */
      Datum* get() {
         if (queue.empty()) {
            // Deferred eof since we always have one datum in memory.
            delete last_datum_read;
            last_datum_read = NULL;
            return last_datum_read;
         }

         // Remove eof stream from priority queue
         Stream* smallest = queue.top();
         assert(smallest != NULL);
         queue.pop();

         // Make the datum from the smallest stream the last datum read.
         smallest->swap_buffer(last_datum_read);
         assert(last_datum_read != NULL);

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
            *total += *current;
         }

         return *total;
      }
};



// main
int main(int argc, char* argv[])
{
   printCopyright(2008, "merge_counts");
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
      error(ETFatal, "You must provide at least of file to merge");
}
