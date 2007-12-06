/**
 * @author Matthew Arnold (slight cleanup by GF)
 * @file kn_smoother.h  Kneser-Ney smoothing.
 * 
 *
 * NOTE: This code is no longer used.
 * 
 * COMMENTS: 
 *
 * Kneser-Ney and IBM-based bag-of-words smoothing for phrase table
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef KN_SMOOTHER_H
#define KN_SMOOTHER_H

namespace Portage {

namespace {
const double EPSILON = 1e-3;
const Uint MAX_LENGTH = 8;
}

/// Structure to hold smoothing bucketing.
struct smoothConst {
   /// Default constructor.
   smoothConst()
   {}
   /**
    * @param count
    * @param d
    * @param r
    * @param rmean
    */
   smoothConst(double count, double d, double r, double rmean)
      :ct(count), D(d), R(r), Rmean(rmean)
   {}
   double ct;     ///< count
   double D;
   double R;
   double Rmean;
};

/// Kneser-Ney smoothing.
class KNSmoother 
{

   //START OF KNESSER-NEY SMOOTHING

   vector<smoothConst> smoothing;  ///< smoothing constants (bucketed, in form bucket (<=))
   bool KNsmoothed;                ///< KN smoothing or not.
   vector<Uint> uniqPairs;         ///< number of total pairs around (calculated only when used, and only if no new phrases are added).

   //shorts to save space -->assume no more than 65K pairs for any individual source or target
   vector<vector<short unsigned int> > uniqTgts;  ///< number of unique target sentences for each bucket.



   /** 
    * Get the number of unique sentence for tgt.
    * Before calling there,  must have called checkTgts first, and not have added any new pairs since that call
    * used to calculate on the fly but was too inefficient.
    * @param tgt
    * @param i must be valid index
    * @return Returns the number of unique target sentences.
    */
   inline Uint numUniqtoTgt(Uint tgt, Uint i) {
      return uniqTgts[tgt][i];
   }

   /**
    * Get the number of unique sentences to src.
    * @param src  
    * @param i  
    * @return Returns the number of unique source sentences.
    */
   inline Uint numUniqtoSrc(const string &src, Uint i) {
      Uint ct = 0;
      typename Phrases::iterator srcIt = phrase_table.find(src);
      for (typename PhraseFreqs::iterator it = srcIt->second.begin(); it != srcIt->second.end(); ++it)
	 if (it->second > smoothing[i].D)
	    ct++;
      //	return //uniqSrcs[src][i];
      return ct;
   }
   
   /**
    * Get the number of unique pairs for i.
    * @param i
    * @return Returns the number of unique pairs for i.
    */
   inline Uint numUniq(Uint i) {
      return uniqPairs[i];
   }

   /**
    * Finds the first index greater or equal to count.
    * @param count  lower bound count to find.
    * @return Returns the first index in the smoothing table where which is
    * greater or equal to count or else the number of smoothing elements.
    */
   Uint Dindex(double count) {
      for (Uint i = 0; i < smoothing.size(); ++i)
	 if (count <= smoothing[i].ct)
	    return i;
      return smoothing.size();
   }

   /**
    *
    * @param index
    * @return Returns
    */
   inline double D(Uint index) {
      if (index == smoothing.size())
      	return 0;
      return smoothing[index].D;
   }

   /**
    * Individual G and H probability functions (need different copies as they're all calculated slightly
    * differently).
    * @param src
    * @param sum
    * @return 
    */
   double Gtgs(const string &src, double sum) {
      Uint index = Dindex(sum);
      if (index == smoothing.size())
	 return 0;
      return D(index) * numUniqtoSrc(src, index) / sum;
   }
   
   /**
    *
    * @param tgt
    * @param sum
    * @return 
    */
   double Gsgt(Uint tgt, double sum) {
      Uint index = Dindex(sum);
      if (index == smoothing.size())
	 return 0;		   
      return D(index) * numUniqtoTgt(tgt, index) / sum;
   }
   
   /**
    *
    * @param tgt
    * @param sum
    * @return 
    */
   double Htgs(Uint tgt, double sum) {
      Uint index = Dindex(sum);
      if (index == smoothing.size())
	 return 0;
      return (double)numUniqtoTgt(tgt, index) / numUniq(index);
   }
   
   /**
    *
    * @param src
    * @param sum
    * @return 
    */
   double Hsgt(const string &src, double sum) {
      Uint index = Dindex(sum);
      if (index == smoothing.size())
	 return 0;
      return (double)numUniqtoSrc(src, index) / numUniq(index);
   }

