/**
 * @author George Foster
 * @file word_align.cc  Implementation of the word alignment module used in
 * gen_phrase_tables.
 *
 *
 * COMMENTS:
 *
 * The word alignment module used in gen_phrase_tables.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <iostream>
#include <numeric>
#include "str_utils.h"
#include "gfstats.h"
#include "quick_set.h"
#include "word_align.h"

using namespace Portage;

// is intersection between two sets of integers non-empty?

static bool intersectionNotNull(QuickSet& qs, vector<Uint>& vs)
{
   for (Uint i = 0; i < vs.size(); ++i)
      if (qs.find(vs[i]) != qs.size())
         return true;
   return false;
}

void WordAligner::close(vector< vector<Uint> >& sets1, vector< vector<Uint> >& csets)
{
   csets.clear();

   // a list of the L1 words that still need to be processed
   vector<Uint> todo(sets1.size());
   for (Uint i = 0; i < sets1.size(); ++i)
      todo[i] = i;

   QuickSet s1, s2;             // current L1, L2 merged sets

   // Do until all L1 words have been merged

   while (!todo.empty()) {

      s1.clear();
      s2.clear();
      Uint orig_s1_size;

      // Pick up the first set of L2 words that is left and grow it by
      // repeatedly sweeping over the other sets until no further merges are
      // possible.
      do {
         orig_s1_size = s1.size();
         for (Uint i = 0; i < todo.size();) {
            Uint w1 = todo[i];
            if (s1.empty() || intersectionNotNull(s2, sets1[w1])) {
               for (Uint j = 0; j < sets1[w1].size(); ++j)
                  s2.insert(sets1[w1][j]);
               s1.insert(w1);
               todo.erase(todo.begin()+i);
            } else
               ++i;
         }
      } while(s1.size() > orig_s1_size);

      // Remember current L1 cset, and put closure into all corresponding L2
      // sets. 

      csets.push_back(s1.contents());
      for (Uint i = 0; i < s1.contents().size(); ++i)
         sets1[s1.contents()[i]] = s2.contents();
   }

}

/*----------------------------------------------------------------------------
  WordAlignerFactory
  --------------------------------------------------------------------------*/

/**
 * This array describes all known aligner classes. Add new classes here, but
 * leave the NULL line at the end.
 */
WordAlignerFactory::TInfo WordAlignerFactory::tinfos[] = {
   {
      DCon<IBMOchAligner>::create,
      "IBMOchAligner", "[-2|-1|1|2|3][exclude]\n\
     Standard Och algorithm (better to use exclude with 1 or 2):\n\
     -1 = forward IBM alignment only, -2 = reverse IBM alignment only,\n\
     1 = use intersection only, 2 = expand to connected points in union, \n\
     3 = try to align all words [3].\n\
     exclude = exclude unlinked words from phrases [include them]"
   },
   {
      DCon<IBMDiagAligner>::create,
      "IBMDiagAligner", "[2|3][exclude]\n\
     Variant on standard Och algorithm (better to use exclude with 1 or 2):\n\
     2 = expand to connected points in union, 3 = try to align all words [3].\n\
     exclude = exclude unlinked words from phrases [include them]"
   },
   {
      DCon<IBMScoreAligner>::create,
      "IBMScoreAligner", "\n\
     experimental..."
   },
   {
      DCon<CartesianAligner>::create,
      "CartesianAligner", "\n\
     Align with no correspondences, so all possible phrases get added.",
   },
   {
      DCon<IdentityAligner>::create,
      "IdentityAligner", "\n\
     Align word for word sentence 1 to sentence 2.",
   },
   {
      DCon<PosteriorAligner>::create,
      "PosteriorAligner", "[delta [exclude-threshold]]\n\
     Align using posterior decoding, i.e., multiplying posteriors from the\n\
     two directorional models.  Delta is the keep threshold: any link with a\n\
     posterior prob product >= delta is kept in the final alignment. [.1]\n\
     Any word with all posterior prob products < Null-threshold is excluded\n\
     from phrases [0.0001]"
   },
   {
      DCon<HybridPostAligner>::create,
      "HybridPostAligner", "[pct]\n\
     Hybrid between IBMOchAligner and PosteriorAligner: start with intersection\n\
     of one-way IBM alignments, then add connected links in order of decreasing\n\
     posterior probabilities, stopping when all words are connected or the link\n\
     score gets too low. Next assign remaining unlinked words their best\n\
     translations, provided the posterior scores of these translations are high\n\
     enough. Words without such translations are left unlinked (not explicitly\n\
     null aligned). The number of top-scoring links to consider valid is \n\
     <pct>*s, where s is the length of the longer of the two input sentences\n\
     [1.0]"
   },
   {
      DCon<ExternalAligner>::create,
      "ExternalAligner", "file\n\
     Read external symmetrized alignments in Moses (sri) format: a list of\n\
     links on each line, with each link in the format srcpos-tgtpos, giving\n\
     0-based word positions."
   },
   {
      DCon<InternalAligner>::create,
      "InternalAligner", "\n\
     For programmatic use only - do not use via the command line."
   },

   {NULL, "", ""}
};

