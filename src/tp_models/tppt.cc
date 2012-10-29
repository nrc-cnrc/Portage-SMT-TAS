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

#include "tppt.h"
#include "tppt_config.h"
#include <cmath>
#include "tpt_typedefs.h"
#include "tpt_pickler.h"
#include "tpt_tightindex.h"
#include "repos_getSequence.h"
#include "tpt_utils.h"
#include <iostream>
#include <fstream>

#ifndef rcast
#define rcast reinterpret_cast
#endif

namespace ugdiss
{

   TpPhraseTable::
   TpPhraseTable() 
      : idxBase(0) 
      , tppt_version(0)
      , third_col_count(0)
      , fourth_col_count(0)
      , num_counts(0)
      , has_alignment(false)
   {}
  
   TpPhraseTable::
   TpPhraseTable(const string& fname)
      : idxBase(0) 
      , tppt_version(0)
      , third_col_count(0)
      , fourth_col_count(0)
      , num_counts(0)
      , has_alignment(false)
   {
      this->open(fname);
   }

   void 
   TpPhraseTable::
   openCodebook(const string& fname)
   {
      assert(sizeof(float) == 4);
      open_mapped_file_source(codebook, fname);
      uint32_t const* p = rcast<uint32_t const*>(codebook.data());
      uint32_t const* end = rcast<uint32_t const*>(codebook.data()+codebook.size());
      if (p >= end)
         cerr << efatal << "Bad tppt codebook file '" << fname << "'." << exit_1;
      size_t numBooks = *p++;
      uint32_t code_book_version = 1;
      if (numBooks == 0) {
         using namespace TPPTConfig;
         if (0 == strncmp(reinterpret_cast<const char*>(p), code_book_magic_number_v2,
                          strlen(code_book_magic_number_v2))) {
            //cerr << "Code book format v2" << endl;
            code_book_version = 2;
            p += strlen(code_book_magic_number_v2) / 4;
            numBooks = *p++;
         }
      }

      if (tppt_version == 1)
         third_col_count = numBooks;

      uint32_t num_floats = third_col_count + fourth_col_count;
      if (num_counts) {
         if (numBooks != num_floats + 1)
            cerr << efatal << "Wrong number of codebooks found in " << fname
                 << " expected " << num_floats << " float codebooks and one uint32_t, "
                 << "found " << numBooks << " codebooks instead." << exit_1;
      } else {
         if (numBooks != num_floats)
            cerr << efatal << "Wrong number of codebooks found in " << fname
                 << " expected " << num_floats << " float codebooks, "
                 << "found " << numBooks << " codebooks instead." << exit_1;
      }

      scoreCoder.resize(numBooks);
      score.resize(numBooks);
      count_base = NULL;
      for (size_t i = 0; i< numBooks; i++)
      {
         if (p+2 >= end)
            cerr << efatal << "Bad tppt codebook file '" << fname << "'." << exit_1;
         bool is_float = true;
         if (code_book_version == 2) {
            if (0 == strncmp(reinterpret_cast<const char*>(p), "float   ", 8))
               is_float = true;
            else
            if (0 == strncmp(reinterpret_cast<const char*>(p), "uint32_t", 8))
               is_float = false;
            else
               cerr << efatal << "Bad tppt codebook v2 file '" << fname << "': invalid data type." << exit_1;
            p += 2;
         }
         if (i < num_floats) {
            if (!is_float)
               cerr << efatal << "Mismatch between found and expected codebook data types in " << fname
                    << " codebook " << i << " is uint32_t, expected float." << exit_1;
         } else {
            if (is_float)
               cerr << efatal << "Mismatch between found and expected codebook data types in " << fname
                    << " codebook " << i << " is float, expected uint32_t." << exit_1;
         }

         size_t numVals   = *p++;
         size_t numBlocks = *p++;
         if (p+numBlocks >= end)
            cerr << efatal << "Bad tppt codebook file '" << fname << "'." << exit_1;
         vector<uint32_t> b(numBlocks);
         for (size_t k = 0; k < numBlocks; k++)
            b[k] = *p++;
         scoreCoder[i].setBlocks(b);
         if (is_float)
            score[i] = rcast<float const*>(p);
         else
            count_base = p;
         p += numVals;
      }
   }

