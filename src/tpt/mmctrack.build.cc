// (c) 2007-2009 Ulrich Germann. All rights reserved.
// Licensed to NRC under special agreement.
/**
 * @author Ulrich Germann
 * @file mmctrack.build.cc
 * @brief Reads tokenized text from stdin and the TokenIndex file, then
 * writes a binary memory mapped corpus track file (.mct).
 *
 * Second step in mmsufa generation. Normally called via build-tp-suffix-array.sh.
 */

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/program_options.hpp>

#include "tpt_tokenindex.h"
#include "tpt_pickler.h"

using namespace std;
using namespace ugdiss;
namespace po=boost::program_options;

static char base_help_message[] = "\n\
mmctrack.build [options] TOKEN_INDEX_FILE MMCTRACK_FILE\n\
\n\
  Read tokenized text from stdin as well as the TOKEN_INDEX_FILE (.tdx),\n\
  then write the binary memory mapped corpus file MMCTRACK_FILE (.mct).\n\
\n\
  This is the second step in creating a memory mapped suffix array for a corpus.\n\
  This program is called from build-tp-suffix-array.sh.\n\
\n\
";

static stringstream options_help;       // set by interpret_args()

inline ostream& help_message(ostream& os)
{
  return os << base_help_message << options_help.str();
}

int toksepChar = ' ';
string toksep = " ";
string vocabFile;
string ctrackFile;
bool quiet=false; // not implemented yet

void interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",  "print this message")
    ("quiet,q", "don't print progress information")
#if 0
    // UNTESTED!
    ("toksep,t",po::value<int>(&toksepChar)->default_value(' '),
     "token separator (character code, not string!)")
#endif
    ;
  options_help << o;

  po::options_description h("Hidden Options");
  h.add_options()
    ("tdx", po::value<string>(&vocabFile),"token index file (.tdx)")
    ("mct", po::value<string>(&ctrackFile),"memory mapped corpus track file (.mct)")
    ;
  h.add(o);

  po::positional_options_description a;
  a.add("tdx",1);
  a.add("mct",1);
  
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

  if (vocabFile.empty())
    cerr << efatal << "TokenIndex file TOKEN_INDEX_FILE (.tdx) required." << endl
         << help_message << exit_1;

  if (ctrackFile.empty())
    cerr << efatal << "Output file for memory mapped corpus track MMCTRACK_FILE (.mct) required." << endl
         << help_message << exit_1;

  toksep[0] = char(toksepChar);
  if (vm.count("quiet"))
    quiet=true;
}

int MAIN(argc, argv)
{
  interpret_args(argc, (char **)argv);
  TokenIndex Tdx(vocabFile);

  // open output file and reserve spaces for index start position and index size.
  ofstream out(ctrackFile.c_str());
  if (out.fail())
    cerr << efatal << "Unable to open corpus track file '" << ctrackFile
         << "' for writing." << exit_1;
  filepos_type startIdx=0;
  id_type idxSize=0,totalWords=0;
  numwrite(out,startIdx);   // place holder, to be filled at the end
  numwrite(out,idxSize);    // place holder, to be filled at the end
  numwrite(out,totalWords); // place holder, to be filled at the end

  vector<id_type> index;
  string line;
  index.push_back(0);
  id_type unkId = Tdx.getUnkId();
  uint64_t unkCnt = 0;
  vector<id_type> snt;
  while (getline(cin,line))
    {
      Tdx.toIdSeq(snt, line);
      totalWords += snt.size();
      // sentence start was pushed on previous iteration.
      // push end for this sentence, which is also the start of the next one.
      index.push_back(totalWords);
      for (size_t i = 0; i < snt.size(); ++i)
        {
          if (snt[i] == unkId)
            ++unkCnt;
          numwrite(out,snt[i]);
        }
      // progress report
      if (!quiet && (index.size()-1)%10000==0)
        cerr << (index.size()-1)/1000 << "K lines processed" << endl;
    }

  if (!quiet)
    {
      cerr << (index.size()-1) << " lines processed in total." << endl;
      cerr << "Writing index ... " << endl;
    }
  startIdx = out.tellp();
  // when writing the index, include the end of the last sentence.
  for (size_t i = 0; i < index.size(); i++)
    numwrite(out,index[i]);
  out.seekp(0);
  idxSize = index.size();
  numwrite(out,startIdx);
  numwrite(out,idxSize-1);
  numwrite(out,totalWords);
  if (out.fail())
    cerr << efatal << "Writing corpus track file '" << ctrackFile << "' failed."
         << exit_1;
  out.close();

  if (unkCnt)
    cerr << efatal << unkCnt << " words in the corpus are not in the TokenIndex." << endl
         << " TokenIndex file '" << vocabFile << "' appears out of sync with the corpus."
         << exit_1;
  if (!quiet) cerr << "Done" << endl;
}
END_MAIN
