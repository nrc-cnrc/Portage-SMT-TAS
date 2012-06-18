/**
 * @author Aaron Tikuisis
 * @file canoe.cc
 * @brief Program canoe, Portage's decoder.
 *
 * $Id$
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
#include <boost/optional/optional.hpp>
#include <cstring>
#include <unistd.h>  // sleep
#include <stdlib.h> // rand

using namespace Portage;
using namespace std;
using boost::optional;

/**
 * This object keeps track of the File when the user wants to output one file
 * instead of separate files.
 */
class OneFileInfo {
   const bool append;                  ///< Keeps track of the user request to make one file
   oSafeMagicStream* f_nbest;          ///< nbestlist Stream
   oSafeMagicStream* f_ffvals;         ///< ffvals stream
   oSafeMagicStream* f_pal;            ///< pal stream
   oSafeMagicStream* f_lattice;        ///< lattice stream
   oSafeMagicStream* f_lattice_state;  ///< lattice state stream
   private:
      OneFileInfo(const OneFileInfo&);            ///< Non-copyable
      OneFileInfo& operator=(const OneFileInfo&); ///< Non-copyable
   public:
      /**
       * Default constructor.
       * @param  c  canoe config to get the file names.
       */
      OneFileInfo(const CanoeConfig& c)
      : append(c.bAppendOutput)
      , f_nbest(NULL)
      , f_ffvals(NULL)
      , f_pal(NULL)
      , f_lattice(NULL)
      , f_lattice_state(NULL)
      {
         if (append && c.latticeOut) {
            f_lattice       = new oSafeMagicStream(c.latticeFilePrefix);
            f_lattice_state = new oSafeMagicStream(addExtension(c.latticeFilePrefix, ".state"));
         }

         ostringstream ext;
         ext << "." << c.nbestSize << "best";
         const string curNbestFile  = addExtension(c.nbestFilePrefix, ext.str());
         if (append && c.nbestOut)
         {
            if (!c.nbestProcessor.empty()) {
               const string filename =
                  "| "
                  + c.nbestProcessor
                  + (isSuffix(".gz", curNbestFile) ? "| gzip > " : ">")
                  + curNbestFile;
               f_nbest = new oSafeMagicStream(filename);
            }
            else {
               f_nbest = new oSafeMagicStream(curNbestFile);
            }

            if (c.ffvals) f_ffvals = new oSafeMagicStream(addExtension(curNbestFile, ".ffvals"));
            if (c.trace)  f_pal    = new oSafeMagicStream(addExtension(curNbestFile, ".pal"));
         }
      }
      /// Destructor.
      ~OneFileInfo()
      {
         if (f_nbest) delete f_nbest; f_nbest = NULL;
         if (f_ffvals) delete f_ffvals; f_ffvals = NULL;
         if (f_pal) delete f_pal; f_pal = NULL;
         if (f_lattice) delete f_lattice; f_lattice = NULL;
         if (f_lattice_state) delete f_lattice_state; f_lattice_state = NULL;
      }
      /// Did the user request a single file
      /// @return Returns true if the user requested one file
      bool one() { return append; }
      oSafeMagicStream* nbest() const { return f_nbest; }
      oSafeMagicStream* ffvals() const { return f_ffvals; }
      oSafeMagicStream* pal() const { return f_pal; }
      oSafeMagicStream* lattice() const { assert(f_lattice); return f_lattice; }
      oSafeMagicStream* lattice_state() const { assert(f_lattice_state); return f_lattice_state; }
};


/**
 * Once the hypotheses stacks are created in memory, this function outputs its
 * content in the proper format the user required.  Outputs the lattice files,
 * nbest files, ffvals files and/or pal files.
 * @param h      final hypothesis stack, which contains the best complete translations.
 * @param model  the PhraseDecoderModel model used to generate the hypotheses in the stack.
 * @param num    current source index [0,S).
 * @param oovs   Source Out-of-Vocabulary.
 * @param c      Global canoe configuration object.
 * @param one_file_info  Specifies to output just one nbest containing all nbests
 */
