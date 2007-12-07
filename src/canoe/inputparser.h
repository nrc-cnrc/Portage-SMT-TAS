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
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "canoe_general.h"
#include <iostream>

#ifndef INPUTPARSER_H
#define INPUTPARSER_H

using namespace std;

namespace Portage
{
   struct MarkedTranslation;

   /// Reads and parses input that may contain marked phrases.
   class InputParser
   {
   private:
      istream &in;   ///< Stream we are reading from

      Uint lineNum;  ///< Current line number

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
       * Assuming that '<' was just read, reads an entire mark of the format:
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
       * @param lastChar  The last character read; should initially be '<',
       *                  and at the end will be the first character after
       *                  the final >.
       * @return  true iff no error was encountered
       */
      bool readMark(vector<string> &sent, vector<MarkedTranslation> &marks,
            char &lastChar);

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

   public:
      /**
       * Creates an InputParser to read from the given input stream.
       * @param in        The input stream to read from.
       */
      InputParser(istream &in);

      /**
       * Tests whether the end of file has been reached.
       * @return  true iff the end of file has been reached.
       */
      bool eof();

      /**
       * lineNum accessor
       * @return the current line number
       */
      Uint getLineNum() const { return lineNum; }

      /**
       * Reads and parses a line of input.  If the line is improperly
       * formatted, terminates the program with an error.
       * @param sent      A vector containing all the words in the
       *                  sentence in order.
       * @param marks     A vector containing the marks in the sentence.
       * @return  true iff no error was encountered
       */
      bool readMarkedSent(vector<string> &sent,
            vector<MarkedTranslation> &marks);

      /**
       * Report occurrence counts for warnings that are only reported a limited
       * number of times.  The occurrence count is only printed if the warning
       * occurred more often than it already got reported.
       */
      void reportWarningCounts() const;

   }; // ends class InputParser
} // ends namespace Portage

#endif // INPUTPARSER_H
