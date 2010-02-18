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
 * @file ptable.encode-scores.cc
 * @brief Preparatory step in converting phrase tables to TPPT:
 * encodes phrase scores.
 *
 * Second step in textpt2tppt conversion. Normally called via textpt2tppt.sh.
 */

// YET TO BE DONE: 
// - allow truncation of scores to get smaller, coarser code books
// - proper help message, command line interface, etc

#include <map>
#include <cmath>
#include <sstream>

#if IN_PORTAGE
#include "file_utils.h"
using namespace Portage;
#else
#include <boost/iostreams/stream.hpp>
#include "ugStream.h"
#endif

#include <boost/program_options.hpp>

#include "tpt_typedefs.h"
#include "tpt_tokenindex.h"
#include "tpt_encodingschemes.h"

static char base_help_message[] = "\n\
ptable.encode-scores [options] [-i] TEXTPT_FILE [-o] OUTPUT_BASE_NAME\n\
\n\
  Encode the phrase scores for a text phrase table file TEXTPT_FILE.\n\
\n\
  The following files are produced:\n\
  OUTPUT_BASE_NAME.scr is an intermediate file containing a score id for each\n\
  score in the phrase table.\n\
  OUTPUT_BASE_NAME.cbk is the codebook file for decoding the scores.\n\
\n\
  This is the second step in the conversion of text phrase tables to tightly\n\
  packed phrase tables (TPPT).\n\
  This program is normally called via textpt2tppt.sh.\n\
\n\
";

static stringstream options_help;       // set by interpret_args()

inline ostream& help_message(ostream& os)
{
  return os << base_help_message << options_help.str();
}


#define rcast reinterpret_cast

namespace po  = boost::program_options;
namespace bio = boost::iostreams;

using namespace std;
using namespace ugdiss;


const char * const COLSEP = " ||| ";

string iFileName;
string oBaseName;
//string truncation;
// vector<size_t> truncactionBits; // add truncation later (maybe)


size_t 
countScores(string const& line)
{
   size_t p = line.rfind(COLSEP); // find the last column separator
   if (p != string::npos)
      p += strlen(COLSEP);      // skip the column separator
  istringstream buf(line.substr(p));
  float f(0);
  size_t count=0;
  while (buf>>f) {
     count++;
  }
  if (!buf.eof())
    cerr << efatal << "Format error in phrase table file:" << endl
         << "Malformed scores in line 1" << endl << ">" << line << "<"
         << exit_1;
  if (f > 2.7179 && f < 2.7181) // moses-style phrase table
     count--;
  return count;
}

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",  "print this message")
    ("quiet,q", "don't print progress information")
    ("input,i",  po::value<string>(&iFileName), "input file")
    ("output,o", po::value<string>(&oBaseName), "base name for output files")
//    ("truncate,x", po::value<string>(&truncation),
//     "how many bits to mask out for truncation (max. 23)")
    ;
  options_help << o;
  po::positional_options_description a;
  a.add("input",1);
  a.add("output",1);
  
  try {
    po::store(po::command_line_parser(ac,av).options(o).positional(a).run(), vm);
    po::notify(vm);
  } catch(std::exception& e) {
    cerr << efatal << e.what() << endl << help_message << exit_1;
  }


  if (vm.count("help"))
    {
      cerr << help_message << endl;
      exit(0);
    }

  if (!iFileName.size())
    cerr << efatal << "TEXTPT_FILE required." << endl << help_message << exit_1;

  if (!oBaseName.size())
    cerr << efatal << "OUTPUT_BASE_NAME required." << endl << help_message
         << exit_1;
}

struct SortByFreq
{
  bool
  operator()(map<float,uint32_t>::iterator const& A,
             map<float,uint32_t>::iterator const& B)
  {
    return B->second < A->second;
  }
};

class Score
{
public:
  uint32_t id;
  uint32_t cnt;
  Score(uint32_t _id) : id(_id), cnt(0) {};
  bool
  operator<(Score const& A)
  {
    return id < A.id;
  }

  
};

struct byFreq
{
  bool operator()(map<float,Score>::const_iterator const& A, 
                  map<float,Score>::const_iterator const& B) 
  { 
    return A->second.cnt > B->second.cnt;
  }
};
    
