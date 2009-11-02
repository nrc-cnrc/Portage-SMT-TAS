/**
 * @author George Foster
 * @file phrase_smoother_cc.h  Implementation for phrase_smoother.h.
 * 
 * 
 * COMMENTS: 
 *
 * Implementation for phrase_smoother.h
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */
#ifndef PHRASE_SMOOTHER_CC_H
#define PHRASE_SMOOTHER_CC_H

#include <map>
#include <cmath>
#include "phrase_smoother.h"

namespace Portage {

/**
 * PhraseSmootherFactory
 */
template<class T>
PhraseSmootherFactory<T>::PhraseSmootherFactory(PhraseTableGen<T>* pt, 
                                                IBM1* ibm_lang2_given_lang1, IBM1* ibm_lang1_given_lang2,
                                                Uint verbose) :
   phrase_table(pt),
   ibm_lang2_given_lang1(ibm_lang2_given_lang1),
   ibm_lang1_given_lang2(ibm_lang1_given_lang2),
   verbose(verbose)
{}

template<class T>
PhraseSmoother<T>* PhraseSmootherFactory<T>::createSmoother(const string& tname, 
                                                            const string& args, bool fail)
{
   for (Uint i = 0; tinfos[i].pf; ++i)
      if (tname == tinfos[i].tname)
	 return tinfos[i].pf(*this, args);
   if (fail)
      error(ETFatal, "unknown smoother class name: " + tname);
   return NULL;
}

template<class T>
string PhraseSmootherFactory<T>::help() {
   string h;
   for (Uint i = 0; tinfos[i].pf; ++i) {
      h += tinfos[i].tname + " " + tinfos[i].help + "\n";
   }
   return h;
}

template<class T>
string PhraseSmootherFactory<T>::help(const string& tname) {
   for (Uint i = 0; tinfos[i].pf; ++i)
      if (tname == tinfos[i].tname)
	 return tinfos[i].help;
   return "";
}

template<class T>
bool PhraseSmootherFactory<T>::usesCounts(const string& tname_and_args) {
   vector<string> toks;
   toks.clear();
   split(tname_and_args, toks, " \n\t", 2);
   toks.resize(1);
   for (Uint i = 0; tinfos[i].pf; ++i)
      if (toks[0] == tinfos[i].tname)
	 return tinfos[i].uses_counts;
   error(ETWarn, "unknown smoother class name: " + toks[0]);
   return true;                 // safer assumption
}

template<class T>
typename PhraseSmootherFactory<T>::TInfo PhraseSmootherFactory<T>::tinfos[] = {
   {
      DCon< RFSmoother<T> >::create, 
      "RFSmoother", "[alpha] - relative-frequency estimates with add-alpha smoothing [0.0]",
      true,
   },{
      DCon< JointFreqs<T> >::create, 
      "JointFreqs", "- just write input joint frequencies",
      true,
   },{
      DCon< LeaveOneOut<T> >::create, 
      "LeaveOneOut", "s - simulated leave-one-out;\n                decr joint & marge freqs if s=1, drop singletons if s=2",
      true,
   },{
      DCon< GTSmoother<T> >::create, 
      "GTSmoother", "- Good-Turing smoothing on joint distn",
      true,
   },{
      DCon< KNSmoother<T> >::create, 
      "KNSmoother", "[numD][-u] - Kneser-Ney smoothing, using numD discount coeffs [1]",
      true,
   },{
      DCon< ZNSmoother<T> >::create, 
      "ZNSmoother", "- Zens-Ney noisy-or lexical smoothing, using IBM1 params",
      false,
   },{
      DCon< IBM1Smoother<T> >::create, 
      "IBM1Smoother", "- IBM1 lexical smoothing, using IBM1 parameters only",
      false,
   },{
      DCon< IBMSmoother<T> >::create, 
      "IBMSmoother", "- lexical smoothing, using full IBM2/HMM model provided\n              (or IBM1 if -ibm 1 is specified)",
      false,
   },{
      DCon< IndicatorSmoother<T> >::create, 
      "IndicatorSmoother", "[alpha] - 1.0 if in table, <alpha> otherwise",
      true,
   },{
      NULL, "", ""
   }
};

/*-----------------------------------------------------------------------------
 * RFSmoother
 */
template<class T>
RFSmoother<T>::RFSmoother(PhraseSmootherFactory<T>& factory, const string& args)
{
   alpha = args == "" ? 0.0 : conv<double>(args);

   PhraseTableGen<T>& pt = *factory.getPhraseTable();
   lang1_marginals.resize(pt.numLang1Phrases());
   lang2_marginals.resize(pt.numLang2Phrases());
   lang1_numtrans.resize(pt.numLang1Phrases());
   lang2_numtrans.resize(pt.numLang2Phrases());

   for (typename PhraseTableGen<T>::iterator it = pt.begin(); !it.equal(pt.end()); it.incr()) {
      lang1_marginals[it.getPhraseIndex(1)] += it.getJointFreq();
      lang2_marginals[it.getPhraseIndex(2)] += it.getJointFreq();
      ++lang1_numtrans[it.getPhraseIndex(1)];
      ++lang2_numtrans[it.getPhraseIndex(2)];
   }
}

template<class T>
double RFSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   double d = lang2_marginals[it.getPhraseIndex(2)];
   return d != 0.0 ? 
      (it.getJointFreq() + alpha) / (d + lang2_numtrans[it.getPhraseIndex(2)] * alpha) :
      1.0 / lang2_numtrans[it.getPhraseIndex(2)];
}

