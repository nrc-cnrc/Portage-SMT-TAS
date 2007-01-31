/**
 * @author Bruno Laferrière / Eric Joanis
 * @file lm.cc  Implementations of LM methods.
 * $Id$
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "lm.h"
#include "lmbin.h"
#include "lmtext.h"
#include <string>
#include <assert.h>
#include <str_utils.h>

using namespace std;
using namespace Portage;

//----------------------------------------------------------------------------
// Constructors
PLM::PLM(Voc* vocab, bool UNK_tag, float oov_unigram_prob) :
   vocab(vocab), isUNK_tagged(UNK_tag), gram_order(0),
   oov_unigram_prob(oov_unigram_prob) {}

PLM::PLM() :
   vocab(NULL), isUNK_tagged(false), gram_order(0),
   oov_unigram_prob(-INFINITY) {}

//----------------------------------------------------------------------------
PLM* PLM::Create(const string& lm_filename, Voc* vocab, bool UNK_tag,
                 bool limit_vocab, Uint limit_order, float oov_unigram_prob,
                 ostream *const os_filtered)
{
   string lm_physical_filename(lm_filename);
   size_t hash_pos = lm_filename.rfind("#");
   if ( hash_pos != string::npos ) {
      Uint naming_limit_order = conv<Uint>(lm_filename.substr(hash_pos+1));
      if ( limit_order && naming_limit_order < limit_order )
         limit_order = naming_limit_order;
      lm_physical_filename.resize(hash_pos);
   }

   if ( vocab ) {
      // Make sure SentStart and SentEnd and UNK_Symbol are in the vocab
      vocab->add(SentStart);
      vocab->add(SentEnd);
      vocab->add(UNK_Symbol);
   }

   PLM* lm;

   // Insert here the code to open any new LM format you write support for.

   if ( isSuffix(".lmdb", lm_physical_filename) ) {
      // The LMDB format depends on Berkeley_DB, which has an expensive license
      // fee for projects that are not fully open source.  It has therefore
      // been removed from this distribution.
      lm = NULL;
      error(ETFatal, "LMDB format not supported, can't open %s",
                     lm_physical_filename.c_str());
   } else if ( isSuffix(".binlm", lm_physical_filename) ||
               isSuffix(".binlm.gz", lm_physical_filename) ) {
      //cerr << ".binlm" << endl;
      assert(vocab);
      lm = new LMBin(lm_physical_filename, *vocab, UNK_tag, limit_vocab,
                     limit_order, oov_unigram_prob);
   } else {
      //cerr << ".lmtext" << endl;
      lm = new LMText(lm_physical_filename, vocab, UNK_tag, limit_vocab,
                      limit_order, oov_unigram_prob, os_filtered);
   }

   if ( lm->description.empty() ) {
      ostringstream description;
      description << "LanguageModel:" << lm_physical_filename;
      if ( limit_order ) description << "#" << limit_order;
      if ( UNK_tag ) description << ":open-voc";
      if ( oov_unigram_prob != 0.0 )
         description << ":P(oov)=exp(" << oov_unigram_prob << ")";
      lm->description = description.str();
   }

   return lm;
} // PLM::Create

//----------------------------------------------------------------------------
// Get the variable content, if its 0, initialize it from lowerclass
Uint PLM::getOrder()
{
   if(gram_order==0)
      gram_order = getGramOrder();

   return gram_order;
}

//----------------------------------------------------------------------------
void PLM::readLine(istream &in, float &prob, string &ph, float &bo_wt,
                   bool &blank, bool &bo_present)
{
   static string line;
   bool eof = ! getline(in, line);

   if (eof || line == "") {
      blank = true;
      ph="";
   } else {
      // This was done using split, but that's way too slow -- so we do it the
      // old but fast C way instead.
      blank = false;
      char* phrase_pos;
      prob = strtof(line.c_str(), &phrase_pos);
      if (phrase_pos[0] != '\t')
         error(ETWarn, "Expected tab character after prob in LM input file");
      else
         phrase_pos++;
      const char* bo_wt_pos = strchr(phrase_pos, '\t');
      if ( bo_wt_pos == NULL ) {
         bo_present = false;
         bo_wt = 0.0;
         ph = phrase_pos;
      } else {
         bo_present = true;
         bo_wt = strtof(bo_wt_pos+1, NULL);
         ph.assign(phrase_pos, bo_wt_pos - phrase_pos);
      }
   }
} // readLine

const char* PLM::UNK_Symbol = "<unk>";
const char* PLM::SentStart  = "<s>";
const char* PLM::SentEnd    = "</s>";
