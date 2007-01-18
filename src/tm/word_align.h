/**
 * @author George Foster
 * @file word_align.h  The word alignment module used in gen_phrase_tables:
 * includes abstract interface, factory class, and aligner classes.
 * 
 * 
 * COMMENTS:
 *
 * The word alignment module used in gen_phrase_tables: includes abstract
 * interface, factory class, and aligner classes.
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef WORD_ALIGN_H
#define WORD_ALIGN_H

#include <vector>
#include "phrase_table.h"
#include "ibm.h"

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
    * @param sets1 For each token position in toks1, a set of corresponding
    * token positions in toks 2. Tokens that have no direct correspondence
    * (eg "le" in "m. le president / mr. president") should be left out of
    * the alignment, ie by giving them an empty set if they are in toks1, or
    * not including their position in any set if they are in toks2. Tokens for
    * which a translation is missing (eg "she" and "ils" in "she said / ils ont
    * dit") should be explicitly aligned to the end position in the other
    * language, ie by putting toks2.size() in corresponding set if they are in
    * toks1, or by including their position in sets1[toks1.size()] if they are
    * in toks2. This final element in sets1 is optional; sets1 may be of size 
    * toks1.size() if no words in toks2 are considered untranslated.
    * @return a score that indicates how reliable the alignment is, used to
    * weight the extracted phrase pairs.
    */
   virtual double align(const vector<string>& toks1, const vector<string>& toks2, 
			vector< vector<Uint> >& sets1) = 0;

   /// Destructor.
   virtual ~WordAligner() {}

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
      PF pf;			///< pointer to create(WordAlignerFactory&, const string&) function
      string tname;		///< name of derived class
      string help;		///< describes args for derived class constructor
   };

   static TInfo tinfos[];	///< array containing all known aligners

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
    */
   WordAlignerFactory(IBM1* ibm_lang2_given_lang1, IBM1* ibm_lang1_given_lang2,
                      Uint verbose, bool twist, bool addSingleWords);

   /**
    * Construct with GizaAlignmentFile in both directions. 
    * @param file_lang2_given_lang1 must point to src - tgt
    * @param file_lang1_given_lang2 must point to tgt - src
    * @param verbose level: 0 for no messages, 1 for basic, 2 for detail
    * @param twist no effect (currently)
    * @param addSingleWords add single-word phrase pairs for each alignment link
    */
   WordAlignerFactory(GizaAlignmentFile* file_lang2_given_lang1, 
                      GizaAlignmentFile* file_lang1_given_lang2,
		      Uint verbose, bool twist, bool addSingleWords);

   /**
    * Create a new aligner according to specifications.
    * @param tname class of aligner
    * @param args args to pass to aligner's constructor
    * @param fail fail with error in case of problems, rather than returning NULL
    * @return Returns a pointer to a new WordAligner.
    */
   WordAligner* createAligner(const string& tname, const string& args, bool fail = true);

   /**
    * Create a new aligner according to specifications.
    * @param tname_and_args  class of aligner and args to pass to aligner's constructor.
    * @param fail fail with error in case of problems, rather than returning NULL
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
    * @param sets1  for each token position in toks1, a set of corresponding
    * token positions in toks 2.
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

   /// @name self explanatory named get methods.
   //@{
   Uint getVerbose() {return verbose;}
   bool getTwist() {return twist;}
   bool getAddSingleWords() {return addSingleWords;}
   //@}

   /**
    * Add all phrases licensed by a given alignment to a phrase table.
    * NB: This probably isn't the best place for this fcn, but it will squat
    * here until it gets a better home. Implementation is at the end of this
    * file.
    *
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
    */
   template<class T>
   void addPhrases(const vector<string>& toks1, const vector<string>& toks2,
		   const vector< vector<Uint> >& sets1, 
		   Uint max_phrase_len1, Uint max_phrase_len2, Uint max_phraselen_diff,
             Uint min_phrase_len1, Uint min_phrase_len2, 
		   PhraseTableGen<T>& pt, T ct=1);
};

/**
 * Basic Och-style symmetric alignment.
 */