template<class T>
double RFSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   double d = lang1_marginals[it.getPhraseIndex(1)];
   return d != 0.0 ? 
      (it.getJointFreq() + alpha) / (d + lang1_numtrans[it.getPhraseIndex(1)] * alpha) : 
      1.0 / lang1_numtrans[it.getPhraseIndex(1)];
}

/*-----------------------------------------------------------------------------
 * IndicatorSmoother
 */
template<class T>
IndicatorSmoother<T>::IndicatorSmoother(PhraseSmootherFactory<T>& factory, const string& args) :
   RFSmoother<T>(factory,args)
{
   if (args == "")
      RFSmoother<T>::alpha = 1.0 / 2.718281828; // different default from RFSmoother
}

template<class T>
double IndicatorSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   double d = RFSmoother<T>::lang2_marginals[it.getPhraseIndex(2)];
   if (d) 
      return it.getJointFreq() ? 1.0 : RFSmoother<T>::alpha;
   else
      return 1.0 / RFSmoother<T>::lang2_numtrans[it.getPhraseIndex(2)];
}

template<class T>
double IndicatorSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   double d = RFSmoother<T>::lang1_marginals[it.getPhraseIndex(1)];
   if (d) 
      return it.getJointFreq() ? 1.0 : RFSmoother<T>::alpha;
   else
      return 1.0 / RFSmoother<T>::lang1_numtrans[it.getPhraseIndex(1)];
}

/*-----------------------------------------------------------------------------
 * LeaveOneOut
 */
template<class T>
LeaveOneOut<T>::LeaveOneOut(PhraseSmootherFactory<T>& factory, const string& args) :
   RFSmoother<T>(factory, args)
{
   if (args == "1")
      strategy = 1;
   else if (args == "2")
      strategy = 2;
   else {
      error(ETWarn, "LeaveOneOut smoothing - unknown arg, using strategy 1");
      strategy = 1;
   }
}

template<class T>
double LeaveOneOut<T>::estm(T jointfreq, T margefreq) {
   if (strategy == 1) {
      if (--jointfreq <= 0) return 0;
      --margefreq;
   } else if (strategy == 2) {
      if (jointfreq-1 <= 0) return 0;
   }
   return jointfreq / (double)margefreq;
}

template<class T>
double LeaveOneOut<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   return estm(it.getJointFreq(), RFSmoother<T>::lang2_marginals[it.getPhraseIndex(2)]);
}

template<class T>
double LeaveOneOut<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   return estm(it.getJointFreq(), RFSmoother<T>::lang1_marginals[it.getPhraseIndex(1)]);
}

/*-----------------------------------------------------------------------------
 * Good-turing.
 */
