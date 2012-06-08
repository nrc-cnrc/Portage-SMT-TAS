/**
 * @author George Foster, with reduced memory usage mods by Darlene Stewart
 * @file phrase_smoother.h  Abstract interface and factory for phrase-table smoothing techniques.
 *
 *
 * COMMENTS:
 *
 * Abstract interface and factory for phrase-table smoothing techniques. See
 * PhraseSmootherFactory for instructions on how to add new smoothers.
 *
 * This is sort of unnecessarily polluted with templateness as a result of
 * PhraseTable being a template class, but the alternatives seem even clunkier
 * to me.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#ifndef PHRASE_SMOOTHER_H
#define PHRASE_SMOOTHER_H

#include <good_turing.h>
#include <cfloat>
#include "phrase_table.h"

namespace Portage {

//-----------------------------------------------------------------------------
/**
 * Abstract smoother. T is the type used to represent phrase-table frequencies.
 */
template<class T>
class PhraseSmoother
{
protected:
   Uint num_tallyMarginals_passes;   // number of passes needed by tallyMarginals
   Uint tallyMarginals_pass;         // current tallyMarginals pass number

public:

   // probabilities are represented as floats in canoe, so don't go below
   // FLT_MIN!
   static const double VERY_SMALL_PROB = FLT_MIN;

   /// Constructor.
   PhraseSmoother(Uint num_passes=1) :
      num_tallyMarginals_passes(num_passes), tallyMarginals_pass(1) {}

   /// Destructor.
   virtual ~PhraseSmoother() {}

   Uint numTallyMarginalsPasses() const {return num_tallyMarginals_passes;}
   Uint currentTallyMarginalsPass() const {return tallyMarginals_pass;}
   /**
    * Given a phrase pair (phrase1, phrase2), tally the marginals.
    * Can be called multiple times for multiple passes over the phrase table.
    * @param it
    */
   virtual void tallyMarginals(const typename PhraseTableGen<T>::iterator& it) {}
   /**
    * Do required computation upon completion of a pass of tallying the marginals.
    */
   virtual void tallyMarginalsFinishPass() {++tallyMarginals_pass;}

   /**
    * Given a phrase pair (phrase1, phrase2), return smoothed estimates for
    * P(phrase1|phrase2).
    * @param it
    * @return Returns  P(phrase1|phrase2)
    */
   virtual double probLang1GivenLang2(
      const typename PhraseTableGen<T>::iterator& it) = 0;
   /**
    * Given a phrase pair (phrase1, phrase2), return smoothed estimates for
    * P(phrase2|phrase1).
    * @param it
    * @return Returns  P(phrase2|phrase1)
    */
   virtual double probLang2GivenLang1(
      const typename PhraseTableGen<T>::iterator& it) = 0;

   /**
    * Return true if this smoother looks at joint counts from the phrasetable
    * (as opposed to just the phrases, like the IBM smoothers).
    */
   virtual bool usesCounts() = 0;

   /**
    * Return true if p(e|f) == p(f|e) always for this smoother.
    */
   virtual bool isSymmetrical() = 0;

};

//-----------------------------------------------------------------------------
/**
 * Factory class for creating smoothers from text descriptions. T is the type
 * used to represent phrase-table frequencies. To add new classes derived from
 * PhraseSmoother, make sure they have constructors that take a factory and a
 * single string as arguments, then add them to the static tinfos[] table in
 * phrase_smoother_cc.h, along with name and help info. See RFSmoother for an
 * example.
 */
template<class T>
class PhraseSmootherFactory
{
   PhraseTableGen<T>* phrase_table;
   Uint verbose;

   vector<IBM1*> ibm_lang2_given_lang1;       ///< IBM model(s)
   vector<IBM1*> ibm_lang1_given_lang2;       ///< IBM model(s)

   /// Generic object allocator for PhraseSmoothers<T>.
   template<class S> struct DCon {
      ///  Allocates a new object of type T constructed with S(factory, s).
      static PhraseSmoother<T>*
         create(PhraseSmootherFactory<T>& factory, const string& s)
      {
	 return new S(factory, s);
      }
   };

   /// PhraseSmoother<T> creation function definition.
   typedef PhraseSmoother<T>*
      (*PF)(PhraseSmootherFactory<T>& factory, const string& s);

   /// Creational information for PhraseSmoother<T>
   struct TInfo {
      PF pf;			///< pointer to create() function
      string tname;		///< name of derived class
      string help;		///< describes args for derived class ctor
      bool uses_counts;         ///< true iff smoother looks at jpt counts
      bool is_symmetrical;      ///< true iff p(e|f) == p(f|e) always
   };
   static TInfo tinfos[];	///< array containing all known smoothers


public:

   /**
    * Construct with phrase table and IBM models in both directions.
    * @param pt
    * @param ibm_lang2_given_lang1 May point to IBM1, IBM2, HMM or be NULL
    * @param ibm_lang1_given_lang2 ""
    * @param verbose level: 0 for no messages, 1 for basic, 2 for detail
    */
   PhraseSmootherFactory(PhraseTableGen<T>* pt,
                         IBM1* ibm_lang2_given_lang1,
                         IBM1* ibm_lang1_given_lang2,
			 Uint verbose);

   /**
    * Construct with phrase table and multiple IBM models in both
    * directions. IBM models are paired, ie ibm_lang2_given_lang1[i] is used
    * with ibm_lang1_given_lang2[i].
    * @param pt
    * @param ibm_lang2_given_lang1 0 or more pointers to IBM1, IBM2, or HMM
    * @param ibm_lang1_given_lang2 0 or more pointers to IBM1, IBM2, or HMM
    * @param verbose level: 0 for no messages, 1 for basic, 2 for detail
    */
   PhraseSmootherFactory(PhraseTableGen<T>* pt,
                         vector<IBM1*> ibm_lang2_given_lang1,
                         vector<IBM1*> ibm_lang1_given_lang2,
			 Uint verbose);

   /**
    * Create a new smoother according to specifications.
    * @param tname class of smoother
    * @param args args to pass to smoother's constructor
    * @param fail fail with error in case of problems, rather than returning NULL
    * @return Returns a new createSmoother.
    */
   PhraseSmoother<T>* createSmoother(const string& tname, const string& args,
                                     bool fail = true);

   /**
    * Create a new smoother according to specifications.
    * @param tname_and_args  class of smoother and args to pass to smoother's constructor
    * @param fail  fail with error in case of problems, rather than returning NULL
    * @return Returns a new createSmoother.
    */
   PhraseSmoother<T>* createSmoother(const string& tname_and_args, bool fail = true) {
      vector<string> toks;
      toks.clear();
      split(tname_and_args, toks, " \n\t", 2);
      toks.resize(2);
      return createSmoother(toks[0], toks[1], fail);
   }

   /**
    * Create one smoother and tally its marginals.
    * @param tname_and_args  class of smoother and args to pass to smoother's constructor
    * @param fail  fail with error in case of problems, rather than returning NULL
    * @return returns created smoother used to generate conditional probabilities
    */
   PhraseSmoother<T>* createSmootherAndTally(const string& tname_and_args, bool fail=true);

   /**
    * Create smoothers and tally the marginals.
    * @param smoothers vector of created smoothers used to generate conditional
    * probabilities
    * @param tname_and_args  vector of class names of smoothing methods with
    * args to pass to smoother's constructor
    * @param fail  fail with error in case of problems, rather than returning NULL
    */
   void createSmoothersAndTally(vector< PhraseSmoother<T>* >& smoothers,
                                vector<string> tnames_and_args, bool fail=true);

   /**
    * Get help message describing all known smoother methods.
    * @return Returns help message describing all known smoother methods.
    */
   static string help();

   /**
    * Get help message describing a given smoother method.
    * @param tname  name of the phrase smoother we need its help string.
    * @return Returns help message describing a given smoother method.
    */
   static string help(const string& tname);

   /**
    * Return true if given smoother method looks at joint counts from the
    * phrasetable (as opposed to just the phrases, like the IBM smoothers).
    */
   static bool usesCounts(const string& tname_and_args);

   /**
    * Return true if p(f|e) == p(e|f) always for a given smoother.
    */
   static bool isSymmetrical(const string& tname_and_args);

