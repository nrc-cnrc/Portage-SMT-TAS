/**
 * @author George Foster
 * @file docid.h  Read a doc-id file and represent its contents: mapping from
 *                sentence indexes to document names and possibly other info.
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
*/

#ifndef DOCID_H
#define DOCID_H

#include <string>
#include <vector>
#include <portage_defs.h>

namespace Portage {

class DocID {

   vector<string> doclines;     // line-no -> info from file
   vector<Uint> docids;         // line-no -> doc index
   vector<Uint> docsizes;       // doc index -> number of lines in that doc

public:

   /**
    * Construct by reading from a docid file in standard format.
    * @param docid_filename Name of a file containing one entry per source
    * line, in the format: docname [optional-string], where \<docname\> is the
    * name of the document to which that line belongs (terminated with
    * whitespace). 
    */
   DocID(const string& docid_filename);

   /**
    * Return the number of lines in the docid file.
    */
   Uint numSrcLines() {return doclines.size();}

   /**
    * Return the number of different documents encountered in the docid file.
    */
   Uint numDocs() {return docsizes.size();}

   /**
    * Return the index of the document to which a given source line belongs.
    * @param src_line index in 0..numSrcLines()-1
    */
   Uint docID(Uint src_line) {return docids[src_line];}

   /**
    * Return the number of lines in a given document.
    * @param doc_id index in 0..numDocs()-1
    */
   Uint docSize(Uint doc_id) {return docsizes[doc_id];}

   /**
    * Does a new document start at this line?
    * @param src_line index in 0..numSrcLines()-1
    */
   bool isNewDoc(Uint src_line) {
      return src_line == 0 || docids[src_line] != docids[src_line-1];
   }

   /**
    * Return given line from docid file.
    * @param src_line index in 0..numSrcLines()-1
    * @return requested line
    */
   const string& docline(Uint src_line) {return doclines[src_line];}

   /**
    * Parse docid entry for a given source line.
    * @param docline string returned by docline()
    * @param docname prefix, up to 1st whitespace
    * @param other suffix, to end of line
    */
   void parse(const string& docline, string& docname, string& other);

   /**
    * Debugging test - fails with fatal error if problems detected.
    */
   void test();
};

}

#endif  // DOCID_H


