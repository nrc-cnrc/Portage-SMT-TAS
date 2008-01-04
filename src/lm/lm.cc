/**
 * @author Bruno Laferriere / Eric Joanis
 * @file lm.cc  Implementations of LM methods.
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "lm.h"
#include "lmbin.h"
#include "lmbin_vocfilt.h"
#include "lmtext.h"
#include "lmmix.h"
#include <string>
#include <assert.h>
#include <str_utils.h>

using namespace std;
using namespace Portage;

//----------------------------------------------------------------------------
// Constructors
PLM::PLM(VocabFilter* vocab, OOVHandling oov_handling, float oov_unigram_prob) :
   vocab(vocab), complex_open_voc_lm(oov_handling == FullOpenVoc),
   gram_order(0), oov_unigram_prob(oov_unigram_prob) {}

PLM::PLM() :
   vocab(NULL), complex_open_voc_lm(false), gram_order(0),
   oov_unigram_prob(-INFINITY) {}

//----------------------------------------------------------------------------
PLM* PLM::Create(const string& lm_filename, 
                 VocabFilter* vocab,
                 OOVHandling oov_handling,
                 float oov_unigram_prob,
                 bool limit_vocab,
                 Uint limit_order,
                 ostream *const os_filtered,
                 bool quiet)
{
   string lm_physical_filename(lm_filename);
   const size_t hash_pos = lm_filename.rfind("#");
   if ( hash_pos != string::npos ) {
      Uint naming_limit_order = conv<Uint>(lm_filename.substr(hash_pos+1));
      if ( limit_order && naming_limit_order < limit_order )
         limit_order = naming_limit_order;
      lm_physical_filename.resize(hash_pos);
   }

   if ( vocab ) {
      // Make sure SentStart and SentEnd and UNK_Symbol are always in the vocab
      vocab->addSpecialSymbol(SentStart);
      vocab->addSpecialSymbol(SentEnd);
      vocab->addSpecialSymbol(UNK_Symbol);
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
   }
   else if ( isSuffix(".mixlm", lm_physical_filename)) {
      lm = new LMMix(lm_physical_filename, vocab, oov_handling,
               oov_unigram_prob, limit_vocab, limit_order);
   }
   else {
      string line;
      {
         IMagicStream is(lm_physical_filename, true);
         getline(is, line);
      }
      if ( line == "Portage BinLM file, format v1.0" ) {
         //cerr << "BinLM v1.0" << endl;
         assert(vocab);
         if ( limit_vocab )
            lm = new LMBinVocFilt(lm_physical_filename, *vocab, oov_handling,
                     oov_unigram_prob, limit_order);
         else
            lm = new LMBin(lm_physical_filename, *vocab, oov_handling,
                     oov_unigram_prob, limit_order);
      }
      else {
         //cerr << ".lmtext" << endl;
         lm = new LMText(lm_physical_filename, vocab, oov_handling,
                  oov_unigram_prob, limit_vocab, limit_order, os_filtered, quiet);
      }
   }

   static const bool debug_auto_voc = false;

   assert(vocab);
   const Uint unk_index = vocab->index(UNK_Symbol);
   const float save_oov_unigram_prob = oov_unigram_prob;
   lm->oov_unigram_prob = -INFINITY; // Because wordProb() might use it!
   lm->oov_unigram_prob = lm->wordProb(unk_index, &unk_index, 0);
   if ( debug_auto_voc )
      cerr << "oov_handling " << oov_handling.toString() << endl
           << "old oov_unigram_prob " << save_oov_unigram_prob << endl
           << "new oov_unigram_prob " << lm->oov_unigram_prob << endl;
   if ( lm->oov_unigram_prob == -INFINITY ) {
      if ( oov_handling == SimpleOpenVoc ) {
         error(ETWarn, "Language model %s opened as SimpleOpenVoc but does not "
               "contain a unigram probability for <unk>.",
               lm_physical_filename.c_str());
      } else if ( oov_handling == FullOpenVoc ) {
         error(ETWarn, "Language model %s opened as FullOpenVoc but does not "
               "contain a unigram probability for <unk>.",
               lm_physical_filename.c_str());
      }
      lm->oov_unigram_prob = save_oov_unigram_prob;
   } else {
      if ( oov_handling == ClosedVoc ) {
         error(ETWarn, "Language model %s opened as ClosedVoc but "
               "contains a unigram probability for <unk>.",
               lm_physical_filename.c_str());
         lm->oov_unigram_prob = save_oov_unigram_prob;
      }
   }
   if ( debug_auto_voc )
      cerr << "final oov_unigram_prob " << lm->oov_unigram_prob << endl;

   assert(lm != NULL);
   if ( lm->description.empty() ) {
      ostringstream description;
      description << "LanguageModel:" << lm_physical_filename;
      if ( limit_order ) description << "#" << limit_order;
      switch ( oov_handling.type ) { 
         case OOVHandling::ClosedVoc: break;
         case OOVHandling::SimpleOpenVoc  : description << ";SimpleOpenVoc";  break;
         case OOVHandling::SimpleAutoVoc  : description << ";SimpleAutoVoc";  break;
         case OOVHandling::FullOpenVoc : description << ";FullOpenVoc"; break;
      }
      if ( oov_handling == ClosedVoc && oov_unigram_prob != -INFINITY )
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
const char* PLM::word(Uint index) const
{
   if ( vocab && index < vocab->size() )
      return vocab->word(index);
   else
      return "";
}

//----------------------------------------------------------------------------
string PLM::OOVHandling::toString() const {
   switch ( type ) {
      case ClosedVoc: return "ClosedVoc";
      case SimpleOpenVoc: return "SimpleOpenVoc";
      case SimpleAutoVoc: return "SimpleAutoVoc";
      case FullOpenVoc: return "FullOpenVoc";
   }
   assert(false);
   return "";
}

const PLM::OOVHandling PLM::ClosedVoc(OOVHandling::ClosedVoc);
const PLM::OOVHandling PLM::SimpleOpenVoc(OOVHandling::SimpleOpenVoc);
const PLM::OOVHandling PLM::SimpleAutoVoc(OOVHandling::SimpleAutoVoc);
const PLM::OOVHandling PLM::FullOpenVoc(OOVHandling::FullOpenVoc);

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


bool PLM::check_file_exists(const string& lm_filename)
{
   // Remove the limit_order parameter
   string lm_physical_filename(lm_filename);
   const size_t hash_pos = lm_filename.rfind("#");
   if ( hash_pos != string::npos ) {
      lm_physical_filename.resize(hash_pos);
   }

   // Special case
   if ( isSuffix(".mixlm", lm_physical_filename)) {
      return LMMix::check_file_exists(lm_physical_filename);
   }
   return check_if_exists(lm_physical_filename);
}


const char* PLM::UNK_Symbol = "<unk>";
const char* PLM::SentStart  = "<s>";
const char* PLM::SentEnd    = "</s>";
