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
 * @file arpalm.sng-av.cc 
 * @brief Sorts ngram files produced by arpalm.encode and encodes information
 * for each node in the trie.
 *
 * Second step in arpalm2tplm conversion. Normally called from arpalm2tplm.sh
 * via the sng-av.jobs script produced by in step one by arpalm.encode.
 */
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <iomanip>
#include <stdint.h>
#include "tpt_tightindex.h"
#include "tpt_pickler.h"
#include "tpt_utils.h"

#ifndef rcast
#define rcast reinterpret_cast
#endif

using namespace std;
using namespace ugdiss;
namespace bio=boost::iostreams;

static char help_message[] = "\n\
arpalm.sng-av [-h] BO_FILE\n\
\n\
  Sort the IRST n-grams.\n\
\n\
  BO_FILE specifies the file holding back-off weight ids for (n-1) grams.\n\
  A second file, P_FILE, holds the probability value ids for n-grams.\n\
  A third file named bowzero provides the IDs of zero back-off weights.\n\
\n\
  BO_FILE must be a name in the format: <N-1>grams.bo.<DDDD>[.sorted]\n\
  with <N-1> being an integer = n-1 and <DDDD> being a 4-digit number.\n\
  The corresponding P_FILE must have the file name: <N>grams.p.<DDDD>[.sorted]\n\
  with <N> being an integer = n and the same <DDDD>.\n\
\n\
  Two output files are produced:\n\
    <N-1>grams.vals.<DDDD>.dat and <N-1>grams.vals.<DDDD>.idx\n\
\n\
  Still rudimentary: All three input files are expected to be in the CWD,\n\
  and must obey the naming conventions stated above.\n\
\n\
  This is the second step in arpalm2tplm conversion.\n\
  This program is normally called from arpalm2tplm.sh via the sng-av.jobs\n\
  script produced by in step one by arpalm.encode.\n\
";

unsigned int bo_order,pv_order;

class bo_ngram
{
public:
  uint32_t const* p; // pointer to first id
  bo_ngram(char const* _p) 
    : p(reinterpret_cast<uint32_t const*>(_p))
  {};

  bo_ngram(uint32_t const* _p) 
    : p(_p)
  {};

  bool operator<(bo_ngram const& other) const
  {
    size_t i = 0;
    while (i < bo_order && (*(this->p+i) == *(other.p+i))) i++;
    return i < bo_order && *(this->p+i) < *(other.p+i);
  };
};

class pv_ngram
{
public:
  uint32_t const* p; // pointer to first id
  pv_ngram(char const* _p) 
    : p(reinterpret_cast<uint32_t const*>(_p))
  {};

  bool operator<(pv_ngram const& other) const
  {
    size_t i = 0;
    while (i < pv_order && (*(this->p+i) == *(other.p+i))) i++;
    return i < pv_order && *(this->p+i) < *(other.p+i);
  };

  bool operator<(bo_ngram const& other) const
  {
    size_t i = 0;
    while (i < bo_order && (*(this->p+i) == *(other.p+i))) i++;
    return i < bo_order && *(this->p+i) < *(other.p+i);
  };

  bool match(bo_ngram const& other) const
  {
    size_t i = 0;
    while (i < bo_order && (*(this->p+i)==*(other.p+i))) i++;
    return i==bo_order;
  }
};

ostream &
operator<<(ostream& out, bo_ngram const& ng)
{
  for (size_t i = 0; i < bo_order; i++)
    out << ng.p[i] << " ";
  out << " => " << ng.p[bo_order];
  return out;
}

ostream &
operator<<(ostream& out, pv_ngram const& ng)
{
  for (size_t i = 0; i < pv_order; i++)
    out << ng.p[i] << " ";
  out << " => " << ng.p[pv_order];
  return out;
}

void 
assemble_value(vector<pv_ngram, boost::pool_allocator<pv_ngram> > const& pv,
               size_t& pi, bo_ngram bong, ostream& datFile, ostream& idxFile)
{
  // cout << "assembling value for " << bong << endl;

  map<uint32_t,uint32_t> M;
  for (;pi < pv.size() && pv[pi].match(bong); pi++)
    M[*rcast<uint32_t const*>(pv[pi].p+bo_order)] 
      = *rcast<uint32_t const*>(pv[pi].p+pv_order);
  ostringstream buf;
  for (map<uint32_t,uint32_t>::iterator m = M.begin(); m != M.end(); m++)
    {
      tightwrite(buf,m->first,false);
      tightwrite(buf,m->second,true);
    }
  uint64_t fpos = datFile.tellp();
  binwrite(datFile,*(rcast<uint32_t const*>(bong.p+bo_order)));
  binwrite(datFile,buf.str().size());
  if (buf.str().size())
    datFile.write(buf.str().c_str(),buf.str().size());
  for (size_t i = 0; i < bo_order; i++)
    idxFile.write(rcast<char const*>(bong.p+i),4);
  idxFile.write(rcast<char const*>(&fpos),8);
}


/** input two files: back-off weight ids for (n-1) grams and probability value ids for n-grams
 *  Still rudimentary:
 *  - expect first and only argument to be file in the cwd following the pattern 
 *    [0-9]-grams.bo.[0-9][0-9][0-9][0-9](.sorted)
 */

