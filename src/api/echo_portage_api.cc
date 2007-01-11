/**
 * @author Patrick Paul
 * @file echo_portage_api.cc  Implementation of PortageAPI
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */
#define PORTAGE_ECHO_PADDLE
#include "portage_api.h"
#include <basicmodel.h>
#include <str_utils.h>
#include <phrasedecoder_model.h>
#include <hypothesisstack.h>
#include <decoder.h>
#include <wordgraph.h>

using namespace Portage;

PortageAPI::PortageAPI(const string& srclang, const string& tgtlang, const string& config,
		       bool models_in_demo_dir, bool verbose) 
{
}

void PortageAPI::translate(const string& src_text, string& tgt_text)
{
   tgt_text = src_text;
}
