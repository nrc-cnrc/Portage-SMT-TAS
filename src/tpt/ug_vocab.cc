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



/**
 * @author Ulrich Germann
 * @file ug_vocab.cc Vocabulary with word count capabilities.
 *
 * Vocab is mutable (can add items on the fly).
 *
 *  (c) 2006,2007,2008 Ulrich Germann.
 */
#include <iostream>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <algorithm>

#ifdef WITH_BOOST_IOSTREAMS
#include <boost/iostreams/categories.hpp> // input_filter_tag
#include <boost/iostreams/operations.hpp> // get, WOULD_BLOCK
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#endif

#include "ug_vocab.h"
#include "tpt_pickler.h"
#include "tpt_constants.h"

namespace ugdiss
{
  void 
  Vocab::clear()
  {
    sIdx.clear();
    nIdx.clear();
  }

  void
  Vocab::init()
  {
    mxStrLen = totalCount = refCount = 0;
    nIdx.push_back(sIdx.insert(EntryType("NULL", IdCntPair(0,0))).first);
    nIdx.push_back(sIdx.insert(EntryType(default_unk_token,  IdCntPair(1,0))).first);
  }

  Vocab::Vocab()
  {
    init();
  }

  Vocab::Vocab(string fname)
  {
    open(fname);
  }

  void 
  Vocab::open(string fname)
  {
    if (fname.size()>4 && fname.substr(fname.size()-5,4)==".bz2")
      open(fname,BZGIZA);
    else if (fname.size()>4 && fname.substr(fname.size()-5,4)==".bin")
      open(fname,BIN);
    else
      open(fname,TXT);
  }

  Vocab::Vocab(string filename, FILEMODE mode)
  {
    open(filename,mode);
  }

  void
  Vocab::open(string filename,FILEMODE mode)
  {
    init();
    if (access(filename.c_str(),F_OK))
      cerr << efatal << "Could not open vocab file '" << filename
                << "' for reading." << exit_1;
    if (mode == BIN) 
      {
        ifstream in(filename.c_str(),ios::in);
        this->unpickle(in);
        in.close();
      }
    else 
      {
        id_type    ID;
        count_type cnt;
        string  S;
#ifdef WITH_BOOST_IOSTREAMS
        using namespace boost::iostreams;
        using namespace std;
        ifstream raw(filename.c_str(), ios_base::in | ios_base::binary);
        filtering_istream in;
        if (mode == BZGIZA) in.push(bzip2_decompressor());
        in.push(raw);
#else
        ifstream in(filename.c_str());
#endif
        while (in >> ID >> S >> cnt)
          {
            // cout << ID << " " << cnt << endl;
            if (ID >= nIdx.size()+VOCAB_STARTID)
              nIdx.resize(ID+VOCAB_STARTID+1,nIdx[0]);
            EntryType foo(S, IdCntPair(ID, cnt));
            nIdx[ID] = sIdx.insert(foo).first;
            if (mxStrLen < S.size())
              mxStrLen = S.size();
          }
      }
  }


  vector<id_type>
  Vocab::
  toIdSeq(string const& foo,bool createIfNecessary)
  {
    istringstream buf(foo);
    vector<id_type> retval;
    string w;
    while (buf>>w) 
      retval.push_back(createIfNecessary ? (*this)[w].id : get(w).id);
    return retval;
  }

  bool
  Vocab::
  exists(string const& wrd)
  {
    iterator i = sIdx.find(wrd);
    return i != sIdx.end();
  }

  Vocab::Word
  Vocab::operator[](string const& S)
  {
    iterator i = sIdx.find(S);
    if (i == sIdx.end()) /* it's a new item */
      {
        id_type ID = nIdx.size()+VOCAB_STARTID;
        EntryType newItem(S,IdCntPair(ID,0));
        i = sIdx.insert(newItem).first;
        nIdx.push_back(i);
        if (mxStrLen < S.size()) { mxStrLen = S.size(); }
      }
    return Word(i->second.first,i->first,i->second.second);
  }

  Vocab::Word 
  Vocab::get(string const& S)
  {
    Vocab::iterator i = sIdx.find(S);
    if (i == sIdx.end()) /* it's a new item */
      {
        assert(nIdx.size());
        iterator j = nIdx[0];
        return Word(0, j->first, j->second.second);
      }
    return Word(i->second.first,i->first,i->second.second);
  }

