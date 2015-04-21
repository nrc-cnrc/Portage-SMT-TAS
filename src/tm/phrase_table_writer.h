/**
 * @author George Foster (split out from joint2cond_phrase_table.cc by Eric Joanis)
 *    with reduced memory usage mods to by Darlene Stewart
 * @file phrase_table_writer.h  Functions to write out a smoothed phrase tables.
 *
 * $Id$
 *
 *
 * COMMENTS:
 *
 * Functions to write out a smoothed phrase tables.  Would logically belong in
 * phrase_table.h, but can't be there because of the cross dependencies it
 * would create between phrase_table.h and phrase_table_smoother.h.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef PHRASE_TABLE_WRITE_H
#define PHRASE_TABLE_WRITE_H

#include "phrase_table.h"
#include "phrase_smoother.h"

namespace Portage {

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
 * @param display_alignments if 0, don't display alignments; if 1, display only
 *                           the most frequent alignment (ties are broken
 *                           arbitrarily); if 2, display all alignments with
 *                           counts.
 * @param write_count  if true, write joint counts too
 * @param verbose yackedy yack
 */
template<class T>
void dumpMultiProb(ostream& ofs, Uint left_lang, PhraseTableGen<T>& pt,
                   vector< PhraseSmoother<T>* >& smoothers,
                   Uint display_alignments, bool write_count, bool write_smoother_state, bool verbose)
{
   Uint total = 0;

   for (typename PhraseTableGen<T>::iterator it = pt.begin(); it != pt.end(); ++it) {
      dumpMultiProb(ofs, left_lang, it, smoothers, display_alignments, write_count, write_smoother_state, verbose);
      ++total;
   }
   ofs.flush();
   if (verbose)
      cerr << "dumped multi-prob distn: " << total << " phrase pairs" << endl;
}

/**
 * Dump all conditional distributions generated for one phrase pair by a given 
 * set of smoothers to a stream.
 * Assuming there are n smoothers, the format is: phrase1 ||| phrase2
 * ||| p_1(phrase1|phrase2) ... p_n(phrase1|phrase2) p_1(phrase2|phrase1)
 * ... p_n(phrase2|phrase1). In other words, a normal phrase pair, followed by
 * backward probabilities for all smoothers, followed by forward probabilities
 * for all smoothers.
 * @param ofs stream to dump on
 * @param left_lang: the language whose phrases are written first; this also
 * affects the order in which probabilities are written, as described above
 * @param it the phrase table iterator of the phrase pair for which the 
 * conditional distributions are to be dumped
 * @param smoothers the set of smoothers used to generate conditional probabilities
 * @param display_alignments display no (0), top (1) or all (2) alignments.
 * @param write_count  if true, write joint counts too
 * @param verbose yackedy yack
 * @return true if non-zero probabilities were written; false otherwise.
 */
template<class T>
bool dumpMultiProb(ostream& ofs, Uint left_lang,
                   typename PhraseTableGen<T>::iterator& it,
                   vector< PhraseSmoother<T>* >& smoothers,
                   Uint display_alignments, bool write_count, bool write_smoother_state, bool verbose)
{
   // EJJ High precision is fine and all, but costs us in space in CPT files,
   // as well as in TPPT files, where the encoding is more expensive to keep
   // all those pointless bits around.
   //ofs.precision(9); // Enough to keep all the precision of a float
   string p1, p2;
   vector<double> vals;

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
   if (find_if(vals.begin(), vals.end(), bind2nd(greater<double>(), 0.0)) == vals.end())
      return false;

   const char* alignments = NULL;
   string alignments_s;
   if ( display_alignments ) {
      it.getAlignmentString(alignments_s, (left_lang != 1), (display_alignments == 1));
      alignments = alignments_s.c_str();
   }

   const char* extra_c = NULL;
   string extra;
   if (write_smoother_state) {
      for (Uint i = 0; i < smoothers.size(); ++i)
         extra += smoothers[i]->getInternalState(it);
      if (!extra.empty())
         extra_c = extra.c_str();
   }

   vector<double>* avals = NULL;
   const double count = write_count ? it.getJointFreq() : 0;
   PhraseTableBase::writePhrasePair(ofs, p1.c_str(), p2.c_str(), alignments,
                                    vals, write_count, count, avals, extra_c);
   return true;
}

} // namespace Portage

#endif // PHRASE_TABLE_WRITE_H

// vim:sw=3:
