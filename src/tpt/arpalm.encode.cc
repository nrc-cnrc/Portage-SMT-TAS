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
 * @file arpalm.encode.cc
 * @brief Creates the TokenIndex and codebook from LM file in standard arpa
 * text format, and encodes n-gram information for assembly.
 * 
 * First step in arpalm2tplm conversion. Normally called via arpalm2tplm.sh.
 */

#include <iostream>
#include <iomanip>

#define USE_HASH 0

#if USE_HASH
#include <ext/hash_map>
#endif

#include <tr1/unordered_map>
#include <map>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <ctime>

#if IN_PORTAGE
#include "file_utils.h"
using namespace Portage;
#else
#include <boost/iostreams/stream.hpp>
#include "ugStream.h" 
#endif

#include <boost/program_options.hpp>

//#define DEBUG_TPT
//#define DEBUG_TPT_2

#include "tpt_typedefs.h"
#include "tpt_tokenindex.h"
#include "tpt_pickler.h"

using namespace std;
using namespace ugdiss;

namespace po = boost::program_options;

static char base_help_message[] = "\n\
arpalm.encode [options] [--lm] LM_FILE [-o] OUTPUT_BASE_NAME [[-u] UNKNOWN_TOKEN]\n\
\n\
  Create the TokenIndex file OUTPUT_BASE_NAME.tdx and the codebook file\n\
  OUTPUT_BASE_NAME.cbk from the LM_FILE in standard arpa text format, and\n\
  encode the n-gram information for assembly producing a set of intermediate\n\
  files (<N>grams.bo.*, <N>grams.p.*, bowzero) and a script sng-av.jobs for\n\
  running the next step.\n\
\n\
  This is the first step in arpalm2tplm conversion.\n\
  This program is normally called via arpalm2tplm.sh.\n\
\n\
";

static stringstream options_help;       // set by interpret_args()

inline ostream& help_message(ostream& os)
{
  return os << base_help_message << options_help.str();
}

string oBaseName; // base name for output
string tidxFile;  // output file for token index (oBaseName+".tdx")
string cbkFile;   // output file for codebook (oBaseName+".cbk")
string lmFile;    // input file
string unkToken;  // 'unknown' token
bool quiet=false;
size_t truncate_bitshift=0;

bool
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",  "print this help message")
    ("quiet,q", "don't print progress information")
    ("unk,u",po::value<string>(&unkToken)->default_value(string("<unk>")),
     "'unknown' token")
    ("lm", po::value<string>(&lmFile),
     "language model file in arpa text format (input)")
    ("output,o", po::value<string>(&oBaseName),
     "base name for output files")
    ("truncate,x", po::value<size_t>(&truncate_bitshift),
     "number of bits by which to truncate the float mantissa (max 23)")
    // ("tidx,t", po::value<string>(&tidxFile), "token index file (output)")
    // ("cbk,t", po::value<string>(&cbkFile), "codebook file (output)")
    ;
  options_help << o;
  po::positional_options_description a;
  a.add("lm",1);
  a.add("output",1);
  a.add("unk",1);

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

  if (!lmFile.size())
    cerr << efatal << "LM_FILE required." << endl << help_message << exit_1;
  if (!oBaseName.size())
    cerr << efatal << "OUTPUT_BASE_NAME required." << endl
         << help_message << exit_1;

  if (truncate_bitshift > 23)
    cerr << efatal << "Invalid value for --truncate (must be in range 0-23)." << endl
         << help_message << exit_1;

  tidxFile = oBaseName+".tdx";
  cbkFile  = oBaseName+".cbk";
  if (vm.count("quiet"))
    quiet=true;
  return true;
}


float 
truncate(float p)
{
  static uint32_t const mask 
    = ((0xFFFFFFFFU)>>truncate_bitshift)<<truncate_bitshift;
  union { float f; uint32_t i; } foo;
  foo.f = p;
  foo.i &= mask;
  return foo.f;
}

/** a hash of files for distributing ngram data over multiple files for
    parallel sorting */
