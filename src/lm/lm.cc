/**
 * @author Bruno Laferriere / Eric Joanis
 * @file lm.cc  Implementations of LM methods.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "lm.h"
#include "lmbin_novocfilt.h"
#include "lmbin_vocfilt.h"
#include "lmtext.h"
#include "lmmix.h"
#include "lmdynmap.h"
#include "tplm.h"
#include "lmrestcost.h"
#include "str_utils.h"

using namespace std;
using namespace Portage;

const char* PLM::lm_order_separator = "#";


namespace Portage {
   // Like a typedef, but works for templates.
   // We don't fully declare the type of cache in lm.h so that it only needs
   // to be compiled when compiling lm.o and nothing else.  We care to limit
   // the compiling of LMCache because compiling PTrie is expensive.
   class LMCache : public PTrie<float, Empty, false> {};
}

//----------------------------------------------------------------------------
// Constructors
PLM::PLM(VocabFilter* vocab, OOVHandling oov_handling, float oov_unigram_prob)
   : cache(NULL)
   , vocab(vocab)
   , complex_open_voc_lm(oov_handling == FullOpenVoc)
   , gram_order(0)
   , oov_unigram_prob(oov_unigram_prob)
   , clearCacheEveryXHit(0)
   , clearCacheHit(0)
{}

PLM::PLM()
   : cache(NULL)
   , vocab(NULL)
   , complex_open_voc_lm(false)
   , gram_order(0)
   , oov_unigram_prob(-INFINITY)
   , clearCacheEveryXHit(0)
   , clearCacheHit(0)
{}

PLM::~PLM() {
   delete cache;
}

//----------------------------------------------------------------------------
PLM::Creator::Creator(const string& lm_physical_filename,
                      Uint naming_limit_order)
   : lm_physical_filename(lm_physical_filename)
   , naming_limit_order(naming_limit_order)
{}

bool PLM::Creator::checkFileExists(vector<string>* list)
{
   if (list) list->push_back(lm_physical_filename);
   return (check_if_exists(lm_physical_filename) && !is_directory(lm_physical_filename));
}

Uint64 PLM::Creator::totalMemmapSize()
{
   return 0;
}

bool PLM::Creator::prime(bool full) {
   return true;
}

//----------------------------------------------------------------------------
shared_ptr<PLM::Creator> PLM::getCreator(const string& lm_filename)
{
   string lm_physical_filename(lm_filename);
   const size_t hash_pos = lm_filename.rfind(lm_order_separator);
   Uint naming_limit_order = 0;
   Uint clearCacheEveryXHit = 0;  // Default: turn caching off altogether (1 would clear it at every sentence)
   if ( hash_pos != string::npos ) {
      string option;
      if (conv(lm_filename.substr(hash_pos+1), option)) {
         if (isPrefix("CACHING", option)) {
            clearCacheEveryXHit = 1;  // In case parsing the clearing frequency fails, lets clear on every sentence.
            const size_t freq_pos = option.rfind(",");  // #CACHING,<freq>
            if ( freq_pos != string::npos ) {
               const string hit = option.substr(freq_pos+1);
               if (!conv(hit, clearCacheEveryXHit)) {
                  error(ETWarn, "Unable to convert to digit: %s", hit.c_str());
                  clearCacheEveryXHit = 1;  // Fallback on clear on every sentence.
               }
            }
            else {
               error(ETWarn, "Using default clear cache frequency value which is: %d\n", clearCacheEveryXHit);
            }
         }
         else {
            naming_limit_order = conv<Uint>(option);
         }
      }
      lm_physical_filename.resize(hash_pos);
   }

   Creator* cr;
   if (LMDynMap::isA(lm_physical_filename)) {
      cr = new LMDynMap::Creator(lm_physical_filename, naming_limit_order);
   } else if (LMMix::isA(lm_physical_filename)) {
      cr = new LMMix::Creator(lm_physical_filename, naming_limit_order);
   } else if (TPLM::isA(lm_physical_filename)) {
      cr = new TPLM::Creator(lm_physical_filename, naming_limit_order);
   } else if (LMRestCost::isA(lm_physical_filename)) {
      cr = new LMRestCost::Creator(lm_physical_filename, naming_limit_order);
   } else {
      // EJJ Important note: we must not call LMText::isA() here, because it
      // calls this function! We could call LMBin::isA(), but that would be
      // pointless since LMBin and LMText share the same Creator implementation.
      cr = new LMTrie::Creator(lm_physical_filename, naming_limit_order);
   }
   assert(cr);
   cr->clearCacheEveryXHit = clearCacheEveryXHit;
   return shared_ptr<PLM::Creator>(cr);
}

PLM* PLM::Create(const string& lm_filename,
                 VocabFilter* vocab,
                 OOVHandling oov_handling,
                 float oov_unigram_prob,
                 bool limit_vocab,
                 Uint limit_order,
                 ostream *const os_filtered,
                 bool quiet,
                 const string descriptor)
{
   shared_ptr<Creator> creator(getCreator(lm_filename));
   if ( creator->naming_limit_order )
      if ( limit_order == 0 || creator->naming_limit_order < limit_order )
         limit_order = creator->naming_limit_order;

   if ( vocab ) {
      // Make sure SentStart and SentEnd and UNK_Symbol are always in the
      // vocab, to prevent them from being filtered out while loading the LM.
      vocab->addSpecialSymbol(SentStart);
      vocab->addSpecialSymbol(SentEnd);
      vocab->addSpecialSymbol(UNK_Symbol);
   }

   PLM* lm = creator->Create(vocab, oov_handling, oov_unigram_prob, limit_vocab,
                             limit_order, os_filtered, quiet);
   assert(lm != NULL);

   // If the creator detects in the filename that the lm must not use the
   // caching, let the lm know.
   lm->clearCacheEveryXHit = creator->clearCacheEveryXHit;

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
               creator->lm_physical_filename.c_str());
      } else if ( oov_handling == FullOpenVoc ) {
         error(ETWarn, "Language model %s opened as FullOpenVoc but does not "
               "contain a unigram probability for <unk>.",
               creator->lm_physical_filename.c_str());
      }
      lm->oov_unigram_prob = save_oov_unigram_prob;
   } else {
      if ( oov_handling == ClosedVoc ) {
         error(ETWarn, "Language model %s opened as ClosedVoc but "
               "contains a unigram probability for <unk>.",
               creator->lm_physical_filename.c_str());
         lm->oov_unigram_prob = save_oov_unigram_prob;
      }
   }
   if ( debug_auto_voc )
      cerr << "final oov_unigram_prob " << lm->oov_unigram_prob << endl;

   if ( lm->description.empty() ) {
      ostringstream description;
      description << descriptor << ":" << creator->lm_physical_filename;
      if ( limit_order ) description << lm_order_separator << limit_order;
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

static const bool debug_memmap_size = false;

Uint64 PLM::totalMemmapSize(const string& lm_filename)
{
   if (debug_memmap_size) {
      shared_ptr<Creator> creator = getCreator(lm_filename);
      cerr << "totalMemmapSize for " << creator->lm_physical_filename
           << " = " << creator->totalMemmapSize() << endl;
   }
   return getCreator(lm_filename)->totalMemmapSize();
}

bool PLM::prime(const string& lm_filename, bool full) {
   return getCreator(lm_filename)->prime(full);
}

//----------------------------------------------------------------------------
// Get the variable content, if its 0, initialize it from lowerclass
Uint PLM::getOrder()
{
   if(gram_order==0)
      gram_order = getGramOrder();

   return gram_order;
}

void PLM::clearCache() {
   // If we are using the cache and we've processed enough sentence that the
   // user asked use to clear the cache then clear the cache.
   if ( cache != NULL && ++clearCacheHit >= clearCacheEveryXHit ) {
      cache->clear();
      clearCacheHit = 0;
   }
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

PLM::OOVHandling::OOVHandling(const string& typestring, bool& valid) {
   valid = true;
   if      ( typestring == "ClosedVoc" )     type = ClosedVoc;
   else if ( typestring == "SimpleAutoVoc" ) type = SimpleAutoVoc;
   else if ( typestring == "SimpleOpenVoc" ) type = SimpleOpenVoc;
   else if ( typestring == "FullOpenVoc" )   type = FullOpenVoc;
   else { type = SimpleAutoVoc; valid = false; }
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


bool PLM::checkFileExists(const string& lm_filename, vector<string>* list)
{
   return getCreator(lm_filename)->checkFileExists(list);
}

float PLM::cachedWordProb(Uint word, const Uint context[],
                          Uint context_length)
{
   // Are we even using the cache?
   if (clearCacheEveryXHit == 0) {
      return wordProb(word, context, context_length);
   }
   else {
      if ( !cache ) cache = new LMCache;
      
      // If the query is too large for this model's order, truncate it up front.
      if ( context_length >= getOrder() )
         context_length = getOrder() - 1;

      // Prepare cache query
      Uint query[context_length+1];
      query[0] = word;
      for (Uint i=0;i<context_length;++i)
         query[i+1] = context[i];

      // Search in cache for matching Uint query
      float query_result(0);
      if ( cache->find(query, context_length+1, query_result) )
         return query_result;

      const float result = wordProb(word, context, context_length);
      cache->insert(query, context_length+1, result);
      return result;
   }
}


const char* PLM::UNK_Symbol = "<unk>";
const char* PLM::SentStart  = "<s>";
const char* PLM::SentEnd    = "</s>";


void PLM::Hits::init(Uint gram_order) {
   values.clear();
   values.resize(gram_order+1, 0);
   latest_hit = 0;
}

PLM::Hits& PLM::Hits::operator+=(const Hits& other) {
   if (values.size() < other.values.size()) values.resize(other.values.size(), 0);
   for (Uint i(0); i<values.size(); ++i)
      values[i] += other.values[i];
   if ( other.latest_hit > latest_hit ) latest_hit = other.latest_hit;
   return *this;
}

PLM::Hits PLM::Hits::operator+(const Hits& other) const {
   Hits total(*this);
   total += other;
   return total;
}

void PLM::Hits::display(ostream& out) const {
   char buffer[128];
   const double total_hits = accumulate(values.begin(), values.end(), 0.0f);
   out << "Found k-grams: ";
   for (Uint i(0); i<values.size(); ++i) {
      snprintf(buffer, 126, "%d:%d(%2.0f", i, values[i], double(values[i])/total_hits*100.0f);
      out << buffer << "%) ";
   }
   out << "Total: " << total_hits << endl;
}

