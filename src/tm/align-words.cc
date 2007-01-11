/**
 * @author George Foster, Eric Joanis, Michel Simard
 * @file align-words.cc  Program that aligns words in a set of line-aligned
 * files using IBM models.
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */
#include <iostream>
#include <iomanip>
#include <ext/hash_map>
#include <map>
#include <cctype>
#include <algorithm>
#include <argProcessor.h>
#include <str_utils.h>
#include <exception_dump.h>
#include <file_utils.h>
#include <printCopyright.h>
#include "tm_io.h"
#include "ibm.h"
#include "phrase_table.h"
#include "word_align.h"
#include "phrase_smoother.h"
#include "phrase_smoother_cc.h"
#include "phrase_table_writer.h"

using namespace Portage;
using namespace __gnu_cxx;

static char help_message[] = "\n\
align-words [-hHvni][-o 'format'][-a 'meth args'][-ibm n][-twist][-giza]\n\
                  ibm-model_lang2_given_lang1 ibm-model_lang1_given_lang2 \n\
                  file1_lang1 file1_lang2 ... fileN_lang1 fileN_lang2\n\
\n\
Align words in a set of line-aligned files using IBM models. The models\n\
should be for p(lang2|lang1) and p(lang1|lang2) respectively (see train_ibm);\n\
<model1> should contain entries of the form 'lang1 lang2 prob', and <model2>\n\
the reverse. Either model argument can be \"no-model\", in which case no model\n\
will be loaded; this may be useful with an alignment method that does not require\n\
models or that requires only one.\n\
\n\
The output format is determined by the output selection options (see below).\n\
\n\
Options:\n\
\n\
-h     Display this help message and quit.\n\
-H     List available word-alignment methods and quit.\n\
-v     Write progress reports to cerr. Use -vv to get more.\n\
-n     Suppress special interpretation of <> characters.\n\
-i     Ignore case (actually: lowercase everything).\n\
-o     Specify output format, one of aachen, hwa, matrix, compact or ugly [aachen]\n\
-a     Word-alignment method and optional args. Use -H for list of methods.\n\
       [IBMOchAligner]\n\
-ibm   Use IBM model <n>: 1 or 2 [1]\n\
-twist With IBM1, assume one language has reverse word order.\n\
       No effect with IBM2.\n\
-giza  IBM-style alignments are to be read from files in GIZA++ format,\n\
       rather than computed at run-time; corresponding alignment files \n\
       should be specified after each pair of text files, like this: \n\
       fileN_lang1 fileN_lang2 align_1_to_2 align_2_to_1...\n\
       Notes:\n\
        - this currently only works with IBMOchAligner\n\
        - you normally still need to provide IBM models as arguments,\n\
          unless you use either of these tricks: specify models called\n\
          \"no-model\", or use the \"-ibm 0\", in which case the program\n\
          does not expect the 2 model arguments.\n\
";

// globals

typedef PhraseTableGen<Uint> PhraseTable;

static char* switches[] = {"v", "vv", "n", "i", "z", "a:", "o:", 
                           "ibm:", "twist", "giza"};

static Uint verbose = 0;
static bool ignore_markup = false;
static bool lowercase = false;
static string align_method;
static bool twist = false;
static bool giza_alignment = false;
static string model1, model2;
static Uint ibm_num = 1;
static bool compress_output = false;
static Uint first_file_arg = 2;
static string output_format = "aachen";

static inline char ToLower(char c) { return tolower(c); }

// Alignment printers

/// Base class for different alignment output styles.
class AlignmentPrinter {
public:
  /**
   * Make the class a callable entity that will output alignment to a stream.
   * @param out    output stream
   * @param toks1  sentence in language 1
   * @param toks2  sentence in language 2
   * @param sets   For each token position in toks1, a set of corresponding
   * token positions in toks 2. Tokens that have no direct correspondence (eg
   * "le" in "m. le president / mr. president") should be left out of the
   * alignment, ie by giving them an empty set if they are in toks1, or not
   * including their position in any set if they are in toks2. Tokens for which
   * a translation is missing (eg "she" and "ils" in "she said / ils ont dit")
   * should be explicitly aligned to the end position in the other language, ie
   * by putting toks2.size() in corresponding set if they are in toks1, or by
   * including their position in sets1[toks1.size()] if they are in toks2. This
   * final element in sets1 is optional; sets1 may be of size toks1.size() if
   * no words in toks2 are considered untranslated.
   * @return Returns the modified out.
   */
  virtual ostream& operator()(ostream &out, 
                              const vector<string>& toks1, const vector<string>& toks2,
                              const vector< vector<Uint> >& sets) = 0;
};

