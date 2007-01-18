/**
 * @author Aaron Tikuisis
 * @file canoe.cc  Program canoe, Portage's decoder
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 *
 * See config_io.h for info on adding parameters.
 */

#include "phrasedecoder_model.h"
#include "phrasefinder.h"
#include "basicmodel.h"
#include "backwardsmodel.h"
#include "decoder.h"
#include "hypothesisstack.h"
#include "wordgraph.h"
#include "inputparser.h"
#include "canoe_help.h"
#include "config_io.h"
#include "nbesttranslation.h"
#include <translationProb.h>

#include <printCopyright.h>
#include <file_utils.h>
#include <exception_dump.h>
#include <arg_reader.h>
#include <logging.h>
#include <errors.h>
#include <stdio.h>

#include <math.h>
#include <time.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <ext/stdio_filebuf.h>

using namespace Portage;
using namespace std;


/**
 * Once the hypotheses stacks are created in memory, this function outputs its
 * content in the proper format the user required.  Outputs the lattice files,
 * nbest files, ffvals files and/or pal files.
 * @param h      final hypothesis stack, which contains the best complete translations.
 * @param model  the PhraseDecoderModel model used to generate the hypotheses in the stack.
 * @param num    current source index [0,S).
 * @param oovs   Source Out-of-Vocabulary.
 * @param c      Global canoe configuration object.
 */
