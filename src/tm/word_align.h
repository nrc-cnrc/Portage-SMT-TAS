/**
 * @author George Foster
 * @file word_align.h
 * @brief The word alignment module used in gen_phrase_tables:
 * includes abstract interface, factory class, and aligner classes.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef WORD_ALIGN_H
#define WORD_ALIGN_H

#include <vector>
#include "phrase_table.h"
#include "ibm.h"
#include "word_align_io.h"

namespace Portage {

/**
 * Defines word alignment.  Virtual base class for all word aligner schemes.
 */
class WordAligner {

public:

   /**
    * Word-align two sentences.
    * @param toks1 sentence in language 1
    * @param toks2 sentence in language 2
    * @param[out] sets1 For each token position in toks1, will contain a set of
    * corresponding token positions in toks 2. Tokens that have no direct
    * correspondence (eg "le" in "m. le president / mr. president") should be
    * left out of the alignment, ie by giving them an empty set if they are in
    * toks1, or not including their position in any set if they are in toks2.
    * Tokens for which a translation is missing (eg "she" and "ils" in "she
    * said / ils ont dit") should be explicitly aligned to the end position in
    * the other language, ie by putting toks2.size() in the corresponding set if
    * they are in toks1, or by including their position in sets1[toks1.size()]
    * if they are in toks2. This final element in sets1 is optional; sets1 may
    * be of size toks1.size() if no words in toks2 are considered untranslated.
    * @return a score that indicates how reliable the alignment is, used to
    * weight the extracted phrase pairs. (in reality, we always return 1.0)
    */
   virtual double align(const vector<string>& toks1,
                        const vector<string>& toks2,
                        vector< vector<Uint> >& sets1) = 0;

   /// Destructor.
   virtual ~WordAligner() {}

   /// Indicates whether the constructor encountered any problems.
   /// @return true if there were not problems at construction time
   bool bad() { return bad_constructor_argument; }

   /**
    * Determine the closure of the given word alignment: whenever there is an
    * alignment path between a source word and a target word, add a direct link
    * connecting the two. The algorithm exploits the standard representation that
    * maintains a set of connected L2 words for each L1 word. If the sets for two
    * different L1 words have at least one element in common, that means that a
    * path exists between each of these L1 words and any element in either of
    * their sets of connected words (from an L2 word to the directly-connected L1
    * word, then to the common L2 word, then to other L1 word). Therefore the
    * connections for both L1 words are the union of their original sets of
    * connections. So the algorithm boils down to: given some sets of integers,
    * merge any two sets that have at least one element in common. Repeat until no
    * further merging is possible. Plus some bookkeeping to keep track of the L1
    * words that go with the merged sets.
    *
    * Words that are unaligned, or that are explicitly aligned to the end position
    * in the other language (and to no other words), are not affected by this
    * operation.
    *
    * @param sets1 word alignments in std WordAligner format. These are modified
    * by adding links for closure.
    * @param csets sets of L1 words that share the same set of L2 connections.
    * These are disjoint and cover all L1 words.
    */
   static void close(vector< vector<Uint> >& sets1, vector< vector<Uint> >& csets);

protected:
   // An subclass constructor should not fail when passed an invalid argument;
   // instead, it should set this flag and use a valid default value instead.
   bool bad_constructor_argument;

   /// Default constructor initializes bad_constructor_argument to false.
   WordAligner() : bad_constructor_argument(false) {}

};

/**
 * Factory class for creating aligners from text descriptions. To add new
 * classes derived from WordAligner, make sure they have constructors that
 * take a factory and a single string as arguments, then add them to the
 * static tinfos[] table in word_align.cc, along with name and help info.
 * See IBMOchAligner for an example.
 */
class WordAlignerFactory
{
   IBM1* ibm_lang2_given_lang1;
   IBM1* ibm_lang1_given_lang2;

   GizaAlignmentFile* file_lang2_given_lang1;
   GizaAlignmentFile* file_lang1_given_lang2;

   Uint verbose;
   bool twist;
   bool addSingleWords;
   bool allow_linkless_pairs;

   /// Generic object to create various WordAligner.
   template<class T> struct DCon {
      /**
       * Allocates a new object of type T constructed with T(factory, s).
       * @param factory
       * @param s        see tinfos[] in word_align.cc for interpretation
       * @return Returns a new object of type T(factory, s)
       */
      static WordAligner* create(WordAlignerFactory& factory, const string& s)
      {
         return new T(factory, s);
      }
   };

   /// Definition of the signature of the function used to create WordAligners.
   typedef WordAligner* (*PF)(WordAlignerFactory& factory, const string& s);

   /// Creational information for WordAlignerFactory
   struct TInfo {
      PF pf;                    ///< pointer to create(WordAlignerFactory&, const string&) function
      string tname;             ///< name of derived class
      string help;              ///< describes args for derived class constructor
   };

   static TInfo tinfos[];       ///< array containing all known aligners

   /// @name scratch space for addPhrases
   //@{
   vector<Uint> earliest1;
   vector<Uint> earliest2;
   vector<Uint> latest1;
   vector<Uint> latest2;
   //@}

public:

   /**
    * Construct with IBM models in both directions. These are assumed to be
    * needed for alignment.
    * @param ibm_lang2_given_lang1 May point to IBM1 or IBM2
    * @param ibm_lang1_given_lang2 ""
    * @param verbose level: 0 for no messages, 1 for basic, 2 for detail
    * @param twist With IBM1, assume one language has reverse word order. No
    * effect with IBM2
    * @param addSingleWords add single-word phrase pairs for each alignment link
    * @param allow_linkless_pairs during phrase extraction, allow phrase pairs
    *        that consist only of unaligned words in each language
    */
   WordAlignerFactory(IBM1* ibm_lang2_given_lang1, IBM1* ibm_lang1_given_lang2,
                      Uint verbose, bool twist, bool addSingleWords,
                      bool allow_linkless_pairs = false);

   /**
    * Construct with GizaAlignmentFile in both directions.
    * @param file_lang2_given_lang1 must point to src - tgt
    * @param file_lang1_given_lang2 must point to tgt - src
    * @param verbose level: 0 for no messages, 1 for basic, 2 for detail
    * @param twist no effect (currently)
    * @param addSingleWords add single-word phrase pairs for each alignment link
    * @param allow_linkless_pairs during phrase extraction, allow phrase pairs
    *        that consist only of unaligned words in each language
    */
   WordAlignerFactory(GizaAlignmentFile* file_lang2_given_lang1,
                      GizaAlignmentFile* file_lang1_given_lang2,
                      Uint verbose, bool twist, bool addSingleWords,
                      bool allow_linkless_pairs = false);

   /**
    * Create a new aligner according to specifications.
    * @param tname class of aligner
    * @param args args to pass to aligner's constructor
    * @param fail fail with error if tname is not known, rather than returning
    *             NULL
    * @return Returns a pointer to a new WordAligner.
    */
   WordAligner* createAligner(const string& tname, const string& args,
                              bool fail = true);

   /**
    * Create a new aligner according to specifications.
    * @param tname_and_args  class of aligner and args to pass to aligner's
    *                        constructor.
    * @param fail            fail with error if tname is not known, rather than
    *                        returning NULL
    * @return Returns a pointer to a new WordAligner.
    */
   WordAligner* createAligner(const string& tname_and_args, bool fail = true) {
      vector<string> toks;
      toks.clear();
      split(tname_and_args, toks, " \n\t", 2);
      toks.resize(2);
      return createAligner(toks[0], toks[1], fail);
   }

   /**
    * Return help message describing all known aligner methods.
    * @return Return help message describing all known aligner methods.
    */
   static string help();

   /**
    * Return help message describing a given aligner method.
    * @param tname  word aligner's name
    * @return Return help message describing a given aligner method.
    */
   static string help(const string& tname);