/// Full alignment output style.
class UglyPrinter : public AlignmentPrinter {
public:
  virtual ostream& operator()(ostream &out, 
                      const vector<string>& toks1, const vector<string>& toks2,
                      const vector< vector<Uint> >& sets) {
   bool exclusions_exist = false;
   for (Uint i = 0; i < toks1.size(); ++i)
      for (Uint j = 0; j < sets[i].size(); ++j)
	 if (sets[i][j] < toks2.size())
	    out << toks1[i] << "/" << toks2[sets[i][j]] << " ";
	 else
	    exclusions_exist = true;
   out << endl;

   if (exclusions_exist || sets.size() > toks1.size()) {
      out << "EXCLUDED: ";
      for (Uint i = 0; i < toks1.size(); ++i)
	 for (Uint j = 0; j < sets[i].size(); ++j)
	    if (sets[i][j] == toks2.size())
	       out << i << " ";
      out << "/ ";
      if (sets.size() > toks1.size())
	 for (Uint i = 0; i < sets.back().size(); ++i)
	    out << sets.back()[i] << " ";
      out << endl;
   }

   return out;
  }
};


/// Aachen alignment output style.
class AachenPrinter : public AlignmentPrinter {
public:
  int sentence_id;  ///< Keeps track of sentence number.
  /// Constructor.
  AachenPrinter() : sentence_id(0) {}

  virtual ostream& operator()(ostream &out, 
                      const vector<string>& toks1, const vector<string>& toks2,
                      const vector< vector<Uint> >& sets) {
    out << "SENT: " << sentence_id++ << endl;
    
    for (Uint i = 0; i < toks1.size(); ++i)
      for (Uint j = 0; j < sets[i].size(); ++j)
        if (sets[i][j] < toks2.size())
          out << "S " << i << ' ' << sets[i][j] << endl;
    out << endl;

    return out;
  }
};


/// Compact alignment output style.
class CompactPrinter : public AlignmentPrinter {
public:
  int sentence_id;
  /// Constructor.
  CompactPrinter() : sentence_id(0) {}

  virtual ostream& operator()(ostream &out, 
                      const vector<string>& toks1, const vector<string>& toks2,
                      const vector< vector<Uint> >& sets) {
    out << sentence_id++ << '\t';
    for (Uint i = 0; i < toks1.size(); ++i) {
      Uint count = 0;
      for (Uint j = 0; j < sets[i].size(); ++j)
        if (sets[i][j] < toks2.size()) {
          if (count++) out << ',';
          out << sets[i][j]+1;
        }
      out << ';';
    }
    out << endl;

    return out;
  }
};


///  Hwa alignment output style in files named "aligned.<sentence_id>".
class HwaPrinter : public AlignmentPrinter {
public:
  int sentence_id;
  /// Constructor.
  HwaPrinter() : sentence_id(0) {}

  virtual ostream& operator()(ostream &out, 
                      const vector<string>& toks1, const vector<string>& toks2,
                      const vector< vector<Uint> >& sets) {
    ostringstream fname;
    fname << "aligned." << ++sentence_id;
    ofstream fout(fname.str().c_str());

    for (Uint i = 0; i < toks1.size(); ++i)
      for (Uint j = 0; j < sets[i].size(); ++j)
        if (sets[i][j] < toks2.size())
          fout << i << "  " << sets[i][j] 
              << "  (" << toks1[i] << ", " << toks2[sets[i][j]] << ")" 
              << endl;

    fout.close();

    return out;
  }
};


