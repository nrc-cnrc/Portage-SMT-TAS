/**
 * @author Samuel Larkin
 * @file filter_models.h  Program that filters TMs and LMs.
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
filter_models -zsL -f <config_file> -suffix <suffix> -vocab <vocab_file> < src\n\
\n\
   Filter all models in config_file to contain only the lines needed by canoe\n\
   to translate the sentences in src.\n\
\n\
Options:\n\
-z      compress outputs [don't]\n\
-s      indicates not to strip path from file name [do]\n\
-L      filter language models also [don't]\n\
-f      canoe config file [canoe.ini]\n\
-suffix suffix of filtered models [.FILT]\n\
-vocab  write the target language vocab for src to vocab_file [automatic.vocab]\n";

       /// Program filter_models' allowed command line arguments
       const char* const switches[] = {"z", "s", "L", "f:", "suffix:", "vocab:"};

       /// Command line arguments processing for filter_models.
       class ARG : public argProcessor
       {
          public:
             string config;       ///< canoe.ini config file name.
             string suffix;       ///< suffix to be added on filtered files.
             string vocab_file;   ///< vocabulary file name if requested.
             bool   compress;     ///< should we compress the outputs.
             bool   strip;        ///< should we strip the path from the models file name
             bool   doLMs;        ///< filter language models

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
             , doLMs(false)
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
                mp_arg_reader->testAndSet("f", config);
                mp_arg_reader->testAndSet("suffix", suffix);
                mp_arg_reader->testAndSet("vocab", vocab_file);
                mp_arg_reader->testAndSet("z", compress);
                // if the option is set we don't want to strip.
                strip = !mp_arg_reader->getSwitch("s");
                doLMs =  mp_arg_reader->getSwitch("L");
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
                ofstream  os(out.c_str());
                // Checks if the output file is read-only.
                if (!os)
                {
                   in  = out;
                   out = "/dev/null";
                }
             }
       }; // ends ARG
   } // ends filter_models namespace
} // ends Portage namespace


#endif // __FILTERED_MODELS_H__
