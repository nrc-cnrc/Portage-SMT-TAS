/**
 * @author George Foster
 * @file doc_vect.h  Representation and manipulation of a SET of "document vectors".
 * 
 * COMMENTS:
 *
 * The representation of feature names is intended for very large phrase sets, so we
 * use a clunky two-stage trie approach.
 *
 * TODO: 
 * - add SVD, etc
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef DOC_VECT_H
#define DOC_VECT_H

#include <iostream>
#include <map>
#include <cmath>
#include <limits>
#include <trie.h>
#include <voc.h>

namespace Portage {

/**
 * A bunch of doc vects, sharing a vocabulary.
 */
class DocVectSet
{
public:

   typedef map<Uint,float> DocVect; ///< feature -> value

private:

   Voc word_voc;                ///< word <-> index
   typedef PTrie<Uint,Empty,false> FeatureMap;
   FeatureMap fmap;             ///< feature (word sequence) -> index

   vector<string> doc_names;    ///< doc -> name
   vector<DocVect> vals;        ///< doc -> (feature -> value)
   vector<Uint> doc_freqs;      ///< feature -> # of docs

   Uint addKeyToFmap(const string& key);
   void writeDocVect(const DocVect& dv, ostream& out,
      FeatureMap::iterator it, FeatureMap::iterator end, vector<Uint>& prefix);

   void getFeatureNames(vector<string>& names,
      FeatureMap::iterator it, FeatureMap::iterator end, vector<Uint>& prefix);

public:

   DocVectSet() {}
   DocVectSet(const vector<string>& files) {read(files);}

   void read(const vector<string>& files) {
      for (Uint i = 0; i < files.size(); ++i) addDocVect(files[i]);
   }

   void clear();
   
   Uint numDocs() {return vals.size();}
   Uint numFeatures() {return doc_freqs.size();}
   Uint numVals();		// number of non-zero values
   const string& docName(Uint i) {return doc_names[i];}
   DocVect& getDocVect(Uint i) {return vals[i];}
   Uint docFreq(Uint feature) {return doc_freqs[feature];}

   /**
    * Find a feature by its name; return numFeatures() if not found.
    */
   Uint getFeature(const string& name);

   /**
    * Set names[i] to the name of the ith feature, for i = 0..numFeatures()-1.
    * (This will require huge memory if feature names are phrase pairs).
    */
   void getFeatureNames(vector<string>& names);

   /**
    * Append new doc to set. By default, the resulting DocVect will be the last
    * one in the set, accessible via getDocVect(numDocs()-1).
    * @param dv_file name of doc vect file containing one feature,value pair
    * per line, in format: "<feature>\s+<value>\s*", ie feature can contain
    * blanks. 
    * @param pos if < size(), new vect will replace existing one at index pos,
    * otherwise it just gets appended. Using pos < size() does not erase any
    * features added by the replaced doc.
    */
   void addDocVect(const string& dv_file, Uint pos=numeric_limits<Uint>::max());


   /**
    * Return the inverse doc freq weight that will be applied to a given feature.
    */
   double idf(Uint feature, double smooth) {
      return log((numDocs()+smooth) / (double) doc_freqs[feature]);
   }

   /**
    * Convert frequency values to tfidf, multplying by log((D+smooth)/d), where
    * d is the document frequency of the feature. Higher values of smooth are 
    * smoother.
    */
   void tfidf(double smooth = 0.0);

   /**
    * Compute cosine proximity between two doc vectors.
    */
   static double cosine(const DocVect& dv1, const DocVect& dv2);

   /**
    * Compute sigmoid transformation on proximity.
    * @name prox proximity metric, eg cosine, in [0,1]
    * @name s steepness: 0 for const sigmoid-.5; 100+ for step @ prox=.5
    */
   double sigmoid(double x, double a, double b) {
      return 1.0 / (1.0 + exp(a * (b-x)));
   }

   /**
    * Compute vector size (L2).
    */
   static double vsize(const DocVect& v);

   /**
    * Sum all values.
    */
   static double sumValues(const DocVect& v);

   /**
    * Set v1 = v1 + v2 * v2_scale.
    */
   static void addScaled(DocVect& v1, const DocVect& v2, double v2_scale);

   /**
    * Count the number of features with frequency one.
    */
   static Uint numOnes(DocVect& v);

   /**
    * Set contents of doc vect to freq 1
    */
   static void setToOnes(DocVect& v);

   /**
    * Cluster docs using kmeans algorithm with cosine proximity metric. The
    * first version is the basic algorithm; the 2nd iterates it using different
    * randomly-chosen starting points. Use the 2nd, generally.
    * @param K the eponymous (desired number of clusters)
    * @param cluster_map doc index -> cluster index, in 0..K-1
    * @param means cluster index -> mean vector for that cluster
    * @param verbose dump cluster contents after each iteration
    * @return kmeans cosine objective function: sum of proximities to class
    * mean vectors (unnormalized & higher is better)
    */
   double kmeans(Uint K, vector<Uint>& cluster_map, vector<DocVect>& means,
		 bool verbose=false);
   double kmeans(Uint K, vector<Uint>& cluster_map, vector<DocVect>& means,
		 Uint niters, bool verbose=false);

   /**
    * For each doc in the set, calculate its cosine proximity to its cluster
    * mean.
    * @param K
    * @param cluster_map
    * @param means
    * @param prox doc index -> proximity
    */
   void calcDocProximities(Uint K, const vector<Uint>& cluster_map, 
			   const vector<DocVect>& means, vector<float>& prox);

   /**
    * Calculate weighted cluster mean vectors - a weighted, non-normalized
    * sum of vector components.
    * @param K
    * @param cluster_map
    * @param wts doc index -> weight on doc
    * @param means cluster index -> mean vector for cluster
    */
   void calcClusterMeans(Uint K, const vector<Uint>& cluster_map, 
			 const vector<float>& wts, vector<DocVect>& means);

   /**
    * Write a doc vect on a stream, in the same format as it was read.
    */
   void writeDocVect(const DocVect& dv, ostream& out);

   /**
    * Write cluster means to files. The kth mean vector will be written to file
    * cluster"\<k\>"."\<suff\>".
    * @param means cluster index -> mean vect
    * @param pref
    * @param suff 
    */
   void writeClusterMeans(const vector<DocVect>& means, 
			  const string& pref, const string& suff);

   /**
    * Dump clustering to a stream.
    * @param K
    * @param cluster_map
    * @param out
    * @param human if true, write files in each cluster on a single line; if
    * false, write file indexes in each cluster surrounded by [], and put ALL
    * clusters on a single line.
    */
   void dumpClusters(Uint K, const vector<Uint>& cluster_map, ostream& out, bool human=false);

   /**
    * Write contents of each doc vect to "<fname>".dump, where "<fname>" is the
    * original file from which that vect was read.
    */
   void dump();
};
}

#endif
