/**
 * @author Bruno Laferriere
 * @file testlm.cc  Program for testing language model implementations.
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#define NGRAM 3

#include "lm.h"
#include <arg_reader.h>
#include <exception_dump.h>
#include <printCopyright.h>
#include <str_utils.h>
#include <fstream>
#include <file_utils.h>
#include <vector>
#include <time.h>
#include <portage_defs.h>
//#include <dmalloc.h>

using namespace std;
using namespace Portage;

static char help_message[] = "\n\
testlm lmfile testfile\n\
\n\
Test suite for LMText and any new LM type added.\n\
The type of LM created depends on the file name <lmfile>.\n\
\n\
Options:\n\
\n\
 -v     verbose output\n\
 -q     quiet output (only print global prob)\n\
 -limit limit vocab (load all input sentences before processing)\n\
\n\
";

//global

static string lm_filename;
static string test_filename;
static bool verbose;
static bool quiet;
static bool limit_vocab = false;
static const float LOG_ALMOST_0 = -18;
static Uint order = 3;

//Functions declarations
static void getArgs(int argc, const char* const argv[]);
static bool file_exist(string filename);
static float processOneLine_fast(const string& line, PLM* lm, Voc& voc);

int MAIN(argc,argv)
{
   printCopyright(2006, "testlm");

   getArgs(argc, argv);

   if(!file_exist(lm_filename)){
       cerr << "The lm file '" << lm_filename << "' does not exist." << endl;
       exit(0);
       }

   //Begin program
   Uint precision = cout.precision();
   cout.precision(6);

   time_t start = time(NULL);             //Time count

   Voc vocab;
   if ( limit_vocab ) {
      string line;
      IMagicStream in(test_filename);
      while (getline(in, line)) {
         if(line.length()==0)
            continue;
         char buf[line.length()+1];
         strcpy(buf, line.c_str());
         char* strtok_state; // state variable for strtok_r
         char* tok = strtok_r(buf, " ", &strtok_state);
         while (tok != NULL) {
            vocab.add(tok);
            tok = strtok_r(NULL, " ", &strtok_state); 
         }
      }
      cerr << "Read input (... " << (time(NULL) - start) << " secs)" << endl;
   }

   PLM* lm = PLM::Create(lm_filename,&vocab, true, limit_vocab, 0, -INFINITY);
   order = lm->getOrder();
   cerr << "Loaded " << order << "-gram model (... "
        << (time(NULL) - start) << " secs)" << endl;

   string line;

   IMagicStream testfile(test_filename);
   float docProb = 0.0;
   while(getline(testfile,line)) {
       if(line.length()==0)
            continue;
       docProb += processOneLine_fast(line,lm,vocab);
       //vector<string> words;
       //split(line,words," ");
       //cout << "----------------------"<< endl << "\x1b[31m" << line << "\x1b[0m\n" << endl;
       //processOneLine(words,lm,vocab);
   }

   if ( quiet || verbose )
      cout << "Document logProb = " << docProb << endl << endl;

   cerr << "End (... " << (time(NULL) - start) << " secs)" << endl;

   Uint seconds =(Uint)difftime(time(NULL), start); //Number of seconds elapsed

   Uint mins = seconds / 60;
   Uint hours = mins/60;
   mins = mins%60;
   seconds = seconds % 60;

   cerr << "Program done in " << hours << "h" << mins << "m" << seconds << "s" << endl;

   cout.precision(precision);
   testfile.close();

   // For debugging purposes, it's best to delete lm, but we don't normally do it.
   //delete lm;
} END_MAIN

//************************************************Functions*******************************************************//
// arg processing

void getArgs(int argc, const char* const argv[])
{
   const char* const switches[] = {"v", "limit", "q"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("q", quiet);
   if ( quiet ) verbose = false;
   arg_reader.testAndSet("limit", limit_vocab);
   arg_reader.testAndSet(0,"lm_filename", lm_filename);
   arg_reader.testAndSet(1,"test_filename", test_filename);
}

bool file_exist(string filename)
{
   ifstream in;
   in.open(filename.c_str());
   if(in.is_open())
       return true;
   else
       return false;
   in.close();
}

float processOneLine_fast(const string& line,PLM* lm,Voc& voc)
{
   float sentProb = 0.0;
   char buf[line.length() + 1];
   Uint context[order - 1];
   for ( Uint i = 0; i < order - 1; ++i ) context[i] = voc.index(PLM::SentStart);
   strcpy(buf, line.c_str());
   char* strtok_state; // state variable for strtok_r
   char* tok = strtok_r(buf, " ", &strtok_state);
   while (tok != NULL) {
      Uint word = limit_vocab ? voc.index(tok) : voc.add(tok);
      float wordProb = lm->wordProb(word, context, order-1);
      if (verbose) cout << "p( " << voc.word(word) << " | "
                        << voc.word(context[0]) << " ...)\t= ";
      if ( !quiet ) cout << wordProb << endl;
      if ( wordProb != -INFINITY ) sentProb += wordProb;

      for ( Uint i = order - 1; i > 0; --i ) context[i] = context[i-1];
      context[0] = word;

      tok = strtok_r(NULL, " ", &strtok_state);
   }
   Uint word = voc.index(PLM::SentEnd);
   float eosProb = lm->wordProb(word, context, order-1);
   if (verbose) cout << "p( " << voc.word(word) << " | "
                     << voc.word(context[0]) << " ...)\t= ";
   if ( !quiet ) cout << eosProb << endl;
   if ( eosProb != -INFINITY ) sentProb += eosProb;

   if (verbose)
      cout << "logProb = " << sentProb << endl << endl;

   return sentProb;
}


