/**
 * @author Aaron Tikuisis
 * @file inputparser.cc Implementation of canoe's InputParser, which reads and
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

#include "inputparser.h"
#include "basicmodel.h"
#include <errors.h>
#include <math.h>
#include <iostream>

using namespace Portage;
using namespace std;

InputParser::InputParser(istream &in)
   : in(in)
   , lineNum(0)
{
   in.unsetf(ios::skipws);
}

bool InputParser::eof()
{
   return in.eof();
}

bool InputParser::readMarkedSent(vector<string> &sent,
      vector<MarkedTranslation> &marks)
{
   lineNum++;
   char c;
   in >> c;
   skipSpaces(c);
   while (!(in.eof() || c == '\n'))
   {
      if (c == '<')
      {
         in >> c;
         if ( in.eof() || c == ' ' || c == '\t' || c == '\r' || c == '\n' )
         {
            if ( inc_warning(STAND_ALONE_LEFT_ANGLE) <= max_warn )
               error(ETWarn, "Format warning in input line %d: "
                     "interpreting stand-alone '<' as regular character "
                     "(use '\\<' to suppress this warning).", lineNum);
            sent.push_back(string("<"));
         } else
         {
            if ( ! readMark(sent, marks, c) )
            {
               //skipRestOfLine(c);
               //return false;
               // Changed behaviour: on invalid input, we attempt to interpret
               // the rest of the line literally, skipping whatever was already
               // consumed.  We still return false, so canoe will normally
               // abort, but with -tolerate-markup-errors, it will still get
               // most of the input line.
               skipSpaces(c);
               while (!in.eof() && c != '\n') {
                  sent.push_back(string());
                  readString(sent.back(), c, (char)0, (char)0, true, true);
                  skipSpaces(c);
               }
               //cerr << "SENT: " << joini(sent.begin(), sent.end()) << endl;
               return false;
            }
         }
      } else
      {
         sent.push_back(string());
         // EJJ 12JUL2005: Allow < and > inside the string, since the first
         // character is not <.
         bool rc = readString(sent.back(), c, (char)0, (char)0, true);
         assert(rc);
      }
      skipSpaces(c);
   }

   if ( sent.empty() and !in.eof() )
   {
      error(ETWarn, "Empty input on line %d.", lineNum);
   }

   return true;
} // readMarkedSent

bool InputParser::readMark(vector<string> &sent, vector<MarkedTranslation>
      &marks, char &lastChar)
{
   char &c = lastChar;

   // EJJ 12JUL2005: the character after '<' is now read by readMarkedSent
   //assert(c == '<');
   //in >> c;

   // EJJ 12JUL2005: we no longer skip spaces here, because the markup format
   // now says that " < " is a simple token, while " <" followed by a non
   // space character starts a marked phrase.
   //skipSpaces(c);

   // Read tag name
   string tagName;
   readString(tagName, c, '<', '>');
   if (c == '>' || c == '<')
   {
      error(ETWarn, "Format error in input line %d: invalid mark format "
            "(unexpected '%c' after '%s').", lineNum, c, tagName.c_str());
      return false;
   }

   skipSpaces(c);

   // Read "english" or "target"
   string buf;
   readString(buf, c, '=');
   skipSpaces(c);
   if (buf != "english" && buf != "target")
   {
      error(ETWarn, "Format error in input line %d: invalid mark format "
            "(expected 'english' or 'target', got '%s'; "
            "perhaps < should be escaped).",
            lineNum, buf.c_str());
      return false;
   } else if (in.eof() || c != '=')
   {
      error(ETWarn, "Format error in input line %d: invalid mark format "
            "(expected '=' after '%s'; perhaps < should be escaped).",
            lineNum, buf.c_str());
      return false;
   }
   in >> c;
   skipSpaces(c);
   if (in.eof() || c != '"')
   {
      error(ETWarn, "Format error in input line %d: invalid mark format "
            "('\"' expected after '%s=').",
            lineNum, buf.c_str());
      return false;
   }

   c = (char)0;
   vector<vector<string> > phrases;
   // Read the target phrases
   while (!(in.eof() || c == '"' || c == '\n'))
   {
      phrases.push_back(vector<string>());
      in >> c;
      while (!(in.eof() || c == '|' || c == '"' || c == '\n'))
      {
         skipSpaces(c);
         phrases.back().push_back(string());
         readString(phrases.back().back(), c, '|', '"');
         if (phrases.back().back().length() == 0)
         {
            phrases.back().pop_back();
         }
      }
   }
   if ( in.eof() || c == '\n' )
   {
      error(ETWarn, "Format error in input line %d: "
            "end of file or line before end of mark.", lineNum);
      return false;
   }
   in >> c;
   skipSpaces(c);

   // Read or automatically assign the probabilities
   vector<double> probs;
   if (!(in.eof() || c == '>'))
   {
      buf.clear();
      readString(buf, c, '=');
      skipSpaces(c);
      if (buf != "prob")
      {
         error(ETWarn, "Format error in input line %d: invalid mark format "
               "('prob' or '>' expected).", lineNum);
         return false;
      } else if (c != '=')
      {
         error(ETWarn, "Format error in input line %d: invalid mark format "
               "('=' expected after 'prob').", lineNum);
         return false;
      }
      in >> c;
      skipSpaces(c);
      if (c != '"')
      {
         error(ETWarn, "Format error in input line %d: invalid mark format "
               "('\"' expected after 'prob=').", lineNum);
         return false;
      }
      c = (char)0;
      while (!(in.eof() || c == '"' || c == ' ' || c == '\t' ||
               c == '\r' || c == '\n'))
      {
         in >> c;
         buf.clear();
         readString(buf, c, '|', '"');
         char *endPtr;
         probs.push_back(strtod(buf.c_str(), &endPtr));
         if (probs.back() <= 0 || endPtr != buf.c_str() + buf.length())
         {
            error(ETWarn, "Format error in input line %d: invalid mark format "
                  "('%s' is not a valid number).", lineNum, buf.c_str());
            return false;
         }
      }
      if (probs.size() != phrases.size())
      {
         error(ETWarn, "Format error in input line %d: invalid mark format "
               "(number of phrase options does not match number of "
               "probabilities).", lineNum);
         return false;
      }
      in >> c;
      skipSpaces(c);
   } else
   {
      while (probs.size() < phrases.size())
      {
         probs.push_back((double)1 / (double)phrases.size());
      }
   }
   if (in.eof())
   {
      error(ETWarn, "Format error in input line %d: "
            "end of file before end of mark.", lineNum);
      return false;
   } else if (c != '>')
   {
      error(ETWarn, "Format error in input line %d: invalid mark format "
            "('>' expected to close start tag).", lineNum);
      return false;
   }

   Range sourceRange;
   sourceRange.start = sent.size();
   // Read the source phrase
   in >> c;
   skipSpaces(c);
   while (!(in.eof() || c == '\n'))
   {
      if (c == '<')
      {
         in >> c;
         if ( c == '/' )
         {
            // "</" marks the start of the end of mark tag
            break;
         } else
         {
            if ( inc_warning(LEFT_ANGLE_IN_MARK) <= max_warn )
               error(ETWarn, "Format warning in input line %d: "
                     "interpreting '<' as regular character inside mark "
                     "(use '\\<' to suppress this warning).", lineNum);
            sent.push_back(string("<"));
            readString(sent.back(), c, '<', (char)0, true);
            skipSpaces(c);
         }
      } else
      {
         sent.push_back(string());
         readString(sent.back(), c, '<', (char)0, true);
         skipSpaces(c);
      }
   }
   sourceRange.end = sent.size();

   if (in.eof())
   {
      error(ETWarn, "Format error in input line %d: "
            "end of file before end of mark.", lineNum);
      return false;
   } else if (c != '/')
   {
      error(ETWarn, "Format error in input line %d: "
            "new line found before end of mark ('</' expected).", lineNum);
      return false;
   }

   // Read the end tag
   skipSpaces(c);
   buf.clear();
   in >> c;
   skipSpaces(c);
   readString(buf, c, '>');
   skipSpaces(c);
   if (in.eof())
   {
      error(ETWarn, "Format error in input line %d: "
            "end of file before end of mark.", lineNum);
      return false;
   } else if (c != '>')
   {
      error(ETWarn, "Format error in input line %d: "
            "invalid mark format ('>' expected to close end tag).", lineNum);
      return false;
   } else if (tagName != buf)
   {
      error(ETWarn, "Format error in input line %d: invalid mark format "
            "(tag name mismatch: start tag was '%s', end tag was '/%s').",
            lineNum, tagName.c_str(), buf.c_str());
      return false;
   }

   for (Uint i = 0; i < phrases.size(); i++)
   {
      marks.push_back(MarkedTranslation());
      marks.back().src_words = sourceRange;
      marks.back().markString = phrases[i];
      marks.back().log_prob = log(probs[i]);
   }
   in >> c;
   return true;
} // readMark

bool InputParser::readString(string &s, char &c, char stopFor1, char
      stopFor2, bool allowAngleBraces, bool quiet)
{
   while ( !(in.eof() || c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
             c == stopFor1 || c == stopFor2) )
   {
      if (c == '\\')
      {
         char next_c = in.peek();
         if ( next_c == '\\' || next_c == '<' || next_c == '>' ) {
            // Here we have a \ correctly escaping a special character, skip
            // the backslash and read the special character in, to be
            // interpreted literally as itself.
            in >> c;
         } else {
            // Here we have a \ followed by a regular character.  It is a
            // minor violation of the input language, so we issue a warning,
            // but interpret the \ literally.
            if ( !quiet && inc_warning(LITERAL_BACKSLASH) <= max_warn )
               error(ETWarn, "Format warning in input line %d: "
                     "Interpreting '\\' not followed by '\\', '<' or '>' "
                     "as a regular character (replace with '\\\\' to suppress "
                     "this warning).", lineNum);
         }
      } else if (c == '>' || c == '<') {
         if ( allowAngleBraces )
         {
            if ( !quiet && inc_warning(LITERAL_ANGLE) <= max_warn )
               error(ETWarn, "Format warning in input line %d: "
                     "interpreting '%c' as regular character "
                     "(use '\\%c' to suppress this warning).",
                     lineNum, c, c);
         } else
         {
            error(ETWarn, "Format error in input line %d: "
                  "unexpected '%c' after '%s' (use \\%c).",
                  lineNum, c, s.c_str(), c);
            return false;
         }
      }
      s.append(1, c);
      in >> c;
   }
   return true;
} // readString

void InputParser::skipSpaces(char &c)
{
   while (!in.eof() && (c == ' ' || c == '\t' || c == '\r')) {
      in >> c;
   }
} // skipSpaces

void InputParser::skipRestOfLine(char &c)
{
   while (!in.eof() && c != '\n') {
      in >> c;
   }
} // skipRestOfLine

Uint InputParser::inc_warning(WarningType warning_type)
{
   if ( warning_counters.size() <= Uint(warning_type) )
      warning_counters.resize(warning_type + 1, 0);
   return ++warning_counters[warning_type];
}

void InputParser::reportWarningCounts() const
{
   if ( warning_counters.size() > Uint(STAND_ALONE_LEFT_ANGLE)
        && warning_counters[STAND_ALONE_LEFT_ANGLE] > max_warn )
      error(ETWarn, "Format warning summary: %d stand-alone '<' were "
            "interpreted literally", warning_counters[STAND_ALONE_LEFT_ANGLE]);
   if ( warning_counters.size() > Uint(LEFT_ANGLE_IN_MARK)
        && warning_counters[LEFT_ANGLE_IN_MARK] > max_warn )
      error(ETWarn, "Format warning summary: %d '<' in marks were "
            "interpreted literally", warning_counters[LEFT_ANGLE_IN_MARK]);
   if ( warning_counters.size() > Uint(LITERAL_BACKSLASH)
        && warning_counters[LITERAL_BACKSLASH] > max_warn )
      error(ETWarn, "Format warning summary: %d '\\' were "
            "interpreted literally", warning_counters[LITERAL_BACKSLASH]);
   if ( warning_counters.size() > Uint(LITERAL_ANGLE)
        && warning_counters[LITERAL_ANGLE] > max_warn )
      error(ETWarn, "Format warning summary: %d '<' and/or '>' within tokens "
            "were interpreted literally", warning_counters[LITERAL_ANGLE]);
}
