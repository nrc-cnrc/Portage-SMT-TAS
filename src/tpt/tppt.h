// This file is derivative work from Ulrich Germann's Tightly Packed Tries
// package (TPTs and related software).
//
// Original Copyright:
// Copyright 2005-2009 Ulrich Germann; all rights reserved.
// Under licence to NRC.
//
// Copyright for modifications:
// Technologies langagieres interactives / Interactive Language Technologies
// Inst. de technologie de l'information / Institute for Information Technology
// Conseil national de recherches Canada / National Research Council Canada
// Copyright 2008-2010, Sa Majeste la Reine du Chef du Canada /
// Copyright 2008-2010, Her Majesty in Right of Canada



// (c) 2007,2008 Ulrich Germann

/** Tightly Packed Phrase Tables
 *  written by Ulrich Germann
 */
#ifndef __tppt_hh
#define __tppt_hh

#include <boost/iostreams/device/mapped_file.hpp>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <stdint.h>
#include "tpt_tokenindex.h"
#include "tpt_bitcoder.h"

namespace bio = boost::iostreams;

namespace ugdiss
{
  class TpPhraseTable
  {
  public:

    class TCand
    {
    public:
      vector<string> words;
      vector<float>  score;
      string toString();
    };
    typedef boost::shared_ptr<vector<TCand> > val_ptr_t;

    class Node
    {
      TpPhraseTable* root;
      char const* idxStart;
      char const* idxStop;
      char const* valStart;
      val_ptr_t valPtr;
    public:
      boost::shared_ptr<Node> find(string const& wrd);
      Node(TpPhraseTable* _root, char const* p, uchar flags);
      Node();
      val_ptr_t const& value(bool cacheValue=true);
      void dump(ostream& out, string prefix);
    };
    typedef boost::shared_ptr<Node> node_ptr_t;

    /** Class Local Phrase Table: a two dimensional table of /boost::shared_ptr/s 
     *  to vectors of translation candidates mapping from startRange, endRange pairs
     *  to translation candidates for the respective source phrases, given a sentence.
     */
    class LocalPT 
    { 
    private:
      vector<vector<val_ptr_t> > T;
    public:
      val_ptr_t get(uint32_t start, uint32_t stop);
      friend class TpPhraseTable;
    };

  private: 
    /**
     *  Top-level index entry
     */
    struct IndexEntry
    {
      filepos_type offset;
      uchar         flags;
    };


    bio::mapped_file_source       codebook;
    bio::mapped_file_source      indexFile;
    bio::mapped_file_source       trgRepos;
    BitCoder<uint32_t>      trgPhraseCoder;
    vector<BitCoder<uint32_t> > scoreCoder;
    vector<float const*>             score;

    // auxiliary functions
    void openCodebook(string fname);
    void openTrgRepos(string fname);

    char const* idxBase;
    id_type     numTokens;
    map<char const*,val_ptr_t> cache;
    
    /// Get the model base name from its name
    static string getBasename(const string& fname);

    /// Sanity checking for index file position.
    void check_index_position(char const* p);

  public:
    TokenIndex srcVcb;
    TokenIndex trgVcb;
    TpPhraseTable();
    TpPhraseTable(string fname);
    void open(string fname);
    node_ptr_t find(string const& word);
    // vector<TCand> readValue(char const* p);
    void clearCache();

    /** look up translation candidates for a single phrase */
    val_ptr_t lookup(vector<string> const& snt, uint32_t start, uint32_t stop);

    /** creates a local phrase table (see documentation for TpPhraseTable::LocalPT)
     *  for sentence /snt/
     *  @param  snt input sentence
     *  @return a local phrase table
     */
    LocalPT mkLocalPhraseTable(vector<string> const& snt);
    void dump(ostream& out);
    
    /** count the number of score columns in the model /fname/
     */
    static uint32_t numScores(string fname);

    /** return false if any of the files associated for fname are missing
     */
    static bool checkFileExists(const string& fname);

    /** return the total size of the files associated with fname, counting any
     *  missing ones as having size 0.
     */
    static uint64_t totalMemmapSize(const string& fname);
  };


} // end of namespace ugdiss 

namespace Portage {
  typedef ugdiss::TpPhraseTable TPPT;
}

#endif
