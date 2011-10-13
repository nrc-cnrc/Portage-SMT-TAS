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
 * To add a new format to either interface:
 * 1) Derive a new class from the interface;
 * 2) Add a constructor call to the interface's create() method;
 * 3) Add the format's name to WORD_ALIGNMENT_WRITER_FORMATS or
 *    WORD_ALIGNMENT_READER_FORMATS;
 * 4) Document the format in WordAlignmentWriter::help().
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
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

   /**
    * Provide documentation on what the different word alignment formats are.
    */
   static string help();

   /// Class with virtual methods should have virtual destructor too
   virtual ~WordAlignmentWriter() {}
};

#define WORD_ALIGNMENT_WRITER_FORMATS "aachen, gale, hwa, matrix, compact, ugly, green, sri, uli"

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
 * position in the other language).  Just as in sets, NULL links are
 * represented as the value toks2.size() for NULL-aligned lang1 tokens, while
 * NULL-aligned lang2 tokens are listed in the toks1.size()th entry in the
 * output.
 */
class GreenWriter : public WordAlignmentWriter {
public:
   virtual ostream& operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets);

   /**
    * Given alignment sets, write in out the alignment for the token
    * subsequences [beg1,end1) / [beg2,end2).  This method combines a subset
    * operation, a translation and writing to a final format.  It is done in a
    * single operation for optimization purposes.
    * @param sep use this separator for the top-level (default is space).
    */
   static void write_partial_alignment(string& out,
                                const vector<string>& toks1, Uint beg1, Uint end1,
                                const vector<string>& toks2, Uint beg2, Uint end2,
                                const vector< vector<Uint> >& sets,
                                char sep = ' ');

   /**
    * Given an alignment in green format, reverse it, i.e., reverse the roles of
    * toks1 and toks2.
    * @param out where to write the results
    * @param green_alignment alignment in green format
    * @param toks1_len the number of tokens in the original lang1 sequence
    * @param toks2_len the number of tokens in the original lang2 sequence
    * @param sep use this separator for the top-level (default is space).
    *            has to be the same as was used to generate green_alignment
    *            in the first place.
    */
   static void reverse_alignment(string& out, const char* green_alignment,
                                 Uint toks1_len, Uint toks2_len, char sep = ' ');

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
    * @return       true if an alignment was read in, false on EOF
    */
   virtual bool operator()(istream &in, 
                           const vector<string>& toks1, const vector<string>& toks2,
                           vector< vector<Uint> >& sets) = 0;
   
   /**
    * Create a new writer using the given format.
    * @param format see WORD_ALIGNMENT_READER_FORMATS for list
    */
   static WordAlignmentReader* create(const string& format);

   /// Class with virtual methods should have virtual destructor too
   virtual ~WordAlignmentReader() {}
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
   
   virtual bool operator()(istream &in, 
                           const vector<string>& toks1, const vector<string>& toks2,
                           vector< vector<Uint> >& sets);
};

/**
 * Green format: this is similar to compact, but distinguishes between
 * unaligned tokens and those that are explicitly linked to null (the end
 * position in the other language).
 */
class GreenReader : public WordAlignmentReader {
   char sep; ///< separator to expect instead of a space.
public:
   int sentence_id;
   /// Constructor.
   GreenReader(char sep = ' ') : sep(sep), sentence_id(0) {}

   virtual bool operator()(istream &in, 
                           const vector<string>& toks1, const vector<string>& toks2,
                           vector< vector<Uint> >& sets);
   /// Same as the other variant of this operator, but takes the input line as
   /// argument instead of reading it from a file or stream, and without the
   /// toks1 and toks2 parameters since the GreenReader doesn't actually use them.
   void operator()(const string& line, vector< vector<Uint> >& sets);
};

/**
 * SRI format: i-j for each link between word i and word j, 0-based. No null links.
 */
class SRIReader : public WordAlignmentReader {
public:
   int sentence_id;
   /// Constructor.
   SRIReader() : sentence_id(0) {}

   virtual bool operator()(istream &in, 
                           const vector<string>& toks1, const vector<string>& toks2,
                           vector< vector<Uint> >& sets);
};

}
#endif
