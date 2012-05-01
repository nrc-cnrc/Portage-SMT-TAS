/**
 * @author George Foster
 * @file docid.h  Read a doc-id file and represent its contents: mapping from
 *                sentence indexes to document names and possibly other info.
 *
 * COMMENTS:
 * This module was originally meant only to handle document names in the 1st
 * column of an id file. It has been extended to handle tags in an arbitrary
 * column, but still thinks of tags as "documents". If the same tag occurs
 * multiple times in non-contiguous blocks, for instance, each block will be
 * treated as belonging to a separate 'document'. Also, the 'other' argument to
 * parse() isn't very useful for tags in an arbitrary field. 
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

   Uint field;                  // space-separated field to choose from id file
   vector<string> doclines;     // line-no -> info from file
   vector<Uint> docids;         // line-no -> doc index
   vector<Uint> docsizes;       // doc index -> number of lines in that doc

public:

   /**
    * Construct by reading from a docid file in standard format.
    * @param docid_filename Name of a file containing one entry per source
    * line, in the form of whitespace-separated fields.
    * @param field Field to pick out from each line (the 1st field is 0).
    */
   DocID(const string& docid_filename, Uint field = 0);

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
    * @param docname field extracted from docline
    * @param other suffix of docline that follows docname
    */
   void parse(const string& docline, string& docname, string& other);

   /**
    * Debugging test - fails with fatal error if problems detected.
    */
   void test();
};

}

#endif  // DOCID_H


