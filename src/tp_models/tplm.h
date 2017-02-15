/**
 * @author Ulrich Germann
 * @file tplm.h Wrapper for back-off language models encoded as tightly packed tries.
 *
 * ATTENTION: This code is currently not safe for multi-threading.
 *            1. The lm currently stores some lookup statistics in
 *               member variables that are currently not under the
 *               protection of a lock.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef __LMTPT_H__
#define __LMTPT_H__

#define LMTPTQ_DEBUG_LOOKUP 0

// IN_PORTAGE determines whether the LM is part of Portage or a stand-alone class
#ifndef IN_PORTAGE
#define IN_PORTAGE 1
#endif

#if IN_PORTAGE
#include "portage_defs.h"
#include "../lm/lm.h"
#endif

// #include "nqlm_entry.hh"
// #include "ugRoTrie.hh"
#include "tpt_tokenindex.h"
#include "tpt_typedefs.h"
#include "tpt_pickler.h"
#include "tpt_tightindex.h"
#include "tpt_utils.h"
#include <vector>
#include <string>
#include <boost/iostreams/device/mapped_file.hpp>
#include <cassert>
#include <boost/static_assert.hpp>

#if IN_PORTAGE
#else
  typedef ugdiss::id_type Uint;
#endif
namespace bio=boost::iostreams;
namespace Portage {
using namespace ugdiss;
using namespace std;

/** LM implementation for LMs represented as tightly packed tries.
 *  It is implemented as a template to accommodate both IRSTLM-stype quantized
 *  and regular arpa-stype back-off LMs.
 *  @param valIdType is the type of value id:
 *  uchar for IRST-style quantized LMs and id_type for 'unquantized' LMs
 */
template<typename valIdType>
class LMtpt
#if IN_PORTAGE
  : public PLM
