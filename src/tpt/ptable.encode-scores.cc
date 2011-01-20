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
#include <tr1/unordered_map>
#include <ctime>
#include <cmath>
#include <sstream>
#include <limits>  // numeric_limits<float>::min & numeric_limits<float>::max

#if IN_PORTAGE
#include "file_utils.h"
using namespace Portage;
#else
#include <boost/iostreams/stream.hpp>
#include "ugStream.h"
#endif

#include <boost/program_options.hpp>

//#define DEBUG_TPT

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

namespace po  = boost::program_options;
namespace bio = boost::iostreams;

using namespace std;
using namespace ugdiss;

const char * const COLSEP = " ||| ";

string iFileName;
string oBaseName;
//string truncation;
// vector<size_t> truncactionBits; // add truncation later (maybe)


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

size_t
count_scores_per_line(string const& line)
{
   size_t p = line.rfind(COLSEP); // find the last column separator
   if (p == string::npos)
     cerr << efatal << "Format error in phrase table file:" << endl
          << "No column separator '" << COLSEP << "' in line 1" << endl
          << ">" << line << "<"
          << exit_1;
  p += strlen(COLSEP);      // skip the column separator
  const char *buf(line.c_str()+p);

  float f(0);
  size_t count=0;
  while(1)
    {
      char *next;
      f = static_cast<float>(strtod(buf, &next));
      if (buf == next) break;
      ++count;
      buf = next;
    }
  if (line.find_first_not_of(" \t", buf-line.c_str()) != string::npos)
    cerr << efatal << "Format error in phrase table file:" << endl
         << "Malformed scores in line 1" << endl << ">" << line << "<"
         << exit_1;
  if (f > 2.7179 && f < 2.7181) // moses-style phrase table
     count--;
  return count;
}

void
process_line(string const& line, ostream& tmp,
             size_t scrs_per_line, size_t linectr)
{
  size_t p = line.rfind(COLSEP); // find the last column separator
  if (p == string::npos)
    cerr << efatal << "Format error in phrase table file:" << endl
         << "No column separator '" << COLSEP << "' in line " << linectr << endl
         << ">" << line << "<"
         << exit_1;
  p += strlen(COLSEP); // skip the column separator
  const char *buf(line.c_str()+p);

  for (size_t i = 0; i < scrs_per_line; i++)
    {
      char *next;
      double s_prime = strtod(buf, &next);
      if (next == buf)
        cerr << efatal << "Format error in phrase table file:" << endl
             << "Missing or bad score in line " << linectr << endl
             << ">" << line << "<"
             << exit_1;
      buf = next;
      // There are some cases where the prob read is outside a float's
      // permitted value.  Let's make sure we cope well with this scenario.
      if (s_prime > 0) {
         if (s_prime < numeric_limits<float>::min()) s_prime = numeric_limits<float>::min();
         if (s_prime > numeric_limits<float>::max()) s_prime = numeric_limits<float>::max();
      }
      else if (s_prime < 0) {
         if (-s_prime < numeric_limits<float>::min()) s_prime = -numeric_limits<float>::min();
         if (-s_prime > numeric_limits<float>::max()) s_prime = -numeric_limits<float>::max();
      }
      float s = static_cast<float>(s_prime);
      tmp.write(reinterpret_cast<char*>(&s),sizeof(s));
    }
  if (line.find_first_not_of(" \t", buf-line.c_str()) != string::npos)
     cerr << efatal << "Format error in phrase table file:" << endl
          << "Extra or bad score in line " << linectr << endl
          << ">" << line << "<"
          << exit_1;
}

// The scores map maps probability scores to ids.
// We use a specialized allocator for the scores map that allocates the value
// nodes in the scores vector, instead of using the standard way that uses
// new to allocate each node. This allows us to manage the space much more
// efficiently. The internal scores vector must not be allowed to grow, so its
// capacity must be set to the maximum allowable in the constructor.

