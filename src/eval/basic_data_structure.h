/**
 * @author Michel Simard
 * @file basic_data_structure.h  Basic types definition used in the eval module.
 *
 * $Id$
 *
 *
 * This module implements a number of data structures for rescoring
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
*/

#ifndef BASIC_DATA_STRUCTURE_H
#define BASIC_DATA_STRUCTURE_H

#include <vector>
#include <algorithm>
#include "portage_defs.h"
#include "str_utils.h"
#include <voc.h>


namespace Portage {

// Here's the menu:
class Sentence;      // string of chars + vector<string> of toks
class Translation;   // Sentence + Alignment
class Nbest;         // vector<Translation>
class PhraseRange;   // first + last
class AlignedPair;   // source first + source last + target first + target last
class Alignment;     // vector<AlignedPair>

class BLEUstats;
typedef vector<BLEUstats>     RowBLEUstats;
typedef vector<RowBLEUstats>  MatrixBLEUstats;
typedef MatrixBLEUstats::const_iterator mbIT;

typedef string        Token;   ///< One token = one word.
typedef vector<Token> Tokens;  ///< More then one token.


/**
 * Tokenize to words a vector of Sentences.
 * @param v  vector of Sentences to tokenize
 */
template<class T>
void tokenize(vector<T>& v)
{
   typedef typename vector<T>::iterator IT;
   for (IT it(v.begin()); it!=v.end(); ++it)
      it->getTokens();
}

/**
 * Tokenize to Uint a vector of Sentences based on a vocabulary.
 * @param src  vector of Sentences to tokenize
 * @param voc  vocabulary to used to tokenize.
 * @param tgt  a vector of the same length as src containing the tokenized
 *             Sentences.
 */
template<class T>
void tokenize(const vector<T>& src, Voc& voc, vector<vector<Uint> >& tgt)
{
   tgt.clear();
   tgt.resize(src.size());
   Voc::addConverter aConverter(voc);

   for (Uint i(0); i<src.size(); ++i) {
      tgt[i].reserve(src[i].size());
      split(src[i].c_str(), tgt[i], aConverter);
   }
}


/// string of chars + vector<string> of toks.
class Sentence : public string
{
private:
   mutable Tokens m_tokens;
public:
   /// Constructor.
   /// @param s  string representing the sentence.
   Sentence(const string& s = "") : string(s) {}
   /**
    * Split the current sentence in a sequence of tokens.
    * @return Returns a white space split representation of this sentence.
    */
   const Tokens& getTokens() const
   {
      if (size() && m_tokens.empty()) {
         split(string(*this), m_tokens);
      }
      return m_tokens;
   }
   /**
    * Clear the token list.
    */
   void clearTokens() {
      m_tokens.clear();
   }
};
/// A set of sentences.
typedef vector<Sentence>  Sentences;


/// A reference is a sentence.
typedef Sentence Reference;
/// A set of references for a source.
/// Most of the time a source sentence has more then one references.
class References : public vector<Reference> {
public:
   References() : vector<Reference>() {}
   explicit References(size_type __n) : vector<Reference>(__n) {}
   /// Tokenize to words each Reference
   void tokenize() const;
};
/// A set of references for a set of sources.
typedef vector<References> AllReferences;


/// A Translation is just a (target-language) Sentence with an
/// alignment (to a source-language Sentence)
class Translation : public Sentence {
public:
   /// Constructor.
   /// @param t the sentence which is a translation.
   Translation(const string& t = "") : Sentence(t), alignment(0) {}
   Alignment *alignment;  ///< associated alignment for this translation
};

/// A Nbest is a list of Translation's.
class Nbest : public vector<Translation> {
public:
   /// Constructor.
   Nbest() {}
   /// Constructor from a vector of sentences.
   Nbest(const vector<string>& nbest);
   /// Tokenize to words each Translation
   void tokenize() const;
};
/// List of nbest lists for more then one source.
typedef vector<Nbest> AllNbests;


/// A PhraseRange is pair of positions, initial and final (inclusive)
/// determining the location of a phrase in a sentence.
class PhraseRange {
public:
   /// Constructor.
   /// @param x  beginning index of this range
   /// @param y  ending index of this range
   PhraseRange(Uint x, Uint y) : first(x), last(y) {}
   /// Empty range constructor.
   PhraseRange() : first(0), last(0) {}
   /// Desturctor.
   ~PhraseRange() {}
   Uint first;   ///< beginning index
   Uint last;    ///< ending index
};

/// An AlignedPair is a pair of PhraseRanges: one in the source, one in the target.
class AlignedPair {
public:
   /// Empty AlignedPair constructor.
   AlignedPair() :
      source(), target() {}
   /// Constructor.
   /// @param src  source PhraseRange
   /// @param tgt  target PhraseRange
   AlignedPair(const PhraseRange &src, const PhraseRange &tgt) :
      source(src), target(tgt) {}
   /**
    * Constructor which builds the PhraseRanges
    * @param src_x  source beginning index
    * @param src_y  source ending index
    * @param tgt_x  target beginning index
    * @param tgt_y  target ending index
    */
   AlignedPair(Uint src_x, Uint src_y, Uint tgt_x, Uint tgt_y) :
      source(src_x, src_y), target(tgt_x, tgt_y) {}
   /// Destructor.
   ~AlignedPair() {}

   PhraseRange source;  ///< source phrase range.
   PhraseRange target;  ///< target phrase range.

   /**
    * Compare aligned pair based on their source ranges.
    * @param p1  left-hand side operand
    * @param p2  right-hand side operand
    * @return Returns true if p1 is before p2 without overlapping.
    * ...[p1) [p2)...
    */
   static bool sourceLessThan(const AlignedPair &p1, const AlignedPair &p2) {
      return (p1.source.last < p2.source.first);
   }

   /**
    * Compare aligned pair based on their target ranges.
    * @param p1  left-hand side operand
    * @param p2  right-hand side operand
    * @return Returns true if p1 is before p2 without overlapping.
    * ...[p1) [p2)...
    */
   static bool targetLessThan(const AlignedPair &p1, const AlignedPair &p2) {
      return (p1.target.last < p2.target.first);
   }
};

/// An alignment is a list of AlignedPair's.
class Alignment : public vector<AlignedPair> {
public:
   /// Constructor.
   Alignment() {}
   /// Destructor.
   ~Alignment() {}

   /// Read one aligment from the input stream.
   /// @param in  input stream.
   /// @return Returns true if there is still some alignment left to read from in.
   bool read(istream &in);
   /// Output the alignment to a stream.
   /// @param out  output stream.
   void write(ostream &out);

   /// @name sorting
   /// The two next functions change the order of the list's elements.
   //@{
   void sortOnSource() { sort(begin(), end(), AlignedPair::sourceLessThan); }
   void sortOnTarget() { sort(begin(), end(), AlignedPair::targetLessThan); }
   //@}
};
}

#endif  // BASIC_DATA_STRUCTURE_H
