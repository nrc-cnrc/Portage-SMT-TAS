// (c) 2006,2007,2008 Ulrich Germann
// Licensed to NRC-CNRC under special agreement.
/**
 * @author Ulrich Germann
 * @file ptable.encode-phrases.cc
 * @brief Preparatory step in converting phrase tables to TPPT: 
 * encodes L1 and L2 phrases.
 *
 * First step in textpt2tppt conversion. Normally called via textpt2tppt.sh.
 */
#include <map>
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
#include "ugMemTable.h"
#include "tpt_utils.h"

#include "arpa_ctype.h"

#define COLSEPSTRING " ||| "
#define COLSEPWORD    "|||"
// COLSEPSTRING is WITH surrounding spaces, COLSEPWORD without

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

class Word
{
public:
  uint32_t id;
  uint32_t count;
  Word() : id(0),count(0){};
  Word(size_t _id) : id(_id),count(0){};
};

#if 0
////////////////////////////////////////////////////////////////////////////
// requires boost 1.36; not sure if this makes things faster;
// we'd also have to sort the vocab items alphabetically before
// writing the token index
#include <boost/unordered_map.hpp>
typedef boost::unordered_map<string,Word>               vocab;
typedef boost::unordered_map<vector<uint32_t>,uint32_t> repos;
////////////////////////////////////////////////////////////////////////////
#else
////////////////////////////////////////////////////////////////////////////
typedef map<string,Word>               vocab;
typedef map<vector<uint32_t>,uint32_t> repos;
////////////////////////////////////////////////////////////////////////////
#endif

#define rcast reinterpret_cast

namespace po  = boost::program_options;
namespace bio = boost::iostreams;

using namespace std;
using namespace ugdiss;

string iFileName, oBaseName;
int mode;

// Arpa format says that a record is space or tab separated.
static std::locale arpa_space(std::locale::classic(), new arpa_ctype);

class cmpWrd
{
public:
  bool operator()(Word const* A, Word const* B) const
  {
    return A->count > B->count;
  }
};

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

/** Write the token index to disk, returning a mapping from preliminary
 *  token ids (based on order of occurrence) to final token IDs
 *  (based on frequency in decreasing order).
 */
vector<uint32_t>
writeTokenIndex(string ofname, vocab& V)
{
  vector<uint32_t> retval(V.size());
  vector<Word*> w;
  w.reserve(V.size());
  for (vocab::iterator m = V.begin(); m != V.end(); m++)
    w.push_back(&(m->second));

  // sort tokens in decreasing order of frequency
  sort(w.begin(),w.end(),cmpWrd());

  // assign new ids, record mapping from old ids to new ones
  // assumes 0-based ids.
  for (size_t i = 0; i < w.size(); i++)
    w[i]->id = (retval[w[i]->id] = i);

  ofstream out(ofname.c_str());
  if (out.fail())
    cerr << efatal << "Unable to open token index file '" << ofname << "' for writing."
         << exit_1;

  uint32_t vsize = V.size();
  out.write(reinterpret_cast<char*>(&vsize),4); // # of tokens
  out.write(reinterpret_cast<char*>(&vsize),4); // UNK id 
  ostringstream data;
  for (vocab::iterator m = V.begin(); m != V.end(); m++)
    {
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
  return retval;
}

#if 0
class phraseComp
{
public:
  size_t depth;
  phraseComp() : depth(0) {};
  bool operator()(unsigned char const* A, 
                  unsigned char const* B) const
  {
    size_t    asize = *A; 
    size_t    bsize = *B; 
    uint32_t const* a = reinterpret_cast<uint32_t const*>(A+1);
    uint32_t const* b = reinterpret_cast<uint32_t const*>(B+1);
    size_t    stop = depth ? min(depth,min(asize,bsize)) : min(asize,bsize);
    size_t i;
    for (i = 0; i < stop && a[i] == b[i]; i++);
    if (i < stop && (!depth || depth==asize || depth==bsize))
      return a[i] < b[i];
    // cout << asize << " " << bsize << " " << depth << endl;
    assert(depth < asize && depth < bsize);
    return asize < bsize;
  }
};

size_t 
count_columns(string const& line)
{
  string::size_t x = 0;
  size_t ret = 1;
  for (x = line.find(COLSEPSTRING,x); 
       x != string::npos; 
       x = line.find(COLSEPSTRING,x))
    ret++;
  return ret;
}
#endif

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
  istringstream buf(line.substr(p));
  buf.imbue(arpa_space);
  size_t numCols = mode == 3 ? 2 : 1;
  string w;
  for (size_t col = 0; col < numCols; col++)
    {
      vector<uint32_t> v;
      for (buf >> w; w != COLSEPWORD && !buf.eof(); buf >> w)
	v.push_back(wid(w,V));

      if (buf.eof())
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

      tmp.write(rcast<char*>(&vsize),4);
      for (size_t i = 0; i < vsize; i++)
	tmp.write(rcast<char*>(&v[i]),4);
    }
}

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