template<class T>
GTSmoother<T>::GTSmoother(PhraseSmootherFactory<T>& factory, const string& args)
{
   PhraseTableGen<T>& pt = *factory.getPhraseTable();
   lang1_marginals.resize(pt.numLang1Phrases());
   lang2_marginals.resize(pt.numLang2Phrases());
   lang1_numtrans.resize(pt.numLang1Phrases());
   lang2_numtrans.resize(pt.numLang2Phrases());
   lang1_num0trans.resize(pt.numLang1Phrases());
   lang2_num0trans.resize(pt.numLang2Phrases());

   // pass 1: count counts, sum original marginals, and count types

   map<Uint,Uint> count_map;	// freq -> num phrases with this freq
   for (typename PhraseTableGen<T>::iterator it = pt.begin(); !it.equal(pt.end()); it.incr()) {
      if (it.getJointFreq() != 0)
         ++count_map[(Uint)ceil((double)it.getJointFreq())];
      lang1_marginals[it.getPhraseIndex(1)] += it.getJointFreq();
      lang2_marginals[it.getPhraseIndex(2)] += it.getJointFreq();
      ++lang1_numtrans[it.getPhraseIndex(1)];
      ++lang2_numtrans[it.getPhraseIndex(2)];
      if (it.getJointFreq() == 0) ++lang1_num0trans[it.getPhraseIndex(1)]; // n_i(*,t,0)
      if (it.getJointFreq() == 0) ++lang2_num0trans[it.getPhraseIndex(2)]; // n_i(s,*,0)
   }

   // estimate GT smoother over joint freqs

   T total_freq = 0;
   vector<Uint> freqs(count_map.size()), freq_counts(count_map.size());
   Uint i = 0;
   for (map<Uint,Uint>::const_iterator p = count_map.begin(); p != count_map.end(); ++p, ++i) {
      freqs[i] = p->first;
      freq_counts[i] = p->second;
      total_freq += p->first * p->second;
   }
   gt = new GoodTuring(count_map.size(), &freqs[0], &freq_counts[0]);

   // Divide 0-freq mass among seen contexts in proportion to each context's
   // original non-zero mass; initialize each langX_marginals[i] with its share
   // of the 0-freq mass. Also save this initializing value in langX_num0trans,
   // for use in the final denominator for 0-freq translations of seen phrases.

   const double zero_mass = gt->zeroCountMass();
   for (Uint i = 0; i < pt.numLang1Phrases(); ++i) {
      lang1_marginals[i] = lang1_marginals[i] * zero_mass / total_freq;
      if (lang1_num0trans[i])   // in paper: z_i(t) / n_i(*,t,0)
         lang1_num0trans[i] = lang1_marginals[i] / lang1_num0trans[i];
   }
   for (Uint i = 0; i < pt.numLang2Phrases(); ++i) {
      lang2_marginals[i] = lang2_marginals[i] * zero_mass / total_freq;
      if (lang2_num0trans[i])   // in paper: z_i(t) / n_i(s,*,0)
         lang2_num0trans[i] = lang2_marginals[i] / lang2_num0trans[i];
   }

   // pass 2: finish summing smoothed marginals.

   for (typename PhraseTableGen<T>::iterator it = pt.begin(); !it.equal(pt.end()); it.incr()) {
      const double gtfreq = gt->smoothedFreq(it.getJointFreq());
      if (lang1_marginals[it.getPhraseIndex(1)]) lang1_marginals[it.getPhraseIndex(1)] += gtfreq;
      if (lang2_marginals[it.getPhraseIndex(2)]) lang2_marginals[it.getPhraseIndex(2)] += gtfreq;
   }

   // finalize denominators for  for 0-freq translations of seen phrases.

   for (Uint i = 0; i < pt.numLang1Phrases(); ++i)
      if (lang1_marginals[i]) lang1_num0trans[i] /= lang1_marginals[i];

   for (Uint i = 0; i < pt.numLang2Phrases(); ++i)
      if (lang2_marginals[i]) lang2_num0trans[i] /= lang2_marginals[i];
}

template<class T>
double GTSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   double d = lang2_marginals[it.getPhraseIndex(2)];
   if (d == 0.0) 
      return 1.0 / lang2_numtrans[it.getPhraseIndex(2)];
   
   double f = it.getJointFreq();
   return f ? 
      gt->smoothedFreq(it.getJointFreq()) / d :
      lang2_num0trans[it.getPhraseIndex(2)];
}

template<class T>
double GTSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   double d = lang1_marginals[it.getPhraseIndex(1)];
   if (d == 0.0) 
      return 1.0 / lang1_numtrans[it.getPhraseIndex(1)];
   
   double f = it.getJointFreq();
   return f ? 
      gt->smoothedFreq(it.getJointFreq()) / d :
      lang1_num0trans[it.getPhraseIndex(1)];
}

/*-----------------------------------------------------------------------------
 * Kneser-Ney. 
 */