static void doOutput(HypothesisStack &h, PhraseDecoderModel &model,
                     Uint num, vector<bool>* oovs, const CanoeConfig& c,
                     OneFileInfo& one_file_info)
{
   bool masse(c.masse); // MUST declare a new masse to be able to disable it only for this function

   // Create print function used for outputting
   PrintFunc *printPtr = NULL;
   if (c.ffvals) {
      BasicModel *bmodel = dynamic_cast<BasicModel *>(&model);
      assert(bmodel != NULL);
      if (c.trace) {
         printPtr = new PrintAll(*bmodel, c.walign, oovs);
      } else {
         printPtr = new PrintFFVals(*bmodel, oovs);
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
   if (c.bLoadBalancing) cout << num << '\t';
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

   if (c.verbosity >= 3)
   {
      // With verbosity >= 3, output the path taken for the best translation as
      // well.
      DecoderState *travBack = cur;
      while (travBack->back != NULL)
      {
         cerr << "[ " << travBack->id << " => ";
         travBack = travBack->back;
         cerr << travBack->id << " ]" << endl;
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

   do {
      vector<string>  openedFile;

      if (c.latticeOut)
      {
            if (one_file_info.one()) {
               const double dMasse = writeWordGraph(
                     one_file_info.lattice(), one_file_info.lattice_state(),
                     print, finalStates, c.backwards, masse);
               if (masse) {
                  fprintf(stderr, "Weight for sentence(%4.4d): %32.32g\n", num, dMasse);
                  masse = false;
               }
            }
            else {
               // Create file names
               char  sent_num[7];
               snprintf(sent_num, 7, ".%.4d", num);
               const string curLatticeFile
                  = addExtension(c.latticeFilePrefix, sent_num);
               const string curCoverageFile
                  = addExtension(c.latticeFilePrefix, string(sent_num) + ".state");

               // Open files for output
               oSafeMagicStream latticeOut(curLatticeFile);
               openedFile.push_back(curLatticeFile);
               oSafeMagicStream covOut(curCoverageFile);
               openedFile.push_back(curCoverageFile);
               // Produce lattice output
               const double dMasse = writeWordGraph(&latticeOut, &covOut, print,
                     finalStates, c.backwards, masse);
               if (masse) {
                  fprintf(stderr, "Weight for sentence(%4.4d): %32.32g\n", num, dMasse);
                  masse = false;
               }
            }
      }

      if (c.nbestOut) {
         lattice_overlay  theLatticeOverlay(
               finalStates.begin(), finalStates.end(), model );

         // Create the object that will output the Nbest list from the lattice.
         NbestPrinter printer(model, oovs);

         if (c.verbosity >= 3)
            printer.attachDebugStream(&cerr);

         if (one_file_info.one()) {
            // Attach the one file output stream
            printer.attachNbestStream(one_file_info.nbest());
            printer.attachFfvalsStream(one_file_info.ffvals());
            printer.attachPalStream(one_file_info.pal());

            print_nbest( theLatticeOverlay, c.nbestSize, printer, c.backwards );
         }
         else {
            // Here we need to create some temp file stream just long enough to
            // output the request data.
            const Uint buffer_size = 31;
            char  sent_num[buffer_size+1];
            snprintf(sent_num, buffer_size, ".%.4d.", num);
            ostringstream ext;
            ext << sent_num << c.nbestSize << "best";
            const string curNbestFile  = addExtension(c.nbestFilePrefix, ext.str());

            oMagicStream  ffvals_stream;
            if (c.ffvals) {
               ffvals_stream.safe_open(addExtension(curNbestFile, ".ffvals"));
               openedFile.push_back(addExtension(curNbestFile, ".ffvals"));
               printer.attachFfvalsStream(&ffvals_stream);
            }

            oMagicStream  pal_stream;
            if (c.trace) {
               pal_stream.safe_open(addExtension(curNbestFile, ".pal"));
               openedFile.push_back(addExtension(curNbestFile, ".pal"));
               printer.attachPalStream(&pal_stream);
            }

            oMagicStream  nbest_stream;
            if (!c.nbestProcessor.empty()) {
               const string filename =
                  "| "
                  + c.nbestProcessor
                  + (isSuffix(".gz", curNbestFile) ? "| gzip > " : ">")
                  + curNbestFile;
               nbest_stream.safe_open(filename);
            }
            else {
               nbest_stream.safe_open(curNbestFile);
            }
            openedFile.push_back(curNbestFile);
            printer.attachNbestStream(&nbest_stream);

            // Must print here since the streams only exist in this scope on purpose
            print_nbest(theLatticeOverlay, c.nbestSize, printer, c.backwards);
         }
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
      // Let's wait between 15 to 30 seconds 
   } while (!good && (++iteration < maxTries) && (sleep(rand() % 5 + 1), true));

   delete printPtr;
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


/**
 * Program canoe's entry point.
 * @return Returns 0 if successful.
 */
int MAIN(argc, argv)
{
   printCopyright(2004, "canoe");

   // Do this here until such a time as we might use argProcessor for canoe.
   Logging::init();

   CanoeConfig c;
   static vector<string> args = c.getParamList();
   args.push_back("options");

   const char* switches[args.size()];
   for (Uint i = 0; i < args.size(); ++i)
      switches[i] = args[i].c_str();
   char help[strlen(HELP) + strlen(argv[0])];
   sprintf(help, HELP, argv[0]);
   ArgReader argReader(ARRAY_SIZE(switches), switches, 0, 0, help, "-h", false);
   argReader.read(argc - 1, argv + 1);


   if ( argReader.getSwitch("options") ) {
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

   // Binds the pid.
   if (c.bind_pid > 0)
      process_bind(c.bind_pid);

   if (c.verbosity >= 2)
      c.write(cerr, 2);

   // If the user request a single file, this object will keep track of the
   // required files
   OneFileInfo  one_file_info(c);

   // Set random number seed
   srand(time(NULL));

   BasicModelGenerator *gen;
   vector<vector<string> > sents;
   vector<vector<MarkedTranslation> > marks;
   vector<Uint> sourceSentenceIds;
   iSafeMagicStream input(c.input);
   InputParser reader(input, c.bLoadBalancing);
   if (c.checkInputOnly) {
      cerr << "Checking input sentences for markup errors." << endl;
      Uint error_count(0);
      do {
         vector<string> sent;
         vector<MarkedTranslation> marks;
         if ( ! reader.readMarkedSent(sent, marks) )
            ++error_count;
      } while (!reader.eof());
      reader.reportWarningCounts();
      if ( error_count )
         cerr << "Found a total of " << error_count << " fatal format errors." << endl;
      else
         cerr << "No fatal format errors found." << endl;
      cerr << "Total line count: " << reader.getLineNum() - 1 << endl;
      exit(error_count ? 1 : 0);
   }
   if (!c.loadFirst)
   {
      cerr << "Reading input sentences." << endl;
      while (true)
      {
         sents.push_back(vector<string>());
         marks.push_back(vector<MarkedTranslation>());
         sourceSentenceIds.push_back(Uint());
         if ( ! reader.readMarkedSent(sents.back(), marks.back(), NULL, &sourceSentenceIds.back()) )
         {
            if ( c.tolerateMarkupErrors )
               error(ETWarn, "Tolerating ill-formed markup.  Source sentence "
                     "%d will be interpreted as having %d valid mark%s and "
                     "this token sequence: %s",
                     sourceSentenceIds.back(), marks.back().size(),
                     (marks.back().size() == 1 ? "" : "s"),
                     join(sents.back()).c_str());
            else
               error(ETFatal, "Aborting because of ill-formed markup");
         }

         // EJJ 12JUL2005 debugging output - might be useful again later.
         //cerr << "INPUT SENTENCE:" << sents.back().size() << " tokens, "
         //     << marks.back().size() << " marks:";
         //for (int i = 0; i < sents.back().size(); i++)
         //{
         //   cerr << sents.back().at(i) << "|";
         //}
         //cerr << "\n";

         if (reader.eof() && sents.back().empty())
         {
            sents.pop_back();
            marks.pop_back();
            sourceSentenceIds.pop_back();
            break;
         }
      }
      reader.reportWarningCounts();
      assert(sents.size() == marks.size());
      assert(sents.size() == sourceSentenceIds.size());
   }

   // get reference (target sentences) if levenshtein or n-gram is used
   vector<vector<string> > tgt_sents;
   // test parameters for levenshtein or n-gram
   const bool usingLev = !c.feature("lev")->empty() || !c.feature("ng")->empty();
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

      if (!c.loadFirst)
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
   gen = BasicModelGenerator::create(c, &sents, &marks);
   cerr << "Loaded data structures in " << difftime(time(NULL), start)
        << " seconds." << endl;

   // If verbose >= 1, each run gets the full model printout.
   if ( c.verbosity >= 1 )
      cerr << endl << "Log-linear model used:"
           << endl << gen->describeModel() << endl;

   if ( c.futScoreUseFtm > 0)
      cerr << endl << "Including forward translation probs in future score calculation." << endl;

   if (c.randomWeights)
      cerr << "NOTE: using random weights (ignoring given weights); init seed="
           << (c.randomSeed + 1) * (Uint)c.firstSentNum << endl;

   vector<string> sent_weights;
   if (c.sentWeights != "")
      readFileLines(c.sentWeights, sent_weights);

   if (!c.loadFirst) {
      cerr << "Translating " << sents.size() << " sentences." << endl;
   } else {
      cerr << "Reading and translating sentences." << endl;
   }
   time(&start);
   Uint i = 0;
   Uint lastCanoe = 1000;
   Timer centisecondTimer;
   AvgVarTotalStat createStats("createModel"), decodeStats("runDecoder"), outputStats("doOutput");
   while (true)
   {
      centisecondTimer.reset();

      newSrcSentInfo nss_info;
      nss_info.internal_src_sent_seq = i;

      vector<bool> oovs;
      nss_info.oovs = &oovs;

      Uint sourceSentenceId(0);
      vector<string> tgt_sent; // for loadfirst, if refs are needed
      if (c.loadFirst) {
         if ( ! reader.readMarkedSent(nss_info.src_sent, nss_info.marks, NULL, &sourceSentenceId) ) {
            if ( c.tolerateMarkupErrors ) {
               error(ETWarn, "Tolerating ill-formed markup.  Source sentence "
                     "%d will be interpreted as having %d valid mark%s and "
                     "this token sequence: %s",
                     sourceSentenceId, nss_info.marks.size(),
                     (nss_info.marks.size() == 1 ? "" : "s"),
                     join(nss_info.src_sent).c_str());
            } else {
               error(ETFatal, "Aborting because of ill-formed markup");
            }
         }
         //TODO: maybe some test here for tgt_sents like phrase alignment when k is used????
         if (reader.eof()) break;

         if (needRef) {
            string line;
            if (!getline(ref, line))
               error(ETFatal, "Unexpected end of reference file before end of source file.");
            split(line, tgt_sent, " ");
            nss_info.tgt_sent = &tgt_sent;
         }
      } else {
         if (i == sents.size()) break;
         // Gather the proper information for the current sentence we want to process.
         nss_info.src_sent = sents[i];
         nss_info.marks    = marks[i];
         if (!tgt_sents.empty()) nss_info.tgt_sent = &tgt_sents[i];
         sourceSentenceId = sourceSentenceIds[i];
      }
      if (!c.bLoadBalancing) sourceSentenceId += c.firstSentNum;

      if (c.verbosity > 1)
         cerr << "INPUT: " << join(nss_info.src_sent) << endl;

      if (c.randomWeights)
         gen->setRandomWeights((c.randomSeed + 1) * sourceSentenceId);

      if (c.sentWeights != "") {
         if (sourceSentenceId < sent_weights.size())
            gen->setWeightsFromString(sent_weights[sourceSentenceId]);
         else
            error(ETWarn, "source sentence index %d out of range for sentWeights file - %s",
                  sourceSentenceId, "using global weights");
      }

      nss_info.external_src_sent_id = sourceSentenceId;
      if (forcedDecoding) gen->lm_numwords = nss_info.tgt_sent->size() + 1;
      BasicModel *model = gen->createModel(nss_info, false);

      const double createTime = centisecondTimer.secsElapsed(1);
      createStats.add(createTime);

      if (c.oov != "pass") {  // skip translation; just write back source, with oov handling
         writeSrc(c.oov == "write-src-deleted", nss_info.src_sent, oovs);
         delete model;
         ++i;
         continue;
      }

      HypothesisStack *h = NULL;

      const Uint sourceLength = nss_info.src_sent.size();
      if (c.maxlen && sourceLength > c.maxlen) {
         //cout << endl;
         error(ETWarn, "Skipping source sentence longer than maxlen (%u>%u).",
               sourceLength, c.maxlen);
         //++i;
         //continue;
         h = new HistogramThresholdHypStack(*model, 1, -1, 1, -1, 0, 0, true);
         h->push(makeEmptyState(sourceLength, usingLev, usingSR));
      } else {
         h = runDecoder(*model, c, usingLev, usingSR);
      }

      const double decodeTime = centisecondTimer.secsElapsed(1) - createTime;
      decodeStats.add(decodeTime);

      switch (i - lastCanoe)
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
               lastCanoe = i + 1;
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

      doOutput(*h, *model, sourceSentenceId, &oovs, c, one_file_info);

      delete h;
      delete model;
      ++i;

      const double outputTime = centisecondTimer.secsElapsed(1) - createTime - decodeTime;
      outputStats.add(outputTime);

      if (c.verbosity > 1 || c.timing)
         cerr << "Timing: create models + decode + output = total: "
              << createTime << " + " << decodeTime << " + " << outputTime << " = "
              << createTime + decodeTime + outputTime << " seconds." << endl;
   } // while
   if ( c.loadFirst ) reader.reportWarningCounts();
   cerr << "Translated " << i << " sentences in "
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

   return 0;
}
END_MAIN