   /**
    * Write representation of alignment to cerr.
    * @param toks1  sentence in language 1
    * @param toks2  sentence in language 2
    * @param[out] sets1  for each token position in toks1, a set of
    *   corresponding token positions in toks 2.
    */
   void showAlignment(const vector<string>& toks1,
                      const vector<string>& toks2,
                      const vector< vector<Uint> >& sets1);

   /// @name IBM model access.
   /// @return Returns the IBM model.
   //@{
   IBM1* getIBMLang1GivenLang2() {return ibm_lang1_given_lang2;}
   IBM1* getIBMLang2GivenLang1() {return ibm_lang2_given_lang1;}
   //@}

   /// @name Get the GIZA alignment file.
   /// @return Returns the GIZA alignment file.
   //@{
   GizaAlignmentFile* getFileLang1GivenLang2() {return file_lang1_given_lang2;}
   GizaAlignmentFile* getFileLang2GivenLang1() {return file_lang2_given_lang1;}
   //@}

   /// @name Get the aligner
   /// @return Returns the IBMAligner.
   //@{
   IBMAligner* getAlignerLang1GivenLang2() {
     if (ibm_lang1_given_lang2)
       return ibm_lang1_given_lang2;
     else
       return file_lang1_given_lang2;
   }
   IBMAligner* getAlignerLang2GivenLang1() {
     if (ibm_lang2_given_lang1)
       return ibm_lang2_given_lang1;
     else
       return file_lang2_given_lang1;
   }
   //@}

   /// @name self explanatorily named get methods.
   //@{
   Uint getVerbose() {return verbose;}
   bool getTwist() {return twist;}
   bool getAddSingleWords() {return addSingleWords;}
   //@}

   /**
    * Structure to represent a phrase pair by indexes into a sentence
    * pair. Used for the optional phrase_pairs argument to addPhrases.
    */
   struct PhrasePair {
      Uint beg1;                // start index in language 1 sentence
      Uint end1;                // end+1 index in language 1 sentence
      Uint beg2;                // start index in language 2 sentence
      Uint end2;                // end+1 index in language 2 sentence

      PhrasePair() {}
      PhrasePair(Uint beg1, Uint end1, Uint beg2, Uint end2) :
         beg1(beg1), end1(end1), beg2(beg2), end2(end2) {}

      bool overlap(PhrasePair& pp) {
         return
            (pp.beg1 < end1 && beg1 < pp.end1) ||
            (pp.beg2 < end2 && beg2 < pp.end2);
      }

      void dump(ostream& os) {
         os << "[" << beg1 << "," << end1 << ")[" << beg2 << "," << end2 << ")";
      }
      void dump(ostream& os, const vector<string>& toks1, const vector<string>& toks2) {
         os << join(toks1.begin()+beg1, toks1.begin()+end1, "_") << "/"
            << join(toks2.begin()+beg2, toks2.begin()+end2, "_");
      }

   };

   /**
    * Add all phrases licensed by a given alignment to a phrase table.
    * NB: This probably isn't the best place for this fcn, but it will squat
    * here until it gets a better home. Implementation is at the end of this
    * file. Note that the behaviour of this function is also influenced by the
    * class-level parameters addSingleWords and allow_linkless_pairs (these
    * should be parameters, since they only pertain to this function).
    * @param toks1 sentence in language 1
    * @param toks2 sentence in language 2
    * @param sets1 For each token position in toks1, a set of corresponding
    * token positions in toks 2, as output from align().
    * @param max_phrase_len1 maximum allowable phrase length from toks1
    * @param max_phrase_len2 maximum allowable phrase length from toks2
    * @param max_phraselen_diff maximum allowable difference in paired phrase lengths
    * @param min_phrase_len1 minimum allowable phrase length from toks1
    * @param min_phrase_len2 minimum allowable phrase length from toks2
    * @param pt destination phrase table
    * @param ct count each pair as having occurred this many times
    * @param phrase_pairs if non-null, output goes here INSTEAD of pt
    * @param store_alignments if true, alignment info is stored in pt with each phrase pair.
    */
   template<class T>
   void addPhrases(const vector<string>& toks1, const vector<string>& toks2,
                   const vector< vector<Uint> >& sets1,
                   Uint max_phrase_len1, Uint max_phrase_len2, Uint max_phraselen_diff,
                   Uint min_phrase_len1, Uint min_phrase_len2,
                   PhraseTableGen<T>& pt, T ct=1,
                   vector<PhrasePair>* phrase_pairs = NULL,
                   bool store_alignments = false);
};

