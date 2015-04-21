/**
 * @author Eric Joanis
 * @file lmbin.cc
 * @brief LMBin implementation
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "lmbin.h"
#include "file_utils.h"
#include "str_utils.h"

using namespace Portage;
using namespace std;


static const string MagicNumber = "Portage BinLM file, format v1.0";

bool LMBin::isA(const string& file) {
   return matchMagicNumber(file, MagicNumber);
}

void LMBin::read_binary(const string& binlm_filename, Uint limit_order)
{
   iSafeMagicStream ifs(binlm_filename);
   string line;
   time_t start = time(NULL);

   // "Magic string" line
   getline(ifs, line);
   if ( line != MagicNumber )
      error(ETFatal, "File %s not in Portage's BinLM format: bad first line",
            binlm_filename.c_str());

   // Order line
   getline(ifs, line);
   vector<string> tokens;
   split(line, tokens);
   if ( tokens.size() != 3 || tokens[0] != "Order" || tokens[1] != "=" ||
        !conv(tokens[2], gram_order ) )
      error(ETFatal, "File %s not in Portage's BinLM format: bad order line",
            binlm_filename.c_str());

   if (limit_order > 0)
      gram_order = min(gram_order, limit_order);
   hits.init(getOrder());

   // Vocab size line
   getline(ifs, line);
   splitZ(line, tokens);
   Uint voc_size = 0;
   if ( tokens.size() != 4 || tokens[0] != "Vocab" || tokens[1] != "size" ||
        tokens[2] != "=" || !conv(tokens[3], voc_size) )
      error(ETFatal, "File %s not in Portage's BinLM format: bad voc size line",
            binlm_filename.c_str());

   // The vocab itself
   if ( voc_map.read_local_vocab(ifs) != voc_size )
      error(ETFatal, "File %s not in Portage's BinLM format: voc size mismatch",
            binlm_filename.c_str());

   // And finally deserialize the trie, applying any caller requested filter
   const Uint nodes_kept = read_bin_trie(ifs, limit_order);

   getline(ifs, line);
   if ( line != "" )
      error(ETFatal, "File %s not in Portage's BinLM format: end corrupt",
            binlm_filename.c_str());
   getline(ifs, line);
   splitZ(line,tokens,"=");
   Uint internal_nodes_in_file = 0;
   if ( tokens.size() != 2 ||
        tokens[0] != "End of Portage BinLM file.  Internal node count" ||
        !conv(tokens[1], internal_nodes_in_file) )
      error(ETFatal, "File %s not in Portage's BinLM format: bad node count",
            binlm_filename.c_str());

   // The local to global vocab cache will not be used again, clear it now.
   voc_map.clear_caches();

   cerr << "Kept " << nodes_kept << " of " << internal_nodes_in_file
        << " nodes.  Done in " << (time(NULL) - start) << "s." << endl;
}


LMBin::LMBin(VocabFilter *vocab, OOVHandling oov_handling, double oov_unigram_prob)
: LMTrie(vocab, oov_handling, oov_unigram_prob)
, voc_map(*vocab)
{
}


LMBin::~LMBin()
{
}
