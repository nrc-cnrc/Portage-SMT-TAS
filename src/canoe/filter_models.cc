/**
 * @author Samuel Larkin
 * @file filter_models.cc  Program that filters TMs and LMs.
 *
 * $Id$
 *
 * LMs & TMs filtering
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include <filter_models.h>
#include <exception_dump.h>
#include <printCopyright.h>
#include <config_io.h>
#include <phrasetable_filter_grep.h>
#include <phrasetable_filter_joint.h>
#include <phrasetable_filter_lm.h>
#include "inputparser.h"
#include "basicmodel.h"
#include <lm.h>
#include <sstream>


using namespace std;
using namespace Portage;
using namespace Portage::filter_models;


/******************************************************************************
 Here's a graph of all possible LM/TM filtering

 +-LM-+-filter_LM
 |
-+    +-filter_grep
 |    |
 |    +-online--+-hard_limit-+-complete
 |    |         |            +-src-grep
 +-TM-+         +-soft_limit-+-complete
      |                      +-src-grep
      |
      +-general-+-hard_limit-+-complete
                |            +-src-grep
                +-soft_limit-+-complete
                             +-src-grep

 online  => not in memory, processing on the fly TMs must be sorted
 general => loaded in memory before processing
 hard_limit => hard_limit_weights -> exactly L or less entries are kept
 soft_limit => LimitTM in Badr et al, 2007
 complete => all TM entries are processed no filtering based on source sentences is done
 src-grep => source are required and are used to prefilter the phrase table entries
******************************************************************************/

/// Logger for filter_models
Logging::logger filter_models_Logger(Logging::getLogger("verbose.canoe.filter_models"));


void processOnline(const CanoeConfig& c, const ARG& arg,
   VocabFilter& tgt_vocab, const vector< vector<string> >& src_sents)
{
   LOG_VERBOSE1(filter_models_Logger, "Processing online/streaming");

   // In online mode there can only be one and only one multiTM and we must be
   // in either hard or soft mode
   if (!(arg.limit()
         && c.multiProbTMFiles.size() == 1
         && c.backPhraseFiles.empty()
         && c.forPhraseFiles.empty()))
   {
      error(ETFatal, "When using the tm-online mode there can only be one and only one multi-prob TM!");
   }

   // The limitPhrase must be false if the user requested filter complete
   const bool limitPhrases(!arg.no_src_grep);

   PhraseTableFilterJoint  phraseTable(limitPhrases, tgt_vocab, c.phraseTablePruneType.c_str(), arg.hard_limit ? &c.transWeights : NULL);
   phraseTable.outputForOnlineProcessing(arg.prepareFilename(arg.limit_file), c.phraseTableSizeLimit);
   // Parses the input source sentences
   if (limitPhrases) {
      LOG_VERBOSE1(filter_models_Logger, "Creating vocabulary from source sentences");
      // Enter all source phrases into the phrase table, and into the vocab
      // in case they are OOVs we need to copy through to the output.
      phraseTable.addSourceSentences(src_sents);
   }

   phraseTable.processMultiProb(c.multiProbTMFiles.front(), "");
}


/**
 * Program filter_models's entry point.
 * @return Returns 0 if successful.
 */
