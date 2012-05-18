/**
 * @author Eric Joanis
 * @file phrase_pair_extractor.cc
 * @brief Implementation of PhrasePairExtractor
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include "phrase_pair_extractor.h"
#include "hmm_aligner.h"
#include "arg_reader.h"
#include "word_align.h"
#include "ibm.h"
#include "phrase_table.h"

using namespace Portage;

void PhrasePairExtractor::dumpParameters() const
{
   cerr << "PhrasePairExtractor::dumpParameters" << endl
        << "check_args_called=" << check_args_called << endl
        << "max_phrase_string=" << max_phrase_string << endl
        << "max_phrase_len1=" << max_phrase_len1 << endl
        << "max_phrase_len2=" << max_phrase_len2 << endl
        << "max_phraselen_diff=" << max_phraselen_diff << endl
        << "min_phrase_string=" << min_phrase_string << endl
        << "min_phrase_len1=" << min_phrase_len1 << endl
        << "min_phrase_len2=" << min_phrase_len2 << endl
        << "align_methods=" << join(align_methods) << endl
        << "model1=" << model1 << endl
        << "model2=" << model2 << endl
        << "ibm_num=" << ibm_num << endl
        << "use_hmm=" << use_hmm << endl
        << "verbose=" << verbose << endl
        << "display_alignments=" << display_alignments << endl
        << "twist=" << twist << endl
        << "add_single_word_phrases=" << add_single_word_phrases << endl
        << "allow_linkless_pairs=" << allow_linkless_pairs << endl;
}

bool PhrasePairExtractor::getArgs(int argc, const char* const argv[], bool errors_are_fatal)
{
   const char* const switches[] = {
      "m:", "min:", "d:", "a:", "ibm:", "hmm", "p0:", "up0:", "v"
   };
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 2,
"Invalid option(s) for PhrasePairExtractor::getArgs() or canoe -leave-one-out.\n\
Supported gen_phrase_table options are -m, -min, -d, -a, -ibm, -hmm, -p0, -up0, -v,\n\
followed by the two required models, t|s and s|t",
   "-h", true);
   arg_reader.read(argc, argv);
   arg_reader.testAndSet("m", max_phrase_string);
   arg_reader.testAndSet("min", min_phrase_string);
   arg_reader.testAndSet("d", max_phraselen_diff);
   arg_reader.testAndSet("a", align_methods);
   arg_reader.testAndSet("ibm", ibm_num);
   arg_reader.testAndSet("hmm", use_hmm);
   arg_reader.testAndSet("p0", p0);
   arg_reader.testAndSet("up0", up0);

   if (arg_reader.getSwitch("v")) verbose = 1;

   arg_reader.testAndSet(0, "model1", model1);
   arg_reader.testAndSet(1, "model2", model2);

   return checkArgs(errors_are_fatal);
}

bool PhrasePairExtractor::getArgs(const string& args, const char* sep, bool errors_are_fatal)
{
   char buffer[args.size() + 1];
   strncpy(buffer, args.c_str(), args.size()+1);
   assert(buffer[args.size()] == '\0');
   char* argv[args.size()];
   Uint argc = destructive_split(buffer, argv, args.size(), sep);
   return getArgs(argc, argv, errors_are_fatal);
}

bool PhrasePairExtractor::checkArgs(bool errors_are_fatal)
{
   check_args_called = true;

   const ErrorType eType = errors_are_fatal ? ETFatal : ETWarn;
   bool ok = true;

   // initialize *_2 parameters from defaults if not explicitly set
   if (!p0_2) p0_2 = p0;
   if (!up0_2) up0_2 = up0;
   if (!alpha_2) alpha_2 = alpha;
   if (!lambda_2) lambda_2 = lambda;
   if (!max_jump_2) max_jump_2 = max_jump;
   if (!anchor_2) anchor_2 = anchor;
   if (!end_dist_2) end_dist_2 = end_dist;
   if (!start_dist_2) start_dist_2 = start_dist;
   if (!final_dist_2) final_dist_2 = final_dist;

   // If the user specified -[no]end-dist, that sets or resets both
   // start-dist and final-dist.
   if ( end_dist ) start_dist = final_dist = end_dist;
   if ( end_dist_2 ) start_dist_2 = final_dist_2 = end_dist_2;

   if ( ibm_num != 0 ) {
      if ( !check_if_exists(model1) || !check_if_exists(model2) ) {
         error(eType, "Models %s and/or %s do not exist.",
               model1.c_str(), model2.c_str());
         ok = false;
      }

      if ( ok && ibm_num == 42 && !use_hmm ) {
         // neither -hmm nor -ibm specified; default is IBM2 if .pos files
         // exist, or else HMM if .dist files exist, or error otherwise: we
         // never assume IBM1, because it is so seldom used, it's probably
         // an error; we want the user to assert its use explicitly.
         if ( check_if_exists(IBM2::posParamFileName(model1)) &&
               check_if_exists(IBM2::posParamFileName(model2)) )
            ibm_num = 2;
         else if ( check_if_exists(HMMAligner::distParamFileName(model1)) &&
               check_if_exists(HMMAligner::distParamFileName(model2)) )
            use_hmm = true;
         else {
            error(eType, "Models %s and/or %s are neither IBM2 nor HMM, specify -ibm N or -hmm explicitly.",
                  model1.c_str(), model2.c_str());
            ok = false;
         }
      }
   }

   if (!max_phrase_string.empty()) {
      vector<Uint> max_phrase_len;
      if (!split(max_phrase_string, max_phrase_len, ",") ||
          max_phrase_len.empty() || max_phrase_len.size() > 2) {
         error(eType, "bad argument for -m switch");
         ok = false;
      } else {
         max_phrase_len1 = max_phrase_len[0];
         max_phrase_len2 = max_phrase_len.size() == 2 ? max_phrase_len[1] : max_phrase_len[0];
      }
   }

   if (!min_phrase_string.empty()) {
      vector<Uint> min_phrase_len;
      if (!split(min_phrase_string, min_phrase_len, ",") ||
          min_phrase_len.empty() || min_phrase_len.size() > 2) {
         error(eType, "bad argument for -min switch");
         ok = false;
      } else {
         min_phrase_len1 = min_phrase_len[0];
         min_phrase_len2 = min_phrase_len.size() == 2 ? min_phrase_len[1] : min_phrase_len[0];
      }
   }

   if (max_phrase_len1 == 0) max_phrase_len1 = Uint(-1);
   if (max_phrase_len2 == 0) max_phrase_len2 = Uint(-1);
   if (min_phrase_len1 == 0) min_phrase_len1 = 1;
   if (min_phrase_len2 == 0) min_phrase_len2 = 1;
   if (ok && (min_phrase_len1 > max_phrase_len1 || min_phrase_len2 > max_phrase_len2)) {
      cerr << "Minimal phrase length is greater than the maximal one!" << endl
           << "lang1 : " << min_phrase_len1 << " " << max_phrase_len1 << endl
           << "lang2 : " << min_phrase_len2 << " " << max_phrase_len2 << endl;
      if ( errors_are_fatal )
         exit(1);
      else
         ok = false;
   }

   if (align_methods.empty())
      align_methods.push_back("IBMOchAligner");

   if (!allow_linkless_pairs)
      for (vector<string>::iterator it = align_methods.begin(); it != align_methods.end(); ++it)
         if (*it == "CartesianAligner")
            error(ETWarn, "CartesianAligner won't output anything without -ali.");

   return ok;
}

void PhrasePairExtractor::loadModels(bool createAligner)
{
   assert(check_args_called);
   
   if (use_hmm) {
      if (verbose)
         cerr << "Loading HMM models" << endl;
      ibm_1 = new HMMAligner(model1, p0, up0, alpha, lambda, anchor,
                             end_dist, max_jump);
      ibm_2 = new HMMAligner(model2, p0_2, up0_2, alpha_2, lambda_2, anchor_2,
                             end_dist_2, max_jump_2);
   } else if (ibm_num == 0) {
      if (verbose) cerr << "**Not** loading IBM models" << endl;
   } else if (ibm_num == 1) {
      if (verbose) cerr << "Loading IBM1 models" << endl;
      ibm_1 = new IBM1(model1);
      ibm_2 = new IBM1(model2);
   } else if (ibm_num == 2) {
      if (verbose) cerr << "Loading IBM2 models" << endl;
      ibm_1 = new IBM2(model1);
      ibm_2 = new IBM2(model2);
   } else
      error(ETFatal, "Invalid option: -ibm %d", ibm_num);
   if (verbose) cerr << "models loaded" << endl;

   aligner_factory = new WordAlignerFactory(
         ibm_1, ibm_2, verbose, twist,
         add_single_word_phrases, allow_linkless_pairs);
   for (Uint i = 0; createAligner and i < align_methods.size(); ++i)
      aligners.push_back(aligner_factory->createAligner(align_methods[i]));
}

void PhrasePairExtractor::extractPhrasePairs(
      const vector<string>& toks1, const vector<string>& toks2,
      vector< vector<Uint> >& sets1,
      const pair<Uint,Uint>* restrictSourcePhrase,
      PhraseTableUint& pt,
      //vector<PhrasePair>* phrase_pairs,
      WordAlignerStats* stats)
{
   assert(ibm_num == 0 || ibm_num == 42 || (ibm_1 && ibm_2));
   for (Uint i = 0; i < aligners.size(); ++i) {
      if (verbose > 1) cerr << "---" << align_methods[i] << "---" << endl;
      aligners[i]->align(toks1, toks2, sets1);
      if (stats) stats->tally(sets1, toks1.size(), toks2.size());

      if (verbose > 1) {
         cerr << "---" << endl;
         aligner_factory->showAlignment(toks1, toks2, sets1);
         cerr << "---" << endl;
      }

      assert(aligner_factory);
      aligner_factory->addPhrases(toks1, toks2, sets1,
                                  max_phrase_len1, max_phrase_len2,
                                  max_phraselen_diff,
                                  min_phrase_len1, min_phrase_len2,
                                  pt, 1u);
      if (verbose > 1) cerr << endl;
   }
}
