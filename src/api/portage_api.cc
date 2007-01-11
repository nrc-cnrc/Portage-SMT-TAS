/**
 * @author George Foster
 * @file portage_api.cc  Portage API
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
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
                       const string& config, bool models_in_demo_dir,
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

   string model_dir = getPortage() + "/models/demo";
   string config_file = model_dir + "/" + config + "." + srclang + "2" + tgtlang;

   c.read(config_file.c_str());
   c.check();

   if (models_in_demo_dir) {
      for (Uint i = 0; i < c.backPhraseFiles.size(); ++i) {
         c.backPhraseFiles[i] = model_dir + "/" + c.backPhraseFiles[i];
         c.forPhraseFiles[i] = model_dir + "/" + c.forPhraseFiles[i];
      }
      for (Uint i = 0; i < c.multiProbTMFiles.size(); ++i)
         c.multiProbTMFiles[i] = model_dir + "/" + c.multiProbTMFiles[i];
      for (Uint i = 0; i < c.lmFiles.size(); ++i)
         c.lmFiles[i] = model_dir + "/" + c.lmFiles[i];
   }

   c.loadFirst = true;

   bmg = BasicModelGenerator::create(c);
}

void PortageAPI::translate(const string& src_text, string& tgt_text)
{
   prep.proc(src_text, src_sents);
   tgt_sents.resize(src_sents.size());

   for (Uint i = 0; i < src_sents.size(); ++i) {
      // should read trans markup here, but assume none & use empty cur_mark
      PhraseDecoderModel* pdm = bmg->createModel(src_sents[i], cur_mark);
      HypothesisStack* h = runDecoder(*pdm, c.maxStackSize,
                                      log(c.pruneThreshold),
                                      c.covLimit, log(c.covThreshold),
                                      c.distLimit, c.verbosity);
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
