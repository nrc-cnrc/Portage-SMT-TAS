/**
 * @author George Foster
 * @file portage_api.cc  Portage API
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
#include "portage_api.h"
#include "portage_env.h"
#include <basicmodel.h>
#include <config_io.h>
#include <str_utils.h>
#include <phrasedecoder_model.h>
#include <hypothesisstack.h>
#include <decoder.h>
#include <wordgraph.h>

using namespace Portage;

PortageAPI::PortageAPI(const string& srclang, const string& tgtlang,
                       const string& config, bool config_in_demo_dir,
                       bool verbose) :
   prep(srclang), post(tgtlang, verbose)
{
   // Default canoe parameters, overwritten by ones read in from config file.
   // For the demo, we never want NO_SIZE_LIMIT!
   c.phraseTableSizeLimit       = 30;
   // for the demo, we also want smaller stacks by default
   c.maxStackSize               = 70;
   c.pruneThreshold             = 0.001;
   c.distLimit                  = 7;

   c.verbosity = 0;

   string config_file = config + "." + srclang + "2" + tgtlang;
   if ( config_in_demo_dir ) {
      string model_dir = getPortage() + "/models/demo";
      config_file = model_dir + "/" + config_file;
   }

   c.read(config_file.c_str());
   c.check();

   c.loadFirst = true;

   bmg = BasicModelGenerator::create(c);
}

void PortageAPI::translate(const string& src_text, string& tgt_text)
{
   prep.proc(src_text, src_sents);
   tgt_sents.resize(src_sents.size());

   for (Uint i = 0; i < src_sents.size(); ++i) {
      // should read trans markup here, but assume none & use empty cur_mark
      BasicModel* pdm = bmg->createModel(src_sents[i], cur_mark);
      HypothesisStack* h = runDecoder(*pdm, c);
      assert(!h->isEmpty());
      PrintPhraseOnly print(*pdm);

      DecoderState *cur = h->pop();
      DecoderState *travBack = cur;
      vector<DecoderState *> stack;
      while (travBack->back != NULL) {
	stack.push_back(travBack);
	travBack = travBack->back;
      }

      tgt_sents[i].clear();
      for (vector<DecoderState *>::reverse_iterator it = stack.rbegin(); it != stack.rend(); it++) {
	 tgt_sents[i].push_back(print(*it));
      }

      // could add fancy stuff based on verbosity level here

      delete h;
      delete pdm;
   }
   post.proc(tgt_sents, tgt_text);
}
