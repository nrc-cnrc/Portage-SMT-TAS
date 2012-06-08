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


// (c) 2006,2007,2008 Ulrich Germann
/**
 * @author Ulrich Germann
 * @file ptable.encode-phrases.cc
 * @brief Preparatory step in converting phrase tables to TPPT: 
 * encodes L1 and L2 phrases.
 *
 * First step in textpt2tppt conversion. Normally called via textpt2tppt.sh.
 */
#include <map>
#include <tr1/unordered_map>
#include <ctime>
#include <sstream>

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
#include "tpt_repos.h"
#include "tpt_utils.h"

// COLSEPSTRING is WITH surrounding spaces, COLSEPWORD without
#define COLSEPSTRING " ||| "
#define COLSEPWORD    "|||"

#if IN_PORTAGE
#define MAX_COLUMN 2
#else
#define MAX_COLUMN 3
#endif

static char base_help_message[] = "\n\
ptable.encode-phrases [options] [-i] TEXTPT_FILE [-c] COL [-o] OUTPUT_BASE_NAME\n\
\n\
  Encode L1 and L2 phrases from a text phrase table file TEXTPT_FILE.\n\
  COL is the column to convert: 1 for L1 'src' phrases, 2 for L2 'trg' phrases.\n\
\n\
  The following files for the language ('src' or 'trg') are produced:\n\
  OUTPUT_BASE_NAME.<src|trg>.tdx contains the token index for the language.\n\
  OUTPUT_BASE_NAME.<src|trg>.repos.idx holds the sequence repository index.\n\
  OUTPUT_BASE_NAME.<src|trg>.repos.dat holds the sequence repository data.\n\
  OUTPUT_BASE_NAME.<src|trg>.col maps phrase table line #'s to sequence ids.\n\
\n\
  Some of the above files are part of the final TPPT model, while some are\n\
  intermediate files that must undergo additional processing.\n\
\n\
  This program is usually run for each language.\n\
\n\
  This is the first step in the conversion of text phrase tables to tightly\n\
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

string iFileName, oBaseName;
int mode;

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",  "print this message")
    ("quiet,q", "don't print progress information")
    ("input,i", po::value<string>(&iFileName), "input file")
    ("col,c", po::value<int>(&mode)->default_value(0),
#if IN_PORTAGE
     "column to process (1=L1, 2=L2)")
#else
     "column(s) to process (1=L1, 2=L2, 3=Aln)")
#endif
    ("output,o", po::value<string>(&oBaseName),
     "base name for output files")
    ;
  options_help << o;
  po::positional_options_description a;
  a.add("input",1);
  a.add("col",1);
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

  if (!mode)
    cerr << efatal << "Non-zero COLUMN required." << endl << help_message << exit_1;
  if (mode < 1 || mode > MAX_COLUMN)
    cerr << efatal << "Invalid COLUMN (must be in range 1-" << MAX_COLUMN << ")."
         << endl << help_message
         << exit_1;

  if (!oBaseName.size())
    cerr << efatal << "OUTPUT_BASE_NAME required." << endl << help_message
         << exit_1;
}

class Word
{
public:
  uint32_t id;
  uint32_t count;
  Word() : id(0),count(0){};
  Word(size_t _id) : id(_id),count(0){};
};

typedef tr1::unordered_map<string,Word> vocab;
//typedef map<string,Word> vocab;  // use to reproduce Uli's original output

/** Return preliminary word ID
 *  (will be remapped to a different ID later) 
 */
uint32_t 
wid(string const& w, vocab& V)
{
  vocab::value_type foo(w,Word(V.size()));     // zero-based ID needed by writeTokenIndex
  vocab::iterator m = V.insert(foo).first;
  m->second.count++;
  return m->second.id;
}

/** Process a line of the phrase table file, converting it to the
 *  fast vocabulary format and writing its word ids to a temporary file.
 */
