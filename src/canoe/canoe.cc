/**
 * @author Aaron Tikuisis
 * @file canoe.cc
 * @brief canoe, PortageII's decoder.
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 *
 * See config_io.h for info on adding parameters.
 */

#include "basicmodel.h"
#include "decoder.h"
#include "hypothesisstack.h"
#include "wordgraph.h"
#include "inputparser.h"
#include "canoe_help.h"
#include "config_io.h"
#include "nbesttranslation.h"
#include "translationProb.h"
#include "exception_dump.h"
#include "printCopyright.h"
#include "logging.h"
#include "errors.h"
#include "arg_reader.h"
#include "process_bind.h"
#include "timer.h"
#include "stats.h"
#include "str_utils.h"
#include "simple_overlay.h"
#include "socket_utils.h"
#include "tpt_error.h"
#include <boost/optional/optional.hpp>
#include <cstring>
#include <unistd.h>  // sleep
#include <cstdlib> // getenv(), rand()
#include <signal.h> // sigaction

using namespace Portage;
using namespace std;
using boost::optional;

/**
 * This object keeps track of the File when the user wants to output one file
 * instead of separate files.
 */
class IFileInfo : private NonCopyable {
   protected:
      const CanoeConfig& c;               ///< Canoe's config.
      oSafeMagicStream* f_nbest;          ///< nbestlist Stream
      oSafeMagicStream* f_ffvals;         ///< ffvals stream
      oSafeMagicStream* f_sfvals;         ///< sfvals stream
      oSafeMagicStream* f_pal;            ///< pal stream
      oSafeMagicStream* f_lattice;        ///< lattice stream
      oSafeMagicStream* f_lattice_state;  ///< lattice state stream

   public:
      string s_nbest;          ///< nbestlist filename
      string s_ffvals;         ///< ffvals filename
      string s_sfvals;         ///< sfvals filename
      string s_pal;            ///< pal filename
      string s_lattice;        ///< lattice filename
      string s_lattice_state;  ///< lattice state filename

   public:
      /**
       * Default constructor.
       * @param  c  canoe config to get the file names.
       */
      IFileInfo(const CanoeConfig& c)
      : c(c)
      , f_nbest(NULL)
      , f_ffvals(NULL)
      , f_sfvals(NULL)
      , f_pal(NULL)
      , f_lattice(NULL)
      , f_lattice_state(NULL)
      { }
      /// Destructor.
      virtual ~IFileInfo() {
         freeFiles();
      }

      /// Tell the file info option we're about to handle sentence number id
      virtual void currentSourceSentenceId(Uint id) {}
      /// Tell the file info we're done the current sentence, so it can close
      /// file handles if that's appropriate.
      virtual void doneSentence() {}

      void freeFiles() {
         if (f_nbest) delete f_nbest; f_nbest = NULL;
         if (f_ffvals) delete f_ffvals; f_ffvals = NULL;
         if (f_sfvals) delete f_sfvals; f_sfvals = NULL;
         if (f_pal) delete f_pal; f_pal = NULL;
         if (f_lattice) delete f_lattice; f_lattice = NULL;
         if (f_lattice_state) delete f_lattice_state; f_lattice_state = NULL;
      }

      /// Did the user request a single file
      /// @return Returns true if the user requested one file
      oSafeMagicStream* nbest() {
         if (f_nbest == NULL) {
            if (!c.nbestProcessor.empty()) {
               const string filename =
                  "| "
                  + c.nbestProcessor
                  + (isSuffix(".gz", s_nbest) ? "| gzip > " : ">")
                  + s_nbest;
               f_nbest = new oSafeMagicStream(filename);
            }
            else {
               f_nbest = new oSafeMagicStream(s_nbest);
            }
         }
         return f_nbest;
      }
      oSafeMagicStream* ffvals() {
         if (c.ffvals && f_ffvals == NULL) f_ffvals = new oSafeMagicStream(s_ffvals);
         return f_ffvals;
      }
      oSafeMagicStream* sfvals() {
         if (c.sfvals && f_sfvals == NULL) f_sfvals = new oSafeMagicStream(s_sfvals);
         return f_sfvals;
      }
      oSafeMagicStream* pal() {
         if (c.trace && f_pal == NULL) f_pal = new oSafeMagicStream(s_pal);
         return f_pal;
      }
      oSafeMagicStream* lattice() {
         if (c.latticeOut && f_lattice == NULL) f_lattice = new oSafeMagicStream(s_lattice);
         return f_lattice;
      }
      oSafeMagicStream* lattice_state() {
         if (c.latticeOut && f_lattice_state == NULL) f_lattice_state = new oSafeMagicStream(s_lattice_state);
         return f_lattice_state;
      }
};

class OneFileInfo : public IFileInfo {
   protected:
      const bool append;                  ///< Keeps track of the user request to make one file

