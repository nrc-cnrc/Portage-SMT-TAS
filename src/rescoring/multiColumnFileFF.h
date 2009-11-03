/**
 * @author Samuel Larkin
 * @file multiColumnFileFF.h  Manager for file with multiple feature
 * functions.
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef __MULTI_COLUMN_FILE_FF_H_
#define __MULTI_COLUMN_FILE_FF_H_

#include <file_utils.h>  // iSafeMagicStream
#include <boostDef.h>
#include <str_utils.h>
#include <errors.h>
#include <map>
#include <string>
#include <boost/noncopyable.hpp>


namespace Portage {
/// Object that handle reading all feature function values for one hypothesis
class multiColumnFileFF
{
   private:
      iSafeMagicStream         m_file;    ///< The stream associated with this unit.
      std::vector<float>   m_values;  ///< Contains the last line's values.
      int                  m_line;    ///< Current line number.
      Uint                 m_expected_size;  ///< number of expected fields

   public:
      /// Constructor.
      multiColumnFileFF(const std::string& filename);
      /// Destructor.
      ~multiColumnFileFF();

      /**
       * Get feature function value at index (k, colIdx)
       * @param colIdx  column index
       * @param k       line number AKA s*K+k
       * @return Returns the feature function value for line k and column
       * colIdx.
       */
      float get(Uint colIdx, int k);

      /**
       * Verifies if this unit is at the end of the file.
       * @return true if unit is at end of file.
       */
      bool eof() { return m_file.peek() == EOF; }

   private:
      /// Disabled constructor.
      multiColumnFileFF();
};

/// Definition for multiColumnFileFF pointer.
typedef boost::shared_ptr<multiColumnFileFF> multiColumnUnit;

/// Multiple column unit manager.
class multiColumnFileFFManager : private noncopyable
{
   private:
      /// Definition that maps a file name to its multiple column unit.
      typedef std::map<std::string, multiColumnUnit> MAP;
      /// An iterator over map that maps file name to its multiple column
      /// unit.
      typedef MAP::iterator MIT;

      /// Keeps track of file name and their multiple column unit.
      MAP m_map;

      /// We only want one Manager so make it a singleton.
      static multiColumnFileFFManager* m_singleton;

   public:
      /// Destructor.
      ~multiColumnFileFFManager();
      /**
       * Get a multiple column unit for a specific file.
       * @param filename  file containing the multiple column feature
       * function file.
       * @return Return a multiple column unit for a specific file.
       */
      multiColumnUnit getUnit(const std::string& filename);

      /**
       * Get the multiple column file manager since it's a singleton.
       * @return Returns the only existing Manager for multiple column unit.
       */
      static multiColumnFileFFManager& getManager() {
         if (m_singleton == 0) {
            m_singleton = new multiColumnFileFFManager;
	    assert(m_singleton);
	    atexit(multiColumnFileFFManager::cleanup);
         }

         return *m_singleton;
      }
      static void cleanup() {
         if (m_singleton) delete m_singleton, m_singleton = NULL;
      }
};
} // ends namespace Portage

#endif // __MULTI_COLUMN_FILE_FF_H_
