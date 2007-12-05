/**
 * @author Eric Joanis
 * @file trie.cc Implementation of non-template things in trie.h
 *
 * $Id$
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "trie.h"

using namespace Portage;

PTrieNullMapper PTrieNullMapper::m;
PTrieNullMapper::PTrieNullMapper() {}
PTrieNullMapper::PTrieNullMapper(const PTrieNullMapper&) { assert(false); }
PTrieNullMapper& PTrieNullMapper::operator=(const PTrieNullMapper&) {
   assert(false);
}

