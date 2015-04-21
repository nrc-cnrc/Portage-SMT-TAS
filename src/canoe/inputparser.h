/**
 * @author Aaron Tikuisis
 * @file inputparser.h  This file contains canoe's InputParser, which reads and
 *                      parses input that may contain marked phrases.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "canoe_general.h"
#include "new_src_sent_info.h"
#include <iostream>
#include <boost/shared_ptr.hpp>

#ifndef INPUTPARSER_H
#define INPUTPARSER_H

using namespace std;

namespace Portage
{
   struct MarkedTranslation;

   /// Reads and parses input that may contain marked phrases.
   class InputParser
   {
   friend class TestInputParser;
   private:
      istream &in;   ///< Stream we are reading from

      Uint lineNum;  ///< Current line number

      bool _done;    ///< set when readMarkedSent() has nothing to return because of eof()

      /// Doing load balancing thus each source sentence has an id
      const bool withId;

      /**
       * Counters for warnings that have been issued already and that should
       * be repeated too many times.
       */
      vector<Uint> warning_counters;

      /// Warning types that can be suppressed after a few instances.
      enum WarningType {
         STAND_ALONE_LEFT_ANGLE,
         LEFT_ANGLE_IN_MARK,
         LITERAL_BACKSLASH,
         LITERAL_ANGLE,
         NULL_CHARACTER,
      };

      /**
       * Increment a warning counter, creating if it necessary, and return its
       * new value.
       * @param warning_type the name of the warning counter to use
       */
      Uint inc_warning(WarningType warning_type);

      /// How many times a warning is reported before it is silenced
      static const Uint max_warn = 3;

      /**
       * Assuming that '<' and the tag name was just read, reads an entire mark
       * of the format:
       * <MARKNAME target = "TGTPHRASE(|TGTPHRASE)*" (prob = "PROB(|PROB)*"|) >
       * SRCPHRASE "</MARKNAME >"
       *
       * Additional constraints: there must be as many PROB's as TGTPHRASE's
       * and the MARKNAME's must be the same.  Each space can be replaced by
       * any number of spaces.  No space is allowed after the opening <.  The
       * source words are added to sent and each mark is added to marks.  The
       * last character read (right after the final >) is placed in lastChar.
       *
       * @param sent      Used to store the source sentence being read.
       * @param marks     Used to store marks.
       * @param tagName   The name of the tag, which has just been read from in.
       * @param lastChar  The last character read; should be the character
       *                  after the tag name on entry; will be the character
       *                  following the final '>' on successful exit.
       * @param class_names  If non-NULL, the class-name of all rules
       *                     found will be added (only once per class name)
       * @return  true iff no error was encountered
       */
      bool readMark(vector<string> &sent,
            vector<MarkedTranslation> &marks,
            const string& tagName,
            char &lastChar,
            vector<string>* class_names = NULL);

      /**
       * Reads a string terminated by stopFor1, stopFor2, space or newline.
       * The terminating character is not considered part of the string.  \ is
       * the escape character when followed by \, < or >, in which case the
       * character following \ is added to the string and does not terminate
       * the string if it would otherwise; followed by anything else, \ is
       * interpreted literally with a warning.
       *
       * @param s         The string read is appended to this.
       * @param c         The last character read is stored here.
       * @param stopFor1  A character that terminates the string.
       * @param stopFor2  A character that terminates the string.
       * @param allowAngleBraces  when true, < and > will be allowed
       *                          as charaters in the string
       * @param quiet     Suppress warnings (e.g., to reduce log clutter after
       *                  an error that has been issued).
       * @return  true iff no error was encountered
       */
      bool readString(string &s, char &c, char stopFor1 = (char)0,  char
            stopFor2 = (char)0, bool allowAngleBraces = false,
            bool quiet = false);

      /**
       * Reads and skips as many spaces as possible, if c is a space.
       * The first non-space character encountered is stored in c.
       * @param c         The last character read is stored here.
       */
      void skipSpaces(char &c);

      /**
       * Reads and skips the rest of the line.  The last character read,
       * is left in c, a newline character unless eof was reached.
       * @param c         The last character read is stored here.
       */
      void skipRestOfLine(char &c);

      /**
       * Reads the class attribute of a rule.
       * class="XYZ"
       * @param lastChar    The last character read is stored here.
       * @param class_name  will hold XYZ.
       * @return Returns false if a parsing error occured.
       */
      bool readClassAttribute(char &lastChar, string& class_name);

   public:
      /**
       * Creates an InputParser to read from the given input stream.
       * @param in        The input stream to read from.
       * @param withId    indicates that each source sentence is preceded
       *                  by it source sentence id
       */
      InputParser(istream &in, bool withId=false);

      /**
       * Tests whether all input sentences have been processed.
       *
       * Unlike the old eof(), done() returns true once readMarkedSent() has
       * failed to read a sentence because it was at eof(), rather than by just
       * looking at the input stream's eof() flag.
       *
       * Now, one can write a loop like this:
       *    newSrcSentInfo nss;
       *    reader.readMarkedSent(nss)
       *    while (!reader.done()) {
       *       process nss
       *       reader.readMarkedSent(nss)
       *    }
       *
       * @return  true iff the last call to readMarkedSent() returned an empty
       *          sentence because of eof.
       */
      bool done() const;

      /**
       * lineNum accessor
       * @return the current line number
       */
      Uint getLineNum() const { return lineNum; }

      /**
       * Reads and parses a line of input.
       *
       * @param[out] nss  Groups most parameters to this option; will be
       *                  cleared before use.
       * @param[out] nss.src_sent A vector containing all the words in the
       *                          sentence in order.
       * @param[out] nss.marks    A vector containing the marks in the sentence.
       * @param[out] nss.external_src_sent_id  Source sentence ID
       * @param[out] nss.zones    Will contain the zones found in the sentence
       * @param[out] nss.walls    Will contain the walls found in the sentence
       * @param[out] class_names  If non-NULL, the class-name of all rules
       *                          found will be added (only once per class name)
       * @return  true iff no error was encountered and the line is properly formatted.
       */
      bool readMarkedSent(newSrcSentInfo& nss, vector<string>* class_names = NULL);

      /**
       * wrapper for readMarkedSent() with getline()-like semantics, and a
       * fatal error in case of bad input.
       * Allows for use of the reader with this simplified loop:
       *    PSrcSent ss;
       *    while (ss = reader.getMarkedSent()) {
       *       process *ss
       *    }
       * @param[out] class_names  If non-NULL, the class-name of all rules
       *                          found will be added (only once per class name)
       * @return a shared_ptr to a newly allocated newSrcSentInfo object that
       *         contains all the info of the sentence that was read; on end of
       *         input, that shared pointer will evaluate as false in boolean
       *         context.
       */
      PSrcSent getMarkedSent(vector<string>* class_names = NULL);

      /**
       * Skips a line of input
       * @return true iff no error was encountered
       */
      bool skipMarkedSent();

      /**
       * Report occurrence counts for warnings that are only reported a limited
       * number of times.  The occurrence count is only printed if the warning
       * occurred more often than it already got reported.
       */
      void reportWarningCounts() const;

   }; // ends class InputParser
} // ends namespace Portage

#endif // INPUTPARSER_H