void
process_line(string const& line, ostream& tmp, vocab& V, size_t linectr)
{
  // skip initial columns
  size_t p = 0;
  size_t CSSTRLEN = strlen(COLSEPSTRING);
  for (int i = 1; i < mode; i++)
    {
      p = line.find(COLSEPSTRING,p);
      if (p == string::npos)
        cerr << efatal << "Format error in phrase table file:" << endl
             << "Column separator '" << COLSEPSTRING << "' missing after column "
             << i << " in line " << linectr << endl << ">" << line << "<"
             << exit_1;
      p += CSSTRLEN;
    }

  // read sequence, assign preliminary ids
  size_t numCols = mode == 3 ? 2 : 1;
  for (size_t col = 0; col < numCols; col++)
    {
      vector<uint32_t> v;
      string w;
      size_t q = p;
      for (p = line.find_first_not_of(" \t", q); p != string::npos;
           p = line.find_first_not_of(" \t", q))
        {
          q = line.find_first_of(" \t", p);
          if (q == string::npos) break;
          w.assign(line, p, q-p);
          if (w == COLSEPWORD) break;
          v.push_back(wid(w, V));
        }

      if (p == string::npos || q == string::npos)
        cerr << efatal << "Format error in phrase table file:" << endl
             << "Column separator '" << COLSEPWORD << "' missing after column "
             << mode+col << " in line " << linectr << endl << ">" << line << "<"
             << exit_1;

      // store backwards except for first column
      if (mode > 1) reverse(v.begin(),v.end());

      uint32_t vsize = v.size();
      if (vsize == 0)
        cerr << efatal << "Format error in phrase table file:" << endl
             << "Empty phrase in line " << linectr << endl << ">" << line << "<"
             << exit_1;

      tmp.write(reinterpret_cast<char*>(&vsize),4);
      for (size_t i = 0; i < vsize; i++)
	tmp.write(reinterpret_cast<char*>(&v[i]),4);
    }
}

struct byFreq
{
  bool operator()(vocab::const_iterator const& A,
                  vocab::const_iterator const& B)
  {
    return A->second.count > B->second.count;
  }
};

struct byKey
{
  bool operator()(vocab::const_iterator const& A,
                  vocab::const_iterator const& B)
  {
    return A->first < B->first;
  }
};

/** Write the token index to disk, returning a mapping from preliminary
 *  token ids (based on order of occurrence) to final token IDs
 *  (based on frequency in decreasing order).
 */
void
writeTokenIndex(string ofname, vocab& V, vector<uint32_t> &retval)
{
  TPT_DBG(cerr << "writeTokenIndex: ofname=" << ofname << " V.size: " << V.size() << endl);
  retval.resize(V.size());
  vector<vocab::iterator> w;
  w.reserve(V.size());
  for (vocab::iterator m = V.begin(); m != V.end(); m++) {
    w.push_back(m);
  }

  // sort tokens in decreasing order of frequency
  sort(w.begin(), w.end(), byFreq());

  // assign new ids, record mapping from old ids to new ones
  // assumes 0-based ids.
  TPT_DBG(cerr << "by freq:" << endl);
  for (size_t i = 0; i < w.size(); i++)
    {
      w[i]->second.id = (retval[w[i]->second.id] = i);
      TPT_DBG(if (i < 20)
                cerr << "w[" << i << "]: '" << w[i]->first << "', "  << w[i]->second.id
                     << ", " << w[i]->second.count << endl);
    }

  ofstream out(ofname.c_str());
  if (out.fail())
    cerr << efatal << "Unable to open token index file '" << ofname << "' for writing."
         << exit_1;

  uint32_t vsize = V.size();
  out.write(reinterpret_cast<char*>(&vsize),4); // # of tokens
  out.write(reinterpret_cast<char*>(&vsize),4); // UNK id
  ostringstream data;
  // sort tokens alphabetically
  sort(w.begin(), w.end(), byKey());
  TPT_DBG(cerr << "alphabetically:" << endl);
  for (uint32_t i = 0; i < w.size(); ++i)
    {
      vocab::iterator m = w[i];
      TPT_DBG(if (i < 20)
                cerr << "w[" << i << "]: '" << m->first << "', "  << m->second.id << endl);
      // first field in index array: offset from start of data block
      uint32_t offset = data.tellp();
      char* p = reinterpret_cast<char*>(&offset);
      out.write(p,4);
      // second field: the token ID
      char* q = reinterpret_cast<char*>(&(m->second.id));
      out.write(q,4);
      // write null-terminated string to data buffer
      data << m->first << char(0);
    }
  out << data.str(); // append data block to index; done
}