WordAlignerFactory::WordAlignerFactory(IBM1* ibm_lang2_given_lang1,
                                       IBM1* ibm_lang1_given_lang2,
                                       Uint verbose, bool twist, bool addSingleWords,
                                       bool allow_linkless_pairs)
   : ibm_lang2_given_lang1(ibm_lang2_given_lang1)
   , ibm_lang1_given_lang2(ibm_lang1_given_lang2)
   , file_lang2_given_lang1(NULL)
   , file_lang1_given_lang2(NULL)
   , verbose(verbose)
   , twist(twist)
   , addSingleWords(addSingleWords)
   , allow_linkless_pairs(allow_linkless_pairs)
{
}

WordAlignerFactory::WordAlignerFactory(GizaAlignmentFile* file_lang2_given_lang1,
                                       GizaAlignmentFile* file_lang1_given_lang2,
                                       Uint verbose, bool twist, bool addSingleWords,
                                       bool allow_linkless_pairs)
   : ibm_lang2_given_lang1(NULL)
   , ibm_lang1_given_lang2(NULL)
   , file_lang2_given_lang1(file_lang2_given_lang1)
   , file_lang1_given_lang2(file_lang1_given_lang2)
   , verbose(verbose)
   , twist(twist)
   , addSingleWords(addSingleWords)
   , allow_linkless_pairs(allow_linkless_pairs)
{
}

WordAligner* WordAlignerFactory::createAligner(const string& tname,
                                               const string& args, bool fail)
{
   for (Uint i = 0; tinfos[i].pf; ++i)
      if (tname == tinfos[i].tname)
         return tinfos[i].pf(*this, args);
   if (fail)
      error(ETFatal, "unknown class name: " + tname);
   return NULL;
}

string WordAlignerFactory::help() {
   string h;
   for (Uint i = 0; tinfos[i].pf; ++i)
      h += tinfos[i].tname + " " + tinfos[i].help + "\n\n";
   return h;
}

string WordAlignerFactory::help(const string& tname) {
   for (Uint i = 0; tinfos[i].pf; ++i)
      if (tname == tinfos[i].tname)
         return tinfos[i].help;
   return "";
}

void WordAlignerFactory::showAlignment(const vector<string>& toks1,
                                       const vector<string>& toks2,
                                       const vector< vector<Uint> >& sets1)
{
   bool exclusions_exist = false;
   for (Uint i = 0; i < toks1.size(); ++i)
      for (Uint j = 0; j < sets1[i].size(); ++j)
         if (sets1[i][j] < toks2.size())
            cerr << toks1[i] << "/" << toks2[sets1[i][j]] << " ";
         else
            exclusions_exist = true;
   cerr << endl;

   if (exclusions_exist || sets1.size() > toks1.size()) {
      cerr << "EXCLUDED: ";
      for (Uint i = 0; i < toks1.size(); ++i)
         for (Uint j = 0; j < sets1[i].size(); ++j)
            if (sets1[i][j] == toks2.size())
               cerr << i << " ";
      cerr << "/ ";
      if (sets1.size() > toks1.size())
         for (Uint i = 0; i < sets1.back().size(); ++i)
            cerr << sets1.back()[i] << " ";
      cerr << endl;
   }
}

/*----------------------------------------------------------------------------
  WordAlignerStats
  --------------------------------------------------------------------------*/

void WordAlignerStats::reset() {
   total_count = 0;
   link_ratio_sum = 0;
   link_ratio_sum2 = 0;
   exclude_ratio_l1_sum = 0;
   exclude_ratio_l1_sum2 = 0;
   exclude_ratio_l2_sum = 0;
   exclude_ratio_l2_sum2 = 0;
   null_ratio_l1_sum = 0;
   null_ratio_l1_sum2 = 0;
   null_ratio_l2_sum = 0;
   null_ratio_l2_sum2 = 0;
}

void WordAlignerStats::tally(const vector<vector<Uint> >& sets1,
                             Uint l1_length, Uint l2_length) {
   ++total_count;

   if ( l1_length == 0 || l2_length == 0 )
      return;

   Uint link_count(0);
   Uint l1_exclude_count(0);
   Uint l1_null_count(0);
   bool l2_covered[l2_length];
   fill_n(&(l2_covered[0]), l2_length, false);

   for ( Uint i = 0; i < l1_length; ++i ) {
      if (sets1[i].empty()) {
         ++l1_null_count;
      } else {
         link_count += sets1[i].size();
         for ( vector<Uint>::const_iterator j_it(sets1[i].begin()),
                                            j_end(sets1[i].end());
               j_it != j_end; ++j_it ) {
            if ( *j_it < l2_length )
               l2_covered[*j_it] = true;
            else if ( *j_it == l2_length ) {
               assert(sets1[i].size() == 1);
               ++l1_exclude_count;
               --link_count;
            }
         }
      }
   }
   if ( sets1.size() > l1_length ) {
      for ( vector<Uint>::const_iterator j_it(sets1[l1_length].begin()),
                                         j_end(sets1[l1_length].end());
            j_it != j_end; ++j_it )
         if ( *j_it < l2_length )
            l2_covered[*j_it] = true;
   }

   Uint l2_null_count(0);
   for ( Uint j = 0; j < l2_length; ++j )
      if ( !l2_covered[j] )
         ++l2_null_count;

   const Uint l2_exclude_count =
      ((sets1.size() > l1_length) ? sets1[l1_length].size() : 0);

   const double link_ratio = double(link_count) / sqrt(l1_length * l2_length);
   link_ratio_sum += link_ratio;
   link_ratio_sum2 += link_ratio * link_ratio;

   const double exclude_ratio_l1 = double(l1_exclude_count) / l1_length;
   exclude_ratio_l1_sum += exclude_ratio_l1;
   exclude_ratio_l1_sum2 += exclude_ratio_l1 * exclude_ratio_l1;

   const double exclude_ratio_l2 = double(l2_exclude_count) / l2_length;
   exclude_ratio_l2_sum += exclude_ratio_l2;
   exclude_ratio_l2_sum2 += exclude_ratio_l2 * exclude_ratio_l2;

   const double null_ratio_l1 = double(l1_null_count) / l1_length;
   null_ratio_l1_sum += null_ratio_l1;
   null_ratio_l1_sum2 += null_ratio_l1 * null_ratio_l1;

   const double null_ratio_l2 = double(l2_null_count) / l2_length;
   null_ratio_l2_sum += null_ratio_l2;
   null_ratio_l2_sum2 += null_ratio_l2 * null_ratio_l2;
} // tally()

