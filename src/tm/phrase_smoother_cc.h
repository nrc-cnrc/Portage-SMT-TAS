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
#include <stdio.h>
#include <sstream>
#include "phrase_smoother.h"

namespace Portage {

/**
 * PhraseSmootherFactory
 */
template<class T>
PhraseSmootherFactory<T>::PhraseSmootherFactory(PhraseTableGen<T>* pt,
                                                IBM1* p_ibm_lang2_given_lang1,
                                                IBM1* p_ibm_lang1_given_lang2,
                                                Uint verbose) :
   phrase_table(pt), verbose(verbose)
{
   if (p_ibm_lang2_given_lang1)
      ibm_lang2_given_lang1.push_back(p_ibm_lang2_given_lang1);

   if (p_ibm_lang1_given_lang2)
      ibm_lang1_given_lang2.push_back(p_ibm_lang1_given_lang2);

   assert(ibm_lang1_given_lang2.size() == ibm_lang2_given_lang1.size());
}

template<class T>
PhraseSmootherFactory<T>::PhraseSmootherFactory(PhraseTableGen<T>* pt,
                      vector<IBM1*> ibm_lang2_given_lang1,
                      vector<IBM1*> ibm_lang1_given_lang2,
                      Uint verbose) :
   phrase_table(pt), verbose(verbose),
   ibm_lang2_given_lang1(ibm_lang2_given_lang1),
   ibm_lang1_given_lang2(ibm_lang1_given_lang2)
{
   assert(ibm_lang1_given_lang2.size() == ibm_lang2_given_lang1.size());
}

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
      Uint counter = 0;
      for (typename PhraseTableGen<T>::iterator it = phrase_table->begin();
            it != phrase_table->end(); ++it) {
         if ( verbose && (++counter % 100000 == 0) ) cerr << ".";
         for (Uint j = 0; j < smoothers_per_pass[i].size(); ++j)
            smoothers_per_pass[i][j]->tallyMarginals(it);
      }
      for (Uint j = 0; j < smoothers_per_pass[i].size(); ++j)
         smoothers_per_pass[i][j]->tallyMarginalsFinishPass();
      if (verbose) cerr << "Done PT smoother pass " << i << endl;
   }

   if (verbose) {
      Uint size = smoothers.size();
      cerr << "created " << size << " smoother"  << (size == 1 ? ":" : "s:") << endl;
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
bool PhraseSmootherFactory<T>::isSymmetrical(const string& tname_and_args) {
   vector<string> toks;
   toks.clear();
   split(tname_and_args, toks, " \n\t", 2);
   toks.resize(1);
   for (Uint i = 0; tinfos[i].pf; ++i)
      if (toks[0] == tinfos[i].tname)
	 return tinfos[i].is_symmetrical;
   error(ETWarn, "unknown smoother class name: " + toks[0]);
   return false;                 // safer assumption
}

template<class T>
typename PhraseSmootherFactory<T>::TInfo PhraseSmootherFactory<T>::tinfos[] = {
   {
      DCon< RFSmoother<T> >::create,
      "RFSmoother", "[alpha] - relative-frequency estimates with add-alpha smoothing [0.0]",
      true, false,
   },{
      DCon< MAPSmoother<T> >::create,
      "MAPSmoother", "[alpha] - MAP estimates with alpha-weighted *external* prior [0.0]",
      true, false,
   },{
      DCon< JointFreqs<T> >::create,
      "JointFreqs", "[d] - write input joint frequencies, possibly discounted by d < 1 [0]",
      true, true,
   },{
      DCon< NGCounts<T> >::create,
      "NGCounts", "lang,file - for lang (1 or 2) phrases, counts from ngram file",
      false, true,
   },{
      DCon< PureIndicator<T> >::create,
      "PureIndicator", "- 1.0 if joint freq > 0, 1/e else",
      true, true,
   },{
      DCon< MarginalFreqs<T> >::create,
      "MarginalFreqs", "1|2 - write l1 (left col of input jpt) or l2 marginal frequencies",
      true, true,
   },{
      DCon< Sig2Probs<T> >::create,
      "Sig2Probs", "- convert significance scores to probs, using 1 - exp(score)",
      true, true,
   },{
      DCon< LeaveOneOutSmoother<T> >::create,
      "LeaveOneOut", "s - simulated leave-one-out;\n                decr joint & marge freqs if s=1, drop singletons if s=2",
      true, false,
   },{
      DCon< AbsoluteDiscountSmoother<T> >::create,
      "AbsoluteDiscount", "[d][-m1 ng1][-m2 ng2] - Absolute discounting of joint counts\n\
         by d [auto]. If ng1/ng2 are supplied, they must be ngram count files in SRILM\n\
         format to be used for marginals instead of summed joint counts.",
      true, false,
   },{
      DCon< GTSmoother<T> >::create,
      "GTSmoother", "- Good-Turing smoothing on joint distn",
      true, false,
   },{
      DCon< KNSmoother<T> >::create,
      "KNSmoother", "[numD][-d coeffs][-u] - Kneser-Ney smoothing, using numD discount\n\
           coeffs; these come from comma-separated values in <coeffs> if -d given,\n\
           otherwise they are determined automatically. Use unigram backoff distn if -u\n\
           given, otherwise std KN backoff [1 coeff, set automatically, KN backoff]\n\
           Internal stats: knstats=<joint frequency of (s,t), frequency of s, frequency of t,\n\
           number of translation candidates for s, number of translation candidates for t>",
      true, false,
   },{
      DCon< KNXSmoother<T> >::create,
      "KNXSmoother", "[opts] - Kneser-Ney smoothing, using *external* backoff distn;\n\
           options are the same as for KNSmoother, except that -u is ignored.",
      true, false,
   },{
      DCon< KNXZSmoother<T> >::create,
      "KNXZSmoother", "[opts] - Kneser-Ney smoothing, using *external* backoff distn\n\
           for zero counts only; options are the same as for KNSmoother.",
      true, false,
   },{
      DCon< EventCounts<T> >::create,
      "EventCounts", "thresh - For each phrase, the number of different translations\n\
       with frequency >= thresh",
       true, false,
   },{
      DCon< ZNSmoother<T> >::create,
      "ZNSmoother", "[n] Zens-Ney lexical smoothing, using ttable of the nth model listed [1]",
      false, false,
   },{
      DCon< IBM1Smoother<T> >::create,
      "IBM1Smoother", "[n] IBM1 lexical smoothing, using ttable of the nth model listed [1]",
      false, false,
   },{
      DCon< IBMSmoother<T> >::create,
      "IBMSmoother", "[n] lexical smoothing, using nth full IBM/HMM model [1]",
      false, false,
   },{
      DCon< IndicatorSmoother<T> >::create,
      "IndicatorSmoother", "[alpha] - 1.0 if in table, uniform if src in table, <alpha> else, ",
      true, false,
   },{
      DCon< CoarseModel<T> >::create,
      "CoarseModel", "cm-cpt l1-classes l2-classes - use fwd and bwd probs from cm-cpt\n\
         with word-to-class mapping from l1-classes and l2-classes.",
      false, false,
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
      const Uint id1 = it.getPhraseIndex(1);
      const Uint id2 = it.getPhraseIndex(2);
      // Grow the vectors if necessary. Note: pruning can change phrase order.
      if (id1 == lang1_marginals.size()) {
         lang1_marginals.push_back(T());
         lang1_numtrans.push_back(0);
      }
      if (id2 >= lang2_marginals.size()) {
         const Uint new_size = (id2+1) * 1.5;
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
   const double d = lang2_marginals[it.getPhraseIndex(2)];
   return d != 0.0 ?
      (it.getJointFreq() + alpha) / (d + lang2_numtrans[it.getPhraseIndex(2)] * alpha) :
      1.0 / lang2_numtrans[it.getPhraseIndex(2)];
}

template<class T>
double RFSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   const double d = lang1_marginals[it.getPhraseIndex(1)];
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
 * NGCounts
 */
template<class T>
NGCounts<T>::NGCounts(PhraseSmootherFactory<T>& factory, const string& args) :
   PhraseSmoother<T>(0)
{
   vector<string> toks;
   if (split(args, toks, " ,") != 2)
      error(ETFatal, "NGCounts: expecting lang,file argument");
   lang = conv<Uint>(toks[0]);
   if (lang != 1 && lang != 2)
      error(ETFatal, "NGCounts: lang must be 1 or 2");
   ng = new NgramCounts(&voc, toks[1]);
}

/*-----------------------------------------------------------------------------
 * MAPSmoother
 */

template<class T>
double MAPSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it,
                                           double pr)
{
   double d = this->lang2_marginals[it.getPhraseIndex(2)];
   return (it.getJointFreq() + this->alpha * pr) / (d + this->alpha);
}

template<class T>
double MAPSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it,
                                           double pr)
{
   double d = this->lang1_marginals[it.getPhraseIndex(1)];
   return (it.getJointFreq() + this->alpha * pr) / (d + this->alpha);
}

/*-----------------------------------------------------------------------------
 * IndicatorSmoother
 */
template<class T>
IndicatorSmoother<T>::IndicatorSmoother(PhraseSmootherFactory<T>& factory, const string& args) :
   RFSmoother<T>(factory,args)
{
   if (args == "")
      this->alpha = 1.0 / 2.718281828; // different default from RFSmoother
}

template<class T>
double IndicatorSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   double d = this->lang2_marginals[it.getPhraseIndex(2)];
   if (d)
      return it.getJointFreq() ? 1.0 : this->alpha;
   else
      return 1.0 / this->lang2_numtrans[it.getPhraseIndex(2)];
}

