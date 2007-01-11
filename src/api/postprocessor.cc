/**
 * @author George Foster
 * @file postprocessor.cc  Implementation of Postprocessor
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / 
 * Copyright 2005, National Research Council of Canada
 */

#include <file_utils.h>
#include <str_utils.h>
#include "postprocessor.h"
#include "portage_env.h"

using namespace Portage;

Postprocessor::Postprocessor(const string& tgtlang, bool verbose) :
   verbose(verbose)
{
   char tmp[] = "paddle_Postprocessor.XXXXXX";
   if (close(mkstemp(tmp)))
      error(ETFatal, "Unable to create temp file in Postprocessor");
   tempfile = tmp;
   if (tgtlang == "en") {
      cmd = "detokenize.pl -lang=en > " + tempfile;
   } else if (tgtlang == "fr") {
      cmd = "detokenize.pl -lang=fr > " + tempfile;
   } else
      error(ETFatal, "Unknown target language: %s", tgtlang.c_str());
}

string& Postprocessor::proc(vector<vector<string> >& mt_out, string& processed_text)
{
   // convert mt_out to array of sentence strings & truecase each
   sents.resize(mt_out.size());

   for (Uint i = 0; i < mt_out.size(); ++i) {
      if (verbose) {
	 string x;
	 cerr << "raw mt:    [" << join(mt_out[i].begin(), mt_out[i].end(), x) << "]" << endl;
      }
      twiddle(mt_out[i]);
      if (verbose) {
	 string x;
	 cerr << "twiddled:  [" << join(mt_out[i].begin(), mt_out[i].end(), x) << "]" << endl;
      }
      sents[i].clear();
      join(mt_out[i].begin(), mt_out[i].end(), sents[i]);
      // Insert a call to your truecasing engine here.
      //tc.convert(sents[i], sents[i]);
      if (verbose) {
	 //cerr << "truecased: [" << sents[i] << "]" << endl;
      }
   }

   // detokenize to file
   FILE* file = popen(cmd.c_str(), "w");
   if (!file)
      error(ETFatal, "unable to open pipe for [%s]", cmd.c_str());
   for (Uint i = 0; i < sents.size(); ++i) {
      fputs(sents[i].c_str(), file);
      fputc('\n', file);
   }
   pclose(file);

   // retun contents of file
   processed_text.clear();
   return gulpFile(tempfile.c_str(), processed_text);
}

void Postprocessor::twiddle(vector<string>& sent)
{
   vector<Uint> pos;

   for (Uint i = 0; i < sent.size(); ++i)
      if (sent[i].find_first_of("\"") != string::npos)
	 pos.push_back(i);
   if (sent.size() > 1 && pos.size() == 1 && sent[pos[0]] == "\"")
      sent.erase(sent.begin() + pos[0]);

   pos.clear();
   for (Uint i = 0; i < sent.size(); ++i)
      if (sent[i] == "(" || sent[i] == ")")
	 pos.push_back(i);
   if (sent.size() > 1 && pos.size() == 1)
      sent.erase(sent.begin() + pos[0]);
   
}
