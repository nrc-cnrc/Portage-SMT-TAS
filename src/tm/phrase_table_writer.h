/**
 * @author George Foster (split out from joint2cond_phrase_table.cc by Eric Joanis)
 * @file phrase_table_writer.h  Functions to write out a smoothed phrase tables.
 * $Id$
 * 
 * COMMENTS: 
 *
 * Functions to write out a smoothed phrase tables.  Would logically belong in
 * phrase_table.h, but can't be there because of the cross dependencies it
 * would create between phrase_table.h and phrase_table_smoother.h.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef PHRASE_TABLE_WRITE_H
#define PHRASE_TABLE_WRITE_H

#include "phrase_table.h"
#include "phrase_smoother.h"

using namespace Portage;

/**
 * Dump phrase pairs and the associated conditional distribution generated by a
 * given smoother to a stream, in standard format: phrase1 ||| phrase2 |||
 * p(phrase1|phrase2).
 * @param ofs stream to dump on
 * @param lang conditioning language: if 1, dump p(l2|l1), else the reverse
 * @param pt the phrase table on which the smoother is based
 * @param smoother the smoother used to generate conditional probs
 * @param verbose yackedy yack
 */
template <class T>
void dumpCondDistn(ostream& ofs, Portage::Uint lang, PhraseTableGen<T>& pt,
                   PhraseSmoother<T>& smoother, bool verbose = false)
{
   ofs.precision(9); // Enough to keep all the precision of a float
   string p1, p2;
   Uint total = 0, non_zero = 0;
   if (lang == 1)
      for ( typename PhraseTableGen<T>::iterator it = pt.begin();
            !it.equal(pt.end()); it.incr()) {
         double p = smoother.probLang2GivenLang1(it);
         if (p) {
            PhraseTableBase::writePhrasePair(ofs, it.getPhrase(2,p1).c_str(),
                                             it.getPhrase(1,p2).c_str(), p);
            ++non_zero;
         }
         ++total;
      }
   else
      for ( typename PhraseTableGen<T>::iterator it = pt.begin();
            !it.equal(pt.end()); it.incr()) {
         double p = smoother.probLang1GivenLang2(it);
         if (p) {
            PhraseTableBase::writePhrasePair(ofs, it.getPhrase(1,p1).c_str(),
                                             it.getPhrase(2,p2).c_str(), p);
            ++non_zero;
         }
         ++total;
      }
   ofs.flush();
   if (verbose) {
      cerr << "dumped conditional distn " << lang << ": "
           << non_zero << " non-zero probs, "
           << total << " phrase pairs" << endl;
   }
}

/**
 * Dump all conditional distributions generated by a given set of smoothers to
 * a stream. Assuming there are n smoothers, the format is: phrase1 ||| phrase2
 * ||| p_1(phrase1|phrase2) ... p_n(phrase1|phrase2) p_1(phrase2|phrase1)
 * ... p_n(phrase2|phrase1). In other words, a normal phrase pair, followed by
 * backward probabilities for all smoothers, followed by forward probabilities
 * for all smoothers. 
 * @param ofs stream to dump on
 * @param left_lang: the language whose phrases are written first; this also
 * affects the order in which probabilities are written, as described above
 * @param pt the phrase table on which the smoothers is based
 * @param smoothers the set of smoothers used to generate conditional probs
 * @param verbose yackedy yack
 */
template<class T>
void dumpMultiProb(ostream& ofs, Uint left_lang, PhraseTableGen<T>& pt,
		   vector< PhraseSmoother<T>* >& smoothers, bool verbose = false)
{
   ofs.precision(9); // Enough to keep all the precision of a float
   string p1, p2;
   Uint total = 0;
   vector<double> vals;

   for (typename PhraseTableGen<T>::iterator it = pt.begin(); !it.equal(pt.end()); it.incr()) {
      vals.clear();
      
      if (left_lang == 1) {
	 it.getPhrase(1, p1);
	 it.getPhrase(2, p2);
	 for (Uint i = 0; i < smoothers.size(); ++i)
	    vals.push_back(smoothers[i]->probLang1GivenLang2(it));
	 for (Uint i = 0; i < smoothers.size(); ++i)
	    vals.push_back(smoothers[i]->probLang2GivenLang1(it));
      } else {
	 it.getPhrase(2,p1);
	 it.getPhrase(1,p2);
 	 for (Uint i = 0; i < smoothers.size(); ++i)
 	    vals.push_back(smoothers[i]->probLang2GivenLang1(it));
  	 for (Uint i = 0; i < smoothers.size(); ++i)
  	    vals.push_back(smoothers[i]->probLang1GivenLang2(it));
      }
      if (find_if(vals.begin(), vals.end(), bind2nd(greater<double>(), 0.0)) != vals.end())
         PhraseTableBase::writePhrasePair(ofs, p1.c_str(), p2.c_str(), vals);
      ++total;
   }
   ofs.flush();
   if (verbose) {
      cerr << "dumped multi-prob distn: "
           << total << " phrase pairs" << endl;
   }
}

#endif // PHRASE_TABLE_WRITE_H

// vim:sw=3:
