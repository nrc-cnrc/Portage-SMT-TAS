/**
 * @author George Foster
 * @file tm_io.h  Utilities to read tokenized, possibly marked-up, input text.
 * 
 * 
 * COMMENTS: 
 *
 * Read tokenized, possibly marked-up, input text. Normal tokens are separated
 * by whitespace; markup is surrounded by angle brackets. There are two kinds
 * of markup: 
 *
 * 1) solo items like "<srclang fr>"
 * 2) group items like "<date 12/11/04>12 Nov 04</date>"
 *
 * The 1st kind of markup is usually deleted. The 2nd kind is replaced with its
 * tag, eg:  "<date 12/11/04>12 Nov 04</date> -> <date>".
 *
 * Special characters can be escaped using the backslash. Examples, with
 * resulting token(s) in quotes:
 *
 * only\ one\ token -> "only one token"
 * \<not markup\>   -> "<not" "markup>"
 * "harm\less"      -> "harmless"
 *
 * NB: THE BACKSLASH FEATURE IS NOW TURNED OFF BY DEFAULT! It causes problems
 * with other parts of the system that aren't ready for words containing
 * blanks. Also, backslashes can occur in GB-encoded Chinese.
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada /
 * Copyright 2005, National Research Council of Canada
 */

#ifndef TM_IO_H
#define TM_IO_H

#include <string>
#include <vector>
#include <portage_defs.h>

namespace Portage {

/// Some utilities to read tokenized, possibly marked-up, input text. 
namespace TMIO {

   /**
    * Set this to "\\" if you want to activate backslashes as the escape character.
    */
   extern const char* escapes;	// set to "" by default
   
   /**
    * Extract tokens from a tokenized line of text.
    * @param line line of text
    * @param tokens place to append extracted tokens
    * @param do_markup what to do with markup:
    * 0 = replace grouped markup by its tag, and remove solo markup, eg: 
    *     "<date 12/11/04>12 Nov 04</date> -> <date>"
    *     "<solo> ->"
    * 1 = ignore all markup,eg:
    *     "<date 12/11/04>12 Nov 04</date> -> <date 12/11/04>12 Nov 04</date>"
    *     "<solo> -> <solo>"
    * 2 = strip, eg:
    *     "<date 12/11/04>12 Nov 04</date> -> 12 Nov 04"
    *     "<solo> ->"
    * @return ref to tokens arg
    */
   vector<string>& getTokens(const string& line, vector<string>& tokens,
				    Uint do_markup = 0);

   /**
    * Join tokens into a single tokenized line.
    * @param tokens to join
    * @param line of text
    * @param bracket put brackets around tokens if true
    * @return "<line>"
    */
   string& joinTokens(const vector<string>& tokens, string& line, bool bracket=false);

   /// Unit testing function.
   void test();
}
}

#endif