/**
 * Tallying object to display alignment statistics.
 */
class WordAlignerStats {
   Uint total_count;             ///< number of sentence pairs processed
   double link_ratio_sum;        ///< sum of link ratios
   double link_ratio_sum2;       ///< sum of squares of link ratios
   double exclude_ratio_l1_sum;  ///< sum of l1 exclude ratios
   double exclude_ratio_l1_sum2; ///< sum squares of l1 exclude ratios
   double exclude_ratio_l2_sum;  ///< sum of l2 exclude ratios
   double exclude_ratio_l2_sum2; ///< sum squares of l2 exclude ratios
   double null_ratio_l1_sum;     ///< sum of l1 null ratios
   double null_ratio_l1_sum2;    ///< sum squares of l1 null ratios
   double null_ratio_l2_sum;     ///< sum of l2 null ratios
   double null_ratio_l2_sum2;    ///< sum squares of l2 null ratios

public:
   /// Constructor.
   WordAlignerStats() { reset(); }

   /// Reset all counters.
   void reset();

   /// Tally all relevant statistics from sets1.
   /// @param sets1 alignment calculated by WordAligner::align() or a subclass.
   /// @param l1_length size of l1 sentence
   /// @param l2_length size of l2 sentence
   void tally(const vector<vector<Uint> >& sets1,
              Uint l1_length, Uint l2_length);

   /// Print all statistics.
   /// @param l1 short name for L1
   /// @param l2 short name for L2
   /// @param os where to output the statistics
   void display(const string& l1 = "l1", const string& l2 = "l2",
                ostream& os = cerr);

}; // class WordAlignerStats

/**
 * Basic Och-style symmetric alignment.
 */
class IBMOchAligner : public WordAligner
{
protected:

   WordAlignerFactory& factory;

   IBMAligner* aligner_lang1_given_lang2;
   IBMAligner* aligner_lang2_given_lang1;

   /**
    * -1 = forward ibm alignment (ignore reverse)
    * -2 = reverse ibm alignment (ignore forward)
    *  1 = intersection only,
    *  2 = expand to union
    *  3 = align all
    */
   int strategy;
   bool exclude;                ///< exclude unlinked words from phrases

   vector<Uint> al1;
   vector<Uint> al2;

   vector<bool> connected2;
   vector<Uint> new_pairs;

   /**
    *
    * @param ii
    * @param jj
    * @param sets1
    * @return
    */
   bool addTest(int ii, int jj, vector< vector<Uint> >& sets1);

   /**
    * Outputs the alignments to cerr.
    * @param toks1
    * @param toks2
    * @param al2
    */
   void showAlignment(const vector<string>& toks1, const vector<string>& toks2,
                      const vector<Uint>& al2);

public:

   /**
    * Construct with ref to factory and arg string.
    * @param factory
    * @param args see tinfos[] in word_align.cc for interpretation
    */
   IBMOchAligner(WordAlignerFactory& factory, const string& args);

   /**
    * Do a IBM Och alignment.
    * @param toks1  sentence in language 1
    * @param toks2  sentence in language 2
    * @param[out] sets1  for each token position in toks1, a set of
    *   corresponding token positions in toks 2.
    * @return a score that indicates how reliable the alignment is, used to
    * weight the extracted phrase pairs. (in reality, we always return 1.0)
    */
   virtual double align(const vector<string>& toks1,
                        const vector<string>& toks2,
                        vector< vector<Uint> >& sets1);
};