void WordAlignerStats::display(const string& l1, const string& l2,
                               ostream& os) {
   os << endl << "Stats: " << total_count << " " << l1 << "/" << l2
      << " sentence pairs processed."
      << endl
      << "avg(link count / sqrt(" << l1 << " length * " << l2 << " length)) = "
      << _mean(link_ratio_sum, total_count)
      << " stdev = " << _sdev(link_ratio_sum, link_ratio_sum2, total_count)
      << endl
      << "avg(" << l1 << " null count / " << l1 << " length) = "
      << _mean(null_ratio_l1_sum, total_count) << " stdev = "
      << _sdev(null_ratio_l1_sum, null_ratio_l1_sum2, total_count)
      << endl
      << "avg(" << l1 << " exclude count / " << l1 << " length) = "
      << _mean(exclude_ratio_l1_sum, total_count) << " stdev = "
      << _sdev(exclude_ratio_l1_sum, exclude_ratio_l1_sum2, total_count)
      << endl
      << "avg(" << l2 << " null count / " << l2 << " length) = "
      << _mean(null_ratio_l2_sum, total_count) << " stdev = "
      << _sdev(null_ratio_l2_sum, null_ratio_l2_sum2, total_count)
      << endl
      << "avg(" << l2 << " exclude count / " << l2 << " length) = "
      << _mean(exclude_ratio_l2_sum, total_count) << " stdev = "
      << _sdev(exclude_ratio_l2_sum, exclude_ratio_l2_sum2, total_count)
      << endl << endl;
}



/*----------------------------------------------------------------------------
  IBMOchAligner
  --------------------------------------------------------------------------*/

/**
 * Explicitly markup unlinked words in a given alignment as non-aligned according
 * to the definition used for WordAligner::align().
 * @param sets1 sets of connections for L1.
 * @param connected2 number of connections (or boolean) for L2.
 */
template<class T>
void removeUnlinkedWords(vector< vector<Uint> >& sets1, const vector<T>& connected2)
{
   for (Uint i = 0; i < sets1.size(); ++i)
      if (sets1[i].empty())
         sets1[i].push_back(connected2.size());

   vector<Uint> exs;            // set of NULL-aligned L2 words
   for (Uint j = 0; j < connected2.size(); ++j)
      if (!connected2[j])
         exs.push_back(j);
   if (!exs.empty())
      sets1.push_back(exs);
}

/*----------------------------------------------------------------------------
  IBMOchAligner
  --------------------------------------------------------------------------*/

IBMOchAligner::IBMOchAligner(WordAlignerFactory& factory,
                             const string& args) :
   factory(factory)
{
   aligner_lang1_given_lang2 = factory.getAlignerLang1GivenLang2();
   aligner_lang2_given_lang1 = factory.getAlignerLang2GivenLang1();

   strategy = 3;                // try to align all words
   exclude = false;             // don't exclude unlinked words from phrases

   vector<string> toks;
   split(args, toks);
   for (Uint i = 0; i < toks.size(); ++i) {
      if (toks[i] == "-1")
         strategy = -1;
      else if (toks[i] == "-2")
         strategy = -2;
      else if (toks[i] == "1")
         strategy = 1;
      else if (toks[i] == "2")
         strategy = 2;
      else if (toks[i] == "3")
         strategy = 3;
      else if (toks[i] == "exclude")
         exclude = true;
      else {
         bad_constructor_argument = true;
         error(ETFatal, "invalid argument to IBMOchAligner: %s", toks[i].c_str());
      }
   }
}