template<typename Tp>
class scores_allocator
{
public:
  typedef size_t            size_type;
  typedef ptrdiff_t         difference_type;
  typedef Tp*              pointer;
  typedef const Tp*        const_pointer;
  typedef Tp&              reference;
  typedef const Tp&        const_reference;
  typedef Tp               value_type;

private:
  typedef struct {
     tr1::__detail::_Hash_node<pair<float, uint32_t>, false> _node;
     float score() const { return _node._M_v.first; }
     uint32_t id () const { return _node._M_v.second; }
  } score_t;

  vector<score_t>* scores;
  bool owner;   // object owns storage for scores vector?

public:
  template<typename Tp1>
  struct rebind { typedef scores_allocator<Tp1> other; };

  scores_allocator(size_t max_size=0) throw()
    : scores(new vector<score_t>), owner(true)
  {
     scores->reserve(max_size);
  }

  scores_allocator(const scores_allocator& o) throw()
    : scores(o.scores), owner(false) {}

  template<typename Tp1>
  scores_allocator(const scores_allocator<Tp1>& o) throw()
    : scores(reinterpret_cast<vector<score_t> *>(&o.get_scores_ref())), owner(false) {}

  ~scores_allocator() throw() { if (owner) delete scores; }

  pointer
  address(reference x) const { return &x; }

  const_pointer
  address(const_reference x) const { return &x; }

  void
  deallocate(pointer p, size_type n)
  {
    // delete p if it was allocated with new.
    if (p < (void *)&scores->front() || p > (void *)&scores->back())
      delete p;
  }

  pointer
  allocate(size_type n, const void* p = 0)
  {
    if (n > 1)
      return static_cast<Tp*>(::operator new(n * sizeof(Tp)));

    assert (sizeof(Tp) == sizeof(score_t));
    if (scores->size() + 1 > scores->capacity())
      std::__throw_bad_alloc();
    scores->resize(scores->size() + 1);
    return (pointer)&scores->back();
  }

  void
  construct(pointer p, const Tp& val)
  {
    ::new((void *)p) value_type(val);
  }

  void
  destroy(pointer p) {}

  size_t
  size() { return scores->size(); }

  size_type
  max_size() const throw() { return scores->capacity(); }

  vector<score_t>&
  get_scores_ref() const { return *scores; }

  vector<float>
  get_scores() const
  {
    vector<float> ret(scores->size());
    for (size_t i=0; i < scores->size(); ++i)
       ret[i] = (*scores)[i].score();
    return ret;
  }
};

template<typename Tp>
inline bool
operator==(const scores_allocator<Tp>&, const scores_allocator<Tp>&)
{ return true; }

template<typename Tp>
inline bool
operator!=(const scores_allocator<Tp>&, const scores_allocator<Tp>&)
{ return false; }

typedef scores_allocator<pair<float, uint32_t> > scores_allocator_t;
typedef tr1::unordered_map<float, uint32_t, tr1::hash<float>, equal_to<float>,
                           scores_allocator_t> scores_map_t;

//typedef tr1::unordered_map<float, uint32_t> scores_map_t;
//typedef map<float, uint32_t> scores_map_t; // use to reproduce Uli's original output
//vector<uint32_t> ids;  // use to reproduce Uli's original sort order.

