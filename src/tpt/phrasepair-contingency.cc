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
 * @file phrasepair-contingency.cc
 * @brief Add raw phrase pair contingency counts to a phrase table.
 */
#include <map>

#include <boost/dynamic_bitset.hpp>
#include <boost/program_options.hpp>

#include "tpt_tokenindex.h"
#include "ug_mm_ctrack.h"
#include "ug_mm_sufa.h"
#include "tpt_tightindex.h"

using namespace std;
using namespace ugdiss;

namespace po = boost::program_options;

static char base_help_message[] = "\n\
phrasepair-contingency [options] BASE_NAME L1 L2\n\
\n\
  Read a phrase table from stdin, and the tightly packed suffix array for\n\
  the corpora for languages L1 and L2, compute raw phrase pair contingency\n\
  counts, and write the results to stdout.\n\
\n\
  BASE_NAME is the base name for the two suffix arrays.\n\
  The files expected for language L1 are:\n\
    BASE_NAME.L1.tpsa/tdx, BASE_NAME.L1.tpsa/mct, BASE_NAME.L1.tpsa/msa\n\
  or\n\
    BASE_NAME.L1/tdx, BASE_NAME.L1/mct, BASE_NAME.L1/msa\n\
  or\n\
    BASE_NAME.L1.tdx, BASE_NAME.L1.mct, BASE_NAME.L1.msa\n\
  Similarly for language L2.\n\
\n\
";

static stringstream options_help;       // set by interpret_args()

inline ostream& help_message(ostream& os)
{
  return os << base_help_message << options_help.str();
}

bool   quiet;
string bname;
string L1;
string L2;

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",    "print this message")
    ("quiet,q",   "don't print progress information")
    ;
  options_help << o;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name for suffix array files")
    ("l1", po::value<string>(&L1), "language 1 name")
    ("l2", po::value<string>(&L2), "language 2 name")
    ;
  h.add(o);

  po::positional_options_description a;
  a.add("bname",1);
  a.add("l1",1);
  a.add("l2",1);

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

  if (bname.empty())
    cerr << efatal << "Base name for suffix arrays (BASE_NAME) required." << endl
         << help_message << exit_1;

  if (L1.empty())
    cerr << efatal << "Language 1 name (L1) required." << endl
         << help_message << exit_1;

  if (L2.empty())
    cerr << efatal << "Language 2 name (L2) required." << endl
         << help_message << exit_1;

  quiet  = vm.count("quiet");
}

void 
fillBitSet(char const* p, char const* const q, 
           boost::dynamic_bitset<uint64_t>& bs)
{
  id_type sid,off;
  while (p < q)
    {
      p = tightread(p,q,sid);
      p = tightread(p,q,off);
      bs.set(sid);
    }
}

typedef map<pair<char const*,ushort>,boost::dynamic_bitset<uint64_t> > myMap;
typedef myMap::iterator myMapIter;
myMap B;


void contingency(vector<id_type> const& p1,
                 vector<id_type> const& p2,
                 mmSufa const& S1, 
                 mmSufa const& S2,
                 size_t& jj, size_t& m1, size_t& m2)
{
  boost::dynamic_bitset<uint64_t> bs1(S1.getCorpus()->size());
  boost::dynamic_bitset<uint64_t> bs2(S2.getCorpus()->size());
  boost::dynamic_bitset<uint64_t>* bsptr1 = &bs1;
  boost::dynamic_bitset<uint64_t>* bsptr2 = &bs2;

  char const* l1 = S1.lower_bound(p1.begin(),p1.end());
  char const* u1 = S1.upper_bound(p1.begin(),p1.end());

  char const* l2 = S2.lower_bound(p2.begin(),p2.end());
  char const* u2 = S2.upper_bound(p2.begin(),p2.end());

  if (l1 && u1-l1 > 4096*1024) // arbitrary threshold for caching bitsets
    {
      pair<char const*,ushort> key(l1,p1.size());
      bsptr1 = &(B[key]);
      if (bsptr1->size() == 0)
        bsptr1->resize(bs1.size());
    }
  if (l1 && bsptr1->count() == 0)
    fillBitSet(l1,u1,*bsptr1);

  if (l2 && u2-l2 > 4096*1024) // arbitrary threshold for caching bitsets
    {
      pair<char const*,ushort> key(l2,p2.size());
      bsptr2 = &(B[key]);
      if (bsptr2->size() == 0)
        bsptr2->resize(bs2.size());
    }
  if (l2 && bsptr2->count() == 0)
    fillBitSet(l2,u2,*bsptr2);

  jj = (*bsptr1&*bsptr2).count();
  m1 = bsptr1->count();
  m2 = bsptr2->count();
}

int MAIN(argc, argv)
{
  interpret_args(argc, (char **)argv);

  TokenIndex V1, V2;
  mmCtrack C1, C2;
  mmSufa S1, S2;
  open_memory_mapped_suffix_array(bname + "." + L1, V1, C1, S1);
  open_memory_mapped_suffix_array(bname + "." + L2, V2, C2, S2);

  string line,w;
  id_type cnt = 0;
  while (getline(cin,line))
    {
      vector<id_type> p1,p2;
      boost::dynamic_bitset<uint64_t> bs1,bs2;
      istringstream buf(line);
      while (buf>>w && w != "|||") p1.push_back(V1[w]);
      while (buf>>w && w != "|||") p2.push_back(V2[w]);
      size_t jj,m1,m2;
      contingency(p1,p2,S1,S2,jj,m1,m2);
      cout << line.substr(0,buf.tellg()) << " " // << " ||| "
           << jj << " " // joint count
           << m1 << " " // marginal count for p1
           << m2 << " " // marginal count for p2
           << C1.size() 
        // << " " << V1.toString(p1) << " ::: " << V2.toString(p2) 
	   << endl;
      if (!quiet && (++cnt)%1000==0)
        cerr << cnt/1000 << "K phrase pairs processed" << endl;
    }

  if (!quiet)
      cerr << cnt << " phrase pairs processed in total." << endl;
}
END_MAIN