template<class T>
double IndicatorSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   double d = this->lang1_marginals[it.getPhraseIndex(1)];
   if (d)
      return it.getJointFreq() ? 1.0 : this->alpha;
   else
      return 1.0 / this->lang1_numtrans[it.getPhraseIndex(1)];
}

/*-----------------------------------------------------------------------------
 * LeaveOneOut
 */
template<class T>
LeaveOneOutSmoother<T>::LeaveOneOutSmoother(PhraseSmootherFactory<T>& factory, const string& args) :
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
double LeaveOneOutSmoother<T>::estm(T jointfreq, T margefreq) {
   if (strategy == 1) {
      if (--jointfreq <= 0) return 0;
      --margefreq;
   } else if (strategy == 2) {
      if (jointfreq-1 <= 0) return 0;
   }
   return jointfreq / (double)margefreq;
}

template<class T>
double LeaveOneOutSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   return estm(it.getJointFreq(), this->lang2_marginals[it.getPhraseIndex(2)]);
}

template<class T>
double LeaveOneOutSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   return estm(it.getJointFreq(), this->lang1_marginals[it.getPhraseIndex(1)]);
}

/*-----------------------------------------------------------------------------
 * Absolute Discounting
 */
template<class T>
AbsoluteDiscountSmoother<T>::AbsoluteDiscountSmoother(PhraseSmootherFactory<T>& factory, const string& args) :
   PhraseSmoother<T>(1), numD(1), verbose(factory.getVerbose()), ng1(NULL), ng2(NULL)
{
   given = false;
   discount = 0.0;
   vector<string> toks;
   split(args, toks);
   for (Uint i = 0; i < toks.size(); ++i) {
      if (toks[i] == "-m1") {
         if (++i == toks.size())
            error(ETFatal, "AbsoluteDiscount: expecting ng1 argument after -m1");
         ng1 = new NgramCounts(&voc, toks[i]);
      } else if (toks[i] == "-m2") {
         if (++i == toks.size())
            error(ETFatal, "AbsoluteDiscount: expecting ng2 argument after -m2");
         ng2 = new NgramCounts(&voc, toks[i]);
      } else {
         discount = conv<double>(toks[i]);
         given = true;
      }
   }

   global_event_counts.resize(numD+1);
   D.resize(numD);

   PhraseTableGen<T>& pt = *factory.getPhraseTable();
   lang1_marginals.resize(pt.numLang1Phrases());
   lang2_marginals.resize(pt.numLang2Phrases());
}