   void 
   TpPhraseTable::
   openTrgRepos(const string& fname)
   {
      open_mapped_file_source(trgRepos, fname);
      char const* p = trgRepos.data() + trgRepos.size();
      // Go backwards in the file until you find the third byte from the end
      // with the stop bit set, and use the offset of the position after that
      // to determine the number of bits.
      --p;
      for (--p; *p>=0 && p > trgRepos.data(); p--); 
      for (--p; *p>=0 && p > trgRepos.data(); p--); 

      // The calculation of tbits is incorrect.
      // It is, in general, 1 bit bigger than it needs to be, but fixing
      // this problem here and in the ptable.assemble program would render
      // current TPPT files unreadable, because this value is not stored
      // anywhere in the TPPT.
      // The correct calculation is:
      // uint32_t tbits = int(ceil(log2((++p-trgRepos.data())+1)));
      uint32_t tbits = int(ceil(log2(++p-trgRepos.data())))+1;
      trgPhraseCoder.setBlocks(vector<uint32_t>(1,tbits)); 
   }

   static bool isSuffix(const string& s1, const string& s2)
   {
      return s1.size() <= s2.size() &&
         0 == s2.compare(s2.size()-s1.size(), s1.size(), s1);
   }

   void
   TpPhraseTable::
   open(const string& fname)
   {
      if (!checkFileExists(fname))
         cerr << efatal << "Unable to open '" << fname << "' tppt for reading."
            << exit_1;

      string bname = getBasename(fname);

      tppt_version = TPPTConfig::read(bname+"config",third_col_count, fourth_col_count,
                                      num_counts, has_alignment);

      // Note that the files other than the index file (tppt) are assumed to
      // be < 4Gb in size, i.e. 32-bit offsets are sometimes assumed.
      openCodebook(bname+"cbk");
      openTrgRepos(bname+"trg.repos.dat");
      srcVcb.open(bname+"src.tdx");
      trgVcb.open(bname+"trg.tdx");


      open_mapped_file_source(indexFile, bname+"tppt");
      uint64_t fSize = getFileSize(bname+"tppt");
      if (fSize < sizeof(filepos_type)+sizeof(id_type))
         cerr << efatal << "Empty tppt index file '" << fname << "'." << exit_1;
      filepos_type idxOffset;
      char const* p = indexFile.data();
      p = numread(p,idxOffset);
      p = numread(p,numTokens);
      idxBase = indexFile.data()+idxOffset;
      if (fSize < idxOffset + numTokens*(sizeof(filepos_type)+1))
         cerr << efatal << "Bad tppt index file '" << fname << "'." << exit_1;

#if 0
      for (size_t i = 0; i < numTokens; i++)
      {
         char const* p = idxBase + i*(sizeof(filepos_type)+1);
         cerr << i
            << " " << *rcast<filepos_type const*>(p) 
            << " " << int(*rcast<uchar const*>(p+sizeof(filepos_type)))
            << endl;

      }
#endif
   }

//   vector<TpPhraseTable::TCand>
//   TpPhraseTable::
//   readValue(char const* p)
//   {
//      uint32_t numEntries;
//      p = binread(p,numEntries);
//      vector<TCand> ret(numEntries);
//      pair<char const*,unsigned char> z(p,0);
//      for (size_t i = 0; i < numEntries; i++)
//      {
//         uint32_t trgPhraseOffset,scoreId;
//         z = trgPhraseCoder.readNumber(z.first,z.second,trgPhraseOffset);
//         ret[i].words = getSequence(trgRepos.data()+trgPhraseOffset,trgVcb);
//         ret[i].score.resize(score.size());
//         for (size_t k = 0; k < score.size(); k++)
//         {
//            uint32_t scoreId;
//            z = scoreCoder[k].readNumber(z.first,z.second,scoreId);
//            ret[i].score[k] = score[k][scoreId];
//         }
//      }
//      return ret;
//   }