static void doOutput(HypothesisStack &h, PhraseDecoderModel &model,
                     Uint num, vector<bool>* oovs, const CanoeConfig& c)
{
   bool masse(c.masse); // MUST declare a new masse to be able to disable it only for this function

   // Create print function used for outputting
   PrintFunc *printPtr = NULL;
   if (c.ffvals) {
      BasicModel *bmodel = dynamic_cast<BasicModel *>(&model);
      assert(bmodel != NULL);
      if (c.trace) {
         printPtr = new PrintAll(*bmodel, oovs);
      } else {
         printPtr = new PrintFFVals(*bmodel, oovs);
      }
   } else if (c.trace) {
      printPtr = new PrintTrace(model, oovs);
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
   } // while

   // Reverse if necessary
   if (c.backwards)
   {
      vector<DecoderState *> tmp;
      tmp.insert(tmp.end(), stack.rbegin(), stack.rend());
      stack.swap(tmp);
   } // if

   // Print the results from the stack using the print function
   for (vector<DecoderState *>::reverse_iterator it = stack.rbegin();
        it != stack.rend(); it++)
   {
      s.append(print(*it));
      s.append(" ");
   } // for

   if (s.length() > 0)
   {
      s.erase(s.length() - 1);
   } // if

   // Output the best sentence to the primary output, cout
   cout << s << endl;

   if ( c.verbosity >= 2 )
   {
      // With verbosity >= 2, output the score assigned to the best translation as well.
      cerr << "BEST: " << s << " " << cur->score << endl;
   } // if

   if (c.verbosity >= 3)
   {
      // With verbosity >= 3, output the path taken for the best translation as well.
      DecoderState *travBack = cur;
      while (travBack->back != NULL)
      {
         cerr << "[ " << travBack->id << " => ";
         travBack = travBack->back;
         cerr << travBack->id << " ]" << endl;
      } // while
   } // if

   // Put all the final states into a vector, which will then be used to
   // produce the lattice output
   vector<DecoderState *> finalStates;
   if (c.latticeOut || c.nbestOut || masse)
   {
      while (true) {
         finalStates.push_back(cur);
         if (h.isEmpty()) break;
         cur = h.pop();
      } // while
      if (c.verbosity >= 2) cerr << "FINAL " << finalStates.size() << " states "
         << " top score " << finalStates[0]->futureScore
         << " bottom score " << cur->futureScore
         << endl;
   }

   if (c.latticeOut)
   {
      // Create file names
      int fileNameLen = c.latticeFilePrefix.length() + 1 + max(1+(int)log10((float)num), 4);
      char curLatticeFile[fileNameLen + 1];
      char curCoverageFile[fileNameLen + strlen(".state") + 1];
      sprintf(curLatticeFile, "%s.%.4d", c.latticeFilePrefix.c_str(), num);
      sprintf(curCoverageFile, "%s.%.4d.state", c.latticeFilePrefix.c_str(), num);

      // Open files for output
      ofstream latticeOut(curLatticeFile);
      ofstream covOut(curCoverageFile);
      if (!latticeOut.good())
      {
         error(ETWarn, "Could not open lattice file %s for writing.", curLatticeFile);
      } else if (!covOut.good())
      {
         error(ETWarn, "Could not open state file %s for writing.", curCoverageFile);
      } else
      {
         // Produce lattice output
         const double dMasse = writeWordGraph(&latticeOut, &covOut, print, finalStates, c.backwards, masse);
         if (masse) {
            fprintf(stderr, "Weight for sentence(%4.4d): %32.32g\n", num, dMasse);
            masse = false;
         }
      } // if
   } // if

   if (c.nbestOut) {
      const string nbsuff("best");
      const string ffsuff(".ffvals");
      const string palsuff(".pal");

      int fileNameLen = c.nbestFilePrefix.length() + 1
         + max(1+(int)log10((float)num), 4) + 1
         + max(1+(int)log10((float)c.nbestSize), 3)
         + nbsuff.length();

      char curNbestFile[fileNameLen + 1];
      char curFfvalsFile[fileNameLen + ffsuff.length() + 1];
      char curPALFile[fileNameLen + palsuff.length() + 1];
      sprintf(curNbestFile, "%s.%.4d.%d%s", c.nbestFilePrefix.c_str(), num, c.nbestSize, nbsuff.c_str());
      sprintf(curFfvalsFile, "%s%s", curNbestFile, ffsuff.c_str());
      sprintf(curPALFile, "%s%s", curNbestFile, palsuff.c_str());

      lattice_overlay  theLatticeOverlay(
         finalStates.begin(), finalStates.end(), model );
      if (c.ffvals || c.trace) {
         std::string  cmd = "| nbest2rescore.pl";
         if (c.ffvals) cmd += " -ffout=" + (string)curFfvalsFile;
         if (c.trace) cmd += " -palout=" + (string)curPALFile;
         cmd += " > " + (string)curNbestFile;

         OMagicStream pipe(cmd.c_str());
         print_nbest( theLatticeOverlay, pipe, c.nbestSize, print, c.backwards );
      } else {
         print_nbest( theLatticeOverlay, ( string ) curNbestFile,
            c.nbestSize, print, c.backwards );
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

   delete printPtr;
} // doOutput


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
    // Do this here until such a time as we might use argProcessor for canoe.
    Logging::init();

    CanoeConfig c;
    static vector<string> args = c.getParamList();

    const char* switches[args.size()];
    for (Uint i = 0; i < args.size(); ++i)
       switches[i] = args[i].c_str();
    char help[strlen(HELP) + strlen(argv[0])];
    sprintf(help, HELP, argv[0]);
    ArgReader argReader(ARRAY_SIZE(switches), switches, 0, 0, help, "-h", false);
    argReader.read(argc - 1, argv + 1);

    printCopyright(2004, "canoe");

    if (!argReader.getSwitch("f", &c.configFile) && 
	!argReader.getSwitch("config", &c.configFile))
       error(ETFatal, "No config file given.  Use -h for help.");

    c.read(c.configFile.c_str()); // set parameters from config file
    c.setFromArgReader(argReader); // override from cmd line
    c.check();

    if (c.verbosity >= 2)
       c.write(cerr, 2);


    // Set random number seed
    srand(time(NULL));

    BasicModelGenerator *gen;
    vector<vector<string> > sents;
    vector<vector<MarkedTranslation> > marks;
    DocumentReader reader(cin);
    if (!c.loadFirst)
    {
        cerr << "reading input sentences" << endl;
        while (true)
        {
            sents.push_back(vector<string>());
            marks.push_back(vector<MarkedTranslation>());
            if ( ! reader.readMarkedSent(sents.back(), marks.back()) )
            {
                if ( c.tolerateMarkupErrors )
                {
                    error(ETWarn, "Tolerating ill-formed markup, but part of the last input line has been discarded");
                } else
                {
                    error(ETFatal, "Aborting because of ill-formed markup");
                }
            }

            // EJJ 12JUL2005 debugging output - might be useful again later.
            //cerr << "INPUT SENTENCE:" << sents.back().size() << " tokens, "
            //     << marks.back().size() << " marks:";
            //for (int i = 0; i < sents.back().size(); i++)
            //{
            //    cerr << sents.back().at(i) << "|";
            //}
            //cerr << "\n";

            if (reader.eof() && sents.back().size() == 0)
            {
                sents.pop_back();
                marks.pop_back();
                break;
            } // if
        } // while
    } // if

    time_t start;
    time(&start);
    gen = BasicModelGenerator::create(c, &sents, &marks);
    cerr << "loaded data structures in " << difftime(time(NULL), start)
         << " seconds" << endl;

    // If verbose >= 2, each run gets the full model printout.
    // If verbose == 1, we print the model too, but not if the first sent num
    // is non-zero, so that only the first job in a canoe-parallel.sh batch
    // gets the model.
    if ( c.verbosity >= 2 || c.verbosity >= 1 && c.firstSentNum == 0 )
        cerr << endl << "Features of the log-linear model used, in order:"
             << endl << gen->describeModel() << endl;

    if (c.randomWeights)
      cerr << "NOTE: using random weights (ignoring given weights); init seed="
           << (c.randomSeed + 1) * (Uint)c.firstSentNum << endl;

    if (!c.loadFirst)
    {
        cerr << "translating " << sents.size() << " sentences" << flush;
    } else
    {
        cerr << "reading and translating sentences" << endl;
    } // if
    time(&start);
    Uint i = 0;
    Uint lastCanoe = 1000;
    while (true)
    {
        vector<string> sent;
        vector<MarkedTranslation> curMarks;
        if (c.loadFirst)
        {
            if ( ! reader.readMarkedSent(sent, curMarks) )
            {
                if ( c.tolerateMarkupErrors )
                {
                    error(ETWarn, "Tolerating ill-formed markup, but part of the last input line has been discarded");
                } else
                {
                    error(ETFatal, "Aborting because of ill-formed markup");
                }
            }
            if (reader.eof()) break;
        } else
        {
            if (i == sents.size()) break;
            sent = sents[i];
            curMarks = marks[i];
        } // if

        if (c.randomWeights)
           gen->setRandomWeights((c.randomSeed + 1) * (Uint)(i+c.firstSentNum));
        vector<bool> oovs;
        PhraseDecoderModel *model = gen->createModel(sent, curMarks, false, &oovs);

	if (c.oov != "pass") {	// skip translation; just write back source, with oov handling
	   writeSrc(c.oov == "write-src-deleted", sent, oovs);
	   delete model;
	   i++;
	   continue;
	}
	
        HypothesisStack *h = runDecoder(*model, c.maxStackSize,
                                        log(c.pruneThreshold),
                                        c.covLimit, log(c.covThreshold),
                                        c.distLimit, c.verbosity);

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
                {
                    lastCanoe = i + 1;
                } // if
                break;
        } // switch

        assert(!h->isEmpty());
        doOutput(*h, *model, i+c.firstSentNum, &oovs, c);

        delete h;
        delete model;
        i++;
    } // while
    cerr << "translated " << i << " sentences in " << difftime(time(NULL),
                                                               start) << " seconds" << endl;
    return 0;
}
END_MAIN
