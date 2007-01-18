/**
 * @author Aaron Tikuisis
 * @file inputparser.h  This file contains the declaration of the function
 * readDocument(), which reads and parses input that may contain marked
 * phrases.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group 
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
    class MarkedTranslation;
    
    /// Reads and parses input that may contain marked phrases.
    class DocumentReader
    {
	private:
	    istream &in;   ///< Stream we are reading from
	    
	    Uint lineNum;  ///< Current line number
	    
	    /**
	     * Assuming that '<' was just read, reads an entire mark of the format:
	     * < MARKNAME target = "TGTPHRASE(|TGTPHRASE)*" (prob = "PROB(|PROB)*"|) >
	     * SRCPHRASE "< \MARKNAME >"
	     * Additional constraints: there must be as many PROB's as TGTPHRASE's and the
	     * MARKNAME's must be the same.  Each space can be replaced by any number of
	     * spaces.  The source words are added to sent and each mark is added to
	     * marks.  The last character read (right after the final >) is placed in
	     * lastChar.
	     * @param sent	Used to store the source sentence being read.
	     * @param marks	Used to store marks.
	     * @param lastChar	The last character read; should initially be '<', and at
	     * 			the end will be the first character after the final >.
             * @return  true iff no error was encountered
	     */
	    bool readMark(vector<string> &sent, vector<MarkedTranslation> &marks,
		    char &lastChar);
	    
	    /**
	     * Reads a string terminated by stopFor1, stopFor2, space or newline.  The
	     * terminating character is not considered part of the string.  \ is the
	     * escape character, and any character following \ is added to the string and
	     * does not terminate the string if it would otherwise.
	     * @param s	The string read is appended to this.
	     * @param c	The last character read is stored here.
	     * @param stopFor1	A character that terminates the string.
	     * @param stopFor2	A character that terminates the string.
             * @param allowAngleBraces  when true, < and > will be allowed
             *                          as charaters in the string
             * @return  true iff no error was encountered
	     */
	    bool readString(string &s, char &c, char stopFor1 = (char)0,  char
		    stopFor2 = (char)0, bool allowAngleBraces = false);
	    
	    /**
	     * Reads and skips as many spaces as possible, if c is a space.  The first
	     * non-space character encountered is stored in c.
	     * @param c	The last character read is stored here.
	     */
	    void skipSpaces(char &c);

	    /**
             * Reads and skips the rest of the line.  The last character read,
             * is left in c, a newline character unless eof was reached.
	     * @param c	The last character read is stored here.
	     */
	    void skipRestOfLine(char &c);

	public:
	    /**
	     * Creates a document reader to read from the given input stream.
	     * @param in	The input stream to read from.
	     */
	    DocumentReader(istream &in);
	    
	    /**
	     * Tests whether the end of file has been reached.
	     * @return	true iff the end of file has been reached.
	     */
	    bool eof();
	    
	    /**
	     * Reads and parses a line of input.  If the line is improperly formatted,
	     * terminates the program with an error.
	     * @param sent	A vector containing all the words in the sentence in
	     * 			order.
	     * @param marks	A vector containing the marks in the sentence.
             * @return  true iff no error was encountered
	     */
	    bool readMarkedSent(vector<string> &sent, vector<MarkedTranslation>
		    &marks);
    }; // ends class DocumentReader
} // ends namespace Portage

#endif // INPUTPARSER_H
