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
/**
 * @author Ulrich Germann
 * @file mmsufa.build.cc
 * @brief Reads memory mapped corpus track file (.mct) and
 * writes a binary memory mapped suffix array file (.msa).
 *
 * Final step in mmsufa generation. Normally called via build-tp-suffix-array.sh.
 */
#include "tpt_typedefs.h"
#include "ug_mm_ctrack.h"
#include "tpt_tightindex.h"
#include "tpt_pickler.h"

#include <boost/program_options.hpp>

using namespace std;
using namespace ugdiss;
namespace po = boost::program_options;

static char base_help_message[] = "\n\
mmsufa.build [options] MMCTRACK_FILE MMSUFA_FILE\n\
\n\
  Read a memory mapped corpus file MMCTRACK_FILE (.mct) and write the \n\
  binary memory mapped suffix array MMSUFA_FILE (.msa).\n\
\n\
  This is the final step in creating a memory mapped suffix array for a corpus.\n\
  This program is called from build-tp-suffix-array.sh.\n\
\n\
";

static stringstream options_help;       // set by interpret_args()

inline ostream& help_message(ostream& os)
{
  return os << base_help_message << options_help.str();
}

string ctrackFile, sufaFile;
bool quiet = false;

void interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",  "print this message")
    ("quiet,q", "don't print progress information")
    ;
  options_help << o;

  po::options_description h("Hidden Options");
  h.add_options()
    ("mct", po::value<string>(&ctrackFile), "memory mapped corpus track file (.mct)")
    ("msa", po::value<string>(&sufaFile), "memory mapped suffix array file (.msa)")
    ;
  h.add(o);

  po::positional_options_description a;
  a.add("mct",1);
  a.add("msa",1);

  try {
    po::store(po::command_line_parser(ac,av).options(h).positional(a).run(), vm);
    po::notify(vm);
  } catch(std::exception& e) {
    cerr << efatal << e.what() << endl << help_message << exit_1;
  }

  if (vm.count("help"))
    {
      cerr << help_message << endl;
      exit(0);
    }

  if (ctrackFile.empty())
    cerr << efatal << "Memory mapped corpus track file MMCTRACK_FILE (.mct) required." << endl
         << help_message << exit_1;

  if (ctrackFile.empty())
    cerr << efatal << "Output file for memory mapped suffix array MMSUFA_FILE (.msa) required." << endl
         << help_message << exit_1;

  if (vm.count("quiet"))
    quiet=true;
}

mmCtrack C;
vector<count_type> wcnt;

class Token
{
public:
  id_type        sid;
  unsigned short offset;
  bool operator<(Token const& other) const;
  Token(id_type s, unsigned short p) : sid(s), offset(p) {};
};

bool
Token::
operator<(Token const& other) const
{
  if (this->sid == other.sid && this->offset == other.offset) 
    return false;
  id_type const* me  = C.sntStart(this->sid)+this->offset;
  id_type const* const myEnd   = C.sntEnd(this->sid);
  id_type const* you = C.sntStart(other.sid)+other.offset;
  id_type const* const yrEnd   = C.sntEnd(other.sid);
  size_t mySize = myEnd-me;
  size_t yrSize = yrEnd-you;
  size_t maxi = min(mySize,yrSize);
  
  for (size_t i = 0; i < maxi; i++,++me,++you)
      if (*me != *you) return *me < *you;
  if (mySize == yrSize) 
    return this->sid < other.sid;
  else
    return mySize < yrSize;
}


void count_words()
{
//  wcnt.reserve(1000000);
  wcnt.reserve(C.size());       // guess at magnitude of # distinct words
  id_type const* i = C.sntStart(0);
  id_type const* const stop = i+C.numTokens();
  while (i < stop)
    {
      while (*i >= wcnt.size()) 
	wcnt.push_back(0);
      wcnt[*i++]++;
    }
#if 0
  // debugging code; can be removed once the thing is stable
  size_t controlCounter=0;
  for (size_t i = 0; i < wcnt.size(); ++i)
    controlCounter += wcnt[i];
  assert(controlCounter == C.numTokens());
#endif
}

