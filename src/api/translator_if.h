/**
 * @author Patrick Paul
 * @file translator_if.h  Translator interface
 *
 * $Id$
 * 
 *
 * Class that specify the translator interface
 * Contains the methods relevant for translating.
 * 
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada 
 */
#ifndef __TRANSLATOR_IF__
#define __TRANSLATOR_IF__
#include <string>

using namespace std;


namespace Portage {

/// Translator interface
class Translator_if
{
  public:
    /**
     * Translates a source sentence and returns its translation sentence.
     * @param source_text  source sentence
     * @param target_text  returned translation of source_text
     */
    virtual void translate(const string& source_text, string& target_text) = 0;   
};// END CLASS DEFINITION Translator_if
} // ends namespace Portage
#endif // __TRANSLATOR_IF__