template<class T>
void AbsoluteDiscountSmoother<T>::tallyMarginals(const typename PhraseTableGen<T>::iterator& it)
{
   static vector<string> toks; // for ngrams lookup
   if (PhraseSmoother<T>::currentTallyMarginalsPass() == 1) {
      Uint id1 = it.getPhraseIndex(1);
      Uint id2 = it.getPhraseIndex(2);
      // Grow the vectors if necessary. Note: pruning can change phrase order.
      if (id1 == lang1_marginals.size()) {
         lang1_marginals.push_back(0);
      }
      if (id2 >= lang2_marginals.size()) {
         Uint new_size = (id2+1) * 1.5;
         lang2_marginals.resize(new_size);
      }
      // Calculate marginals and event counts
      if (ng1) {   // external marginals from ngrams
         if (lang1_marginals[id1] == 0) {
            it.getPhrase(1, toks);
            lang1_marginals[id1] = ng1->count(toks);
            if (lang1_marginals[id1] == 0)
               error(ETFatal, "AbsoluteDiscount: phrase %s not found in ngrams", it.getPhrase(1).c_str());
         }
      } else
         lang1_marginals[id1] += it.getJointFreq();

      if (ng2) {   // external marginals from ngrams
         if (lang2_marginals[id2] == 0) {
            it.getPhrase(2, toks);
            lang2_marginals[id2] = ng2->count(toks);
            if (lang2_marginals[id2] == 0)
               error(ETFatal, "AbsoluteDiscount: phrase %s not found in ngrams", it.getPhrase(2).c_str());
         }
      } else
         lang2_marginals[id2] += it.getJointFreq();


      // Save f values
      Uint f = (Uint)ceil((double)it.getJointFreq());
      if (f == 0) {
         string phrase1, phrase2;
         it.getPhrase(1, phrase1);
         it.getPhrase(2, phrase2);
         error(ETFatal, "AbsoluteDiscountSmoother: zero joint count for phrase pair %s ||| %s",
               phrase1.c_str(), phrase2.c_str());
      }
      if (f <= numD+1) ++global_event_counts[f-1];
   }
}