#endif
{
public:
  class Entry;  // a wrapper for the value of each node in the trie
  class Reader; // a function object to read that value from file

private:
  // typedef RoTrie<Entry,Reader> trie_t;
  // typedef typename trie_t::node_t node_t;

  Reader reader;

  /** The actual trie structure representing the LM */
  // trie_t trie;
  bio::mapped_file_source file;
  char const* idxStart;
  size_t topLevelRecSize;

  /** Code book mapping from score IDs to actual scores */
  bio::mapped_file_source codebook;

  /** Sizes of code books for probabilities for each n-gram length */
  vector<size_t> cbsize_pval;

  /** Pointers to the first entry in each codebook for prob. vals */
  vector<float const*> pval;

  /** Sizes of code books for back-off weights for each n-gram length */
  vector<size_t> cbsize_bow;

  /** Pointers to the first entry in each codebook for back-off weights */
  vector<float const*> bow;

  /** Pointer to vector of unigram probability indices */
  valIdType const* uniprob;

  /** Sometimes IDs between 0 and some 'first actual' ID X are reserved
   *  (e.g. for NULL word and UNK) and not part of the language model. In this
   *  case we have to shift the IDs before computing offsets. This variable
   *  determines the shift value.  (You can also think of it as the ID of the
   *  "first actual" LM token. UNSAFE, NOT TESTED!
   */
  size_t wid_shift;

  /** Size of the vocabulary of the LM. (=highest ID-wid_shift+1) */
  size_t numTokens;

  /** Maps from 'LM-external' word id to 'LM-internal' one */
  inline
  Uint mapId(Uint id);

  /** Stores the length of the longest context found for the most recent lookup */
  size_t longest_context;
  /** Stores the length of the longest n-gram found for the most recent lookup */
  size_t longest_ngram;

  /** For caching lookups */
  vector<map<vector<Uint>,float> > luCache;

#if IN_PORTAGE
  /** Maps from external word ids to internal ones. Right now we build the
   *  vector at load time. We should consider precomputing it or think about a
   *  way of unifying IDs for processing. */
  vector<Uint> idmap;

  /** Keep track of what vocab type this is */
  OOVHandling oov_policy;

  /** A value to indicate that a value has not been mapped yet from the
   *  external VocabFilter ID to the internal TokenIndex ID */
  Uint UNMAPPED;
#else
  /** Order of highest ngram stored */
  size_t gram_order; // remove when used with portage!
  float  oov_unigram_prob;
#endif

  float private_wordProb(Uint word, const Uint context[], Uint context_length);

public:

  /** Maps from token strings to LM-internal IDs and vice versa */
  TokenIndex tindex;

#if IN_PORTAGE
  /// Return true if lm_physical_filename describes a TPLM
  static bool isA(const string& lm_physical_filename) {
    return isSuffix(".tplm", lm_physical_filename) || isSuffix(".tplm/", lm_physical_filename);
  }

  struct Creator : public PLM::Creator {
    Creator(const string& lm_physical_filename, Uint naming_limit_order);
    virtual bool checkFileExists(vector<string>* list);
    virtual Uint64 totalMemmapSize();
    virtual bool prime(bool full = false);
    virtual PLM* Create(VocabFilter* vocab,
                        OOVHandling oov_handling,
                        float oov_unigram_prob,
                        bool limit_vocab,
                        Uint limit_order,
                        ostream *const os_filtered,
                        bool quiet);
  private:
    const string basename;
    static string getBasename(const string& lm_physical_filename);
  };

#endif

  /** Constructor
   *  @param basename base name of all the component files
   *  @param vocab shared vocab object for all models
   *  @param oov_handling: how to handle OOVs
   *         (4 possible policies, see PLM documentation in lm.h)
   * @param oov_unigram_prob   the unigram prob of OOVs (if oov_handling ==
   *                           ClosedVoc)
   */
  LMtpt(string          basename
#if IN_PORTAGE
         ,
         VocabFilter*       vocab,
         OOVHandling oov_handling,
         float   oov_unigram_prob
#endif
         );
  /** @return order of the language model */
  virtual Uint getGramOrder();

  /** Get the word represented by index in this language model.
   *  @param index  index to look up.
   *  @return  the word index maps to, or NULL if not in the vocabulary
   */
  virtual char const* word(Uint index) const;

  /**
    * log probability of a word in context.
    *
    * Calculate log(p(word|context)), where context is reversed, i.e., for
    * p(w4|w1 w2 w3), you must pass word=w4, and context=(w3,w2,w1).
    * If w4 is not found, uses p(w4) = oov_unigram_prob in the back off
    * calculations, unless isUNK_tagged is true, in which case the LM's
    * predictions for unknown words applies.
    * @param word               word whose prob is desired
    * @param context            context for word, in reverse order
    * @param context_length     length of context
    * @return log(p(word|context))
    */
  virtual float wordProb(Uint word, const Uint context[], Uint context_length);

  virtual Uint minContextSize(const Uint context[], Uint context_length);

  /** @return a string that describes a wordProb lookup request */
  string describeRequest(Uint word, const Uint ctxt[], Uint context_length);

  /** Unigram log probability
   *  @param word word Id of word in question
   */
  float wordProb(Uint word);

  /** Has no effect for this LM */
  //virtual void clearCache(){};

  virtual Uint getLatestNgramDepth() const { return longest_ngram; }

  // Additional functions specific to this LM
  /** @return the longest ngram found in the most recent call to wordProb(id,ctxt,ctxt_len) */
  size_t longestNgram() const   { return longest_ngram; }
  /** @return the longest ngram found in the most recent call to wordProb(id,ctxt,ctxt_len) */
  size_t longestContext() const { return longest_context; }

}; // end of class declaration LMtpt

#if IN_PORTAGE
typedef LMtpt<id_type> TPLM;
BOOST_STATIC_ASSERT(sizeof(id_type) == sizeof(Uint));

const char* const tplm_extensions[] = { "tdx", "cbk", "tplm" };

template<typename valIdType>
LMtpt<valIdType>::Creator::Creator(
      const string& lm_physical_filename, Uint naming_limit_order)
   : PLM::Creator(lm_physical_filename, naming_limit_order)
   , basename(getBasename(lm_physical_filename))
{
}

template<typename valIdType>
bool LMtpt<valIdType>::Creator::checkFileExists(vector<string>* list)
{
  bool ok = true;
  for ( int i = 0; i < 3; ++i )
  {
    if ( ! check_if_exists(basename + tplm_extensions[i], false) )
    {
      cerr << ewarn << "TPLM(" << lm_physical_filename << ") can't access: "
           << basename << tplm_extensions[i] << endl;
      ok = false;
      // if the .tplm file itself is missing, don't complain about the others,
      // since they're nearly guaranteed not to exist either.
      if ( i == 0 ) break;
    }
  }
  if (list) list->push_back(lm_physical_filename);
  return ok;
}

template<typename valIdType>
Uint64 LMtpt<valIdType>::Creator::totalMemmapSize()
{
  uint64_t total_memmap_size = 0;
  for ( int i = 0; i < 3; ++i ) {
    uint64_t memmap_size = getFileSize(basename + tplm_extensions[i]);
    if (memmap_size == uint64_t(-1))
      cerr << "Error: TPLM sub file " << (basename + tplm_extensions[i]) << " does not exist" << exit_1;
    total_memmap_size += memmap_size;
  }
  return total_memmap_size;
}

template<typename valIdType>
bool LMtpt<valIdType>::Creator::prime(bool full)
{
   cerr << "\tPriming: " << basename << endl;  // SAM DEBUGGING
   for ( int i = 0; i < 3; ++i ) {
      if ( ! check_if_exists(basename + tplm_extensions[i], false) ) {
         cerr << ewarn << "TPLM(" << lm_physical_filename << ") can't access: "
            << basename << tplm_extensions[i] << endl;
      }
      else {
         gulpFile(basename + tplm_extensions[i]);
      }
   }

   return true;
}

template<typename valIdType>
PLM* LMtpt<valIdType>::Creator::Create(VocabFilter* vocab,
                            OOVHandling oov_handling,
                            float oov_unigram_prob,
                            bool limit_vocab,
                            Uint limit_order,
                            ostream *const os_filtered,
                            bool quiet)
{
   if ( ! checkFileExists(NULL) )
      cerr << efatal << "Unable to open TPLM " << lm_physical_filename
           << " or one of its associated files." << exit_1;
   if ( limit_order )
      cerr << ewarn << "Ignoring LM order limit for TPLM "
           << lm_physical_filename << endl;
   return new LMtpt<valIdType>(basename, vocab, oov_handling, oov_unigram_prob);
}

template<typename valIdType>
string LMtpt<valIdType>::Creator::getBasename(
   const string& lm_physical_filename)
{
   ifstream file_in_subdir((lm_physical_filename+"/tplm").c_str());
   if ( file_in_subdir )
      return lm_physical_filename + "/";
   else if ( isSuffix(".tplm", lm_physical_filename) )
      return lm_physical_filename.substr(0,lm_physical_filename.size()-4);
   else
      return lm_physical_filename;
}

#endif

template<typename valIdType>
LMtpt<valIdType>::
LMtpt(string basename
#if IN_PORTAGE
       ,
       VocabFilter* vocab,
       OOVHandling oov_handling,
       float oov_unigram_prob
#endif
       )
  :
#if IN_PORTAGE
 PLM(vocab,oov_handling,oov_unigram_prob),
#endif
   // trie(Entry(),Reader()),

   longest_context(0),
   longest_ngram(0)
#if IN_PORTAGE
   ,oov_policy(oov_handling)
