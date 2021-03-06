/**
 * @author Samuel Larkin
 * @file filter_models.cc 
 * @brief Program that filters TMs and LMs.
 *
 * LMs & TMs filtering
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "filter_models.h"
#include "exception_dump.h"
#include "printCopyright.h"
#include "config_io.h"
#include "phrasetable_filter_grep.h"
#include "phrasetable_filter_joint.h"
#include "phrasetable_filter_lm.h"
#include "inputparser.h"
#include "basicmodel.h"
#include "lm.h"
#include "lmtext.h"  // LMText::isA
#include <sstream>


using namespace std;
using namespace Portage;
using namespace Portage::filter_models;


/******************************************************************************
 Here's a graph of all possible LM/TM filtering

 ,-LM-+-filter_LM
 |
-+    ,-filter_grep
 |    |
 |    +-online--+-tm_hard_limit-+-complete
 |    |         |               `-src-grep
 `-TM-+         `-tm_soft_limit-+-complete
      |                         `-src-grep
      |
      `-general-+-tm_hard_limit-+-complete
                |               `-src-grep
                `-tm_soft_limit-+-complete
                                `-src-grep

 online  => not in memory, processing on the fly TMs must be sorted
 general => loaded in memory before processing
 tm_hard_limit => tm_hard_limit_weights -> exactly L or less entries are kept
 tm_soft_limit => LimitTM in Badr et al, 2007
 complete => all TM entries are processed no filtering based on source sentences is done
 src-grep => source are required and are used to prefilter the phrase table entries
******************************************************************************/

/// Logger for filter_models
Logging::logger filter_models_Logger(Logging::getLogger("verbose.canoe.filter_models"));


/**
 *
 * @return Returns true if a filtered CPT was created.
 */
