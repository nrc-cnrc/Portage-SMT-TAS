/**
 * @author Samuel Larkin
 * @file printCopyright.h GTLI/ILTG copyright notice.
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
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
      if ( startDate < 2006 ) {
         fprintf(stderr, "\n%s, Copyright (c) %d - 2006, Conseil national de recherches Canada / National Research Council Canada\n", progName, startDate);
      } else {
         fprintf(stderr, "\n%s, Copyright (c) 2006, Conseil national de recherches Canada / National Research Council Canada\n", progName, startDate);
      }
   }
}

#endif // _PRINT_COPYRIGHT_H__