template<class T>
void AbsoluteDiscountSmoother<T>::tallyMarginalsFinishPass()
{
   if (PhraseSmoother<T>::currentTallyMarginalsPass() == 1) {
      if (!given) {
         // Calculate discounting coeffs
         const double Y = global_event_counts[0] / (global_event_counts[0] + 2.0 * global_event_counts[1]);
         for (Uint c = 1; c <= numD; ++c) {
            D[c-1] = c - (c+1) * Y * global_event_counts[c] / global_event_counts[c-1];
            if (verbose) cerr << "D" << c << " = " << D[c-1] << endl;
            if (c == 1) discount = D[c-1];
         }
         if (verbose) {
            cerr << "Using KN auto-discounts: " << discount << endl;
         }
      }
   }

   PhraseSmoother<T>::tallyMarginalsFinishPass();
}

template<class T>
double AbsoluteDiscountSmoother<T>::estm(T jointfreq, T margefreq) {
   double dfreq;
   if (discount < 1.0 && jointfreq < 1) { // poor man's digamma is a quadratic
      double m = 1.0 - discount;
      double m2 = m*m;
      double j2 = jointfreq*jointfreq;
      dfreq = (m-m2)*j2 + m2*jointfreq;
   } else {
      dfreq = jointfreq - discount;
      if (dfreq < 0) dfreq = 0;
   }
   return dfreq / (double)margefreq;
}

template<class T>
double AbsoluteDiscountSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   return estm(it.getJointFreq(), lang2_marginals[it.getPhraseIndex(2)]);
}

template<class T>
double AbsoluteDiscountSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   return estm(it.getJointFreq(), lang1_marginals[it.getPhraseIndex(1)]);
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
   PhraseSmoother<T>(1), numD(0), given(false), unigram(false), verbose(factory.getVerbose())
{
   if (args != "") {
      vector<string> toks;
      split(args, toks);
      for (Uint i = 0; i < toks.size(); ++i)
         if (toks[i] == "-u") {
            unigram = true;
         } else if (toks[i] == "-d") {
            if (++i == toks.size())
               error(ETFatal, "expecting KN discount coeffs after -d");
            given = true;
            split<double>(toks[i], D, ",");
            if (numD != 0 && numD != D.size())
               error(ETWarn, "KN numD and -d params conflict; ignoring numD");
            numD = D.size();
         } else {
            numD = conv<Uint>(toks[i]);
            if (given && numD != D.size())
               error(ETWarn, "KN numD and -d params conflict; ignoring numD");
         }
   }

   if (numD == 0)
      numD = 1;
   D.resize(numD);

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

   event_count_sum = 0;
   tot_freq = 0.0;
}

