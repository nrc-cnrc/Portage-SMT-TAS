/**
 * @author George Foster
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
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */
#ifndef PHRASE_SMOOTHER_H
#define PHRASE_SMOOTHER_H

#include <good_turing.h>
#include <cfloat>
#include "phrase_table.h"

namespace Portage {

//-------------------------------------------------------------------------------------
/**
 * Abstract smoother. T is the type used to represent phrase-table frequencies.
 */
template<class T>
class PhraseSmoother 
{
public:

   // probabilities are represented as floats in canoe, so don't go below FLT_MIN!
   static const double VERY_SMALL_PROB; // = FLT_MIN;


   /// Destructor.
   virtual ~PhraseSmoother() {}

   /**
    * Given a phrase pair (phrase1, phrase2), return smoothed estimates for
    * P(phrase1|phrase2).
    * @param it  
    * @return Returns  P(phrase1|phrase2)
    */
   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it) = 0;
   /**
    * Given a phrase pair (phrase1, phrase2), return smoothed estimates for
    * P(phrase2|phrase1).
    * @param it  
    * @return Returns  P(phrase2|phrase1)
    */
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it) = 0;
};


//-------------------------------------------------------------------------------------
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
   IBM1* ibm_lang2_given_lang1;       ///< IBM model
   IBM1* ibm_lang1_given_lang2;       ///< IBM model

   Uint verbose;

   /// Generic object allocator for PhraseSmoothers<T>.
   template<class S> struct DCon {
      ///  Allocates a new object of type T constructed with S(factory, s). 
      static PhraseSmoother<T>* create(PhraseSmootherFactory<T>& factory, const string& s)
      {
	 return new S(factory, s);
      }
   };

   /// PhraseSmoother<T> creation function definition.
   typedef PhraseSmoother<T>* (*PF)(PhraseSmootherFactory<T>& factory, const string& s);

   /// Creational information for PhraseSmoother<T>
   struct TInfo {
      PF pf;			///< pointer to create() function
      string tname;		///< name of derived class
      string help;		///< describes args for derived class constructor
   };
   static TInfo tinfos[];	///< array containing all known smoothers


public:

   /**
    * Construct with phrase table and IBM models in both directions.
    * @param pt
    * @param ibm_lang2_given_lang1 May point to IBM1 or IBM2, or be NULL
    * @param ibm_lang1_given_lang2 ""
    * @param verbose level: 0 for no messages, 1 for basic, 2 for detail
    */
   PhraseSmootherFactory(PhraseTableGen<T>* pt, 
                         IBM1* ibm_lang2_given_lang1, IBM1* ibm_lang1_given_lang2,
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
    * Info functions.  Self explanatory named functions.
    */
   //@{
   PhraseTableGen<T>* getPhraseTable() {return phrase_table;}
   IBM1* getIBMLang1GivenLang2() {return ibm_lang1_given_lang2;}
   IBM1* getIBMLang2GivenLang1() {return ibm_lang2_given_lang1;}
   Uint getVerbose() {return verbose;}
   //@}
 };



//-------------------------------------------------------------------------------------
/**
 * Plain relative-frequency estimates - no smoothing.
 */
template<class T>
class RFSmoother : public PhraseSmoother<T>
{
protected:

   vector<T> lang1_marginals;
   vector<T> lang2_marginals;

public:

   /**
    * Constructor.
    * @param factory
    * @param args
    */
   RFSmoother(PhraseSmootherFactory<T>& factory, const string& args);

   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
};


//-------------------------------------------------------------------------------------
/**
 * Simulated leave-one-out.
 */
template<class T>
class LeaveOneOut : public RFSmoother<T>
{
   Uint strategy;

   double estm(T jointfreq, T margefreq);

public:

   /**
    * Constructor.
    * @param factory
    * @param args
    */
   LeaveOneOut(PhraseSmootherFactory<T>& factory, const string& args);

   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
};


//-------------------------------------------------------------------------------------
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

   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
};


//-------------------------------------------------------------------------------------
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
   
   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
};


//-------------------------------------------------------------------------------------
/**
 * Zens-Ney lexical smoothing. This differs from the above approaches in that
 * it ignores joint counts and derives conditional distributions strictly from
 * IBM1 probabilities.
 */
template<class T>
class ZNSmoother : public PhraseSmoother<T>
{
   IBM1* ibm_lang2_given_lang1;
   IBM1* ibm_lang1_given_lang2;

   vector<string> l1_phrase;
   vector<string> l2_phrase;

   vector<double> phrase_probs;

public:
   
   /**
    * Constructor.
    * @param factory
    * @param args
    */
   ZNSmoother(PhraseSmootherFactory<T>& factory, const string& args);
   /// Destructor.
   ~ZNSmoother() {}
   
   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
};


//-------------------------------------------------------------------------------------
/**
 * IBM lexical smoothing. Like Zens-Ney, but use the standard IBM1 formula to
 * calculate p(phrase1|phrase2).
 */
template<class T>
class IBMSmoother : public PhraseSmoother<T>
{
   IBM1* ibm_lang2_given_lang1;
   IBM1* ibm_lang1_given_lang2;

   vector<string> l1_phrase;
   vector<string> l2_phrase;

public:
   
   /**
    * Constructor.
    * @param factory
    * @param args
    */
   IBMSmoother(PhraseSmootherFactory<T>& factory, const string& args);
   /// Destructor.
   ~IBMSmoother() {}
   
   virtual double probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it);
   virtual double probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it);
};


}

#endif
