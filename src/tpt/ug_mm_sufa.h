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


// (c) 2007-2009 Ulrich Germann. All rights reserved.

#ifndef _ug_mm_sufa_h
#define _ug_mm_sufa_h

#include <boost/iostreams/device/mapped_file.hpp>

#include "ug_sufa_base.h"
#include "ug_mm_ctrack.h"
#include "tpt_tokenindex.h"

namespace ugdiss
{
  using namespace std;
  namespace bio=boost::iostreams;

  class mmSufa : public Sufa
  {
  public:
    class tree_iterator;
    friend class tree_iterator;
  private:
    bio::mapped_file_source file;

  public: // temporarily for debugging

    filepos_type const* index; // random access to top-level sufa ranges

  private:

    char const* index_jump(char const* a, char const* z, float ratio) const;
    char const* getLowerBound(id_type id) const;
    char const* getUpperBound(id_type id) const;
		   
  public:
    mmSufa();
    mmSufa(string fname, Ctrack const* c);
    void open(string fname, Ctrack const* c);

    char const*
    random_sample(char const* p, char const* q) const;

    count_type
    sntCnt(char const* p, char const * const q) const;

    count_type
    rawCnt(char const* p, char const * const q) const;

    void
    getCounts(char const* p, char const * const q, 
              count_type& sids, count_type& raw) const;

    char const* 
    readSid(char const* p, char const* q, id_type& sid) const;

    char const* 
    readOffset(char const* p, char const* q, uint16_t& offset) const;

    void sanityCheck() const;
  };

  class
  mmSufa::
  tree_iterator : public Sufa::tree_iterator
  {
  public:
    tree_iterator(mmSufa const* s);
    bool down();
    bool over();
  };

  /** Open the three memory mapped files associated with a suffix array.
   * @param basename base name for the file
   * @param token_index return TokenIndex for the corpus.
   * @param corpus_track return mmCtrack (memory mapped corpus track).
   * @param suffix_array return mmSufa (memory mapped suffix array).
   *
   * Filenames are of the format basename.tpsa/type, basename/type or
   * basename.type, where type is one of "tdx", "mct", or "msa" as appropriate.
   *
   * The reverse index is also initialized for the token index.
   */
  void
  open_memory_mapped_suffix_array(const string& basename,
                                  TokenIndex& token_index,
                                  mmCtrack& corpus_track,
                                  mmSufa& suffix_array);

}

#endif
