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
/**
 * @author Ulrich Germann
 * @file ptable.assemble.cc 
 * @brief Assemble the final TPPT from the output of ptable.encode-scores and
 * ptable.encode-phrases.
 *
 * Final step in textpt2tppt conversion. Normally called via textpt2tppt.sh.
 */

// YET TO BE DONE: also encode phrase-internal alignments (optional)

#include <cmath>

#include <boost/iostreams/device/mapped_file.hpp>

//#define DEBUG_TPT

#include "tpt_typedefs.h"
#include "tpt_tokenindex.h"
#include "tpt_tightindex.h"
#include "tpt_pickler.h"
#include "tpt_bitcoder.h"
#include "tpt_utils.h"
#include "tppt_config.h"

static char help_message[] = "\n\
ptable.assemble [-h] OUTPUT_BASE_NAME\n\
\n\
  Sort the entries and assemble the encoded phrase table (TPPT) from the\n\
  various intermediate files.\n\
\n\
  The TPPT index file OUTPUT_BASE_NAME.tppt is created.\n\
\n\
  This is the final step in the conversion of text phrase tables to tightly\n\
  packed phrase tables (TPPT).\n\
  This program is normally called via textpt2tppt.sh.\n\
\n\
";

#define VERIFY_VALUE_ENCODING 0

namespace bio = boost::iostreams;

using namespace std;
using namespace ugdiss;

bio::mapped_file_source src, trg, scores, aln;
vector<uint32_t> srcPhraseBlocks,trgPhraseBlocks;
vector<uint32_t> srcPO;
ofstream idx;
uint32_t const *srcBase,*trgBase,*scoreBase;
uint16_t const *alnBase,*alnEnd;

BitCoder<uint64_t> trgCoder;
vector<BitCoder<uint64_t> > scoreCoder;
BitCoder<uint64_t> countCoder;
BitCoder<uint16_t> alignmentCoder;

size_t numEntries;
size_t trg_size, score_size, aln_size;

uint32_t num_float_scores = 0;
uint32_t num_uint_scores = 0;
uint32_t num_alignment_offset_slots = 0;
uint32_t num_scores = 0;
bool has_alignments = false;

// return a list of codebooks, where each entry has a bool indicating whether
// the code book encodes floats (true) or uint32_t (false), and a list of the
// number of bits in each block for the value ID encoding.
vector<pair<bool, vector<uint32_t> > >
getBitBlocksFromCodeBook(string fname)
{
   assert(sizeof(float) == 4);
   bio::mapped_file_source cbk;
   open_mapped_file_source(cbk, fname);
   uint32_t const* p = reinterpret_cast<uint32_t const*>(cbk.data());
   uint32_t const* cbk_end = reinterpret_cast<uint32_t const*>(cbk.data()+cbk.size());
   if (p > cbk_end-1)
      cerr << efatal << "Empty codebook file '" << fname << "'." << exit_1;
   size_t numBooks = *p++;
   uint32_t code_book_version = 1;
   if (numBooks == 0) {
      using namespace TPPTConfig;
      if (0 == strncmp(reinterpret_cast<const char*>(p), code_book_magic_number_v2,
                       strlen(code_book_magic_number_v2))) {
         cerr << "Code book format v2" << endl;
         code_book_version = 2;
         p += strlen(code_book_magic_number_v2) / 4;
         numBooks = *p++;
      }
   }
   cerr << numBooks << " code books" << endl;
   vector<pair<bool, vector<uint32_t> > > ret(numBooks);
   for (size_t i = 0; i< numBooks; i++)
   {
      if (p > cbk_end-2)
         cerr << efatal << "Corrupt codebook file '" << fname << "'." << exit_1;
      bool is_float = true;
      if (code_book_version == 2) {
         if (0 == strncmp(reinterpret_cast<const char*>(p), "float   ", 8))
            is_float = true;
         else
         if (0 == strncmp(reinterpret_cast<const char*>(p), "uint32_t", 8))
            is_float = false;
         else
            cerr << efatal << "Corrupt codebook v2 file '" << fname << "': invalid data type." << exit_1;
         p += 2;
      }
      ret[i].first = is_float;
      size_t numVals   = *p++;
      size_t numBlocks = *p++;
      cerr << "code book " << i+1 << " has " << numVals
           << " values and " << numBlocks << " blocks: ";
      ret[i].second.resize(numBlocks);
      if (p > cbk_end-numBlocks)
         cerr << efatal << "Corrupt codebook file '" << fname << "'." << exit_1;
      for (size_t k = 0; k < numBlocks; k++)
      {
         ret[i].second[k] = *p++;
         cerr << ret[i].second[k] << " ";
      }
      if (!is_float) cerr << "(type=uint32_t)";
      cerr << endl;
      if (p > cbk_end-numVals)
         cerr << efatal << "Corrupt codebook file '" << fname << "'." << exit_1;
      p += numVals;
   }
   return ret;
}

