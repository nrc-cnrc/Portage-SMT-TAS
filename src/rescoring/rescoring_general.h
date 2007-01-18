/**
 * @author Aaron Tikuisis
 * @file rescoring_general.h  Utilities used in rescoring module.
 *
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef RESCORING_GENERAL_H
#define RESCORING_GENERAL_H

#include <ctype.h>
#include <iostream>
#include <portage_defs.h>
#include <boostDef.h>

using namespace std;

#define MAX_LINE_LENGTH 2000
#define MAX_WORDS_PER_LINE 200

#define MAX(A,B) (A < B) ? B : A
#define MIN(A,B) (A < B) ? A : B

#define SQR(A) (A)*(A)

#ifdef DEBUG_ON
#define RSC_DEBUG(A) cout << A << endl;
#else
#define RSC_DEBUG(A)
#endif

inline void lowercase(char *str)
{
   for (;*str != '\0'; str++)
   {
      *str = tolower(*str);
   }
}

inline void uppercase(char *str)
{
   for (;*str != '\0'; str++)
   {
      *str = toupper(*str);
   }
}

inline unsigned int my_vector_max_index(const uVector& v)
{
   unsigned int result(0);
   for (unsigned int i(1); i<v.size(); ++i)
   {
      if (v(i) > v(result) || (isnan(v(result)) && !isnan(v(i))) )
      {
         result = i;
      }
   }
   return result;
}

inline double my_vector_max(const uVector& v)
{
    return v(my_vector_max_index(v));
}

inline unsigned int my_vector_min_index(const uVector& v)
{
   unsigned int result(0);
   for (unsigned int i(1); i<v.size(); ++i)
   {
      if (v(i) < v(result) || (isnan(v(result)) && !isnan(v(i))) )
      {
         result = i;
      }
   }
   return result;
}

inline double my_vector_min(const uVector& v)
{
    return v(my_vector_min_index(v));
}


#endif // RESCORING_GENERAL_H
