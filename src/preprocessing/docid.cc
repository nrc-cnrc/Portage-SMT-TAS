/**
 * @author George Foster
 * @file docid.cc  Represent info from a doc-id file: mapping from sentence
 * indexes to document names and other info.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
*/

#include <file_utils.h>
#include <str_utils.h>
#include "docid.h"

using namespace Portage;

DocID::DocID(const string& docid_filename)
{
   if (docid_filename.empty())
      error(ETFatal, "You must provide a docid file");

   iSafeMagicStream is(docid_filename);
   string line;
   while (getline(is, line))
      doclines.push_back(line);

   docids.resize(doclines.size());
   Uint docid = 0;
   string prevdoc, currdoc, other;
   if (doclines.size()) {
      parse(doclines[0], prevdoc, other);
      docsizes.push_back(0);
   }
   
   for (Uint i = 0; i < doclines.size(); ++i) {
      parse(doclines[i], currdoc, other);
      if (currdoc != prevdoc) {
         ++docid;
         prevdoc = currdoc;
         docsizes.push_back(0);
      }
      ++docsizes.back();
      docids[i] = docid;
   }
}

void DocID::parse(const string& docline, string& docname, string& other)
{
   vector<string> toks;
   split(docline, toks, " \t\n", 2);

   docname = toks.size() > 0 ? toks[0] : "";
   other = toks.size() > 1 ? toks[1] : "";
}


void DocID::test()
{
   int doc = -1;
   Uint beg = 0;

   for (Uint i = 0; i < doclines.size(); ++i) {
      if (isNewDoc(i)) {
         ++doc;
         if (doc > 0) {
            if (docSize(doc-1) != i - beg)
               error(ETFatal, "size mismatch for doc %d: %d versus %d",
                     doc-1, docSize(doc-1), i-beg);
            beg = i;
         }
      }
   }
   if (numDocs() && docSize(doc) != doclines.size()-beg)
      error(ETFatal, "size mismatch for doc %d: %d versus %d",
            doc, docSize(doc), doclines.size()-beg);

}