// Count score occurrences, writing preliminary score IDs to the .scr file.
// Fill the scrs and cnts vectors, ordered by id, with scores and their
// corresponding counts, respectively.
void
count_scores(bio::mapped_file &scr_file, size_t which_scr, size_t scrs_per_line,
             vector<float>& scrs, vector<uint32_t>& cnts)
{
  float* scr_end = reinterpret_cast<float*>(scr_file.data() + scr_file.size());
  float* p = reinterpret_cast<float*>(scr_file.data());
  size_t num_lines = (scr_end - p)/scrs_per_line;
  cnts.reserve((num_lines + 7) / 8);

  scores_allocator_t scores_alloc(num_lines);
  TPT_DBG(cerr << "scores_alloc: max_size=" << scores_alloc.max_size() << endl);
  scores_map_t *scores(new scores_map_t(0, tr1::hash<float>(), equal_to<float>(), scores_alloc));
//  scores_map_t *scores(new scores_map_t);  // for map or default unordered_map scores_map_t

  p += which_scr;
  while (p < scr_end)
    {
      scores_map_t::value_type new_score(*p, scores->size());
      TPT_DBG(cerr << "inserting: " << new_score.first << " " << new_score.second << endl);
      scores_map_t::iterator score_it = scores->insert(new_score).first;
      assert(score_it->second <= cnts.size());
      if (score_it->second == cnts.size())
         cnts.push_back(0);
      ++cnts[score_it->second];
      // Convert the score in the file to its preliminary score id.
      *reinterpret_cast<uint32_t*>(p) = score_it->second;
      p += scrs_per_line;
    }
  TPT_DBG(cerr << "cnts: size = " << cnts.size() << ", capacity = " << cnts.capacity() << endl);
//  sleep(5);           // maximum space used at this point

  delete scores;  // no longer needed

  // Retrieve the scores from the scores allocator. scrs will be ordered by id.
  scrs = scores_alloc.get_scores();

  // Code for using map<float, uint32_t> to reproduce Uli's original output:
  // Use global ids vector to reproduce Uli's original sort order.
//  scrs.resize(cnts.size());
//  ids.resize(cnts.size());
//  uint i = 0;
//  for (scores_map_t::const_iterator m = scores->begin(); m != scores->end(); m++)
//    {
//      scrs[m->second] = m->first;
//      ids[i++] = m->second;
//    }
//  delete scores;
}

struct byFreq
{
  vector<uint32_t>& cnts;
  byFreq(vector<uint32_t>& cnts) : cnts(cnts) {}
  bool operator()(uint32_t const& A, uint32_t const& B)
  {
     return cnts[A] > cnts[B];
  }
};

// Write the codebook for a scores column to the .cbk file and convert cnts to
// a remapping vector mapping the preliminary score IDs to final score IDs.
void
mkCodeBook(ostream& codebook, vector<float>& scrs, vector<uint32_t>& cnts)
{
  TPT_DBG(cerr << "mkCodeBook: scrs.size = " << scrs.size() << endl);
  vector<uint32_t>& remap = cnts;  // cnts doubles as the remap vector.

  // Note: comment out the next 3 lines when reproducing Uli's original output.
  vector<uint32_t> ids(scrs.size());
  for (size_t i = 0; i < scrs.size(); ++i)
     ids[i] = i;
  sort(ids.begin(), ids.end(), byFreq(cnts));

  vector<uint32_t> bits_needed(max(2, int(ceil(log2(ids.size()))))+1, 0);
  for (size_t k = 0; k < scrs.size(); k++)
    {
      size_t bn = max(2, int(ceil(log2(k+1))));
      bits_needed[bn] += cnts[ids[k]];
    }
  for (size_t k = 0; k < remap.size(); k++)
     remap[ids[k]] = k;       // remap the ids.
  TPT_DBG(cerr << "  bits_needed.size = " << bits_needed.size() << endl);

  // determine the best encoding scheme
  Escheme best = best_scheme(bits_needed,5);
  vector<uint32_t> const& blocks = best.blockSizes;

  TPT_DBG(cerr << "  scrs.size = " << scrs.size() << endl);
  uint32_t x = scrs.size(); // number of distinct scores in this column
  codebook.write(reinterpret_cast<char*>(&x), 4);
  TPT_DBG(cerr << "  blocks.size = " << blocks.size() << endl);
  x = blocks.size(); // number of bit blocks used to encode score IDs
  codebook.write(reinterpret_cast<char*>(&x), 4);
  for (size_t k = 0; k < blocks.size(); k++)
    {
      TPT_DBG(cerr << "  blocks[" << k << "] = " << blocks[k] << endl);
      codebook.write(reinterpret_cast<char const*>(&(blocks[k])), 4); // bit block sizes
    }
  for (size_t k = 0; k < scrs.size(); k++)
    codebook.write(reinterpret_cast<char const*>(&scrs[ids[k]]), sizeof(float));
}