/** Build an in-memory reverse trie of sequences of phrase IDs.
 */
void
build_repos_trie(string vfname, size_t linectr, vector<uint32_t> &remap,
                 tp_trie_t& rev_trie, vector<uint32_t>& pids, uint32_t& max_pid)
{
   bio::mapped_file_source dat;
   open_mapped_file_source(dat, vfname);
   const uint32_t* p = reinterpret_cast<const uint32_t*>(dat.data());
   pids.reserve(linectr);

   max_pid = 0; // Note: preliminary phrase ids start at 1
   for (size_t k = 0; k < linectr; k++)
     {
       TPT_DBG(cerr << "\nmain: k = " << k << ", p = 0x" << hex << (uint64_t)p << dec);
       size_t vsize = *p++;
       if (vsize == 0)
         cerr << efatal << "Failed processing phrase # " << k+1 << "/" << linectr << endl
              << "Encountered phrase length of 0 in temporary file '" << vfname << "'."
              << exit_1;
       TPT_DBG(cerr << ", vsize = " << vsize << ", *p = " << (*p)
                    << ", remap[*p] = " << remap[*p] << endl);

#ifdef USE_PTRIE
       uint32_t tids[vsize];
       for (uint32_t i=0; i < vsize; ++i)
          tids[i] = remap[*p++];
       uint32_t *val_ptr;
       if (rev_trie.find_or_insert(tids, vsize, val_ptr)) {
          TPT_DBG(cerr << "found val_ptr = 0x" << hex << val_ptr << dec
                       << " *val_ptr = " << *val_ptr << endl);
          pids.push_back(*val_ptr);
       } else {
          uint32_t *val_ptrs[vsize];
          uint32_t insert_pids[vsize];
          rev_trie.find_path(tids, vsize, val_ptrs);
          for (uint32_t i=0; i < vsize-1; ++i) {
             insert_pids[i] = val_ptrs[i] ? 0 : ++max_pid;  // preliminary phrase ids start at 1
             TPT_DBG(if (!insert_pids[i])
                       cerr << "val_ptrs[" << i << "] = 0x" << hex
                            << val_ptrs[i] << dec << " *val_ptrs[" << i << "] = "
                            << *val_ptrs[i] << endl);
          }
          *val_ptrs[vsize-1] = ++max_pid; // preliminary phrase ids start at 1
          TPT_DBG(cerr << "val_ptrs[" << (vsize-1) << "] = 0x"
                       << hex << val_ptrs[vsize-1] << dec
                       << " set *val_ptrs[" << vsize-1 << "] = "
                       << *val_ptrs[vsize-1] << endl);
          pids.push_back(max_pid);
          for (uint32_t i=0; i < vsize-1; ++i) {
             if (insert_pids[i]) {
                rev_trie.insert(tids, i+1, insert_pids[i], &val_ptr);
                TPT_DBG(cerr << "i = " << i << ": inserted val_ptr = 0x" << hex
                             << val_ptr << dec << " *val_ptr = " << *val_ptr << endl);
             }
          }
       }
 #else
       const uint32_t* stop = p+vsize;
       MemTreeNode<uint32_t>* n = rev_trie.get(remap[*p],true);
       if (!n->val)
         n->val = ++max_pid; // preliminary phrase ids start at 1
       TPT_DBG(cerr << "    n = 0x" << hex << (uint64_t)n << dec << ", n->id = "
                    << n->id << ", n->val = " << n->val << endl);

       while (++p != stop)
         {
           n = n->get(remap[*p],true);
           TPT_DBG(cerr << "  main: p = 0x" << hex << (uint64_t)p << dec
                        << ", *p = "<< (*p) << ", remap[*p] = " << remap[*p] << endl);
           if (!n->val)
             n->val = ++max_pid;
           TPT_DBG(cerr << "    n = 0x" << hex << (uint64_t)n << dec << ", n->id = "
                        << n->id << ", n->val = " << n->val << endl);
         }
       pids.push_back(n->val);
 #endif

       TPT_DBG(cerr << "pids.push_back: " << pids.back() << endl);
     }

//   sleep(5);           // maximum space used at this point
   dat.close();
}