double IBMOchAligner::align(const vector<string>& toks1, const vector<string>& toks2,
                            vector< vector<Uint> >& sets1) {
   al1.clear();
   al2.clear();

   if (aligner_lang2_given_lang1)
      aligner_lang2_given_lang1->align(toks1, toks2, al2, factory.getTwist());
   if (aligner_lang1_given_lang2)
      aligner_lang1_given_lang2->align(toks2, toks1, al1, factory.getTwist());

   if (factory.getVerbose() > 1) {
      showAlignment(toks2, toks1, al1);
      showAlignment(toks1, toks2, al2);
   }

   sets1.resize(toks1.size());
   connected2.assign(toks2.size(), false);
   new_pairs.clear();

   if (strategy == -1) {
      for (Uint j = 0; j < al2.size(); ++j)
         if (al2[j] < toks1.size())
            sets1[al2[j]].push_back(j);
   } else if (strategy == -2) {
      for (Uint i = 0; i < al1.size(); ++i)
         if (al1[i] < toks2.size())
            sets1[i].push_back(al1[i]);
   } else { // strategy > 0

      // initialize to intersection
      for (Uint i = 0; i < al1.size(); ++i) {
         sets1[i].clear();
         if (al1[i] < al2.size() && al2[al1[i]] == i) {
            sets1[i].push_back(al1[i]);
            connected2[al1[i]] = true;
            new_pairs.push_back(i); new_pairs.push_back(al1[i]);
         }
      }

      // expand into union via connected points
      if (strategy > 1) {
         for (Uint pi = 0; pi+1 < new_pairs.size(); pi += 2) {
            int ii(new_pairs[pi]), jj(new_pairs[pi+1]);
            for (int j = -1; j <= 1; ++j) {
               for (int i = -1; i <= 1; ++i) {
                  if (i == 0 && j == 0) continue;
                  if (addTest(ii+i, jj+j, sets1)) {
                     new_pairs.push_back(ii+i);
                     new_pairs.push_back(jj+j);
                  }
               }
            }
         }
      }

      // link any remaining unlinked words that aren't null-aligned
      if (strategy > 2) {
         for (Uint i = 0; i < sets1.size(); ++i) {
            if (sets1[i].empty() && al1[i] < al2.size()) {
               sets1[i].push_back(al1[i]);
               connected2[al1[i]] = true;
               new_pairs.push_back(i);
               new_pairs.push_back(al1[i]);
            }
         }
         for (Uint j = 0; j < connected2.size(); ++j) {
            if (!connected2[j] && al2[j] < al1.size()) {
               Uint i = al2[j];
               vector<Uint>::iterator p = lower_bound(sets1[i].begin(), sets1[i].end(), j);
               sets1[i].insert(p, j);
               connected2[j] = true;
               new_pairs.push_back(al2[j]);
               new_pairs.push_back(j);
            }
         }
      }
   }

   // mark unlinked words for exclusion
   if (exclude)
      removeUnlinkedWords(sets1, connected2);

   return 1.0;
}

bool IBMOchAligner::addTest(int ii, int jj, vector< vector<Uint> >& sets1)
{
   Uint i(ii), j(jj);
   if (ii < 0 || jj < 0 || i >= al1.size() || j >= al2.size())
      return false;

   if ((sets1[i].size() > 0 && connected2[j]) || // both words already connected
       (al1[i] != j && al2[j] != i))             // not in union
      return false;

   vector<Uint>::iterator p = lower_bound(sets1[i].begin(), sets1[i].end(), j);
   if (p != sets1[i].end() && *p == j)
      return false;             // already in sets1      if

   sets1[i].insert(p, jj);
   connected2[j] = true;
   return true;
}

void IBMOchAligner::showAlignment(const vector<string>& toks1,
                                  const vector<string>& toks2,
                                  const vector<Uint>& al2) {
   for (Uint i = 0; i < toks2.size(); ++i) {
      string t = al2[i] < toks1.size() ? toks1[al2[i]] : "NONE";
      cerr << toks2[i] << "/" << t << "(" << al2[i] << ")" << " ";
   }
   cerr << endl;
}

/*----------------------------------------------------------------------------
  IBMDiagAligner
  --------------------------------------------------------------------------*/

IBMDiagAligner::IBMDiagAligner(WordAlignerFactory& factory,
                               const string& args) :
   IBMOchAligner(factory, args)
{
   if (strategy < 1 || strategy > 3)
      error(ETFatal, "strategy %d not valid for IBMDiagAligner", strategy);
}

double IBMDiagAligner::align(const vector<string>& toks1, const vector<string>& toks2,
                            vector< vector<Uint> >& sets1) 
{
   al1.clear();
   al2.clear();

   if (aligner_lang2_given_lang1)
      aligner_lang2_given_lang1->align(toks1, toks2, al2, factory.getTwist());
   if (aligner_lang1_given_lang2)
      aligner_lang1_given_lang2->align(toks2, toks1, al1, factory.getTwist());

   if (factory.getVerbose() > 1) {
      showAlignment(toks2, toks1, al1);
      showAlignment(toks1, toks2, al2);
   }

   sets1.resize(toks1.size());
   connected2.assign(toks2.size(), false);
   new_pairs.clear();

   // initialize to intersection, as in IBMOchAligner
   for (Uint i = 0; i < al1.size(); ++i) {
      sets1[i].clear();
      if (al1[i] < al2.size() && al2[al1[i]] == i) {
         sets1[i].push_back(al1[i]);
         connected2[al1[i]] = true;
         new_pairs.push_back(i); new_pairs.push_back(al1[i]);
      }
   }

   // expand into union
   if (strategy > 1) {

      Uint start = 0;
      do {

	 // starting at link <start>, add strictly diagonal links, continuing
	 // until no more can be added
	 for (Uint pi = start; pi+1 < new_pairs.size(); pi += 2) {
	    int ii(new_pairs[pi]), jj(new_pairs[pi+1]);
	    if (addTest(ii-1, jj-1, sets1)) {
	       new_pairs.push_back(ii-1); 
               new_pairs.push_back(jj-1);
            }
	    if (addTest(ii+1, jj+1, sets1)) {
	       new_pairs.push_back(ii+1); 
               new_pairs.push_back(jj+1);
            }
	 }

	 // add "v-connected points" wherever possible, but process existing
	 // set of links only once (ie, don't operate on new links added by
	 // the current pass, as is done for diagonal links)
	 start = new_pairs.size();
	 for (Uint pi = 0; pi+1 < start; pi += 2) {
	    int ii(new_pairs[pi]), jj(new_pairs[pi+1]);
	    if (addTest(ii, jj-1, sets1)) {
	       new_pairs.push_back(ii); 
               new_pairs.push_back(jj-1);
            }
	    if (addTest(ii-1, jj, sets1)) {
	       new_pairs.push_back(ii-1); 
               new_pairs.push_back(jj);
            }
	    if (addTest(ii+1, jj, sets1)) {
	       new_pairs.push_back(ii+1); 
               new_pairs.push_back(jj);
            }
	    if (addTest(ii, jj+1, sets1)) {
	       new_pairs.push_back(ii); 
               new_pairs.push_back(jj+1);
            }
	 }

      } while (new_pairs.size() > start);
   }

   // link any remaining unlinked words that aren't null-aligned, as in IBMOchAligner
   if (strategy > 2) {
      for (Uint i = 0; i < sets1.size(); ++i) {
         if (sets1[i].empty() && al1[i] < al2.size()) {
            sets1[i].push_back(al1[i]);
            connected2[al1[i]] = true;
            new_pairs.push_back(i); 
            new_pairs.push_back(al1[i]);
         }
      }
      for (Uint j = 0; j < connected2.size(); ++j) {
         if (!connected2[j] && al2[j] < al1.size()) {
            Uint i = al2[j];
            vector<Uint>::iterator p = lower_bound(sets1[i].begin(), sets1[i].end(), j);
            sets1[i].insert(p, j);
            connected2[j] = true;
            new_pairs.push_back(al2[j]); 
            new_pairs.push_back(j);
         }
      }
   }

   
   // mark unlinked words for exclusion
   if (exclude)
      removeUnlinkedWords(sets1, connected2);

   return 1.0;
}

