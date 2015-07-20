/**
 * @author George Foster
 * @file split_parallel_text_by_tag.cc
 * @brief Split a parallel text by tags in a docid file.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#include <queue>
#include "docid.h"
#include "arg_reader.h"
#include "file_utils.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
split_parallel_text_by_tag [-r][-s sep][-d dir][-f field] docidfile file1 [file2...]\n\
\n\
Split one or more line-aligned files according to tags found in a docid file.\n\
Each line in <fileI> that is aligned to tag FOO in the first column of <docidfile>\n\
will be written to <fileI><sep>FOO. Warning: this silently overwrites any existing\n\
files with these names!\n\
\n\
Options:\n\
\n\
-r  Write output files FOO<sep><fileI> [write <fileI><sep>FOO]\n\
-s  Use <sep> as indicated above [.]\n\
-d  Write results to directory <dir> [.]\n\
-f  Use tags in field <field> [0 = first]\n\
-m  Maximum number of tags for which to allow open filehandles [512]\n\
";

// globals

static string sep = ".";
static string dir = ".";
static Uint field = 0;
static Uint maxfiles = 512;
static bool rev_output = false;
static string docidfilename = "";
static vector<string> filenames(1);

static void getArgs(int argc, char* argv[]);

static string outFileName(const string& tag, Uint fileindex) {
   return dir + "/" + 
      (rev_output ? tag + sep + BaseName(filenames[fileindex]) :
       BaseName(filenames[fileindex]) + sep + tag);
}

static void closeOutFiles(vector<ostream*>& ofiles) {
   for (vector<ostream*>::iterator f = ofiles.begin(); f != ofiles.end(); ++f)
      delete *f;
   ofiles.resize(0); // indicate that files are closed
}

// main

int main(int argc, char* argv[])
{
   printCopyright(2007, "split_parallel_text_by_tag");
   getArgs(argc, argv);

   DocID docids(docidfilename, field);

   vector<istream*> files(filenames.size());
   for (Uint i = 0; i < filenames.size(); ++i)
      files[i] = new iSafeMagicStream(filenames[i]);

   typedef map< string, vector<ostream*> > Map;   // tag -> list of output files
   queue<string> openfiles;     // list of tags for which files are currently open
   Map outfiles;
   string line;
   
   for (Uint i = 0; i < docids.numSrcLines(); ++i) {

      string tag, other;
      docids.parse(docids.docline(i), tag, other);
      
      pair<Map::iterator, bool> res = outfiles.insert(make_pair(tag,vector<ostream*>(0)));
      if (res.second) {         // new tag
         if (openfiles.size() == maxfiles) { // close oldest tag's files if limit exceeded
            closeOutFiles(outfiles[openfiles.front()]);
            openfiles.pop();
         }
         for (Uint j = 0; j < files.size(); ++j) // open new files
            res.first->second.push_back(new oSafeMagicStream(outFileName(tag, j)));
         openfiles.push(tag);
      } else if (res.first->second.size() == 0) { // existing tag with closed files
         if (openfiles.size() == maxfiles) { // close oldest tag's files if limit exceeded
            closeOutFiles(outfiles[openfiles.front()]);
            openfiles.pop();
         }
         for (Uint j = 0; j < files.size(); ++j) { // reopen this tag's files
            string s = outFileName(tag, j);
            res.first->second.push_back(new ofstream(s.c_str(), ios_base::app));
         }
         openfiles.push(res.first->first);
      }
      for (Uint j = 0; j < files.size(); ++j) {
         if (!getline(*files[j], line))
            error(ETFatal, "file %s is too short", filenames[j].c_str());
         *res.first->second[j] << line << endl;
      }
   }

   for (Map::iterator p = outfiles.begin(); p != outfiles.end(); ++p)
      for (vector<ostream*>::iterator f = p->second.begin(); f != p->second.end(); ++f)
         delete *f;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"r", "s:", "d:", "f:", "m:"};

   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message, "-h", true);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("r", rev_output);
   arg_reader.testAndSet("s", sep);
   arg_reader.testAndSet("d", dir);
   arg_reader.testAndSet("f", field);
   arg_reader.testAndSet("m", maxfiles);

   if (maxfiles == 0)
      error(ETFatal, "maxfiles can't be 0");

   arg_reader.testAndSet(0, "docidfile", docidfilename);
   arg_reader.testAndSet(1, "file", filenames[0]);
   arg_reader.getVars(2, filenames);
}   
