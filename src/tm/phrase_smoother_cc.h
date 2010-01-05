/**
 * @author George Foster, with reduced memory usage mods by Darlene Stewart
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
                                                IBM1* ibm_lang2_given_lang1,
                                                IBM1* ibm_lang1_given_lang2,
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

/**
 * Create one smoother and tally its marginals.
 * @param smoothing_method name of smoothing method plus arguments
 * @returns created smoother used to generate conditional probabilities
 */
template<class T>
PhraseSmoother<T>* PhraseSmootherFactory<T>::createSmootherAndTally(const string& tname_and_args,
                                                                    bool fail/*=true*/)
{
   PhraseSmoother<T>* smoother = createSmoother(tname_and_args, fail);
   if (! smoother) return smoother;

   // Tally the marginals for the smoother, making possibly multiple passes
   // over the phrase table.
   for (Uint i = 0; i < smoother->numTallyMarginalsPasses(); ++i) {
      for (typename PhraseTableGen<T>::iterator it = phrase_table->begin();
            it != phrase_table->end(); ++it)
         smoother->tallyMarginals(it);
      smoother->tallyMarginalsFinishPass();
   }

   if (verbose) {
      Uint passes = smoother->numTallyMarginalsPasses();
      cerr << "created '" << tname_and_args << "' smoother using "
           << passes << " phrase table pass" << (passes == 1 ? "." : "es.") << endl;
   }

   return smoother;
}