template<class T>
void KNSmoother<T>::tallyMarginals(const typename PhraseTableGen<T>::iterator& it)
{
   if (PhraseSmoother<T>::currentTallyMarginalsPass() == 1) {
      // Grow the vectors if necessary. Note: pruning can change phrase order.

      const Uint id1 = it.getPhraseIndex(1);
      assert(id1 <= lang1_marginals.size());
      if (id1 == lang1_marginals.size()) {
         lang1_marginals.push_back(0);
         for (Uint i = 0; i < numD; ++i)
            lang1_event_counts[i].push_back(0);
      }

      const Uint id2 = it.getPhraseIndex(2);
      if (id2 >= lang2_marginals.size()) {
         const Uint new_size = (id2+1) * 1.5;
         lang2_marginals.resize(new_size);
         for (Uint i = 0; i < numD; ++i)
            lang2_event_counts[i].resize(new_size);
      }
      //
      // Calculate marginals and event counts
      lang1_marginals[id1] += it.getJointFreq();
      lang2_marginals[id2] += it.getJointFreq();
      tot_freq += it.getJointFreq();

      Uint f = (Uint)ceil((double)it.getJointFreq());
      if (f == 0) {
         if (this->needsPrior())
            return; // tolerate 0's if we're really a KNX*Smoother
         string phrase1, phrase2;
         it.getPhrase(1, phrase1);
         it.getPhrase(2, phrase2);
         error(ETFatal, "KNSmoother: zero joint count for phrase pair %s ||| %s",
               phrase1.c_str(), phrase2.c_str());
      }
      if (f <= numD+1) ++global_event_counts[f-1];
      if (f > numD) f = numD;
      ++lang1_event_counts[f-1][id1];
      ++lang2_event_counts[f-1][id2];
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
      // Let's make sure we can calculate Y and not get a division by zero.
      if (!given and global_event_counts[0] == 0 and global_event_counts[1] == 0) {
         error(ETFatal, "No singleton found, cannot estimate KN dicount parameters.");
      }
      // Note: possible division by zero if no phrase pair has a joint count of 1.
      const double Y = global_event_counts[0] / (global_event_counts[0] + 2.0 * global_event_counts[1]);
      for (Uint c = 1; c <= numD; ++c) {
         if (!given) {
            if (global_event_counts[c-1] == 0) {
               error(ETFatal, "No singleton found, cannot estimate KN dicount parameters.");
            }
            D[c-1] = c - (c+1) * Y * global_event_counts[c] / global_event_counts[c-1];
         }
         if (verbose)
            cerr << "D" << c << " = " << D[c-1] << endl;
      }
   }

   PhraseSmoother<T>::tallyMarginalsFinishPass();
}

template<class T>
double KNSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   const Uint id1 = it.getPhraseIndex(1);
   const Uint id2 = it.getPhraseIndex(2);
   Uint f = (Uint)ceil((double)it.getJointFreq());
   assert (f != 0);
   if (f > numD) f = numD;
   const double disc = D[f-1];

   double gamma = 0.0;
   for (Uint i = 0; i < numD; ++i)
      gamma += D[i] * lang2_event_counts[i][id2];

   double lower_order_distn = 0.0;
   if (unigram) {
      lower_order_distn = lang1_marginals[id1] / tot_freq;
   } else {
      for (Uint i = 0; i < numD; ++i)
         lower_order_distn += lang1_event_counts[i][id1];
      lower_order_distn /= event_count_sum;
   }

   if (verbose > 1)
      dumpProbInfo("L1|L2", it.getJointFreq(), disc, gamma, lower_order_distn,
                   lang2_marginals[id2]);

   return (it.getJointFreq() - disc + gamma * lower_order_distn) /
      lang2_marginals[id2];
}

// copy of above, just swap 1 <-> 2 everwhere

template<class T>
double KNSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   const Uint id1 = it.getPhraseIndex(1);
   const Uint id2 = it.getPhraseIndex(2);
   Uint f = (Uint)ceil((double)it.getJointFreq());
   assert (f != 0);
   if (f > numD) f = numD;
   const double disc = D[f-1];

   double gamma = 0.0;
   for (Uint i = 0; i < numD; ++i)
      gamma += D[i] * lang1_event_counts[i][id1];

   double lower_order_distn = 0.0;
   if (unigram) {
      lower_order_distn = lang2_marginals[id2] / tot_freq;
   } else {
      for (Uint i = 0; i < numD; ++i)
         lower_order_distn += lang2_event_counts[i][id2];
      lower_order_distn /= event_count_sum;
   }

   if (verbose > 1)
      dumpProbInfo("L2|l1", it.getJointFreq(), disc, gamma, lower_order_distn,
                   lang1_marginals[id1]);

   return (it.getJointFreq() - disc + gamma * lower_order_distn) /
      lang1_marginals[id1];
}