   public:
      /**
       * Default constructor.
       * @param  c  canoe config to get the file names.
       */
      OneFileInfo(const CanoeConfig& c)
      : IFileInfo(c)
      , append(c.bAppendOutput)
      {
         assert(append);

         ostringstream ext;
         ext << "." << c.nbestSize << "best";
         s_nbest  = addExtension(c.nbestFilePrefix, ext.str());
         s_ffvals = addExtension(s_nbest, ".ffvals");
         s_sfvals = addExtension(s_nbest, ".sfvals");
         s_pal    = addExtension(s_nbest, ".pal");

         s_lattice       = c.latticeFilePrefix;
         s_lattice_state = addExtension(s_lattice, ".state");
      }
};


/**
 *
 */
class MultipleFileInfo : public IFileInfo {
   public:
      MultipleFileInfo(const CanoeConfig& c)
      : IFileInfo(c)
      {}

      virtual void currentSourceSentenceId(Uint id) {
         freeFiles();

         // Create Nbest file names.
         s_nbest  = generateNbestName(id);
         s_ffvals = addExtension(s_nbest, ".ffvals");
         s_sfvals = addExtension(s_nbest, ".sfvals");
         s_pal    = addExtension(s_nbest, ".pal");

         // Create Lattice file names.
         s_lattice       = generateLatticeName(id);
         s_lattice_state = addExtension(s_lattice, ".state");
      }

      virtual void doneSentence() {
         freeFiles();
      }

   protected:
      virtual string generateNbestName(Uint num) const = 0;
      virtual string generateLatticeName(Uint num) const = 0;
};


/**
 *
 */
class FlatFileInfo : public MultipleFileInfo {
   public:
      FlatFileInfo(const CanoeConfig& c)
      : MultipleFileInfo(c)
      {}

      virtual string generateNbestName(Uint num) const {
         const Uint buffer_size = 31;
         char  sent_num[buffer_size+1];
         snprintf(sent_num, buffer_size, ".%.4d.", num);
         ostringstream ext;
         ext << sent_num << c.nbestSize << "best";
         return addExtension(c.nbestFilePrefix, ext.str());
      }

      virtual string generateLatticeName(Uint num) const {
         char  sent_num[7];
         snprintf(sent_num, 7, ".%.4d", num);
         return  addExtension(c.latticeFilePrefix, sent_num);
      }
};


/**
 *
 */
class HierarchyFileInfo : public MultipleFileInfo {
   public:
      /**
       * Default constructor.
       * @param  c  canoe config to get the file names.
       */
      HierarchyFileInfo(const CanoeConfig& c)
      : MultipleFileInfo(c)
      { }

      virtual string generateNbestName(Uint num) const {
         string dirname, filename;
         DecomposePath(c.nbestFilePrefix, &dirname, &filename);

         const Uint buffer_size = 1024;
         char  name[buffer_size+1];
         snprintf(name, buffer_size, "%s%.3d/%s.%.6d.%dbest%s",
            (dirname.empty() ? "" : (dirname + "/").c_str()),
            num / 1000,
            removeZipExtension(filename).c_str(),
            num,
            c.nbestSize,
            (isZipFile(filename) ? ".gz" : ""));

         if (!mkDirectories(DirName(name).c_str()))
            error(ETFatal, "Failed to create a directory since %s is not a directory!", DirName(name).c_str());

         return name;
      }

      virtual string generateLatticeName(Uint num) const {
         string dirname, filename;
         DecomposePath(c.latticeFilePrefix, &dirname, &filename);

         const Uint buffer_size = 1024;
         char  name[buffer_size+1];
         snprintf(name, buffer_size, "%s%.3d/%s.%.6d%s",
            (dirname.empty() ? "" : (dirname + "/").c_str()),
            num / 1000,
            removeZipExtension(filename).c_str(),
            num,
            (isZipFile(filename) ? ".gz" : ""));

         if (!mkDirectories(DirName(name).c_str()))
            error(ETFatal, "Failed to create a directory since %s is not a directory!", DirName(name).c_str());

         return name;
      }
};


/**
 * Once the hypotheses stacks are created in memory, this function outputs its
 * content in the proper format the user required.  Outputs the lattice files,
 * nbest files, ffvals files, sfvals, and/or pal files.
 * @param h      final hypothesis stack, which contains the best complete translations.
 * @param model  the BasicModel used to generate the hypotheses in the stack.
 * @param num    current source index [0,S).
 * @param oovs   Source Out-of-Vocabulary.
 * @param c      Global canoe configuration object.
 * @param file_info  Specifies to output just one nbest containing all nbests
 * @return the best Hypothesis string.
 */
