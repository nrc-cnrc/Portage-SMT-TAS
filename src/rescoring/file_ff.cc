/**
 * @author Aaron Tikuisis / George Foster / Samuel Larkin / Eric Joanis
 * @file file_ff.cc  Read-from-file feature function and its dynamic version
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005 - 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005 - 2008, Her Majesty in Right of Canada
 */

#include "file_ff.h"
#include "fileReader.h"

using namespace Portage;

//---------------------------------------------------------------------------
// FileFF
//---------------------------------------------------------------------------
const char FileFF::separator = ',';

FileFF::FileFF(const string& filespec)
: FeatureFunction(filespec)
, m_column(0)
{}

bool FileFF::parseAndCheckArgs()
{
   const string::size_type idx = argument.rfind(separator);
   m_filename = argument.substr(0, idx);
   if (idx != string::npos) {
      const string coldesc = argument.substr(idx+1);
      if (!conv(coldesc, m_column)) {
         // If we can't convert coldesc to a number, we assume the user simply
         // has a comma within the filename - an error message will be produced
         // later if that file doesn't exist, but it's not a syntax error.
         m_column = 0;
         m_filename = argument;
      } else
         --m_column;  // Convert to 0 based index
   }

   if (m_filename.empty()) {
      error(ETWarn, "You must provide a filename to FileFF");
      return false;
   }

   return true;
}

bool FileFF::loadModelsImpl()
{
   m_info = multiColumnFileFFManager::getManager().getUnit(m_filename);
   assert(m_info != 0);
   return m_info != 0;
}


//---------------------------------------------------------------------------
// FileDFF
//---------------------------------------------------------------------------
const char FileDFF::separator = ',';

FileDFF::FileDFF(const string& filespec)
: FeatureFunction(filespec)
, m_column(0)
{}

bool FileDFF::parseAndCheckArgs()
{
   const string::size_type idx = argument.rfind(separator);
   m_filename = argument.substr(0, idx);
   if (idx != string::npos) {
      const string coldesc = argument.substr(idx+1);
      if (!conv(coldesc, m_column)) {
         // If we can't convert coldesc to a number, we assume the user simply
         // has a comma within the filename - an error message will be produced
         // later if that file doesn't exist, but it's not a syntax error.
         m_column = 0;
         m_filename = argument;
      }
      // no need to decrement m_column here: in FileDFF m_column is 1-based,
      // not 0-based. -- 0 means no columns.
   }

   if (m_filename.empty()) {
      error(ETWarn, "You must provide a filename to FileFF");
      return false;
   }

   return true;
}

bool FileDFF::loadModelsImpl()
{
   vector<string> fields;
   FileReader::DynamicReader<string> dr(m_filename, 1);
   vector<string> gc;
   int prev_index = -1;
   while (dr.pollable())
   {
      Uint index(0);
      dr.poll(gc, &index);
      while (++prev_index < int(index))
         m_vals.push_back(vector<double>());
      const Uint K(gc.size());
      m_vals.push_back(vector<double>(K, 0.0f));
      for (Uint k(0); k<K; ++k)
      {
         string* convertme = &gc[k];
         if (m_column)
         {
            fields.clear();
            split(gc[k], fields, " \t\n\r");
            assert(fields.size() >= m_column);
            convertme = &fields[m_column-1];
         }

         if (!conv(*convertme, m_vals.back()[k]))
            error(ETFatal, "can't convert value to double: %s", (*convertme).c_str());
      }
   }
   return !m_vals.empty();
}

void FileDFF::source(Uint s, const Nbest * const nbest)
{
   if (s >= m_vals.size())
      error(ETFatal, "Too many source sentences %d/%d", s, m_vals.size());

   this->s = s;
}

double FileDFF::value(Uint k)
{
   if (s < m_vals.size())
   {
      if (k >= m_vals[s].size())
         error(ETFatal, "Not that many hypothesis for s: %d, k: %d/%d", s, k, m_vals[s].size());
   }
   else
      error(ETFatal, "Too many source sentences %d/%d", s, m_vals.size());

   return m_vals[s][k];
}