   /** 
    * pr_KN?G? = Knesser-Ney probability (target given source or source given target).
    * tgtIt is a valid iterator for src (ie. tgtIt is a valid iterator
    * in [srcIt->second.begin(), srcIt->second.end())).
    * @param srcIt
    * @param tgtIt
    * @param sum
    * @return 
    */
   inline double pr_KNTGS(const typename Phrases::iterator &srcIt, const typename PhraseFreqs::iterator &tgtIt, double sum) {
   	double count = tgtIt->second - D(Dindex(sum));
      return (count > 0? count : 0) / sum + Gtgs(srcIt->first, sum) * Htgs(tgtIt->first, sum);
   }
   
   /**
    * Baseline function (works if Tgt is never aligned with srcIt) -- only used for checks (check and checkMargin).
    * @param srcIt
    * @param tgt
    * @param sum
    * @return
    */
   inline double pr_KNTGS(const typename Phrases::iterator &srcIt, Uint tgt, double sum) {
      typename PhraseFreqs::iterator tgtIt = srcIt->second.find(tgt);
      if (tgtIt  != srcIt->second.end())
	 return pr_KNTGS(srcIt, tgtIt, sum);
      else	return 0.0 / sum + Gtgs(srcIt->first, sum) * Htgs(tgt, sum);
      //max(c(s-t)-D, 0) 		/ sum		+ G(src) * H(tgt)
   }
	
   /**
    *
    * @param srcIt
    * @param tgtIt
    * @return
    */
   inline double pr_KNSGT(const typename  Phrases::iterator &srcIt, const typename PhraseFreqs::iterator &tgtIt) {
      double sum = (double)lang2_voc.freq(tgtIt->first);
      double count = tgtIt->second - D(Dindex(sum));
      return (count > 0?count:0) / sum + Gsgt(tgtIt->first, sum) *  Hsgt(srcIt->first, sum);
   }
   
   /**
    * Verify that the uniqTgts vector is the correct length.
    * @param force  values: 0 = normal (only what needed), 1 = only tgt, 2 = only src, 3 = all (must do).
    */
   void check_tgts(Uint force = 0) {
      //KN smoothing update
      Uint size = smoothing.size();
      bool toFix = false;
      if (KNsmoothed && (uniqTgts.size() != lang2_voc.size()/* || uniqSrcs.size() != phrase_table.size()*/)) {
	 uniqTgts.clear();
	 uniqPairs.clear();
	 uniqTgts.resize(lang2_voc.size());
	 uniqPairs.resize(size);
	 toFix = true;
      }
      if (BWMeansmoothed && (normSrcs.size() != phrase_table.size() || normTgts.size() != lang2_voc.size())) {
	 normSrcs.clear();
	 normTgts.clear();
	 normTgts.resize(lang2_voc.size());
	 toFix = true;
      }//*/
      if (toFix || force == 3)
         for (typename Phrases::iterator it = phrase_table.begin(); it != phrase_table.end(); ++it)
	    for (typename PhraseFreqs::iterator pf = it->second.begin(); pf != it->second.end(); ++pf) {
	       if (KNsmoothed) {
	          //KN smoothing
	          uniqTgts[pf->first].resize(size);
	          for (Uint i = 0; i < size; ++i)
		     if (pf->second > smoothing[i].D) {
		        uniqTgts[pf->first][i]++;
		        uniqPairs[i]++;
		     }//end if, for KN
	       }//end KN
	       // BW smoothing
	       if (BWMeansmoothed) {
	          if (force == 2 || force == 0 || force == 3)
		     normSrcs[it->first] += pr_BWTGS(it, pf, true);
	          if (force == 1 || force == 0 || force == 3)
		     normTgts[pf->first] += pr_BWSGT(it, pf, true);
	       }//end BW
	    }//end fors
   }//end check

   //END OF KNESSER_NEY SMOOTHING

   //START OF BAG OF WORDS SMOOTHING

   LengthProb<MAX_LENGTH> length;     ///< for bag of words smoothing model
   bool BWsmoothed;                   ///< using model or not
   bool BWMeansmoothed;
   //floats to save space
   vector<float> normTgts;  ///< normalisation value for target sentences
   hash_map<string, float, StringHash> normSrcs;  ///< for source sentences
	 
   IBM1 *ibm_sgt;	///< ibm model
   IBM1 *ibm_tgs;	///< ibm model