/// Matrix alignment output style.
class MatrixPrinter : public AlignmentPrinter {
public:
  virtual ostream& operator()(ostream &out, 
                      const vector<string>& toks1, const vector<string>& toks2,
                      const vector< vector<Uint> >& sets) {
    // Write out column names
    Uint more = toks2.size(); // any non-zero value would do actually
    for (Uint k = 0; more > 0; ++k) {
      more = 0;
      for (Uint j = 0; j < toks2.size(); ++j) {
        out << '|' << ((k < toks2[j].size()) ? toks2[j][k] : ' ');
        if (k+1 < toks2[j].size()) ++more;
      }
      out << '|' << endl;
    }
    
    //top ruler
    for (Uint j = 0; j < toks2.size(); ++j) 
      out << "+-";
    out << "+" << endl;

    // write out rows
    vector<char> xs(toks2.size());
    for (Uint i = 0; i < toks1.size(); ++i) {
      xs.assign(toks2.size(), ' ');
      for (Uint k = 0; k < sets[i].size(); ++k)
        if (sets[i][k] < toks2.size())
          xs[sets[i][k]] = 'x';
      for (Uint j = 0; j < toks2.size(); ++j)
        out << '|' << xs[j];
      out << "|" << toks1[i] << endl;
    }
    
    // bottom ruler
    for (Uint j = 0; j < toks2.size(); ++j) 
      out << "+-";
    out << "+" << endl << endl;

    return out;
  }
};




// arg processing

/// align words namespace.
/// Prevents global namespace polution in doxygen.
namespace alignWords {
/// Specific argument processing class for align-words program.
class ARG : public argProcessor {
private:
    Logging::logger m_logger;

public:
   /**
    * Default constructor.
    * @param argc  same as the main argc
    * @param argv  same as the main argv
    * @param alt_help alternate help message
    */
   ARG(const int argc, const char* const argv[], const char* alt_help) : 
      argProcessor(ARRAY_SIZE(switches), switches, 1, -1, help_message, "-h", false, alt_help, "-H"),
      m_logger(Logging::getLogger("verbose.main.arg"))
   {
      argProcessor::processArgs(argc, argv);
   }

   /// See argProcessor::processArgs()
   virtual void processArgs() {
      LOG_INFO(m_logger, "Processing arguments");

      string max_phrase_string;

      if (mp_arg_reader->getSwitch("v")) {verbose = 1;}
      if (mp_arg_reader->getSwitch("vv")) verbose = 2;
      
      mp_arg_reader->testAndSet("n", ignore_markup);
      mp_arg_reader->testAndSet("i", lowercase);
      mp_arg_reader->testAndSet("a", align_method);
      mp_arg_reader->testAndSet("z", compress_output);
      mp_arg_reader->testAndSet("o", output_format);
      mp_arg_reader->testAndSet("ibm", ibm_num);
      mp_arg_reader->testAndSet("twist", twist);
      mp_arg_reader->testAndSet("giza", giza_alignment);

      if (ibm_num == 0) {
        if (!giza_alignment)
          error(ETFatal, "Can't use -ibm=0 trick unless -giza is used");
        first_file_arg = 0;
      } else {
        mp_arg_reader->testAndSet(0, "model1", model1);
        mp_arg_reader->testAndSet(1, "model2", model2);
      }
   }
};
} // ends namespace alignWords.


