/**
 * @author George Foster
 * @file word_align_io.h  Read and write word alignments in different formats
 *
 * COMMENTS:
 *
 * Defines the interface WordAlignmentWriter for writing alignments in
 * different formats, and the interface WordAlignmentReader for reading
 * them. In all cases the alignment is represented internally in the canonical
 * format defined by AlignWords (align_word.h).
 *
 * To add a new format to either interface: 1) Derive a new class from
 * the interface; 2) Add a constructor call to the interface's create() method;
 * 3) Add the format's name to WORD_ALIGNMENT_WRITER_FORMATS or
 * WORD_ALIGNMENT_READER_FORMATS.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef WORD_ALIGN_IO_H
#define WORD_ALIGN_IO_H

#include <file_utils.h>

namespace Portage {

/*------------------------------------------------------------------------------
  WordAlignmentWriter
------------------------------------------------------------------------------*/

/**
 * Base class for different alignment output styles.
 */
class WordAlignmentWriter {

public:
   /**
    * Write an alignment to a stream.
    * @param out    output stream
    * @param toks1  sentence in language 1
    * @param toks2  sentence in language 2
    * @param sets   alignment to write, in the representation produced by AlignWords::align().
    * @return       modified out.
    */
   virtual ostream& operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets) = 0;


   /**
    * Create a new writer using the given format.
    * @param format see WORD_ALIGNMENT_WRITER_FORMATS for list
    */
   static WordAlignmentWriter* create(const string& format);
};

#define WORD_ALIGNMENT_WRITER_FORMATS "aachen, gale, hwa, matrix, compact, ugly, green, sri, or uli"

/// Full alignment output style.
class UglyWriter : public WordAlignmentWriter {
public:
   virtual ostream& operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets);
};

/// Aachen alignment output style.
class AachenWriter : public WordAlignmentWriter {
public:
   int sentence_id;  ///< Keeps track of sentence number.
   /// Constructor.
   AachenWriter() : sentence_id(0) {}

   virtual ostream& operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets);
};

/// GALE (RWTH) alignment output style.
/// ID 0 # Source Sentence # Raw Hypothesis # Postprocessed Hypothesis @ Alignment # Scores

class GALEWriter : public WordAlignmentWriter {
public:
   // This needs to be set manually for current source sentence - doesn't
   // change with different nbest hyps.
   int sentence_id;

   // Set this manually to a post-processed version of toks2. If not set, toks2
   // will be used for this.
   const vector<string>* postproc_toks2;

   /// Constructor.
   GALEWriter() : sentence_id(0), postproc_toks2(NULL) {}

   virtual ostream& operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets);
};

/// Compact alignment output style.
class CompactWriter : public WordAlignmentWriter {
public:
   int sentence_id;
   /// Constructor.
   CompactWriter() : sentence_id(0) {}

   virtual ostream& operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets);
};

/**
 * Hwa alignment output style in files named "aligned.<sentence_id>". Both
 * unaligned and null-aligned words are represented as missing alignments.
 */
class HwaWriter : public WordAlignmentWriter {
public:
   int sentence_id;
   /// Constructor.
   HwaWriter() : sentence_id(0) {}
   
   virtual ostream& operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets);
};


/// Matrix alignment output style.
class MatrixWriter : public WordAlignmentWriter {
public:
   virtual ostream& operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets);
};

/**
 * Green format: this is similar to compact, but distinguishes between
 * unaligned tokens and those that are explicitly linked to null (the end
 * position in the other language).
 */
class GreenWriter : public WordAlignmentWriter {
public:
   virtual ostream& operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets);
};

/**
 * SRI format: i-j for each link between word i and word j, 0-based. No null links.
 */
class SRIWriter : public WordAlignmentWriter {
public:
   virtual ostream& operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets);
};

/**
 * Format for Uli Germann's Yawat alignment interface
 *    sent-index (group)*
 *       group = words1:words2:tag
 *          words = pos(,pos)*
 *          tag = "unspec" or other characterization of grp
 */
class UliWriter : public WordAlignmentWriter {
   vector< vector<Uint> > msets;   // closed-format alignment
   vector< vector<Uint> > csets;   // ""

public:
   int sentence_id;  ///< Keeps track of sentence number.

   UliWriter() : sentence_id(0) {}

   virtual ostream& operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets);
};

/*------------------------------------------------------------------------------
  WordAlignmentReader
------------------------------------------------------------------------------*/

/**
 * Base class for different alignment input styles.
 */
class WordAlignmentReader {

public:
   /**
    * Read an alignment from a stream.
    * @param in     input stream
    * @param toks1  sentence in language 1
    * @param toks2  sentence in language 2
    * @param sets   alignment read in, in the representation produced by AlignWords::align().
    * @return       modified in.
    */
   virtual istream& operator()(istream &in, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               vector< vector<Uint> >& sets) = 0;
   
   /**
    * Create a new writer using the given format.
    * @param format see WORD_ALIGNMENT_READER_FORMATS for list
    */
   static WordAlignmentReader* create(const string& format);
};

#define WORD_ALIGNMENT_READER_FORMATS "hwa, green, sri"

/**
 * Hwa alignment format in files named "aligned-in.<sentence_id>". Any
 * missing alignments are explicitly NULL-aligned (to the last+1 position in
 * the other language), since this is the semantics of the Hwa alignment
 * interface. 
 */
class HwaReader : public WordAlignmentReader {

public:
   int sentence_id;
   /// Constructor.
   HwaReader() : sentence_id(0) {}
   
   virtual istream& operator()(istream &in, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               vector< vector<Uint> >& sets);
};

/**
 * Green format: this is similar to compact, but distinguishes between
 * unaligned tokens and those that are explicitly linked to null (the end
 * position in the other language).
 */
class GreenReader : public WordAlignmentReader {
public:
   int sentence_id;
   /// Constructor.
   GreenReader() : sentence_id(0) {}

   virtual istream& operator()(istream &in, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               vector< vector<Uint> >& sets);
};

/**
 * SRI format: i-j for each link between word i and word j, 0-based. No null links.
 */
class SRIReader : public WordAlignmentReader {
public:
   int sentence_id;
   /// Constructor.
   SRIReader() : sentence_id(0) {}

   virtual istream& operator()(istream &in, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               vector< vector<Uint> >& sets);
};

}
#endif