static string doOutput(HypothesisStack &h,
                     BasicModel &model,
                     Uint num,
                     vector<bool>* oovs,
                     const CanoeConfig& c,
                     IFileInfo& file_info)
{
   bool masse(c.masse); // MUST declare a new masse to be able to disable it only for this function

   // Create print function used for outputting
   PrintFunc *printPtr = NULL;
   if (c.ffvals || c.sfvals) {
      if (c.trace) {
         printPtr = new PrintAll(model, c.sfvals, c.walign, oovs);
      } else {
         printPtr = new PrintFFVals(model, c.sfvals, oovs);
      }
   } else if (c.trace) {
      printPtr = new PrintTrace(model, c.walign, oovs);
   } else {
      printPtr = new PrintPhraseOnly(model, oovs);
   } // if
   PrintFunc &print = *printPtr; // Use reference for readability

   // Traverse the best translation, storing the results in a stack
   string s;
   DecoderState *cur = h.pop();
   DecoderState *travBack = cur;
   vector<DecoderState *> stack;
   while (travBack->back != NULL)
   {
      stack.push_back(travBack);
      travBack = travBack->back;
   }

   // Reverse if necessary
   if (c.backwards)
   {
      vector<DecoderState *> tmp;
      tmp.insert(tmp.end(), stack.rbegin(), stack.rend());
      stack.swap(tmp);
   }

   // Print the results from the stack using the print function
   for (vector<DecoderState *>::reverse_iterator it = stack.rbegin();
        it != stack.rend(); it++)
   {
      s.append(print(*it));
      s.append(" ");
   }

   if (s.length() > 0)
   {
      s.erase(s.length() - 1);
   }

   // Output the best sentence to the primary output, cout
   if (c.bLoadBalancing || !c.canoeDaemon.empty()) cout << num << '\t';
   cout << s << endl;

   if ( c.verbosity >=1 && ShiftReducer::usingSR(c))
   {
      // CAC: Track final ITG stats
      assert (cur->trans->shiftReduce!=NULL);
      if(!cur->trans->shiftReduce->isOneElement())
         ShiftReducer::incompleteStackCnt++;
   }

   if ( c.verbosity >= 2 )
   {
      // With verbosity >= 2, output the score assigned to the best translation
      // as well.
      cerr << "BEST: " << s << " " << cur->score << endl;
   }

   if (c.verbosity >= 2)
   {
      // With verbosity >= 2, output the path taken for the best translation as
      // well.
      DecoderState *travBack = cur;
      while (travBack->back != NULL)
      {
         cerr << "[ " << travBack->id << " => ";
         travBack = travBack->back;
         cerr << travBack->id << " ]" << endl;
      }

      // Also output the full details for each state on the 1-best path
      for (travBack = cur; travBack != NULL; travBack = travBack->back) {
         travBack->display(cerr, &model, oovs->size());
         if (travBack->back)
            model.scoreTranslation(*travBack->trans, 3);
         cerr << "\tbase score            " << travBack->score << endl;
         cerr << "\tfuture score          " << travBack->futureScore << endl;
      }
   }

   // Put all the final states into a vector, which will then be used to
   // produce the lattice output
   vector<DecoderState *> finalStates;
   if (c.latticeOut || c.nbestOut || masse)
   {
      while (true) {
         finalStates.push_back(cur);
         if (h.isEmpty()) break;
         cur = h.pop();
      }
      if (c.verbosity >= 2) cerr << "FINAL " << finalStates.size() << " states "
         << " top score " << finalStates[0]->futureScore
         << " bottom score " << cur->futureScore
         << endl;
   }

   Uint iteration(0);
   const Uint maxTries(3);
   bool good = true;
   file_info.currentSourceSentenceId(num);

   do {
      vector<string>  openedFile;

      if (c.latticeOut) {
         if ( c.latticeOutputOptions == "carmel" ) {
            const double dMasse = writeWordGraph(
                  file_info.lattice(), file_info.lattice_state(),
                  print, finalStates, c.backwards, masse);
            openedFile.push_back(file_info.s_lattice);
            openedFile.push_back(file_info.s_lattice_state);
            if (masse) {
               fprintf(stderr, "Weight for sentence(%4.4d): %32.32g\n", num, dMasse);
               masse = false;
            }
         }
         else if ( c.latticeOutputOptions == "overlay" ) {
            LatticeEdge::setMinScore(c.latticeMinEdge);
            // Length to use for density calculation
            Uint len;
            if(c.latticeSourceDensity) {
               // Density based on source length
               len  = finalStates[0]->trans->numSourceWordsCovered;
            } else {
               // Density based on length of Viterbi translation
               vector<string> toks;
               DecoderState* plainSnt = *finalStates.begin();
               PrintPhraseOnly plain(model, oovs);
               while(plainSnt != NULL) {
                  split(plain(plainSnt), toks, " ");
                  plainSnt = plainSnt->back;
               }
               len = toks.size();
            }

            // Build the lattice
            SimpleOverlay overlay(finalStates.begin(), finalStates.end(), model, c.latticeLogProb);
            overlay.print_pruned_lattice(*file_info.lattice(), print, c.latticeDensity, len, c.verbosity);
            openedFile.push_back(file_info.s_lattice);
         }
         else {
            error(ETFatal, "Illegal -lattice-output-options  Use -h for help.");
         }
      }

      if (c.nbestOut) {
         lattice_overlay  theLatticeOverlay(
               finalStates.begin(), finalStates.end(), model );

         // Create the object that will output the Nbest list from the lattice.
         NbestPrinter printer(model, oovs);

         if (c.verbosity >= 3)
            printer.attachDebugStream(&cerr);

         if (c.ffvals) {
            openedFile.push_back(file_info.s_ffvals);
            printer.attachFfvalsStream(file_info.ffvals());
         }

         if (c.sfvals) {
            openedFile.push_back(file_info.s_sfvals);
            printer.attachSfvalsStream(file_info.sfvals());
         }

         if (c.trace) {
            openedFile.push_back(file_info.s_pal);
            printer.attachPalStream(file_info.pal());
         }

         openedFile.push_back(file_info.s_nbest);
         printer.attachNbestStream(file_info.nbest());

         // Must print here since the streams only exist in this scope on purpose
         print_nbest(theLatticeOverlay, c.nbestSize, printer, c.backwards);
      }

      if (masse)
      {
         const double dMasse = writeWordGraph(NULL, NULL, print, finalStates, c.backwards, true);
         fprintf(stderr, "Weight for sentence(%4.4d): %32.32g\n", num, dMasse);
         masse = false;

         // Temporary commented out until an option is available
         if (false)
         {
            char file4sl[256];
            sprintf(file4sl, "file.%4.4d", num);
            translationProb  tp(model);
            tp.find(file4sl, finalStates, dMasse);
         }
      }

      for (Uint i(0); i<openedFile.size(); ++i) {
         if (!check_if_exists(openedFile[i])) {
            error(ETWarn, "Looks like %s wasn't written on disk at iter %d/%d",
                  openedFile[i].c_str(), iteration+1, maxTries);
            good = false;
         }
      }
      // Let's wait between 1 and 5 seconds
   } while (!good && (++iteration < maxTries) && (sleep(rand() % 5 + 1), true));

   // EJJ - Close our file handles if we're doing one file per sentence
   // (non-append mode). Could this also have fixed the problem the above loop
   // fixes? No idea, and I'm not going to test it or touch it cause it ain't
   // really broke.
   file_info.doneSentence();

   delete printPtr;

   return s;
} // doOutput