class filePool
{
  size_t order; // ngram-order
  string tag;
  vector<ofstream*> files;
  vector<string> fileNames;
  float files_per_log;
public:
  filePool(string t, size_t o, id_type num_tokens, vector<size_t>& num_ng)
    : order(o), tag(t)
  {
    uint64_t max_size = 0;
    for(size_t i = 0; i < num_ng.size(); ++i)
      {
        uint64_t sz = (i+1) * num_ng[i];
        if (sz > max_size) max_size = sz;
      }
    files_per_log = max_size / 500000000.;
    float log_tokens = log10(num_tokens);
    size_t num_files = max(1, ceil(log_tokens * files_per_log));
    files_per_log = num_files / log_tokens;
    TPT_DBG(cerr << "filePool '" << t << "' order " << o
                 << ": max(n * #ngrams) = " << max_size
                 << ", # files = " << num_files
                 << ", files_per_log = " << files_per_log << endl);
    files.resize(num_files, NULL);
  };

  ~filePool()
  {
    for (size_t i = 0; i < files.size(); i++)
      if (files[i])
	{
	  files[i]->close();
	  delete files[i];
	}
  }

  ofstream&
  operator[](id_type idx)
  {
    float o = log10(idx+1);
    TPT_DBG(assert(o >= 0.));
    size_t i = o * files_per_log;
    TPT_DBG(if (i > files.size())
              cerr << ewarn << "Oops, for idx = " << idx << ", i > " << files.size()
                   << "; o = " << o << " i = " << i << endl);
    if (i >= files.size())
      {
        if (i == files.size())
          --i;  // handle potential rounding glitch
        else
          files.resize(i+1);
      }
    if (!files[i])
      {
	ostringstream buf; buf.fill('0');
	buf << order << "grams." << tag << "." << setw(4) << i;
        fileNames.push_back(buf.str());
	files[i] = new ofstream(buf.str().c_str());
	if (files[i]->fail())
	   cerr << "Error: Unable to open " << buf << " for writing." << exit_1;
	files[i]->put(uchar(order));
      }
    return *(files[i]);
  }

  vector<string> const& 
  getFileNames()
  {
    return fileNames;
  }
};

static const char * const fmt_error = "Format error in plaintext arpa LM file at line ";

/**
 * Read the header and n-gram count specification from an arpa LM file.
 * Return vector of n-gram counts in numNg.
 */
void
read_arpalm_ngram_counts(istream& arpalm_in, size_t& lctr, vector<size_t>& numNg)
{
  string line;
  line.reserve(40);
  do {
    if (!getline(arpalm_in, line))
      cerr << efatal << fmt_error << lctr << ": " << endl
           << "Premature EOF. '\\data\\' not found." << exit_1;
    lctr++;
  } while (line.compare(0, 6, "\\data\\") != 0);

  do {
    if (!getline(arpalm_in, line))
      cerr << efatal << fmt_error << lctr << ": " << endl
           << "Premature EOF. '\\1-grams:' not found." << exit_1;
    lctr++;
    if (line.compare(0, 5, "ngram") == 0)
      {
        int ngsize, ngcount;
        if (sscanf(line.c_str()+6, "%d=%d", &ngsize, &ngcount) != 2)
          cerr << efatal << fmt_error << lctr << ": " << endl
               << "Invalid ngram count specification."
               << exit_1;
        TPT_DBG(cerr << "ngsize=" << ngsize << " ngcount=" << ngcount << endl);
        numNg.push_back(ngcount);
        if (ngsize != int(numNg.size()))
          cerr << efatal << fmt_error << lctr << ": " << endl
               << "Out of order ngram count specification." << exit_1;
      }
  } while (line.compare(0, 9, "\\1-grams:") != 0);

  TPT_DBG(cerr << "# ngram counts: " << numNg.size() << endl);
  if (numNg.size() == 0)
    cerr << efatal << fmt_error << lctr << ": " << endl
         << "Ngram count specification not found: no counts per ngram-length."
         << exit_1;
}

