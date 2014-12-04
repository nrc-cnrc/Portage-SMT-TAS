/**
 * @author Samuel Larkin
 * @file printCopyright.h GTLI/ILT copyright notice.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
*/
           
#ifndef __PRINT_COPYRIGHT_H__
#define __PRINT_COPYRIGHT_H__

#include <cstdio>
#include <cstdlib>

namespace Portage {
   /**
    * Prints the GTLI/ILT copyright notice on the standard err.
    * @param startDate the creation date of the application.
    * @param progName  Program's name
    */
   inline void printCopyright(const unsigned int startDate,
                              const char* progName = NULL)
   {
      static const char* const quench_copyright = getenv("PORTAGE_INTERNAL_CALL");
      if ( quench_copyright ) return;

      if ( progName ) {
         fprintf(stderr, "\n%s, ", progName);
      } else {
         fprintf(stderr, "\n");
      }

      const unsigned int current_year(2014);
      if ( startDate < current_year ) {
         fprintf(stderr,
            "NRC-CNRC, (c) %d - %d, Her Majesty in Right of Canada\n",
            startDate, current_year);
      } else {
         fprintf(stderr,
            "NRC-CNRC, (c) %d, Her Majesty in Right of Canada\n",
            startDate);
      }

      fprintf(stderr, "Please run \"portage_info -notice\" for Copyright notices of 3rd party libraries.\n\n");
   }
}

#endif // _PRINT_COPYRIGHT_H__