/**
 * Reads and tokenizes sentences (not marked, just space separated tokens).
 * This is not used to read src language sentences, only for target langauge
 * references for the Levenshtein and Ngranmatch features.
 * @param[in]  in     from what to read the sentences.
 * @param[out] sents  returned tokenized sentences.
 */
void readRefSentences(istream &in, vector< vector<string> > &sents)
{
   while (true)
   {
      string line;
      getline(in, line);
      if (in.eof() && line == "") break;
      sents.push_back(vector<string>());
      split(line, sents.back(), " ");
   } // while
} // readRefSentences

/**
 * Write the source sentence with OOVs either marked up or stripped.
 * @param del Delete OOVs from output if true, otherwise mark them up like: "<oov>bla</oov>"
 * @param src_sent
 * @param oovs One per src_sent position
 */
static void writeSrc(bool del, const vector<string>& src_sent, const vector<bool>& oovs)
{
   bool first = true;
   for (Uint i = 0; i < src_sent.size(); ++i) {
      if (oovs[i]) {
         if (!del) {
            if (first) {first = false;}
            else cout << ' ';
            cout << "<oov>" << src_sent[i] << "</oov>";
         }
      } else {
         if (first) {first = false;}
         else cout << ' ';
         cout << src_sent[i];
      }
   }
   cout << endl;
}

static bool debugDaemon = false;

/// Singleton socket object to talk to the canoe daemon.
static const SocketUtils *canoeDaemonSocket = NULL;

/**
 * Init the singleton daemon socket object from the canoeDaemonSpec, thus
 * saving the specs for all connections to the canoe daemon.
 * @param canoeDaemonSpec  value of -canoe-daemon command line option
 */
static void setDaemonSpecs(const string& canoeDaemonSpec) {
   assert(canoeDaemonSocket == NULL);
   canoeDaemonSocket = new SocketUtils(canoeDaemonSpec);
}

/**
 * Send a command to the canoe daemon
 * @param message          message to send (e.g., GET, DONE, PING)
 * @param responseExpected set this iff you expect a non-empty answer from the daemon
 * @return the daemon's response
 */
static string sendMessageToDaemon(const string& message, bool responseExpected)
{
   if (debugDaemon) cerr << "Send message to daemon: " << message << endl;
   assert(canoeDaemonSocket != NULL);

   string response;
   bool success = canoeDaemonSocket->sendRecv(message + canoeDaemonSocket->selfDescription(), response);
   if ((!success || response.empty()) && responseExpected)
      error(ETWarn, "No response for message %s to %s",
            message.c_str(), canoeDaemonSocket->remoteDescription().c_str());
   trim(response, "\n");
   if (debugDaemon) cerr << "Received response from daemon: " << response << endl;

   return response;
}