class mySortFunc
{
   uint32_t const* p;
public:
   mySortFunc(uint32_t const* base) : p(base) {};
   bool operator()(uint32_t a, uint32_t b)
   {
      return p[a] < p[b];
   }
};


size_t 
encodeOneEntry(string& dest, size_t bo, size_t o
#if VERIFY_VALUE_ENCODING
               ,vector<uint32_t>& v // for debugging 
#endif
               )
{
   TPT_DBG(cerr << "encodeOneEntry: bo=" << bo << " o=" << o << endl);
   if (o >= trg_size)
      cerr << efatal << "Invalid index into .trg.col file: " << o << exit_1;
   bo = trgCoder.writeNumber(dest,bo,trgBase[o]);
#if VERIFY_VALUE_ENCODING
   v.push_back(trgBase[o]);
#endif
   if ((o+1)*num_scores-1 >= score_size)
      cerr << efatal << "Invalid index into .scr file: " << (o+1)*num_scores-1
           << exit_1;
   // each float score (3rd and 4th column) is encoded using its own code book
   for (size_t k = 0; k < num_float_scores; k++)
   {
      uint32_t scoreId = scoreBase[o*num_scores+k];
      bo = scoreCoder[k].writeNumber(dest,bo,scoreId);
#if VERIFY_VALUE_ENCODING
      v.push_back(scoreId);
#endif
   }
   // All count fields (c= field) are encoded using one shared code book.
   for (uint32_t j = 0; j < num_uint_scores; ++j) {
      uint32_t countId = scoreBase[o*num_scores+num_float_scores+j];
      bo = countCoder.writeNumber(dest,bo,countId);
   }

   if (has_alignments) {
      size_t alnOffset = *(reinterpret_cast<const size_t*>(&scoreBase[o*num_scores+num_float_scores+num_uint_scores]));
      const uint16_t *alnLink = alnBase + alnOffset;
      assert(alnLink <= alnEnd);
      for ( ; alnLink < alnEnd; ++alnLink) {
         // pack each alignment link into the precalculated number of bits.
         bo = alignmentCoder.writeNumber(dest,bo,*alnLink);
         // We're done when we've hit and written the packed link with value 0,
         // which marks the end.
         if (*alnLink == 0) break;
      }
      assert(*alnLink == 0);
   }
      
   TPT_DBG(cerr << "encodeOneEntry: returning bo=" << bo << endl);
   return bo;
}

size_t 
encodeAllEntries(string& dest, size_t o)
{
   assert(dest.size()==0);
#if VERIFY_VALUE_ENCODING
   vector<uint32_t> v; // for debugging
#endif

   TPT_DBG(cerr << "encodeAllEntries: o=" << o << endl);

   uint32_t curSrcPid = srcBase[srcPO[o]];

#if VERIFY_VALUE_ENCODING
   size_t bo = encodeOneEntry(dest,0,srcPO[o],v);
#else
   size_t bo = encodeOneEntry(dest,0,srcPO[o]);
#endif

   size_t i = o+1;
#if VERIFY_VALUE_ENCODING
   while (i < numEntries && srcBase[srcPO[i]] == curSrcPid)
      bo = encodeOneEntry(dest,bo,srcPO[i++],v);
#else
   while (i < numEntries && srcBase[srcPO[i]] == curSrcPid)
      bo = encodeOneEntry(dest,bo,srcPO[i++]);
#endif

#if VERIFY_VALUE_ENCODING
   cerr << "\n\n\n" << bitpattern(dest) << endl;


   // verify encoding
   size_t z=0;
   pair<char const*,uchar> r(reinterpret_cast<char const*>(dest.c_str()),0);
   for (size_t k = o; k < i; k++)
   {
      uint32_t foo;

#if 0
      cerr << "[" << k << "]t: " << bitpattern(v[z]) << " EXPECTED" << endl;
      cerr << "[" << k << "]t: "
           << bitpattern(r.first[0]) << " "
           << bitpattern(r.first[1]) << " "
           << bitpattern(r.first[2]) << " "
           << "bo=" << int(r.second) << endl;
#endif
      r = trgCoder.readNumber(r,foo);

      cerr << "t[" << k << "]: " << v[z] << " " << foo << endl;
      assert(foo == v[z]);
      z++;
      for (size_t x = 0; x < num_float_scores; x++)
      {
#if 0
         cerr << "s[" << x << "]: " << bitpattern(v[z]) << " EXPECTED" << endl;
         cerr << "s[" << x << "]: "
            << bitpattern(r.first[0]) << " "
            << bitpattern(r.first[1]) << " "
            << bitpattern(r.first[2]) << " "
            << "bo=" << int(r.second) << endl;
#endif          
         r = scoreCoder[x].readNumber(r,foo);
         cerr << "s[" << x << "]: " << v[z] << " " << foo << endl;
         assert(foo == v[z]);
         z++;
      }
   }
#endif
   TPT_DBG(cerr << "encodeAllEntries: returning " << (i-o) << endl);
   return i-o;
}