/**
 * Like IBMOchAligner but with stricter diagonalization heuristic. This differs
 * from IBMOchAligner only in the strategy 2 and 3 methods used.
 */
class IBMDiagAligner : public IBMOchAligner
{
public:

   /**
    * Construct with ref to factory and arg string.
    * @param factory
    * @param args see tinfos[] in word_align.cc for interpretation
    */
   IBMDiagAligner(WordAlignerFactory& factory, const string& args);

   /**
    * Carry out word alignment.
    * @param toks1  sentence in language 1
    * @param toks2  sentence in language 2
    * @param[out] sets1  for each token position in toks1, a set of
    *   corresponding token positions in toks 2.
    * @return a score that indicates how reliable the alignment is, used to
    * weight the extracted phrase pairs. (in reality, we always return 1.0)
    */
   virtual double align(const vector<string>& toks1,
                        const vector<string>& toks2,
                        vector< vector<Uint> >& sets1);
};


/**
 * Symmetrized-score alignment
 * TODO:
 * - implement twist or say no to this
 * - stopping conditions
 * - remove unlinked words...
 */
class IBMScoreAligner : public WordAligner
{
   WordAlignerFactory& factory;

   IBM1* ibm_lang1_given_lang2;
   IBM1* ibm_lang2_given_lang1;

   bool norm1;                  ///< s(w1|w2) = p(w1|w2) / sum_w p(w1|w)
   bool norm2;                  ///< s(w2|w1) = p(w2|w1) / sum_w p(w2|w)
   double thresh;               ///< cutoff for links, relative to best link

   vector< vector<double> > score_matrix; ///< w1,w2 -> score(w1,w2)
   vector<double> col;          ///< utility (column of scores in score_matrix)

   vector< pair< double,pair<Uint,Uint> > > links; ///< (score, (pos1,pos2))

   vector<Uint> connections2;   ///< pos2 -> number of connections for that word

   /**
    * Normalizes v.
    * @param v  vector to normalise.
    */
   void normalize(vector<double>& v);

   /**
    * Outputs to cerr the link.
    * @param link
    * @param toks1
    * @param toks2
    */
   void showLink(const pair< double,pair<Uint,Uint> >& link,
                 const vector<string>& toks1, const vector<string>& toks2);

public:

   /**
    * Construct with ref to factory and arg string.
    * @param factory
    * @param args see tinfos[] in word_align.cc for interpretation
    */
   IBMScoreAligner(WordAlignerFactory& factory, const string& args);

   /**
    * Do a IBM score alignment.
    * @param toks1  sentence in language 1
    * @param toks2  sentence in language 2
    * @param[out] sets1  for each token position in toks1, will contain a set
    *   of corresponding token positions in toks 2.
    * @return a score that indicates how reliable the alignment is, used to
    * weight the extracted phrase pairs. (in reality, we always return 1.0)
    */
   virtual double align(const vector<string>& toks1,
                        const vector<string>& toks2,
                        vector< vector<Uint> >& sets1);
};

/**
 * Register all words as having no direct correspondence, so that addPhrases
 * will add the cartesian product of phrases.
 */
class CartesianAligner : public WordAligner
{
public:

   /**
    * Construct with ref to factory and arg string.
    * @param factory
    * @param args see tinfos[] in word_align.cc for interpretation
    */
   CartesianAligner(WordAlignerFactory& factory, const string& args) {}

   /**
    * Do a Cartesian alignment.
    * @param toks1  sentence in language 1
    * @param toks2  sentence in language 2
    * @param[out] sets1  for each token position in toks1, will contain a set
    *   of corresponding token positions in toks 2.
    * @return a score that indicates how reliable the alignment is, used to
    * weight the extracted phrase pairs. (in reality, we always return 1.0)
    */
   virtual double align(const vector<string>& toks1,
                        const vector<string>& toks2,
                        vector< vector<Uint> >& sets1);
};

/**
 * Align word for word sentence 1 to sentence 2.
 */