   /**
    * Get the ith IBM model, or return NULL if no such.
    */
   IBM1* getIBMLang1GivenLang2(Uint i = 0) {
      return ibm_lang1_given_lang2.size() > i ? ibm_lang1_given_lang2[i] : NULL;
   }
   IBM1* getIBMLang2GivenLang1(Uint i = 0) {
      return ibm_lang2_given_lang1.size() > i ? ibm_lang2_given_lang1[i] : NULL;
   }

   /**
    * Info functions.  Self explanatory named functions.
    */
   //@{
   PhraseTableGen<T>* getPhraseTable() {return phrase_table;}
   Uint getVerbose() {return verbose;}
   //@}
 };



//-----------------------------------------------------------------------------
/**
 * Plain relative-frequency estimates - with optional add-alpha smoothing
 */
template<class T>
class RFSmoother : public PhraseSmoother<T>
{
protected:

   double alpha;

   vector<T> lang1_marginals;   // l1 phrase -> total frequency
   vector<T> lang2_marginals;   // l2 phrase -> total frequency

   vector<Uint> lang1_numtrans;	// l1 phrase -> num different translations
   vector<Uint> lang2_numtrans;	// l2 phrase -> num different translations

public:

   /**
    * Constructor.
    * @param factory
    * @param args [alpha]
    */
   RFSmoother(PhraseSmootherFactory<T>& factory, const string& args);

   virtual void tallyMarginals(const typename PhraseTableGen<T>::iterator& it);

   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
   virtual bool usesCounts() {return true;}
   virtual bool isSymmetrical() {return false;}
};


//-----------------------------------------------------------------------------
/**
 * Don't estimate conditional probabilities - just write joint frequencies.
 */
template<class T>
class JointFreqs : public PhraseSmoother<T>
{
public:

   /**
    * Constructor does nothing.
    * @param factory
    * @param args
    */
   JointFreqs(PhraseSmootherFactory<T>& factory, const string& args) {}

   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it) {
      return it.getJointFreq();
   }
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it) {
      return it.getJointFreq();
   }
   virtual bool usesCounts() {return true;}
   virtual bool isSymmetrical() {return true;}
};

//-----------------------------------------------------------------------------
/**
 * Just indicate presence or absence of phrase pair.
 */
template<class T>
class PureIndicator : public PhraseSmoother<T>
{
public:
   /**
    * Constructor does nothing.
    * @param factory
    * @param args
    */
   PureIndicator(PhraseSmootherFactory<T>& factory, const string& args) : 
      PhraseSmoother<T>(0) {}

   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it) {
      return it.getJointFreq() ? 1.0 : 0.3;
   }
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it) {
      return it.getJointFreq() ? 1.0 : 0.3;
   }
   virtual bool usesCounts() {return true;}
   virtual bool isSymmetrical() {return true;}
};

//-----------------------------------------------------------------------------
/**
 * Marginal frequencies
 */
template<class T>
class MarginalFreqs : public PhraseSmoother<T>
{
   Uint lang;                   // 1 or 2
   vector<T> marginals;         // l1|l2 phrase -> total frequency

public:

   /**
    * Constructor
    * @param factory
    * @param args
    */
   MarginalFreqs(PhraseSmootherFactory<T>& factory, const string& args);

   virtual void tallyMarginals(const typename PhraseTableGen<T>::iterator& it);

   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it) {
      return marginals[it.getPhraseIndex(lang)];
   }
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it) {
      return marginals[it.getPhraseIndex(lang)];
   }
   virtual bool usesCounts() {return true;}
   virtual bool isSymmetrical() {return true;}
};


//-----------------------------------------------------------------------------
/**
 * Simulated leave-one-out.
 */
template<class T>
class LeaveOneOutSmoother : public RFSmoother<T>
{
   Uint strategy;

   double estm(T jointfreq, T margefreq);

public:

   /**
    * Constructor.
    * @param factory
    * @param args
    */
   LeaveOneOutSmoother(PhraseSmootherFactory<T>& factory, const string& args);

   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
   virtual bool usesCounts() {return true;}
   virtual bool isSymmetrical() {return false;}
};


//-----------------------------------------------------------------------------
/**
 * Good-Turing. The float/double implementation currently maps frequencies to
 * their integer ceilings in order to come up with smoothed counts. This may
 * not be the best binning method.
 */
template<class T>
class GTSmoother : public PhraseSmoother<T>
{
   vector<double> lang1_marginals;
   vector<double> lang2_marginals;

   vector<Uint> lang1_numtrans;	// l1 phrase -> num different translations
   vector<Uint> lang2_numtrans;	// l2 phrase -> num different translations

   // the following are used initially for the number of translations with 0
   // freq, then transformed into the probability assigned to each of those
   // translations

   vector<double> lang1_num0trans; // l1 phrase -> num translations with 0 freq
   vector<double> lang2_num0trans; // l2 phrase -> num translations with 0 freq

   // count_map is used by tallyMarginalsPass1 and tallyMarginalsPass1Finish.
   map<Uint,Uint> count_map;	// freq -> num phrases with this freq

   GoodTuring* gt;

public:

   /**
    * Constructor.
    * @param factory
    * @param args
    */
   GTSmoother(PhraseSmootherFactory<T>& factory, const string& args);
   /// Destructor.
   ~GTSmoother() {if (gt) delete gt;}

   virtual void tallyMarginals(const typename PhraseTableGen<T>::iterator& it);
   virtual void tallyMarginalsFinishPass();

   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
   virtual bool usesCounts() {return true;}
   virtual bool isSymmetrical() {return false;}
};

//-----------------------------------------------------------------------------
/**
 * Kneser-Ney. Similar comment applies to the above for G-T. Consider this a
 * very crude workaround for float counts.
 */
template<class T>
class KNSmoother : public PhraseSmoother<T>
{
   vector<double> lang1_marginals;
   vector<double> lang2_marginals;

   Uint numD;                   ///< number of discounting coeffs
   bool given;                  ///< coeffs were specified by user
   bool unigram;                ///< use unigram lower-order distribution
   Uint verbose;

   // c-1,i -> num L2 phrases with freq c paired with L1 phrase with index i (and
   // v.v.), for c = 1..numD
   vector< vector<Uint> > lang1_event_counts;
   vector< vector<Uint> > lang2_event_counts;

   Uint event_count_sum;
   double tot_freq;

   vector<Uint> global_event_counts; // c-1 -> num pairs with freq c, c = 1..numD+1
   vector<double> D;            // c-1 -> D(c), c = 1..numD

   /**
    *
    * @param desc
    * @param jointfreq
    * @param disc
    * @param gamma
    * @param lower_order
    * @param marge
    */
   void dumpProbInfo(const char* desc,
                     T jointfreq, double disc, double gamma, double lower_order, double marge)
   {
      cerr << desc << ": " <<
         jointfreq << "-" << disc << "+" << gamma << "*" << lower_order << "/" << marge << "  " <<
         jointfreq / marge << " -> " << (jointfreq - disc + gamma * lower_order) / marge << endl;
   }

public:

   /**
    * Constructor.
    * @param factory
    * @param args
    */
   KNSmoother(PhraseSmootherFactory<T>& factory, const string& args);
   /// Destructor.
   ~KNSmoother() {}

   virtual void tallyMarginals(const typename PhraseTableGen<T>::iterator& it);
   virtual void tallyMarginalsFinishPass();

   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
   virtual bool usesCounts() {return true;}
   virtual bool isSymmetrical() {return false;}
};


//-----------------------------------------------------------------------------
/**
 * Abstract class grouping all lexical smoothers, so they can also be used in a
 * stand-alone fashion.
 */
template<class T>
class LexicalSmoother : public PhraseSmoother<T>
{
   vector<string> l1_phrase;
   vector<string> l2_phrase;

protected:
   IBM1* ibm_lang2_given_lang1;
   IBM1* ibm_lang1_given_lang2;

   /**
    * Constructor for subclasses to use in the standard factory/iterator
    * framework.
    * @parameter args if non-empty, the first token should be a 1-based index
    * specifying the pair of ibm models to use in the constructor, eg via
    * factory.getIBMLang1GivenLang2(i-1) and factory.getIBMLang2GivenLang1(i-1).
    */
   LexicalSmoother(PhraseSmootherFactory<T>& factory, const string& args);