template<class T>
void PhraseSmootherFactory<T>::createSmoothersAndTally(vector< PhraseSmoother<T>* >& smoothers,
                                                       vector<string> tnames_and_args,
                                                       bool fail/*=true*/)
{
   smoothers.resize(tnames_and_args.size());
   for (Uint i = 0; i < tnames_and_args.size(); ++i)
      smoothers[i] = createSmoother(tnames_and_args[i], fail);

   vector< vector < PhraseSmoother<T>* > > smoothers_per_pass;
   for (Uint i = 0; i < smoothers.size(); ++i) {
      if (!smoothers[i]) continue;
      Uint passes = smoothers[i]->numTallyMarginalsPasses();
      if (passes > smoothers_per_pass.size())
         smoothers_per_pass.resize(passes);
      for(Uint j = 0; j < passes; ++j)
         smoothers_per_pass[j].push_back(smoothers[i]);
   }

   // Tally the marginals for each smoother, making possibly multiple passes
   // over the phrase table.
   for (Uint i = 0; i < smoothers_per_pass.size(); ++i) {
      for (typename PhraseTableGen<T>::iterator it = phrase_table->begin();
            it != phrase_table->end(); ++it)
         for (Uint j = 0; j < smoothers_per_pass[i].size(); ++j)
            smoothers_per_pass[i][j]->tallyMarginals(it);
      for (Uint j = 0; j < smoothers_per_pass[i].size(); ++j)
         smoothers_per_pass[i][j]->tallyMarginalsFinishPass();
   }

   if (verbose) {
      Uint size = smoothers.size();
      cerr << "created " << size << " smoother:"  << (size == 1 ? ":" : "s:") << endl;
      for (Uint i = 0; i < smoothers.size(); ++i) {
         Uint passes = smoothers[i]->numTallyMarginalsPasses();
         cerr << "  created '" << tnames_and_args[i] << "' smoother using "
              << passes << " phrase table pass" << (passes == 1 ? "." : "es.") << endl;
      }
   }
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
      DCon< MarginalFreqs<T> >::create,
      "MarginalFreqs", "1|2 - write l1 (left col of input jpt) or l2 marginal frequencies",
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
RFSmoother<T>::RFSmoother(PhraseSmootherFactory<T>& factory, const string& args) :
   PhraseSmoother<T>(1)
{
   alpha = args == "" ? 0.0 : conv<double>(args);

   PhraseTableGen<T>& pt = *factory.getPhraseTable();
   lang1_marginals.resize(pt.numLang1Phrases());
   lang2_marginals.resize(pt.numLang2Phrases());
   lang1_numtrans.resize(pt.numLang1Phrases());
   lang2_numtrans.resize(pt.numLang2Phrases());
}

template<class T>
void RFSmoother<T>::tallyMarginals(const typename PhraseTableGen<T>::iterator& it)
{
   if (PhraseSmoother<T>::currentTallyMarginalsPass() == 1) {
      Uint id1 = it.getPhraseIndex(1);
      Uint id2 = it.getPhraseIndex(2);
      // Grow the vectors if necessary. Note: pruning can change phrase order.
      if (id1 == lang1_marginals.size()) {
         lang1_marginals.push_back(T());
         lang1_numtrans.push_back(0);
      }
      if (id2 >= lang2_marginals.size()) {
         Uint new_size = (id2+1) * 1.5;
         lang2_marginals.resize(new_size);
         lang2_numtrans.resize(new_size);
      }
      // Tally marginals.
      lang1_marginals[id1] += it.getJointFreq();
      lang2_marginals[id2] += it.getJointFreq();
      ++lang1_numtrans[id1];
      ++lang2_numtrans[id2];
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
 * MarginalFreqs
 */
template<class T>
MarginalFreqs<T>::MarginalFreqs(PhraseSmootherFactory<T>& factory, const string& args) :
   PhraseSmoother<T>(1)
{
   lang = conv<Uint>(args);
   if (lang != 1 && lang != 2)
      error(ETFatal, "argument to MarginalFreqs must be 1 or 2");

   PhraseTableGen<T>& pt = *factory.getPhraseTable();
   marginals.resize(lang == 1 ? pt.numLang1Phrases() : pt.numLang2Phrases());
}

template<class T>
void MarginalFreqs<T>::tallyMarginals(const typename PhraseTableGen<T>::iterator& it)
{
   if (PhraseSmoother<T>::currentTallyMarginalsPass() == 1) {
      Uint id = it.getPhraseIndex(lang);
      // Grow the vectors if necessary. Note: pruning can change phrase order.
      if (id >= marginals.size())
         marginals.resize((Uint)((id+1) * 1.5));
      // Tally marginals.
      marginals[id] += it.getJointFreq();
   }
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
GTSmoother<T>::GTSmoother(PhraseSmootherFactory<T>& factory, const string& args) :
   PhraseSmoother<T>(2)
{
   PhraseTableGen<T>& pt = *factory.getPhraseTable();
   lang1_marginals.resize(pt.numLang1Phrases());
   lang2_marginals.resize(pt.numLang2Phrases());
   lang1_numtrans.resize(pt.numLang1Phrases());
   lang2_numtrans.resize(pt.numLang2Phrases());
   lang1_num0trans.resize(pt.numLang1Phrases());
   lang2_num0trans.resize(pt.numLang2Phrases());
}

template<class T>
void GTSmoother<T>::tallyMarginals(const typename PhraseTableGen<T>::iterator& it)
{
   if (PhraseSmoother<T>::currentTallyMarginalsPass() == 1) {
      Uint id1 = it.getPhraseIndex(1);
      Uint id2 = it.getPhraseIndex(2);
      // Grow the vectors if necessary. Note: pruning can change phrase order.
      if (id1 == lang1_marginals.size()) {
         lang1_marginals.push_back(T());
         lang1_numtrans.push_back(0);
         lang1_num0trans.push_back(0);
      }
      if (id2 >= lang2_marginals.size()) {
         Uint new_size = (id2+1) * 1.5;
         lang2_marginals.resize(new_size);
         lang2_numtrans.resize(new_size);
         lang2_num0trans.resize(new_size);
      }
      // pass 1: count counts, sum original marginals, and count types.
      if (it.getJointFreq() != 0)
	 ++count_map[(Uint)ceil((double)it.getJointFreq())];
      lang1_marginals[id1] += it.getJointFreq();
      lang2_marginals[id2] += it.getJointFreq();
      ++lang1_numtrans[id1];
      ++lang2_numtrans[id2];
      if (it.getJointFreq() == 0)
	 ++lang1_num0trans[id1]; // n_i(*,t,0)
      if (it.getJointFreq() == 0)
	 ++lang2_num0trans[id2]; // n_i(s,*,0)
   }

   else if (PhraseSmoother<T>::currentTallyMarginalsPass() == 2) {
      // pass 2: finish summing smoothed marginals.
      const double gtfreq = gt->smoothedFreq(it.getJointFreq());
      if (lang1_marginals[it.getPhraseIndex(1)])
         lang1_marginals[it.getPhraseIndex(1)] += gtfreq;
      if (lang2_marginals[it.getPhraseIndex(2)])
         lang2_marginals[it.getPhraseIndex(2)] += gtfreq;
   }
}

template<class T>
void GTSmoother<T>::tallyMarginalsFinishPass()
{
   if (PhraseSmoother<T>::currentTallyMarginalsPass() == 1) {
      // finished pass 1 of tallyMarginals
      // Estimate GT smoother over joint freqs
      T total_freq = 0;
      vector<Uint> freqs(count_map.size()), freq_counts(count_map.size());
      Uint i = 0;
      for (map<Uint,Uint>::const_iterator p = count_map.begin();
           p != count_map.end(); ++p, ++i) {
         freqs[i] = p->first;
         freq_counts[i] = p->second;
         total_freq += p->first * p->second;
      }
      gt = new GoodTuring(count_map.size(), &freqs[0], &freq_counts[0]);
      count_map.clear();

      // Divide 0-freq mass among seen contexts in proportion to each context's
      // original non-zero mass; initialize each langX_marginals[i] with its share
      // of the 0-freq mass. Also save this initializing value in langX_num0trans,
      // for use in the final denominator for 0-freq translations of seen phrases.
      const double zero_mass = gt->zeroCountMass();
      for (Uint i = 0; i < lang1_marginals.size(); ++i) {
         lang1_marginals[i] = lang1_marginals[i] * zero_mass / total_freq;
         if (lang1_num0trans[i])   // in paper: z_i(t) / n_i(*,t,0)
            lang1_num0trans[i] = lang1_marginals[i] / lang1_num0trans[i];
      }
      for (Uint i = 0; i < lang2_marginals.size(); ++i) {
         lang2_marginals[i] = lang2_marginals[i] * zero_mass / total_freq;
         if (lang2_num0trans[i])   // in paper: z_i(t) / n_i(s,*,0)
            lang2_num0trans[i] = lang2_marginals[i] / lang2_num0trans[i];
      }
   }

   else if (PhraseSmoother<T>::currentTallyMarginalsPass() == 2) {
      // finished pass 2 of tallyMarginals
      // Finalize denominators for  for 0-freq translations of seen phrases.
      for (Uint i = 0; i < lang1_marginals.size(); ++i)
         if (lang1_marginals[i]) lang1_num0trans[i] /= lang1_marginals[i];
      for (Uint i = 0; i < lang2_marginals.size(); ++i)
         if (lang2_marginals[i]) lang2_num0trans[i] /= lang2_marginals[i];
   }

   PhraseSmoother<T>::tallyMarginalsFinishPass();
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
   PhraseSmoother<T>(1), numD(1), unigram(false), verbose(factory.getVerbose())
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

   event_count_sum = 0;
   tot_freq = 0.0;
}

template<class T>
void KNSmoother<T>::tallyMarginals(const typename PhraseTableGen<T>::iterator& it)
{
   if (PhraseSmoother<T>::currentTallyMarginalsPass() == 1) {
      // Grow the vectors if necessary. Note: pruning can change phrase order.
      Uint id1 = it.getPhraseIndex(1);
      if (id1 == lang1_marginals.size()) {
         lang1_marginals.push_back(0);
         for (Uint i = 0; i < numD; ++i)
            lang1_event_counts[i].push_back(0);
      }
      Uint id2 = it.getPhraseIndex(2);
      if (id2 >= lang2_marginals.size()) {
         Uint new_size = (id2+1) * 1.5;
         lang2_marginals.resize(new_size);
         for (Uint i = 0; i < numD; ++i)
            lang2_event_counts[i].resize(new_size);
      }
      // Calculate marginals and event counts
      lang1_marginals[it.getPhraseIndex(1)] += it.getJointFreq();
      lang2_marginals[it.getPhraseIndex(2)] += it.getJointFreq();
      tot_freq += it.getJointFreq();
      Uint f = (Uint)ceil((double)it.getJointFreq());
      if(f == 0) {
         Uint index1 = it.getPhraseIndex(1);
         Uint index2 = it.getPhraseIndex(2);
         string phrase1, phrase2;
         it.getPhrase(1, phrase1);
         it.getPhrase(2, phrase2);
         Uint count = it.getJointFreq();
         cerr << "KN smoother: " << index1 << " '" << phrase1 << "' "
              << index2 << " '" << phrase2 << "' " << count << " " << endl;
      }
      assert(f != 0);
      if (f <= numD+1) ++global_event_counts[f-1];
      if (f > numD) f = numD;
      ++lang1_event_counts[f-1][it.getPhraseIndex(1)];
      ++lang2_event_counts[f-1][it.getPhraseIndex(2)];
      ++event_count_sum;
   }
}

template<class T>
void KNSmoother<T>::tallyMarginalsFinishPass()
{
   if (PhraseSmoother<T>::currentTallyMarginalsPass() == 1) {
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

   PhraseSmoother<T>::tallyMarginalsFinishPass();
}

template<class T>
double KNSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   Uint f = (Uint)ceil((double)it.getJointFreq());
   if (f == 0) {
      Uint index1 = it.getPhraseIndex(1);
      Uint index2 = it.getPhraseIndex(2);
      string phrase1, phrase2;
      it.getPhrase(1, phrase1);
      it.getPhrase(2, phrase2);
      Uint count = it.getJointFreq();
      cout << "KNSmoother<T>::probLang1GivenLang2: " << index1 << " " << phrase1 << " "
           << index2 << " " << phrase2 << " " << count << endl;
   }
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
ZNSmoother<T>::ZNSmoother(PhraseSmootherFactory<T>& factory, const string& args) :
   PhraseSmoother<T>(0)
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
IBM1Smoother<T>::IBM1Smoother(PhraseSmootherFactory<T>& factory, const string& args) :
   PhraseSmoother<T>(0)
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
IBMSmoother<T>::IBMSmoother(PhraseSmootherFactory<T>& factory, const string& args):
   PhraseSmoother<T>(0)
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
