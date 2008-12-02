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
         string suffix;   ///< phrase
         Uint   count;    ///< phrase's count

         void print(ostream& out) const {
            out << prefix << count << suffix << endl;
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

            void swap_buffer(Datum*& spare) {
                assert(spare != NULL);
                swap(_data, spare);
            }

            Datum* get() {
               if (getline(input, buffer)) {
                  const string::size_type pos = buffer.rfind(' ');
                  _data->prefix = buffer.substr(0, pos+1);  // Keep the space
                  if (!convT(buffer.substr(pos).c_str(), _data->count))
                     error(ETWarn, "Count is not a number %s", buffer.c_str());
               }
               return NULL;
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

      priority_queue<Stream*, vector<Stream*>, Stream::bestThan> queue;

      Datum* spare;
      Datum* total;

   public:
      mergeStream(const vector<string>& infiles)
      : spare(new Datum)
      , total(new Datum)
      {
         for (Uint i(0); i<infiles.size(); ++i) {
            queue.push(new Stream(infiles[i]));
         }

         spare = get();
      }

      ~mergeStream() {
         while (!queue.empty()) {
            Stream* smallest = queue.top();
            queue.pop();
            delete smallest;
         }
         assert(queue.empty());
      }

      bool eof() const { return queue.empty(); }

      Datum* get() {
         if (queue.empty()) return NULL;

         // Remove eof stream from priority queue
         Stream* smallest = queue.top();
         queue.pop();

         smallest->swap_buffer(spare);

         smallest->get();
         if (!smallest->eof()) {
            queue.push(smallest);
         }
         else {
            delete smallest;
         }

         return spare;
      }

      Datum next() {
         Datum* current;
         swap(total, spare);  // Initialzes total with the smallest value
         while ((current = get()) != NULL && *(current) == *(total)) {
            total->count += current->count;
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
   // Unfortunately we still need to print the last record.
   ms.next().print(out);
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
