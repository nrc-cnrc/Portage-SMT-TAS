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
#ifndef __ugTokenIndex_hh
#define __ugTokenIndex_hh
#include <iostream>
#include <sstream>
#include <fstream>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include "tpt_typedefs.h"
#include "tpt_constants.h"
#include <vector>

using namespace std;
namespace bio=boost::iostreams;

namespace ugdiss
{
  class TokenIndex
  {
    /** Reverse index: maps from ID to char const* */
    vector<char const*> ridx;
    /** Label for the UNK token */
    string unkLabel; 
    id_type unkId,numTokens;
  public:
    /** string->ID lookup works via binary search in a vector of Entry instances */
    class Entry
    {
    public:
      uint32_t offset;
      id_type  id; 
    };

    /** Comparison function object used for Entry instances */
    class CompFunc
    {
    public:
      char const* base;
      CompFunc();
      bool operator()(Entry const& A, char const* w);
    };

    bio::mapped_file_source file;
    Entry const* startIdx;
    Entry const* endIdx;
    CompFunc comp;
    TokenIndex();
    TokenIndex(string fname,string unkToken=default_unk_token);
    void open(string fname,string unkToken=default_unk_token);
    id_type operator[](char const* w)  const;
    id_type operator[](string const& w)  const;
    char const* const operator[](id_type id) const;
    char const* const operator[](id_type id);
    vector<char const*> reverseIndex() const;

    string toString(vector<id_type> const& v);
    string toString(vector<id_type> const& v) const;

    string toString(id_type const* start, id_type const* const stop);
    string toString(id_type const* start, id_type const* const stop) const;

    void toIdSeq(vector<id_type>& idSeq, string const& line) const;
    void iniReverseIndex();
    id_type getNumTokens() const;
    id_type getUnkId() const;
    char const* const getUnkToken() const;
  };

  /** for sorting words by frequency */
  class compWords
  {
    string unk;
  public: 
    compWords(string _unk) : unk(_unk) {};
    
    bool
    operator()(pair<string,size_t> const& A, 
               pair<string,size_t> const& B) const
    {
      if (A.first == unk) return false;// do we still need this special treatment?
      if (B.first == unk) return true; // do we still need this special treatment?
      if (A.second == B.second)
        return A.first < B.first;
      return A.second > B.second;
    }
  };

  template<class MYMAP>
  void
  mkTokenIndex(string ofile,MYMAP const& M,string unkToken)
  {
    typedef pair<uint32_t,id_type> IndexEntry; // offset and id
    typedef pair<string,uint32_t>  Token;      // token and id


    // first, sort the word list in decreasing order of frequency, so that we 
    // can assign IDs in an encoding-efficient manner (high frequency. low ID)
    vector<pair<string,size_t> > wcounts(M.size()); // for sorting by frequency
    typedef typename MYMAP::const_iterator myIter;
    size_t z=0;
    for (myIter m = M.begin(); m != M.end(); m++)
      {
	// cout << m->first << " " << m->second << endl;
	wcounts[z++] = pair<string,size_t>(m->first,m->second);
      }
    compWords compFunc(unkToken);
    sort(wcounts.begin(),wcounts.end(),compFunc);

    // Assign IDs ...
    vector<Token> tok(wcounts.size()); 
    for (size_t i = 0; i < wcounts.size(); i++)
      tok[i] = Token(wcounts[i].first,i);
    // and re-sort in alphabetical order
    sort(tok.begin(),tok.end()); 

    // Write token strings to a buffer, keep track of offsets
    vector<IndexEntry> index(tok.size()); 
    ostringstream data;
    id_type unkId = tok.size(); 
    for (size_t i = 0; i < tok.size(); i++)
      {
        if (tok[i].first == unkToken)
          unkId = tok[i].second;
        index[i].first  = data.tellp();   // offset of string
        index[i].second = tok[i].second;  // respective ID
        data<<tok[i].first<<char(0);      // write string to buffer
      }
    
    // Now write the actual file
    ofstream out(ofile.c_str());
    if (out.fail())
       cerr << efatal << "Unable to open TokenIndex file '" << ofile << "' for writing."
            << exit_1;
    uint32_t vsize = index.size(); // how many vocab items?
    out.write(reinterpret_cast<char*>(&vsize),4);
    out.write(reinterpret_cast<char*>(&unkId),sizeof(id_type));
    for (size_t i = 0; i < index.size(); i++)
      {
        out.write(reinterpret_cast<char*>(&index[i].first),4);
        out.write(reinterpret_cast<char*>(&index[i].second),sizeof(id_type));
      }
    out<<data.str();
  }

}
#endif