/*----------------------------------------------------------------------------
  IBMScoreAligner
  --------------------------------------------------------------------------*/

IBMScoreAligner::IBMScoreAligner(WordAlignerFactory& factory, const string& args) :
   factory(factory)
{
   ibm_lang1_given_lang2 = factory.getIBMLang1GivenLang2();
   ibm_lang2_given_lang1 = factory.getIBMLang2GivenLang1();

   if (!(ibm_lang1_given_lang2 && ibm_lang2_given_lang1))
     error(ETFatal, "IBMScoreAligner: Need IBM models to create");

   norm1 = false;
   norm2 = true;

   thresh = 0.0001;
}

void IBMScoreAligner::normalize(vector<double>& v)
{
   double sum = accumulate(v.begin(), v.end(), 0.0);
   if (sum != 0.0)
      for (vector<double>::iterator p = v.begin(); p != v.end(); ++p)
         *p /= sum;
}

double IBMScoreAligner::align(const vector<string>& toks1,
                              const vector<string>& toks2,
                              vector< vector<Uint> >& sets1)
{
   bool imp_L12 = ibm_lang1_given_lang2->useImplicitNulls;
   bool imp_L21 = ibm_lang2_given_lang1->useImplicitNulls;
   ibm_lang1_given_lang2->useImplicitNulls = false;
   ibm_lang2_given_lang1->useImplicitNulls = false;

   // build score matrix

   score_matrix.resize(toks1.size());
   for (Uint i = 0; i < toks1.size(); ++i) {
      score_matrix[i].resize(toks2.size());
      (void) ibm_lang1_given_lang2->pr(toks2, toks1[i], i, toks1.size(), &score_matrix[i]);
      if (norm1) normalize(score_matrix[i]);
   }

   links.clear();
   col.resize(toks1.size());
   for (Uint i = 0; i < toks2.size(); ++i) {
      (void) ibm_lang2_given_lang1->pr(toks1, toks2[i], i, toks2.size(), &col);
      if (norm2) normalize(col);
      for (Uint j = 0; j < toks1.size(); ++j) {
         score_matrix[j][i] *= -col[j];
         links.push_back(make_pair(score_matrix[j][i], make_pair(j,i)));
      }
   }
   ibm_lang1_given_lang2->useImplicitNulls = imp_L12;
   ibm_lang2_given_lang1->useImplicitNulls = imp_L21;

   // init for deriving word alignment from matrix

   sort(links.begin(), links.end());

   // Uint num_words_aligned = 0;
   sets1.assign(toks1.size(), vector<Uint>());
   connections2.assign(toks2.size(), 0);
   double cutoff = 0.0;
   if (links.size())
      cutoff = links[0].first * thresh;

   // pass 1: 1-1 connections

   for (Uint i = 0; i < links.size(); ++i) {
      if (links[i].first >= cutoff)
         break;
      pair<Uint,Uint>& p = links[i].second;
      if (sets1[p.first].empty() && connections2[p.second] == 0) {
         sets1[p.first].push_back(p.second);
         ++connections2[p.second];
         if (factory.getVerbose() > 1)  showLink(links[i], toks1, toks2);
      }
   }

   // pass 2: non-connected words

   for (Uint i = 0; i < links.size(); ++i) {
      if (links[i].first >= cutoff)
         break;
      pair<Uint,Uint>& p = links[i].second;
      if (sets1[p.first].empty() || connections2[p.second] == 0) {
         sets1[p.first].push_back(p.second);
         if (connections2[p.second] == 0)
            ++connections2[p.second];
         if (factory.getVerbose() > 1)  showLink(links[i], toks1, toks2);
      }
   }

   if (factory.getVerbose() > 1)
      cerr << endl;

   removeUnlinkedWords(sets1, connections2);

   return 1.0;
}