  Vocab::Word
  Vocab::operator[](id_type id) const
  {
    if (id>=nIdx.size()+VOCAB_STARTID)
      {
        return Word(1, nIdx[1]->first, nIdx[1]->second.second);
      }
    else
      {
        iterator i = nIdx[id];
        return Word(id, i->first, i->second.second);
      }
  }

  /* Re-assign IDs so that frequent words have lower IDs (more space efficent
   * when encoding corpora in binary format) */
  void
  Vocab::optimizeIDs()
  {
    /* sort words in descending order of counts: */
    sort(this->nIdx.begin()+2,this->nIdx.end(),cmpWrdsByCnt());

    /* re-assign IDs so that more frequent items get lower ID's
     * (lower ID numbers require less space in our extending,
     * storage scheme for integers. During this process we also 
     * determine the mxStrLen 
     */
    for (size_t i = 2; i < this->nIdx.size(); i++)
      {
        iterator foo = nIdx[i];
        foo->second.first = i + VOCAB_STARTID;
        if (this->mxStrLen < foo->first.size())
          { this->mxStrLen = foo->first.size(); }
      }
  }

  /** Sort Vocab alphabetically */
  void
  Vocab::sortAlphabetically()
  {
    /* sort words in ascending alphabeticalorder: */
    sort(this->nIdx.begin()+2,this->nIdx.end(),cmpWrdsByAlpha());

    /* re-assign IDs so that more frequent items get lower ID's
     * (lower ID numbers require less space in our extending,
     * storage scheme for integers. During this process we also 
     * determine the mxStrLen 
     */
    for (size_t i = 2; i < this->nIdx.size(); i++)
      {
        iterator foo = nIdx[i];
        foo->second.first = i + VOCAB_STARTID;
        if (this->mxStrLen < foo->first.size())
          { this->mxStrLen = foo->first.size(); }
      }
  }

  filepos_type 
  Vocab::pickle(string ofile)
  {
    ofstream out(ofile.c_str());
    if (out.fail())
      cerr << efatal << "Could not open vocab file '" << ofile << "' for writing."
           << exit_1;
    filepos_type ret = this->pickle(out);
    if (out.fail())
      cerr << efatal << "Writing vocab file '" << ofile << "' failed." << exit_1;
    out.close();
    return ret;
  }
  
  filepos_type
  Vocab::pickle(ostream& out)
  {
    filepos_type myPos = out.tellp();
    /* now actually write data in binary format */
    iterator m = sIdx.begin();
    binwrite(out,sIdx.size());       // Vocabulary size
    binwrite(out,mxStrLen);          // length of longest string
                                     // (for buffer allocation when loading)
    binwrite(out,totalCount);        // sum of counts of all vocab items
    // cerr << "Vocab size: " << sIdx.size() << endl;
    // cerr << "max string length: " << mxStrLen << endl;
    // cerr << "total word count: " << totalCount << endl;
    
    binwrite(out,size_t(0));         // start position for first string 
    string const* a = &(m->first);
    binwrite(out,a->size());         // how many bytes to read for first string
    out.write(a->c_str(),a->size()); // write string
    binwrite(out,m->second.first);   // ID
    binwrite(out,m->second.second);  // count
    while(++m != this->sIdx.end())
      {
        /* successive strings are written in the following format:
         * start at position /z/ in buffer, read /n/ characters into buffer */
        string const* b = &(m->first);
        size_t z;
        for (z = 0; z < a->size() && z < b->size() && (a->at(z) == b->at(z)); z++);
        if (z < b->size())
          {
            binwrite(out, z);
            binwrite(out, b->size()-z);
            out.write(b->c_str()+z, b->size()-z);
            binwrite(out, m->second.first);
            binwrite(out, m->second.second);
          }
        a = b;
      }
    return myPos;
  }
  
