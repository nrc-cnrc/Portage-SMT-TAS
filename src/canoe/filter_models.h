/**
 * @author Samuel Larkin
 * @file filter_models.h  Program that filters TMs and LMs.
 *
 * $Id$
 *
 * LMs & TMs filtering
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef __FILTERED_MODELS_H__
#define __FILTERED_MODELS_H__

#include <argProcessor.h>
#include <string>
#include "file_utils.h"


namespace Portage
{
   /// Program filter_models' related stuff.
   namespace filter_models
   {
       /// Program filter_models' usage.
       static char help_message[] = "\n\
filter_models [-f config] [options] [-no-src-grep | < src]\n\
\n\
Filter models used by a canoe config file. Filtered versions of the models are\n\
created in the current directory, by default, along with a new canoe config\n\
file <config>.FILT that refers to them. Filtering currently applies to TMs\n\
(phrasetables), DMs, and LMs, as follows:\n\
\n\
TM filtering is controlled by 3 separate parameters:\n\
1) Unless -no-src-grep is given, phrase pairs are filtered to match a source\n\
   text supplied on stdin.\n\
2) For each retained source phrase, a set of allowed translations is determined\n\
   by either hard filtering (-tm-hard-limit), soft filtering (-tm-soft-limit), or no\n\
   filtering (the default). Hard filtering uses the parameters in <config> to\n\
   mimic canoe's filtering. Soft filtering eliminates phrase pairs that will\n\
   never receive high scores, no matter what (non-negative) weights are\n\
   assigned to forward phrase probabilities. e weights used for hard or soft\n\
   filtering are specified by [ttable-prune-type] in <config>: either\n\
   'forward-weights' or 'backward-weights' can be used, but not 'combined'.\n\
3) If doing either hard or soft filtering, the number of translations to retain\n\
   is determined by the [ttable-limit] parameter in <config>. This may be\n\
   overridden by the local -tm-prune switch, as described below. The\n\
   [ttable-threshold] parameter in <config> is ignored. \n\
If either -tm-hard-limit or -tm-soft-limit is specified, a single filtered phrase\n\
table is created, combining entries from all tables in <config>. Its name is\n\
given by the argument to -*-limit switch. Otherwise, one filtered table is\n\
created for each table in <config>, named n.FILT.gz, where n.gz is the base\n\
name of the original table.\n\
\n\
DM filtering is invoked by the -ldm switch. It filters a lexicalized DM dm.gz\n\
specified in <config> to contain only phrase pairs found in the current\n\
filtered phrase table, and writes a local version dm.FILT.gz.\n\
\n\
LM filtering is invoked by the -lm switch. It filters ngrams not needed for\n\
decoding with current filtered phrase table(s) from any LM(s) lm.gz specified\n\
in <config>, and writes local version(s) lm.FILT.gz.\n\
\n\
Use -H for examples, and further details.\n\
\n\
Options:\n\
\n\
-f c  canoe config file [canoe.ini]\n\
-c    do not output a new canoe.ini [do]\n\
-z    compress outputs [don't, unless filename in <config> is compressed]\n\
-s    don't strip the path from model file names [do]\n\
-r    don't overwrite existing filtered files [do]\n\
-suffix s  use <s> as suffix for filtered models and config files [.FILT]\n\
-no-src-grep  don't filter phrase table(s) for current source text [do]\n\
-tm-hard-limit|-tm-soft-limit out  use hard or soft TM filtering as described above,\n\
      writing filtered phrase table to out.FILT(.gz?).\n\
-ttable-limit Use T instead of the ttable-limit parameter in CONFIG_FILE. This\n\
             value does not get written back to CONFIG_FILE.FILT.\n\
-tm-prune s#n  TM pruning strategy, overrides [ttable-limit] in <config>. Retains n \n\
      top translations for each source phrase if style is 'fix', and n * w if\n\
      style is 'linear', where w is the number of words in the source phrase.\n\
-tm-online   If -*-limit is specified, process one source phrase at a time to\n\
      save memory. Requires that <config> contain only one TM, sorted on source\n\
      phrases (use tmtext_sort.sh and join_phrasetables).\n\
-ldm  filter lexicalized DMs as described above [don't]\n\
-lm   filter language models as described avove [don't]\n\
-no-per-sent|-phaseIIb  LM filtering strategy, either global (less filtering\n\
      than default) or 'phaseIIb' (more filtering) [filter with per-sent voc]\n\
-vocab v  write the target language vocab for <src> to file <v> [don't]\n\
\n\
";

static char alternate_help_message[] = "\n\
Warning:\n\
\n\
   Running filter_models in limit mode twice in sequence may yield, for the\n\
   second run and the same ttable-limit, a subset of the first run.  This is\n\
   due to the fact that the hard and soft limit filtering process load probs,\n\
   convert them to logprobs and back causing rounding errors.  If identical\n\
   output is required, one can filter an \"original set\" by using filter_models\n\
   in limit mode with a ttable-limit of <huge number>.\n\
\n\
NOTE:\n\
   -tm-soft-limit: This is Limit-TM, as described by Badr et al, (2007).\n\
\n\
Examples:\n\
\n\
* Filter all TMs in canoe.ini by matching source phrases from src:\n\
\n\
    filter_models -z -f canoe.ini < src\n\
\n\
* Filter TMs in canoe.ini by matching source phrases from src, then soft\n\
  filtering to choose their translations. Write results to soft.FILT.gz:\n\
\n\
    filter_models -f canoe.ini -tm-soft-limit soft.gz < src\n\
\n\
* Hard-filter a single multiprob TM in canoe.ini, using minimal memory. Write\n\
  results to hard.FILT.gz:\n\
\n\
    filter_models -f canoe.ini -no-src-grep -tm-online -tm-hard-limit hard.gz\n\
\n\
";

       /// Program filter_models' allowed command line arguments
       const char* const switches[] = {
          "z", "s", "r", "lm", "no-per-sent", "no-src-grep",
          "tm-online", "c", "tm-hard-limit:", "tm-soft-limit:", "f:", "suffix:",
          "ttable-limit:", "vocab:", "input:", "tm-prune:", "ldm", "v"
       };

       /// Command line arguments processing for filter_models.
       class ARG : public argProcessor
       {
          public:
             string config;       ///< canoe.ini config file name.
             string suffix;       ///< suffix to be added on filtered files.
             string vocab_file;   ///< vocabulary file name if requested.
             bool   compress;     ///< should we compress the outputs.
             bool   strip;        ///< should we strip the path from the models file name
             bool   readonly;     ///< treat existing filt files as readonly
             bool   filterLMs;    ///< filter language models
             bool   tm_soft_limit;   ///< soft filter limit the phrase table;
             bool   nopersent;    ///< disables per-sentence vocab LM filt
             string limit_file;   ///< multi probs filename for filter30
             int    ttable_limit; ///< ttable limit override if >= 0
             bool   tm_hard_limit;   ///< perform the tm hard limit filter
             bool   no_src_grep;  ///< process all entries disregarding the source sentences.
             bool   tm_online;    ///< indicates to process source tm in a streaming mode
             bool   output_config;///< indicates to output the modified canoe.ini
             string input;        ///< Source sentences to filter on
             string pruning_type_switch;  ///< What kind of pruning was specified by the user.
             bool   filterLDMs;   ///< Should we filter Lexicalized Distortion Models?
             bool   verbose;      ///< Should we display process on screen?

          public:
             /// Default constructor.
             /// @param argc  number of command line arguments.
             /// @param argv  command line argument vector.
             ARG(const int argc, const char* const argv[])
             : argProcessor(ARRAY_SIZE(switches), switches, 0, 3, help_message, "-h", true, alternate_help_message, "-H")
             , config("canoe.ini")
             , suffix(".FILT")
             , vocab_file("")
             , compress(false)
             , strip(false)
             , readonly(false)
             , filterLMs(false)
             , tm_soft_limit(false)
             , nopersent(false)
             , ttable_limit(-1.0)
             , tm_hard_limit(false)
             , no_src_grep(false)
             , tm_online(false)
             , output_config(true)
             , input("-")
             , pruning_type_switch("")
             , filterLDMs(false)
             , verbose(false)
             {
                argProcessor::processArgs(argc, argv);
             }

             /// Inherited from parent but we don't do anything here for now.
             virtual void printSwitchesValue()
             {
             }

             /// Set the local variables with command line argument's value.
             virtual void processArgs()
             {
                mp_arg_reader->testAndSet("tm-soft-limit", tm_soft_limit);
                mp_arg_reader->testAndSet("tm-soft-limit", limit_file);
                mp_arg_reader->testAndSet("tm-hard-limit", tm_hard_limit);
                mp_arg_reader->testAndSet("tm-hard-limit", limit_file);
                mp_arg_reader->testAndSet("ttable-limit", ttable_limit);
                mp_arg_reader->testAndSet("f", config);
                mp_arg_reader->testAndSet("suffix", suffix);
                mp_arg_reader->testAndSet("vocab", vocab_file);
                mp_arg_reader->testAndSet("z", compress);
                mp_arg_reader->testAndSet("no-per-sent", nopersent);
                mp_arg_reader->testAndSet("no-src-grep", no_src_grep);
                mp_arg_reader->testAndSet("tm-online", tm_online);
                mp_arg_reader->testAndSet("input", input);
                mp_arg_reader->testAndSet("tm-prune", pruning_type_switch);
                mp_arg_reader->testAndSet("ldm", filterLDMs);
                mp_arg_reader->testAndSet("v", verbose);
                // if the option is set we don't want to strip.
                strip         = !mp_arg_reader->getSwitch("s");
                output_config = !mp_arg_reader->getSwitch("c");
                mp_arg_reader->testAndSet("r", readonly);
                filterLMs =  mp_arg_reader->getSwitch("lm");

                // WARN user of potentiel problems
                if (nopersent && !filterLMs)
                   error(ETWarn, "The -no-per-sent flag only takes effect if the L flag is active (process LMs).");
                if (!tm_online && no_src_grep && (tm_soft_limit || tm_hard_limit))
                   error(ETWarn, "Loading models in memory - make sure you have enough RAM to hold them all.");
                if (tm_online && !tm_soft_limit && !tm_hard_limit)
                   error(ETWarn, "Superfluous -tm-online since grepping is always online(Not loading in memory)");

                // Check for user misuage of flags
                if (suffix == "")
                   error(ETFatal, "You must provide a non empty suffix");
                if (tm_soft_limit && tm_hard_limit)
                   error(ETFatal, "Cannot do soft_limit and hard_limit at the same time.");
                if (no_src_grep && !tm_soft_limit && !tm_hard_limit)
                   error(ETWarn, "no TM filtering will be performed");
             }

             /// Checks if the user requested the vocab
             /// @return Returns if the user asked for the vocab
             inline bool vocab() const
             {
                return vocab_file.size() > 0;
             }

             /**
              * Helper function that strips the path from the file name, added
              * the suffix and the .gz extension.
              * @param source original file name.
              * @return Returns modified source file name.
              */
             string prepareFilename(const string& source) const
             {
                if ( source == "-" ) return source;
                string filename(source);
                if (strip) filename = extractFilename(filename);
                if (compress && !isZipFile(filename)) filename += ".gz";
                return addExtension(filename, suffix);
             }

             /**
              * Checks if the filename exists and is read-only, if so modifies
              * in and out accordingly.  We don't want to reprocess a model
              * that has been already filtered but we still need to read it to
              * load in memory its vocabulary thus if the filtered model file
              * exists and is read-only we will make it the input file and send
              * the filtered outpout to /dev/null.
              * @param filename  original file name.
              * @param in        returns the modified input file name.
              * @param out       returns the modified output file name.
              * @return whether out was read-only
              */
             bool getFilenames(const string& filename, string& in, string& out) const
             {
                in  = filename;
                out = prepareFilename(filename);
                if (in == out)
                   error(ETFatal, "Your input and output file names are identical: i:%s o:%s", in.c_str(), out.c_str());
                ofstream os;    // blunderbuss approach for portability?
                ifstream is;    // two blunderbussi
                // Checks if the output file is read-only.
                if (isReadOnly(out)) {
                   in  = out;
                   out = "/dev/null";
                   return true;
                }
                return false;
             }

             /**
              * Indicates if any tm filtering was requested
              */
              bool limit() const { return tm_soft_limit || tm_hard_limit; }

              bool limitPhrases() const { return !no_src_grep; }

              /**
               * Returns true if the file exists and if it is marked as readonly.
               * Note: readonly can be user specified or the file can actually be readonly on disk.
               * @param file  filename
               */
              bool isReadOnly(const string& file) const {
                 ofstream os;
                 return check_if_exists(file) && (readonly || !(os.open(file.c_str()), os));
              }

              bool isReadOnlyOnDisk(const string& file) const {
                 ofstream os;
                 // If the file exists but we can't open it in write-mode, it means it is read-only on disk.
                 return check_if_exists(file) && !(os.open(file.c_str()), os);
              }

       }; // ends ARG
   } // ends filter_models namespace
} // ends Portage namespace


#endif // __FILTERED_MODELS_H__
