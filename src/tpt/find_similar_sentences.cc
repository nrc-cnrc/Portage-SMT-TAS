/**
 * @author Ulrich Germann
 * @file find_similar_sentences.cc
 * @brief Finds sentences in a corpus similar to those in a test set.
 *
 * Currently measures sentence similarity via the Dice coefficient
 * as defined here: http://en.wikipedia.org/wiki/Dice_coefficient
 * (i.e., number of overlapping bigrams divided by average number of
 * bigrams in the two sentences). There's a conceptual flaw in this
 * implementation because if an ngram occurs twice in the input sentence
 * but only once in a reference candidate, both occurrences are counted.
 */
#include <vector>
#include <cmath>
#include <queue>

#include <boost/dynamic_bitset.hpp>
#include <boost/program_options.hpp>

#include "tpt_typedefs.h"
#include "tpt_tokenindex.h"
#include "tpt_tightindex.h"
#include "ug_mm_ctrack.h"
#include "ug_mm_sufa.h"

using namespace std;
using namespace ugdiss;

namespace po = boost::program_options;

static char base_help_message[] = "\n\
find_similar_sentences [options] BASE_NAME\n\
\n\
  Read tokenized sentences from stdin, find similar sentences in a corpus\n\
  loaded from files BASE_NAME.tdx, BASE_NAME.mct, BASE_NAME.msa, and report\n\
  the results to stdout.\n\
\n\
";

static stringstream options_help;       // set by interpret_args()

inline ostream& help_message(ostream& os)
{
  return os << base_help_message << options_help.str();
}

mmSufa     S;
mmCtrack   C;
TokenIndex T;

bool   quiet;
string bname; // common base name of .tdx .mct .msa

size_t topN;   // print topN matches
size_t ngSize; // size of ngrams used to find candidates
float  minSim; // threshold for similarity measure

void 
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",    "print this message")
    ("quiet,q",   "don't print progress information")
    ("top-n,t", po::value<size_t>(&topN)->default_value(3),
     "number of best matches to show")
    ("ngram-size,n", po::value<size_t>(&ngSize)->default_value(3),
     "size of ngrams used for initial filtering")
    ("min-sim,s", po::value<float>(&minSim)->default_value(.5),
     "minimum similarity for a candidate to be considered a match in range (0.0-1.0]")
    ;
  options_help << o;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name for corpus files")
    ;
  h.add(o);

  po::positional_options_description a;
  a.add("bname",1);

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
    cerr << efatal << "Base name for corpus files (BASE_NAME) required." << endl
         << help_message << exit_1;

  if (minSim <= 0 || minSim > 1)
    cerr << efatal << "Minimum similarity must be > 0.0 and <= 1.0." << endl
         << help_message << exit_1;

  quiet  = vm.count("quiet");
}

void
findCandidates(vector<id_type> const& snt)
{
  size_t ngs = min(ngSize,snt.size());
  map<id_type,short> ngmatch;
  
  typedef std::vector<id_type>::const_iterator iter;
  for (iter p = snt.begin(); p+ngs <= snt.end(); ++p)
    {
      char const* x = S.lower_bound(p,p+ngs);
      if (!x) continue;
      boost::dynamic_bitset<uint64_t> check(S.getCorpusSize());
      id_type sid,off;
      for (char const* const z = S.upper_bound(p,p+ngs); x < z;)
        {
          x = tightread(x,z,sid);
          x = tightread(x,z,off);
          if (check[sid]) continue;
          check.set(sid);
          ngmatch[sid]++;
        }
    }
  priority_queue<pair<float,id_type> > Q;
  typedef map<id_type,short>::iterator miter;
  for (miter m = ngmatch.begin(); m != ngmatch.end(); ++m)
    {
      size_t olen = C.sntEnd(m->first)-C.sntStart(m->first);
      float x = (2.*m->second)/(snt.size()+olen-2*(ngs-1));
      if (x > minSim)
        {
          // float y = 2*lcss(snt,(*S.corpus)[m->first]);
          // y /= snt.size()+(*S.corpus)[m->first].size();
          Q.push(pair<float,id_type>(x,m->first));
        }
    }
  for (size_t i = 1; Q.size() && i <= topN; ++i)
    {
      // surround the float by whitespace for diff-round.pl processing.
      cout << "[" << i << ": "
           << Q.top().first 
           << " :" << Q.top().second << "] "
           << C.str(Q.top().second,T) << endl;
      Q.pop();
   }
}

int MAIN(argc, argv)
{
  interpret_args(argc, (char **)argv);

  open_memory_mapped_suffix_array(bname, T, C, S);

  string line;
  vector<id_type> snt;
  while (getline(cin,line))
    {
      cout << line << endl;
      T.toIdSeq(snt, line);
      findCandidates(snt);
      cout << endl;
    }
}
END_MAIN