template<class T>
string KNSmoother<T>::getInternalState(const typename PhraseTableGen<T>::iterator& it) const {
   ostringstream oss;
   const Uint id1 = it.getPhraseIndex(1);
   const Uint id2 = it.getPhraseIndex(2);

   Uint U = 0;
   Uint V = 0;
   assert(lang1_event_counts.size() == lang2_event_counts.size());
   for (Uint i = 0; i < numD; ++i) {
      U += lang1_event_counts[i][id1];
      V += lang2_event_counts[i][id2];
   }

   oss << " knstats"
       << '=' << it.getJointFreq()
       << ',' << lang1_marginals[id1]
       << ',' << lang2_marginals[id2]
       << ',' << U
       << ',' << V;

   return oss.str();
}


/*-----------------------------------------------------------------------------
 * Kneser-Ney with external backoff distribution.
 *
 * Using the clunky "this->" syntax everywhere to avoid the even clunkier
 * "KNSmoother<T>::" qualification.  Inheritance co-exists uneasily with
 * templates...
 */
template<class T>
double KNXSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it,
                                           double bkpr)
{
   Uint f = (Uint)ceil((double)it.getJointFreq());
   if (f > this->numD) f = this->numD;
   double disc = f ? this->D[f-1] : 0.0;
   double gamma = 0.0;
   for (Uint i = 0; i < this->numD; ++i)
      gamma += this->D[i] * this->lang2_event_counts[i][it.getPhraseIndex(2)];
   double m = this->lang2_marginals[it.getPhraseIndex(2)];
   if (this->verbose > 1)
      this->dumpProbInfo("L1|L2", it.getJointFreq(), disc, gamma, bkpr, m);
   return m ? (it.getJointFreq() - disc + gamma * bkpr) / m : bkpr;
}

// copy of above, just swap 1 <-> 2 everwhere

template<class T>
double KNXSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it,
                                           double bkpr)
{
   Uint f = (Uint)ceil((double)it.getJointFreq());
   if (f > this->numD) f = this->numD;
   double disc = f ? this->D[f-1] : 0.0;
   double gamma = 0.0;
   for (Uint i = 0; i < this->numD; ++i)
      gamma += this->D[i] * this->lang1_event_counts[i][it.getPhraseIndex(1)];
   double m = this->lang1_marginals[it.getPhraseIndex(1)];
   if (this->verbose > 1)
      this->dumpProbInfo("L2|l1", it.getJointFreq(), disc, gamma, bkpr, m);
   return m ? (it.getJointFreq() - disc + gamma * bkpr) / m : bkpr;
}

/*-----------------------------------------------------------------------------
 * Kneser-Ney with external backoff distribution for 0 counts.
 */

template<class T>
double KNXZSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it,
                                            double bkpr)
{
   Uint f = (Uint)ceil((double)it.getJointFreq());
   if (f) return KNSmoother<T>::probLang1GivenLang2(it);
   double m = this->lang2_marginals[it.getPhraseIndex(2)];
   if (m == 0) return bkpr;
   double gamma = 0.0;
   for (Uint i = 0; i < this->numD; ++i)
      gamma += this->D[i] * this->lang2_event_counts[i][it.getPhraseIndex(2)];
   return gamma * bkpr / m;
}


template<class T>
double KNXZSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it,
                                           double bkpr)
{
   Uint f = (Uint)ceil((double)it.getJointFreq());
   if (f) return KNSmoother<T>::probLang2GivenLang1(it);
   double m = this->lang1_marginals[it.getPhraseIndex(1)];
   if (m == 0) return bkpr;
   double gamma = 0.0;
   for (Uint i = 0; i < this->numD; ++i)
      gamma += this->D[i] * this->lang1_event_counts[i][it.getPhraseIndex(1)];
   return gamma * bkpr / m;
}

/*-----------------------------------------------------------------------------
 * EventCounts
 */

template<class T>
EventCounts<T>::EventCounts(PhraseSmootherFactory<T>& factory, const string& args) :
   PhraseSmoother<T>(1), thresh(1)
{
   thresh = conv<Uint>(args);
   PhraseTableGen<T>& pt = *factory.getPhraseTable();
   lang1_counts.resize(pt.numLang1Phrases(), 0);
   lang2_counts.resize(pt.numLang2Phrases(), 0);
}

template<class T>
void EventCounts<T>::tallyMarginals(const typename PhraseTableGen<T>::iterator& it)
{
   Uint f = (Uint)ceil((double)it.getJointFreq());
   if (f >= thresh) {
      ++lang1_counts[it.getPhraseIndex(1)];
      ++lang2_counts[it.getPhraseIndex(2)];
   }
}