void IBMScoreAligner::showLink(const pair< double,pair<Uint,Uint> >& link,
                               const vector<string>& toks1, const vector<string>& toks2)
{
   cerr << toks1[link.second.first] << '(' << link.second.first << ")/"
        << toks2[link.second.second] << '(' << link.second.second << ")="
        << link.first << " ";
}


/*----------------------------------------------------------------------------
  CartesianAligner
  --------------------------------------------------------------------------*/

double CartesianAligner::align(const vector<string>& toks1, const vector<string>& toks2,
                               vector< vector<Uint> >& sets1)
{
   sets1.clear();
   sets1.resize(toks1.size());
   return 1.0;
}


/*----------------------------------------------------------------------------
  IdentityAligner
  --------------------------------------------------------------------------*/

double IdentityAligner::align(const vector<string>& toks1, const vector<string>& toks2,
                               vector< vector<Uint> >& sets1)
{
   sets1.clear();
   const Uint s1 = toks1.size();
   const Uint s2 = toks2.size();
   // If sentence 2 is longer than sentence 1 we will need to align some word
   // of sentence 2 to the token one passed the end of sentence 1.
   sets1.resize(s2 > s1 ? s1+1 : s1);
   Uint i = 0;

   // Marking all aligned words which will result in a "Identity matrix".
   for (; i<min(s1, s2); ++i)
        sets1[i].push_back(i);

   // If sentence 1 is longer than sentence 2, we will set any unaligned word
   // from sentence 1 to null which is defined to be the token one passed the
   // end of sentence 2.
   for (; i<s1; ++i)
        sets1[i].push_back(toks2.size());

   // If sentence 2 is longer than sentence 1, we will set any unaligned word
   // from sentence 2 to null which is defined to be the token one passed the
   // end of sentence 1 thus we pool all unaligned words of sentence 2 to
   // sentence1[sentence1.length].
   if (s2 > s1) {
      for (Uint j(s1); j<s2; ++j)
           sets1[s1].push_back(j);
   }

   return 1.0;
}


/*----------------------------------------------------------------------------
  PosteriorAligner
  --------------------------------------------------------------------------*/

PosteriorAligner::PosteriorAligner(WordAlignerFactory& factory,
                                   const string& args)
   : ibm_lang1_given_lang2(factory.getIBMLang1GivenLang2())
   , ibm_lang2_given_lang1(factory.getIBMLang2GivenLang1())
   , verbose(factory.getVerbose())
{
   if ( !ibm_lang1_given_lang2 || !ibm_lang2_given_lang1 )
      error(ETFatal, "PosteriorAligner requires two IBM1 (or subtype) models");

   const double default_delta = 0.1;
   const double default_exclude_threshold = 0.0001;
   delta = default_delta;
   exclude_threshold = default_exclude_threshold;

   vector<string> toks;
   split(args, toks);
   if ( toks.size() >= 1 ) {
      if ( ! conv(toks[0], delta) ) {
         bad_constructor_argument = 1;
         delta = default_delta;
         error(ETWarn, "Invalid delta for PosteriorAligner - can't convert "
                       "%s to a float - using %g",
                       toks[0].c_str(), default_delta);
      } else if ( delta < 0.0 || delta > 1.0 ) {
         bad_constructor_argument = 1;
         delta = default_delta;
         error(ETWarn, "Invalid delta for PosteriorAligner: %s - should "
                       "be between 0 and 1 - using .g.",
                       toks[0].c_str(), default_delta);
      }
   }
   if ( toks.size() >= 2 ) {
      if ( ! conv(toks[1], exclude_threshold) ) {
         bad_constructor_argument = 1;
         exclude_threshold = default_exclude_threshold;
         error(ETWarn, "Invalid exclude-threshold for PosteriorAligner - "
                       "can't convert %s to a float - using %g.",
                       toks[1].c_str(), default_exclude_threshold);
      } else if ( exclude_threshold < 0.0 || exclude_threshold > delta ) {
         bad_constructor_argument = 1;
         exclude_threshold = default_exclude_threshold;
         error(ETWarn, "Invalid exclude-threshold for PosteriorAligner: %s "
                       "- should be between 0 and delta - using %g.",
                       toks[1].c_str(), default_exclude_threshold);
      }
   }
   if ( toks.size() >= 3 ) {
      bad_constructor_argument = 1;
      error(ETWarn, "Too many arguments to PosteriorAligner - "
                    "expected at most 2.");
   }
}