template<class T>
KNSmoother<T>::KNSmoother(PhraseSmootherFactory<T>& factory, const string& args) :
   numD(1), unigram(false), verbose(factory.getVerbose())
{
   if (args != "") {
      vector<string> toks;
      split(args, toks);
      for (Uint i = 0; i < toks.size(); ++i)
         if (toks[i] == "-u")
            unigram = true;
         else
            numD = conv<Uint>(toks[i]);
   }

   // Set up structs

   PhraseTableGen<T>& pt = *factory.getPhraseTable();

   lang1_marginals.resize(pt.numLang1Phrases());
   lang2_marginals.resize(pt.numLang2Phrases());

   lang1_event_counts.resize(numD);
   lang2_event_counts.resize(numD);
   for (Uint i = 0; i < numD; ++i) {
      lang1_event_counts[i].resize(pt.numLang1Phrases());
      lang2_event_counts[i].resize(pt.numLang2Phrases());
   }

   global_event_counts.resize(numD+1);
   D.resize(numD);

   // Calculate marginals and event counts

   event_count_sum = 0;
   tot_freq = 0.0;
   for (typename PhraseTableGen<T>::iterator it = pt.begin(); !it.equal(pt.end()); it.incr()) {
      lang1_marginals[it.getPhraseIndex(1)] += it.getJointFreq();
      lang2_marginals[it.getPhraseIndex(2)] += it.getJointFreq();
      tot_freq += it.getJointFreq();
      Uint f = (Uint)ceil((double)it.getJointFreq());
      assert(f != 0);
      if (f <= numD+1) ++global_event_counts[f-1];
      if (f > numD) f = numD;
      ++lang1_event_counts[f-1][it.getPhraseIndex(1)];
      ++lang2_event_counts[f-1][it.getPhraseIndex(2)];
      ++event_count_sum;
   }

   // Calculate discounting coeffs

   if (verbose) {
      if (unigram) cerr << "Using unigram lower-order distribution" << endl;
      cerr << "Discounting coeffs: " << endl;
   }
   const double Y = global_event_counts[0] / (global_event_counts[0] + 2.0 * global_event_counts[1]);
   for (Uint c = 1; c <= numD; ++c) {
      D[c-1] = c - (c+1) * Y * global_event_counts[c] / global_event_counts[c-1];
      if (verbose)
         cerr << "D" << c << " = " << D[c-1] << endl;
   }
}

template<class T>
double KNSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   Uint f = (Uint)ceil((double)it.getJointFreq());
   assert(f != 0);
   if (f > numD) f = numD;
   double disc = D[f-1];
   
   double gamma = 0.0;
   for (Uint i = 0; i < numD; ++i)
      gamma += D[i] * lang2_event_counts[i][it.getPhraseIndex(2)];

   double lower_order_distn = 0.0;
   if (unigram) {
      lower_order_distn = lang1_marginals[it.getPhraseIndex(1)] / tot_freq;
   } else {
      for (Uint i = 0; i < numD; ++i)
         lower_order_distn += lang1_event_counts[i][it.getPhraseIndex(1)];
      lower_order_distn /= event_count_sum;
   }

   if (verbose > 1)
      dumpProbInfo("L1|L2", it.getJointFreq(), disc, gamma, lower_order_distn, 
                   lang2_marginals[it.getPhraseIndex(2)]);

   return (it.getJointFreq() - disc + gamma * lower_order_distn) / 
      lang2_marginals[it.getPhraseIndex(2)];
}

// copy of above, just swap 1 <-> 2 everwhere

template<class T>
double KNSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   Uint f = (Uint)ceil((double)it.getJointFreq());
   assert(f != 0);
   if (f > numD) f = numD;
   double disc = D[f-1];
   
   double gamma = 0.0;
   for (Uint i = 0; i < numD; ++i)
      gamma += D[i] * lang1_event_counts[i][it.getPhraseIndex(1)];

   double lower_order_distn = 0.0;
   if (unigram) {
      lower_order_distn = lang2_marginals[it.getPhraseIndex(2)] / tot_freq;
   } else {
      for (Uint i = 0; i < numD; ++i)
         lower_order_distn += lang2_event_counts[i][it.getPhraseIndex(2)];
      lower_order_distn /= event_count_sum;
   }

   if (verbose > 1)
      dumpProbInfo("L2|l1", it.getJointFreq(), disc, gamma, lower_order_distn, 
                   lang1_marginals[it.getPhraseIndex(1)]);

   return (it.getJointFreq() - disc + gamma * lower_order_distn) / 
      lang1_marginals[it.getPhraseIndex(1)];
}

