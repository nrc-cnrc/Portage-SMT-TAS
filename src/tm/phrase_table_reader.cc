/**
 * @author Aaron Tikuisis
 * @file phrase_table_reader.cc  Implementation of PhraseTableReader.
 *
 * $Id$
 * 
 * Translation-Model Utilities
 * 
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada 
 * 
 * This file contains the implementation of the PhraseTableReader class, which is used to
 * read a conditional phrase table with the standard format
 * src_phrase ||| tgt_phrase ||| p(src|tgt).
 */

#include "phrase_table_reader.h"
#include <str_utils.h>
#include <errors.h>
#include <assert.h>

using namespace std;
using namespace Portage;

const char *Portage::PHRASE_TABLE_SEP = "|||";

PhraseTableEntry::PhraseTableEntry(): probComputed(false) {}

double PhraseTableEntry::prob()
{
    if (!probComputed)
    {
	if (!conv(probString, p))
	{
	    error(ETFatal, "Invalid number format in phrase table at line %d", lineNum);
	} // if
    } // if
    return p;
} // prob

PhraseTableReader::PhraseTableReader(istream &in): in(in), lineNum(0) {}

PhraseTableEntry PhraseTableReader::readNext()
{
    // Skip blank lines
    while (line == "")
    {
	getline(in, line);
	lineNum++;
    }
    assert(!eof());
    
    PhraseTableEntry result;
    result.lineNum = lineNum;
    // Find the (first) two occurrences of ||| in the line (if there are more occurrences
    // of ||| then this will cause a later problem with parsing the probability).
    string::size_type index1 = line.find(PHRASE_TABLE_SEP, 0);
    string::size_type index2 = line.find(PHRASE_TABLE_SEP, index1 + strlen(PHRASE_TABLE_SEP));
    if (index2 == string::npos)
    {
	error(ETFatal, "Bad format in phrase file at line %d", lineNum);
    } // if
    
    result.phrase1 = line.substr(0, index1);
    trim(result.phrase1);
    result.phrase2 = line.substr(index1 + strlen(PHRASE_TABLE_SEP), index2 - index1 -
	    strlen(PHRASE_TABLE_SEP));
    trim(result.phrase2);
    result.probString = line.substr(index2 + strlen(PHRASE_TABLE_SEP));
    trim(result.probString);
    
    getline(in, line);
    lineNum++;
    return result;
} // readNext

bool PhraseTableReader::eof()
{
    // Skip all blank lines
    while (!in.eof() && line == "")
    {
	getline(in, line);
	lineNum++;
    } // while
    return in.eof();
} // eof
