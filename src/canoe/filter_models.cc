/**
 * @author Samuel Larkin
 * @file filter_models.cc  Program that filters TMs and Lms.
 *
 * $Id$
 *
 * LMs & TMs filtering
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */
          
#include <filter_models.h>
#include <exception_dump.h>
#include <printCopyright.h>
#include <config_io.h>
#include <basicmodel.h>
#include <tm_io.h>


using namespace std;
using namespace Portage;
using namespace Portage::filter_models;



/**
 * Program filter_models's entry point.
 * @return Returns 0 if successful.
 */
int MAIN(argc, argv)
{
   printCopyright(2006, "filter_models");
   ARG arg(argc, argv);

   CanoeConfig c;
   c.read(arg.config.c_str());
   c.check();   // Check that the canoe.ini file is coherent

   vector< vector<string> > sents;
   {
      string line;
      //IMagicStream is(in_file.size() ? in_file : "-");
      IMagicStream is("-");
      while (getline(is, line)) {
         sents.resize(sents.size()+1);
         TMIO::getTokens(line, sents.back(), 2);
      }
   }
                        
   // Create the model generator
   vector<vector<MarkedTranslation> > marks;
   BasicModelGenerator bmg(c, sents, marks);
   
   // Add TMtext to vocab and filter them
   for (Uint i = 0; i < c.backPhraseFiles.size(); ++i)
   {
      string bck, fbck;
      arg.getFilenames(c.backPhraseFiles[i], bck, fbck);
      
      if (!c.forPhraseFiles.empty())
      {
         string fwd, ffwd;
         arg.getFilenames(c.forPhraseFiles[i], fwd, ffwd);

         bmg.filterTranslationModel(bck.c_str(), fbck , fwd.c_str() , ffwd);
      }
      else
      {
         bmg.filterTranslationModel(bck.c_str(), fbck);
      } // if
   } // for

   // Add multi-prob TMs to vocab and filter them
   for (Uint i = 0; i < c.multiProbTMFiles.size(); ++i)
   {
      string bck, fbck;
      arg.getFilenames(c.multiProbTMFiles[i], bck, fbck);
      bmg.filterMultiProbTransModel(bck.c_str(), fbck);
   }
   
   // Add LMs
   if (arg.doLMs)
   {
      for (Uint i = 0; i < c.lmFiles.size(); ++i)
      {
         string lm, flm;
         arg.getFilenames(c.lmFiles[i], lm, flm);

         OMagicStream  os_filtered(flm);
         bmg.addLanguageModel(lm.c_str(), c.lmWeights[i], c.lmOrder, &os_filtered);
      } // for
   }

   // Print out the vocab if necessary
   if (arg.vocab_file.size() > 0)
   {
      cerr << "Dumping Vocab" << endl;
      OMagicStream os_vocab(arg.vocab_file);
      bmg.DumpVocab(os_vocab);
   }
} END_MAIN