/**
 * Read an n-gram line from an arpa LM file.
 * Return the probability score in lscore, the presence of a
 * backoff probability in has_rscore, the backoff probability in rscore,
 * and the vector of words for the ngram.
 */
void
read_arpalm_ngram(istream& arpalm_in, size_t order, size_t& lctr, float& lscore,
                  bool &has_rscore, float& rscore, vector<string>& words)
{
  string line;
  line.reserve(80);
  do {
    if (!getline(arpalm_in, line))
      cerr << efatal << fmt_error << lctr << ": " << endl
           << "Premature EOF." << exit_1;
    lctr++;
  } while (line.find_first_not_of(" \t") == string::npos);

  char *next;
  lscore = strtof(line.c_str(), &next);
  if (next == line.c_str())
    cerr << efatal << fmt_error << lctr << exit_1;

  words.resize(order);
  size_t q = next - line.c_str();
  for (size_t k = 0; k < order; k++)
    {
      size_t p = line.find_first_not_of(" \t", q);
      if (p == string::npos)
        cerr << efatal << fmt_error << lctr << ": " << endl
             << "Reading word " << k+1 << endl
             << "<" << line << ">"
             << exit_1;
      q = line.find_first_of(" \t", p);
      words[k].assign(line, p, q-p);
    }
  // Attempt to read the backoff prob.
  has_rscore = false;
  if (q != string::npos)
    {
      rscore = strtof(line.c_str()+q, &next);
      has_rscore = next != line.c_str()+q;
      q = next - line.c_str();
    }
  // At this point there shouldn't be anything left in the buffer,
  // but we want to ignore any extra whitespace.
  if (line.find_first_not_of(" \t", q) != string::npos)
    cerr << efatal << fmt_error << lctr << ": " << endl
         << "Invalid line format, some words left in buffer" << endl
         << "[" << &line[q] << "]" << endl
         << "<" << line << ">"
         << exit_1;
}

/**
 * Skip to the start of the next N-grams section in the arpa LM file,
 * i.e., read until the next "\N-grams:" or "\end" line is read.
 */
void
read_arpalm_ngram_header(istream& arpalm_in, size_t order, size_t max_order, size_t& lctr)
{
  // Read next "\N-grams:" or "\end\" line, where N is order.
  string line;
  line.reserve(40);
  char look_for[12];
  sprintf(look_for, order<=max_order ? "\\%d-grams:" : "\\end\\", (int)order);
  int look_for_length = strlen(look_for);
  do {
    if (!getline(arpalm_in, line))
      cerr << efatal << fmt_error << lctr << ": " << endl
           << "Premature EOF. '" << look_for << "' not found." << exit_1;
    lctr++;
  } while (line.empty() || line.compare(0, look_for_length, look_for) != 0);
}


#if USE_HASH
typedef hash_map<string,size_t> word_cnt_map_t;
#else
typedef tr1::unordered_map<string,size_t> word_cnt_map_t;
#endif

typedef tr1::unordered_map<float,size_t> value_cnt_map_t;

/** converts a map of value counts to a sorted vector of count-value pairs */
vector<pair<size_t,float> >
vmap2svec(value_cnt_map_t const& M)
{
  vector<pair<size_t,float> > retval(M.size());
  size_t z=0;
  for (value_cnt_map_t::const_iterator m = M.begin(); m != M.end(); m++)
    retval[z++] = pair<size_t,float>(m->second,m->first);
  sort(retval.begin(),retval.end(),greater<pair<size_t,float> >());
  return retval;
}

