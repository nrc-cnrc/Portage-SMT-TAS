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
filter_models [-czsr] [-f CONFIG_FILE] [-suffix SUFFIX] [-vocab VOCAB_FILE]\n\
              [-hard-limit OUTFILE | -soft-limit OUTFILE]\n\
              [-L [-no-per-sent]] [-ttable-limit T]\n\
              [-tm-online] [-no-src-grep | < SRC]\n\
\n\
   SWISS ARMY KNIFE OF FILTERING ;)\n\
\n\
   Filter all models in CONFIG_FILE to contain only the lines needed by canoe\n\
   to translate the sentences in SRC.\n\
   - filter_grep filters based on the source phrases only.\n\
   - limit filters target phrases based on the ttable-limit.\n\
     - hard limit reduces the phrasetable(s) to exactly what will be\n\
       used by canoe given a specific set of weights\n\
     - soft limit is Limit-TM, as described by Badr et al, (2007): it applies\n\
       the ttable-limit in a way that is compatible with any set of non-\n\
       negative weights.\n\
\n\
   NOTES:\n\
      Soft and hard limit filtering will produce a single multiprob\n\
      phrasetable, while filter_grep filtering will create one new phrasetable\n\
      for each of the phrasetables in the CONFIG_FILE by adding SUFFIX to\n\
      each of them.\n\
\n\
   HINTS:\n\
      Use tmtext_sort.sh and join_phrasetables if you need to build one TM.\n\
\n\
   WARNING:\n\
      Soft or hard limit filtering with [ttable-prune-type] forward-weights\n\
      or combined is not implemented.\n\