#endif
{
#if !IN_PORTAGE
  basename += ".";
#endif
  open_mapped_file_source(file, basename+"tplm");
  uint64_t fSize = getFileSize(basename+"tplm");
  if (fSize < sizeof(filepos_type))
    cerr << efatal << "Empty tplm file '" << basename << "tplm'." << exit_1;
  filepos_type iStart;
  numread(file.data(),iStart);
  if (fSize < iStart)
    cerr << efatal << "Bad index start (" << iStart << ") in tplm file '"
         << basename << "tplm'." << exit_1;
  this->idxStart = file.data()+iStart;
  this->topLevelRecSize = sizeof(filepos_type)+1;
  // trie.open(basename+"dat");
  tindex.open(basename+"tdx");
  tindex.iniReverseIndex();
#if IN_PORTAGE
  UNMAPPED = tindex.getNumTokens()+1;
#endif

  // Aug 13, 2008: Codebook handling is still at the experimental stage.
  //
  // We currently open it twice. First as a regular file to get some
  // essential parameters, and then a second time as a memory mapped file
  // giving us "direct" access to the database.
  //
  // We may at some point want to implement it in a way that we have to open the
  // code book only once.
  //
  // Get essential information from codebook file. The order and encoding of
  // elements must be coordinated with the code at the end of int main() in
  // irst.encode.cc.
  ifstream cb((basename+"cbk").c_str());
  wid_shift = uchar(cb.get());        // see explanation in lmtpt.h
  binread(cb,this->numTokens); // vocab size
  this->gram_order = cb.get(); // max. n-gram order
#if IN_PORTAGE
  hits.init(this->gram_order);
#endif
  size_t x;
  for (size_t i = 0; i < this->gram_order; i++)
    { // get sizes of the code books for probabilities for
      // each n-gram order up to gram_order
      binread(cb,x);
      cbsize_pval.push_back(x);
    }
  for (size_t i = 0; i+1 < this->gram_order; i++)
    { // get sizes of the code books for back-off weights for
      // each n-gram order up to gram_order
      binread(cb,x);
      cbsize_bow.push_back(x);
    }
  size_t offset_to_first_pval = cb.tellg();
  if (cb.fail())
    cerr << efatal << "Bad tplm codebook file '" << basename << "cbk'." << exit_1;
  cb.close();

  // Now we re-open the codebook as a memory-mapped file and set the pointers
  // to the respective first element of the various probability value and
  // back-off weight code books.
  open_mapped_file_source(codebook, basename+"cbk");
  x = offset_to_first_pval;
  for (size_t i = 0; i < cbsize_pval.size(); i++)
    {
      pval.push_back(reinterpret_cast<float const*>(codebook.data()+x));
      x += cbsize_pval[i]*sizeof(float);
    }
  for (size_t i = 0; i < cbsize_bow.size(); i++)
    {
      bow.push_back(reinterpret_cast<float const*>(codebook.data()+x));
      x += cbsize_bow[i]*sizeof(float);
    }
  uniprob = reinterpret_cast<valIdType const*>(codebook.data()+x) - wid_shift;
  if (x >= codebook.size())
    cerr << efatal << "Bad tplm codebook file '" << basename << "cbk'." << exit_1;

  if (fSize < iStart + this->numTokens*this->topLevelRecSize)
     cerr << efatal << "Mismatch between codebook file '" << basename
          << "cbk' and tplm file '" << basename << "tplm'." << endl
          << "Vocab size of " << this->numTokens << "is too big."
          << exit_1;

#if IN_PORTAGE
  // Build the mapping from external vocab IDs to LM-internal ids.
  // In the long run, it would be better to precompute this table
  // rather than constructing it each time the LM is loaded.
  // The problem right now is that words are currently added to the
  // external vocabulary dynamically, so we don't know a-priori
  // all the words in the vocab.
  // For starters, we give idmap a reasonable size. If needed, we grow the
  // array later.
  idmap.resize(tindex.getNumTokens(), UNMAPPED);
#if 0
  // OOVHandlers don't allow assignment after construction ???
  if (oov_policy == SimpleAutoVoc)
    oov_policy = (tindex.getUnkId() == tindex.getNumTokens()
                  ? PLM::ClosedVoc : PLM::SimpleOpenVoc);
#endif
  luCache.resize(tindex.getNumTokens());

#else
  // i.e., not in Portage
  this->oov_unigram_prob = -7; // log_10 of unigram prob
#endif
}


/** Maps from external word ID to internal one */
template<typename valIdType>
inline
Uint
LMtpt<valIdType>::
mapId(Uint id)
{
#if IN_PORTAGE
  if (id >= idmap.size())\
    {
      assert(id < vocab->size());
      idmap.resize(vocab->size(),UNMAPPED);
    }
  Uint& x = idmap[id];
  if (x == UNMAPPED)
    {
      assert(id < vocab->size());
      x = tindex[vocab->word(id)];
    }
  return x;
#else
  return id;
#endif
}


template<typename valIdType>
Uint
LMtpt<valIdType>::
getGramOrder()
{
  return this->gram_order;
}

template<typename valIdType>
char const*
LMtpt<valIdType>::
word(Uint index)
const
{
#if IN_PORTAGE
  return this->vocab->word(index);
#else
  return tindex[index];
#endif
}

template<typename valIdType>
float
LMtpt<valIdType>::
wordProb(Uint word)
{
  this->longest_context=0;
  Uint w = mapId(word);
  if (w == tindex.getUnkId() && w == tindex.getNumTokens())
    {
      this->longest_ngram=0;
      return this->oov_unigram_prob; // as set externally
    }
  this->longest_ngram=1;
  assert(w >= wid_shift);
  assert(w < wid_shift+numTokens);
#if LMTPTQ_DEBUG_LOOKUP
  cerr << "unigram pvalId=" << uniprob[w] << " pval=" << pval[0][uniprob[w]] << endl;
#endif
  return pval[0][uniprob[w]];
}