  void Vocab::unpickle(istream& in)
  {
    assert (!in.eof());
    if (!in.good())
      cerr << efatal << "Unable to read vocab file due to EOF or error." << exit_1;
    uint64_t VocabSize;
    binread(in, VocabSize); 
    // cerr << "Vocab size: " << VocabSize << endl;
    nIdx.resize(VocabSize);
    binread(in, mxStrLen); 
    // cerr << "max string length: " << mxStrLen << endl;
    char buf[mxStrLen+1];
    binread(in, totalCount);
    // cerr << "total word count: " << totalCount << endl;
    bool recount = (totalCount == 0);
    uint64_t strt, numchars, ID, cnt;
    for (size_t i = 0; i < VocabSize; i++)
      {
        binread(in,strt);
        // cerr << "strt=" << strt << endl;
        binread(in,numchars);
        // cerr << "numchars=" << numchars << endl;
        in.read(buf+strt,numchars);
        buf[strt+numchars] = 0;
        binread(in,ID);
        binread(in,cnt);
        // cerr << "ID=" << ID << " " << buf << " count=" << cnt << endl;
        if (recount) totalCount += cnt;
        EntryType foo(buf,IdCntPair(ID,cnt));
        iterator m = sIdx.insert(foo).first;
        nIdx[ID-VOCAB_STARTID] = m;
      }
    if (in.fail())
      cerr << efatal << "Reading vocab file failed." << exit_1;
  }

  void 
  Vocab::dump(ostream& out, bool writeSpecials, bool withP, bool withCumP,
              bool noColumns)
  {
    using namespace std;
    double cump=0;
    size_t lexcolwidth = min(count_type(60),mxStrLen);
    for (size_t i = writeSpecials ? 0 : 2; i < nIdx.size(); i++)
      {
        Vocab::iterator m = nIdx[i];
        if (noColumns)
          out  << m->second.first  << " "
               << m->first         << " "
               << m->second.second;
        else
          // NOTE: doesn't necessarily work properly, depending on the
          // encoding. The standard string class assumes every character
          // is exactly one byte, which is not true for utf8, for example
          out  << setw(10) << m->second.first
               << right  << " " << setw(lexcolwidth) << m->first
               << right << " " << setw(10) << m->second.second;
        if (withP)
          {
            double px = p(i);
            out << " ";
            if (!noColumns)
              out << setw(10);
            out << px;
            if (withCumP)
              {
                out << " ";
                if (!noColumns)
                  out << setw(10);
                out << (cump+=px);
              }
          }
        out << endl;
      }
  }

  Vocab&
  Vocab::operator=(Vocab const& other)
  {
    sIdx = other.sIdx;
    nIdx = other.nIdx;
    mxStrLen = other.mxStrLen;
    totalCount = other.totalCount;
    return *this;
  }


  Vocab::iterator
  Vocab::begin() { return sIdx.begin(); }

  Vocab::iterator
  Vocab::end() { return sIdx.end(); }

  ostream& operator<<(ostream& out, Vocab& V)
  { V.pickle(out);  return out; };

  istream& operator>>(istream& in,  Vocab& V)
  { V.unpickle(in); return in; };

  double 
  Vocab::p(id_type id)
  { return double((*this)[id].cnt)/totalCount; }

  double 
  Vocab::p(string wrd)
  { return double((*this)[wrd].cnt)/totalCount; }

  string
  Vocab::toString(vector<id_type> const& v, size_t start, size_t stop) const
   {
      if (v.size() == 0) return "";
      if (stop==0) stop = v.size();
      if (start==stop) return "";
      string ret = (*this)[v[start]].str;
      for (size_t i = start+1; i < stop; i++)
         {
            ret += string(" ")+(*this)[v[i]].str;
         }
      return ret;
   }

  string
  Vocab::toString(vector<id_type>::const_iterator a,
                  vector<id_type>::const_iterator const& z) const
  {
    if (a==z) return "";
    string ret = (*this)[*a].str;
    for (++a; a < z; ++a)
      ret += string(" ")+(*this)[*a].str;
    return ret;
  }

  void
  Vocab::toTokenIndex(ostream& out)
  {
    typedef map<string, IdCntPair>::iterator myIter;

    uint32_t vsize = this->size(); // how many vocab items?
    out.write(reinterpret_cast<char*>(&vsize),4);
    id_type unkId=1;
    out.write(reinterpret_cast<char*>(&unkId),sizeof(id_type));
    ostringstream data;
    for (myIter m = sIdx.begin(); m != sIdx.end(); m++)
      {
        uint32_t offset = data.tellp();
        out.write(reinterpret_cast<char*>(&offset), 4);
        out.write(reinterpret_cast<char*>(&m->second.first), sizeof(id_type));
        data << m->first << char(0);
      }
    out<<data.str();
  }


} // end namespace ugdiss