   TpPhraseTable::
   Node::
   Node()
      : root(NULL)
   {
      idxStart=idxStop=valStart=0;
   }

   TpPhraseTable::
   Node::
   Node(TpPhraseTable* _root, char const* p, uchar flags)
      : root(_root)
   {
      root->check_index_position(p);
      if (flags&HAS_CHILD_MASK)
      {
         idxStop = p;
         uint32_t offset;
         p = binread(p,offset);
         idxStart = idxStop-offset;
         root->check_index_position(idxStart);
      }
      else
      {
         idxStart = idxStop = NULL;
      }
      valStart = (flags&HAS_VALUE_MASK) ? p : NULL;
   }

   void
   TpPhraseTable::
   check_index_position(char const* p)
   {
      if (p < indexFile.data() || p >= idxBase)
         cerr << efatal << "Encountered bad node offset: " << (p - indexFile.data())
              << exit_1;
   }

   // find member for NODE
   TpPhraseTable::node_ptr_t
   TpPhraseTable::Node::
   find(string const& word) 
   {
      id_type wid = root->srcVcb[word];
      //cerr << "[2] wid=" << wid << endl;
      if (wid == root->srcVcb.getUnkId()) 
         return TpPhraseTable::node_ptr_t();
      if (wid > root->numTokens)
         cerr << efatal << "Encountered bad wid: " << wid << exit_1;
      uchar flags;
      char const* p = tightfind(idxStart,idxStop,wid,flags);
      if (!p)
         return TpPhraseTable::node_ptr_t();
      filepos_type offset;
      tightread(p,idxStop,offset);
      return TpPhraseTable::node_ptr_t(new Node(root,idxStart-offset,flags));
   }

   // find member for TABLE
   TpPhraseTable::node_ptr_t
   TpPhraseTable::
   find(string const& word)
   {
      const id_type wid  = srcVcb[word];
      if (wid == srcVcb.getUnkId()) 
         return TpPhraseTable::node_ptr_t();
      //cerr << "wid=" << wid << endl;
      if (wid > numTokens)
         cerr << efatal << "Encountered bad wid: " << wid << exit_1;

      filepos_type offset = *rcast<filepos_type const*>(idxBase+wid*(sizeof(filepos_type)+1));
      //cerr << "offset=" << offset << endl;
      if (!offset)
         return TpPhraseTable::node_ptr_t();

      // uchar flags = *rcast<uchar const*>(idxBase+wid+1);
      uchar flags 
         = *rcast<uchar const*>(idxBase+(wid+1)*(sizeof(filepos_type)+1)-1);
      // According to Uli, I should comment this line out because the check fails
      // if there is no top level entry for the word
      //assert(flags&HAS_VALUE_MASK);
      char const* p = indexFile.data()+offset;
      return TpPhraseTable::node_ptr_t(new Node(this,p,flags));
   }

