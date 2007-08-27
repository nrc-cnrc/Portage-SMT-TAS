/**
 * @author George Foster
 * @file joint2cond_phrase_tables.cc  Program that converts joint-frequency phrase table.
 *
 *
 * COMMENTS:
 *
 * Convert a joint-frequency phrase table (as output by gen_phrase_tables -i or -j)
 * into 2 conditional probability tables (as required by canoe).
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <unistd.h>
#include <iostream>
#include <arg_reader.h>
#include <printCopyright.h>
#include "phrase_table.h"
#include "phrase_smoother.h"
#include "phrase_smoother_cc.h"
#include "phrase_table_writer.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
joint2cond_phrase_tables [-Hviz][-1 l1][-2 l2][-o name][-s 'meth args']\n\
                         [-ibm n][-ibm_l2_given_l1 m][-ibm_l1_given_l2 m]\n\
                         [-tmtext][-multipr d][jtable]\n\
\n\
Convert joint-frequency phrase table <jtable> (stdin if no <jtable> parameter\n\
given) into two standard conditional-probability phrase tables\n\
<name>.<l1>_given_<l2> and <name>.<l2>_given_<l1>. <jtable> can be one or more\n\
individual joint tables cat'd together. Note that the ibm-model parameters are\n\
required only for certain smoothing schemes.\n\
\n\
Options:\n\
\n\
-H    List available smoothing methods and quit.\n\
-v    Write progress reports to cerr.\n\
-i    Counts are integers [counts are floating point]\n\
-z    Compress the output files[don't]\n\
-1    Name of language 1 (one in left column of <jtable>) [en]\n\
-2    Name of language 2 (one in right column of <jtable>) [fr]\n\
-o    Set base name for output tables [phrases]\n\
-s    Smoothing method for conditional probs. Use -H for list of methods.\n\
      Multiple methods may be specified by using -s repeatedly, but these are only\n\
      useful if -multipr output is selected. [RFSmoother]\n\
-ibm  Use IBM model <n>: 1 or 2 [1]\n\
-ibm_l2_given_l1  Name of IBM model for language 2 given language 1 [none]\n\
-ibm_l1_given_l2  Name of IBM model for language 1 given language 2 [none]\n\
-tmtext     Write TMText format phrase tables (delimited text files)\n\
            <name>.<lang1>_given_<lang2> and <name>.<lang2>_given_<lang1>.\n\
            [default if neither -tmtext nor -multipr is given]\n\
-multipr d  Write text phrase table(s) with multiple probabilities: for each\n\
            phrase pair, one or more 'backward' probabilities followed by one\n\
            or more 'forward' probabilities (more than one when multiple smoothing\n\
            methods are selected). d may be one of 'fwd', 'rev', or 'both', to select:\n\
            output <name>.<lang1>2<lang2>, the reverse, or both directions.\n\
-force  Overwrite any existing files\n\
";

// globals

static bool verbose = false;
static bool int_counts = false;
static string lang1("en");
static string lang2("fr");
static string name("phrases");
static vector<string> smoothing_methods;
static Uint ibm_num = 1;
static string ibm_l2_given_l1;
static string ibm_l1_given_l2;
static bool tmtext_output = false;
static string multipr_output = "";
static bool force = false;
static string in_file;
static bool compress_output = false;
static string extension(".gz");

static void getArgs(int argc, char* argv[]);
void delete_or_error_if_exists(const string& filename);

template<class T>
void doEverything(const char* prog_name);

// main

int main(int argc, char* argv[])
{
   printCopyright(2005, "joint2cond_phrase_tables");
   getArgs(argc, argv);

   if (int_counts)
      doEverything<Uint>(argv[0]);
   else
      doEverything<float>(argv[0]);
}

static string makeFinalFileName(string orignal_filename)
{
   if (compress_output) orignal_filename += extension;
   return orignal_filename;
}
      
template<class T>
void doEverything(const char* prog_name)
{
   // Early error checking

   if ( tmtext_output ) {
      delete_or_error_if_exists(makeFinalFileName(name + "." + lang1 + "_given_" + lang2));
      delete_or_error_if_exists(makeFinalFileName(name + "." + lang2 + "_given_" + lang1));
   }
   if ( multipr_output == "fwd" || multipr_output == "both" )
      delete_or_error_if_exists(makeFinalFileName(name + "." + lang1 + "2" + lang2));
   if ( multipr_output == "rev" || multipr_output == "both" )
      delete_or_error_if_exists(makeFinalFileName(name + "." + lang2 + "2" + lang1));

   IMagicStream in(in_file.size() ? in_file : "-");
   PhraseTableGen<T> pt;
   pt.readJointTable(in);

   if (verbose) {
      cerr << "read joint table: "
           << pt.numLang1Phrases() << " " << lang1 << " phrases, "
           << pt.numLang2Phrases() << " " << lang2 << " phrases" << endl;
   }

   IBM1* ibm_1 = NULL;
   IBM1* ibm_2 = NULL;
   if (ibm_num == 1) {
      if (ibm_l2_given_l1 != "") ibm_1 = new IBM1(ibm_l2_given_l1);
      if (ibm_l1_given_l2 != "") ibm_2 = new IBM1(ibm_l1_given_l2);
   } else {
      if (ibm_l2_given_l1 != "") ibm_1 = new IBM2(ibm_l2_given_l1);
      if (ibm_l1_given_l2 != "") ibm_2 = new IBM2(ibm_l1_given_l2);
   }
   if (ibm_1 && ibm_1->trainedWithNulls()) ibm_1->useImplicitNulls = true;
   if (ibm_2 && ibm_2->trainedWithNulls()) ibm_2->useImplicitNulls = true;

   PhraseSmootherFactory<T> smoother_factory(&pt, ibm_1, ibm_2, verbose);
   vector< PhraseSmoother<T>* > smoothers;
   for (Uint i = 0; i < smoothing_methods.size(); ++i)
      smoothers.push_back(smoother_factory.createSmoother(smoothing_methods[i]));

   if (verbose) cerr << "created smoother(s)" << endl;

   string filename;

   if ( tmtext_output ) {
      filename = makeFinalFileName(name + "." + lang2 + "_given_" + lang1);
      {
         OMagicStream ofs(filename);
         dumpCondDistn<T>(ofs, 1, pt, *smoothers[0], verbose);
      }

      filename = makeFinalFileName(name + "." + lang1 + "_given_" + lang2);
      {
         OMagicStream ofs(filename);
         dumpCondDistn<T>(ofs, 2, pt, *smoothers[0], verbose);
      }
   }

   if (multipr_output == "fwd" || multipr_output == "both") {
      string filename = makeFinalFileName(name + "." + lang1 + "2" + lang2);
      if (verbose) cerr << "Writing " << filename << endl;
      OMagicStream ofs(filename);
      dumpMultiProb(ofs, 1, pt, smoothers, verbose);
   }
   if (multipr_output == "rev" || multipr_output == "both") {
      string filename = makeFinalFileName(name + "." + lang2 + "2" + lang1);
      if (verbose) cerr << "Writing " << filename << endl;
      OMagicStream ofs(filename);
      dumpMultiProb(ofs, 2, pt, smoothers, verbose);
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const string alt_help = PhraseSmootherFactory<Uint>::help();
   const char* const switches[] = {"v", "i", "z", "s:", "1:", "2:", "o:", "force", 
		       "ibm:", "ibm_l1_given_l2:", "ibm_l2_given_l1:",
                       "tmtext", "multipr:"};

   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 1, help_message, "-h", true,
                        alt_help.c_str(), "-H");
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("i", int_counts);
   arg_reader.testAndSet("z", compress_output);
   arg_reader.testAndSet("1", lang1);
   arg_reader.testAndSet("2", lang2);
   arg_reader.testAndSet("o", name);
   arg_reader.testAndSet("s", smoothing_methods);
   arg_reader.testAndSet("ibm", ibm_num);
   arg_reader.testAndSet("ibm_l2_given_l1", ibm_l2_given_l1);
   arg_reader.testAndSet("ibm_l1_given_l2", ibm_l1_given_l2);
   arg_reader.testAndSet("tmtext", tmtext_output);
   arg_reader.testAndSet("multipr", multipr_output);
   arg_reader.testAndSet("force", force);

   arg_reader.testAndSet(0, "jtable", in_file);

   if (smoothing_methods.size() == 0)
      smoothing_methods.push_back("RFSmoother");

   if ( !tmtext_output && multipr_output == "" )
      tmtext_output = true;

   if (multipr_output != "" && multipr_output != "fwd" && multipr_output != "rev" && 
       multipr_output != "both")
      error(ETFatal, "Unknown value for -multipr switch: %s", multipr_output.c_str());
   
   if (smoothing_methods.size() > 1 && multipr_output == "") {
      error(ETWarn, "Multiple smoothing methods are only used with -multipr output - ignoring all but %s",
            smoothing_methods[0].c_str());
      smoothing_methods.resize(1);
   }
}

void delete_or_error_if_exists(const string& filename) {
   if ( force )
      delete_if_exists(filename.c_str(),
         "File %s exists - deleting and recreating");
   else
      error_if_exists(filename.c_str(),
         "File %s exists - won't overwrite without -force option");
}
