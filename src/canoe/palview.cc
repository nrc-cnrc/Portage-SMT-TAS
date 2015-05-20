/**
 * @author George Foster
 * @file palview.cc
 * @brief Pretty printer for phrase alignments
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include "file_utils.h"
#include "colours.h"
#include "arg_reader.h"
#include "parse_pal.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
palview [options] src out pal\n\
\n\
\n\
For given source, canoe output, and phrase alignment files, write HTML with\n\
colour-coded phrase correspondences to stdout. (Sentences with empty alignments\n\
are shown in black italics.)\n\
\n\
The phrase alignment file <pal> can be in n-best format, with n alignments for\n\
each source line in <src> (padded with blank lines if there are fewer than n\n\
actual alignments); this is detected automatically. The output file <out> can\n\
have either one line per <src> line (ie, if the n-best alignments pertain to a\n\
fixed target sentence) or one line per <pal> line (if n-best alignments\n\
characterize alternative translations); this is also detected automatically.\n\
\n\
Options:\n\
\n\
  -v     Write progress reports to cerr.\n\
  -pv    Alternate output: write files {src,out}.pv annotated with phrase\n\
         correspondences. Nothing is written to stdout with this option.\n\
  -n n   Write at most <n> alignments per source sentence, no matter how many\n\
         are available in <pal> [0 -> write all alignments].\n\
";

// globals

static bool verbose = false;
static bool pv = false;
static Uint n = 0;
static string srcfile;
static string outfile;
static string palfile;
static void getArgs(int argc, char* argv[]);

void writeColourPhrase(const ColourInfo& ci, int colour, const string& phrase)
{
   cout << "<span style=\"color:rgb(" << ci.colours[colour].rgb_str << ")\">" 
        << phrase << "</span> ";
}

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   PalReader reader(srcfile, outfile, palfile, verbose);

   oSafeMagicStream *srcpal = pv ? new oSafeMagicStream(srcfile + ".pv") : NULL;
   oSafeMagicStream *outpal = pv ? new oSafeMagicStream(outfile + ".pv") : NULL;
   
   ColourInfo ci;
   if (!pv) {
      cout << "<html>" << endl;
      cout << "<head>" << endl;
      cout << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
      cout << "</head>" << endl;
   }

   string src_line, out_line, pal_line, s;
   vector<string> src_toks, tgt_toks;
   vector<PhrasePair> phrase_pairs;

   while (reader.readNext(src_line, out_line, pal_line)) {

      if (n != 0 && reader.pos() > n)
         continue;

      if (!parsePhraseAlign(pal_line, phrase_pairs))
         error(ETFatal, "can't parse line from %s: \n%s\n", 
               palfile.c_str(), pal_line.c_str());
      if (reader.pos() == 1)
         splitZ(src_line, src_toks);
      splitZ(out_line, tgt_toks);

      if (!pv) {
         cout << "<p>";
         sortBySource(phrase_pairs.begin(), phrase_pairs.end());
         vector<PhrasePair>::iterator p;
         for (p = phrase_pairs.begin(); p != phrase_pairs.end(); ++p) {
            p->ne_id = (p - phrase_pairs.begin()) % ci.colours.size();
            writeColourPhrase(ci, p->ne_id, p->src(src_toks, s));
         }
         if (phrase_pairs.empty()) cout << "<i>" << src_line << "</i>";
         cout << "<br/>" << endl;
         if (phrase_pairs.empty()) {
            if (out_line.empty()) cout << "[EMPTY]";
            else cout << "<i>" << out_line << "</i>";
         }
         sortByTarget(phrase_pairs.begin(), phrase_pairs.end());
         for (p = phrase_pairs.begin(); p != phrase_pairs.end(); ++p)
            writeColourPhrase(ci, p->ne_id, p->tgt(tgt_toks, s));
         cout << "</p>" << endl;
      } else {
         sortByTarget(phrase_pairs.begin(), phrase_pairs.end());
         for (Uint i = 0; i < phrase_pairs.size(); ++i) {
            (*outpal) << i+1 << '[' << phrase_pairs[i].tgt(tgt_toks, s) << "] ";
            phrase_pairs[i].ne_id = (int) i;  // abuse this field to store tgt position
         }
         (*outpal) << endl;
         sortBySource(phrase_pairs.begin(), phrase_pairs.end());
         for (Uint i = 0; i < phrase_pairs.size(); ++i)
            (*srcpal) << phrase_pairs[i].ne_id + 1 << '['
                      << phrase_pairs[i].src(src_toks, s) << "] ";
         (*srcpal) << endl;
      }
   }

   if (!pv) {
      cout << "</html>" << endl;
      delete srcpal;
      delete outpal;
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "pv", "n:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 3, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("n", n);
   arg_reader.testAndSet("pv", pv);

   arg_reader.testAndSet(0, "src", srcfile);
   arg_reader.testAndSet(1, "out", outfile);
   arg_reader.testAndSet(2, "pal", palfile);
}
