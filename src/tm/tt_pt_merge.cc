/**
 * @author Denis Yuen
 * @file tt_pt_merge.cc  Program that merges phrase_table and ibm_model using a weight of W for the ibm_model. 
 * 
 * 
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada 
 *
 */

#include <stdlib.h>
#include <vector>
#include <queue>
#include <map>
#include <errors.h>
#include <iostream>
#include <sstream>
#include <str_utils.h>
#include <file_utils.h>
#include <portage_defs.h>
#include "ttable.h"
#include "arg_reader.h"
#include <time.h>
#include <printCopyright.h>

using namespace Portage;
using namespace __gnu_cxx; 

static char help_message[] = "\n\
tt_pt_merge  [-t X] [-x Y] [-w W] [-no-clean] phrase_table ibm_model \n\
\n\
Merge phrase_table and ibm_model using a weight of W for the ibm_model\n\
If no-clean is set, the temporary files will be left intact after completion. \n\
      Modes: \n\
      -t f -x Y 	Keep all ibm entries with a probability greater than Y\n\
      -t t -x Y		Keep the Y best matches for a given target word\n\
\n\
Default setting: -t f -x 0.5 -w 0.1 \n\
Note: Uses UNIX sort, which appears to have locale issues, resulting in duplicates\n\
      Temporary fix: setenv LC_ALL C \n\
";

/// Sstores information about one line from the phrase table.
struct CondProb{ 
   string target;
   string source;
   double prob;
   bool ibm;
   /**
    * Default construtor.
    * @param target
    * @param source
    * @param prob
    * @param ibm
    */
   CondProb(string target, string source, double prob, bool ibm) : 
      target(target), source(source), prob(prob), ibm(ibm) {};
};

/// Stored in priority queues, overloaded < for this purpose.
struct WordProbPair{ 
   string word;
   double prob;
   /**
    * Default constructor.
    * @param word
    * @param prob
    */
   WordProbPair(string word, double prob) : word(word), prob(prob) {};
   /**
    * Compares two WordProbPairs based on their prob.
    * @param a  right-hand side operand.
    * @return Returns true if a < this.
    */
   bool operator< (const WordProbPair &a) const{
      return a.prob < prob;
   };
};

void inline substrSplit(string input, vector<string>& store, string pattern = " ")
{
   for(;;)
   {
      input = trim(input);
      string::size_type i = input.find(pattern);
      if (i == string::npos) {store.push_back(input); break;}
      store.push_back(input.substr(0,i-1));
      input.erase(0,i+pattern.size()); 
   }
}
   
/**
 * Reads in an IBM model from in, outputting only the probabilities higher than
 * type_mod to out
 * @param in
 * @param out
 * @param type_mod
 */
void readIBM_fixed(istream& in, ostream& out, double type_mod)
{
    int count = 0;
    int i = 0;
    cerr << "Processing IBM model: ";
    for (;;i++)
    {
       if (i % 100000 == 0) {cerr << ".";}
       string line;
       getline(in, line);
       if (in.eof() && line == "") break;
       vector<string> phrases;
       split(line, phrases, " ");
       if (phrases.size() != 3)
       { error(ETFatal, "IBM file has incorrect format"); } 

       stringstream temp;
       double prob; // ugly conversion hack, need Perl 
       temp << phrases[2]; 
       temp >> prob;
       if (prob > type_mod) 
       { 
	  count++; 
	  out << phrases[1] << " ||| " << phrases[0] << " ||| " << prob << " ||| ibm" << endl; 
       }
    }
    cerr << endl << "IBM total lines: "<< i << endl;
    cerr << "IBM lines kept:  "<< count << endl;
}

/**
 * Reads in an IBM model from in, outputting only the type_mod best entries (where better is higher 
 * probability) per target word to out.
 * @param in
 * @param out
 * @param type_mod
 */
void readIBM_targetted(istream& in, ostream& out, double type_mod)
{
    hash_map<string, priority_queue<WordProbPair> ,StringHash> storage;
    int count = 0;
    int i = 0;
    cerr << "Processing IBM model: ";
    for (;;i++)
    {
       if (i % 100000 == 0) {cerr << ".";}
       string line;
       getline(in, line);
       if (in.eof() && line == "") break;
       vector<string> phrases;
       split(line, phrases, " ");
       if (phrases.size() != 3)
       { error(ETFatal, "IBM file has incorrect format"); } 

       stringstream temp;
       double prob; // ugly conversion hack, need Perl 
       temp << phrases[2]; 
       temp >> prob;
       storage[phrases[0]].push(WordProbPair(phrases[1],prob));
       if (storage[phrases[0]].size() > type_mod)
       { storage[phrases[0]].pop(); }
    }
    for (hash_map<string, priority_queue<WordProbPair> ,StringHash>::iterator iter = storage.begin(); 
	  iter != storage.end(); iter++)
    {
       while(!iter->second.empty())
       { WordProbPair temp = iter->second.top();
	 out << temp.word << " ||| " << iter->first << " ||| " << temp.prob << " ||| ibm" << endl; 
         iter->second.pop();
	 count++;
       }
    }
    cerr << endl << "IBM total lines: "<< i << endl;
    cerr << "IBM lines kept:  "<< count << endl;
}

