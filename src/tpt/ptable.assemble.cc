// (c) 2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.
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

#include "tpt_typedefs.h"
#include "tpt_tokenindex.h"
#include "ugMemTable.h"
#include "tpt_bitcoder.h"
#include "tpt_utils.h"

static char help_message[] = "\n\
ptable.assemble [options] OUTPUT_BASE_NAME\n\
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

#define rcast reinterpret_cast

#define VERIFY_VALUE_ENCODING 0

namespace bio = boost::iostreams;

using namespace std;
using namespace ugdiss;

bio::mapped_file_source src, trg, /*aln,*/ scores;
vector<uint32_t> srcPhraseBlocks,trgPhraseBlocks;
vector<uint32_t> srcPO;
ofstream idx;
uint32_t const *srcBase,*trgBase,*scoreBase;

BitCoder<uint64_t> trgCoder;
vector<BitCoder<uint64_t> > scoreCoder;

size_t numEntries;
size_t trg_size, score_size;

vector<vector<uint32_t> >
getBitBlocksFromCodeBook(string fname)
{
  assert(sizeof(float) == 4);
  bio::mapped_file_source cbk;
  open_mapped_file_source(cbk, fname);
  uint32_t const* p = rcast<uint32_t const*>(cbk.data());
  uint32_t const* cbk_end = rcast<uint32_t const*>(cbk.data()+cbk.size());
  if (p > cbk_end-1)
    cerr << efatal << "Empty codebook file '" << fname << "'." << exit_1;
  size_t numBooks = *p++;
  cerr << numBooks << " code books" << endl;
  vector<vector<uint32_t> > ret(numBooks);
  for (size_t i = 0; i< numBooks; i++)
    {
      if (p > cbk_end-2)
        cerr << efatal << "Corrupt codebook file '" << fname << "'." << exit_1;
      size_t numVals   = *p++;
      size_t numBlocks = *p++;
      cerr << "code book " << i+1 << " has " << numVals
           << " values and " << numBlocks << " blocks: ";
      ret[i].resize(numBlocks);
      if (p > cbk_end-numBlocks)
        cerr << efatal << "Corrupt codebook file '" << fname << "'." << exit_1;
      for (size_t k = 0; k < numBlocks; k++)
        {
          ret[i][k] = *p++;
          cerr << ret[i][k] << " ";
        }
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
  if (o >= trg_size)
    cerr << efatal << "Invalid index into .trg.col file: " << o << exit_1;
  bo = trgCoder.writeNumber(dest,bo,trgBase[o]);
#if VERIFY_VALUE_ENCODING
  v.push_back(trgBase[o]);
#endif
  if ((o+1)*scoreCoder.size()-1 >= score_size)
    cerr << efatal << "Invalid index into .scr file: " << (o+1)*scoreCoder.size()-1
         << exit_1;
  for (size_t k = 0; k < scoreCoder.size(); k++)
    {
      uint32_t scoreId = scoreBase[o*scoreCoder.size()+k];
      bo = scoreCoder[k].writeNumber(dest,bo,scoreId);
#if VERIFY_VALUE_ENCODING
      v.push_back(scoreId);
#endif
    }
  return bo;
}

size_t 
encodeAllEntries(string& dest, size_t o)
{
  assert(dest.size()==0);
#if VERIFY_VALUE_ENCODING
  vector<uint32_t> v; // for debugging
#endif

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
  cout << "\n\n\n" << bitpattern(dest) << endl;
  

  // verify encoding
  size_t z=0;
  pair<char const*,uchar> r(rcast<char const*>(dest.c_str()),0);
  for (size_t k = o; k < i; k++)
    {
      uint32_t foo;

#if 0
      cout << "[" << k << "]t: " << bitpattern(v[z]) << " EXPECTED" << endl;
      cout << "[" << k << "]t: " 
           << bitpattern(r.first[0]) << " "
           << bitpattern(r.first[1]) << " "
           << bitpattern(r.first[2]) << " "
           << "bo=" << int(r.second) << endl;
#endif
      r = trgCoder.readNumber(r,foo);

      cout << "t[" << k << "]: " << v[z] << " " << foo << endl;
      assert(foo == v[z++]);
      for (size_t x = 0; x < scoreCoder.size(); x++)
        {
#if 0
          cout << "s[" << x << "]: " << bitpattern(v[z]) << " EXPECTED" << endl;
          cout << "s[" << x << "]: " 
               << bitpattern(r.first[0]) << " "
               << bitpattern(r.first[1]) << " "
               << bitpattern(r.first[2]) << " "
               << "bo=" << int(r.second) << endl;
#endif          
          r = scoreCoder[x].readNumber(r,foo);
          cout << "s[" << x << "]: " << v[z] << " " << foo << endl;
          assert(foo == v[z++]);
        }
    }
#endif
  return i-o;
}

char const* refIdx_start;
char const* refIdx_end;

pair<filepos_type,uchar>
processNode(ostream& out, char const* const p, uchar flags, uint32_t& o)
{
  id_type flagmask = FLAGMASK;
  id_type flagfilter = ~flagmask;
  char const* q = p;
  char const* idxStop  = p;
  char const* idxStart = p;
  string myValue;
  size_t numPhrases=0;
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
      // cout << "source id from idx: " << srcPid << endl;
      // cout << "top of stack: " << srcBase[srcPO[o]] << endl;
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
            = processNode(out,idxStart-offset,id&flagmask,o);
          id = (id&flagfilter)+jar.second;
          I.push_back(pair<id_type,filepos_type>(id,jar.first));
        }
      else 
        {
#if 0
          cout << "offset=" << offset << " flags=" << int(id&flagmask) 
               << "tos=" << srcBase[srcPO[o]] 
               << endl;
#endif
          if (offset == srcBase[srcPO[o]])
            {
              string chldval; // child value
              uint32_t numCP = encodeAllEntries(chldval,o); // num child phrase entries
              o += numCP;
              id_type key = (id&flagfilter)+HAS_VALUE_MASK;
              I.push_back(pair<id_type,filepos_type>(key,out.tellp()));
              binwrite(out,numCP);
              out.write(chldval.c_str(),chldval.size());
            }
        }
    }

  filepos_type myIStart = out.tellp();
  for (size_t i = 0; i < I.size(); i++)
    {
      tightwrite(out,I[i].first,false);
      if (I[i].first&flagmask)
        tightwrite(out,myIStart-I[i].second,true);
      else
        tightwrite(out,I[i].second,true);
    }
  filepos_type myPos = out.tellp();
  pair<filepos_type,uchar> ret(0,0);
  if (I.size())
    {
      binwrite(out,myPos-myIStart);
      ret.second += HAS_CHILD_MASK;
    }
  if (myValue.size())
    {
      binwrite(out,numPhrases);
      out.write(myValue.c_str(),myValue.size());
      ret.second += HAS_VALUE_MASK;
    }
  if (ret.second)
    ret.first = myPos;
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
  numEntries = src.size()/4;

  string trgName = bname + ".trg.col";
  open_mapped_file_source(trg, trgName);

  // aln.open(bname+"aln.col");

  string scrName = bname + ".scr";
  open_mapped_file_source(scores, scrName);

  string idxName = bname + ".tppt";
  idx.open(idxName.c_str());
  if (idx.fail())
    cerr << efatal << "Unable to open tppt index file '" << idxName << "' for writing."
         << exit_1;

  srcBase   = rcast<uint32_t const*>(src.data());
  trgBase   = rcast<uint32_t const*>(trg.data());
  scoreBase = rcast<uint32_t const*>(scores.data());
  trg_size  = trg.size() / sizeof(uint32_t);
  score_size = scores.size() / sizeof(uint32_t);

  srcPO.resize(numEntries); 
  for (size_t i = 0; i < numEntries; i++) 
    srcPO[i] = i;
  sort(srcPO.begin(), srcPO.end(), mySortFunc(srcBase));

  // determine the sequence of bit blocks to encode the phrase / score IDs
  size_t highestTrgPhraseId = getMaxId(bname+".trg.repos.dat");
  size_t bitsNeededForTrgPhraseIds = int(ceil(log2(highestTrgPhraseId)))+1;
  cerr << "bits needed for encoding target phrase Ids: "
       << bitsNeededForTrgPhraseIds << endl;
  vector<vector<uint32_t> > blocks = getBitBlocksFromCodeBook(bname+".cbk");
  trgPhraseBlocks.push_back(bitsNeededForTrgPhraseIds);
  
  trgCoder.setBlocks(trgPhraseBlocks);
  scoreCoder.resize(blocks.size());
  for (size_t i = 0; i < blocks.size(); i++)
    scoreCoder[i].setBlocks(blocks[i]);
  
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
  binwrite(idx,0U); // root value 
  binwrite(idx,0U); // default value
  
  uint32_t o = 0;
  vector<pair<filepos_type,uchar> > I(refNumTokens,pair<filepos_type,uchar>(0,0));

  r = refIdx.data()+refStartIdx;
  if (r + (sizeof(filepos_type)+1)*(filepos_type)refNumTokens > refIdx_end)
    cerr << efatal << "Corrupt index file '" << refIdxName << "'." << exit_1;
  for (size_t i = 0; i < refNumTokens; i++)
    {
      filepos_type offset; uchar flags;
      r = numread(r,offset);
      flags = *r++;
      if (flags)
        I[i] = processNode(idx,refIdx.data()+offset,flags,o);
      else if (offset == srcBase[srcPO[o]])
        {
          string chldval; // child value
          uint32_t numCP = encodeAllEntries(chldval,o); // num child phrase entries
          I[i] = pair<filepos_type,uchar>(idx.tellp(),HAS_VALUE_MASK);
          binwrite(idx,numCP);
          idx.write(chldval.c_str(),chldval.size());
          o += numCP;
        }
    }
  filepos_type idxStart = idx.tellp();
  idx.seekp(0);
  numwrite(idx,idxStart);
  idx.seekp(idxStart);
  for (size_t i = 0; i < I.size(); i++)
    {
      numwrite(idx,I[i].first);
      idx.put(I[i].second);
    }
  idx.close();
}
END_MAIN
