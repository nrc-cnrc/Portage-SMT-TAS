/**
 * @author Aaron Tikuisis
 * @file inputparser.cc Implementation of canoe's InputParser, which reads and
 *                      parses input that may contain marked phrases.
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "inputparser.h"
#include "basicmodel.h"
#include "errors.h"
#include <math.h>
#include <iostream>

using namespace Portage;
using namespace std;

InputParser::InputParser(istream &in, bool withId, bool quietEmptyLines)
   : in(in)
   , lineNum(0)
   , _done(false)
   , withId(withId)
   , quietEmptyLines(quietEmptyLines)
{
   in.unsetf(ios::skipws);
}

bool InputParser::done() const
{
   return _done;
}

bool InputParser::skipMarkedSent()
{
   if (in.eof()) {
      _done = true;
      return false;
   }

   ++lineNum;
   char c;
   in >> c;
   while (!(in.eof() || c == '\n'))
      in >> c;

   return true;
}

PSrcSent InputParser::getMarkedSent(vector<string>* class_names)
{
   PSrcSent ss(new newSrcSentInfo);
   if (!readMarkedSent(*ss, class_names))
      error(ETFatal, "Aborting due to ill-formatted input at line %u.", lineNum);
   if (done()) ss.reset();
   return ss;
}

bool InputParser::readMarkedSent(newSrcSentInfo& nss,
      vector<string>* class_names)
{
   nss.clear();

   // Alias for sent because we use the src_sent variable a lot of times!
   vector<string> &sent(nss.src_sent);

   // Parse the source sentence id
   // integer \t SourceSentence
   if (withId)
      in >> nss.external_src_sent_id;
   else 
      nss.external_src_sent_id = lineNum;

   ++lineNum;
   vector<Uint> open_zones;
   vector<Uint> open_localwalls;
   char c;
   in >> c;
   skipSpaces(c);
   while (!(in.eof() || c == '\n'))
   {
      if (c == '<')
      {
         in >> c;
         if (in.eof() || c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if ( inc_warning(STAND_ALONE_LEFT_ANGLE) <= max_warn )
               error(ETWarn, "Format warning in input line %d: "
                     "interpreting stand-alone '<' as regular character "
                     "(use '\\<' to suppress this warning).", lineNum);
            sent.push_back(string("<"));
         } else {
            string tagName;
            readString(tagName, c, '<', '>');
            if (tagName == "zone") {
               skipSpaces(c);
               SrcZone zone;
               zone.range.start = sent.size();
               if (c != '>') {
                  string buf1, buf2;
                  if (!readString(buf1, c, '=') || buf1 != "name" || c != '=' ||
                      !(in >> c) || c != '"' ||
                      !(in >> c) ||
                      !readString(buf2, c, '"') || c != '"' ||
                      !(in >> c) || c != '>') {
                     error(ETWarn, "syntax error around <zone on line %d.\nThe syntax is strict: <zone name=\"name\"> with no slash or extra spaces.", lineNum);
                     return false;
                  }
                  //cerr << "zone starting at " << sent.size() << " name: '" << buf2 << "'" << endl;
                  zone.name = buf2;
               }
               open_zones.push_back(nss.zones.size());
               nss.zones.push_back(zone);
               in >> c;
            } else if (tagName == "/zone") {
               if (open_zones.empty()) {
                  error(ETWarn, "Closing a zone that was not previously opened on line %d.", lineNum);
                  return false;
               }
               if (c != '>') {
                  error(ETWarn, "syntax error around </zone on line %d.", lineNum);
                  return false;
               }
               assert(nss.zones.size() > open_zones.back());
               assert(nss.zones[open_zones.back()].range.end == 0);
               nss.zones[open_zones.back()].range.end = sent.size();

               while (!open_localwalls.empty()) {
                  if (nss.local_walls[open_localwalls.back()].pos > nss.zones[open_zones.back()].range.start) {
                     nss.local_walls[open_localwalls.back()].zone = nss.zones[open_zones.back()].range;
                     open_localwalls.pop_back();
                  }
                  else break;
               }

               open_zones.pop_back();

               in >> c;
            } else if (tagName == "wall") {
               skipSpaces(c);
               string buf1, buf2;
               if (!readString(buf1, c, '=') || buf1 != "name" || c != '=' ||
                   !(in >> c) || c != '"' ||
                   !(in >> c) ||
                   !readString(buf2, c, '"') || c != '"' ||
                   !(in >> c) || c != '/' ||
                   !(in >> c) || c != '>') {
                  error(ETWarn, "syntax error around <wall on line %d.\nThe syntax is strict: <wall name=\"name\"/> with slash and no extra spaces.", lineNum);
                  return false;
               }
               //cerr << "wall at " << sent.size() << " name: '" << buf2 << "'" << endl;
               nss.walls.push_back(SrcWall(sent.size(), buf2));
               in >> c;
            } else if (tagName == "wall/") {
               if (c == '>') {
                  // Save the wall position
                  nss.walls.push_back(SrcWall(sent.size()));
                  in >> c;
               } else {
                  error(ETWarn, "syntax error around <wall/ on line %d.", lineNum);
                  return false;
               }
            } else if (tagName == "localwall") {
               skipSpaces(c);
               string buf1, buf2;
               if (!readString(buf1, c, '=') || buf1 != "name" || c != '=' ||
                   !(in >> c) || c != '"' ||
                   !(in >> c) ||
                   !readString(buf2, c, '"') || c != '"' ||
                   !(in >> c) || c != '/' ||
                   !(in >> c) || c != '>') {
                  error(ETWarn, "syntax error around <localwall on line %d.\nThe syntax is strict: <localwall name=\"name\"/> with slash and no extra spaces.", lineNum);
                  return false;
               }
               open_localwalls.push_back(nss.local_walls.size());
               nss.local_walls.push_back(SrcLocalWall(sent.size(), buf2));
               in >> c;
            } else if (tagName == "localwall/") {
               if (open_zones.empty()) {
                  error(ETWarn, "<localwall/> must be contained within a zone at line %d.", lineNum);
                  return false;
               } else if (c != '>') {
                  error(ETWarn, "Syntax error around <localwall/ on line %d. Only <localwall/> is accepted.", lineNum);
                  return false;
               }
               open_localwalls.push_back(nss.local_walls.size());
               nss.local_walls.push_back(SrcLocalWall(sent.size()));
               in >> c;
            } else {
               // Regular mark uses the old mark parser.
               if (! readMark(sent, nss.marks, tagName, c, class_names)) {
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
                  //cerr << "SENT: " << join(sent.begin(), sent.end()) << endl;
                  return false;
               }
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

   if (!open_zones.empty() || !open_localwalls.empty()) {
      error(ETWarn, "Opened <zone> was not closed on line %d.", lineNum);
      nss.zones.pop_back();
      return false;
   }

   if (sent.empty() and !in.eof() and !quietEmptyLines)
      error(ETWarn, "Empty input on line %d.", lineNum);

   if (in.eof() && sent.empty())
      _done = true;

   return true;
} // readMarkedSent

bool InputParser::readMark(vector<string> &sent,
      vector<MarkedTranslation> &marks,
      const string& tagName,
      char &lastChar,
      vector<string>* class_names)
{
   char &c = lastChar;

   // EJJ 12JUL2005: the character after '<' is now read by readMarkedSent
   //assert(c == '<');
   //in >> c;

   // EJJ 12JUL2005: we no longer skip spaces here, because the markup format
   // now says that " < " is a simple token, while " <" followed by a non
   // space character starts a marked phrase.
   //skipSpaces(c);

   // EJJ 06JAN2014: Tag name was read by caller, so we don't read it here
   // anymore, but we pick up the check just after.
   if (c == '>' || c == '<')
   {
      error(ETWarn, "Format error in input line %d: invalid mark format "
            "(unexpected '%c' after '%s').", lineNum, c, tagName.c_str());
      return false;
   }

   skipSpaces(c);

   // Temp buffer to read the attribute's name
   string buf;

   // If the tag's name is rule there is an added attribute to be read.
   // new attribute is the class name.
   string class_name("");
   if (toLower(tagName, buf) == "rule") {
      if (!readClassAttribute(c, class_name)) return false;

      // If the user asked for the class name's list, add the class name to the list
      if (class_names != NULL) {
         if (find(class_names->begin(), class_names->end(), class_name) == class_names->end()) {
            class_names->push_back(class_name);
         }
      }
   }

   // Read "english" or "target"
   buf.clear();
   if (!readString(buf, c, '=')) return false;
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
   in >> c; // eat "\""
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
         if (!readString(phrases.back().back(), c, '|', '"')) return false;
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
         if (probs.back() <= 0 || endPtr != buf.c_str() + buf.length()) {
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
      // The user did not provide any probs.
      // Assign a uniform distribution of probs to those tgt phrases
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
   while (!(in.eof() || c == '\n')) {
      if (c == '<') {
         in >> c;
         if ( c == '/' ) {
            // "</" marks the start of the end of mark tag
            break;
         }
         else {
            if ( inc_warning(LEFT_ANGLE_IN_MARK) <= max_warn )
               error(ETWarn, "Format warning in input line %d: "
                     "interpreting '<' as regular character inside mark "
                     "(use '\\<' to suppress this warning).", lineNum);
            sent.push_back(string("<"));
            readString(sent.back(), c, '<', (char)0, true);
            skipSpaces(c);
         }
      }
      else {
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
      marks.back().src_words  = sourceRange;
      marks.back().markString = phrases[i];
      marks.back().log_prob   = log(probs[i]);  // natural logarithm of prob
      marks.back().class_name = class_name;
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
         int next_c = in.peek();
         if ( next_c == '\\' || next_c == '<' || next_c == '>' ||
              next_c == stopFor1 || next_c == stopFor2 ) {
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
            //cerr << "next_c=" << next_c << " stopFor1=" << stopFor1
            //     << " stopFor2=" << stopFor2 << endl;
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
            // PTG-71: skip the offending character so that if the false return
            // value is ignored, at least we don't go into an infinite loop
            // Also, where important, do catch the false return value.
            s.append(1, c);
            in >> c;
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
   // SL: seems that if there is a null character we will end up in an infite
   // loop thus let's read it and skip it.
   while (!in.eof() && (c == ' ' || c == '\t' || c == '\r' || c == '\0')) {
      if (c == '\0') {
         if (inc_warning(NULL_CHARACTER) <= max_warn) {
            error(ETWarn,
               "NULL characters in plain text input files are invalid on line %d.", lineNum);
         }
      }
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
   if ( warning_counters.size() > Uint(NULL_CHARACTER)
        && warning_counters[NULL_CHARACTER] > max_warn )
      error(ETWarn, "Format warning summary: %d null character found in stream"
            , warning_counters[NULL_CHARACTER]);
}


bool InputParser::readClassAttribute(char& c, string& class_name)
{
   // Read the attribute's tag
   string buf;
   buf.clear();
   if (!readString(buf, c, '=')) return false;
   skipSpaces(c);

   if (buf != "class") {
      error(ETWarn, "Format error in input line %d: invalid mark format "
            "(expected 'class', got '%s'; perhaps < should be escaped).",
            lineNum, buf.c_str());
      return false;
   }
   else if (in.eof() || c != '=') {
      error(ETWarn, "Format error in input line %d: invalid mark format "
            "(expected '=' after '%s'; perhaps < should be escaped).",
            lineNum, buf.c_str());
      return false;
   }

   in >> c;  // eat the "="
   skipSpaces(c);
   if (in.eof() || c != '"') {
      error(ETWarn, "Format error in input line %d: invalid mark format "
            "('\"' expected after '%s=').",
            lineNum, buf.c_str());
      return false;
   }
   in >> c;  // eat "\""

   if (!readString(class_name, c, '"')) return false;
   if (in.eof() || c != '"') {
      error(ETWarn, "Format error in input line %d: invalid mark format "
            "('\"' expected after '%s=').",
            lineNum, class_name.c_str());
      return false;
   }
   in >> c;  // consume "\""
   skipSpaces(c);

   return true;
}