\n\
   WARNING:\n\
      Running filter_models in limit mode twice in sequence may yield, for the\n\
      second run and the same ttable-limit, a subset of the first run.\n\
      This is due to the fact that the hard and soft limit filtering process\n\
      load probs, convert them to logprobs and back causing rounding errors.\n\
      If identical output is required, one can filter an \"original set\" by\n\
      using filter_models in limit mode with a ttable-limit of <huge number>.\n\
\n\
Options:\n\
-f   canoe config file [canoe.ini]\n\
-c   do not output a new canoe.ini [do]\n\
-z   compress outputs [don't, unless filename in CONFIG_FILE is compressed]\n\
-s   indicates not to strip the path from file names [do]\n\
-r   don't overwrite existing filtered files [do, unless they're readonly]\n\
-L   filter language models [don't]\n\
-ttable-limit Use T instead of the ttable-limit parameter in CONFIG_FILE. This\n\
             value does not get written back to CONFIG_FILE.FILT.\n\
-no-per-sent do global voc LM filtering only [do per sent LM filtering]\n\
-suffix      SUFFIX of filtered models when no -*-limit option is used [.FILT]\n\
-vocab       write the target language vocab for SRC to VOCAB_FILE [don't]\n\
-soft-limit  Chop the phrase tables' tails; CONFIG_FILE must set ttable-limit;\n\
             write the result as a multi-prob file in OUTFILE.SUFFIX.\n\
             This is Limit-TM, as described by Badr et al, (2007). [don't]\n\
-hard-limit  Apply the TM weights to the phrase table(s) and keep the\n\
             ttable-limit best tgt phrases for each SRC phrase, exactly as\n\
             canoe would with the same weights.  Output is also one multi-prob,\n\
             OUTFILE.SUFFIX. [don't]\n\
-no-src-grep Process all source phrases, ignoring SRC (STDIN won't be read).\n\
             Warning: memory intensive unless used with -tm-online. [don't]\n\
-tm-online   In -*-limit mode, process one source phrase at a time, deleting it\n\
             from memory before processing the next; this online process has\n\
             minimal, constant memory requirements for arbitrarily large TMs.\n\
             Requires that all the TMs be in a single multi prob TM file,\n\
             sorted on source language phrases; the join_phrasetables program\n\
             can be used to generate such a file. [don't]\n\
\n\
  EXAMPLES:\n\
\n\
     Filter_grep all TMs in canoe.ini with source phrases from src:\n\
\n\
     filter_models -z -f canoe.ini < src\n\
\n\
     Soft limit filter all phrasetables in canoe.ini, applying filter_grep\n\
     using src at the same time:\n\
\n\
     filter_models -f canoe.ini -soft-limit SOFT.OUT.gz < src\n\
\n\
     Hard limit filter a complete multiprob TM, without filter_grep, with\n\
     minimal memory requirements:\n\
\n\
     filter_models -f canoe.ini -no-src-grep -tm-online -hard-limit HARD.OUT.gz\n\
";

       /// Program filter_models' allowed command line arguments
       const char* const switches[] = {
          "z", "s", "r", "L", "no-per-sent", "no-src-grep",
          "tm-online", "c", "hard-limit:", "soft-limit:", "f:", "suffix:",
          "ttable-limit:", "vocab:", "input:"
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
             bool   doLMs;        ///< filter language models
             bool   soft_limit;   ///< soft filter limit the phrase table;
             bool   nopersent;    ///< disables per-sentence vocab LM filt
             string limit_file;   ///< multi probs filename for filter30
             int   ttable_limit;  ///< ttable limit override if >= 0
             bool   hard_limit;   ///< perform the hard limit filter
             bool   no_src_grep;  ///< process all entries disregarding the source sentences.
             bool   tm_online;    ///< indicates to process source tm in a streaming mode
             bool   output_config; ///< indicates to output the modified canoe.ini
             string input;         ///< Source sentences to filter on

          public:
             /// Default constructor.
             /// @param argc  number of command line arguments.
             /// @param argv  command line argument vector.
             ARG(const int argc, const char* const argv[])
             : argProcessor(ARRAY_SIZE(switches), switches, 0, 3, help_message, "-h", true)
             , config("canoe.ini")
             , suffix(".FILT")
             , vocab_file("")
             , compress(false)
             , strip(false)
             , readonly(false)
             , doLMs(false)
             , soft_limit(false)
             , nopersent(false)
             , ttable_limit(-1.0)
             , hard_limit(false)
             , no_src_grep(false)
             , tm_online(false)
             , output_config(true)
             , input("-")
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
                mp_arg_reader->testAndSet("soft-limit", soft_limit);
                mp_arg_reader->testAndSet("soft-limit", limit_file);
                mp_arg_reader->testAndSet("hard-limit", hard_limit);
                mp_arg_reader->testAndSet("hard-limit", limit_file);
                mp_arg_reader->testAndSet("ttable-limit", ttable_limit);
                mp_arg_reader->testAndSet("f", config);
                mp_arg_reader->testAndSet("suffix", suffix);
                mp_arg_reader->testAndSet("vocab", vocab_file);
                mp_arg_reader->testAndSet("z", compress);
                mp_arg_reader->testAndSet("no-per-sent", nopersent);
                mp_arg_reader->testAndSet("no-src-grep", no_src_grep);
                mp_arg_reader->testAndSet("tm-online", tm_online);
                mp_arg_reader->testAndSet("input", input);
                // if the option is set we don't want to strip.
                strip         = !mp_arg_reader->getSwitch("s");
                output_config = !mp_arg_reader->getSwitch("c");
                mp_arg_reader->testAndSet("r", readonly);
                doLMs =  mp_arg_reader->getSwitch("L");

                // WARN user of potentiel problems
                if (nopersent && !doLMs)
                   error(ETWarn, "The -no-per-sent flag only takes effect if the L flag is active (process LMs).");
                if (!tm_online && no_src_grep && (soft_limit || hard_limit))
                   error(ETWarn, "You better have gobs and gobs of RAM or fear the god of bus error!");
                if (tm_online && !soft_limit && !hard_limit)
                   error(ETWarn, "Superfluous tm-online since grepping is always online(Not loading in memory)");

                // Check for user misuage of flags
                if (suffix == "")
                   error(ETFatal, "You must provide a non empty suffix");
                if (soft_limit && hard_limit)
                   error(ETFatal, "Cannot do soft_limit and hard_limit at the same time.");
                if (no_src_grep && !soft_limit && !hard_limit)
                   error(ETFatal, "When using grep mode you must provide source sentences.");
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
              */
             void getFilenames(const string& filename, string& in, string& out) const
             {
                in  = filename;
                out = prepareFilename(filename);
                if (in == out)
                   error(ETFatal, "Your input and output file names are identical: i:%s o:%s", in.c_str(), out.c_str());
                ofstream os;    // blunderbuss approach for portability?
                ifstream is;    // two blunderbussi
                // Checks if the output file is read-only.
                if ((readonly && (is.open(out.c_str()), is)) ||
                    ((is.open(out.c_str()), is) && !(os.open(out.c_str()), os)))
                {
                   in  = out;
                   out = "/dev/null";
                }
             }
             /**
              * Indicates if any tm filtering was requested
              */
              bool limit() const { return soft_limit || hard_limit; }

       }; // ends ARG
   } // ends filter_models namespace
} // ends Portage namespace


#endif // __FILTERED_MODELS_H__