   /**
    * Bag of Words probability source given target.
    * Pre: MUST send in IBM table that is a SRC_given_TGT model.
    * Post: NOT nomralised bag of words model score -> if mean = true.
    * @param srcIt  
    * @param tgtIt  
    * @param mean  
    * @return Returns the raw score, to be divided by the sum total to obtain the "real" score
    * for it.
    */
   double pr_BWSGT(const typename Phrases::iterator &srcIt, const typename PhraseFreqs::iterator &tgtIt, bool mean = false) {
      assert(ibm_sgt);
      vector<string> srctoks, tgttoks;

      decodePhrase(srcIt->first, srctoks, wvoc1);
      decodePhrase(lang2_voc.word(tgtIt->first), tgttoks, wvoc2);

      double pr = 1.0;
      for (Uint i = 0; i < srctoks.size(); ++i)
	 pr *= ibm_sgt->pr(tgttoks, srctoks[i]);
      if (mean)
	 pr = pow(pr, 1.0/srctoks.size());
      return pr * length.pr_sgt(srctoks.size(), tgttoks.size());
   }  //

   /**
    * Bag of Words probability target given source.
    * Pre: MUST send in IBM table that is a TGT_given_SRC model.
    * Post: NOT nomralised bag of words model score -> if mean = true.
    * @param srcIt  
    * @param tgtIt  
    * @param mean  
    * @return Returns the raw score, to be divided by the sum total to obtain the "real" score
    * for it.
    */
   double pr_BWTGS(const typename Phrases::iterator &srcIt, const typename PhraseFreqs::iterator &tgtIt, bool mean = false) {
      assert(ibm_tgs);
      vector<string> srctoks, tgttoks;

      decodePhrase(srcIt->first, srctoks, wvoc1);
      decodePhrase(lang2_voc.word(tgtIt->first), tgttoks, wvoc2);

      double pr = 1.0; 
      for (Uint i = 0; i < tgttoks.size(); ++i)
	 pr *= ibm_tgs->pr(srctoks, tgttoks[i]);
      if (mean)
	 pr = pow(pr, 1.0/tgttoks.size());
      return pr * length.pr_tgs(srctoks.size(), tgttoks.size());
   }


   /**
    * Does the interpolation of the score => iterates through interpolation list given until it finds the correct
    * bucket for target given source.
    * @param srcIt  
    * @param tgtIt  
    * @param count
    * @param value
    * @return
    */
   double interpolate_BW_tgs(const typename Phrases::iterator &srcIt, const typename PhraseFreqs::iterator &tgtIt, double count, double value) {
      for (Uint i = 0; i < smoothing.size(); ++i)
	 if (count <= smoothing[i].ct) {
	    if (BWMeansmoothed && normSrcs[srcIt->first] == 0.0)
	       normSrcs[srcIt->first] = 1.0;
	    double bwA = smoothing[i].R;
	    double bwB = smoothing[i].Rmean;
	    return bwA * pr_BWTGS(srcIt, tgtIt, false) + 
	       (bwB != 0 ? (bwB * pr_BWTGS(srcIt, tgtIt, true) / normSrcs[srcIt->first]) : 0) + (1 - bwA - bwB) * value;
	 }//end if smoothing
      return value;
   }

   /**
    * Does the interpolation of the score => iterates through interpolation list given until it finds the correct
    * bucket for source given target.
    * @param srcIt  
    * @param tgtIt  
    * @param count
    * @param value
    * @return
    */
   double interpolate_BW_sgt(const typename Phrases::iterator &srcIt, const typename PhraseFreqs::iterator &tgtIt, double count, double value) {
      for (Uint i = 0; i < smoothing.size(); ++i)
	 if (count <= smoothing[i].ct) {
	    if (BWMeansmoothed && normTgts[tgtIt->first] == 0.0)
	       normTgts[tgtIt->first] = 1.0;
	    double bwA = smoothing[i].R;
	    double bwB = smoothing[i].Rmean;
	    return bwA * pr_BWSGT(srcIt, tgtIt) + (bwB != 0 ? (bwB * pr_BWSGT(srcIt, tgtIt, true) / normTgts[tgtIt->first]) : 0) + (1 - bwA - bwB) * value;
	 }//end if smoothing
      return value;
   }
   //*/

public:

   /// Constructor.
   KNSmoother() :
      KNsmoothed(false), BWsmoothed(false), BWMeansmoothed(false), ibm_sgt(0), ibm_tgs(0)
   {}

