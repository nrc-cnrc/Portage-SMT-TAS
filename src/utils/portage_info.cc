/**
 * @author Eric Joanis
 * @file portage_info.cc
 * @brief Print global PortageII copyright and version information.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <iostream>
#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"
#include <boost/version.hpp>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
portage_info [-options]\n\
\n\
  Print global PortageII Copyright and version information.\n\
\n\
Options:\n\
\n\
  -version   Print the current PortageII version\n\
  -notice    Display the 3rd party Copyright notices\n\
  -with-icu  Indicates whether PortageII was compiled with ICU or not,\n\
             exiting with 0 status iff it was.\n\
  -build-options  Display build options used when compiling PortageII\n\
\n\
";

// globals

static void getArgs(int argc, char* argv[]);
static bool notice = false;
static bool version = false;
static bool with_icu = false;
static bool build_options = false;
static const char VERSION[] = "PortageII 2.0";

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   if ( version ) {
      cout << VERSION << endl;
   } else if ( with_icu ) {
      cout << VERSION << " was compiled with";
      #ifdef NOICU
         cout << "out ICU" << endl;
         exit(1);
      #else
         cout << " ICU" << endl;
         exit(0);
      #endif
   } else if ( build_options ) {
      printCopyright(2004, VERSION);

      #ifdef NOICU
         cout << "ICU: no" << endl;
      #else
         cout << "ICU: yes" << endl;
      #endif

      #ifdef NO_LOGGING
         cout << "Logging: no" << endl;
      #else
         cout << "Logging: yes" << endl;
      #endif

      #ifdef NO_EDUMP
         cout << "Exception dump: no" << endl;
      #else
         cout << "Exception dump: yes" << endl;
      #endif

      cout << "Boost version: " << BOOST_LIB_VERSION << endl;

      #ifdef __GNUC__
         cout << "Compiler: g++ " << __VERSION__ << endl;
      #endif

      cout << endl;

   } else {
      printCopyright(2004, VERSION);
      if ( notice ) {
         const char* const PORTAGE = getenv("PORTAGE");
         if ( !PORTAGE ) {
            cerr << "The PORTAGE environment variable is not set, please set it before" << endl
                 << "running any PortageII software." << endl
                 << "The 3rd party Copyright notices should be located in $PORTAGE/NOTICE." << endl;
            exit(1);
         }

         iMagicStream in;
         // First try $PORTAGE/NOTICE
         string notice_path = string(PORTAGE) + "/NOTICE";
         //cerr << notice_path << endl;
         in.open(notice_path);
         if (!in) {
            // Second try the parent directory of where this program is
            // installed, which will work if this program has been installed in
            // the bin directory under Portage.
            const string program_path = GetAppPath();
            //cerr << program_path << endl;
            const string program_dir = DirName(program_path);
            notice_path = program_dir + "/../NOTICE";
            //cerr << notice_path << endl;
            in.open(notice_path);
            if (!in) {
               // Third, if we're running portage_info directly in the src/utils
               // directory, or in the case of a binary distribution, you have
               // to do ../..  instead of just ..
               notice_path = program_dir + "/../../NOTICE";
               //cerr << notice_path << endl;
               in.open(notice_path);

               if (!in) {
                  cerr << "Can't open the NOTICE file; it should be located in $PORTAGE/NOTICE." << endl;
                  exit(1);
               }
            }
         }
         assert(in);
         string line;
         while (getline(in, line))
            cout << line << endl;
      }
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"version", "notice", "with-icu", "build-options"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 0, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("version", version);
   arg_reader.testAndSet("notice", notice);
   arg_reader.testAndSet("with-icu", with_icu);
   arg_reader.testAndSet("build-options", build_options);
}