vector<uint32_t>
mkCodeBook(map<float,Score> const& M, ostream& codebook)
{
  vector<uint32_t> ret(M.size());
  vector<map<float,Score>::const_iterator> v(M.size());
  size_t i = 0;
  for (map<float,Score>::const_iterator m = M.begin(); m != M.end(); m++)
    v[i++] = m;
  sort(v.begin(),v.end(),byFreq());
  vector<uint32_t> bits_needed(max(2,int(ceil(log2(v.size()-1))))+1,0);
  for (size_t k = 0; k < v.size(); k++)
    {
      assert(v[k]->second.id < ret.size());
      ret[v[k]->second.id] = k;
      size_t bn = max(2,int(ceil(log2(k))));
      bits_needed[bn] += v[k]->second.cnt;
    }
  // determine the best encoding scheme
  Escheme best = best_scheme(bits_needed,5);
  vector<uint32_t> const& blocks = best.blockSizes;
  uint32_t x = v.size();               
  // number of distinct scores in this column
  codebook.write(rcast<char*>(&x),4); 
  x = blocks.size();                   
  // number of bit blocks used to encode score IDs
  codebook.write(rcast<char*>(&x),4); 
  for (size_t k = 0; k < blocks.size(); k++)
    codebook.write(rcast<char const*>(&(blocks[k])),4);   // bit block sizes
  for (size_t k = 0; k < v.size(); k++)
    codebook.write(rcast<char const*>(&(v[k]->first)),sizeof(float));
  return ret;
}


void
process_line(string const& line, ostream& tmp,
             vector<map<float,Score> >& scoreCount, size_t linectr)
{
  size_t p = line.rfind(COLSEP); // find the last column separator
  if (p == string::npos)
    cerr << efatal << "Format error in phrase table file:" << endl
         << "No column separator '" << COLSEP << "' in line " << linectr << endl
         << ">" << line << "<"
         << exit_1;
  p += strlen(COLSEP); // skip the column separator
  istringstream buf(line.substr(p));

  float s; 
  for (size_t i = 0; i < scoreCount.size(); i++)
    {
      buf >> s;
      if (buf.fail())
        cerr << efatal << "Format error in phrase table file:" << endl
             << "Missing or bad score in line " << linectr << endl
             << ">" << line << "<"
             << exit_1;
      map<float,Score>::value_type foo(s,Score(scoreCount[i].size()));
      map<float,Score>::iterator bar = scoreCount[i].insert(foo).first;
      bar->second.cnt++;
      tmp.write(rcast<char*>(&(bar->second.id)),sizeof(bar->second.id));
    }
}

int MAIN(argc, argv)
{
  interpret_args(argc,(char **)argv);
  string line;

  // count score occurrences, writing preliminary results to the .scr file.
  string scrName(oBaseName+".scr");
  ofstream tmpFile(scrName.c_str());
  if (tmpFile.fail())
    cerr << efatal << "Unable to open preliminary score file '" << scrName << "' for writing."
         << exit_1;

  #if IN_PORTAGE
  iSafeMagicStream in(iFileName);
#else
  filtering_istream in;
  open_input_stream(iFileName,in);
#endif

  getline(in,line);
  vector<map<float,Score> > scoreCount(countScores(line));
  size_t linectr = 1;
  process_line(line, tmpFile, scoreCount, linectr);
  while (getline(in,line))
    {
      if (++linectr%1000000 == 0)
        cerr << "Counting scores: " << linectr/1000000 << "M lines read" << endl;
#if 0
      if (linectr%10000 == 0 || linectr>235180)
        cerr << "[" << linectr << "] " << line << endl;
#endif
      process_line(line, tmpFile, scoreCount, linectr);
    }
  tmpFile.close();
  
  // write codebook and get remapping vectors mapping from
  // preliminary score IDs to final score IDs
  vector<vector<uint32_t> > remap(scoreCount.size());
  string cbkName(oBaseName+".cbk");
  ofstream cbk(cbkName.c_str());
  if (cbk.fail())
    cerr << efatal << "Unable to open codebook file '" << cbkName << "' for writing."
         << exit_1;
  uint32_t scnt = scoreCount.size();
  cbk.write(rcast<char*>(&scnt),4);
  for (size_t i = 0; i < scoreCount.size(); i++)
    remap[i] = mkCodeBook(scoreCount[i],cbk);
  
  // now remap the temporary .scr file to its final version
  bio::mapped_file scr;
  try {
    scr.open(scrName, ios::in|ios::out);
    if (!scr.is_open()) throw std::exception();
  } catch(std::exception& e) {
    cerr << efatal << "Unable to open final score file '" << scrName << "' for read/write."
         << exit_1;
  }
  uint32_t* p = rcast<uint32_t*>(scr.data());
  size_t expected_size = linectr * scoreCount.size() * sizeof(uint32_t);
  if (scr.size() != expected_size)
    cerr << efatal << "Incorrect score file size: " << scr.size()
         << "; expected " << expected_size
         << exit_1;
  for (size_t x = 0; x < linectr; x++)
    for (size_t y = 0; y < scoreCount.size(); y++, p++)
      {
        assert(*p < remap[y].size());
        *p = remap[y][*p];
      }
}
END_MAIN