   /// Constructor for subclasses to use in the stand-alone case
   LexicalSmoother(IBM1* ibm_lang2_given_lang1, IBM1* ibm_lang1_given_lang2);

public:
   /// get p(l1|l2) outside the factory/iterator framework
   virtual double probLang1GivenLang2(const vector<string>& l1_phrase, const vector<string>& l2_phrase) = 0;
   /// get p(l2|l1) outside the factory/iterator framework
   virtual double probLang2GivenLang1(const vector<string>& l1_phrase, const vector<string>& l2_phrase) = 0;
   
   // standard factory/iterator framework API
   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
   virtual bool usesCounts() {return false;}
   virtual bool isSymmetrical() {return false;}
};

//-----------------------------------------------------------------------------
/**
 * Zens-Ney lexical smoothing. This differs from the above approaches in that
 * it ignores joint counts and derives conditional distributions strictly from
 * IBM1 probabilities.
 */
template<class T>
class ZNSmoother : public LexicalSmoother<T>
{
   vector<double> phrase_probs;

public:

   /**
    * Constructor for use in the standard factory/iterator framework
    * @param factory
    * @param args
    */
   ZNSmoother(PhraseSmootherFactory<T>& factory, const string& args);
   /// Constructor for use in the stand-alone case
   ZNSmoother(IBM1* ibm_lang2_given_lang1, IBM1* ibm_lang1_given_lang2);
   /// Destructor.
   ~ZNSmoother() {}

   virtual double probLang1GivenLang2(const vector<string>& l1_phrase, const vector<string>& l2_phrase);
   virtual double probLang2GivenLang1(const vector<string>& l1_phrase, const vector<string>& l2_phrase);
};


//-----------------------------------------------------------------------------
/**
 * IBM1 lexical smoothing. Like Zens-Ney, but use the standard IBM1
 * formula to calculate p(phrase1|phrase2).
 */
template<class T>
class IBM1Smoother : public LexicalSmoother<T>
{
public:

   /**
    * Constructor for use in the standard factory/iterator framework
    * @param factory
    * @param args
    */
   IBM1Smoother(PhraseSmootherFactory<T>& factory, const string& args);
   /// Constructor for use in the stand-alone case
   IBM1Smoother(IBM1* ibm_lang2_given_lang1, IBM1* ibm_lang1_given_lang2);
   /// Destructor.
   ~IBM1Smoother() {}

   virtual double probLang1GivenLang2(const vector<string>& l1_phrase, const vector<string>& l2_phrase);
   virtual double probLang2GivenLang1(const vector<string>& l1_phrase, const vector<string>& l2_phrase);
};

//-----------------------------------------------------------------------------
/**
 * IBM lexical smoothing. Like Zens-Ney, but use the standard IBM1/2/HMM
 * formula to calculate p(phrase1|phrase2).
 */
template<class T>
class IBMSmoother : public LexicalSmoother<T>
{
public:

   /**
    * Constructor for use in the standard factory/iterator framework
    * @param factory
    * @param args
    */
   IBMSmoother(PhraseSmootherFactory<T>& factory, const string& args);
   /// Constructor for use in the stand-alone case
   IBMSmoother(IBM1* ibm_lang2_given_lang1, IBM1* ibm_lang1_given_lang2);
   /// Destructor.
   ~IBMSmoother() {}

   virtual double probLang1GivenLang2(const vector<string>& l1_phrase, const vector<string>& l2_phrase);
   virtual double probLang2GivenLang1(const vector<string>& l1_phrase, const vector<string>& l2_phrase);
};

//-----------------------------------------------------------------------------
/**
 * For known conditioning phrases, a value of 1.0 if translation is also known,
 * else alpha [1/e]. For unknown conditioning phrases, a uniform distribution.
 */
template<class T>
class IndicatorSmoother : public RFSmoother<T>
{
public:

   /**
    * Constructor.
    * @param factory
    * @param args [alpha]
    */
   IndicatorSmoother(PhraseSmootherFactory<T>& factory, const string& args);

   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
   virtual bool usesCounts() {return true;}
   virtual bool isSymmetrical() {return false;}
};

} // Portage

#endif