double PosteriorAligner::align(const vector<string>& toks1,
                               const vector<string>& toks2,
                               vector< vector<Uint> >& sets1)
{
   bool imp_L12 = ibm_lang1_given_lang2->useImplicitNulls;
   bool imp_L21 = ibm_lang2_given_lang1->useImplicitNulls;
   ibm_lang1_given_lang2->useImplicitNulls = true;
   ibm_lang2_given_lang1->useImplicitNulls = true;

   ibm_lang1_given_lang2->linkPosteriors(toks2, toks1, posteriors_lang1_given_lang2);
   ibm_lang2_given_lang1->linkPosteriors(toks1, toks2, posteriors_lang2_given_lang1);

   ibm_lang1_given_lang2->useImplicitNulls = imp_L12;
   ibm_lang2_given_lang1->useImplicitNulls = imp_L21;

   sets1.assign(toks1.size(), vector<Uint>());
   bool connectable1[toks1.size()];
   fill_n(&(connectable1[0]), toks1.size(), false);
   bool connectable2[toks2.size()];
   fill_n(&(connectable2[0]), toks2.size(), false);

   if ( verbose > 1 ) {
      cerr << endl << "Posterior products:" << endl;
      for ( Uint j = 0; j < toks2.size(); ++j )
         fprintf(stderr, "%-8s ", toks2[j].substr(0,8).c_str());
      cerr << endl;
   }
   for ( Uint i = 0; i < toks1.size(); ++i ) {
      for ( Uint j = 0; j < toks2.size(); ++j ) {
         const double posterior = 
            posteriors_lang1_given_lang2[i][j] *
            posteriors_lang2_given_lang1[j][i];
         if ( posterior >= delta )
            sets1[i].push_back(j);
         if ( posterior >= exclude_threshold ) {
            connectable1[i] = true;
            connectable2[j] = true;
         }
         if ( verbose > 1 )
            fprintf(stderr, (posterior >= 0.0001 ? "%7.5f  " : "%7.2g  "), posterior);
      }
      if ( verbose > 1 ) cerr << toks1[i] << endl;
   }
   if ( verbose > 1 ) cerr << endl;

   // mark unconnectable L1 words for exclusion
   for ( Uint i = 0; i < toks1.size(); ++i ) {
      if ( !connectable1[i] ) {
         assert(sets1[i].empty());
         sets1[i].push_back(toks2.size());
      }
   }
   // mark unconnectable L2 words for exclusion
   vector<Uint> ex2;
   for ( Uint j = 0; j < toks2.size(); ++j )
      if ( !connectable2[j] )
         ex2.push_back(j);
   if ( !ex2.empty() )
      sets1.push_back(ex2);

   return 1.0;
}

/*----------------------------------------------------------------------------
  HybridPostAligner
  --------------------------------------------------------------------------*/

HybridPostAligner::HybridPostAligner(WordAlignerFactory& factory,
                                   const string& args) : 
   ibm_lang1_given_lang2(factory.getIBMLang1GivenLang2()),
   ibm_lang2_given_lang1(factory.getIBMLang2GivenLang1()),
   verbose(factory.getVerbose())
{
   if ( !ibm_lang1_given_lang2 || !ibm_lang2_given_lang1 )
      error(ETFatal, "HybridPostAligner requires two IBM1 (or subtype) models");

   pct = 1.0;

   vector<string> toks;
   split(args, toks);
   if ( toks.size() > 1 ) {
      bad_constructor_argument = 1;
      error(ETWarn, "Too many arguments to HybridPostAligner - "
                    "expected at most 1.");
   } else if (toks.size() == 1) {
      if (! conv(toks[0], pct) ) {
         bad_constructor_argument = 1;
         error(ETWarn, "Invalid pct for HybridPostAligner - "
               "can't convert %s to a float - using %g.",
               toks[0].c_str(), pct);
      }
   }
}

struct Cmp {

   const vector< vector<double> >& post_matrix;
   Cmp(vector< vector<double> >& post_matrix) : post_matrix(post_matrix) {}
   
   bool operator()(Uint ind1, Uint ind2) {
      Uint s = post_matrix.size() ? post_matrix[0].size() : 0;
      return s ? 
         post_matrix[ind1/s][ind1%s] > post_matrix[ind2/s][ind2%s] :
         false;
   }
};

