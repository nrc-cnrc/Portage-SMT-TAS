/**
 * @author Patrick Paul
 * @file echo_portage_api.cc  Implementation of PortageAPI
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
#define PORTAGE_ECHO_PADDLE
#include "portage_api.h"
#include <basicmodel.h>
#include <str_utils.h>
#include <phrasedecoder_model.h>
#include <hypothesisstack.h>
#include <decoder.h>
#include <wordgraph.h>

// These #defines in log4cxx conflict with variables named the same in ace, but
// we don't use them, so we just undef them here before including ace stuff.
#undef PACKAGE_VERSION
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME

#include <ace/Env_Value_T.h>

using namespace Portage;

PortageAPI::PortageAPI(const string& srclang, const string& tgtlang, const string& config,
		       bool models_in_demo_dir, bool verbose) 
{
}

void PortageAPI::translate(const string& src_text, string& tgt_text)
{
   tgt_text = src_text;
}
