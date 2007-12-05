/**
 * @author Samuel Larkin
 * @file printCopyright.h GTLI/ILT copyright notice.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
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
    * @param progName  Program's name
    */
   inline void printCopyright(const unsigned int startDate,
                              const char* progName = NULL)
   {
      if ( progName ) {
         fprintf(stderr, "\n%s, ", progName);
      } else {
         fprintf(stderr, "\n");
      }

      const unsigned int thisYear(2007);
      if ( startDate < thisYear ) {
         fprintf(stderr,
            "NRC-CNRC, (c) %d - %d, Her Majesty in Right of Canada\n",
            startDate, thisYear);
      } else {
         fprintf(stderr,
            "NRC-CNRC, (c) %d, Her Majesty in Right of Canada\n",
            startDate);
      }

      fprintf(stderr, "\n");
   }
}

#endif // _PRINT_COPYRIGHT_H__