template<class T>
double EventCounts<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   return lang1_counts[it.getPhraseIndex(1)];
}

template<class T>
double EventCounts<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   return lang2_counts[it.getPhraseIndex(2)];
}

/*-----------------------------------------------------------------------------
 * Base class for all lexical smoothers
 */
template<class T>
LexicalSmoother<T>::LexicalSmoother(PhraseSmootherFactory<T>& factory, const string& args) :
   PhraseSmoother<T>(0)
{
   Uint model_index = 0;
   if (args != "") {
      vector<string> toks;
      split(args, toks, " \t\n", 1);
      model_index = conv<Uint>(toks[0]);
      if (model_index-- == 0)
         error(ETFatal, "IBM model index must be non-0 for LexicalSmoothers");
   }
   ibm_lang2_given_lang1 = factory.getIBMLang2GivenLang1(model_index);
   ibm_lang1_given_lang2 = factory.getIBMLang1GivenLang2(model_index);
   if (!(ibm_lang2_given_lang1 && ibm_lang1_given_lang2))
     error(ETFatal, "Can't create Lexical (Zens-Ney, IBM1 or IBM) smoother without IBM models");
}

template<class T>
double LexicalSmoother<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   it.getPhrase(1, l1_phrase);
   it.getPhrase(2, l2_phrase);

   return probLang1GivenLang2(l1_phrase, l2_phrase);
}

template<class T>
double LexicalSmoother<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   it.getPhrase(1, l1_phrase);
   it.getPhrase(2, l2_phrase);

   return probLang2GivenLang1(l1_phrase, l2_phrase);
}

/*-----------------------------------------------------------------------------
 * Zens-Ney using IBM1 parameters.
 */
template<class T>
ZNSmoother<T>::ZNSmoother(PhraseSmootherFactory<T>& factory, const string& args) :
   LexicalSmoother<T>(factory, args)
{}

template<class T>
ZNSmoother<T>::ZNSmoother(IBM1* ibm_lang2_given_lang1, IBM1* ibm_lang1_given_lang2) :
   LexicalSmoother<T>(ibm_lang2_given_lang1, ibm_lang1_given_lang2)
{}

template<class T>
double ZNSmoother<T>::probLang1GivenLang2(const vector<string>& l1_phrase, const vector<string>& l2_phrase)
{
   double pr_global = 1.0;
   for (Uint j = 0; j < l1_phrase.size(); ++j) {

      // get p(l1_pharase[j]|x) for all x in l2_phrase
      this->ibm_lang1_given_lang2->IBM1::pr(l2_phrase, l1_phrase[j], &phrase_probs);

      double pr_local = 1.0;
      for (Uint i = 0; i < phrase_probs.size(); ++i)
         pr_local *= (1.0 - phrase_probs[i]);

      pr_global *= (1.0 - pr_local);
   }

   return pr_global == 0.0 ? PhraseSmoother<T>::VERY_SMALL_PROB : pr_global;
}