/// Signal handler to report problems to daemon if there's an error
void reportSignalToDaemon(int s) {
   ostringstream oss;
   if (s == SIGINT) {
      oss << "SIGNALED ***(rc=2)*** (caught SIGINT/Ctrl-C)";
   } else if (s == SIGQUIT) {
      oss << "SIGNALED ***(rc=3)*** (caught SIGQUIT/Ctrl-\\)";
   } else if (s == SIGABRT) {
      oss << "SIGNALED ***(rc=6)*** (SIGABRT: assertion failure, ETFatal error, memory problem or other call to abort. Look for this string in worker logs for details)";
   } else if (s == SIGFPE) {
      oss << "SIGNALED ***(rc=8)*** (SIGFPE: numerical error of some kind)";
   } else if (s == SIGUSR1) {
      oss << "SIGNALED ***(rc=10)*** (caught SIGUSR1)";
   } else if (s == SIGUSR2) {
      oss << "SIGNALED ***(rc=12)*** (caught SIGUSR2)";
   } else if (s == SIGTERM) {
      oss << "SIGNALED ***(rc=15)*** (caught SIGTERM/kill/qdel)";
   } else {
      oss << "SIGNALED ***(rc=" << s << ")*** (signal=" << s << ")";
   }
   sendMessageToDaemon(oss.str(), false);
   cerr << "reporting signal to daemon and exiting: " << oss.str() << endl;
   exit(128+s);
}

/// Get the next sentence number to translate, from the canoe daemon
/// Returns next sentence to translate, or Uint(-1) if the daemon said done.
static Uint getNextSentenceIDFromDaemon()
{
   string response = sendMessageToDaemon("GET", false);
   if (response == "***EMPTY***" || response == "")
      return Uint(-1);

   vector<string> tokens;
   if (split(response, tokens, " ", 3) != 2)
      error(ETFatal, "Bad response (should be two tokens) from canoe daemon for GET message: %s",
            response.c_str());

   Uint sentenceID;
   if (!conv(tokens[1], sentenceID))
      error(ETFatal, "Bad response (should be sent ID) from canoe daemon for GET message: %s",
            response.c_str());

   return sentenceID;
}

/// Get the current host name, for logging purposes.
string getHostName() {
   const char* hostname = getenv("HOSTNAME");
   if (hostname) {
      return hostname;
   } else {
      // in some circumstances, HOSTNAME is not defined, e.g, in a crontab, so
      // we fall back to calling the hostname command.
      iMagicStream in("hostname |");
      string line;
      if (getline(in, line)) {
         trim(line, " \t\n");
         return line;
      } else {
         return "MysteryHost";
      }
   }
}

/// For logging purposes
static void logTime(const char* type) {
   time_t now = time(0);
   tm* localtm = localtime(&now);
   static const string hostname = getHostName();

   cerr << "canoe " << type << " on " << hostname << " on " << asctime(localtm) << endl;
}

/**
 * Program canoe's entry point.
 * @return Returns 0 if successful.
 */