class IdentityAligner : public WordAligner
{
public:

   /**
    * Construct with ref to factory and arg string.
    * @param factory
    * @param args see tinfos[] in word_align.cc for interpretation
    */
   IdentityAligner(WordAlignerFactory& factory, const string& args) {}

   /**
    * Do an identity alignment : sets the diagonal.  If either sentence is
    * longer than the other, extra words are NULL aligned, so they can't
    * participate in any phrases, when the alignment is used for phrase
    * extraction.
    * @param toks1  sentence in language 1
    * @param toks2  sentence in language 2
    * @param[out] sets1  for each token position in toks1, will contain a set
    *   corresponding to itself in toks2
    * @return 1.0
    */
   virtual double align(const vector<string>& toks1,
                        const vector<string>& toks2,
                        vector< vector<Uint> >& sets1);
};

/**
 * Aligner using posterior "decoding" with a delta threshold.
 * Based on Liang, Taskar and Klein (HLT 2006), this method uses the product of
 * posterior probabilities in each direction for each link, instead of using
 * the Viterbi alignment from each directional model.  They claim a reduced AER
 * using this method, we have to experiment to see if it increases BLEU.
 */
class PosteriorAligner : public WordAligner
{
   IBM1* ibm_lang1_given_lang2;
   IBM1* ibm_lang2_given_lang1;

   /// Posterior alignment threshold - keep links with posterior prod >= delta.
   double delta;
   /// Mark words that have no links with posterior above exclude_threshold as
   /// non-translated, therefore to be excluded from phrases.
   double exclude_threshold;

   /// verbose parameter copied from factory
   Uint verbose;

   vector<vector<double> > posteriors_lang1_given_lang2;
   vector<vector<double> > posteriors_lang2_given_lang1;

public:
   /**
    * Construct with ref to factory and arg string.
    * @param factory
    * @param args see tinfos[] in word_align.cc for interpretation; must be
    *             convertible to a float
    */
   PosteriorAligner(WordAlignerFactory& factory, const string& args);

   /**
    * Calculate a posterior alignment.
    * @param toks1  sentence in language 1
    * @param toks2  sentence in language 2
    * @param[out] sets1  for each token position in toks1, will contain a set
    *   of corresponding token positions in toks 2.
    * @return a score that indicates how reliable the alignment is, used to
    * weight the extracted phrase pairs. (in reality, we always return 1.0)
    */
   virtual double align(const vector<string>& toks1,
                        const vector<string>& toks2,
                        vector< vector<Uint> >& sets1);
};


/**
 * HybridPostAligner. This is a hybrid between IBMOchAligner and
 * PosteriorAligner.  Start out with the intersection of two one-way
 * alignments, which has a much higher precision than anything that can be
 * easily generated from PosteriorAligner. Then add connected links, in
 * decreasing order by posterior-prob score, but considering only the top pct%
 * scoring links. Adding is a bit tricky, since a lower-scoring link may create
 * a connection to a previously isolated higher-scoring link, so the list of
 * links has to be traversed many times. Link adding stops when all words in
 * each sentence have at least one connection, or when the list of links is
 * exhausted. When this happens, all remaining unlinked words are assigned
 * their top-scoring translation, provided the link probability of this
 * translation is among the top pct%. Words without such a translation are left
 * unlinked (but not explicitly null-aligned).
 *
 * Implementation note: this uses quite a large number of l1xl2 matrices,
 * where l1 and l2 are the lengths of the input sentences. This space
 * requirement could probably be reduced.
 */
class HybridPostAligner : public WordAligner
{
   IBM1* ibm_lang1_given_lang2;
   IBM1* ibm_lang2_given_lang1;
   Uint verbose;

   double pct;			// proportion of top scoring links deemed valid

   vector<Uint> al1;		// one-way alignment
   vector<Uint> al2;		// other-way alignment
   vector<bool> connected1;	// l1 pos -> word has link
   vector<bool> connected2;	// l2 pos -> word has link
   vector<vector<bool> > links;	// pos1,pos2 -> linked or not