/** string representation of a lookup request; for debugging
 *  @param w word id in question
 *  @param ctxt context/history in reverse order (rightmost word first)
 *  @param ctxtlen context/history length */
template<typename valIdType>
string
LMtpt<valIdType>::
describeRequest(Uint w, Uint const ctxt[], Uint ctxtlen)
{
  ostringstream buf;
  for (int k = 0; k < int(ctxtlen); k++)
    buf << (k?",":" ") << mapId(ctxt[k]);
  buf << " => " << mapId(w) << endl;

  buf << "p(" << tindex[mapId(w)];
  for (int k = 0; k < int(ctxtlen); k++)
    buf << (k?",":"|") << tindex[mapId(ctxt[ctxtlen-k-1])];
  buf << ")";
  return buf.str();
}

template<class valIdType>
float
LMtpt<valIdType>::
wordProb(Uint word, const Uint context[], Uint context_length)
{
#if 0
  vector<Uint> key(context,context+context_length);
  map<vector<Uint>,float>::iterator m;
  if (word >= luCache.size())
    {
      luCache.resize(word+1000);
    }
  else
    {
      m = luCache[word].find(key);
      if (m != luCache[word].end())
        return m->second;
    }
  // ATTENTION: longest_ngram and longest_context won't be set!
  float ret = private_wordProb(word,context,context_length);
  luCache[word].insert(map<vector<Uint>,float>::value_type(key,ret));
  return ret;
#else
  const float logprob = private_wordProb(word,context,context_length);
#if IN_PORTAGE
  hits.hit(this->longest_ngram);
#endif
  return logprob;
#endif

}

