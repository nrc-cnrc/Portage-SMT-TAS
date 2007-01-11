/**
 * @author Aaron Tikuisis
 * @file inputparser.cc  This file contains the implementation of the function
 * readDocument(), which reads and parses input that may contain marked
 * phrases.
 * 
 * $Id$
 *
 * Canoe Decoder
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
 */

#include "inputparser.h"
#include "basicmodel.h"
#include <errors.h>
#include <math.h>
#include <iostream>

using namespace Portage;
using namespace std;

DocumentReader::DocumentReader(istream &in): in(in), lineNum(0)
{
    in.unsetf(ios::skipws);
} // DocumentReader

bool DocumentReader::eof()
{
    return in.eof();
} // eof

bool DocumentReader::readMarkedSent(vector<string> &sent,
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
                error(ETWarn, "Format warning in input line %d: interpreting '<' as regular character "
                    "(use '\\<' to suppress this warning).", lineNum);
                sent.push_back(string("<"));
            } else
            {
                if ( ! readMark(sent, marks, c) )
                {
                    skipRestOfLine(c);
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
	} // if
	skipSpaces(c);
    } // while

    if ( sent.size() == 0 and !in.eof() )
    {
        error(ETWarn, "Empty input on line %d.", lineNum);
    }

    return true;
} // readMarkedSent
   
bool DocumentReader::readMark(vector<string> &sent, vector<MarkedTranslation>
	&marks, char &lastChar)
{
    char &c = lastChar;

    // EJJ 12JUL2005: the character after '<' is now read by readMarkedSent
    //assert(c == '<');
    //in >> c;
    
    // EJJ 12JUL2005: we no longer skip spaces here, because the markup format
    // now says that " < " is a simple token, while " <" followed by a now
    // space character starts a marked phrase.
    //skipSpaces(c);
    
    // Read tag name
    string tagName;
    readString(tagName, c, '<', '>');
    if (c == '>' || c == '<')
    {
	error(ETWarn, "Format error in input line %d: invalid mark format (unexpected '%c' after '%s').",
		lineNum, c, tagName.c_str());
        return false;
    } // if
    
    skipSpaces(c);
    
    // Read "english" or "target"
    string buf;
    readString(buf, c, '=');
    skipSpaces(c);
    if (buf != "english" && buf != "target")
    {
	error(ETWarn, "Format error in input line %d: invalid mark format "
            "(expected 'english' or 'target', got '%s'; perhaps < should be escaped).", 
            lineNum, buf.c_str());
        return false;
    } else if (in.eof() || c != '=')
    {
	error(ETWarn, "Format error in input line %d: invalid mark format "
            "(expected '=' after '%s'; perhaps < should be escaped).",
            lineNum, buf.c_str());
        return false;
    } // if
    in >> c;
    skipSpaces(c);
    if (in.eof() || c != '"')
    {
	error(ETWarn, "Format error in input line %d: invalid mark format "
            "('\"' expected after '%s=').",
            lineNum, buf.c_str());
        return false;
    } // if
    
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
	    } // if
	} // while
    } // while
    if ( in.eof() || c == '\n' )
    {
	error(ETWarn, "Format error in input line %d: end of file or line before end of mark.", lineNum);
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
	} // if
	in >> c;
	skipSpaces(c);
	if (c != '"')
	{
	    error(ETWarn, "Format error in input line %d: invalid mark format "
                "('\"' expected after 'prob=').", lineNum);
            return false;
	} // if
	c = (char)0;
	while (!(in.eof() || c == '"' || c == ' ' || c == '\t' || c == '\r' || c == '\n'))
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
	    } // if
	} // while
	if (probs.size() != phrases.size())
	{
	    error(ETWarn, "Format error in input line %d: invalid mark format "
                "(number of phrase options does not match number of probabilities).", lineNum);
            return false;
	} // if
	in >> c;
	skipSpaces(c);
    } else
    {
	while (probs.size() < phrases.size())
	{
	    probs.push_back((double)1 / (double)phrases.size());
	} // while
    } // if
    if (in.eof())
    {
	error(ETWarn, "Format error in input line %d: end of file before end of mark.", lineNum);
        return false;
    } else if (c != '>')
    {
	error(ETWarn, "Format error in input line %d: invalid mark format ('>' expected to close start tag).", lineNum);
        return false;
    } // if
    
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
                error(ETWarn, "Format warning in input line %d: interpreting '<' as regular character inside mark "
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
        } // if
    } // while
    sourceRange.end = sent.size();
    
    if (in.eof())
    {
	error(ETWarn, "Format error in input line %d: end of file before end of mark.", lineNum);
        return false;
    } else if (c != '/')
    {
	error(ETWarn, "Format error in input line %d: new line found before end of mark ('</' expected).", lineNum);
        return false;
    } // if
    
    // Read the end tag
    skipSpaces(c);
    buf.clear();
    in >> c;
    skipSpaces(c);
    readString(buf, c, '>');
    skipSpaces(c);
    if (in.eof())
    {
	error(ETWarn, "Format error in input line %d: end of file before end of mark.", lineNum);
        return false;
    } else if (c != '>')
    {
	error(ETWarn, "Format error in input line %d: invalid mark format ('>' expected to close end tag).", lineNum);
        return false;
    } else if (tagName != buf)
    {
	error(ETWarn, "Format error in input line %d: invalid mark format "
            "(tag name mismatch: start tag was '%s', end tag was '/%s').",
            lineNum, tagName.c_str(), buf.c_str());
        return false;
    } // if
    
    for (Uint i = 0; i < phrases.size(); i++)
    {
	marks.push_back(MarkedTranslation());
	marks.back().src_words = sourceRange;
	marks.back().markString = phrases[i];
	marks.back().log_prob = log(probs[i]);
    } // for
    in >> c;
    return true;
} // readMark

bool DocumentReader::readString(string &s, char &c, char stopFor1, char
	stopFor2, bool allowAngleBraces)
{
    while ( !(in.eof() || c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == stopFor1 || c == stopFor2) )
    {
	if (c == '\\')
	{
	    in >> c;
	} else if (c == '>' || c == '<')
	{
            if ( allowAngleBraces )
            {
                error(ETWarn, "Format warning in input line %d: interpreting '<' as regular character "
                    "(use '\\%c' to suppress this warning).",
                    c, lineNum, c);
            } else
            {
                error(ETWarn, "Format error in input line %d: unexpected '%c' after '%s' (use \\%c).", 
                    lineNum, c, s.c_str(), c);
                return false;
            }
	} // if
	s.append(1, c);
	in >> c;
    } // while
    return true;
} // readString

void DocumentReader::skipSpaces(char &c)
{
    while (!in.eof() && (c == ' ' || c == '\t' || c == '\r'))
    {
	in >> c;
    } // while
} // skipSpaces

void DocumentReader::skipRestOfLine(char &c)
{
    while (!in.eof() && c != '\n')
    {
        in >> c;
    } // while
} // skipRestOfLine

