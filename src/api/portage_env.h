/**
 * @author George Foster
 * @file portage_env.h  Specific function to get the PORTAGE variable from the linux environment.
 * 
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */
#ifndef PORTAGE_ENV_H
#define PORTAGE_ENV_H

#include <errors.h>

namespace Portage {

/**
 * Get the value of PORTAGE's environment variable.
 * @param die_unless_found  Specifies if we should halt the program if the
 * environment variable PORTAGE is not found
 * @return  Returns the value of PORTAGE if found or the empty string if not
 * and dies if not found and asked to die.
 */
inline string getPortage(bool die_unless_found = true) 
{
   const char* PORTAGE = getenv("PORTAGE");
   if (!PORTAGE && die_unless_found)
      error(ETFatal, "PORTAGE enviromnent variable not set");
   return PORTAGE ? (string)PORTAGE : "";
}
}

#endif