char const* refIdx_start;
char const* refIdx_end;

pair<filepos_type,uchar>
processNode(ostream& out, filepos_type& curOutPos, char const* const p, uchar flags, uint32_t& o)
{
   id_type flagmask = FLAGMASK;
   id_type flagfilter = ~flagmask;
   char const* q = p;
   char const* idxStop  = p;
   char const* idxStart = p;
   string myValue;
   size_t numPhrases=0;
   TPT_DBG(cerr << "processNode: flags=" << (flags&0xff) << " o=" << o << endl);
   if (q < refIdx_start || q >= refIdx_end)
      cerr << efatal << "Corrupt .src.repos.idx file (invalid offset)." << exit_1;
   if (flags&HAS_CHILD_MASK)
   {
      filepos_type idxStartOffset; 
      q = binread(q,idxStartOffset);
      if (q > refIdx_end)
         cerr << efatal << "Corrupt .src.repos.idx file (invalid offset)." << exit_1;
      idxStart -= idxStartOffset;
      if (idxStart < refIdx_start)
         cerr << efatal << "Corrupt .src.repos.idx file (invalid offset)." << exit_1;
   }
   if (flags&HAS_VALUE_MASK)
   {
      uint32_t srcPid;
      q = binread(q,srcPid);
      if (q > refIdx_end)
         cerr << efatal << "Corrupt .src.repos.idx file (invalid offset)." << exit_1;
      TPT_DBG(cerr << "source id from idx: " << srcPid << endl);
      TPT_DBG(cerr << "top of stack: " << srcBase[srcPO[o]] << endl);
      assert (srcBase[srcPO[o]] >= srcPid);
      if (srcBase[srcPO[o]] == srcPid)
         o += (numPhrases = encodeAllEntries(myValue,o));
   }
   vector<pair<id_type,filepos_type> > I;
   for (q = idxStart; q < idxStop;)
   {
      id_type      id;
      filepos_type offset;
      q = tightread(q,idxStop,id);
      q = tightread(q,idxStop,offset);

      if (id&flagmask)
      {
         pair<filepos_type,uchar> jar 
            = processNode(out, curOutPos, idxStart-offset, id&flagmask, o);
         id = (id&flagfilter)+jar.second;
         I.push_back(pair<id_type,filepos_type>(id,jar.first));
      }
      else 
      {
         TPT_DBG(cerr << "offset=" << offset << " flags=" << int(id&flagmask)
                      << "tos=" << srcBase[srcPO[o]] << endl);
         if (offset == srcBase[srcPO[o]])
         {
            string chldval; // child value
            uint32_t numCP = encodeAllEntries(chldval,o); // num child phrase entries
            o += numCP;
            id_type key = (id&flagfilter)+HAS_VALUE_MASK;
            TPT_DBG(assert((filepos_type)out.tellp() == curOutPos));
            I.push_back(pair<id_type,filepos_type>(key, curOutPos));
            curOutPos += binwrite(out, numCP);
            out.write(chldval.c_str(),chldval.size());
            curOutPos += chldval.size();
         }
      }
   }

   TPT_DBG(assert((filepos_type)out.tellp() == curOutPos));
   filepos_type myIStart = curOutPos;
   for (size_t i = 0; i < I.size(); i++)
   {
      curOutPos += tightwrite(out, I[i].first, false);
      if (I[i].first&flagmask)
         curOutPos += tightwrite(out, myIStart-I[i].second, true);
      else
         curOutPos += tightwrite(out, I[i].second, true);
   }
   TPT_DBG(assert((filepos_type)out.tellp() == curOutPos));
   filepos_type myPos = curOutPos;
   pair<filepos_type,uchar> ret(0,0);
   if (I.size())
   {
      curOutPos += binwrite(out, myPos-myIStart);
      ret.second += HAS_CHILD_MASK;
   }
   if (myValue.size())
   {
      curOutPos += binwrite(out, numPhrases);
      out.write(myValue.c_str(),myValue.size());
      curOutPos += myValue.size();
      ret.second += HAS_VALUE_MASK;
   }
   if (ret.second)
      ret.first = myPos;
   TPT_DBG(cerr << "processNode: returning ret.first=" << ret.first
                << " ret.second=" << (ret.second&0xff) << " o=" << o << endl);
   return ret;
}

