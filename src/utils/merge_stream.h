// $Id$
/**
 * @author Samuel Larkin
 * @file utils/merge_stream.h
 * @brief Keep track of several streams and merge/tally them.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */


#ifndef __MERGE_STREAM_H__
#define __MERGE_STREAM_H__

#include "file_utils.h"
#include <queue>  // priority_queue
#include <string>

namespace Portage {

/**
 * Datum Interface.
 * NOTE: Don't inherite from this, it will make your code slower.  This is a
 * documentation of what Datum is required to provide.
 */
struct IDatum {
   protected:
      string key;       ///< phrase
   public:
      /// Stream need the key to validate the order of it underlying input file.
      const string& getKey() const { return key; }
      virtual bool parse(const string& buffer, Uint pId) = 0;
      virtual void print(ostream& out) const = 0;
};


/**
 * Keeps track of multiple streams and only allows access to the smallest
 * stream which is the stream with the smallest phrase in ascii order.
 */
template <class Datum>
class mergeStream : private NonCopyable
{
   private:
      /**
       * Reads, parses and track each datum from a file.
       */
      class Stream : private NonCopyable {
         private:
            const string       file;   ///< Filename
            Uint               lineno; ///< Current line number
            iSafeMagicStream   input;  ///< file stream
            Datum*             _data;  ///< Datum
            string             buffer; ///< line buffer
            /// We need a placeholder to keep track of the entries to check if
            /// the stream is LC_ALL=C sorted.
            string             previous_key;
            const Uint         positional_id;  ///< this stream has what position on the command line.

         public:
            /// Default constructor
            Stream(const string& file, Uint pId)
            : file(file)
            , lineno(0)
            , input(file)
            , _data(new Datum)
            , positional_id(pId)
            {
               // Triggers reading the first record which is required to have
               // the stream ordered.
               get();
            }

            /// Destructor.
            ~Stream() { delete _data; }

            /// eof returns true on eof and other errors
            bool eof() const { return !input; }

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
               if (getline(input, buffer)) {
                  ++lineno;
                  if (!_data->parse(buffer, positional_id)) {
                     error(ETFatal, "Error parsing line %d of %s\n", file.c_str(), lineno);
                  }

                  // Validates the entries' order.
                  if (previous_key > _data->getKey()) {
                     error(ETFatal, "%s is not LC_ALL=C sorted at line %u:\n%s\n%s",
                           file.c_str(), lineno, previous_key.c_str(), _data->getKey().c_str());
                  }
                  previous_key = _data->getKey();
               }
               if (input.bad())
                  error(ETFatal, "Problem after line %d of %s.  File may be corrupt.\n",
                        lineno, file.c_str());
            }

            bool operator<(const Stream& other) const {
               // Make sure the exhausted streams are at the end of the queue
               if (other.eof()) return true;
               if (eof()) return false;
               return *(_data) < *(other._data);
            }

            /**
             * Let's keep the stream order.
             */
            struct betterThan : binary_function <Stream* const, Stream* const, bool> {
               bool operator() (const Stream* const x, const Stream* const y) const {
                  assert(x != NULL);
                  assert(y != NULL);
                  // false when equal
                  return !(*x < *y);
               }
            };
      };


   private:
      /// Keeps track of the next "smallest" stream to get datum from.
      priority_queue<Stream*, vector<Stream*>, typename Stream::betterThan> queue;

      /// Last datum read which also acts has a buffered input.
      Datum* last_datum_read;
      /// Used to tally counts for a particular source.  Spares some allocations.
      Datum* total;
      /// Indicates if there are datums to process in the stream.
      bool finished;

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
            Stream* s = new Stream(infiles[i], i);
            if ( s->eof() ) {
               cerr << "Ignoring empty input file: " << infiles[i] << endl;
               delete s;
            }
            else {
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
       * Signals that there is nothing left in the stream(s).
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
            *total += *current;
         }

         return *total;
      }
};

};  // ends namespace Portage

#endif  // ends __MERGE_STREAM_H__