/** collects word and value counts from a language model in arpa text format */
void
collectCounts(string ifile,
              word_cnt_map_t& wCnt,
              vector<vector<pair<size_t,float> > >& pvals,
              vector<vector<pair<size_t,float> > >& bvals)
{
  vector<value_cnt_map_t> P,B; // p.-vals, b.o.-weights

  // open input
#if IN_PORTAGE
  iSafeMagicStream arpalm_in(ifile);
#else
  filtering_istream arpalm_in;
  open_input_stream(ifile,arpalm_in);
#endif

  // read header information (get number of ngrams for each ngram length)
  vector<size_t> numNg;
  size_t lctr=0;
  read_arpalm_ngram_counts(arpalm_in, lctr, numNg);

  P.resize(numNg.size());
  B.resize(numNg.size()-1);

  // now read ngram information
  for (size_t n = 0; n < numNg.size(); n++)
    {
      cerr << "Collecting word and value counts: reading "
           << n+1 << "-grams" << endl;
      time_t start_time(time(NULL));
      float lscore; bool has_rscore; float rscore;
      vector<string> w(n+1);
      for (size_t i = 0; i < numNg[n]; i++)
	{
          read_arpalm_ngram(arpalm_in, n+1, lctr, lscore, has_rscore, rscore, w);
          P[n][truncate(lscore)]++;
          for (size_t k = 0; k <= n; k++)
            wCnt[w[k]]++;
          // Handle backoff prob, if present.
          if (has_rscore)
            B[n][truncate(rscore)]++;
	}
      if (n < B.size())
        B[n][0.0] += 0; // make sure there's a value zero available for back-off w.
      // Read next '\N-grams:' or '\end\' line.
      read_arpalm_ngram_header(arpalm_in, n+2, numNg.size(), lctr);
      cerr << "Reading " << n+1 << "-grams took " << (time(NULL) - start_time)
           << " seconds." << endl;
    }

  // SORT VALUES BY DECREASING FREQUENCY
  cerr << "Sorting counts." << endl;
  pvals.reserve(P.size());
  for (size_t i = 0; i < P.size(); i++)
    pvals.push_back(vmap2svec(P[i]));
  bvals.reserve(B.size());
  for (size_t i = 0; i < B.size(); i++)
    bvals.push_back(vmap2svec(B[i]));
  return;
}

class valComp
{
public:
  bool
  operator()(pair<float,id_type> const& A, float const& B) const
  {
    return A.first < B;
  }

  bool
  operator()(float const& A, pair<float,id_type> const& B) const
  {
    return A < B.first;
  }

  bool
  operator()(pair<float,id_type> const& A, pair<float,id_type> const& B) 
  {
    return A.first < B.first;
  }
};

id_type 
valueId(vector<pair<float,id_type> > const& vec, float const& val)
{
  assert(vec.size());
  vector<pair<float,id_type> >::const_iterator v;
  v = upper_bound(vec.begin(),vec.end(),val,valComp());
  if (v == vec.begin())
    return v->second;
  if (v == vec.end())
    return vec.back().second;
  assert((v-1)->first <= val && val < v->first);
  return ((val - (v-1)->first) < (v->first -val)
          ? (v-1)->second
          : v->second);
}

/** writes a set of files (created dynamically via the filePool instances
 *  pfiles and bfiles) encoding the ngram information for fast parallel sorting
 *  (arpalm.sort-ng) and subsequent merging (arpalm.assemble) into a tightly packet trie
 *  representation.
 * @param file  name of the language model file in arpa format
 * @param tidx  name of the TokenIndex mapping from strings to word Ids
 * @param pvals vector of vectors of ngram probability value -id pairs,
 *        sorted in ascending order of probability values
 * @param bvals vector of vectors of back-off weight value-id pairs,
 *        sorted in ascending order of back-off weights
 * @returns a vector of unigram probability Ids
 */
