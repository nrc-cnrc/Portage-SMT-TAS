/**
 * @author Samuel Larkin
 * @file Prog.h  This file servers as a guide on how to use the argProcessor class.
 *
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
 *
 * Description:
 *
 */

#ifndef YOUR_APPLICATION_GUARD_H
#define YOUR_APPLICATION_GUARD_H

#include <portage_defs.h>  ///< Most likely you will need the portage definitions
#include <argProcessor.h>  ///< Must be included since we are going to derive argProcessor


namespace Portage
{
   /// Your application namespace prevents polluting to much the Portage namespace.
   namespace YourApplication
   {
      /// Example of how to create a logger for your application.
      Logging::logger SpecificLogger = Logging::createLogger("{output|debug|verbose}.Specific");

      /// Your help message to guide user on how to use your application.
      static char help_message[] = "A message describing the arguments to your program";

      /// The list of valid switches already includes -verbose: and -debug.
      const char* const switches[] = {"s1", "s2", "t1:", "t1:", "v"};

      /**
       * A class that will hold and parse command line arguments.
       * Class that will handle the final parsing of command line switches,
       * checking their integrity ans so on and activating loggers for example.
       * Also, it's suggested that this class servers as a placeholder for all
       * switches' values.
       */
      class ARG : public argProcessor
      {
         private:
            Logging::logger m_logger;     ///< class' internal verbose logger
            Logging::logger m_debug;      ///< class' internal debug logger

         public:
            bool     bVerbose;   ///< suggestion: describes if the verbose is on or off without specifing the level
            bool     bS1;        ///< value of s1
            bool     bS2;        ///< value of s2
            string   sT1;        ///< value of t1
            Uint     nT2;        ///< value of t2

         public:
         /**
          * The constructor and you initialize your members.
          * There is a call to the base class (argProcessor) and you MUST fill in the
          * proper information for your application needs.
          * <BR>argProcessor(number of switches, the switches, min arg, max arg, help msg, help switch, );
          * @param argc number of arguments in vector
          * @param argv vector of arguments
          */
         ARG(const int argc, const char* const argv[])
            : argProcessor(ARRAY_SIZE(switches), switches, 0, 0, help_message, "-h", true)
            , m_logger(Logging::createLogger("verbose.main.arg"))
            , m_debug(Logging::createLogger("debug.main.arg"))
            , bVerbose(false)
            , bS1(false)
            , bS2(false)
            , sT1("Initialisation")
            , nT2(0)
         {
            // MANDATORY CALL TO THE BASE CLASS TO PROCESS THE ARGUMENTS
            argProcessor::processArgs(argc, argv);
         }

         /**
          * Logs the values of every switches.
          * In this function you log the values of the arguments from the command line
          * this function is called from the base class argument parsing and is guarded
          * by the if statement and the debuglogger
          */
         virtual void printSwitchesValue()
         {
            // Here m_debug can be replaced by debugLogger from Logging
            LOG_INFO(m_logger, "Printing arguments");
            if (m_debug->isDebugEnabled())   // short-circuiting for efficiency
            {
               LOG_DEBUG(m_debug, "Verbose: %s", (bVerbose ? "ON" : "OFF"));
               LOG_DEBUG(m_debug, "switch1: %s", (bS1 ? "ON" : "OFF"));
               LOG_DEBUG(m_debug, "switch2: %s", (bS2 ? "ON" : "OFF"));
               LOG_DEBUG(m_debug, "t1: %s", sT1.c_str());
               LOG_DEBUG(m_debug, "t2: %d", nT2);

               // An example of a more elaborated scheme
               std::stringstream oss;
               LOG_DEBUG(m_debug, "Number of reference files: %d", sRefFiles.size());
               for (Uint i(0); i<sRefFiles.size(); ++i)
               {
                  oss << "- " << sRefFiles[i].c_str() << " ";
               }
               LOG_DEBUG(m_debug, oss.str().c_str());
            }
         }

         /**
          * Here is the code to extract to variable the value of the switches
          * from the command line.  This is where all the work is done.
          * Extract all switches' values and set them in class members.
          */
         virtual void processArgs()
         {
            LOG_INFO(m_logger, "Processing arguments");

            // mp_arg_reader is a member of the base class that contains the parse command line arguments
            // mp_arg_reader->testAndSet
            // mp_arg_reader->getSwitch

            // Taking care of general flags
            //
            mp_arg_reader->testAndSet("v", bVerbose);
            mp_arg_reader->testAndSet("s1", bS1);
            mp_arg_reader->testAndSet("s2", bS2);
            mp_arg_reader->testAndSet("t1", sT1);
            mp_arg_reader->testAndSet("t2", nT2);
            // Some logger activation or any kind of argument consistency checking can be done
            if (bS1)
            {
               // Example of activating a logger from the commande line
               //Specific->setLevel(Logging::LogLevel::ACTIVATED);
            }

            // Elaborated switch checking based on the presence of some switch
            //
            if (mp_arg_reader->getSwitch("some switch"))
            {
               // get some switches
               mp_arg_reader->testAndSet("S", S);
               mp_arg_reader->testAndSet("K", K);
               mp_arg_reader->testAndSet("best-trans-file", sBestTransFile);
            }
            else
            {
               // error if not set
               LOG_FATAL(m_logger, "You must provide a best translation file");
            }
         }
      }; // ends ARG
   }; // ends the YourApplication namespace

   /// Any declaration of functions related to your main goes here.
   Uint function1(bool verbose = false);

} // ends the Portage namespace

#endif // YOUR_APPLICATION_GUARD_H
