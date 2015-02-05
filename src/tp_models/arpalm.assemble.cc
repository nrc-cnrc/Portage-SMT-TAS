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
 * @file arpalm.assemble.cc 
 * @brief Assemble the final TPLM from intermediate files produced by arpalm.sng-av.
 *
 * Final step in arpalm2tplm conversion. Normally called via arpalm2tplm.sh.
 */
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <boost/iostreams/device/mapped_file.hpp>
#include <vector>
#include <glob.h>

//#define DEBUG_TPT
//#define DEBUG_TPT_2

#include "tpt_typedefs.h"
#include "tpt_tightindex.h"
#include "tpt_tokenindex.h"
#include "tpt_pickler.h"
#include "tpt_utils.h"

static char help_message[] = "\n\
arpalm.assemble [-h] LM_ORDER OUTPUT_BASE_NAME\n\
\n\
  Assemble the encoded LM (TPLM) from the sorted ngram files.\n\
\n\
  LM_ORDER specifies the order of the language model (2 <= LM_ORDER <= 100).\n\
  The TokenIndex file OUTPUT_BASE_NAME.tdx is read to determine the number of\n\
  tokens. The TPLM file OUTPUT_BASE_NAME.tplm is created from the intermediate\n\
  value files (all Ngrams.vals.DDDD.dat and Ngrams.vals.DDDD.idx pairs).\n\
\n\
  This is the final step in arpalm2tplm conversion.\n\
  This program is normally via arpalm2tplm.sh.\n\
";

using namespace std;
using namespace ugdiss;
TokenIndex* tidx=NULL;

id_type numTokens;

class ngram
{
public:
  size_t   order;
  id_type const* id;
  ngram() : order(0), id(NULL) {};
  ngram(char const* _p, size_t _o)
    : order(_o), id(reinterpret_cast<id_type const*>(_p))
  {}
  
  bool operator==(ngram const& other)
  {
    if (this->id == NULL || other.id == NULL)
      return false;
    if (this->order != other.order) 
      return false;
    for (size_t i = 0; i < order; i++)
      if (this->id[i] != other.id[i]) 
	return false;
    return true;
  }

  bool operator<(ngram const& other)
  {
    if (this->id == NULL || other.id == NULL)
      return false;
    assert(other.order <= this->order);
    size_t i = 0;
    while (i < other.order && this->id[i] == other.id[i])
      i++;
    if (i < other.order)
      return this->id[i] < other.id[i];
    else
      return false;
  }

  bool 
  match(ngram const& other)
  {
    if (this->id == NULL || other.id == NULL) 
      return false;
    assert(other.order <= this->order);
    for (size_t i = 0; i < other.order; i++)
      if (this->id[i] != other.id[i]) return false;
    return true;
  }

  id_type 
  val() const
  {
    return *(id+order);
  }
  
};

ostream& 
operator<<(ostream& out, ngram const& ng)
{
  if (tidx)
    {
      for (size_t i = 0; i < ng.order; i++)
	out << (i == 0 ? "" : " ") << (*tidx)[ng.id[i]];
    }
  else
    {
      for (size_t i = 0; i < ng.order; i++)
	out << (i == 0 ? "" : " ") << ng.id[i];
    }
  out << " => " << int(ng.val());
  return out;
}


class ngramStreamer
{
public:
  size_t order;
  size_t valueSize; // how many bytes does the record at the end of the ngram occupy?
  bio::mapped_file_source idx;
  bio::mapped_file_source dat;
  vector<string> fileNames; // list of filenames, in order
  ngram top;
  size_t filectr;