/** open a repository file, go backwards in the file till you find the third
 *  byte from the end of file that has the stop bit set , and report the offset
 *  of the position after that
 */

size_t getMaxId(string fname) 
{ 
   bio::mapped_file_source foo;
   open_mapped_file_source(foo, fname);
   char const* p = foo.data() + foo.size();
   --p;
   for (--p; p > foo.data() && *p >= 0; p--);
   for (--p; p > foo.data() && *p >= 0; p--);
   return p > foo.data() ? (++p - foo.data()) : 0;
}

bio::mapped_file_source refIdx;

int MAIN(argc, argv)
{
   cerr << "ptable.assemble starting." << endl;
   if (argc > 1 && !strcmp(argv[1], "-h"))
   {
      cerr << help_message << endl;
      exit(0);
   }
   if (argc < 2)
      cerr << efatal << "OUTPUT_BASE_NAME required." << endl
           << help_message << exit_1;
   if (argc > 2)
      cerr << efatal << "Too many arguments." << endl << help_message << exit_1;

   string bname = argv[1];
   string srcName = bname + ".src.col";
   open_mapped_file_source(src, srcName);
   numEntries = src.size() / sizeof(uint32_t);

   string trgName = bname + ".trg.col";
   open_mapped_file_source(trg, trgName);

   string scrName = bname + ".scr";
   open_mapped_file_source(scores, scrName);

   string configName = bname + ".config";
   uint32_t third_col_count, fourth_col_count, num_counts;
   uint32_t tppt_version =
      TPPTConfig::read(configName, third_col_count, fourth_col_count, num_counts,
                       has_alignments);

   if (has_alignments) {
      assert(tppt_version >= 2);
      string alnName = bname + ".aln";
      open_mapped_file_source(aln, alnName);
   }

   string idxName = bname + ".tppt";
   idx.open(idxName.c_str());
   if (idx.fail())
      cerr << efatal << "Unable to open tppt index file '" << idxName << "' for writing."
           << exit_1;

   srcBase   = reinterpret_cast<uint32_t const*>(src.data());
   trgBase   = reinterpret_cast<uint32_t const*>(trg.data());
   scoreBase = reinterpret_cast<uint32_t const*>(scores.data());
   trg_size  = trg.size() / sizeof(uint32_t);
   score_size = scores.size() / sizeof(uint32_t);
   if (has_alignments) {
      alnBase   = reinterpret_cast<uint16_t const*>(aln.data());
      alnEnd    = reinterpret_cast<uint16_t const*>(aln.data()+aln.size());
      aln_size  = aln.size() / sizeof(uint16_t);
      assert(size_t(alnEnd-alnBase) == aln_size);
   }

   srcPO.resize(numEntries); 
   for (size_t i = 0; i < numEntries; i++)
      srcPO[i] = i;
   sort(srcPO.begin(), srcPO.end(), mySortFunc(srcBase));

   // determine the sequence of bit blocks to encode the phrase / score IDs
   size_t highestTrgPhraseId = getMaxId(bname+".trg.repos.dat");
   cerr << "highestTrgPhraseId: " << highestTrgPhraseId << endl;
   // The calculation of bitsNeededForTrgPhraseIds is incorrect.
   // It is, in general, 1 bit bigger than it needs to be, but fixing
   // this problem here and in TpPhraseTable::openTrgRepos() would render
   // current TPPT files unreadable, because this value is not stored
   // anywhere in the TPPT.
   // The correct calculation is:
   // size_t bitsNeededForTrgPhraseIds = int(ceil(log2(highestTrgPhraseId+1)));
   size_t bitsNeededForTrgPhraseIds = int(ceil(log2(highestTrgPhraseId)))+1;
   cerr << "bits needed for encoding target phrase Ids: "
        << bitsNeededForTrgPhraseIds << endl;
   vector<pair<bool, vector<uint32_t> > > blocks = getBitBlocksFromCodeBook(bname+".cbk");
   trgPhraseBlocks.push_back(bitsNeededForTrgPhraseIds);

   if (tppt_version >= 2) {
      num_float_scores = third_col_count + fourth_col_count;
      if (num_float_scores + (num_counts ? 1 : 0) + (has_alignments ? 1 : 0) != blocks.size())
         cerr << efatal << "config file " << configName << " and code book file " << (bname+".cbk")
              << " have a different number of score columns." << exit_1;
      num_uint_scores = num_counts;
      if (has_alignments)
         num_alignment_offset_slots = sizeof(size_t)/sizeof(uint32_t);
      else
         num_alignment_offset_slots = 0;
   } else {
      num_float_scores = blocks.size();
      num_uint_scores = num_alignment_offset_slots = 0;
   }
   num_scores = num_float_scores + num_uint_scores + num_alignment_offset_slots;

   trgCoder.setBlocks(trgPhraseBlocks);
   scoreCoder.resize(blocks.size());
   for (size_t i = 0; i < num_float_scores; i++)
      scoreCoder[i].setBlocks(blocks[i].second);
   if (num_counts > 0)
      countCoder.setBlocks(blocks[num_float_scores].second);
   if (has_alignments)
      alignmentCoder.setBlocks(blocks.back().second);

   string refIdxName = bname + ".src.repos.idx";
   open_mapped_file_source(refIdx, refIdxName);
   refIdx_start = refIdx.data();                 // for sanity checking
   refIdx_end = refIdx_start + refIdx.size();    // for sanity checking
   filepos_type refStartIdx;
   id_type refNumTokens;
   char const* r = refIdx.data();
   r = numread(r,refStartIdx);
   r = numread(r,refNumTokens);

   numwrite(idx,filepos_type(0)); // reserve room for start of index 
   numwrite(idx,refNumTokens);    // number of tokens
   filepos_type curIdxPos = sizeof(filepos_type) + sizeof(id_type);
   curIdxPos += binwrite(idx, 0U); // root value
   curIdxPos += binwrite(idx, 0U); // default value

   uint32_t o = 0;
   vector<pair<filepos_type,uchar> > I(refNumTokens,pair<filepos_type,uchar>(0,0));

   cerr << "ptable.assemble processing phrase table nodes." << endl;

   r = refIdx.data()+refStartIdx;
   if (r + (sizeof(filepos_type)+1)*(filepos_type)refNumTokens > refIdx_end)
      cerr << efatal << "Corrupt index file '" << refIdxName << "'." << exit_1;
   for (size_t i = 0; i < refNumTokens; i++)
   {
      TPT_DBG(cerr << "ptable.assemble processing i = " << i << endl);
      filepos_type offset; uchar flags;
      r = numread(r,offset);
      flags = *r++;
      TPT_DBG(cerr << "main: i=" << i << " flags=" << (unsigned)flags
                   << " offset=" << offset << " o=" << o << endl);
      if (flags)
         I[i] = processNode(idx, curIdxPos, refIdx.data()+offset, flags, o);
      else if (o < numEntries && offset == srcBase[srcPO[o]])
      {
         string chldval; // child value
         uint32_t numCP = encodeAllEntries(chldval,o); // num child phrase entries
         TPT_DBG(assert((filepos_type)idx.tellp() == curIdxPos));
         I[i] = pair<filepos_type,uchar>(curIdxPos, HAS_VALUE_MASK);
         curIdxPos += binwrite(idx, numCP);
         idx.write(chldval.c_str(),chldval.size());
         curIdxPos += chldval.size();
         o += numCP;
      }
      TPT_DBG(cerr << "main: now o=" << o << endl);
   }

   cerr << "ptable.assemble writing index." << endl;
   TPT_DBG(assert((filepos_type)idx.tellp() == curIdxPos));
   filepos_type idxStart = curIdxPos;
   for (size_t i = 0; i < I.size(); i++)
   {
      numwrite(idx,I[i].first);
      idx.put(I[i].second);
   }
   idx.seekp(0);
   numwrite(idx,idxStart);
   idx.close();
   cerr << "ptable.assemble finished." << endl;
}
END_MAIN