class IBMOchAligner : public WordAligner 
{
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
   bool exclude; 		///< exclude unlinked words from phrases

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
    * @param sets1  for each token position in toks1, a set of corresponding
    * token positions in toks 2.
    * @return a score that indicates how reliable the alignment is, used to
    * weight the extracted phrase pairs.
    */
   double align(const vector<string>& toks1, const vector<string>& toks2, 
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

   bool norm1;			///< s(w1|w2) = p(w1|w2) / sum_w p(w1|w)
   bool norm2;			///< s(w2|w1) = p(w2|w1) / sum_w p(w2|w)
   double thresh;		///< cutoff for links, relative to best link

   vector< vector<double> > score_matrix; ///< w1,w2 -> score(w1,w2)
   vector<double> col;		///< utility (column of scores in score_matrix)

   vector< pair< double,pair<Uint,Uint> > > links; ///< (score, (pos1,pos2))

   vector<Uint> connections2;	///< pos2 -> number of connections for that word

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
    * @param sets1  for each token position in toks1, a set of corresponding
    * token positions in toks 2.
    * @return a score that indicates how reliable the alignment is, used to
    * weight the extracted phrase pairs.
    */
    double align(const vector<string>& toks1, const vector<string>& toks2, 
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
    * @param sets1  for each token position in toks1, a set of corresponding
    * token positions in toks 2.
    * @return a score that indicates how reliable the alignment is, used to
    * weight the extracted phrase pairs.
    */
   double align(const vector<string>& toks1, const vector<string>& toks2, 
		vector< vector<Uint> >& sets1);
};

/*---------------------------------------------------------------------------
  Definitions only past this point; if you just want to use these classes,
  stop reading here!
 *--------------------------------------------------------------------------*/

/**
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
 */
template <class T>
void WordAlignerFactory::addPhrases(const vector<string>& toks1, const vector<string>& toks2,
				    const vector< vector<Uint> >& sets1, 
				    Uint max_phrase_len1, Uint max_phrase_len2, Uint max_phraselen_diff,
				    Uint min_phrase_len1, Uint min_phrase_len2,
				    PhraseTableGen<T>& pt, T ct)
 {
   // initialize all words with empty range
   earliest1.assign(toks1.size(), toks2.size()); latest1.assign(toks1.size(), 0);
   earliest2.assign(toks2.size(), toks1.size()); latest2.assign(toks2.size(), 0);

   for (Uint i = 0; i < sets1.size(); ++i) {
      if (sets1[i].size() && i < toks1.size()) {
	 earliest1[i] = sets1[i].front();
	 latest1[i] = sets1[i].back();
      }
      for (Uint j = 0; j < sets1[i].size(); ++j) {
	 Uint jj = sets1[i][j];
	 if (jj < toks2.size()) {
	    earliest2[jj] = min(earliest2[jj], i);
	    latest2[jj] = max(latest2[jj], i);

         if (addSingleWords)
           pt.addPhrasePair(toks1.begin()+i, toks1.begin()+i+1,
                            toks2.begin()+jj, toks2.begin()+jj+1, ct);
	 }
      }
   }

   for (Uint b1 = 0; b1 < toks1.size(); ++b1) {
      Uint ea1 = earliest1[b1], la1 = latest1[b1];
      for (Uint e1 = b1+min_phrase_len1; e1 <= min(size_t(b1+max_phrase_len1), toks1.size()); ++e1) {
	 if (latest1[e1-1] == toks2.size()) 
	    break;	       // splitter word; no phrase possible for [b1,e1+)
	 ea1 = min(ea1, earliest1[e1-1]); la1 = max(la1, latest1[e1-1]);
	 
	 for (Uint b2 = 0; b2 < toks2.size(); ++b2) {
	    Uint ea2 = earliest2[b2], la2 = latest2[b2];
         for (Uint e2 = b2+min_phrase_len2; e2 <= min(size_t(b2+max_phrase_len2), toks2.size()); ++e2) {
	       if (latest2[e2-1] == toks1.size())
		  break;      // splitter word; no phrase possible for [b2,e2+)
	       ea2 = min(ea2, earliest2[e2-1]); la2 = max(la2, latest2[e2-1]);
	       
	       if ((ea1 > la1 || (ea1 >= b2 && la1 < e2)) &&
		   (ea2 > la2 || (ea2 >= b1 && la2 < e1)) &&
		   abs((int)e1 - (int)b1 - (int)e2 + (int)b2) <= max_phraselen_diff) {

  		  pt.addPhrasePair(toks1.begin()+b1, toks1.begin()+e1, 
  				   toks2.begin()+b2, toks2.begin()+e2, ct);

		  if (verbose > 1) {
		     string p1, p2;
		     PhraseTableBase::codePhrase(toks1.begin()+b1, toks1.begin()+e1, p1, "_");
		     PhraseTableBase::codePhrase(toks2.begin()+b2, toks2.begin()+e2, p2, "_");
		     cerr << p1 << "/" << p2 << " ";
		  }
	       }
	    }
	 }
      }
   }
   if (verbose > 1) cerr << endl;
}


}

#endif