template<class T>
double ZNSmoother<T>::probLang2GivenLang1(const vector<string>& l1_phrase, const vector<string>& l2_phrase)
{
   double pr_global = 1.0;
   for (Uint j = 0; j < l2_phrase.size(); ++j) {

      // get p(l2_pharase[j]|x) for all x in l1_phrase
      this->ibm_lang2_given_lang1->IBM1::pr(l1_phrase, l2_phrase[j], &phrase_probs);

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
   LexicalSmoother<T>(factory, args)
{}

template<class T>
IBM1Smoother<T>::IBM1Smoother(IBM1* ibm_lang2_given_lang1, IBM1* ibm_lang1_given_lang2) :
   LexicalSmoother<T>(ibm_lang2_given_lang1, ibm_lang1_given_lang2)
{}

template<class T>
double IBM1Smoother<T>::probLang1GivenLang2(const vector<string>& l1_phrase, const vector<string>& l2_phrase)
{
   return exp(this->ibm_lang1_given_lang2->IBM1::logpr(l2_phrase, l1_phrase, 1e-07));
}

template<class T>
double IBM1Smoother<T>::probLang2GivenLang1(const vector<string>& l1_phrase, const vector<string>& l2_phrase)
{
   return exp(this->ibm_lang2_given_lang1->IBM1::logpr(l1_phrase, l2_phrase, 1e-07));
}

/*-----------------------------------------------------------------------------
 * IBMSmoother.
 */
template<class T>
IBMSmoother<T>::IBMSmoother(PhraseSmootherFactory<T>& factory, const string& args):
   LexicalSmoother<T>(factory, args)
{}

template<class T>
IBMSmoother<T>::IBMSmoother(IBM1* ibm_lang2_given_lang1, IBM1* ibm_lang1_given_lang2) :
   LexicalSmoother<T>(ibm_lang2_given_lang1, ibm_lang1_given_lang2)
{}

template<class T>
double IBMSmoother<T>::probLang1GivenLang2(const vector<string>& l1_phrase, const vector<string>& l2_phrase)
{
   return exp(this->ibm_lang1_given_lang2->logpr(l2_phrase, l1_phrase, 1e-10));
}

template<class T>
double IBMSmoother<T>::probLang2GivenLang1(const vector<string>& l1_phrase, const vector<string>& l2_phrase)
{
   return exp(this->ibm_lang2_given_lang1->logpr(l1_phrase, l2_phrase, 1e-10));
}

/*-----------------------------------------------------------------------------
 * Coarse Model
 */

template<class T>
CoarseModel<T>::CoarseModel(PhraseSmootherFactory<T>& factory, const string& args) :
   PhraseSmoother<T>(0)
{
   vector<string> toks;
   split(args, toks);
   if (toks.size() != 3)
      error(ETFatal, "Coarse Model smoother requires 3 filepath arguments: coarse_model l1_classes l2_classes.");
   l1_classes.read(toks[1]);
   l2_classes.read(toks[2]);

   iSafeMagicStream cm_stream(toks[0]);
   string line;
   vector<string> cm_toks;
   PhraseTableBase::ToksIter b1, b2, e1, e2, v, a, f;
   Uint pp_index;
   Uint num_cols = 0;   // number of columns in coarse model CPT
   Uint num_phrase_pairs = 0;
   while (getline(cm_stream, line)) {
      cm_pt.extractTokens(line, cm_toks, b1, e1, b2, e2, v, a, f, true);
      num_cols = static_cast<Uint>(a-v);
      if (num_cols != 2)
         error(ETFatal, "Coarse Model smoother requires 2 value columns in coarse model: found %d.", num_cols);
      if (! cm_pt.exists(b1, e1, b2, e2, pp_index)) {
         pp_index = num_phrase_pairs++;
         cm_pt.addPhrasePair(b1, e1, b2, e2, pp_index);
         cm_probs.push_back(make_pair(conv<float>(*v), conv<float>(*(v+1))));
      } else {
         error(ETFatal, "Coarse Model smoother found duplicate phrase pair in coarse model: '%s ||| %s'",
               join(b1,e1).c_str(), join(b2,e2).c_str());
      }
   }
}

template<class T>
Uint CoarseModel<T>::getCMPhrasePairIndex(const typename PhraseTableGen<T>::iterator& it)
{
   Uint pp_index;
   vector<string> toks1, toks2;
   it.getPhrase(1, toks1);
   it.getPhrase(2, toks2);

   vector<string> ctoks1, ctoks2;
   ctoks1.reserve(toks1.size());
   ctoks2.reserve(toks2.size());
   char sbuf[12];
   for (Uint i = 0; i < toks1.size(); ++i) {
      sprintf(sbuf, "%d", l1_classes.classOf(toks1[i].c_str()));
      ctoks1.push_back(string(sbuf));
   }
   for (Uint i = 0; i < toks2.size(); ++i) {
      sprintf(sbuf, "%d", l2_classes.classOf(toks2[i].c_str()));
      ctoks2.push_back(string(sbuf));
   }

   if (!cm_pt.exists(ctoks1.begin(), ctoks1.end(), ctoks2.begin(), ctoks2.end(), pp_index))
      error(ETFatal, "Coarse Model smoother failed to find '%s ||| %s' ('%s ||| %s') in coarse model.",
            join(toks1).c_str(), join(toks2).c_str(), join(ctoks1).c_str(), join(ctoks2).c_str());
   return pp_index;
}

template<class T>
double CoarseModel<T>::probLang1GivenLang2(const typename PhraseTableGen<T>::iterator& it)
{
   return cm_probs[getCMPhrasePairIndex(it)].first;
}

template<class T>
double CoarseModel<T>::probLang2GivenLang1(const typename PhraseTableGen<T>::iterator& it)
{
   return cm_probs[getCMPhrasePairIndex(it)].second;
}

} // Portage
#endif