  bool // returns true on success 
  open_next_file()
  {
    if (idx.is_open()) idx.close();
    if (dat.is_open()) dat.close();
    if (filectr == fileNames.size())
      top.id = NULL;
    else
      {
        string datName = fileNames[filectr++];
        string idxName = fileNames[filectr++];
        if (datName.substr(datName.size()-4) != ".dat")
          cerr << efatal << "Expected '.dat' suffix on file '" << datName << "'."
               << exit_1;
        if (idxName.substr(idxName.size()-4) != ".idx")
          cerr << efatal << "Expected '.idx' suffix on file '" << idxName << "'."
               << exit_1;
        if (datName.substr(0, datName.size()-4) != idxName.substr(0, idxName.size()-4))
          cerr << efatal << "Basename mismatch between files '"
               << datName << "' and '" << idxName << "'."
               << exit_1;
        cerr << idxName << endl;
        open_mapped_file_source(idx, idxName);
        open_mapped_file_source(dat, datName);
        top.id = reinterpret_cast<id_type const*>(idx.data()+1);
      }
    return dat.is_open() && idx.is_open();
  }

  ngramStreamer(size_t _order, size_t _valSize, string pattern)
    : order(_order), 
      valueSize(_valSize),
      filectr(0)
  {
    top.order = _order;
    
    ostringstream patbuf;
    patbuf << order << pattern;
    glob_t gl_buf;
    glob(patbuf.str().c_str(),0,NULL,&gl_buf);
    // fileNames.resize(gl_buf.gl_pathc);
    for (size_t i = 0; i < gl_buf.gl_pathc; i+=2)
      {
        if (!access(gl_buf.gl_pathv[i],R_OK) 
            && getFileSize(gl_buf.gl_pathv[i]))
          {
            fileNames.push_back(gl_buf.gl_pathv[i]);
            fileNames.push_back(gl_buf.gl_pathv[i+1]);
          }
      }
            
    globfree(&gl_buf);
    open_next_file();
  };

  void  
  pop()
  {
    char const* foo = reinterpret_cast<char const*>(top.id);
    foo += order*sizeof(id_type)+valueSize;
    if (foo < idx.data()+idx.size())
      top.id = reinterpret_cast<id_type const*>(foo);
    else if (!open_next_file())
      top.id = 0;
  }