int MAIN(argc, argv)
{
   printCopyright(2004, "canoe");

   // Do this here until such a time as we might use argProcessor for canoe.
   Logging::init();

   // Check if we have a canoe daemon to talk to; do this before processing the
   // rest of the command line arguments so we report even those errors to the
   // master. We do it before even initializing the ArgReader so that errors
   // caught at that stage are also reported back to the master.
   struct sigaction sigHandler;
   string canoeDaemon;
   for (int i = 1; i < argc; ++i) {
      if (strcmp(argv[i],"-canoe-daemon") == 0 && i+1 < argc) {
         canoeDaemon = argv[i+1];
         break;
      }
   }
   const bool useCanoeDaemon = !canoeDaemon.empty();
   if (useCanoeDaemon) {
      setDaemonSpecs(canoeDaemon);
      if (sendMessageToDaemon("PING", true) == "PONG") {
         // set signal handlers so we report errors to the daemon too.
         sigHandler.sa_handler = reportSignalToDaemon;
         sigemptyset(&sigHandler.sa_mask);
         sigHandler.sa_flags = 0;

         sigaction(0, &sigHandler, NULL);
         sigaction(SIGINT, &sigHandler, NULL);
         sigaction(SIGQUIT, &sigHandler, NULL);
         sigaction(SIGILL, &sigHandler, NULL);
         sigaction(SIGTRAP, &sigHandler, NULL);
         sigaction(SIGABRT, &sigHandler, NULL);
         sigaction(SIGFPE, &sigHandler, NULL);
         sigaction(SIGUSR1, &sigHandler, NULL);
         sigaction(SIGSEGV, &sigHandler, NULL);
         sigaction(SIGUSR2, &sigHandler, NULL);
         sigaction(SIGTERM, &sigHandler, NULL);

         // Make error(ETFatal) use abort() so we can catch it with SIGABRT.
         Error_ns::Current::errorCallback = Error_ns::abortOnErrorCallBack;
         ugdiss::exit_1_use_abort = true;
      } else {
         error(ETFatal, "Error PINGing canoe daemon; job might already have been completed by other workers, so this error might not be a problem.");
      }
   }

   // Regular command line and config file processing
   CanoeConfig c;
   static vector<string> args = c.getParamList();
   args.push_back("options");

   const char* switches[args.size()];
   for (Uint i = 0; i < args.size(); ++i)
      switches[i] = args[i].c_str();
   vector<string> prog_path_parts;
   split(argv[0], prog_path_parts, "/");
   assert(!prog_path_parts.empty());
   string basename = prog_path_parts.back();
   char help[strlen(HELP) + basename.size() + 1];
   sprintf(help, HELP, basename.c_str());
   ArgReader argReader(ARRAY_SIZE(switches), switches, 0, 0, help, "-h", false);
   argReader.read(argc - 1, argv + 1);

   if (argReader.getSwitch("options")) {
      vector<string> help_lines;
      split(string(HELP), help_lines, "\n");
      for ( Uint i(0); i < help_lines.size(); ++i ) {
         if ( help_lines[i].size() >= 2 &&
              help_lines[i].substr(0,2) == " -" )
            cerr << help_lines[i] << endl;
      }
      exit(0);
   }

   if (!argReader.getSwitch("f", &c.configFile) &&
       !argReader.getSwitch("config", &c.configFile))
      error(ETFatal, "No config file given.  Use -h for help.");

   c.read(c.configFile.c_str()); // set parameters from config file
   c.setFromArgReader(argReader); // override from cmd line
   c.check();
   PhraseTable::log_almost_0 = c.phraseTableLogZero;

   // we log the start time after the checks, so it's not produced when the
   // command line or canoe.ini is rejected.
   if (c.verbosity >= 1) logTime("starting");

   // Binds the pid.
   if (c.bind_pid > 0)
      process_bind(c.bind_pid);

   if (c.verbosity >= 2)
      c.write(cerr, 2);

   // If the user request a single file, this object will keep track of the
   // required files
   IFileInfo* file_info = NULL;
   if (c.bAppendOutput) {
      file_info = new OneFileInfo(c);
   }
   else {
      if (c.hierarchy)
         file_info = new HierarchyFileInfo(c);
      else
         file_info = new FlatFileInfo(c);
   }

   // Set random number seed
   srand(time(NULL));

   BasicModelGenerator *gen;
   VectorPSrcSent sents;
   iSafeMagicStream input(c.input);
   InputParser reader(input, c.bLoadBalancing, c.quietEmptyLines);
   if (c.checkInputOnly) {
      cerr << "Checking input sentences for markup errors." << endl;
      Uint error_count(0);
      newSrcSentInfo nss;
      do {
         if (!reader.readMarkedSent(nss))
            ++error_count;
      } while (!reader.done());
      reader.reportWarningCounts();
      cerr << "Total line count: " << reader.getLineNum() - 1 << endl;
      if (error_count) {
         error(ETFatal, "Found %u fatal format errors.", error_count);
      } else {
         cerr << "No fatal format errors found." << endl;
         exit(0);
      }
   }
   if (!c.loadFirst && !c.describeModelOnly)
   {
      cerr << "Reading input sentences." << endl;
      PSrcSent nss;
      while (nss = reader.getMarkedSent())
         sents.push_back(nss);
   }

   // Read source tags if available. This always reads the whole tags file even
   // if we're currently only processing a portion of the input via
   // canoe-parallel.sh. Sub-optimal space-wise, but more convenient for users
   // and implementers.
   vector<string> srctag_lines;
   if (!c.srctags.empty())
      readFileLines(c.srctags, srctag_lines);

   // get reference (target sentences) if levenshtein or n-gram is used
   vector<vector<string> > tgt_sents;
   // test parameters for levenshtein or n-gram
   const bool usingLev = !c.featureGroup("lev")->empty() || !c.featureGroup("ng")->empty();
   const bool usingSR = ShiftReducer::usingSR(c);
   const bool forcedDecoding = c.forcedDecoding || c.forcedDecodingNZ;
   const bool needRef = usingLev || forcedDecoding;
   iMagicStream ref;
   if ( needRef ){
      if (c.refFile.empty())
         error(ETFatal, "You have to provide a reference file!\n%s",
               usingLev
                ? "Levenshtein and n-gram decoder features cannot be calculated without!"
                : "Forced decoding cannot work without!");
      ref.safe_open(c.refFile);

      if (!c.loadFirst && !c.describeModelOnly)
      {
         // Read reference (target) sentences.
         readRefSentences(ref, tgt_sents);
         if (tgt_sents.size() != sents.size())
            error(ETFatal, "Number of lines (%d) in %s is not the same as the number of lines (%d) in %s",
                  tgt_sents.size(), c.refFile.c_str(), sents.size(), c.input.c_str());
      }
   }

   time_t start;
   time(&start);
   gen = BasicModelGenerator::create(c, &sents);

   if (c.describeModelOnly) {
      cout << gen->describeModel();
      exit(0);
   }

   cerr << "Loaded data structures in " << difftime(time(NULL), start)
        << " seconds." << endl;

   // If verbose >= 1, each run gets the full model printout.
   if ( c.verbosity >= 1 )
      cerr << endl << "Log-linear model used:"
           << endl << gen->describeModel() << endl;

   if (c.randomWeights)
      cerr << "NOTE: using random weights (ignoring given weights); init seed="
           << (c.randomSeed + 1) * (Uint)c.firstSentNum << endl;

   vector<string> sent_weights;
   if (c.sentWeights != "")
      readFileLines(c.sentWeights, sent_weights);


   oSafeMagicStream* nssiStream = NULL;     ///< Stream to write the newSrcSentInfo;
   if (!c.nssiFilename.empty())
      nssiStream = new oSafeMagicStream(c.nssiFilename);

   oSafeMagicStream* triangularArrayAsCPTStream = NULL;     ///< Stream to write the triangular array as a CPT;
   if (!c.triangularArrayFilename.empty())
      triangularArrayAsCPTStream = new oSafeMagicStream(c.triangularArrayFilename);

   if (!c.loadFirst) {
      cerr << "Translating " << sents.size() << " sentences." << endl;
   } else {
      cerr << "Reading and translating sentences." << endl;
   }
   time(&start);
   Uint i = 0;
   Uint num_translated = 0;
   Uint lastCanoe = 1000;
   Timer centisecondTimer;
   AvgVarTotalStat createStats("createModel"), decodeStats("runDecoder"), outputStats("doOutput");
   while (true)
   {
      centisecondTimer.reset();

      Uint save_i = i;
      if (useCanoeDaemon) {
         i = getNextSentenceIDFromDaemon();
         /*
         if (i == 0)
            error(ETFatal, "Testing fatal error catching");
         else if (i == 1)
            assert(false && "Testing assertion failure catching");
         else if (i == 2)
            i = i / num_translated; // forces a division by 0
         */

         if (i == Uint(-1))
            break;
         if (c.verbosity > 1)
            cerr << "Daemon says do sentence " << i << ".\n";
         else
            cerr << i;
      }

      PSrcSent nss;

      vector<string> tgt_sent; // for loadfirst, if refs are needed
      if (c.loadFirst) {
         if (useCanoeDaemon) {
            if (i < save_i)
               error(ETFatal, "Daemon asked for earlier sentence - can't go back in -load-first mode.");
            while (save_i < i) {
               if (!reader.skipMarkedSent())
                  error(ETFatal, "Error skipping input sentence - maybe daemon gave index (%u) past end of input file?", i);
               if (needRef) {
                  string line;
                  if (!getline(ref, line))
                     error(ETFatal, "Unexpected end of reference file before end of source file while skipping to daemon supplied line ID.");
               }
               ++save_i;
            }
         }
         nss = reader.getMarkedSent();
         if (!nss)
            break;

         if (needRef) {
            string line;
            if (!getline(ref, line))
               error(ETFatal, "Unexpected end of reference file before end of source file.");
            split(line, tgt_sent, " ");
            nss->tgt_sent = &tgt_sent;
         }
      } else {
         if (useCanoeDaemon && i >= sents.size())
            error(ETFatal, "Canoe daemon provided sentence ID (%u) beyond end of input file (%u)", i, sents.size());
         if (i == sents.size()) break;
         // Gather the proper information for the current sentence we want to process.
         nss = sents[i];
         if (!tgt_sents.empty()) nss->tgt_sent = &tgt_sents[i];
      }

      Uint sourceSentenceId = nss->external_src_sent_id;
      if (!c.bLoadBalancing) sourceSentenceId += c.firstSentNum;

      vector<bool> oovs;
      nss->oovs = &oovs;
      nss->internal_src_sent_seq = i;

      if (c.verbosity > 1)
         cerr << "INPUT: " << join(nss->src_sent) << endl;

      if (c.randomWeights)
         gen->setRandomWeights((c.randomSeed + 1) * sourceSentenceId);

      if (c.sentWeights != "") {
         if (sourceSentenceId < sent_weights.size())
            gen->setWeightsFromString(sent_weights[sourceSentenceId]);
         else
            error(ETWarn, "source sentence index %d out of range for sentWeights file - %s",
                  sourceSentenceId, "using global weights");
      }

      if (srctag_lines.size()) {
         if (srctag_lines.size() <= sourceSentenceId)
            error(ETFatal, "Unexpected end of source tags file before end of source file.");
         if (splitZ(srctag_lines[sourceSentenceId], nss->src_sent_tags) !=
             nss->src_sent.size())
            error(ETFatal, "Number of tags in line %d doesn't match number of source tokens.",
                  sourceSentenceId);
      }

      nss->external_src_sent_id = sourceSentenceId;
      if (forcedDecoding) gen->lm_numwords = nss->tgt_sent->size() + 1;
      BasicModel *model = gen->createModel(*nss, false);

      const double createTime = centisecondTimer.secsElapsed(1);
      createStats.add(createTime);

      if (c.oov != "pass") {  // skip translation; just write back source, with oov handling
         writeSrc(c.oov == "write-src-deleted", nss->src_sent, oovs);
         delete model;
         ++i;
         continue;
      }

      HypothesisStack *h = NULL;

      const Uint sourceLength = nss->src_sent.size();
      if (c.maxlen && sourceLength > c.maxlen) {
         error(ETWarn, "Skipping source sentence longer than maxlen (%u>%u).",
               sourceLength, c.maxlen);
         h = new HistogramThresholdHypStack(*model, 1, -1, 1, -1, 0, 0, true);
         h->push(makeEmptyState(sourceLength, usingLev, usingSR));
      } else {
         h = runDecoder(*model, c, usingLev, usingSR);
      }

      const double decodeTime = centisecondTimer.secsElapsed(1) - createTime;
      decodeStats.add(decodeTime);

      switch (num_translated - lastCanoe)
      {
         case 0:
            cerr << '\\' << flush;
            break;
         case 1: case 2: case 3:
            cerr << '_' << flush;
            break;
         case 4:
            cerr << '/' << flush;
            break;
         default:
            cerr << '.' << flush;
            if (((double)rand() / (double)RAND_MAX) < 0.01)
               lastCanoe = num_translated + 1;
            break;
      } // switch

      if ( h->isEmpty() ) {
         if ( forcedDecoding )
            // In forced decoding, empty results are common, so be brief
            cerr << "0";
         else
            error(ETWarn, "No translation found for sentence %d with the current settings.",
                  sourceSentenceId);
         // Push an empty state onto a fresh stack, so n-best and lattice and ffvals
         // output all actually happen correctly, even with no translation.
         delete h;
         h = new HistogramThresholdHypStack(*model, 1, -1, 1, -1, 0, 0, true);
         h->push(makeEmptyState(sourceLength, usingLev, usingSR));
      }

      assert(!h->isEmpty());

      const string bestHypothesis = doOutput(*h, *model, sourceSentenceId, &oovs, c, *file_info);
      if (nssiStream != NULL)
         nss->toJSON(*nssiStream, &(gen->get_voc())) << endl;;
      if (triangularArrayAsCPTStream != NULL) {
         *triangularArrayAsCPTStream << bestHypothesis << endl;
         nss->printTriangularArrayAsCPT(*triangularArrayAsCPTStream);
         *triangularArrayAsCPTStream << endl;
      }

      nss.reset();
      delete h;
      delete model;

      ++i;
      ++num_translated;

      const double outputTime = centisecondTimer.secsElapsed(1) - createTime - decodeTime;
      outputStats.add(outputTime);

      if (c.verbosity > 1 || c.timing)
         cerr << "Timing: create models + decode + output = total: "
              << createTime << " + " << decodeTime << " + " << outputTime << " = "
              << createTime + decodeTime + outputTime << " seconds." << endl;

      if (useCanoeDaemon) {
         const string response =
            sendMessageToDaemon("DONE " + toString(i) + " (rc=0)", false);
         if (response.substr(0,10) == "ALLSTARTED")
            break;
      }
   } // while
   cerr << endl << "Translated " << num_translated << " sentences in "
        << difftime(time(NULL), start) << " seconds." << endl;
   if (c.verbosity >= 1 || c.timing) {
      cerr << "TimingStats over per-sentence model creation, decoding, and output times, in seconds:" << endl;
      createStats.write(cerr);
      decodeStats.write(cerr);
      outputStats.write(cerr);
   }

   //CompactPhrase::print_ref_count_stats();
   if (c.verbosity >= 1) gen->displayLMHits(cerr);

   // CAC: To measure effectiveness of ITG constraints and/or features
   if (c.verbosity >= 1 && ShiftReducer::usingSR(c)) {
      fprintf(stderr, "Used %.3g non-ITG reductions, left %d incomplete stacks\n",
              double(ShiftReducer::getNonITGCount()),
              ShiftReducer::incompleteStackCnt
              );
   }

   delete gen;
   delete file_info;
   delete nssiStream;
   delete triangularArrayAsCPTStream;

   if (c.verbosity >= 1) logTime("done");

   return 0;
}
END_MAIN