vector<id_type>
encodeData(string file, TokenIndex& tidx,
           vector<vector<pair<float,id_type> > >& pvals,
           vector<vector<pair<float,id_type> > >& bvals)
{
  id_type num_tokens = tidx.getNumTokens();
  vector<id_type> uniprobIds(num_tokens); // return value

  // open input stream
#if IN_PORTAGE
  iSafeMagicStream arpalm_in(file);
#else
  filtering_istream arpalm_in;
  open_input_stream(file,arpalm_in);
#endif

  // read header information (get number of ngrams for each ngram length)
  vector<size_t> numNg;
  size_t lctr=0;
  read_arpalm_ngram_counts(arpalm_in, lctr, numNg);

  ofstream sortJobs("sng-av.jobs"); // file that contains all the sort jobs
  if (sortJobs.fail())
    cerr << efatal << "Unable to open sng-av.jobs for writing." << exit_1;

  // now read ngram information
  for (size_t n = 0; n < numNg.size(); n++)
    {
      cerr << "Encoding ngram data " << n+1 << "-grams" << endl;
      time_t start_time(time(NULL));
      filePool pfiles("p", n+1, num_tokens, numNg);
      filePool bfiles("bo", n+1, num_tokens, numNg);
      id_type ngram[n+1];
      float lscore; bool has_rscore; float rscore;
      vector<string> w;
      w.reserve(n+1);
      for (size_t i = 0; i < numNg[n]; i++)
        {
          read_arpalm_ngram(arpalm_in, n+1, lctr, lscore, has_rscore, rscore, w);
          id_type pvalId = valueId(pvals[n], lscore);
          for (size_t k = 0; k <= n; k++)
             ngram[k] = tidx[w[k]];
          if (n)
            {
              ostream& pout = pfiles[ngram[n-1]];
              for (int k = n-1; k >=0; k--)
                pout.write(reinterpret_cast<char*>(&(ngram[k])),sizeof(id_type));
              pout.write(reinterpret_cast<char*>(&(ngram[n])),sizeof(id_type));
              pout.write(reinterpret_cast<char*>(&pvalId),sizeof(id_type));
              TPT_DBG2(cerr << "n=" << n << " * ";
                       for (int k = n-1; k >=0; k--)
                         cerr << ngram[k] << " ";
                       cerr << ngram[n] << " " << pvalId << endl);
            }
          else
            uniprobIds[ngram[0]] = pvalId;
          if (has_rscore)
            {
              id_type bowId = valueId(bvals[n],rscore);
              ostream& bout = bfiles[ngram[n]];
              for (int k = n; k >=0; k--)
                bout.write(reinterpret_cast<char*>(&ngram[k]),sizeof(id_type));
              bout.write(reinterpret_cast<char*>(&bowId),sizeof(id_type));
            }
        }
      read_arpalm_ngram_header(arpalm_in, n+2, numNg.size(), lctr);

#if 0
      vector<string> const& pfn = pfiles.getFileNames();
      for (size_t i = 0; i < pfn.size(); i++)
          sortJobs << "arpalm.sng-av " << pfn[i] << endl;
#endif
      vector<string> const& bfn = bfiles.getFileNames();
      for (size_t i = 0; i < bfn.size(); i++)
          sortJobs << "arpalm.sng-av " << bfn[i] << endl;
      cerr << "Encoding " << n+1 << "-grams took " << (time(NULL) - start_time)
           << " seconds." << endl;
    }
  return uniprobIds;
}