// reads in a single phrase table entry from in
CondProb readPTLine(istream& in)
{
   vector<string> curr;
   string line;
   getline(in, line);
   if (in.eof() && line == "") return CondProb("","",-1,false);
   substrSplit(line, curr, "|||");
   for (Uint i = 0; i < curr.size(); i++) // get rid of extra spacing
   { curr[i] = trim(curr[i]); }
   stringstream temp;
   double prob; // ugly conversion hack, need Perl 
   temp << curr[2]; 
   temp >> prob;
   return CondProb(curr[0], curr[1], prob, curr.size() == 4);
}

// merges duplicates from a merged IBM file and a phrase table
// using a normalizing formula as suggest by George Foster
// note: ibm entries are marked with an extra column to distinguish them
int clearDups(istream& in, double weight)
{   // kinda want a simple FSM, use labelled break to emulate, but still
   int count = 0;
   cerr << "Merging duplicates: ";
    for (int i = 0;;i++)
    {
      if (i % 100000 == 0) {cerr << ".";}
      CondProb curr = readPTLine(in);
      if (curr.prob == -1) {cerr << endl; return count;} // eof
      for(;;) // a static multi-exit loop here would be useful but we don't have them, grr
      {
	 CondProb next = readPTLine(in);
	 if (next.prob == -1) //eof
	 { double modProb = curr.ibm ? curr.prob*weight : curr.prob*(1.00-weight);
	   cout << curr.target << " ||| " << curr.source << " ||| " << modProb << endl;
	   cerr << endl;
	   return count;
	 }
	 if (curr.target == next.target && curr.source == next.source) //combine
	 {  double modProb = 0;
	    if (curr.ibm && !next.ibm)
	    { modProb = weight * curr.prob + (1.00-weight) * next.prob; }
	    else if (!curr.ibm && next.ibm)
	    { modProb = weight * next.prob + (1.00-weight) * curr.prob; }
	    else
	    { error(ETFatal, "Merged file has incorrect format"); } 
	   cout << curr.target << " ||| " << curr.source << " ||| " << modProb << endl;
	   count++;
	   break;
	 }
	 else // output curr and return to state 1
	 { double modProb = curr.ibm ? curr.prob*weight : curr.prob*(1.00-weight);
	   cout << curr.target << " ||| " << curr.source << " ||| " << modProb << endl;
	   curr = next;
	 }
      }
   }
}

int main(Uint argc, const char * const * argv)
{
    printCopyright(2004, "tt_pt_merge");
    const char *switches[] = {"t:", "x:", "w:","no-clean"}; 
    // t is a required parameter indicating type
    ArgReader argReader(ARRAY_SIZE(switches), switches, 2, 2, help_message);
    argReader.read(argc - 1, argv + 1);
    // Process command-line arguments.
    const string &ptFile = argReader.getVar(0);
    const string &ibmFile = argReader.getVar(1);
    // create names for temporary files
    stringstream temp1;
    stringstream temp2;
    temp1 << "tt_pt_merge." << ptFile << ".";
    temp2 << "tt_pt_merge." << ptFile << ".";
    long currTime = time(NULL);
    temp1 << currTime;
    temp2 << currTime;
    temp1 << ".tmp";
    temp2 << ".tmp2";
    string tempFile;
    string sortedFile;
    temp1 >> tempFile;
    temp2 >> sortedFile;
    
    char type = 'f';
    argReader.testAndSet("t", type);
    double type_mod = 0.5;
    argReader.testAndSet("x", type_mod);
    double weight = 0.1;
    argReader.testAndSet("w",weight);
    bool no_clean = false;
    argReader.testAndSet("no-clean", no_clean);
    
    // Open input files
    IMagicStream ibm_in(ibmFile);
    IMagicStream pt_in(ptFile);

    // open temporary file 
    OMagicStream t_out(tempFile);
    
    // read and filter ibm model based on type and type_modifier
    if (type == 'f') // filter based on a fixed threshold
    { readIBM_fixed(ibm_in, t_out, type_mod); }
    if (type == 't') // filter based on a fixed threshold
    { readIBM_targetted(ibm_in, t_out, type_mod); }

    // temp file now contains filtered IBM model, append phrase table
    // sort phrase table and ibm model together
    string command = "sort " + tempFile + " " + ptFile + " > " + sortedFile;
    system(command.c_str());
    
    // go through line by line and merge duplicates based on weight
    IMagicStream sorted_in(sortedFile);

    int cleared = clearDups(sorted_in, weight);
    cerr << "Duplicates merged: " << cleared << endl;

    //clean-up
    if (!no_clean)
    {
       command = "rm " + tempFile; 
       system(command.c_str());
       command = "rm " + sortedFile;
       system(command.c_str());
    }

    return 0;
} // main