template<class valIdType>
float
LMtpt<valIdType>::
private_wordProb(Uint word, const Uint context[], Uint context_length)
{

#if LMTPTQ_DEBUG_LOOKUP
  cerr << "\n" << describeRequest(word,context,context_length) << endl;
#endif

  if (context_length == 0)
    return wordProb(word);

  this->longest_context=777; /* nonsense initialization for tracking failure to set it
                              * properly; for debugging */
  Uint w = mapId(word);

// #if 0
//   // Oct 10, 2008: Apparently the following is not the behavior we want. [UG]
//   // if w is UNK, just return the oov unigram probability,
//   // unless the LM handles UNK as an explicit token
//   if (w == tindex.getUnkId()
// #if IN_PORTAGE
//       && !(oov_policy == FullOpenVoc)
// #endif
//       )
//     {
//       this->longest_ngram=0;
//       return this->oov_unigram_prob;
//     }
// #endif

#if IN_PORTAGE
  bool isNotUnk = w!=tindex.getUnkId() || (oov_policy==FullOpenVoc);
#else
  bool isNotUnk = w!=tindex.getUnkId();
#endif

  float uniGramProb = (isNotUnk
                       ? pval[0][uniprob[w]]
                       : this->oov_unigram_prob);

  // the following code is a bit ugly but optimized for speed
  // vpos: an array of offset positions of the node values
  //       to save time, we delay reading the values and read them
  //       only if we need them. If we were to use node_t instances,
  //       we'd automatically read them via the node_t constructor
  //       even if we end up not needing them
  //istream& f = *(trie.file);
  filepos_type   offset;
  uint64_t diff;
  char const* vpos[context_length];
  uchar flags;
  Uint cwid = mapId(context[0]);
  if (cwid==tindex.getUnkId()
#if IN_PORTAGE
      && !(oov_policy == FullOpenVoc)
#endif
      )
    {
      this->longest_ngram=1;
      this->longest_context=0;
      // return pval[0][uniprob[w]];
      return uniGramProb;
    }


  // f.seekg(trie.idxStart+(sizeof(filepos_type)+1)*cwid); // +1 is for the /flags/ uchar
  char const *p = numread(idxStart + (cwid * topLevelRecSize),offset);
  if (!offset)
    {
      this->longest_ngram=1;
      // return pval[0][uniprob[w]];
      return uniGramProb;
    }
  int  i=-1;
  Uint cx=1;
  flags  = *p;
  float bowsum=0;
  if (!flags)
    bowsum = bow[i+1][offset];
  else
    {
      char const* iStop = file.data()+offset;
      while (flags)
        {
#if LMTPTQ_DEBUG_LOOKUP
          cerr << "found " << tindex[cwid] << " i=" << i << " cx=" << cx
               << " cl=" << context_length << " flags=" << Uint(flags)
               << " offset=" << offset
               << " order=" << gram_order
               << endl;
#endif
          bool has_child = flags & HAS_CHILD_MASK;
          char const* p = (has_child
                           ? binread(iStop,diff)
                           : iStop);
          if (flags & HAS_VALUE_MASK)
            vpos[++i] = p;
          if (!has_child || cx == context_length)
            break;
          cwid = mapId(context[cx]);
          if (cwid == tindex.getUnkId())
            break;
          cx++;
          // cerr << "[1] " << cwid << endl;
          char const* iStart = iStop - diff;
          p = tightfind(iStart,iStop,cwid,flags);
          // cerr << "[2] " << int(p) << endl;
          if (!p)
            break;
          tightread(p,iStop,diff);
          if (flags)
            iStop = iStart - diff;
          else
            {
#if LMTPTQ_DEBUG_LOOKUP
              cerr << "found " << tindex[cwid] << " i=" << i << " cx=" << cx
                   << " cl=" << context_length << " flags=" << Uint(flags)
                   << " offset=" << offset
                   << " order=" << gram_order
                   << endl;
#endif
              bowsum = bow[i+1][diff];
              break;
            }
        }
    }

#if LMTPTQ_DEBUG_LOOKUP
  cerr << "i = " << i << " cx = " << cx << endl;
#endif

  // We've found the longest matching context, we now backtrack until we find a
  // match for the word in question
  this->longest_context = cx;
  id_type pvalId;
  if (i>=0)
    {
      Entry E;
      // f.seekg(vpos[i]);
      // (*(trie.reader))(f,E);
      reader(vpos[i],E);
      if (isNotUnk && E.find_pidx(w,pvalId))
        {
          this->longest_ngram = i+2;
#if LMTPTQ_DEBUG_LOOKUP
          cerr << longest_ngram << "-gram pvalId=" << pvalId;
          cerr << " pval=" << pval[i+1][pvalId]
               << " bowsum=" << bowsum
               << endl;
#endif
          // if (longest_ngram == gram_order)
          return pval[i+1][pvalId]+bowsum; // [i][E.bo_idx];
          // else
          // return pval[i+1][pvalId]+bow[i][E.bo_idx];
        }
      else
        bowsum += bow[i][E.bo_idx];

#if LMTPTQ_DEBUG_LOOKUP
      cerr << i << " bow=" << bow[i][E.bo_idx] << endl;
#endif
      for (--i; i >= 0; i--)
        {
          // f.seekg(vpos[i]);
          // (*(trie.reader))(f,E);
          reader(vpos[i],E);
          if (isNotUnk && E.find_pidx(w,pvalId))
            {
              this->longest_ngram = i+2;
              break;
            }
          bowsum += bow[i][E.bo_idx];
#if LMTPTQ_DEBUG_LOOKUP
          cerr << i << " bow=" << bow[i][E.bo_idx] << endl;
#endif
        }
    }
  if (i<0)
    {
      pvalId = uniprob[w];
      this->longest_ngram = 1;
    }
#if LMTPTQ_DEBUG_LOOKUP
  cerr << "returning ";
  if (this->longest_ngram==1)
    cerr << uniGramProb+bowsum;
  else
    cerr << pval[this->longest_ngram-1][pvalId]+bowsum;
  cerr << endl;
#endif
  if (this->longest_ngram==1)
    return uniGramProb+bowsum;
  else
    return pval[this->longest_ngram-1][pvalId]+bowsum;
} // end of function private_wordProb

