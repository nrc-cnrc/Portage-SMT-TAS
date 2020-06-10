/**
 * @author Samuel Larkin
 * @file argProcessor.h  Base class used to facilitate parsing of command line arguments and basic loggers.
 *
 *
 * COMMENT: Argument processor is base class that takes care of loggers also and is intended to be used in Portage Application.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef __ARG_PROCESSOR_H__
#define __ARG_PROCESSOR_H__

#include "arg_reader.h"
#include "portage_defs.h"
#include "logging.h"
#include <iostream>

namespace Portage
{
   //                                   +- verboseLogger (verbose.main) --- (verbose.main.arg)
   //             +- verboseRootLogger -+
   //             |                     +- yourVLogger (verbose.yourVLogger)
   //             |
   //             |                     +- debugLogger (debug.main) --- (debug.main.arg)
   // rootLogger -+- debugRootLogger ---+
   //             |                     +- yourDLogger (debug.yourDLogger)
   //             |
   //             |                     +- outputLogger (output.main)
   //             +- outputRootLogger --+
   //                                   +- yourOLogger (output.yourOLogger)

   extern Logging::logger verboseLogger; ///< A global verbose logger for every application
   extern Logging::logger debugLogger;   ///< A global debug logger for every application
   extern Logging::logger outputLogger;  ///< A global output logger for every application

   /**
    * Argument processor for parsing command line arguments.
    * This class is a wrapper around ArgReader and is intended to be used
    * as a base class for a class ARG which contains all arguments for a
    * particular application.  It adds some default switches to help debugging.
    */
   class argProcessor : private NonCopyable
   {
      private:
         /// Internal switches used by argProcessor.
         enum SWITCHES_NAME
         {
            LOGGERS,
            VERBOSE,
            DEBUG,
            WFILE,            ///< watch file name
            WTIME,            ///< watch time (rereads the watch file)
            NUMBER_SWITCHES   ///< total number of internal switches
         };

         /// Description of an internal switch.
         struct SWITCHES_DESC
         {
            const char* const name;
            const char* const desc;
         };

      protected:
         ArgReader*  mp_arg_reader;    ///< argument reader
         Uint        m_nVerboseLevel;  ///< verbose Level

      private:
         Logging::logger         m_logger;              ///< A specific logger for argProcessor
         const long              m_lDefaultWatchTime;   ///< The default watch time
         static SWITCHES_DESC    m_switches[];          ///< Internal switches description


      public:
         /**
          * Default constructor.
          * @param nNumberSwitches number of accepted switches
          * @param switchesList list of accepted switches
          * @param min minimum number of expected switches inputed by the user
          * @param max maximum number of expected switches inputed by the user
          * @param helpMsg help message to be displayed
          * @param helpSwitch switch to display the help message
          * @param print_help_on_error indicates if the help message should be displayed on error
          * @param alt_help_msg an alternative help message
          * @param alt_help_switch switch to display alternative help message
          */
         argProcessor(Uint nNumberSwitches,
            const char* const switchesList[],
            int min,
            int max,
            const char* helpMsg,
            const char* helpSwitch = "-h",
            bool print_help_on_error = false,
            const char* alt_help_msg = "",
            const char* alt_help_switch = "");
         virtual ~argProcessor();

         /**
          * Processes the command line arguments.
          * @param argc  number of arguments in vector
          * @param argv  vector of arguments
          */
         void processArgs(Uint argc, const char* const argv[]);

         /**
          * Get the number of variable arguments.
          * @return the number of variable arguments
          */
         Uint numVars() const { return mp_arg_reader->numVars(); }

         /**
          * Get the variable argument i.
          * @param i index of the variable argument to retrieve
          * @return the variable argument at index i
          */
         const string& getVar(Uint i) const { return mp_arg_reader->getVar(i); }

         /**
          * Get all variable arguments from a certain index.
          * @param start_index starting index
          * @param thosevars a vector of all variable arguments strings
          */
         void getVars(Uint start_index, vector<string>& thosevars) const
            { return mp_arg_reader->getVars(start_index, thosevars); }

         /**
          */
         template<class T>
         void testAndSet(Uint var_index, const char* var_name, T& val)
            { mp_arg_reader->testAndSet(var_index, var_name, val); }

         /// Gets value for switch sw.
         template<class T>
         void testAndSet(const char* sw, T& val)
            { mp_arg_reader->testAndSet(sw, val); }

         /**
          * Test for a dual -x/-nox switch, with the results in a "tribool".
          * Use with BOOL_TYPE=bool if a default is desired, or
          * BOOL_TYPE=optional\<bool\> if you want to be able to know neither
          * switch was found on the command line.
          * @param set_sw   on switch
          * @param reset_sw off switch
          * @param val      will be set to true if set_sw is found, false if
          *                 reset_sw is found, and left untouched if neither is
          *                 found.  In particular, if val was formally
          *                 uninitialized before calling this method, it will
          *                 still be uninitialized after.
          */
         template <class BOOL_TYPE>
         void testAndSetOrReset(const char* set_sw, const char* reset_sw,
                                BOOL_TYPE& val)
            { mp_arg_reader->testAndSetOrReset(set_sw, reset_sw, val); }

         /**
          * Display all available loggers.
          * @param os output stream to display available loggers
          */
         void displayLoggers(ostream& os = cout) const;

         /**
          * Displays help for base arg obj.
          * @param os output stream to display help
          */
         void displayHelp(ostream& os = cout) const;

         /**
          * Verbose level.
          * @return the verbose level
          */
         inline Uint getVerboseLevel() const { return m_nVerboseLevel; }

         /**
          * Sets the verbose level.
          * @param level verbose level
          */
         void setVerboseLevel(Uint level);

         /**
          * If the user specified is own verbose switch, use this function to properly parse and set the log level.
          * @param _switch  user's specific verbose switch
          * @return Returns if the verbose is enalbed
          */
         bool checkVerbose(const char* const _switch = "v");

      protected:
         /**
          * Child arguments parsing.
          * This method must be declared by every subclass since it's the core processing of the command line arguments
          */
         virtual void processArgs() = 0;

         /// child arguments displaying (child's responsability not to display).
         virtual void printSwitchesValue();

      private:
         /// Deactivated constructor.
         argProcessor();

         /**
          * Class helpers function which checks if a particular switch needed by argProcessor is enabled.
          * @param sn the switch's name to check if it's enabled
          */
         inline bool isEnabled(const SWITCHES_NAME sn) const
         {
            return (sn<NUMBER_SWITCHES) && mp_arg_reader->getSwitch(m_switches[sn].name);
         }
   };
} // END Portage

#endif   // __ARG_PROCESSOR_H__