   vector<vector<double> > posteriors_lang1_given_lang2;
   vector<vector<double> > posteriors_lang2_given_lang1;
   vector<vector<double> > posteriors; // pos1,pos2 -> posterior prob
   vector<vector<Uint> > ranks;	// pos1,pos2 -> rank within link_list
   vector<Uint> link_list;	// rank -> index = pos1 * size1 + pos2, by decr score

public:

   /**
    * Construct with ref to factory and arg string.
    * @param factory
    * @param args see tinfos[] in word_align.cc for interpretation
    */
   HybridPostAligner(WordAlignerFactory& factory, const string& args);

   /**
    * Calculate a posterior alignment.
    * @param toks1  sentence in language 1
    * @param toks2  sentence in language 2
    * @param[out] sets1  for each token position in toks1, will contain a set
    *   of corresponding token positions in toks 2.
    * @return a score that indicates how reliable the alignment is, used to
    * weight the extracted phrase pairs. (in reality, we always return 1.0)
    */
   virtual double align(const vector<string>& toks1,
                        const vector<string>& toks2,
                        vector< vector<Uint> >& sets1);
};

/**
 * Test class to read in external symmetrized alignments in Moses (aka "sri") format.
 * @param factory
 * @param args name of a file containing alignments in Moses/SRI format (srcpos-tgtpos),
 * one per line.
 */
class ExternalAligner : public WordAligner, private NonCopyable
{
   string ext_file_name;
   Uint lineno;
   istream* external_alignments;
   SRIReader reader;

public:

   ExternalAligner(WordAlignerFactory& factory, const string& args);

   ~ExternalAligner();

   virtual double align(const vector<string>& toks1,
                        const vector<string>& toks2,
                        vector< vector<Uint> >& sets1);
};


/**
 * Class to use an internal aligner: the sets1 argument already contains the
 * alignments before align() is called.
 */
class InternalAligner : public WordAligner
{
public:
   InternalAligner(WordAlignerFactory& factory, const string& args) {}
   virtual double align(const vector<string>& toks1,
                        const vector<string>& toks2,
                        vector< vector<Uint> >& sets1)
   { return 0; }
};


/*---------------------------------------------------------------------------
  Definitions only past this point; if you just want to use these classes,
  stop reading here!
 *--------------------------------------------------------------------------*/

/*
 * For each word, we keep track of the range [e,l] of its earliest and latest
 * connection positions in the other sentence. "Joiner" words are coded as
 * [s,0] and "splitter" words as [s,s] (where s is the size of the other
 * sentence) - neither of these ranges can occur for normal words. The range
 * for a phrase candidate is the span of the ranges of its constituent words. A
 * phrase pair is valid if each phrase contains the other's range. Joiner and
 * splitter word have the desired properties due to their encoding: the empty
 * range for joiner words will never affect the range of a surrounding phrase;
 * and the out-of-bounds range for splitter words always prevents any
 * surrounding phrase from being paired.
 * Note: "Joiner" words are the unaligned ones, while "splitter" words are the
 * NULL-aligned ones.
 */
