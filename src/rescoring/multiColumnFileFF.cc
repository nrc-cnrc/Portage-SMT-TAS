/**
 * @author Samuel Larkin
 * @file multiColumnFileFF.cc  Implementation for the manager for file with multiple feature
 * functions.
 *
 * $Id$
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
 */
#include <multiColumnFileFF.h>

using namespace Portage;

multiColumnFileFFManager* multiColumnFileFFManager::m_singleton = 0;


////////////////////////////////////////
//
multiColumnFileFF::multiColumnFileFF(const std::string& filename)
: m_file(filename)
, m_line(-1)
{ }

multiColumnFileFF::~multiColumnFileFF()
{ }

float multiColumnFileFF::get(const Uint colIdx, const int k)
{
   // Are we processing a new line, then fill the array with new values
   // WHY a while and not an if => because we've hit some empty hypothesis and needs to skip some line to catch up.
   // Reason: computeFFMatrix skip empty hypothesis.
   while (k > m_line) {
      std::string line;
      std::vector<std::string> fields;
      
      std::getline(m_file, line);
      ++m_line;
      split(line, fields, " \t\n\r");
      // assert(fields.size() > 0);

      /**
       * Lines can be empty if the whole N-best list is empty which happens for empty input (source) sentences
       */
      if (fields.size() == 0)
        return 0;

      // Should be the first time we read data thus allocate once and for all our vector
      if (m_values.size() == 0) m_values.resize(fields.size());
      assert(m_values.size() == fields.size());

      for (unsigned int i(0); i<fields.size(); ++i)
         if (!conv(fields[i], m_values[i]))
            error(ETFatal, "can't convert value to a numerical value: %s", fields[i].c_str());
   }

   if (colIdx >= m_values.size())
      error(ETFatal, "Invalid column index in multi featured file: %d\n", colIdx);

   return m_values[colIdx];
}


////////////////////////////////////////
// multiCOlumnFFolumnFFfileManager
multiColumnFileFFManager::~multiColumnFileFFManager()
{
   //for (MIT mit(m_map.begin()); mit!=m_map.end(); ++mit)
   //   delete mit->second;
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
                                                                                       