int MAIN(argc, argv)
{
   typedef vector<string> FileList;
   typedef FileList::iterator FL_iterator;

   printCopyright(2006, "filter_models");
   ARG arg(argc, argv);

   CanoeConfig c;
   c.read(arg.config.c_str());
   c.check();   // Check that the canoe.ini file is coherent
   // Make sure we are running in backward-weights mode
   if (arg.limit() && !c.forwardWeights.empty() && c.phraseTablePruneType != "backward-weights")
      error(ETFatal, "Filter models only supports filtering with backward-weights (requested: %s)\n\
           Add [ttable-prune-type] backward-weights to your canoe.ini", c.phraseTablePruneType.c_str());

   // Print the requested running mode from user
   if (arg.tm_online)   cerr << "  Running in online / streaming mode" << endl;
   else                 cerr << "  Running in load all in memory" << endl;
   if (arg.no_src_grep) cerr << "  Running without source sentences => processing all table entries" << endl;
   else                 cerr << "  Running with source sentences => filtering phrase table base on source phrases" << endl;
   if (arg.soft_limit)  cerr << "  Running in SOFT mode using limit(" << c.phraseTableSizeLimit << ")" << endl;
   if (arg.hard_limit)  cerr << "  Running in HARD mode using limit(" << c.phraseTableSizeLimit << ")" << endl;
   if (arg.tm_online)   error(ETWarn, "Be sure that your phrasetable is sorted before calling filter_models (LC_ALL=C)");


   // The limitPhrase must be false if the user requested filter complete
   const bool limitPhrases(!arg.no_src_grep);

   // Setting the value of log_almost_zero
   PhraseTable::log_almost_0 = c.phraseTableLogZero;

   // Prepares the source sentences
   vector< vector<string> > src_sents;
   if (!arg.no_src_grep)
   {
      LOG_VERBOSE1(filter_models_Logger, "Loading source sentences");
      string line;
      //IMagicStream is(in_file.size() ? in_file : "-");
      IMagicStream is("-");
      InputParser reader(is);
      vector<MarkedTranslation> dummy; // Required to call readMarkedSent()
      while (!reader.eof()) {
         src_sents.push_back(vector<string>());
         reader.readMarkedSent(src_sents.back(), dummy);
      }
      reader.reportWarningCounts();
      if ( !src_sents.empty() && src_sents.back().empty() )
         src_sents.pop_back();

      if (src_sents.empty())
         error(ETFatal, "No source sentences, nothing to do! (or did you forget to say -no-src-grep?");
      else
         cerr << "Using " << src_sents.size() << " source sentences" << endl;
   }


   // CHECKING consistency in user's request.
   LOG_VERBOSE1(filter_models_Logger, "Consistency check of user's request");
   // Logic checking
   if (arg.limit() && !(c.phraseTableSizeLimit > NO_SIZE_LIMIT))
      error(ETFatal, "You're using filter TM, please set a value greater then 0 to [ttable-limit]");
   // When using hard limit, user must provide the tms-weights
   if (arg.hard_limit && c.transWeights.empty())
      error(ETFatal, "You must provide the TMs weights when doing a hard_limit");
   if (src_sents.empty() && !arg.limit())
      error(ETFatal, "You must provide source sentences when doing grep");



   VocabFilter  tgt_vocab(src_sents.size());
   if (arg.tm_online) {
      processOnline(c, arg, tgt_vocab, src_sents);
   }
   else {
      // Creating what will be needed for filtering
      LOG_VERBOSE1(filter_models_Logger, "Creating the models");
      PhraseTableFilter*  phraseTable(NULL);
      if (arg.soft_limit)
         phraseTable = new PhraseTableFilterJoint(limitPhrases, tgt_vocab, c.phraseTablePruneType.c_str());
      else if (arg.hard_limit)
         phraseTable = new PhraseTableFilterJoint(limitPhrases, tgt_vocab, c.phraseTablePruneType.c_str(), &c.transWeights);
      else
         if (arg.doLMs)
            phraseTable = new PhraseTableFilterLM(limitPhrases, tgt_vocab, c.phraseTablePruneType.c_str());
         else
            phraseTable = new PhraseTableFilterGrep(limitPhrases, tgt_vocab, c.phraseTablePruneType.c_str());
      assert(phraseTable != NULL);


      // Parses the input source sentences
      if (limitPhrases) {
         LOG_VERBOSE1(filter_models_Logger, "Creating vocabulary from source sentences");
         // Enter all source phrases into the phrase table, and into the vocab
         // in case they are OOVs we need to copy through to the output.
         phraseTable->addSourceSentences(src_sents);
      }



      // Add TMtext to vocab and filter them
      if (!c.backPhraseFiles.empty()) LOG_VERBOSE1(filter_models_Logger, "Processing Single Probs");
      for (Uint i(0); i<c.backPhraseFiles.size(); ++i) {
         string bck, fbck;
         arg.getFilenames(c.backPhraseFiles[i], bck, fbck);

         if (!arg.doLMs && !arg.limit())
            c.backPhraseFiles[i] = arg.prepareFilename(c.backPhraseFiles[i]);

         if (!c.forPhraseFiles.empty()) {
            string fwd, ffwd;
            arg.getFilenames(c.forPhraseFiles[i], fwd, ffwd);

            if (!arg.doLMs && !arg.limit())
               c.forPhraseFiles[i] = arg.prepareFilename(c.forPhraseFiles[i]);

            phraseTable->processSingleProb(bck, fbck, fwd.c_str(), ffwd.c_str());
         }
         else {
            phraseTable->processSingleProb(bck, fbck);
         }
      }


      // Add multi-prob TMs to vocab and filter them
      if (!c.multiProbTMFiles.empty()) LOG_VERBOSE1(filter_models_Logger, "Processing multi-probs");
      for (FL_iterator it(c.multiProbTMFiles.begin()); it!=c.multiProbTMFiles.end(); ++it) {
         string bck, fbck;
         arg.getFilenames(*it, bck, fbck);
         LOG_VERBOSE2(filter_models_Logger, "bck: %s fbck:%s", bck.c_str(), fbck.c_str());
         phraseTable->processMultiProb(bck, fbck);
         if (!arg.doLMs && !arg.limit())
            *it = arg.prepareFilename(*it);
      }


      // Filtering phrase tables to limit them to c.phraseTableSizeLimit
      if (arg.limit()) {
         LOG_VERBOSE1(filter_models_Logger, "Filtering with filter_joint with: %d", c.phraseTableSizeLimit);
         time_t start_time = time(NULL);
         phraseTable->filter_joint(arg.prepareFilename(arg.limit_file), c.phraseTableSizeLimit);
         cerr << " ... done in " << (time(NULL) - start_time) << "s" << endl;
      }


      // Add LMs
      if (arg.doLMs) {
         if (!c.lmFiles.empty()) LOG_VERBOSE1(filter_models_Logger, "Processing Language Models");
         for (FL_iterator it(c.lmFiles.begin()); it!=c.lmFiles.end(); ++it) {
            string lm, flm;
            arg.getFilenames(*it, lm, flm);

            // Skip LMDBs.
            if ( lm.size() >= 5 && lm.compare(lm.size()-5, 5, ".lmdb") == 0 )
               continue;

            if (!check_if_exists(lm))
               error(ETFatal, "Cannot read from language model file %s", lm.c_str());

            cerr << "loading language model from " << lm << endl;
            time_t start_time = time(NULL);
            OMagicStream  os_filtered(flm);
            PLM *lm_model = PLM::Create(lm, &tgt_vocab, PLM::ClosedVoc,
                  LOG_ALMOST_0, limitPhrases, c.lmOrder, &os_filtered);
            cerr << " ... done in " << (time(NULL) - start_time) << "s" << endl;
            if (lm_model) delete lm_model;

            *it = arg.prepareFilename(*it);
         } // for
      }

      if (phraseTable) { delete phraseTable; phraseTable = NULL; }
   }


   // Print out the vocab if necessary
   if (arg.vocab_file.size() > 0) {
      cerr << "Dumping Vocab" << endl;
      if (tgt_vocab.per_sentence_vocab) {
         fprintf(stderr, "Average vocabulary size per source sentences: %f\n",
            tgt_vocab.per_sentence_vocab->averageVocabSizePerSentence());
      }
      OMagicStream os_vocab(arg.vocab_file);
      tgt_vocab.write(os_vocab);
   }


   // Builds a new canoe.ini with the modified models
   LOG_VERBOSE1(filter_models_Logger, "Creating new canoe.ini");
   const string configFile(addExtension(arg.config, arg.suffix));
   if (arg.limit()) {
      c.readStatus("ttable-file-t2s")   = false;
      c.readStatus("ttable-file-s2t")   = false;
      c.readStatus("ttable-multi-prob") = true;
      c.multiProbTMFiles.clear();  // We should end up with only one multi-prob
      c.multiProbTMFiles.push_back(arg.prepareFilename(arg.limit_file));
   }

   if (arg.output_config) {
      cerr << "New config file is: " << configFile << endl;
      c.write(configFile.c_str(), 1, true);
      //c.write(cerr, 1, true);
   }

} END_MAIN