template <class T>
void WordAlignerFactory::addPhrases(const vector<string>& toks1, const vector<string>& toks2,
                                    const vector< vector<Uint> >& sets1,
                                    Uint max_phrase_len1, Uint max_phrase_len2, Uint max_phraselen_diff,
                                    Uint min_phrase_len1, Uint min_phrase_len2,
                                    PhraseTableGen<T>& pt, T ct, vector<PhrasePair>* phrase_pairs,
                                    bool store_alignments)
{
   // Avoid overflow when max_phrase_len is "infinite": Uint(-1).
   if (max_phrase_len1 > toks1.size()) max_phrase_len1 = toks1.size() + 1;
   if (max_phrase_len2 > toks2.size()) max_phrase_len2 = toks2.size() + 1;

   if (phrase_pairs)
      phrase_pairs->clear();

   string green_alignment_s; // reduce frequent alloc/realloc
   const char* green_alignment(NULL);
   GreenWriter green_writer;
   if ( store_alignments && addSingleWords) {
      // all single links have the same alignment, so we don't call
      // green_writer.write_partial_alignment for this case.
      green_alignment_s = "0";
      green_alignment = green_alignment_s.c_str();
   }

   // initialize all words with empty range
   earliest1.assign(toks1.size(), toks2.size()); latest1.assign(toks1.size(), 0);
   earliest2.assign(toks2.size(), toks1.size()); latest2.assign(toks2.size(), 0);

   for (Uint i = 0; i < sets1.size(); ++i) {
      if (sets1[i].size() && i < toks1.size()) {
         earliest1[i] = sets1[i].front();
         latest1[i] = sets1[i].back();
      }
      for (Uint j = 0; j < sets1[i].size(); ++j) {
         const Uint jj = sets1[i][j];
         if (jj < toks2.size()) {
            earliest2[jj] = min(earliest2[jj], i);
            latest2[jj] = max(latest2[jj], i);

            if (addSingleWords) {
               if (phrase_pairs)
                  phrase_pairs->push_back(PhrasePair(i, i+1, jj, jj+1));
               else
                  pt.addPhrasePair(toks1.begin()+i, toks1.begin()+i+1,
                                   toks2.begin()+jj, toks2.begin()+jj+1,
                                   ct, green_alignment);
            }
         }
      }
   }

   for (Uint b1 = 0; b1 < toks1.size(); ++b1) {
      Uint ea1 = earliest1[b1], la1 = latest1[b1];
      for (Uint e1 = b1+min_phrase_len1; e1 <= min(size_t(b1+max_phrase_len1), toks1.size()); ++e1) {
         if (latest1[e1-1] == toks2.size())
            break;            // splitter word; no phrase possible for [b1,e1+)
         ea1 = min(ea1, earliest1[e1-1]); la1 = max(la1, latest1[e1-1]);

         for (Uint b2 = 0; b2 < toks2.size(); ++b2) {
            Uint ea2 = earliest2[b2], la2 = latest2[b2];
            for (Uint e2 = b2+min_phrase_len2; e2 <= min(size_t(b2+max_phrase_len2), toks2.size()); ++e2) {
               if (latest2[e2-1] == toks1.size())
                  break;      // splitter word; no phrase possible for [b2,e2+)
               ea2 = min(ea2, earliest2[e2-1]); la2 = max(la2, latest2[e2-1]);

               if ((ea1 >= b2 && la1 < e2 && ea2 >= b1 && la2 < e1) &&
                   (allow_linkless_pairs || la1 >= ea1 || la2 >= ea2) &&
                   (Uint(abs((int)e1 - (int)b1 - (int)e2 + (int)b2)) <= max_phraselen_diff)) {

                  if (phrase_pairs)
                     phrase_pairs->push_back(PhrasePair(b1,e1,b2,e2));
                  else {
                     if ( store_alignments ) {
                        green_writer.write_partial_alignment(green_alignment_s,
                           toks1, b1, e1, toks2, b2, e2, sets1, '_');
                        green_alignment = green_alignment_s.c_str();
                     }
                     pt.addPhrasePair(toks1.begin()+b1, toks1.begin()+e1,
                                      toks2.begin()+b2, toks2.begin()+e2,
                                      ct, green_alignment);
                  }

                  if (verbose > 1) {
                     const string p1 = join(toks1.begin()+b1, toks1.begin()+e1, "_");
                     const string p2 = join(toks2.begin()+b2, toks2.begin()+e2, "_");
                     cerr << b1 << ':' << p1 << ':' << e1 << "/" << b2 << ':' << p2 << ':' << e2 << " ";
                     if ( store_alignments ) cerr << "a=(" << green_alignment << ") ";
                  }
               }
            }
         }
      }
   }
   if (verbose > 1) cerr << endl;
}


} // namespace Portage

#endif
