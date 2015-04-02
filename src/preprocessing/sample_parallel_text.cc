/**
 * @author George Foster
 * @file sample_parallel_text.cc 
 * @brief Sub-sample a parallel text (with one or more references).
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <set>
#include "docid.h"
#include "arg_reader.h"
#include "file_utils.h"
#include "gfstats.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
sample_parallel_text [-r|-b|-c][-n samplesize|-p pc][-seed s]\n\
                     [-w wsize][-s suff][-l min-max][-d docidfile]\n\
                     [-docid docidfile]\n\
                     src tgt1 [tgt2 tgt3 ...]\n\
\n\
Produce a sub-sample of a parallel text, with one or more target texts. Output\n\
files are written to <src>.<suffix> and <tgt_i>.<suffix>.\n\
\n\
Options:\n\
\n\
-v  Invert the sampling technique, writing only lines that do NOT match. This\n\
    is useful for extracting the exact complement of a previous sample.\n\
-r  Sample randomly, without replacement [sample by choosing lines at fixed\n\
    intervals]\n\
-b  Sample randomly, with replacement [sample by choosing lines at fixed\n\
    intervals]. This option also permutes order of output text. Not compatible\n\
    with -v -w -d -docid -l.\n\
-c  Sample randomly by choosing each line with probability <pc>/100. This\n\
    reinterprets the -p switch (as sampling probability, rather than final\n\
    size), and is not compatible with -v -w -d -docid -l.\n\
-n  Produce a sample of size samplesize [0 = use -p to determine size]\n\
-p  Produce a sample that is pc percent of src size [10]\n\
-seed Set random seed to <s>.\n\
-w  For each line chosen for the sample, also choose the <wsize> lines starting\n\
    at that point (if there are enough lines left in the file). This means the\n\
    total sample size may be as large as <samplesize> * <wsize>. [1]\n\
-s  Use given suffix for output files [.sample]\n\
-d  Read document id file containing one entry per source line, in the format:\n\
    docname [optional-string]. With this option, -n is interpreted as applying\n\
    to documents rather than lines, and the sampled text will never split a\n\
    document boundary.\n\
-docid docid file to split in parallel with the src and tgts. \n\
    NB: this option is deprecated: you can accomplish the same thing by putting\n\
    <docidfile> in the list of <tgt> files.\n\
-l  Choose segments having between min and max tokens (inclusive). Max may be\n\
    omitted to specify no limit. This is incompatible with most other options,\n\
    and always extracts all matching segments.\n\
";

// globals

static bool invert = false;
static bool randomsel = false;
static bool bootstrapsel = false;
static bool choosesel = false;
static Uint samplesize = 0;
static double sampleprop = 10.0;
static Uint seed = 0;
static Uint wsize = 1;
static string length_spec = "";
static Uint minlength, maxlength;
static string suffix = ".sample";
static string docidfilename = "";
static string srcfilename;
static vector<string> tgtfilenames;
static string toSplitDocId;

static void getArgs(int argc, char* argv[]);

// Sampling method for -b is different from other methods and switch combinations:

static void doBootstrap(Uint numlines)
{
   if (samplesize) {
      samplesize = numlines * sampleprop / 100.0;
      if (samplesize == 0) samplesize = 1;
   }

   // choose line numbers to output

   vector<Uint> lines(samplesize);
   for (Uint i = 0; i < lines.size(); ++i)
      lines[i] = rand(numlines);

   // write new files

   vector<string> allfiles(tgtfilenames);
   allfiles.push_back(srcfilename);
   vector<string> contents;

   for (vector<string>::iterator p = allfiles.begin(); p != allfiles.end(); ++p) {

      contents.clear();
      readFileLines(*p, contents);
      if (contents.size() != numlines)
         error(ETFatal, "file %s has the wrong number of lines", p->c_str());

      const string outfile = addExtension(*p, suffix);
      oSafeMagicStream ostr(outfile);

      for (Uint i = 0; i < lines.size(); ++i)
         ostr << contents[lines[i]] << endl;

   }
} 

// Sampling method for -c is different from other methods and switch combinations:

static void doChoose(Uint numlines)
{
   if (samplesize)
      error(ETWarn, "samplesize (-n) specification irrelevant with -c");

   // choose line numbers to output: 0 means no; 1 means yes

   vector<Uint> lines(numlines);
   for (Uint i = 0; i < lines.size(); ++i)
      lines[i] = rand(100) < sampleprop;
   
   // write new files

   vector<string> allfiles(tgtfilenames);
   allfiles.push_back(srcfilename);
   vector<string> contents;

   for (vector<string>::iterator p = allfiles.begin(); p != allfiles.end(); ++p) {

      contents.clear();
      readFileLines(*p, contents);
      if (contents.size() != numlines)
         error(ETFatal, "file %s has the wrong number of lines", p->c_str());

      string outfile = addExtension(*p, suffix);
      oSafeMagicStream ostr(outfile);

      for (Uint i = 0; i < contents.size(); ++i)
         if (lines[i])
            ostr << contents[i] << endl;
   }
} 


// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "sample_parallel_text");
   getArgs(argc, argv);

   if (seed != 0)
      std::srand(seed);

   set<Uint> line_numbers;

   DocID* docids = NULL;
   if (docidfilename != "")
      docids = new DocID(docidfilename);

   // count lines & tokens

   Uint num_lines = 0;
   {
      iSafeMagicStream sfile(srcfilename);
      string line;
      vector<string> toks;
      while (getline(sfile, line)) {
         if (!length_spec.empty()) {
            toks.clear();
            const Uint ntoks = split(line, toks);
            if (ntoks >= minlength && ntoks <= maxlength)
               line_numbers.insert(num_lines);
         }
         ++num_lines;
      }
   }

   if (bootstrapsel) {
      doBootstrap(num_lines);
      return 0;
   } else if (choosesel) {
      doChoose(num_lines);
      return 0;
   }

   // generate set of indexes (line or document numbers) to output

   const Uint nl = docids ? docids->numDocs() : num_lines;

   if (samplesize == 0) {
      samplesize = nl * sampleprop / 100.0;
      if (samplesize == 0) samplesize = 1;
   }
   if (samplesize > nl) {
      error(ETWarn, "sample size too big; lowering it to %d", nl);
      samplesize = nl;
   }

   vector<Uint> unchosen;
   if (randomsel) {
      unchosen.resize(nl);
      for (Uint i = 0; i < nl; ++i)
         unchosen[i] = i;
   }

   if (length_spec.empty()) {
      for (Uint i = 0; i < samplesize; ++i) {
         Uint lno;
         if (randomsel) {
            const Uint ui = rand(unchosen.size());
            lno = unchosen[ui];
            unchosen.erase(unchosen.begin()+ui);
         } else 
            lno = Uint(((double)nl / samplesize) * i);
         for (Uint j = 0; j < wsize && lno+j < nl; ++j)
            line_numbers.insert(lno+j);
      }
      if (wsize == 1)
         assert(line_numbers.size() == samplesize);
   }
  
   if (docids) {           // convert doc indexes into line numbers
      set<Uint> doc_numbers(line_numbers);
      line_numbers.clear();
      for (Uint i = 0; i < docids->numSrcLines(); ++i)
         if (doc_numbers.find(docids->docID(i)) != doc_numbers.end())
            line_numbers.insert(i);
   }

   // read input files and write sample files
   
   iSafeMagicStream srcfile(srcfilename);
   oSafeMagicStream srcsamplefile(addExtension(srcfilename, suffix));

   iSafeMagicStream* iDocId = NULL;
   oSafeMagicStream* oDocId = NULL;
   if (!toSplitDocId.empty()) {
      iDocId = new iSafeMagicStream(toSplitDocId);
      oDocId = new oSafeMagicStream(addExtension(toSplitDocId, suffix));
   }

   vector<istream*> tgtfiles(tgtfilenames.size());
   vector<ostream*> tgtsamplefiles(tgtfilenames.size());
   for (Uint i = 0; i < tgtfilenames.size(); ++i) {
      tgtfiles[i]       = new iSafeMagicStream(tgtfilenames[i]);
      tgtsamplefiles[i] = new oSafeMagicStream(addExtension(tgtfilenames[i], suffix));
   }

   num_lines = 0;
   string src_line;
   string docid_line;
   vector<string> tgt_lines(tgtfilenames.size());
   for (set<Uint>::iterator p = line_numbers.begin(); p != line_numbers.end(); ++p) {
      for (; num_lines < *p+1; ++num_lines) {
         // SOURCE FILE
	 if (!getline(srcfile, src_line))
	    error(ETFatal, "source file " + srcfilename + " too short!");
         // DOCIDS
         if (iDocId && !getline(*iDocId, docid_line))
            error(ETFatal, "docid file " + toSplitDocId + " too short!");
         // TARGET FILES
         for (Uint i = 0; i < tgtfilenames.size(); ++i)
            if (!getline(*tgtfiles[i], tgt_lines[i]))
               error(ETFatal, "target file " + tgtfilenames[i] + " too short!");
         
         if ((invert && num_lines != *p) || (!invert && num_lines == *p)) {
            srcsamplefile << src_line << endl;
            if (oDocId) *oDocId << docid_line << endl;
            for (Uint i = 0; i < tgtfilenames.size(); ++i)
               (*tgtsamplefiles[i]) << tgt_lines[i] << endl;
         }
      }
   }
   if (invert) {
      while (getline(srcfile, src_line)) {
         if (iDocId && !getline(*iDocId, docid_line))
            error(ETFatal, "docid file " + toSplitDocId + " too short!");
         for (Uint i = 0; i < tgtfilenames.size(); ++i)
            if (!getline(*tgtfiles[i], tgt_lines[i]))
               error(ETFatal, "target file " + tgtfilenames[i] + " too short!");

         srcsamplefile << src_line << endl;
         if (oDocId) *oDocId << docid_line << endl;
         for (Uint i = 0; i < tgtfilenames.size(); ++i)
            (*tgtsamplefiles[i]) << tgt_lines[i] << endl;
      }
   }
   delete iDocId;
   delete oDocId;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "r", "b", "c", "n:", "p:", "seed:", "w:", "s:", "l:", "d:", "docid:"};

   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message, "-h", true);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", invert);
   arg_reader.testAndSet("r", randomsel);
   arg_reader.testAndSet("b", bootstrapsel);
   arg_reader.testAndSet("c", choosesel);
   arg_reader.testAndSet("n", samplesize);
   arg_reader.testAndSet("p", sampleprop);
   arg_reader.testAndSet("seed", seed);
   arg_reader.testAndSet("w", wsize);
   arg_reader.testAndSet("s", suffix);
   arg_reader.testAndSet("l", length_spec);
   arg_reader.testAndSet("d", docidfilename);
   arg_reader.testAndSet("docid", toSplitDocId);
   
   arg_reader.testAndSet(0, "src", srcfilename);
   arg_reader.getVars(1, tgtfilenames);

   if (arg_reader.getSwitch("p") && arg_reader.getSwitch("n"))
      error(ETFatal, "only one of -n and -p can be used");

   if (arg_reader.getSwitch("b") || arg_reader.getSwitch("c"))
      if (arg_reader.getSwitch("v") ||
          arg_reader.getSwitch("w") || arg_reader.getSwitch("docid") ||
          arg_reader.getSwitch("d") || arg_reader.getSwitch("l"))
         error(ETFatal, "Switches -v, -w, -d, -docid, and -l are not compatible with -b/-c");

   if ((randomsel && (bootstrapsel || choosesel)) || (bootstrapsel && choosesel))
      error(ETFatal, "at most one of -r, -b and -c can be specified");

   if (length_spec != "") {
      if (arg_reader.getSwitch("r") || arg_reader.getSwitch("b") || 
          arg_reader.getSwitch("n") || arg_reader.getSwitch("w") ||
          arg_reader.getSwitch("d") || arg_reader.getSwitch("p"))
         error(ETFatal, "Switches -r|-b, -n, and -w are not compatible with -l");
      vector<string> toks;
      if (split(length_spec, toks, "- ,") != 2)
         error(ETFatal, "-l argument must be in the form 'min-max'");
      minlength = conv<Uint>(toks[0]);
      maxlength = conv<Uint>(toks[1]);
   }
}   
