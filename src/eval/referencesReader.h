/**
 * @author Samuel Larkin
 * @file referencesReader.h  References reader allows to read fix/dinamic size blocks(lines) from multiple files.
 *
 * $id: referencesReader.h$
 * 
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Conseil national de recherches du Canada / Copyright 2006, National Research Council of Canada
 */

#ifndef __REFERENCES_READER_H__
#define __REFERENCES_READER_H__

#include <basic_data_structure.h>
#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

namespace Portage {
using namespace std;

/// Handles reading references from multiple files for a source sentence.
class referencesReader
{
   private:
      const unsigned int  m_R;       ///< Number of reference files
      vector<istream*>    m_ifRefs;  ///< m_R input streams

   public:
      /// Constructor
      /// @param sRefFiles  list of files containing the references to read.
      referencesReader(const vector<string>& sRefFiles);
      
      /// Destructor.
      virtual ~referencesReader();


      /**
       * Reads all references for a source.
       * @param gr  returned references.
       */
      void poll(References& gr);
      
      /**
       * Reads all references for all sources.
       * @param ar  returned references.
       * @param S   number of sources.
       */
      void poll(AllReferences& ar, const unsigned int S);


      /**
       * Makes sure we've read all reference files properly.
       * @param bAfterAWhile  indicates if we are checking integrity after
       * using referencesReader in a while loop.
       */
      void integrityCheck(bool bAfterAWhile = false);

      /**
       * Get the number of reference files thus the number of references per source.
       * @return Returns the number of reference files.
       */
      inline unsigned int getR() { return m_R; }
}; //ends class referencesReader
} // ends namespace Portage

#endif // __REFERENCES_READER_H__
