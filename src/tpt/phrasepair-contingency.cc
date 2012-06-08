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
#include "tpt_tightindex.h"
#include "ug_mm_ttrack.h"
#include "ug_mm_tsa.h"
#include "timer.h"
//#include "vector_map.h"
#include <tr1/unordered_map>

namespace ugdiss {
  typedef L2R_Token<SimpleWordId> Token;
  typedef mmTtrack<Token> mmTTtrack;
  typedef mmTSA<Token> mmTSufa;
}

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
bool   sigfet = false;
bool   timer = false;
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
    ("sigfet,s",  "output in a format compatible with sigprune_fet")
    ("time,t",    "track the time taken for each phrase pair")
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
  sigfet = vm.count("sigfet");
  timer  = vm.count("time");
}

template <class SetT>
void
fillSet(char const* p, char const* const q, 
        SetT& set)
{
  id_type sid,off;
  while (p < q)
    {
      p = tightread(p,q,sid);
      p = tightread(p,q,off);
      set[sid] = 1;
    }
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

template <class MapT1, class MapT2>
size_t
intersection_size(const MapT1& set1, const MapT2& set2)
{
  size_t count = 0;
  for (typename MapT1::const_iterator i1(set1.begin()), e1(set1.end());
       i1 != e1; ++i1)
    if (set2.find(i1->first) != set2.end()) ++count;
  return count;
}

template <class MapT1>
size_t
intersection_size(const MapT1& set1, const boost::dynamic_bitset<uint64_t>& set2)
{
  size_t count = 0;
  for (typename MapT1::const_iterator i1(set1.begin()), e1(set1.end());
       i1 != e1; ++i1)
    if (set2.test(i1->first)) ++count;
  return count;
}

void chooseAndFillCachedSet(mmTSufa const& S,
      char const* l, char const* u, ushort key_size,
      boost::dynamic_bitset<uint64_t>& non_cached,
      boost::dynamic_bitset<uint64_t>*& setp)
{
  static const int min_cache_size = S.getCorpus()->size() / 100;
  if (u-l > min_cache_size) { // arbitrary threshold for caching bitsets
    pair<char const*,ushort> key(l,key_size);
    setp = &(B[key]);
  } else {
    setp = &non_cached;
  }
  if (setp->empty()) {
    setp->resize(S.getCorpus()->size());
    fillBitSet(l,u,*setp);
  }
}

void contingency(vector<Token> const& p1,
                 vector<Token> const& p2,
                 mmTSufa const& S1, 
                 mmTSufa const& S2,
                 size_t& jj, size_t& m1, size_t& m2)
{
  char const* l1 = S1.lower_bound(p1.begin(),p1.end());
  char const* u1 = S1.upper_bound(p1.begin(),p1.end());

  char const* l2 = S2.lower_bound(p2.begin(),p2.end());
  char const* u2 = S2.upper_bound(p2.begin(),p2.end());

  static const int max_vector_map_size = S1.getCorpus()->size() / 500;

  // vector_map is trivially faster for small to medium cases:
  //typedef vector_map<id_type,bool> SmallMapT;
  // unordered_map is a faster (up to 5%) for very large cases, and allows
  // a larger max_vector_map_size to remain competitive.
  typedef tr1::unordered_map<id_type,bool> SmallMapT;

  // Uglier code, yes, but choosing data structure by size speeds this code
  // up an order of magnitude.
  if (!l1) {
    m1 = jj = 0;
    if (!l2) {
      m2 = 0;
    } else if (u2-l2 < max_vector_map_size) {
      SmallMapT set2;
      fillSet(l2,u2,set2);
      m2 = set2.size();
    } else {
      boost::dynamic_bitset<uint64_t> set2, *set2p(NULL);
      chooseAndFillCachedSet(S2, l2, u2, p2.size(), set2, set2p);
      m2 = set2p->count();
    }
  } else if (u1-l1 < max_vector_map_size) {
    SmallMapT set1;
    fillSet(l1,u1,set1);
    m1 = set1.size();
    if (!l2) {
      m2 = jj = 0;
    } else if (u2-l2 < max_vector_map_size) {
      SmallMapT set2;
      fillSet(l2,u2,set2);
      m2 = set2.size();
      if (m1 < m2)
        jj = intersection_size(set1,set2);
      else
        jj = intersection_size(set2,set1);
    } else {
      boost::dynamic_bitset<uint64_t> set2, *set2p(NULL);
      chooseAndFillCachedSet(S2, l2, u2, p2.size(), set2, set2p);
      m2 = set2p->count();
      jj = intersection_size(set1,*set2p);
    }
  } else {
    boost::dynamic_bitset<uint64_t> set1, *set1p(NULL);
    chooseAndFillCachedSet(S1, l1, u1, p1.size(), set1, set1p);
    m1 = set1p->count();
    if (!l2) {
      m2 = jj = 0;
    } else if (u2-l2 < max_vector_map_size) {
      SmallMapT set2;
      fillSet(l2,u2,set2);
      m2 = set2.size();
      jj = intersection_size(set2,*set1p);
    } else {
      boost::dynamic_bitset<uint64_t> set2, *set2p(NULL);
      chooseAndFillCachedSet(S2, l2, u2, p2.size(), set2, set2p);
      m2 = set2p->count();
      jj = ((*set1p)&(*set2p)).count();
    }
  }

/*
  boost::dynamic_bitset<uint64_t> bs1(S1.getCorpus()->size());
  boost::dynamic_bitset<uint64_t> bs2(S2.getCorpus()->size());
  boost::dynamic_bitset<uint64_t>* bsptr1 = &bs1;
  boost::dynamic_bitset<uint64_t>* bsptr2 = &bs2;

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
*/
}

int MAIN(argc, argv)
{
  interpret_args(argc, (char **)argv);

  TokenIndex V1, V2;
  mmTTtrack C1, C2;
  mmTSufa S1, S2;
  open_mm_tsa(bname + "." + L1, V1, C1, S1);
  open_mm_tsa(bname + "." + L2, V2, C2, S2);

  string line,w;
  id_type cnt = 0;
  Timer t, t1, t2;
  t1.reset();
  t2.reset();
  while (getline(cin,line))
    {
      if (timer) t.reset();
      vector<Token> p1,p2;
      //boost::dynamic_bitset<uint64_t> bs1,bs2;

      size_t q(0), p(0), key_end(0);
      int part = 0;
      while (true) {
         p = line.find_first_not_of(" \t", q);
         if (p == string::npos) break;
         q = line.find_first_of(" \t", p);

         string token = line.substr(p,q-p);
         if (token == "|||") {
            if (part == 1) { key_end = q; break;    }
            else           { ++part;      continue; }
         }
         if (part == 0) p1.push_back(V1[token]);
         else           p2.push_back(V2[token]);

         if (q == string::npos) break;
      }

      size_t jj,m1,m2;
      contingency(p1,p2,S1,S2,jj,m1,m2);
      if (timer) cout << t.secsElapsed(1) << "s\t";
      if (sigfet) {
         cout << "\t" << jj
            << "\t" << m1
            << "\t" << m2
            << "\t" << C1.size()
            << "\t" << line
            << endl;
      }
      else {
         cout
            << line.substr(0,key_end) << " " // << " ||| "
            << jj << " " // joint count
            << m1 << " " // marginal count for p1
            << m2 << " " // marginal count for p2
            << C1.size() 
            // << " " << V1.toString(p1) << " ::: " << V2.toString(p2) 
            << endl;
      }
      if (!quiet && (++cnt)%1000==0) {
        cerr << cnt/1000 << "K phrase pairs processed in " << t1.secsElapsed(1) << "s." << endl;
        t1.reset();
      }
    }

  if (!quiet)
      cerr << cnt << " phrase pairs processed in total, in " << t2.secsElapsed(1) << "s." << endl;
}
END_MAIN