Uint elapsed(size_t start_time) {
   return time(NULL) - start_time;
}

#define ELAPSED " (" << elapsed(start_time) << "s)"

int MAIN(argc, argv)
{
  interpret_args(argc, (char **)argv);
  size_t start_time = time(NULL);

  C.open(ctrackFile);
  count_words();
  
  vector<vector<Token> > sufa(wcnt.size());

  // EJJ Write leader in suffix array file.
  ofstream out(sufaFile.c_str());
  if (out.fail())
    cerr << efatal << "Unable to open suffix array file '" << sufaFile
         << "' for writing." << exit_1;
  filepos_type idxStart(0);
  id_type idxSize(sufa.size());
  numwrite(out,idxStart);
  numwrite(out,idxSize);
  vector<filepos_type> index(sufa.size()+1);

  // EJJ Multi-pass process with exponential growth in number of bins handled.
  // Tokens are sorted by reverse frequency, which means, by Zipfean logic, and
  // confirmed empirically, that each pass will have similar amounts of data to
  // process.  We thus significantly reduce the amount of memory required by
  // this program: a little more than the corpus track is all that's needed,
  // instead of 2 to 3 times the size of the corpus track.  The time overhead
  // for this iterative processing is small, so we can safely hard-code this
  // logic without parameterizing it.
  size_t batch_start = 0;
  size_t batch_end = 10;
  while (true) {
    if (batch_end > wcnt.size()) batch_end = wcnt.size();

    if (!quiet)
      cerr << "Starting batch for vocids in [" << batch_start << "," << batch_end << ")" << ELAPSED << endl;

    for (size_t i = batch_start; i < batch_end; i++)
      sufa[i].reserve(wcnt[i]);

    for (id_type i = 0; i < C.size(); i++)
      {
        id_type const* k = C.sntStart(i);
        id_type const* const stop = C.sntEnd(i);
        for (ushort p = 0; k < stop; ++p,++k)
          if (*k >= batch_start && *k < batch_end)
            sufa[*k].push_back(Token(i,p));
        if (!quiet && (i+1)%1000000==0)
          cerr << (i+1)/1000000 << "M sentences processed" << ELAPSED << endl;
      }
    
    if (!quiet)
      {
        cerr << C.size() << " sentences processed in total." << ELAPSED << endl;
        cerr << "Sorting ..." << endl;
      }
    for (size_t i = batch_start; i < batch_end; i++)
      {
        if (sufa[i].size() > 5000 && !quiet)
          cerr << "Sorting " << sufa[i].size() << " items for id " << i << "."<< ELAPSED << endl;
        sort(sufa[i].begin(),sufa[i].end(),less<Token>());
        #if 0
          // debugging code; can be removed once the thing is stable
          // Sanity check ...
          typedef vector<Token>::iterator iter;
          for (iter m = sufa[i].begin(); m != sufa[i].end(); ++m)
            assert(*(C.sntStart(m->sid)+m->offset) == i);
        #endif
      }

    if (!quiet)
      cerr << "Writing data..." << ELAPSED << endl;
    for (size_t i = batch_start; i < batch_end; i++)
      {
        index[i] = out.tellp();
        for (size_t k = 0; k < wcnt[i]; k++)
          {
            tightwrite(out,sufa[i][k].sid,0);
            tightwrite(out,sufa[i][k].offset,1);
          }
        
        // Release the memory from sufa[i], since we're done with it
        {
          vector<Token> empty;
          sufa[i].swap(empty);
        }
      }

    if ( batch_end >= wcnt.size() ) break;
    batch_start = batch_end;
    batch_end *= 2;
  }
   
  // EJJ write the end of the file, now that all the batches have been processed.
  idxStart = index[sufa.size()] = out.tellp();
  for (size_t i = 0; i < index.size(); i++)
    numwrite(out,index[i]-index[0]);
  out.seekp(0);
  numwrite(out,idxStart);
  if (out.fail())
    cerr << efatal << "Writing suffix array file '" << sufaFile << "' failed."
         << exit_1;
  out.close();
  if (!quiet) cerr << "Done" << ELAPSED << endl;
}
END_MAIN
