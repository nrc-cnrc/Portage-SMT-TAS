/**
 * @author Samuel Larkin
 * @file printCopyright.h GTLI/ILTG copyright notice.
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
           
#ifndef __PRINT_COPYRIGHT_H__
#define __PRINT_COPYRIGHT_H__

#include <stdio.h>

namespace Portage {
   /**
    * Prints the GTLI/ILT copyright notice on the standard err.
    * @param startDate the creation date of the application.
    */
   inline void printCopyright(const unsigned int startDate, const char* progName)
   {
      if ( startDate < 2007 ) {
         fprintf(stderr, "\n%s, NRC-CNRC, (c) %d - 2007, Her Majesty in Right of Canada\n", progName, startDate);
      } else {
         fprintf(stderr, "\n%s, NRC-CNRC, (c) 2007, Her Majesty in Right of Canada\n", progName);
      }
   }
}

#endif // _PRINT_COPYRIGHT_H__