  char const*
  topValStart()
  {
    assert (top.id != NULL);
    return dat.data()+*reinterpret_cast<filepos_type const*>(top.id+order);
  }

#if 0
  size_t
  topValSize()
  {
    assert (top.id != NULL);
    if (reinterpret_cast<char const*>(top.id+2*order)-idx.data() <= idx.size() -valSize)
      return (*(reinterpret_cast<uint64_t const*>(top.id+2*order)+1)
              -;
  }
#endif

};

pair<filepos_type,uchar>
writeNode(ostream& out, vector<ngramStreamer*>& B, size_t i, filepos_type& outPos)
{
  TPT_DBG2(cerr << "writing " << B[i]->top << endl);
  // process children, if any
  typedef pair<id_type,filepos_type> idx_entry_t;
  vector<idx_entry_t> tmpidx;
  if (i+1 < B.size())
    {
      while (B[i+1]->top < B[i]->top)
	{
	  cerr << ewarn << "Ignoring back-off weight for "
               << i+2 << "-gram " << B[i+1]->top << endl;
	  B[i+1]->pop();
	}
      while (B[i+1]->top.match(B[i]->top))
        {
          id_type wid = B[i+1]->top.id[i+1];
          if (!tmpidx.empty()) assert((wid<<FLAGBITS) > tmpidx.back().first);
          pair<filepos_type,uchar> jar = writeNode(out, B, i+1, outPos); 
          // writeNode also pops B[i+1]
          tmpidx.push_back(idx_entry_t((wid<<FLAGBITS)+jar.second, jar.first));
        }
    }

  // write the index
  TPT_DBG(assert(outPos == (filepos_type)out.tellp()));
  filepos_type idxStart = outPos;
  typedef vector<idx_entry_t>::iterator idx_iter_t;
  for (idx_iter_t it = tmpidx.begin(); it != tmpidx.end(); it++)
    {
      outPos += tightwrite(out, it->first, false);
      if (it->first&FLAGMASK)
        outPos += tightwrite(out, idxStart - it->second, true);
      else
        outPos += tightwrite(out, it->second, true);
    }
  TPT_DBG(assert(outPos >= 0); assert(outPos == (filepos_type)out.tellp()));
  filepos_type myPosition = outPos;
  uchar flags = tmpidx.empty() ? 0: HAS_CHILD_MASK;
  if (flags)
    outPos += binwrite(out, myPosition - idxStart);

  // get node value computed earlier by assemble.sng-av:
  id_type bowId,pvalEncSize;
  char const* p = B[i]->topValStart();
  p = binread(p,bowId);
  p = binread(p,pvalEncSize);

  if (flags || pvalEncSize)
    {
      outPos += binwrite(out, bowId);
      outPos += binwrite(out, pvalEncSize);
      if (pvalEncSize)
        {
          out.write(p, pvalEncSize);
          outPos += pvalEncSize;
        }
      flags += HAS_VALUE_MASK;
    }
  else
    myPosition = bowId;

  B[i]->pop();
  return pair<filepos_type,uchar>(myPosition,flags);
}


uint32_t
getNumTokens(char const* fname)
{
  ifstream in(fname);
  if (in.fail())
     cerr << efatal << "Unable to open '" << fname << "' for reading." << exit_1;
  char buf[4];
  in.read(buf,4);
  in.close();
  return *reinterpret_cast<uint32_t*>(buf);
}

int MAIN(argc, argv)
{
  cerr << "starting arpalm.assemble" << endl;
  if (argc > 1 && !strcmp(argv[1], "-h"))
    {
      cerr << help_message << endl;
      exit(0);
    }
  if (argc < 3)
    cerr << efatal << "LM_ORDER and OUTPUT_BASE_NAME required." << endl
         << help_message << exit_1;
  if (argc > 3)
    cerr << efatal << "Too many arguments." << endl << help_message << exit_1;

  vector<ngramStreamer*> B;

  size_t order  = atoi(argv[1]);
  if (order < 2 || order > 100)
    cerr << efatal << "LM_ORDER == " << order << " is not reasonable." << endl
         << "LM_ORDER must be an integer in the range 2-100." << endl
         << help_message << exit_1;
  string datFileName = string(argv[2])+".tplm";
  string tdxFileName = string(argv[2])+".tdx";
  tidx = new TokenIndex(tdxFileName, "<unk>"); // To print legible warnings; not used otherwise
  ofstream out(datFileName.c_str());
  if (out.fail())
    cerr << efatal << "Unable to open '" << datFileName << "' for writing."
         << exit_1;
  numTokens = getNumTokens(tdxFileName.c_str());
  TPT_DBG(cerr << numTokens << " tokens" << endl);
  for (size_t i = 1; i < order; i++)
    B.push_back(new ngramStreamer(i, sizeof(filepos_type), "grams.vals.*"));

  vector<filepos_type> offset(numTokens,0);
  vector<uchar>         flags(numTokens,0);
  numwrite(out,filepos_type(0)); // reserve room for start index pos.
  numwrite(out,numTokens);       // reserve room for index size
  filepos_type outPos = sizeof(filepos_type) + sizeof(id_type);
  outPos += binwrite(out,0U); outPos += binwrite(out,0U); // root value
  outPos += binwrite(out,0U); outPos += binwrite(out,0U); // default value
  assert(B.size());
  while (B[0]->top.id != NULL)
    {
      id_type wid = B[0]->top.id[0];
      if (wid >= offset.size())
	cerr << efatal << "Encountered wid > numTokens." << endl
	     << "Token index and ngram-files must be out of sync."
	     << exit_1;
      pair<filepos_type,uchar> jar = writeNode(out, B, 0, outPos);
      offset[wid] = jar.first;
      flags[wid] = jar.second;
    }
  TPT_DBG(assert(outPos == (filepos_type)out.tellp()));
  filepos_type sIdx = outPos;
  for (size_t i = 0; i < offset.size(); i++)
    {
      numwrite(out,offset[i]);
      out.put(flags[i]);
    }
  out.seekp(0);
  numwrite(out,sIdx);
  cerr << "done arpalm.assemble" << endl;
}
END_MAIN
