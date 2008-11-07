/**
 * @author George Foster
 * @file doc_vect.cc
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <str_utils.h>
#include <file_utils.h>
#include <gfstats.h>
#include "doc_vect.h"

using namespace Portage;

Uint DocVectSet::numVals()
{
   Uint nv = 0;
   for (Uint i = 0; i < vals.size(); ++i)
      nv += vals[i].size();
   return nv;
}

Uint DocVectSet::addKeyToFmap(const string& key)
{
   vector<string> toks;
   vector<Uint> codes;
   split(key, toks, " ");
   for (Uint i = 0; i < toks.size(); ++i)
      codes.push_back(word_voc.add(toks[i].c_str()));

   Uint* fp;
   bool found = fmap.find_or_insert(&codes[0], codes.size(), fp);
   if (!found) *fp = numFeatures()+1; // reserve 0 for the empty value in the trie

   return *fp-1;
}

Uint DocVectSet::getFeature(const string& name)
{
   vector<string> toks;
   vector<Uint> codes;
   split(name, toks, " ");
   for (Uint i = 0; i < toks.size(); ++i)
      codes.push_back(word_voc.index(toks[i].c_str()));

   Uint f(0);
   bool found = fmap.find(&codes[0], codes.size(), f);
   return (found && f != 0) ? f-1 : numFeatures();
}

void DocVectSet::addDocVect(const string& dv_file, Uint pos)
{
   iSafeMagicStream in(dv_file);

   DocVect* dv;
   if (pos < numDocs()) {
      doc_names[pos] = dv_file;
      vals[pos].clear();
      dv = &vals[pos];
   } else {
      doc_names.push_back(dv_file);
      DocVect empty;
      vals.push_back(empty);
      dv = &vals.back();
   }

   string line;
   while (getline(in, line)) {

      string::size_type end = line.find_last_not_of(" \t");
      if (end == string::npos) {
	 error(ETWarn, "skipping blank line in %s", dv_file.c_str());
	 continue;
      }
      string::size_type beg = line.find_last_of(" \t", end);
      if (beg == string::npos) {
	 error(ETWarn, "skipping badly-formed line in " + dv_file + ": " + line);
	 continue;
      }
      string val = line.substr(beg+1, end-beg);
      end = line.find_last_not_of(" \t", beg);
      if (end == string::npos) {
	 error(ETWarn, "badly-formed line in " + dv_file + ": " + line);
	 continue;
      }
      string key = line.substr(0, end+1);

      if (val == "" || key == "") {
	 error(ETWarn, "badly-formed line in " + dv_file + ": " + line);
	 continue;
      }

      Uint f = addKeyToFmap(key);
      (*dv)[f] = conv<float>(val);

      if (f >= doc_freqs.size()) doc_freqs.resize(f+1);
      ++doc_freqs[f];
   }
}

void DocVectSet::clear() {
   word_voc.clear();
   fmap.clear();
   doc_names.clear();
   vals.clear();
   doc_freqs.clear();
}

void DocVectSet::dump()
{
   for (Uint i = 0; i < doc_names.size(); ++i) {
      string newname = doc_names[i] + ".dump";
      oSafeMagicStream out(newname);
      writeDocVect(vals[i], out);
   }
}

void DocVectSet::tfidf(double smooth)
{
   for (Uint i = 0; i < numDocs(); ++i)
      for (DocVect::iterator p = vals[i].begin(); p != vals[i].end(); ++p)
	 p->second *= idf(p->first, smooth);
}

double DocVectSet::cosine(const DocVect& dv1, const DocVect& dv2)
{
   const DocVect *dv_short, *dv_long;
   if (dv1.size() < dv2.size()) {
      dv_short = &dv1;
      dv_long  = &dv2;
   } else {
      dv_short = &dv2;
      dv_long  = &dv1;
   }

   double s = 0.0;
   for (DocVect::const_iterator p = dv_short->begin(); p != dv_short->end(); ++p) {
      DocVect::const_iterator pf = dv_long->find(p->first);
      if (pf != dv_long->end()) 
	 s += p->second * pf->second;
   }
   double ds1 = vsize(dv1);
   double ds2 = vsize(dv2);
   return (ds1 > 0 && ds2 > 0) ? s / (ds1 * ds2) : 0.0;
}

void DocVectSet::setToOnes(DocVect& dv) {
   for (DocVect::iterator p = dv.begin(); p != dv.end(); ++p)
      p->second = 1;
}

double DocVectSet::vsize(const DocVect& dv) {
   double s = 0.0;
   for (DocVect::const_iterator p = dv.begin(); p != dv.end(); ++p) {
      s += (p->second * p->second);
   }
   return sqrt(s);
}

double DocVectSet::sumValues(const DocVect& dv) {
   double s = 0.0;
   for (DocVect::const_iterator p = dv.begin(); p != dv.end(); ++p) {
      s += p->second;
   }
   return s;
}

Uint DocVectSet::numOnes(DocVect& dv) {
   Uint n = 0;
   for (DocVect::const_iterator p = dv.begin(); p != dv.end(); ++p) {
      if (p->second == 1.0)
	 ++n;
   }
   return n;
}

void DocVectSet::addScaled(DocVect& dv1, const DocVect& dv2, double v2_scale)
{
   for (DocVect::const_iterator p = dv2.begin(); p != dv2.end(); ++p)
      dv1[p->first] += p->second * v2_scale;
}

double DocVectSet::kmeans(Uint K, vector<Uint>& clusters, vector<DocVect>& means, 
			  bool verbose)
{
   // check if this is a hard constraint
   assert(K <= numDocs());

   // initialize with K random means
   vector<Uint> ind(numDocs());
   for (Uint i = 0; i < numDocs(); ++i) ind[i] = i;
   random_shuffle(ind.begin(), ind.end());
   means.clear();
   for (Uint k = 0; k < K; ++k)
      means.push_back(vals[ind[k]]);

   clusters.resize(numDocs());
   vector<Uint> old_clusters(numDocs(), K);

   bool done = false;
   do {
      // calculate cluster membership for each doc
      for (Uint i = 0; i < numDocs(); ++i) {
	 double max_cosine = 0;
	 Uint best_k = 0;
	 for (Uint k = 0; k < K; ++k) {
	    double cos = cosine(vals[i], means[k]);
	    if (cos > max_cosine) {
	       max_cosine = cos;
	       best_k = k;
	    }
	 }
	 clusters[i] = best_k;
      }
      done = (clusters == old_clusters);
      old_clusters = clusters;
      
      if (verbose) {dumpClusters(K, clusters, cerr);}

      // calculate new means
      // !!! merge this with calcClusterMeans - just settle the scaling part
      for (Uint k = 0; k < K; ++k)
	 means[k].clear();
      for (Uint i = 0; i < numDocs(); ++i)
	 addScaled(means[clusters[i]], vals[i], 1.0 / vsize(vals[i]));
      
   } while (!done);

   // compute objective

   double obj = 0;
   for (Uint i = 0; i < numDocs(); ++i)
      obj += cosine(vals[i], means[clusters[i]]);
   return obj;
}

double DocVectSet::kmeans(Uint K, vector<Uint>& cluster_map, vector<DocVect>& means,
			  Uint niters, bool verbose)
{
   vector<Uint> cm;
   vector<DocVect> mns;
   double best_score = 0;

   for (Uint iter = 0; iter < niters; ++iter) {
      double score = kmeans(K, cm, mns, verbose);
      if (score > best_score) {
	 best_score = score;
	 cluster_map = cm;
	 means = mns;
      }
      if (verbose)
	 cerr << "done iter " << iter+1 << ": score = " << score << endl;
   }
   return best_score;
}

void DocVectSet::calcDocProximities(Uint K, const vector<Uint>& cluster_map, 
				    const vector<DocVect>& means, vector<float>& prox)
{
   prox.clear();
   for (Uint i = 0; i < cluster_map.size(); ++i)
      prox.push_back(cosine(vals[i], means[cluster_map[i]]));
}

void DocVectSet::calcClusterMeans(Uint K, const vector<Uint>& cluster_map, 
				  const vector<float>& wts, vector<DocVect>& means)
{
   means.resize(K);
   for (Uint k = 0; k < K; ++k)
      means[k].clear();
   for (Uint i = 0; i < numDocs(); ++i)
      addScaled(means[cluster_map[i]], vals[i], wts[i]);
}

void DocVectSet::writeClusterMeans(const vector<DocVect>& means, const string& pref, const string& suff)
{
   for (Uint k = 0; k < means.size(); ++k) {
      ostringstream fname;
      fname << pref << "cluster" << k+1 << suff;
      oSafeMagicStream out(fname.str());
      writeDocVect(means[k], out);
   }
}

void DocVectSet::dumpClusters(Uint K, const vector<Uint>& cluster_map, ostream& out, bool human)
{
   for (Uint k = 0; k < K; ++k) {
      if (!human) out << "[";
      bool beg = true;
      for (Uint i = 0; i < cluster_map.size(); ++i) 
	 if (cluster_map[i] == k) {
	    if (!beg) {out << " ";} else {beg = false;}
	    if (human) out << docName(i) << " ";
	    else out << i;
	 }
      if (human) out << endl;
      else out << "] ";
   }
   if (!human) out << endl;
}

void DocVectSet::writeDocVect(const DocVect& dv, ostream& out)
{
   vector<Uint> prefix;
   writeDocVect(dv, out, fmap.begin_children(), fmap.end_children(), prefix);
}

void DocVectSet::writeDocVect(const DocVect& dv, ostream& out,
   FeatureMap::iterator it, FeatureMap::iterator end, vector<Uint>& prefix)
{
   for ( ; it != end; ++it ) {
      prefix.push_back(it.get_key());

      // if node is a leaf & index is found in doc vect, print out feature
      // name and value
      if ( it.is_leaf() && it.get_value() != 0 ) {
         DocVect::const_iterator p;
         if ((p = dv.find(it.get_value()-1)) != dv.end()) {
            string fname;
            for (Uint i = 0; i < prefix.size(); ++i)
               fname += word_voc.word(prefix[i]) + (string)" ";
            out << fname << p->second << endl;
         }
      }

      // recurse
      if ( it.has_children() )
         writeDocVect(dv, out, it.begin_children(), it.end_children(), prefix);
      
      prefix.pop_back();
   }
}

void DocVectSet::getFeatureNames(vector<string>& names)
{
   names.resize(numFeatures());
   vector<Uint> prefix;
   getFeatureNames(names, fmap.begin_children(), fmap.end_children(), prefix);
}

void DocVectSet::getFeatureNames(vector<string>& names,
   FeatureMap::iterator it, FeatureMap::iterator end, vector<Uint>& prefix)
{
   for ( ; it != end; ++it ) {
      prefix.push_back(it.get_key());

      if ( it.is_leaf() && it.get_value() != 0) {
         string& fname = names[it.get_value()-1];
         for (Uint i = 0; i < prefix.size(); ++i) {
            fname += word_voc.word(prefix[i]);
            if (i+1 < prefix.size()) fname += (string)" ";
         }
      }
   
      // recurse
      if ( it.has_children() ) 
         getFeatureNames(names, it.begin_children(), it.end_children(), prefix);

      prefix.pop_back();
   }
}
