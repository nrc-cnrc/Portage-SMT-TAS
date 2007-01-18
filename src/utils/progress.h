/**
 * @author Samuel Larkin
 * @file progress.h  Simple progress bar.
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#ifndef __PROGRESS_BAR_H__
#define __PROGRESS_BAR_H__

#include <portage_defs.h>
#include <iostream>

namespace Portage {
/// Simple progress bar.
class Progress
{
   private:
      const Uint m_max;        ///< Maximum number of steps
      const bool m_bVerbose;   ///< Should the progress bar be displayed
      const Uint m_barSize;    ///< size of the progress bar in charaters
      Uint       m_progress;   ///< current displayed progress
      Uint       m_s;          ///< current step
      
   public:
      /**
       * Construtor.
       * @param max       maximum expected steps
       * @param bVerbose  indicates if the progress bar should be display
       * @param BarSize   on screen progress bar size
       */
      Progress(const Uint max, const bool bVerbose = false, const Uint BarSize = 75)
      : m_max(max)
      , m_bVerbose(bVerbose)
      , m_barSize(BarSize)
      , m_progress(0)
      , m_s(0)
      {}

      /// Prints out to standard error the progress bar's header.
      void displayBar() const
      {
         if (m_bVerbose)
         {
            cerr << "|";
            Uint i(1); 
            for (; i<m_barSize/2; ++i)
               cerr << "-";
            cerr << "+"; ++i;
            for (; i<m_barSize-1; ++i)
               cerr << "-";
            cerr << "|" << endl;
         }
      }
      /// Tells the progress bar that a step was made and the progress bar
      /// will update the display if needed.
      void step()
      {
         ++m_s;
         const Uint p(m_barSize*m_s/m_max); 
         if (m_bVerbose && (p > m_progress))
         {
            for (;m_progress<p && m_progress<m_barSize; ++m_progress)
               cerr << ".";
            if (m_progress >= m_barSize) cerr << endl;
         }
      }
};
} // ends namespace Portage 

#endif // __PROGRESS_BAR_H__
