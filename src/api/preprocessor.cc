/**
 * @author George Foster
 * @file preprocessor.cc  Implementation of Preprocessor
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */

#include <stdlib.h>
#include <str_utils.h>
#include <file_utils.h>
#include "portage_env.h"
#include "preprocessor.h"

using namespace Portage;

Preprocessor::Preprocessor(const string& srclang)
{
   char tmp[] = "paddle_Preprocessor.XXXXXX";
   if (close(mkstemp(tmp)))
      error(ETFatal, "Unable to create temp file in Preprocessor");
   tempfile = tmp;

   if (srclang == "en") {
      cmd = "tokenize.pl | lc-latin.pl > " + tempfile;
   } else if (srclang == "fr") {
      cmd = "tokenize.pl -lang=fr | lc-latin.pl > " + tempfile;
   } else if (srclang == "ch") {
      cmd = "sen-mandarin.pl | mansegment.perl " + getPortage() + "/models/mansegment/Mandarin.fre > " + tempfile;
   } else
      error(ETFatal, "Unknown source language: %s", srclang.c_str());
}

void Preprocessor::proc(const string& raw_text, vector<vector<string> >& prep_text)
{
   FILE* file = popen(cmd.c_str(), "w");
   if (!file)
      error(ETFatal, "unable to open pipe for [%s]", cmd.c_str());
   fputs(raw_text.c_str(), file);
   pclose(file);

   prep_text.clear();
   IMagicStream ifstr(tempfile);
   string line;
   vector<string> empty;
   while (getline(ifstr, line)) {
      prep_text.push_back(empty);
      split(line, prep_text.back());
   }
}