   /// Removes smoothing (not needed now, but could be useful later).
   void unsmooth() {
      KNsmoothed = false;
      BWsmoothed = false;
      BWMeansmoothed = false;
      ibm_sgt = 0;
      ibm_tgs = 0;
      smoothing.clear();
      normSrcs.clear();
      normTgts.clear();
      uniqTgts.clear();
      uniqPairs.clear();
   }

   /**
    * Set up smoothing: read in file in format:
    * bucket1 D1 R1 Rmean1
    * bucket2 D2 R2 Rmean2
    * ...
    *
    * where bucket 1 refers to applying smoothing to all phrases 0 < ct(P) <= bucket1 for both forwards and
    * reverse
    * bucket1 < bucket2 < bucket3 < ...
    * if bucket <= 0, then it counts as INFINITY, and will effectively be the last bucket (since buckets are
    * searched sequentially)
    *
    * @param file file name.
   */
   void smooth(const string &file) {
      IMagicStream in(file);
      double ct, d, r, rmean; //order of values in file
      while (in >> ct) {
	 if (! (in >> d >>  r >> rmean)) {
	    error(ETWarn, "Invalid file format for smoothing.  Ignoring rest of file...");
	    return;
	 }
	 //read in ct, d, r, rmean
	 if (ct <= 0.0)
	    ct = INFINITY;
	 if (r < 0 || r >= 1.0)
	    error(ETFatal, "BW variant A constant (%d) out of range", r);
	 if (rmean < 0 || rmean >= 1.0)
	    error(ETFatal, "BW variant B constant (%d) out of range", rmean);
	 if (r+rmean >= 1.0)//cannot sum to 1
	    error(ETFatal, "BW variants combine for too large value (%d + %d)", r, rmean);
	 if (d < 0)
	    error(ETFatal, "Discounting coefficient cannot be negative");
	 smoothing.push_back(smoothConst(ct, d, r, rmean));
	 if (d != 0.0)
	    KNsmoothed = true;
	 if (r != 0.0)
	    BWsmoothed = true;
	 if (rmean != 0.0)
	    BWMeansmoothed = true;
      }//end while
   }


   /**
    * Add in ibm models for BW smoothing.
    * @param tgs  target given source.
    * @param sgt  soruce given target.
    */
   void add_ibmBW(IBM1 *tgs, IBM1 *sgt) {
      ibm_sgt = sgt;
      ibm_tgs = tgs;
   }
   
   /**
    * All check functions were added to verify math of program.
    * (KN smoothing)=>extremely slow, probably could be removed.
    * @param  srcIt
    * @return
    */
   bool check(const typename Phrases::iterator &srcIt) {
      //check to make sure that sum_tgt (p(t|s)) == 1
      //code mostly the same as that for calculating p(t|s)
      check_tgts();
      double sum = 0.0;
      typename PhraseFreqs::iterator pf;
      for (pf = srcIt->second.begin(); pf != srcIt->second.end(); ++pf)
	 sum += pf->second;
      //calculate c(src)
      double total = 0.0;
      for (Uint i = 0; i < lang2_voc.size(); ++i) {
	 total += pr_KNTGS(srcIt, i, sum);
      }
      //EPSILON declared above
      if (fabs(total - 1.0) < EPSILON)
	 return true;
      else {
	 cerr << srcIt->first << " sum = " << total << endl;
	 return false;
      }//end if else
   }//end func

   /**
    *
    * @param tgt
    * @return 
    */
   bool checkMargin(Uint tgt) {
      check_tgts();
      double total = 0.0;
      for (typename Phrases::iterator p = phrase_table.begin(); p != phrase_table.end(); ++p) 
	 total += pr_KNTGS(p, tgt, 1.0); //since pr_KNTGS  = x/sum and we want sum * pr_KNTGS
      if (fabs(total - lang2_voc.freq(tgt)) < EPSILON)
	 return true;
      else {
	 cerr << lang2_voc.word(tgt) << " count = " << total << " should be: " << lang2_voc.freq(tgt) << endl;
	 return false;
      }//end if else  */
   }

   ///
   bool checkall() {
      if (!KNsmoothed)
	 return true;
      for (typename Phrases::iterator it = phrase_table.begin(); it != phrase_table.end(); ++it)
	 if (!check(it))
	    return false;
      for (Uint i = 0; i < lang2_voc.size(); ++i)
	 if (!checkMargin(i))
	    return false;
      return true;
   }//end checkall	*/

