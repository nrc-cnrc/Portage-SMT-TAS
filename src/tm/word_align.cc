/**
 * @author George Foster
 * @file word_align.cc  Implementation of the word alignment module used in gen_phrase_tables.
 * 
 * 
 * COMMENTS:
 *
 * The word alignment module used in gen_phrase_tables.
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */

#include <iostream>
#include <numeric>
#include <str_utils.h>
#include "word_align.h"

using namespace Portage;

/*--------------------------------------------------------------------------------
  WordAlignerFactory
  ------------------------------------------------------------------------------*/

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
      DCon<IBMScoreAligner>::create, 
      "IBMScoreAligner", "\n\
     experimental..."
   },
   {
      DCon<CartesianAligner>::create, 
      "CartesianAligner", "\n\
     Align with no correspondences, so all possible phrases get added.",
   },
   {NULL, "", ""}
};

WordAlignerFactory::WordAlignerFactory(IBM1* ibm_lang2_given_lang1, 
				       IBM1* ibm_lang1_given_lang2, 
                                       Uint verbose, bool twist, bool addSingleWords) :
   ibm_lang2_given_lang1(ibm_lang2_given_lang1),
   ibm_lang1_given_lang2(ibm_lang1_given_lang2),
   file_lang2_given_lang1(0),
   file_lang1_given_lang2(0),
   verbose(verbose), twist(twist), addSingleWords(addSingleWords)
{
}

WordAlignerFactory::WordAlignerFactory(GizaAlignmentFile* file_lang2_given_lang1, 
				       GizaAlignmentFile* file_lang1_given_lang2, 
				       Uint verbose, bool twist, bool addSingleWords) :
   ibm_lang2_given_lang1(0),
   ibm_lang1_given_lang2(0),
   file_lang2_given_lang1(file_lang2_given_lang1),
   file_lang1_given_lang2(file_lang1_given_lang2),
   verbose(verbose), twist(twist), addSingleWords(addSingleWords)
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
      h += tinfos[i].tname + " " + tinfos[i].help + "\n";
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

/*--------------------------------------------------------------------------------
  IBMOchAligner
  ------------------------------------------------------------------------------*/

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
      if (sets1[i].size() == 0)
	 sets1[i].push_back(connected2.size());

   vector<Uint> exs;		// set of NULL-aligned L2 words
   for (Uint j = 0; j < connected2.size(); ++j)
      if (!connected2[j])
	 exs.push_back(j);
   if (exs.size())
      sets1.push_back(exs);
}

/*--------------------------------------------------------------------------------
  IBMOchAligner
  ------------------------------------------------------------------------------*/

IBMOchAligner::IBMOchAligner(WordAlignerFactory& factory, 
					 const string& args) :
   factory(factory)
{
   aligner_lang1_given_lang2 = factory.getAlignerLang1GivenLang2();
   aligner_lang2_given_lang1 = factory.getAlignerLang2GivenLang1();

   strategy = 3;		// try to align all words
   exclude = false;		// don't exclude unlinked words from phrases

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
      else
	 error(ETWarn, "ignoring invalid argument to IBMOchAligner: ", toks[i].c_str());
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
              new_pairs.push_back(ii+i); new_pairs.push_back(jj+j);
            }
          }
        }
      }
    }

    // link any remaining unlinked words that aren't null-aligned
    if (strategy > 2) {
      for (Uint i = 0; i < sets1.size(); ++i) {
        if (sets1[i].size() == 0 && al1[i] < al2.size()) {
          sets1[i].push_back(al1[i]);
          connected2[al1[i]] = true;
          new_pairs.push_back(i); new_pairs.push_back(al1[i]);
        }
      }
      for (Uint j = 0; j < connected2.size(); ++j) {
        if (!connected2[j] && al2[j] < al1.size()) {
          Uint i = al2[j];
          vector<Uint>::iterator p = lower_bound(sets1[i].begin(), sets1[i].end(), j);
          sets1[i].insert(p, j);
          connected2[j] = true;
          new_pairs.push_back(al2[j]); new_pairs.push_back(j);
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

   if (sets1[i].size() > 0 && connected2[j] || // both words already connected
       al1[i] != j && al2[j] != i)	       // not in union
      return false;

   vector<Uint>::iterator p = lower_bound(sets1[i].begin(), sets1[i].end(), j);
   if (p != sets1[i].end() && *p == j)
      return false;		// already in sets1

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

/*--------------------------------------------------------------------------------
  IBMScoreAligner
  ------------------------------------------------------------------------------*/

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

double IBMScoreAligner::align(const vector<string>& toks1, const vector<string>& toks2, 
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
      if (sets1[p.first].size() == 0 && connections2[p.second] == 0) {
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
      if (sets1[p.first].size() == 0 || connections2[p.second] == 0) {
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


double CartesianAligner::align(const vector<string>& toks1, const vector<string>& toks2, 
			       vector< vector<Uint> >& sets1)
{
   sets1.clear();
   sets1.resize(toks1.size());
   return 1.0;
}