void write_codebook(string cbkFile, vector<id_type> const& uniprobs,
                    vector<vector<pair<size_t,float> > > const& pvals,
                    vector<vector<pair<size_t,float> > > const& bvals)
{
  size_t maxNg = pvals.size();
  // write codebook and unigram probabilities
  ofstream cb(cbkFile.c_str());
  if (cb.fail())
    cerr << efatal << "Unable to open codebook file '" << cbkFile << "' for writing."
         << exit_1;
  cb.put(0);                      // adjustment to map from wids to index positions
  binwrite(cb,uniprobs.size());   // number of vocab items
  cb.put(uchar(pvals.size()));    // max length of ngrams stored

  // record sizes of probability codebooks
  for (size_t i = 0; i < maxNg; i++)
    binwrite(cb,pvals[i].size());  

  // record sizes of backoff weight codebooks
  for (size_t i = 0; i < bvals.size(); i++)
    binwrite(cb,bvals[i].size()); 

  // write probability codebooks
  for (size_t i = 0; i < maxNg; i++)
    for (size_t k = 0; k < pvals[i].size(); k++)
      cb.write(reinterpret_cast<char const*>(&(pvals[i][k].second)), sizeof(float));

  // write backoff codebooks
  for (size_t i = 0; i < bvals.size(); i++)
    for (size_t k = 0; k < bvals[i].size(); k++)
      cb.write(reinterpret_cast<char const*>(&(bvals[i][k].second)), sizeof(float));

  // write unigram probabilities
  for (size_t i = 0; i < uniprobs.size(); i++)
    cb.write(reinterpret_cast<char const*>(&(uniprobs[i])),sizeof(id_type));
  
  cb.close();

#if 0
  // for debugging
  // write probability codebooks
  for (size_t i = 0; i < maxNg; i++)
    {
      ostringstream buf;
      buf << "codebook.p." << i+1;
      ofstream out(buf.str().c_str());
      for (size_t k = 0; k < pvals[i].size(); k++)
        out << k << " " 
            << pvals[i][k].second << " " 
            << pvals[i][k].first 
            << endl;
    }

  for (size_t i = 0; i < bvals.size(); i++)
    {
      ostringstream buf;
      buf << "codebook.b." << i+1;
      ofstream out(buf.str().c_str());
      for (size_t k = 0; k < bvals[i].size(); k++)
        out << k << " " << bvals[i][k].second << " " 
            << bvals[i][k].first 
            << endl;
    }
#endif

}

/** convert vector of count-value pairs to a vector of value-id pairs
 *  sorted in ascending value order */
vector<pair<float,id_type> >
valCounts2valIdMap(vector<pair<size_t,float> > const& vec)
{
  vector<pair<float,id_type> > ret(vec.size());
  for (size_t i = 0; i < vec.size(); i++)
    ret[i] = pair<float,id_type>(vec[i].second,i);
  sort(ret.begin(),ret.end(),valComp());
  return ret;
}

int MAIN(argc, argv)
{
  cerr << "starting arpalm.encode" << endl;
  interpret_args(argc, (char **)argv);

  // probability values for each n-gram order 
  // sorted in descending order of frequency
  vector<vector<pair<size_t,float> > > pvalCounts; 

  // back-off values for each n-gram order
  // sorted in descending order of frequency
  vector<vector<pair<size_t,float> > > bvalCounts;

  // word counts
  word_cnt_map_t wCnt;

  // collect counts for words and values
  collectCounts(lmFile,wCnt,pvalCounts,bvalCounts);
  TPT_DBG(cerr << "number of tokens (wCnt size): " << wCnt.size() << endl);

  // write the token index to disk and open it for encoding the data
  cerr << "Writing token index." << endl;
  mkTokenIndex(tidxFile,wCnt,unkToken);
  TokenIndex tidx(tidxFile,unkToken);

  cerr << "Creating value maps." << endl;
  vector<vector<pair<float,id_type> > > pvalMap(pvalCounts.size());
  vector<vector<pair<float,id_type> > > bowMap(bvalCounts.size());
  for (size_t i = 0; i < pvalMap.size(); i++)
    pvalMap[i] = valCounts2valIdMap(pvalCounts[i]);
  for (size_t i = 0; i < bowMap.size(); i++)
    bowMap[i] = valCounts2valIdMap(bvalCounts[i]);

  vector<id_type> uniprobIds = encodeData(lmFile,tidx,pvalMap,bowMap);

  // write the code book
  cerr << "Writing codebook." << endl;
  write_codebook(cbkFile,uniprobIds,pvalCounts,bvalCounts);

  // write a file with the IDs of zero back-off weights 
  // (needed if back-off weights of zero are omitted in the lm text file)
  cerr << "Writing bowzero file." << endl;
  ofstream bzero("bowzero");
  if (bzero.fail())
    cerr << efatal << "Unable to open bowzero for writing." << exit_1;
  for (size_t x = 0; x < bowMap.size(); x++)
    bzero << valueId(bowMap[x],0.0) << " ";
  bzero << endl;
  cerr << "done arpalm.encode" << endl;
}
END_MAIN