   TpPhraseTable::val_ptr_t const&
   TpPhraseTable::Node::
   value(bool cacheValue)
   {
      const uint32_t num_floats = root->third_col_count + root->fourth_col_count;
      if (root && valStart && !valPtr)
      {
         typedef map<char const*,TpPhraseTable::val_ptr_t>::iterator myIter;
         myIter m = root->cache.find(valStart);
         if (m != root->cache.end())
            valPtr = m->second;
         else
         {
            uint32_t numPhrases;
            char const* p = binread(valStart,numPhrases);
            valPtr.reset(new vector<TCand>(numPhrases));
            pair<char const*,unsigned char> z(p,0);
            BitCoder<uint32_t>&          t = root->trgPhraseCoder;
            vector<BitCoder<uint32_t> >& s = root->scoreCoder;
            vector<TCand>& v = *valPtr;
            for (size_t i = 0; i < numPhrases; ++i)
            {
               uint32_t trgPhraseOffset,scoreId;
               z = t.readNumber(z.first,z.second,trgPhraseOffset);
               if (trgPhraseOffset >= root->trgRepos.size())
                  cerr << efatal << "Bad trg.repos.dat file." << exit_1;
               char const* q = root->trgRepos.data()+trgPhraseOffset;
               //EJJ v[i].words = getSequence(q,root->trgVcb);
               v[i].words.reserve(5);
               getSequence(v[i].words, q, root->trgVcb);
               v[i].score.resize(num_floats);
               // Expand all the compressed scores into a vector<float>, using
               // their respective codebooks.
               for (size_t k = 0; k < num_floats; ++k)
               {
                  z = s[k].readNumber(z.first,z.second,scoreId);
                  //cerr << "score id [" << k << "]: " << scoreId << endl;
                  v[i].score[k] = root->score[k][scoreId];
               }
               if (root->num_counts) {
                  v[i].counts.resize(root->num_counts);
                  for (uint32_t k = 0; k < root->num_counts; ++k) {
                     z = s[num_floats].readNumber(z.first,z.second,scoreId);
                     v[i].counts[k] = root->count_base[scoreId];
                  }
               }
               if (root->has_alignment) {
                  // Once implemented, this is where we'll read the alignment information.
                  assert(false);
               }
            }
            if (cacheValue)
               root->cache[valStart] = valPtr;
         }
      }
      return valPtr;
   }

   void 
   TpPhraseTable::
   clearCache() 
   {
      cache.clear();
   }


   TpPhraseTable::val_ptr_t
   TpPhraseTable::lookup(vector<string> const& snt, uint32_t start, uint32_t stop)
   {
      if (stop==start) 
         return val_ptr_t(); 
      assert(stop > start && start < snt.size() && stop <= snt.size());
      node_ptr_t n = find(snt[start]);
      for (size_t i = start+1; i < stop && n != NULL; i++)
         n = n->find(snt[i]);
      if (n == NULL)
         return val_ptr_t();
      return n->value();
   }

   TpPhraseTable::val_ptr_t
   TpPhraseTable::LocalPT::
   get(uint32_t start, uint32_t stop)
   {
      if (start >= T.size())
         return TpPhraseTable::val_ptr_t();
      assert(stop>start);
      uint32_t x = stop-start-1;
      if (x >= T[start].size()) 
         return val_ptr_t();
      return T[start][x];
   }

   TpPhraseTable::LocalPT
   TpPhraseTable::
   mkLocalPhraseTable(vector<string> const& snt)
   {
      LocalPT LPT;
      if (snt.size() == 0)
         return LPT;
      LPT.T.resize(snt.size());
      for (size_t start = 0; start < snt.size(); start++)
      {
         node_ptr_t n = find(snt[start]);
         if (n != NULL)
         {
            size_t stop = start+1;
            LPT.T[start].push_back(n->value());
            while (stop < snt.size())
            {
               n = n->find(snt[stop++]);
               if (n == NULL) break;
               LPT.T[start].push_back(n->value());
            } 
         }
      }
      return LPT;
   }

   void
   TpPhraseTable::
   dump(ostream& out)
   {
      char const* p = idxBase;
      for (size_t i = 0; i < numTokens; i++)
      {
         filepos_type offset = *rcast<filepos_type const*>(p);
         p += sizeof(filepos_type);
         uchar flags = *p++;
         if (offset)
            Node(this,indexFile.data()+offset,flags).dump(out,srcVcb[i]);
      }
   }

