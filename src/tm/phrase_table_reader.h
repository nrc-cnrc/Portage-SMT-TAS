/**
 * @author Aaron Tikuisis
 * @file phrase_table_reader.h  Translation-Model Utilities.
 * $Id$
 * 
 * Translation-Model Utilities
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada 
 * 
 * This file contains the declaration of the PhraseTableReader class, which is used to
 * read a conditional phrase table with the standard format
 * src_phrase ||| tgt_phrase ||| p(src|tgt).
 */

#ifndef PHRASE_TABLE_READER_H
#define PHRASE_TABLE_READER_H

#include <portage_defs.h>
#include <iostream>
#include <string>

using namespace std;

namespace Portage
{
    /**
     * The seperator between columns of the phrase table, "|||".
     */
    extern const char *PHRASE_TABLE_SEP;
    
    /**
     * Represents one entry in the phrase table.
     */
    struct PhraseTableEntry
    {
	private:
	    /**
	     * The probability value.
	     */
	    double p;
	    
	    /**
	     * Whether the probability has been parsed yet.
	     */
	    bool probComputed;
	public:
	    /**
	     * Creates a new PhraseTableEntry.
	     */
	    PhraseTableEntry();
	    
	    /**
	     * The first phrase.
	     */
	    string phrase1;
	    
	    /**
	     * The second phrase.
	     */
	    string phrase2;
	    
	    /**
	     * The string containing the probability.
	     */
	    string probString;
	    
	    /**
	     * The line number of this entry.
	     */
	    Uint lineNum;
	    
	    /**
	     * Returns the probability value.  If the probString does not convert to a
	     * number, then the program is terminated with an appropriate error message.
	     */
	    double prob();
    }; // PhraseTableEntry
    
    /**
     *  Helper class that makes reading phrase table files easier.
     */
    class PhraseTableReader
    {
	private:
	    /**
	     * The input stream used to read from.
	     */
	    istream &in;
	    
	    /**
	     * The next line being read.
	     */
	    string line;
	    
	    /**
	     * The current line number (starting at 1).
	     */
	    Uint lineNum;
	public:
	    /**
	     * Creates a new PhraseTableReader.
	     * @param in	The input stream to read the phrase table from.
	     */
	    PhraseTableReader(istream &in);
	    
	    /**
	     * Reads and returns the next entry from the phrase table.  eof() must be
	     * false before this is called.  If the next line is not of the correct
	     * format, then the program is terminated with an appropriate error message.
	     * @return	A new PhraseTableEntry representing the line read.
	     */
	    PhraseTableEntry readNext();
	    
	    /**
	     * Determines if the end-of-file has been reached.
	     * @return	true iff the end-of-file has been reached.
	     */
	    bool eof();
    }; // PhraseTableReader
} // Portage

#endif // PHRASE_TABLE_READER_H
