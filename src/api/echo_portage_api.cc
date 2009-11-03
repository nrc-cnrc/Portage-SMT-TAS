/**
 * @author Patrick Paul
 * @file echo_portage_api.cc  Implementation of PortageAPI
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#define PORTAGE_ECHO_PADDLE
#include "portage_api.h"

using namespace Portage;

PortageAPI::PortageAPI(const string& srclang, const string& tgtlang, const string& config,
		       bool models_in_demo_dir, bool verbose) 
{
}

void PortageAPI::translate(const string& src_text, string& tgt_text)
{
   tgt_text = src_text;
}