int MAIN(argc, argv)
{
  if (argc < 2)
    cerr << efatal << "BO_FILE (<N-1>grams.bo.<DDDD>[.sorted]) required." << endl
         << help_message << exit_1;
  if (!strcmp(argv[1], "-h"))
    {
      cerr << help_message << endl;
      exit(0);
    }
  if (argc > 2)
    cerr << efatal << "Too many arguments." << endl << help_message << exit_1;

  vector<bo_ngram, boost::pool_allocator<bo_ngram> > bo;
  vector<pv_ngram, boost::pool_allocator<pv_ngram> > pv;

  string boFile = argv[1];

  // make sure the boFile exists and is readable
  if (access(boFile.c_str(),R_OK))
    cerr << efatal << "File '" << boFile << "' does not exist or is not readable!"
         << exit_1;

  // make sure the file 'bowzero' exists and is readable
  if (access("bowzero",R_OK))
    cerr << efatal << "File 'bowzero' does not exist or is not readable!"
         << exit_1;

  bio::mapped_file_source bo_in;
  open_mapped_file_source(bo_in, boFile);

  unsigned int chunkId;
  if (sscanf(argv[1], "%ugrams.bo.%u", &bo_order, &chunkId) != 2)
     cerr << efatal << "Input file name '" << argv[1]
          << "' not in required format: <N-1>grams.bo.<DDDD>[.sorted]"
          << exit_1;
  if (bo_order < 1 || bo_order > 100)
    cerr << efatal << "N-gram order == " << bo_order << " of file '" << boFile
         << "' is not reasonable." << endl
         << "N-gram order must be an integer in the range 1-100." << endl
         << exit_1;
  size_t bo_recSize=4*(bo_order+1);
  char const* bo_end =  bo_in.data()+bo_in.size();
 
  cerr << "Sorting " << boFile << endl;
  for (const char* x = bo_in.data()+1; x < bo_end; x += bo_recSize)
    bo.push_back(bo_ngram(x));
  sort(bo.begin(),bo.end());

  // get the id of the back-off weight zero
  ifstream bzero("bowzero");
  if (bzero.fail())
    cerr << efatal << "Unable to open file 'bowzero' for reading." << exit_1;
  uint32_t zeroId;
  for (size_t i = 0; i < bo_order; i++)
    bzero>>zeroId;
  if (bzero.fail())
    cerr << efatal << "Unable to read the id of back-off weight zero for order "
         << bo_order << " from file 'bowzero'."
         << exit_1;
  bzero.close();

  // encode probability values, if the p-file exists
  pv_order = bo_order+1;
  ostringstream buf;
  buf.fill('0');
  buf << pv_order << "grams.p." << setw(4) << chunkId;
  string pvFile = buf.str();
  
  bool havePFile = !access(pvFile.c_str(),R_OK);
  bio::mapped_file_source pv_in;
  if (havePFile)
    {
      open_mapped_file_source(pv_in, pvFile);
      size_t pv_recSize=4*(pv_order+1);
      cerr << "Sorting " << pvFile << endl;
      char const* pv_end =  pv_in.data()+pv_in.size();
      for (const char* x = pv_in.data()+1; x < pv_end; x += pv_recSize)
        pv.push_back(pv_ngram(x));
      sort(pv.begin(),pv.end());
    }
  else
    {
      cerr << ewarn << "File '" << pvFile << "' does not exist or is not readable."
           << endl;
      // cerr << "If the language model is very small, this may be OK, but this "
      // << "should not happen with any reasonably sized language model."
      // << endl;
    }
  ostringstream buf2; buf2.fill('0');
  buf2 << bo_order << "grams.vals." << setw(4) << chunkId;
  string datFileName = buf2.str()+".dat";
  string idxFileName = buf2.str()+".idx";
  ofstream datFile(datFileName.c_str());
  if (datFile.fail())
    cerr << efatal << "Unable to open file '" << datFileName << "' for writing."
         << exit_1;
  ofstream idxFile(idxFileName.c_str());
  if (idxFile.fail())
    cerr << efatal << "Unable to open file '" << idxFileName << "' for writing."
         << exit_1;

  cerr << "Writing " << datFileName << " and " << idxFileName << endl;

  idxFile.put(char(bo_order));
  size_t pi = 0;
  for (size_t bi = 0; bi < bo.size(); bi++)
    {
      while (pi < pv.size() && pv[pi] < bo[bi])
        {
          // missing back-off weight
          uint32_t foo[pv_order];
          for (size_t i = 0 ; i < bo_order; i++)
            foo[i] = pv[pi].p[i];
          foo[bo_order] = zeroId;
          assemble_value(pv,pi,bo_ngram(foo),datFile,idxFile);
        }
      if (!pv.size() || (pi < pv.size() && !(pv[pi] < bo[bi])))
        {
          assemble_value(pv,pi,bo[bi],datFile,idxFile);
        }
    }
  if (pi != pv.size())
    cerr << efatal << "Extra probability values found." << endl
         << "Mismatch between files '" << boFile << "' and '" << pvFile << "'."
         << exit_1;
}
END_MAIN