using namespace alignWords;
int MAIN(argc, argv)
{
   printCopyright(2005, "align-words");
   static string alt_help = 
      "--- word aligners ---\n" + WordAlignerFactory::help();
   ARG args(argc, argv, alt_help.c_str());
   string z_ext(compress_output ? ".gz" : "");

   if (align_method.empty())
     align_method = "IBMOchAligner";

   IBM1* ibm_1=0;
   IBM1* ibm_2=0;

   if (ibm_num == 0) {
     if (verbose) cerr << "**Not** loading IBM models" << endl;
   } else {
     if (ibm_num == 1) {
       if (model1 != "no-model") ibm_1 = new IBM1(model1);
       if (model2 != "no-model") ibm_2 = new IBM1(model2);
     } else {
       if (model1 != "no-model") ibm_1 = new IBM2(model1);
       if (model2 != "no-model") ibm_2 = new IBM2(model2);
     }
     if (ibm_1 && ibm_1->trainedWithNulls()) ibm_1->useImplicitNulls = true;
     if (ibm_2 && ibm_2->trainedWithNulls()) ibm_2->useImplicitNulls = true;
     if (verbose) cerr << "models loaded" << endl;
   }

   WordAlignerFactory* aligner_factory = 0;
   WordAligner* aligner = 0;

   if (!giza_alignment) {
     aligner_factory = new WordAlignerFactory(ibm_1, ibm_2, verbose, twist, false);
     aligner = aligner_factory->createAligner(align_method);
   }

   string in_f1, in_f2;
   string alfile1, alfile2;
   Uint fpair = 0;

   GizaAlignmentFile* al_1=0;
   GizaAlignmentFile* al_2=0;

   AlignmentPrinter *print = 0;
   if (output_format == "aachen")
     print = new AachenPrinter();
   else if (output_format == "compact")
     print = new CompactPrinter();
   else if (output_format == "ugly")
     print = new UglyPrinter();
   else if (output_format == "matrix")
     print = new MatrixPrinter();
   else if (output_format == "hwa")
     print = new HwaPrinter();
   else 
     error(ETFatal, "Unknown output format: " + output_format);

   for (Uint arg = first_file_arg; arg+1 < args.numVars(); arg += 2) {

      args.testAndSet(arg, "file1", in_f1);
      args.testAndSet(arg+1, "file2", in_f2);
      if (verbose)
         cerr << "reading " << in_f1 << "/" << in_f2 << endl;
      IMagicStream in1(in_f1);
      IMagicStream in2(in_f2);

      if (giza_alignment) {
        arg+=2;
        if (arg+1 >= args.numVars())
          error(ETFatal, "Missing arguments: alignment files");
        args.testAndSet(arg, "alfile1", alfile1);
        args.testAndSet(arg+1, "alfile2", alfile2);
        if (verbose) 
          cerr << "reading aligment files " << alfile1 << "/" << alfile2 << endl;
        if (al_1) delete al_1;
        al_1 = new GizaAlignmentFile(alfile1);
        if (al_2) delete al_2;
        al_2 = new GizaAlignmentFile(alfile2);
        if (aligner_factory) delete aligner_factory;
        aligner_factory = new WordAlignerFactory(al_1, al_2, verbose, twist, false);

        if (aligner) delete aligner;
        aligner = aligner_factory->createAligner(align_method);
      }

      Uint line_no = 0;
      string line1, line2;
      vector<string> toks1, toks2;
      vector< vector<Uint> > sets1;

      while (getline(in1, line1)) {
         if (!getline(in2, line2)) {
            error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
                  in_f1.c_str(), in_f2.c_str());
            break;
         }
         ++line_no;

         if (verbose > 1) cerr << "--- " << line_no << " ---" << endl;

         if (lowercase) {
           transform(line1.begin(), line1.end(), line1.begin(), ToLower);
           transform(line2.begin(), line2.end(), line2.begin(), ToLower);
         }

         toks1.clear(); toks2.clear();
         TMIO::getTokens(line1, toks1, ignore_markup ? 1 : 0);
         TMIO::getTokens(line2, toks2, ignore_markup ? 1 : 0);

         if (verbose > 1)
            cerr << line1 << endl << line2 << endl;

         sets1.clear();
         aligner->align(toks1, toks2, sets1);

         (*print)(cout, toks1, toks2, sets1);

         if (verbose > 1) cerr << endl; // end of block
         if (verbose == 1 && line_no % 1000 == 0)
            cerr << "line: " << line_no << endl;
      }

      if (getline(in2, line2))
         error(ETWarn, "skipping rest of file pair %s/%s because line counts differ",
               in_f1.c_str(), in_f2.c_str());

      ++fpair;
   }

   if (verbose) cerr << "done" << endl;
}
END_MAIN