// Remap the preliminary score IDs for a scores column in the .scr file to
// the final score IDs.
void
remap_ids(bio::mapped_file& scr_file, size_t which_scr, size_t scrs_per_line,
         vector<uint32_t>& remap)
{
  uint32_t* p = reinterpret_cast<uint32_t*>(scr_file.data()) + which_scr;
  uint32_t* scr_end = reinterpret_cast<uint32_t*>(scr_file.data() + scr_file.size());
  while (p < scr_end)
    {
      assert(*p < remap.size());
      *p = remap[*p];
      p += scrs_per_line;
    }
}


int MAIN(argc, argv)
{
  cerr << "ptable.encode-scores starting." << endl;

  interpret_args(argc,(char **)argv);
  string line;

  // Read the phrase table, storing the scores into the .scr file for fast access.
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

  cerr << "Reading phrase table." << endl;
  time_t start_time(time(NULL));
  getline(in,line);
  uint32_t scrs_per_line = count_scores_per_line(line);
  size_t linectr = 1;
  process_line(line, tmpFile, scrs_per_line, linectr);
  while (getline(in,line))
    {
      if (++linectr%1000000 == 0)
        cerr << "Reading phrase table: " << linectr/1000000 << "M lines read" << endl;
      process_line(line, tmpFile, scrs_per_line, linectr);
    }
  tmpFile.close();

  cerr << "Read " << linectr << " lines in " << (time(NULL) - start_time) << " seconds." << endl;

  // Open the .scr (memory-mapped, read/write) and .cbk (write) files
  bio::mapped_file scr;
  try {
    scr.open(scrName, ios::in|ios::out);
    if (!scr.is_open()) throw std::exception();
  } catch(std::exception& e) {
    cerr << efatal << "Unable to open final score file '" << scrName << "' for read/write."
         << exit_1;
  }
  size_t expected_size = linectr * scrs_per_line * sizeof(uint32_t);
  if (scr.size() != expected_size)
    cerr << efatal << "Incorrect score file size: " << scr.size()
         << "; expected " << expected_size
         << exit_1;

  string cbkName(oBaseName+".cbk");
  ofstream cbk(cbkName.c_str());
  if (cbk.fail())
    cerr << efatal << "Unable to open codebook file '" << cbkName << "' for writing."
         << exit_1;
  cbk.write(reinterpret_cast<char*>(&scrs_per_line),4); // number of code books.

  // For each score field (column) in the .scr file:
  // 1. Count score occurrences, writing preliminary score IDs to the .scr file.
  // 2. Write its codebook to the .cbk file and get remapping vector mapping
  //    the preliminary score IDs to final score IDs.
  // 3. Remap the preliminary score IDs in the .scr file to the final score IDs.
  for (size_t i = 0; i < scrs_per_line; i++)
    {
      cerr << "Counting scores for code book " << i << "." << endl;
      vector<uint32_t> cnts;
      vector<float> scrs;
      count_scores(scr, i, scrs_per_line, scrs, cnts);
      cerr << "Writing code book " << i << "." << endl;
      vector<uint32_t>& remap = cnts;  // cnts doubles as remap vector
      mkCodeBook(cbk, scrs, cnts);
      cerr << "Remapping score ids for code book " << i << "." << endl;
      remap_ids(scr, i, scrs_per_line, remap);
    }

  cerr << "ptable.encode-scores finished." << endl;
}
END_MAIN