/*-----------------------------------------------------------------------------
 * Zens-Ney using IBM1 parameters. 
 */
template<class T>
ZNSmoother<T>::ZNSmoother(PhraseSmootherFactory<T>& factory, const string& args)
{
   ibm_lang2_given_lang1 = factory.getIBMLang2GivenLang1();
   ibm_lang1_given_lang2 = factory.getIBMLang1GivenLang2();
   if (!(ibm_lang2_given_lang1 && ibm_lang1_given_lang2))
     error(ETFatal, "Can't create Zens-Ney smoother without IBM models");
}

template<class T>
double ZNSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   it.getPhrase(1, l1_phrase);
   it.getPhrase(2, l2_phrase);

   double pr_global = 1.0;
   for (Uint j = 0; j < l1_phrase.size(); ++j) {

      // get p(l1_pharase[j]|x) for all x in l2_phrase
      ibm_lang1_given_lang2->IBM1::pr(l2_phrase, l1_phrase[j], &phrase_probs);

      double pr_local = 1.0;
      for (Uint i = 0; i < phrase_probs.size(); ++i)
         pr_local *= (1.0 - phrase_probs[i]);

      pr_global *= (1.0 - pr_local);
   }

   return pr_global == 0.0 ? PhraseSmoother<T>::VERY_SMALL_PROB : pr_global;
}

template<class T>
double ZNSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   it.getPhrase(1, l1_phrase);
   it.getPhrase(2, l2_phrase);

   double pr_global = 1.0;
   for (Uint j = 0; j < l2_phrase.size(); ++j) {

      // get p(l2_pharase[j]|x) for all x in l1_phrase
      ibm_lang2_given_lang1->IBM1::pr(l1_phrase, l2_phrase[j], &phrase_probs);

      double pr_local = 1.0;
      for (Uint i = 0; i < phrase_probs.size(); ++i)
         pr_local *= (1.0 - phrase_probs[i]);

      pr_global *= (1.0 - pr_local);
   }

   return pr_global == 0.0 ? PhraseSmoother<T>::VERY_SMALL_PROB : pr_global;
}

/*-----------------------------------------------------------------------------
 * IBM1Smoother. 
 */
template<class T>
IBM1Smoother<T>::IBM1Smoother(PhraseSmootherFactory<T>& factory, const string& args)
{
   ibm_lang2_given_lang1 = factory.getIBMLang2GivenLang1();
   ibm_lang1_given_lang2 = factory.getIBMLang1GivenLang2();
   if (!(ibm_lang2_given_lang1 && ibm_lang1_given_lang2))
     error(ETFatal, "Can't create IBM1 smoother without IBM models");
}

template<class T>
double IBM1Smoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   it.getPhrase(1, l1_phrase);
   it.getPhrase(2, l2_phrase);

   return exp(ibm_lang1_given_lang2->IBM1::logpr(l2_phrase, l1_phrase, 1e-07));
}

template<class T>
double IBM1Smoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   it.getPhrase(1, l1_phrase);
   it.getPhrase(2, l2_phrase);

   return exp(ibm_lang2_given_lang1->IBM1::logpr(l1_phrase, l2_phrase, 1e-07));
}


/*-----------------------------------------------------------------------------
 * IBMSmoother. 
 */
template<class T>
IBMSmoother<T>::IBMSmoother(PhraseSmootherFactory<T>& factory, const string& args)
{
   ibm_lang2_given_lang1 = factory.getIBMLang2GivenLang1();
   ibm_lang1_given_lang2 = factory.getIBMLang1GivenLang2();
   if (!(ibm_lang2_given_lang1 && ibm_lang1_given_lang2))
     error(ETFatal, "Can't create IBM smoother without IBM1/2/HMM models");
}

template<class T>
double IBMSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   it.getPhrase(1, l1_phrase);
   it.getPhrase(2, l2_phrase);

   return exp(ibm_lang1_given_lang2->logpr(l2_phrase, l1_phrase, 1e-10));
}

template<class T>
double IBMSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   it.getPhrase(1, l1_phrase);
   it.getPhrase(2, l2_phrase);

   return exp(ibm_lang2_given_lang1->logpr(l1_phrase, l2_phrase, 1e-10));
}


} // Portage
#endif
