// $Id$
/**
 * @author Eric Joanis
 * @file tm_entry.cc
 * @brief Implementation of TMEntry
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include "tm_entry.h"
#include "str_utils.h"

using namespace Portage;

const char* TMEntry::sep = " ||| ";
const Uint TMEntry::sep_len = strlen(TMEntry::sep);

TMEntry::TMEntry(const string& filename, bool reversed_table)
   : buffer(new char[200])
   , buffer_size(200)
   , src(NULL)
   , tgt(NULL)
   , third(NULL)
   , fourth(NULL)
   , reversed_table(reversed_table)
   , filename(filename)
   , lineno(0)
   , third_count(0)
   , fourth_count(0)
   , is_initialized(false)
{
   buffer[buffer_size-1] = '\0';
}

TMEntry::~TMEntry()
{
   delete [] buffer;
}

void TMEntry::init()
{
   vector<string> third_tokens;
   split(third, third_tokens);
   third_count = third_tokens.size();
   while (third_count > 0 && third_tokens[third_count-1].find('=') != string::npos)
      --third_count;

   if (fourth) {
      vector<string> fourth_tokens;
      split(fourth, fourth_tokens);
      fourth_count = fourth_tokens.size();
   } else {
      fourth_count = 0;
   }

   is_initialized = true;
}

void TMEntry::init(const string& line)
{
   newline(line);
}

void TMEntry::newline(const string& line)
{
   if (line.size() + 1 > buffer_size) {
      delete [] buffer;
      buffer_size = max(buffer_size * 2, line.size() + 1);
      buffer = new char[buffer_size];
      buffer[buffer_size-1] = '\0';
   }
   // Copy line into the buffer we own, which we can safely parse destructively
   strcpy(buffer, line.c_str());
   assert(buffer[buffer_size-1] == '\0');
   ++lineno;

   // Look for two or three occurrences of |||.
   const string::size_type index1 = line.find(sep, 0);
   if (index1 == string::npos)
      error(ETFatal, "Bad format in %s at line %d: %s", filename.c_str(), lineno, buffer);
   const string::size_type index2 = line.find(sep, index1 + sep_len);
   if (index2 == string::npos)
      error(ETFatal, "Bad format in %s at line %d: %s", filename.c_str(), lineno, buffer);
   const string::size_type index3 = line.find(sep, index2 + sep_len - 1);

   buffer[index1] = '\0';
   buffer[index2] = '\0';
   if (index3 != string::npos)
      buffer[index3] = buffer[index3+1] = '\0';

   src       = trim(buffer, " ");
   tgt       = trim(buffer + index1 + sep_len);
   third     = trim(buffer + index2 + sep_len);
   if (index3 != string::npos)
      fourth = trim(buffer + index3 + sep_len);
   else
      fourth = NULL;

   if (reversed_table)
      std::swap(src, tgt);

   if (lineno % 1000000 == 0)
      cerr << '.' << flush;

   if (!is_initialized)
      init();
}