int MAIN(argc, argv)
{
  cerr << "ptable.encode-phrases starting." << endl;

  interpret_args(argc, (char **)argv);
  string coltag = (mode == 1 ? ".src" : mode == 2 ? ".trg" : ".aln");
  string line, w;
#if IN_PORTAGE
  iSafeMagicStream in(iFileName);
#else
  filtering_istream in;
  open_input_stream(iFileName,in);
#endif

  // Step 1: Count words, and write a tmp file that is fast to process later.
  cerr << "Converting to a fast vocabulary format." << endl;
  vocab* V(new vocab);
  size_t linectr = 0;
  const string tmpName = oBaseName+coltag+".tmp";
  ofstream tmpFile(tmpName.c_str());
  if (tmpFile.fail())
    cerr << efatal << "Unable to open temporary file '" << tmpName << "' for writing."
         << exit_1;
  time_t start_time(time(NULL));
  while (getline(in,line))
    {
      if (++linectr%1000000 == 0) cerr << ".";
      if (linectr%10000000 == 0)
        cerr << "Counting tokens: " << linectr/1000000 << "M lines read (..."
             << (time(NULL) - start_time) << "s)" << endl;
        process_line(line, tmpFile, *V, linectr);
    }
  tmpFile.close();
  cerr << "Read " << linectr << " lines in " << (time(NULL) - start_time) << " seconds." << endl;

  // Step 2: Sort words in order of descending frequency and assign new IDs;
  //         remap stores mapping from old preliminary IDs to new IDs.
  cerr << "Writing token index." << endl;
  const string tdxName = oBaseName+coltag+".tdx";
  vector<uint32_t> remap;
  writeTokenIndex(tdxName, *V, remap);
  delete V;    // vocabulary is no longer needed.

  // Step 3: Build an in-memory trie of sequences.
  cerr << "Building in-memory trie." << endl;
  uint32_t highest_pid=0;       // highest preliminary phrase ID
#ifdef USE_PTRIE
  tp_trie_t rev_trie(10); // reverse trie mapping to preliminary phrase IDs
#else
  tp_trie_t rev_trie;     // reverse trie mapping to preliminary phrase IDs
#endif
  vector<uint32_t> pids;
  build_repos_trie(tmpName, linectr, remap, rev_trie, pids, highest_pid);
  //remove(tmpName.c_str()); // not needed any more

  // Step 4: Write out the trie of sequences to disk.
  cerr << "Writing repository." << endl;
  string reposName(oBaseName + coltag + ".repos");
  vector<uint32_t> pidmap = toRepos(reposName, rev_trie, remap.size(), highest_pid);

  cerr << "Writing pid map (.col file)." << endl;
  string colName(oBaseName+coltag+".col");
  ofstream out(colName.c_str());
  if (out.fail())
    cerr << efatal << "Unable to open file '" << colName << "' for writing."
         << exit_1;
  for (size_t i = 0; i < pids.size(); i++)
    out.write(reinterpret_cast<char*>(&pidmap[pids[i]]),4);
  out.close();

  cerr << "ptable.encode-phrases finished." << endl;
}
END_MAIN