template<class valIdType>
Uint
LMtpt<valIdType>::
minContextSize(const Uint context[], Uint context_length)
{

#if LMTPTQ_DEBUG_LOOKUP
  cerr << "\nminContextSize: " << describeRequest(0,context,context_length) << endl;
#endif

  if (context_length == 0)
    return 0;

  this->longest_context=777; /* nonsense initialization for tracking failure to set it
                              * properly; for debugging */

  // the following code is a bit ugly but optimized for speed
  filepos_type   offset;
  uint64_t diff;
  uchar flags;
  Uint cwid = mapId(context[0]);
  if (cwid==tindex.getUnkId()
#if IN_PORTAGE
      && !(oov_policy == FullOpenVoc)
#endif
      )
    {
      return 0;
    }


  // f.seekg(trie.idxStart+(sizeof(filepos_type)+1)*cwid); // +1 is for the /flags/ uchar
  char const *p = numread(idxStart + (cwid * topLevelRecSize),offset);
  if (!offset)
    {
      return 0;
    }
  Uint cx=1;
  flags  = *p;
  if (flags)
    {
      char const* iStop = file.data()+offset;
      while (flags)
        {
#if LMTPTQ_DEBUG_LOOKUP
          cerr << "found " << tindex[cwid] << " cx=" << cx
               << " cl=" << context_length << " flags=" << Uint(flags)
               << " offset=" << offset
               << " order=" << gram_order
               << endl;
#endif
          bool has_child = flags & HAS_CHILD_MASK;
          char const* p = (has_child
                           ? binread(iStop,diff)
                           : iStop);
#if LMTPTQ_DEBUG_LOOKUP
          cerr << "   has_child: " << has_child;
#endif
          if (!has_child || cx == context_length)
            break;
          cwid = mapId(context[cx]);
#if LMTPTQ_DEBUG_LOOKUP
          cerr << "   cwid: " << cwid << (cwid == tindex.getUnkId() ? "(unk)" : "") << endl;
#endif
          if (cwid == tindex.getUnkId())
            break;
          // cerr << "[1] " << cwid << endl;
          char const* iStart = iStop - diff;
          p = tightfind(iStart,iStop,cwid,flags);
          // cerr << "[2] " << int(p) << endl;
          if (!p)
            break;
          ++cx;
          tightread(p,iStop,diff);
          if (flags)
            iStop = iStart - diff;
          else
            {
#if LMTPTQ_DEBUG_LOOKUP
              cerr << "found " << tindex[cwid] << " cx=" << cx
                   << " cl=" << context_length << " flags=" << Uint(flags)
                   << " offset=" << offset
                   << " order=" << gram_order
                   << endl;
#endif
              break;
            }
        }
    }

#if LMTPTQ_DEBUG_LOOKUP
  cerr << " cx = " << cx << endl;
#endif

  // We've found the longest matching context, return it.

  // We could backtrack until we find a node that has continuations or a
  // non-zero back-off weight, but if the substring assumption holds, every
  // context node has continuations by definition, so we don't bother with the
  // actual check.

  return cx;
} // end if function minContextSize

template<typename valIdType>
class
LMtpt<valIdType>::
Entry
{
  friend class LMtpt<valIdType>;
  friend class LMtpt<valIdType>::Reader;
  /** pointer to the data stream where the entry 'resides' */
  istream* file;

  /** start point of data block in /file/ that maps from word Ids to the respective
   *  probability value IDs */
  // filepos_type startPIdx;
  char const* startPIdx;

  /** end point of data block in /file/ that maps from word Ids to the respective
   *  probability value IDs */
  // filepos_type stopPIdx;
  char const* stopPIdx;
  /** Id of backoff weight */
  valIdType bo_idx;

public:
  /** retrieves probability value Id for a word, given the context represented by
   *  the node in the trie
   *  @param key external Id of the prob. value id
   *  @param val where to store the value Id retrieved
   *  @return true if found, false otherwise */

  bool find_pidx(id_type key, id_type& val) const
#if 1
    ;
#else
  {
    cerr << "OOPS, lmtpt::Entry::find_pindx() should "
         << "have a specialized implementation!" << endl;
    assert(0);
    return false;
  }
#endif
  // NOTE: this function needs to be implemented via template specializations,
  // because of differences in the underlying data structures for IRST-style
  // quantization and 'regular' LMs

  /** constructor */
  Entry() : file(NULL), startPIdx(0), stopPIdx(0), bo_idx(0) {};

  /** constructor with initialization from an integer */
  Entry(filepos_type dummy)
    : file(NULL), startPIdx(0), stopPIdx(0), bo_idx(dummy) {};
};

template<typename valIdType>
class
LMtpt<valIdType>::
Reader
{
public:
  void operator()(istream& in, LMtpt<valIdType>::Entry& v) const;
  char const* operator()(char const* p, LMtpt<valIdType>::Entry& v) const;
};

}  // end of namespace Portage
#endif
