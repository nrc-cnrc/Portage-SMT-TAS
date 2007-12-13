/**
 * @author Samuel Larkin
 * @file multiColumnFileFF.cc  Implementation for the manager for file with multiple feature
 * functions.
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */
#include <multiColumnFileFF.h>

using namespace Portage;

multiColumnFileFFManager* multiColumnFileFFManager::m_singleton = 0;


////////////////////////////////////////
//
multiColumnFileFF::multiColumnFileFF(const std::string& filename)
: m_file(filename)
, m_line(-1)
, m_expected_size(0)
{ }

multiColumnFileFF::~multiColumnFileFF()
{ }

float multiColumnFileFF::get(const Uint colIdx, const int k)
{
   using namespace std;
   static bool hasBeenWarned = false;

   // It is an error to ask for previous line
   // A user can only ask the same last read line or the future ones.
   assert(k >= m_line);

   // Are we processing a new line, then fill the array with new values.
   // WHY a while and not an if => because we've hit some empty hypothesis and
   // needs to skip some line to catch up.
   // Reason: computeFFMatrix skip empty hypothesis.
   while (k > m_line) {
      string line;
      
      if (getline(m_file, line).eof())
         error(ETFatal, "Premature end of file while reading ffval");

      ++m_line;
      if (splitCheckZ(line.c_str(), m_values) > 0) {
         if (m_expected_size == 0) {
            m_expected_size = m_values.size();
         }
         if (m_expected_size != m_values.size())
            error(ETFatal, "Multi-column FFVals files with inconsistent number"
                  " of columns at line %d: expected %d, got %d", 
                  m_line, m_expected_size, m_values.size());
      }
      else {
         if (!hasBeenWarned) {
            error(ETWarn, "Suspicious ffvals values, they shouldn't be empty!!");
            hasBeenWarned = true;
         }

         m_values.resize(m_expected_size);
         fill(m_values.begin(), m_values.end(), 0.0f);
      }
   }

   if (colIdx >= m_values.size())
      error(ETFatal, "Invalid column index in multi featured file: %d\n", colIdx);

   return m_values[colIdx];
}


////////////////////////////////////////
// multiCOlumnFFolumnFFfileManager
multiColumnFileFFManager::~multiColumnFileFFManager()
{
   m_map.clear();
}


multiColumnUnit multiColumnFileFFManager::getUnit(const std::string& filename)
{
   MIT mit = m_map.find(filename);
   if (mit == m_map.end()) {
      return m_map[filename] = multiColumnUnit(new multiColumnFileFF(filename));
   }
   else {
      return mit->second;
   }
}