double HybridPostAligner::align(const vector<string>& toks1,
                                const vector<string>& toks2,
                                vector< vector<Uint> >& sets1)
{
   // start with intersection of two viterbi alignments, as in IBMOchAligner

   al1.clear();
   al2.clear();

   ibm_lang2_given_lang1->align(toks1, toks2, al2, false);
   ibm_lang1_given_lang2->align(toks2, toks1, al1, false);

   connected1.assign(toks1.size(), false);
   connected2.assign(toks2.size(), false);
   
   links.resize(toks1.size());
   for (Uint i = 0; i < toks1.size(); ++i) {
      links[i].assign(toks2.size(), false);
      if (al1[i] < al2.size() && al2[al1[i]] == i)
	 links[i][al1[i]] = connected1[i] = connected2[al1[i]] = true;
   }

   Uint num_connections1 = count(connected1.begin(), connected1.end(), true);
   Uint num_connections2 = count(connected2.begin(), connected2.end(), true);

   // construct matrix of posterior link probs, as in PosteriorAligner

   bool imp_L12 = ibm_lang1_given_lang2->useImplicitNulls;
   bool imp_L21 = ibm_lang2_given_lang1->useImplicitNulls;
   ibm_lang1_given_lang2->useImplicitNulls = true;
   ibm_lang2_given_lang1->useImplicitNulls = true;
   ibm_lang1_given_lang2->linkPosteriors(toks2, toks1, posteriors_lang1_given_lang2);
   ibm_lang2_given_lang1->linkPosteriors(toks1, toks2, posteriors_lang2_given_lang1);
   ibm_lang1_given_lang2->useImplicitNulls = imp_L12;
   ibm_lang2_given_lang1->useImplicitNulls = imp_L21;

   posteriors.resize(toks1.size());
   ranks.resize(toks1.size());
   for (Uint i = 0; i < toks1.size(); ++i) {
      posteriors[i].resize(toks2.size());
      ranks[i].resize(toks2.size());
      for (Uint j = 0; j < toks2.size(); ++j) {
	 posteriors[i][j] = 
	    posteriors_lang2_given_lang1[j][i] * posteriors_lang1_given_lang2[i][j];
      }
   }

   // sort all link indexes by decreasing score (index = row * row_size + col),
   // and put the rank of each link into the ranks matrix.

   link_list.resize(toks1.size() * toks2.size());
   for (Uint i = 0; i < link_list.size(); ++i)
      link_list[i] = i;
   sort(link_list.begin(), link_list.end(), Cmp(posteriors));
   for (Uint i = 0; i < link_list.size(); ++i)
      ranks[link_list[i]/toks2.size()][link_list[i]%toks2.size()] = i;

   Uint low_rank = max(toks1.size(), toks2.size()) * pct;
   if (low_rank > link_list.size())
      low_rank = link_list.size();

   if (verbose > 2) {
      cerr << "RANKS:" << endl;
      for (Uint i = 0; i < ranks.size(); ++i) {
         for (Uint j = 0; j < ranks.size(); ++j)
            cerr << ranks[i][j] << ' ';
         cerr << endl;
      }
      cerr << "LINKS:" << endl;
      for (Uint i = 0; i < links.size(); ++i) {
         for (Uint j = 0; j < links.size(); ++j)
            cerr << links[i][j] << ' ';
         cerr << endl;
      }
   }

   // Walk the list in descending order, adding links iff they are adjacent to
   // an existing link. Stop when all words are connected, or we're out of links.

   Uint next = 0;

   for (Uint ind = 0; ind < low_rank; ind = next) {

      Uint i = link_list[ind] / toks2.size();
      Uint j = link_list[ind] % toks2.size();
      next = ind + 1;

      bool add = false;
      Uint high_rank = next;

      // try to add this link if needed and not already there

      if (!links[i][j] && (!connected1[i] || !connected2[j])) { 

         for (Uint ii = i == 0 ? 0 : i-1; ii < min(i+2, toks1.size()); ++ii) {
            for (Uint jj = j == 0 ? 0 : j-1; jj < min(j+2, toks2.size()); ++jj) {
               if (ii == i && jj == j) continue;
               if (links[ii][jj]) 
                  add = true;	// found a linked neighbour
               else if (ranks[ii][jj] < high_rank && (!connected1[ii] || !connected2[jj]))
                  high_rank = ranks[ii][jj]; // higher-ranking unlinked neighbour
            }
         }
         if (add) {
            if (!connected1[i]) ++num_connections1;
            if (!connected2[j]) ++num_connections2;
            connected1[i] = connected2[j] = true;
            links[i][j] = true;
            next = high_rank;   // try to link highest-ranking unlinked neighbour
         }
         
         if (num_connections1 == toks1.size() && num_connections2 == toks2.size())
            break;
      }
      if (verbose > 2) {
         cerr << "rank " << ind << " (" << i << "," << j << ")" 
              << " done, add=" << add << ", next=" << next 
              << ", nc1=" << num_connections1
              << ", nc2=" << num_connections2 
              << endl;
      }
   }

   // Now try to connect any remaining unlinked words. To preserve symmetry,
   // this deliberately avoids recording links in the other dimension. For
   // example, if w1 is assigned w2 as a translation, then w2 should be marked
   // as connected if it isn't already. But we avoid this so that w2 can be
   // assigned its best translation, which may not be w1.

   for (Uint i = 0; i < connected1.size(); ++i)
      if (!connected1[i]) {
	 Uint best_j = toks2.size();
	 for (Uint j = 0; j < toks2.size(); ++j)
	    if (ranks[i][j] < low_rank)
	       best_j = j;
	 if (best_j != toks2.size()) {
	    links[i][best_j] = true;
	    connected1[i] = true;
	    ++num_connections1;
	 }
      }
   
   for (Uint j = 0; j < connected2.size(); ++j) 
      if (!connected2[j]) {
	 Uint best_i = toks1.size();
	 for (Uint i = 0; i < toks1.size(); ++i)
	    if (ranks[i][j] < low_rank)
	       best_i = i;
	 if (best_i != toks1.size()) {
	    links[best_i][j] = true;
	    connected2[j] = true;
	    ++num_connections2;
	 }
      }

   if (verbose > 2) {
      cerr << "DONE:" << endl;
      cerr << "nc1=" << num_connections1 << ", nc2=" << num_connections2 << endl;
      cerr << "LINKS:" << endl;
      for (Uint i = 0; i < links.size(); ++i) {
         for (Uint j = 0; j < links.size(); ++j)
            cerr << links[i][j] << ' ';
         cerr << endl;
      }
   }

   // put the results into sets1 for return (no null connections)

   sets1.assign(toks1.size(), vector<Uint>());
   for (Uint i = 0; i < toks1.size(); ++i) {
      for (Uint j = 0; j < toks2.size(); ++j) {
         if (links[i][j])
            sets1[i].push_back(j);
      }
   }

   return 1.0;
}

ExternalAligner::ExternalAligner(WordAlignerFactory& factory, const string& args)
   : ext_file_name(args)
   , lineno(0)
{
   external_alignments = new iSafeMagicStream(args);
}

ExternalAligner::~ExternalAligner()
{
   if (external_alignments)
      delete external_alignments;
}

double ExternalAligner::align(const vector<string>& toks1,
		              const vector<string>& toks2,
                              vector< vector<Uint> >& sets1)
{
   if (!reader(*external_alignments, toks1, toks2, sets1))
      error(ETFatal, "No more alignments available in %s after line %u",
            ext_file_name.c_str(), lineno);
   ++lineno;
   return 1.0;
}