int MAIN(argc, argv)
{
  interpret_args(argc, (char **)argv);
  string coltag = (mode == 1 ? ".src" : mode == 2 ? ".trg" : ".aln");
  string line, w;
#if IN_PORTAGE
  iSafeMagicStream in(iFileName);
#else
  filtering_istream in;
  open_input_stream(iFileName,in);
#endif

  // Imbue the input stream to treat \v as been non-spaces.
  in.imbue(arpa_space);

  // Step 1: Count words, and write a tmp file that is fast to process later.
  cerr << "Converting to a fast vocabulary format." << endl;
  vocab V;
  size_t linectr = 0;
  const string tmpName = oBaseName+coltag+".tmp";
  ofstream tmpFile(tmpName.c_str());
  if (tmpFile.fail())
    cerr << efatal << "Unable to open temporary file '" << tmpName << "' for writing."
         << exit_1;
  while (getline(in,line))
    {
      if (++linectr%1000000 == 0)
        cerr << "Counting tokens: " << linectr/1000000 << "M lines read" << endl;
      process_line(line, tmpFile, V, linectr);
    }
  tmpFile.close();

  // Step 2: Sort words in order of descending frequency and assign new IDs;
  //         remap stores mapping from old preliminary IDs to new IDs.
  cerr << "Writing token index." << endl;
  const string tdxName = oBaseName+coltag+".tdx";
  vector<uint32_t> remap = writeTokenIndex(tdxName,V);

  // TokenIndex tdx(tdxName);   // for debugging

  // Step 3: Build an in-memory trie of sequences.
  uint32_t prelimPid=0;    // preliminary phrase ID
  MemTable<uint32_t> M;    // reverse trie mapping to preliminary phrase IDs
  vector<uint32_t> pids;
  bio::mapped_file_source dat;
  open_mapped_file_source(dat, tmpName);
  const uint32_t* p = reinterpret_cast<const uint32_t*>(dat.data());
  for (size_t k = 0; k < linectr; k++)
    {
      size_t vsize = *p++;
      if (vsize == 0)
        cerr << efatal << "Failed processing phrase # " << k+1 << "/" << linectr << endl
             << "Encountered phrase length of 0 in temporary file '" << tmpName << "'."
             << exit_1;
      const uint32_t* stop = p+vsize;
      MemTreeNode<uint32_t>* n = M.get(remap[*p],true);
      if (!n->val)
        n->val = ++prelimPid; // preliminary phrase ids start at 1
      while (++p != stop)
        {
          n = n->get(remap[*p],true);
          if (!n->val)
            n->val = ++prelimPid;
        }
      pids.push_back(n->val);
    }
  dat.close();
  remove(tmpName.c_str()); // not needed any more

  // Step 4: Write out the trie of sequences to disk.
  string reposName(oBaseName + coltag + ".repos");
  vector<uint32_t> pidmap = toRepos(reposName, M, remap.size(), prelimPid);

  string colName(oBaseName+coltag+".col");
  ofstream out(colName.c_str());
  if (out.fail())
    cerr << efatal << "Unable to open file '" << colName << "' for writing."
         << exit_1;
  for (size_t i = 0; i < pids.size(); i++)
    out.write(reinterpret_cast<char*>(&pidmap[pids[i]]),4);
  out.close();
}
END_MAIN