bool processOnline(const CanoeConfig& c, const ARG& arg,
   VocabFilter& tgt_vocab, const VectorPSrcSent& src_sents)
{
   LOG_VERBOSE1(filter_models_Logger, "Processing online/streaming");

   // In online mode there can only be one multiTM and we must be
   // in either hard or soft mode
   if (!(arg.limit() && c.multiProbTMFiles.size() == 1))
      error(ETFatal, "When using the tm-online mode there can only be one multi-prob TM!");

   const string filename = c.multiProbTMFiles.front();

   // What's the final cpt filename.
   const string filtered_cpt_filename = arg.prepareFilename(arg.limit_file);

   // If the filtered TM exists there is nothing to do.
   if (arg.isReadOnly(filtered_cpt_filename)) {
      error(ETWarn, "Translation Model %s has already been filtered.  Skipping...", filename.c_str());
      return false;
   }
   else if (arg.isReadOnlyOnDisk(filtered_cpt_filename)) {
      error(ETWarn,
            "Cannot filter %s since %s is read-only.",
            filename.c_str(),
            filtered_cpt_filename.c_str());
      return false;
   }


   error_unless_exists(filename, true, "translation model");

   // Instanciate the proper pruning style.
   const pruningStyle* pruning_style = pruningStyle::create(arg.pruning_strategy_switch, c.phraseTableSizeLimit);
   assert(pruning_style);

   PhraseTableFilterJoint  phraseTable(
         arg.limitPhrases(),
         tgt_vocab,
         pruning_style,
         arg.phraseTablePruneType,
         c.forwardWeights,
         c.transWeights,
         arg.tm_hard_limit,
         c.appendJointCounts);

   // Parses the input source sentences
   phraseTable.addSourceSentences(src_sents);

   // Do the actual online filtering.
   phraseTable.filter(filename, filtered_cpt_filename);

   delete pruning_style; pruning_style = NULL;

   return true;
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

   if (arg.phraseTablePruneType.empty())
      arg.phraseTablePruneType = c.phraseTablePruneType;

   const Uint ttable_limit_from_config = c.phraseTableSizeLimit; // save for write-back
   if (arg.ttable_limit >= 0) // use ttable_limit for pruning, instead of config file value
      c.phraseTableSizeLimit = Uint(arg.ttable_limit);

   if (arg.limit() && arg.phraseTablePruneType == "full") {
      error(ETWarn, "filter_models -tm-soft-limit or -tm-hard-limit implements \"-ttable-prune-type full\" approximately.\n");

      if (arg.ttable_limit < 0 && arg.pruning_strategy_switch.empty()) {
         c.phraseTableSizeLimit = (1.0 + arg.full_prune_extra/100.0) * c.phraseTableSizeLimit;
         error(ETWarn, "Increasing -ttable-limit by %u%% -- to %u -- to compensate for approximated \"full\" implementation.",
               arg.full_prune_extra, c.phraseTableSizeLimit);
      }
   }

   if (arg.filterLDMs and c.LDMFiles.empty()) {
      error(ETWarn, "You asked to filter lexicalized DMs but your config file doesn't contain any.");
      // Disable filtering LDMs since there are no LDMs in the canoe.ini file.
      arg.filterLDMs = false;
   }

   // Print the requested running mode from user
   if (arg.verbose) {
      if (arg.tm_online)   cerr << "  Running in online / streaming mode" << endl;
      else                 cerr << "  Running in load all in memory" << endl;
      if (arg.no_src_grep) cerr << "  Running without source sentences => processing all table entries" << endl;
      else                 cerr << "  Running with source sentences => filtering phrase table base on source phrases" << endl;
      if (arg.tm_soft_limit)  cerr << "  Running in SOFT mode using limit(" << c.phraseTableSizeLimit << ")" << endl;
      if (arg.tm_hard_limit)  cerr << "  Running in HARD mode using limit(" << c.phraseTableSizeLimit << ")" << endl;
   }
   if (arg.tm_online)   error(ETWarn, "Be sure that your phrasetable is sorted before calling filter_models (LC_ALL=C)");


   // Setting the value of log_almost_zero
   PhraseTable::log_almost_0 = c.phraseTableLogZero;

   // Prepares the source sentences
   VectorPSrcSent src_sents;
   if (arg.limitPhrases()) {
      LOG_VERBOSE1(filter_models_Logger, "Loading source sentences");
      string line;
      iSafeMagicStream is(arg.input);
      InputParser reader(is);
      PSrcSent ss;
      while (ss = reader.getMarkedSent())
         src_sents.push_back(ss);

      if (src_sents.empty())
         error(ETFatal, "No source sentences, nothing to do! (or did you forget to say -no-src-grep?");
      else
         cerr << "Using " << src_sents.size() << " source sentences" << endl;
   }


   // CHECKING consistency in user's request.
   LOG_VERBOSE1(filter_models_Logger, "Consistency check of user's request");
   // Logic checking
   if (arg.limit() and !c.allNonMultiProbPTs.empty())
      error(ETFatal, "You can't use limit aka filter30 with ttables other than multi-prob ones");
   if (arg.limit() and !(c.phraseTableSizeLimit > NO_SIZE_LIMIT))
      error(ETFatal, "You're using filter TM, please set a value greater than 0 to [ttable-limit]");
   // When using tm hard limit, user must provide the tms-weights
   if (arg.tm_hard_limit and c.transWeights.empty())
      error(ETFatal, "You must provide the TMs weights when doing a tm_hard_limit");
   if (src_sents.empty() and arg.limitPhrases())
      error(ETFatal, "You must provide source sentences when doing grep");
   if (arg.filterLMs and !c.allNonMultiProbPTs.empty())
      error(ETFatal, "Filtering Language Models (-L) only works for multi-prob phrase tables");
   if (arg.filterLDMs and (!(arg.limit() or c.multiProbTMFiles.size() == 1))) {
      if (arg.limitPhrases())
         error(ETWarn, "Filtering LDMs with multiple CPTs and no hard/soft filtering; taking only source phrases into account for LDM filtering, not target phrases.");
      else
         error(ETFatal, "To filter LDMs, we must either have one cpt or use hard/soft filtering to produce one cpt before filtering LDMs.  Alternatively, use filter-grep mode, in which case LDM filtering takes into account source phrases, but not target phrases, as it would normally.");
   }
   if (arg.filterLDMs and !c.allNonMultiProbPTs.empty()) {
      if (arg.limitPhrases())
         error(ETWarn, "Filtering LDMs when using phrase tables other than multi-prob takes only source phrases into account for LDM filtering, not target phrases.");
      else
         error(ETFatal, "Filtering LDMs when using phrase tables other than multi-prob and not limiting phrases is not supported.");
   }


   // online mode only applies to tm_hard_limit or tm_soft_limit.
   if (!arg.limit()) arg.tm_online = false;

   // hack - rely on a side effect of maxSourceSentence4filtering to disable
   // per-sentence filtering
   if ( arg.nopersent ) VocabFilter::maxSourceSentence4filtering = 1;


   ////////////////////////////////////////
   // Changing what to do based on what the user asked for and what has already been done.
   if (arg.limit()) {
      // What's the final cpt filename.
      const string filtered_cpt_filename = arg.prepareFilename(arg.limit_file);

      if (check_if_exists(filtered_cpt_filename)) {
         if (arg.readonly) {
            error(ETWarn, "Translation Model %s has already been filtered.  Skipping...", filtered_cpt_filename.c_str());
            // The filtered TM already exists thus we don't want to redo the
            // work.  We will disable soft/hard filtering and also disable grep
            // filtering.  If the user requested LM filtering, the TM's
            // vocabulary will be loaded.
            arg.tm_soft_limit  = false;
            arg.tm_hard_limit  = false;
            arg.tm_online   = false;
            arg.no_src_grep = true;

            // Change the config file to reflect the new filtered TM.
            c.readStatus("ttable-multi-prob") = true;
            c.multiProbTMFiles.clear();  // We should end up with only one multi-prob
            c.multiProbTMFiles.push_back(filtered_cpt_filename);
         }
         // The filtered model exists but it's read-only and we are supposed to overwrite it.
         // We can't proceed since we should be filtering a new model and thus
         // loading the vocabulary for potentially filtering LMs & LDMs.
         else if (arg.isReadOnlyOnDisk(filtered_cpt_filename)) {
            error(ETFatal,
                  "Incoherent scenario, a filtered TM exists (%s), is read-only but user want to overwrite.  Cannot proceed....",
                  filtered_cpt_filename.c_str());
         }
         // The filtered model exists and it's not readonly in any shape or form thus we will generate a new one.
         else {
            error(ETWarn,
                  "A filtered TM exists (%s) but we have no indication from the user that we shouldn't overwrite it.  Overwriting...",
                  filtered_cpt_filename.c_str());
         }
      }
   }
   else if (arg.limitPhrases()) {
      if (c.multiProbTMFiles.size() == 1) {
         string& file = c.multiProbTMFiles.front();
         const string translationModelFilename = file;
         const string filteredTranslationModelFilename = arg.prepareFilename(translationModelFilename);

         error_unless_exists(translationModelFilename, true, "translation model");

         if (arg.readonly && check_if_exists(filteredTranslationModelFilename)) {
            // The filtered version exists let's use the much faster tm vocab loading mechanism with PhraseTable_filter_lm.
            if (arg.filterLMs) arg.no_src_grep = true;
            file = filteredTranslationModelFilename;
            error(ETWarn, "Translation Model %s has already been filtered.  Skipping...",
                  filteredTranslationModelFilename.c_str());
         }
         else if (arg.isReadOnlyOnDisk(filteredTranslationModelFilename)) {
            error(ETFatal, "Cannot filter %s since %s is read-only.",
                  translationModelFilename.c_str(),
                  filteredTranslationModelFilename.c_str());
         }
      }
      /*
      else
         if (arg.filterLDMs)
            error(ETFatal, "It is impossible to either filter LDMs in source grep mode unless you only have one TM.");
      */
   }


   VocabFilter tgt_vocab(src_sents.size());
   GlobalVoc::set(&tgt_vocab);


   ////////////////////////////////////////
   // Filter Conditional Phrase Tables.
   bool weve_created_a_cpt = false;
   // We are doing either soft or hard filtering.
   if (arg.limit()) {
      // What's the final cpt filename.
      const string filtered_cpt_filename = arg.prepareFilename(arg.limit_file);

      if (arg.tm_online) {
         weve_created_a_cpt = processOnline(c, arg, tgt_vocab, src_sents);
      }
      else {
         // Creating what will be needed for filtering
         LOG_VERBOSE1(filter_models_Logger, "Creating the models");

         // Get the proper pruning specifier.
         const pruningStyle* pruning_style = pruningStyle::create(arg.pruning_strategy_switch, c.phraseTableSizeLimit);
         assert(pruning_style != NULL);

         PhraseTableFilterJoint  phraseTable(
               arg.limitPhrases(),
               tgt_vocab,
               pruning_style,
               arg.phraseTablePruneType,
               c.forwardWeights,
               c.transWeights,
               arg.tm_hard_limit,
               c.appendJointCounts);

         // Parses the input source sentences
         phraseTable.addSourceSentences(src_sents);


         // Read in memory all Translation Models.
         // Side effect: load vocabulary into tgt_vocab.
         LOG_VERBOSE1(filter_models_Logger, "Processing multi-probs");
         for (FL_iterator file(c.multiProbTMFiles.begin()); file!=c.multiProbTMFiles.end(); ++file) {
            LOG_VERBOSE2(filter_models_Logger,
                  "Translation Model filename: %s", file->c_str());

            error_unless_exists(*file, true, "translation model");

            // If there exists a filtered version of *file, use it instead.
            phraseTable.readMultiProb(*file);
         }

         LOG_VERBOSE1(filter_models_Logger, "Filtering with filter_joint with: %d", c.phraseTableSizeLimit);
         const time_t start_time = time(NULL);

         // Perform hard or soft filtering on the in-memory trie strucutre and writes it to disk.
         phraseTable.filter(filtered_cpt_filename);

         weve_created_a_cpt = true;

         delete pruning_style; pruning_style = NULL;

         if (arg.verbose) {
            cerr << " ... done in " << (time(NULL) - start_time) << "s" << endl;
         }
      }

      // Change the config file to reflect the new filtered TM.
      c.readStatus("ttable-multi-prob") = true;
      c.multiProbTMFiles.clear();  // We should end up with only one multi-prob
      c.multiProbTMFiles.push_back(filtered_cpt_filename);
   }
   // This is grep filtering.
   else if (arg.limitPhrases()) {
      // Creating what will be needed for filtering
      LOG_VERBOSE1(filter_models_Logger, "Creating the models");
      PhraseTableFilterGrep phraseTable(arg.limitPhrases(), tgt_vocab,
                                        arg.phraseTablePruneType, c.appendJointCounts);

      // Parses the input source sentences
      phraseTable.addSourceSentences(src_sents);


      // Add multi-prob TMs to vocab and filter them
      LOG_VERBOSE1(filter_models_Logger, "Processing multi-probs");
      for (FL_iterator file(c.multiProbTMFiles.begin()); file!=c.multiProbTMFiles.end(); ++file) {
         const string translationModelFilename = *file;
         const string filteredTranslationModelFilename = arg.prepareFilename(translationModelFilename);

         LOG_VERBOSE2(filter_models_Logger,
               "Translation Model filename: %s filtered Translation Model filename: %s",
               translationModelFilename.c_str(),
               filteredTranslationModelFilename.c_str());

         error_unless_exists(translationModelFilename, true, "translation model");

         if (arg.readonly && check_if_exists(filteredTranslationModelFilename)) {
            error(ETWarn, "Translation Model %s has already been filtered.  Skipping...",
                  filteredTranslationModelFilename.c_str());
         }
         else if (arg.isReadOnlyOnDisk(filteredTranslationModelFilename)) {
            error(ETWarn, "Cannot filter %s since %s is read-only.",
                  translationModelFilename.c_str(),
                  filteredTranslationModelFilename.c_str());
         }
         else {
            phraseTable.filter(translationModelFilename, filteredTranslationModelFilename);
            weve_created_a_cpt = true;
         }

         *file = filteredTranslationModelFilename;
      }
   }
   // Reading the vocabulary in the TMs in order to filter LMs.
   // Here, we don't create a new TM.
   else if (arg.filterLMs) {
      // PhraseTableFilterLM will not load the phrase table in memory but it will simply populate tgt_vocab.
      PhraseTableFilterLM phraseTable(arg.limitPhrases(),
                                      tgt_vocab, arg.phraseTablePruneType, c.appendJointCounts);

      // Parses the input source sentences
      phraseTable.addSourceSentences(src_sents);

      // Add multi-prob TMs to vocab and filter them
      if (!c.multiProbTMFiles.empty()) LOG_VERBOSE1(filter_models_Logger, "Processing multi-probs");
      for (FL_iterator file(c.multiProbTMFiles.begin()); file!=c.multiProbTMFiles.end(); ++file) {
         LOG_VERBOSE2(filter_models_Logger,
               "Reading in %s's vocab to filter the LMs.",
               file->c_str());

         error_unless_exists(*file, true, "translation model");

         string filename, dummy;
         arg.getFilenames(*file, filename, dummy);
         // This function call doesn't actually load the LM in memory but
         // rather populates the tgt_vocab for us.
         phraseTable.readMultiProb(filename);
      }
   }
   else {
      LOG_VERBOSE2(filter_models_Logger, "No filtering of TM was performed.");
   }



   ////////////////////////////////////////
   // Filter Language Models.
   // In order to filter LMs, we need tgt_vocab to be populated at this point.
   if (arg.filterLMs and !c.lmFiles.empty()) {
      // We must get the vocab from the TPPTs first
      if ( !c.allNonMultiProbPTs.empty() && arg.limitPhrases() && c.lmFiles.size() > 0 ) {
         LOG_VERBOSE1(filter_models_Logger, "Extracting vocabulary from TPPTs");
         const time_t start_time = time(NULL);
         if (arg.verbose)
            cerr << "Extracting target vocabulary from TPPTs";
         PhraseTable  phraseTable(tgt_vocab, NULL, c.appendJointCounts);
         phraseTable.extractVocabFromTPPTs(0);  // Will extract the TPPT voc into tgt_vocab.
         if (arg.verbose) {
            cerr << " ... done in " << (time(NULL) - start_time) << "s" << endl;
         }
      }

      // At this point, we must have a populated voc or else filtering will remove everything.
      assert(!tgt_vocab.empty());

      LOG_VERBOSE1(filter_models_Logger, "Processing Language Models");
      for (FL_iterator file(c.lmFiles.begin()); file!=c.lmFiles.end(); ++file) {
         // Extract the physical file name.
         const size_t hash_pos = file->rfind(PLM::lm_order_separator);
         const string lm  = file->substr(0, hash_pos);
         const string option = (hash_pos != string::npos ? file->substr(hash_pos+1) : "");
         const string flm = arg.prepareFilename(lm);

         // Let's skip Language Model types that we cannot filter.
         if (!LMText::isA(lm)) {
            error(ETWarn, "Cannot filter Language Models of types other than ARPA LM!  Skipping %s...", lm.c_str());
            continue;
         }

         error_unless_exists(lm, true, "language model");

         if (arg.readonly && check_if_exists(flm)) {
            error(ETWarn, "Language Model %s has already been filtered.  Skipping...", lm.c_str());
         }
         else if (arg.isReadOnlyOnDisk(flm)) {
            error(ETWarn, "Cannot filter %s since %s is read-only.", lm.c_str(), flm.c_str());
         }
         else {
            if (arg.verbose) {
               cerr << "loading Language Model from " << lm << " to " << flm << endl;
            }
            const time_t start_time = time(NULL);
            oSafeMagicStream  os_filtered(flm);
            const PLM *lm_model = PLM::Create(lm, &tgt_vocab, PLM::ClosedVoc,
                  LOG_ALMOST_0, arg.limitPhrases(), c.lmOrder, &os_filtered);
            if (arg.verbose) {
               cerr << " ... done in " << (time(NULL) - start_time) << "s" << endl;
            }
            if (lm_model) { delete lm_model; lm_model = NULL; }

            *file = flm + option;
         }
      }
   }



   ////////////////////////////////////////
   // Filter Lexicalized Distortion Models.
   if (arg.filterLDMs && !c.LDMFiles.empty()) {
      LOG_VERBOSE1(filter_models_Logger, "Processing Lexicalized Distortion Models");

      // There can only be one cpt to filter ldms.
      if (c.multiProbTMFiles.size() != 1 or !c.allNonMultiProbPTs.empty()) {
         if (arg.limitPhrases())
            error(ETWarn, "There is more than one CPT or there are other types of phrase tables, so LDM filtering can't look at target phrases; doing filter-grep only on the LDM(s).");
         else
            error(ETFatal, "In order to filter LDMs, we must have only one CPT and no TPPTs, or do filter-grep.");
      }

      const string cpt_filename = c.multiProbTMFiles.front();
      for (FL_iterator file(c.LDMFiles.begin()); file!=c.LDMFiles.end(); ++file) {
         // We don't know how to filter TPLDMs, let's skip it.
         if (isSuffix(".tpldm", *file)) {
            error(ETWarn, "Cannot filter Lexicalized Distortion Models of TPLDM type (%s)!  Skipping...", file->c_str());
            continue;
         }

         error_unless_exists(*file, true, "Lexicalized Distortion Model");

         // Don't redo work if there exists a filtered ldm.
         const string filtered_ldm = arg.prepareFilename(*file, arg.preserve_ldm_paths);
         if (arg.readonly && check_if_exists(filtered_ldm)) {
            if (weve_created_a_cpt)
               error(ETFatal, "%s hasn't been filtered even though a new cpt was created (%s) since you asked not to overwrite models.",
                     file->c_str(),
                     cpt_filename.c_str());
            else
               error(ETWarn, "Lexicalized Distortion Model %s has already been filtered. Skipping...", file->c_str());
         }
         else if (arg.isReadOnlyOnDisk(filtered_ldm)) {
            error(ETWarn, "Cannot filter %s since %s is read-only.", file->c_str(), filtered_ldm.c_str());
         }
         else {
            if (c.multiProbTMFiles.size() != 1 || !c.allNonMultiProbPTs.empty()) {
               assert(arg.limitPhrases());
               PhraseTableFilterGrep phraseTable(arg.limitPhrases(),
                                                 tgt_vocab, arg.phraseTablePruneType, 
                                                 c.appendJointCounts);
               phraseTable.addSourceSentences(src_sents);
               error_unless_exists(*file, true, "LDM");
               phraseTable.filter(*file, filtered_ldm);
               const string bkoff_file = (isZipFile(*file) ? removeExtension(*file) : *file) + ".bkoff";
               const string filt_bkoff_file = (isZipFile(filtered_ldm) ? removeExtension(filtered_ldm) : filtered_ldm) + ".bkoff";
               iSafeMagicStream bk_in(bkoff_file.c_str());
               oSafeMagicStream bk_out(filt_bkoff_file.c_str());
               string line;
               while (getline(bk_in, line))
                  bk_out << line;
            } else {
               // filter-distortion-model.pl -v ${CPT} ${LDM} ${FLDM}
               const string cmd = "filter-distortion-model.pl -v " + cpt_filename
                                  + " " + *file
                                  + " " + filtered_ldm;
               if (arg.verbose)
                  cerr << "Filtering LDM using: " << cmd << endl;  // SAM DEBUGGING
               const int rc = system(cmd.c_str());
               if (rc != 0)
                  error(ETFatal, "Error filtering Lexicalized Distortion Model with filter-distortion-model.pl! (rc=%d)", rc);
               // NOTE: the associated .bkoff for the newly filtered LDM will be created by filter-distortion-model.pl.
            }
         }

         // Replace unfiltered ldm in config file with the filtered one.
         *file = filtered_ldm;
      }
   }


   // Print out the vocab if necessary
   if (arg.vocab_file.size() > 0) {
      if (arg.verbose) {
         cerr << "Dumping Vocab" << endl;
      }
      if (tgt_vocab.per_sentence_vocab) {
         fprintf(stderr, "Average vocabulary size per source sentences: %f\n",
                 tgt_vocab.per_sentence_vocab->averageVocabSizePerSentence());
      }
      oSafeMagicStream os_vocab(arg.vocab_file);
      tgt_vocab.write(os_vocab);
   }


   // Builds a new canoe.ini with the modified models
   LOG_VERBOSE1(filter_models_Logger, "Creating new canoe.ini");
   const string configFile(addExtension(arg.config, arg.suffix));

   if (arg.ttable_limit >= 0)      // restore original configfile limit
      c.phraseTableSizeLimit = ttable_limit_from_config;

   if (arg.output_config) {
      if (arg.verbose) {
         cerr << "New config file is: " << configFile << endl;
      }
      c.write(configFile.c_str(), 1, true);
   }

} END_MAIN