   /// Clears the uniq and norm.
   void clear()
   {
      if (KNsmoothed) {
	 uniqTgts.clear();
	 uniqPairs.clear();
      }
      if (BWMeansmoothed) {
	 normTgts.clear();
	 normSrcs.clear();
      }	
   }

   /**
    *
    * @param beg1
    * @param end1
    * @param beg2
    * @param end2
    * @param val
    */
   void addPhrasePair(ToksIter beg1, ToksIter end1, ToksIter beg2, ToksIter end2, T val=1) 
   {
//       string phrase1, phrase2;
//       codePhrase(beg1, end1, phrase1, wvoc1);
//       codePhrase(beg2, end2, phrase2, wvoc2);

//       Uint id2 = lang2_voc.add(phrase2.c_str(), val);
//       phrase_table[phrase1][id2] += val;

      //added
      if (KNsmoothed) {
	 uniqTgts.clear();
	 uniqPairs.clear();
      }
      if (BWsmoothed) {
	 length.add((int)(end1-beg1),(int)(end2-beg2));//assuming end1-beg1 == size(beg1, end1) 
	 normSrcs.clear();
	 normTgts.clear();
      }
   }

   /** 
    * Print the sum count of the length values of all pairs.
    * @param os  stream to output.
    */
   void printLengths(ostream &os) {
      for (Uint i = 1; i <= MAX_LENGTH; i++) {
	 for (Uint j = 1; j <= MAX_LENGTH; j++)
	    os << length.sum(i, j) << "\t";
	 os << ":" << length.sum_src(i) << endl;
      }
      for (Uint j = 1; j <= MAX_LENGTH; j++)
	 os << ":" << length.sum_tgt(j) << "\t";
      os << endl;
   }

   /**
    * Currently not doing anything.
    * @param ostr
    * @param l1o_strategy
    */
   void dump_prob_lang2_given_lang1(ostream& ostr, Uint l1o_strategy = 0)
   {
//       check_tgts(2);
//       typename Phrases::iterator p;
//       for (p = phrase_table.begin(); p != phrase_table.end(); ++p) {
// 	 //p->first = source string, p->second = map(target index, val)
// 	 double sum = 0.0;
// 	 typename PhraseFreqs::iterator pf;
// 	 for (pf = p->second.begin(); pf != p->second.end(); ++pf)
// 	    sum += pf->second;
// 	 for (pf = p->second.begin(); pf != p->second.end(); ++pf) {
// 	    //pf->first = target index, pf->second = val
// 	    //ladder added for smoothing
// 	    double value;
// 	    if (BWsmoothed)
// 	       if (KNsmoothed)
// 	          value = interpolate_BW_tgs(p, pf, sum, pr_KNTGS(p, pf, sum));
// 	       else value = interpolate_BW_tgs(p, pf, sum, pf->second / sum);
// 	    else 
// 	       if (KNsmoothed)
// 	          value = pr_KNTGS(p, pf, sum);
// 	       else {	
// 		  value = leaveOneOut(l1o_strategy, pf->second, sum); //baseline
//                   if (value == 0) continue;
//                }
// 	    writePhrasePair(ostr, lang2_voc.word(pf->first), p->first.c_str(), value, wvoc2, wvoc1);
// 	 }
//       }
   }

   /**
    * Currently not doing anything.
    * @param ostr
    * @param l1o_strategy
    */
   void dump_prob_lang1_given_lang2(ostream& ostr, Uint l1o_strategy = 0) 
   {
//       check_tgts(1);
//       typename Phrases::iterator p;
//       for (p = phrase_table.begin(); p != phrase_table.end(); ++p) {
//          typename PhraseFreqs::iterator pf;
//          for (pf = p->second.begin(); pf != p->second.end(); ++pf) {
//             //pf->first = target index, pf->second = val
//             double value;
// 	    double sum = lang2_voc.freq(pf->first);
// 	    //smoothing ladder
// 	    if (BWsmoothed)
// 	       if (KNsmoothed)
// 	          value = interpolate_BW_sgt(p, pf, sum, pr_KNSGT(p, pf));
// 	       else value = interpolate_BW_sgt(p, pf, sum, pf->second / sum);
// 	    else 
// 	       if (KNsmoothed)
// 	          value = pr_KNSGT(p, pf);
// 	       else {
// 		  value = leaveOneOut(l1o_strategy, pf->second, sum); //baseline
//                   if (value == 0) continue;
//                }
// 	    writePhrasePair(ostr, p->first.c_str(), lang2_voc.word(pf->first), value, wvoc1, wvoc2);
// 	 }
//       }
   }



};

} // Portage


#endif
