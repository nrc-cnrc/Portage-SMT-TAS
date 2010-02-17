// (c) 2007-2009 Ulrich Germann. All rights reserved.
// Licensed to NRC under special agreement.
/**
 * @author Ulrich Germann
 * @file mmctrack.dump.cc
 * @brief Dumps memory mapped corpus track file (.mct) in plain text format to stdout.
 */
#include "tpt_typedefs.h"
#include "ug_mm_ctrack.h"
#include "tpt_tokenindex.h"

#include <boost/program_options.hpp>

using namespace std;
using namespace ugdiss;
namespace po = boost::program_options;

static char base_help_message[] = "\n\
mmctrack.dump [options] TOKEN_INDEX_FILE MMCTRACK_FILE [RANGE1...RANGEN]\n\
\n\
  Dump a binary memory mapped corpus track file MMCTRACK_FILE (.mct) to stdout\n\
  as plain text.\n\
  TOKEN_INDEX_FILE (.tdx) is the corresponding token index for the corpus.\n\
  Each RANGEi is a 0-based range in the form N1[-N2] of sentences to dump.\n\
\n\
";

static stringstream options_help;       // set by interpret_args()

inline ostream& help_message(ostream& os)
{
  return os << base_help_message << options_help.str();
}

string tdxFile,mctFile;
vector<string> range;
bool withNumbers;

TokenIndex V;
mmCtrack   C;

void 
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",    "print this message")
    ("numbers,n", "print sentence ids as first token")
    ;
  options_help << o;
  
  po::options_description h("Hidden Options");
  h.add_options()
    ("tdx", po::value<string>(&tdxFile), "token index file (.tdx)")
    ("mct", po::value<string>(&mctFile), "memory mapped corpus track file (.mct)")
    ("range", po::value<vector<string> >(&range), "range (0-n)*")
    ;

  po::positional_options_description a;
  a.add("tdx",1);
  a.add("mct",1);
  a.add("range",-1);
  

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

  if (tdxFile.empty())
    cerr << efatal << "Token index TOKEN_INDEX_FILE (.tdx) required." << endl
         << help_message << exit_1;

  if (mctFile.empty())
    cerr << efatal << "Memory mapped corpus track MMCTRACK_FILE (.mct) required." << endl
         << help_message << exit_1;

  withNumbers = vm.count("numbers");
}

void 
printRange(size_t start, size_t stop)
{
  for (;start < stop; start++)
    {
      if (withNumbers) cout << start << " ";
      cout << C.str(start,V) << endl;
    }
}

int MAIN(argc, argv)
{
  interpret_args(argc, (char **)argv);
  V.open(tdxFile); V.iniReverseIndex();
  C.open(mctFile);
  if (!range.size())
    printRange(0,C.size());
  else
    {
      for (size_t i = 0; i < range.size(); i++)
        {
          istringstream buf(range[i]);
          size_t first,last; uchar c;
          buf>>first;
          if (buf.peek() == '-')
            buf>>c>>last;
          else
            last=first;
          if (last >= C.size())
             last = C.size() - 1;
          if (buf.eof() && first <= last)
            printRange(first,last+1);
          else
            cerr << ewarn << "Invalid range '" << range[i] << "'." << endl;
        }
    }
}
END_MAIN