   void
   TpPhraseTable::Node::
   dump(ostream& out,string prefix)
   {
      TpPhraseTable::val_ptr_t v = value(false);
      if (v != NULL)
      {
         for (size_t i = 0; i < v->size(); i++)
         {
            vector<string>   const&  w = (*v)[i].words;
            vector<float>    const&  s = (*v)[i].score;
            vector<uint32_t> const&  c = (*v)[i].counts;
            out << prefix << " ||| ";
            for (size_t k = 0; k < w.size(); ++k)
               out << w[k] << " ";
            out << "|||";
            assert(s.size() == root->third_col_count + root->fourth_col_count);
            for (size_t k = 0; k < root->third_col_count; ++k)
               out << " " << s[k];
            if (root->num_counts) {
               assert(c.size() == root->num_counts);
               out << " c=" << c[0];
               for (size_t k = 1; k < c.size(); ++k)
                  out << "," << c[k];
            }
            if (root->has_alignment) {
               // Once implemented, we need to write out the a= field here.
               assert(false);
            }
            if (root->fourth_col_count) {
               out << " |||";
               for (size_t k = root->third_col_count; k < s.size(); ++k)
                  out << " " << s[k];
            }
            out << endl;
         }
      }

      id_type id; id_type flagmask = FLAGMASK; filepos_type offset;
      for (char const* p = idxStart; p < idxStop;)
      {
         p = tightread(p,idxStop,id);
         p = tightread(p,idxStop,offset);
         uchar flags = id&flagmask;
         id >>= FLAGBITS;
         Node(root,idxStart-offset,flags).dump(out,prefix+" "+root->srcVcb[id]);
      }
   }

   string
   TpPhraseTable::TCand::
   toString()
   {
      ostringstream buf;
      for (size_t i = 0; i < words.size(); i++)
         buf << words[i] << " ";
      buf << "|||";
      for (size_t i = 0; i < score.size(); i++)
         buf << " " << score[i];
      if (!counts.empty()) {
         buf << " c=" << counts[0];
         for (size_t i = 1; i < counts.size(); ++i)
            buf << "," << counts[i];
      }
      return buf.str();
   }

   void TpPhraseTable::numScores(const string& fname, uint32_t& third_col, uint32_t& fourth_col,
                                 uint32_t& counts, bool& has_alignment)
   {
      uint32_t tppt_version = TPPTConfig::read(getBasename(fname) + "config",
            third_col, fourth_col, counts, has_alignment);
      if (tppt_version == 1) {
         ifstream cbk((getBasename(fname) + "cbk").c_str());
         if (!cbk) return;
         numread(cbk,third_col);
      } else if (tppt_version == 2) {
         // TPPTConfig::read() did all the work, nothing more to do.
      } else
         assert(false);
   }

   static bool warnIfNotExists(const string& fname)
   {
      ifstream file(fname.c_str());
      if ( !file ) {
         cerr << ewarn << "File '" << fname << "' does not exist." << endl;
         return false;
      } else {
         return true;
      }
   }

   static const char* const tppt_extensions[] = {
      "tppt", "cbk", "trg.repos.dat", "src.tdx", "trg.tdx"
   };
   static const unsigned int NUM_EXTENSIONS = 5;

   string TpPhraseTable::getBasename(const string& fname)
   {
      ifstream file_in_subdir((fname+"/tppt").c_str());
      if ( file_in_subdir ) 
         return fname + "/";
      else if ( isSuffix(".tppt", fname) )
         return fname.substr(0, fname.size()-4);
      else
         return fname + ".";
   }

   bool TpPhraseTable::checkFileExists(const string& fname)
   {
      string bname = getBasename(fname);
      bool ok = true;
      for ( unsigned int i = 0; i < NUM_EXTENSIONS; ++i ) {
         if ( ! warnIfNotExists(bname+tppt_extensions[i]) ) {
            ok = false;
            // if the .tppt file itself is missing, don't complain about the
            // others, since they're nearly guaranteed not to exist either.
            if ( i == 0 ) break; 
         }
      }
      return ok;
   }

   uint64_t TpPhraseTable::totalMemmapSize(const string& fname)
   {
      string bname = getBasename(fname);
      uint64_t total_memmap_size = 0;
      for ( unsigned int i = 0; i < NUM_EXTENSIONS; ++i )
         total_memmap_size += getFileSize(bname+tppt_extensions[i]);
      return total_memmap_size;
   }

} // end of namespace ugdiss
