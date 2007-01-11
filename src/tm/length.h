/**
 * @author Matthew Arnold
 * @file length.h  Obtain probabilities that a string od length A will generate a string of length B.
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */


#include <vector>
#include <ext/hash_map>
#include <string>

namespace Portage {

/// Obtain probabilities that a string od length A will generate a string of length B.
template <Uint MAX>
class LengthProb {
private:
   /// Matrix of lengths, corresponding to counts observed.
   vector<vector<Uint> > myLengths;
   vector<Uint> srcMargins;
   vector<Uint> tgtMargins;
   /// Total number of pairs observed.
   Uint myCount;
//   vector<Uint> fact;
   double recalc;

public:
   /// Constrictor. Initialise matrix to MAX x MAX.
   LengthProb()
      :myLengths(MAX, vector<Uint>(MAX, 0))
      ,srcMargins(MAX, 0)
      ,tgtMargins(MAX, 0)
      ,myCount(0)
//      ,fact(MAX)
   {
  // 	fact[0]=1;
//	for (Uint i = 1; i < MAX+1; ++i)
//	   fact[i] = i * fact[i-1];
   }

   /// Destructor.
   virtual ~LengthProb()
   {}

   /**
    * Add in src-tgt pair to list.
    * @param src source index
    * @param tgt target index
    */
   virtual void add(Uint src, Uint tgt) {
      myLengths[src > MAX ? MAX-1 : src-1][tgt > MAX ? MAX-1 : tgt-1]++;
      myCount++;
      srcMargins[src > MAX ? MAX-1 : src-1]++;
      tgtMargins[tgt > MAX ? MAX-1 : tgt-1]++;
   }


	//PROBABILITIES
   /** 
    * Calculate pr (srcL == src | tgtL == tgt).
    * @param src source index
    * @param tgt target index
    * @param SMOOTH smoothing value if sum == 0.
    * @return Returns pr (srcL == src | tgtL == tgt).
    */
   inline virtual double pr_sgt(Uint src, Uint tgt, double SMOOTH = 0.5) const {
      return (double)(sum(src, tgt)?sum(src, tgt):SMOOTH) / (double)sum_tgt(tgt);
   }

   /** 
    * Calculate pr (tgtL == tgt | srcL == src).
    * @param src source index
    * @param tgt target index
    * @param SMOOTH smoothing value if sum == 0.
    * @return Returns pr (tgtL == tgt | srcL == src).
    */
   inline virtual double pr_tgs(Uint src, Uint tgt, double SMOOTH = 0.5) const {
      return (double)(sum(src, tgt)?sum(src, tgt):SMOOTH) / (double)sum_src(src);
   }

   /** 
    * Calculate pr (srcL == src && tgtL == tgt).
    * @param src source index
    * @param tgt target index
    * @return Returns pr (srcL == src && tgtL == tgt).
    */
   inline virtual double pr(Uint src, Uint tgt) const {
      return (double)sum(src, tgt) / (double)total();
   }

   /**
    * Get the total number of pairs observed.
    * @return Returns the total number of pairs observed.
    */
   inline virtual Uint total() const {
      return myCount;
   }

   /**
    * Get sum (L(tgt)| L(src)).
    * @param src source index
    * @param tgt target index
    * @return Returns sum (L(tgt)| L(src)) = #lengths[src][tgt]
    *	= probability that the pair (src, tgt) will occur.
    */
   inline virtual Uint sum(Uint src, Uint tgt) const {
      return myLengths[src > MAX ? MAX-1 : src-1][tgt > MAX ? MAX-1 : tgt-1];
   }

   /**
    * Get marginal sum (srcL == src).
    * @param src source index
    * @return Returns marginal sum (srcL == src) = #src phrases of length src.
    */
   inline virtual Uint sum_src(Uint src) const {
      return srcMargins[src > MAX ? MAX-1 : src-1];
   }
        
   /**
    * Get marginal sum (tgtL == tgt).
    * @param tgt target index
    * @return Returns marginal sum (tgtL == tgt) = #tgt phrases of length tgt.
    */
   inline virtual Uint sum_tgt(Uint tgt) const {
      return tgtMargins[tgt > MAX ? MAX-1 : tgt-1];
   }

/*   //calculates the number of permutations of the words in toks
   Uint perm(const vector<string> & toks) {
      hash_map<string, Uint, StringHash> myToks;
      for (Uint i = 0; i < toks.size(); ++i)	
         myToks[toks[i]]++;
      Uint total = fact[toks.size()];
      for (hash_map<string, Uint, StringHash>::iterator it = myToks.begin(); it != myToks.end(); ++it)
         total /= fact[it->second];
      return total;
   }*/
     
};//end class

}
