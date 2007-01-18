/**
 * @author Nicola Ueffing
 * @file levenshtein.h Determines Levenshtein alignment for two sequences.
 *
 *
 * COMMENTS: determines Levenshtein alignment for two sequences of something (string, chars,...)
 * sequences are represented through vectors
 * either distance or a matrix with the alignment is returned
 *
 * MORE COMMENTS: this file contains the definitions of the functions as well because the compiler will output
 * funny linker errors if a separate .cc or .cpp file is created
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef LEVENSHTEIN_H
#define LEVENSHTEIN_H

#include <vector>
#include <iostream>
#include "portage_defs.h"

namespace Portage {
 
/// represent correct words, substitution, insertion and deletion through 'char'.
const char corr=0, sub=1, del=2, ins=3;


/**
 * Class for backpointers storing the best predecessor in the Levenshtein alignment.
 * Elements: indices of best predecessor in the Levenshtein alignment matrix: j=row, i=column
 *           char indicating correct (0), substitution (1), deletion (2), insertion (3), dummy for initialization (4)
 */
class bkp {
 public:
  int  j,i;  
  char c;

  /// Default empty constructor.
  bkp() : j(-1), i(-1), c(4) {}
  /// Copy constructor.
  bkp(const bkp &b) : j(b.j), i(b.i), c(b.c) {}
  /// Constructor from components.
  bkp(const int &_j, const int &_i, const char &_c) : j(_j), i(_i), c(_c) {}
  /// Destructor.
  ~bkp() {};

  /// Assignment operator.
  inline void operator=(const bkp &b) {
    j=b.j; i=b.i; c=b.c;
  }

  /// Outputs a bkp instance to a ostream.
  friend ostream &operator<<(ostream &out, const bkp &b) {
    out << "(" << b.j << "," << b.i << "," << (int)b.c << ")\t";
    return out;
  }
};


/**
 * Class for Levenshtein alignment.
 * Elements: matrix storing the best predecessor of each point
 *           total (minimal) costs
 */
template<class T> class Levenshtein {
private:
  vector< vector<bkp> > Book;
  vector<int>           Q, Q_old;
  int                   costs;
  int                   verbose;
  int                   subcost;

public:
  typedef vector<T>                 Tv;
  typedef typename vector<T>::iterator       Tvi;
  typedef typename vector<T>::const_iterator Tvci;


   /// Default constructor.
   Levenshtein() : Book(vector< vector<bkp> >()), Q(vector<int>()), 
		   Q_old(vector<int>()), costs(0), verbose(0), subcost(1) {}
   /// Destructor.
  ~Levenshtein() {};
  
  /// Clears the content of the instance.
  void clear();

  /// Sets verbosity.
  void setVerbosity(const int &v) {verbose=v;}
  /// Sets the subcost.
  void setSubCost(int c) {subcost = c;}

  void        LevenAlign(const Tv &hyp, const Tv &ref, 
			 const vector<int>* ins_costs=NULL, const vector<int>* del_costs=NULL);
  vector<int> LevenAlig (const Tv &hyp, const Tv &ref, 
			 const vector<int>* ins_costs=NULL, const vector<int>* del_costs=NULL);

  int         LevenDist(const Tv &hyp, const Tv &ref, 
			 const vector<int>* ins_costs=NULL, const vector<int>* del_costs=NULL);
  int         LevenDistIncompleteRef(const Tv &hyp, const Tv &ref, 
			 const vector<int>* ins_costs=NULL, const vector<int>* del_costs=NULL);

  void        dumpAlign(const Tv &hyp, const Tv &ref, const vector<int>& align, ostream& os);
  
};

/********************************************************/

template<class T>
void Levenshtein<T>::clear() {
  for (vector< vector<bkp> >::iterator itr=Book.begin(); itr!=Book.end(); itr++)
    itr->clear();
  Book.clear();
  Q.clear();
  Q_old.clear();
}


/**
 * Determine Levenshtein alignment between two sequences and store information in 'Book'
 * Note: Book[j] is the j-th column. 
 *
 * Default costs are 1 for substitution, insertion, and deletion; the latter
 * two can be changed with optional ins_costs and del_costs parameters.
 *
 * LevenAlig calls this and determines and returns the vector of alignments
 * Levendist calls this and returns only cost
 *
 * @param hyp        a vector representing the tokens in the hypothesis
 * @param ref        a vector representing the tokens in the reference
 * @param ins_costs  if non-null, (*ins_costs)[i] contains insertion cost for hyp[i], 
 *        i in 0..hyp.size()-1
 * @param del_costs  if non-null, (*del_costs)[i] contains deletion cost for ref[i], 
 *        i in 0..ref.size()-1
 */
template<class T>
void Levenshtein<T>::LevenAlign(const Tv &hyp, const Tv &ref, 
				const vector<int>* ins_costs, const vector<int>* del_costs) 
{
  clear();

  if (ins_costs) assert(ins_costs->size() == hyp.size());
  if (del_costs) assert(del_costs->size() == ref.size());

  if (verbose>0) {
    cerr << "hyp: ";
    for (Tvci itr=hyp.begin(); itr!=hyp.end(); itr++)
      cerr << *itr << " ";
    cerr << endl << "trg: ";
    for (Tvci itr=ref.begin(); itr!=ref.end(); itr++)
      cerr << *itr << " ";
    cerr << endl;
  }

  /** 
   * init 
   */
  bkp dummy(-1,-1,4);
  vector<bkp> dummyvec(ref.size()+1,dummy);
  Book.resize(hyp.size()+1,dummyvec);
  /**
   * backpointers for deleting/inserting all words
   */
  for (uint i=1; i<=ref.size(); i++)
    Book[0][i] = bkp(0,i-1,del);
  for (uint j=1; j<=hyp.size(); j++)
    Book[j][0] = bkp(j-1,0,ins);

  /**
   * store current column of the Levenshtein matrix in 'Q', and previous column in 'Q_old'
   * the other information is not needed in Dynamic Programming :-)
   * Q is initialized with costs 'i' for position 'i' (deletions), unless del_costs specified
   */
  for (uint i=0; i<ref.size()+1; i++) {
     if (del_costs) {
	if (i == 0) Q.push_back(0);
	else Q.push_back(Q[i-1] + (*del_costs)[i-1]);
     } else
	Q.push_back(i);
  }
  Q_old.resize(ref.size()+1);

  /**
   * Proceed along positions in 'hyp' and update values in Q
   * prefer substitutions to deletions and insertions
   */
  int qsub, qins, qdel, subcosts;
  for (uint j=1; j<=hyp.size(); j++) {
    Q_old = Q;

    /**
     * position 0 in reference corresponds to insertion of all hyp words
     */
    Q[0] = Q_old[0] + (ins_costs ? (*ins_costs)[j-1] : 1);
    Book[j][0] = bkp(j-1,0,ins);

    for (uint i=1; i<=ref.size(); i++) {

      /**
       * costs of substitution/correct, deletion, insertion
       */
      subcosts = (hyp[j-1]==ref[i-1]) ? 0 : subcost;
      if (verbose>1)
	 cerr << hyp[j-1] << " " << ref[i-1] << " " << j << " " << i << endl;

      qsub = Q_old[i-1] + subcosts;
      qdel = Q[i-1] + (del_costs ? (*del_costs)[i-1] : 1);
      qins = Q_old[i] + (ins_costs ? (*ins_costs)[j-1] : 1);

      if (verbose>1) 
	cerr << "  j,i: " << j << "," << i << " , subcosts: " << subcosts << ", sub: " << qsub << ", del: " << qdel << ", ins: " << qins << endl;
      if (qsub <= qdel) {
	if (qsub <= qins) {
	  Q[i]       = qsub; 
	  Book[j][i] = bkp(j-1,i-1,subcosts);
	  if (verbose>5) {
	    cerr << "  sub, bkp: " << Book[j][i];
	    cerr << " " << Book[j-1][i-1];
	    cerr<<endl;
	  }
	} // substitution
	else {
	  Q[i]       = qins; 
	  Book[j][i] = bkp(j-1,i,ins);
	  if (verbose>5) {
	    cerr << "  ins, bkp: " << Book[j][i];
	    cerr << " " << Book[j-1][i];
	    cerr<<endl;
	  }
	} // insertion
      }
      else if (qdel <= qins) {
	Q[i]       = qdel;
	Book[j][i] = bkp(j,i-1,del);
	  if (verbose>5) {
	    cerr << "  del, bkp: " << Book[j][i];
	    cerr << " " << Book[j][i-1];
	    cerr<<endl;
	  }
      } // deletion
      else {
	Q[i]       = qins; 
	Book[j][i] = bkp(j-1,i,ins);
	if (verbose>5) {
	  cerr << "  ins, bkp: " << Book[j][i];
	  cerr << " " << Book[j-1][i];
	  cerr<<endl;
	}
      } // insertion
    } // for i

    if (verbose>3) {
      for (uint j=0; j<hyp.size(); j++) {
	for (uint i=0; i<ref.size(); i++) {
	  cerr << Book[j][i] << "\t";
	}
	cerr << endl;
      } // for i
    }

  } // for j

  if (verbose>3) {
    for (uint j=0; j<hyp.size(); j++) {
      for (uint i=0; i<ref.size(); i++) {
	cerr << Book[j][i] << "\t";
      }
      cerr << endl;
    } // for i
  }
  costs = Q[ref.size()];
}

/**
 * Determine Levenshtein alignment between two sequences and return a vector indicating 
 * the positions which the elements of 'hyp' are aligned to in 'ref'.
 * Note: positions start at 0
 *   -1 indicates deletion of the element
 *   inserted elements (in 'ref') are stored in the vector entries >=
 *   hyp.size() [is that a good idea? GF: probably not; it's just cost me a core
 *   dump, because I forgot about this trick, and assumed that the returned
 *   alignment would be the same length as hyp - so maybe change it some day :-)]
 * Note: Book[j] is the j-th column
 *
 * Default costs are 1 for substitution, insertion, and deletion; the latter
 * two can be changed with optional ins_costs and del_costs
 * parameters.
 *
 * @param hyp        a vector representing the tokens in the hypothesis
 * @param ref        a vector representing the tokens in the reference
 * @param ins_costs  if non-null, (*ins_costs)[i] contains insertion cost for hyp[i], 
 *        i in 0..hyp.size()-1
 * @param del_costs  if non-null, (*del_costs)[i] contains deletion cost for ref[i], 
 *        i in 0..ref.size()-1
 * @return Returns a vector indicating the positions which the elements of
 * 'hyp' are aligned to in 'ref'
 */
template<class T>
vector<int> Levenshtein<T>::LevenAlig(const Tv &hyp, const Tv &ref,
				      const vector<int>* ins_costs, const vector<int>* del_costs) 
{
  vector<int> result(hyp.size(),-1);

  LevenAlign(hyp, ref, ins_costs, del_costs);

  if (verbose>0)
    cerr << "minimal costs: " << costs << " " << endl;

  /** 
   * Determine alignment between words in hyp and ref
   */  
  bkp b = Book[hyp.size()][ref.size()];
  vector<int> deletions;
  int j = b.j;
  int i = b.i;
  
  while (b.c<4) {
    if (verbose>4)
      cerr << "traceback to " << b;

    if (b.c<del)         // correct or substitution
      result[j] = i;
    else if (b.c==del)   // deletion
      {
	deletions.push_back(i);
	if (verbose>3)
	  cerr << "deleted " << i << " at bkp " << b << endl;
      }
    b = Book[j][i];
    j = b.j;
    i = b.i;
    if (verbose>4)
      cerr << " : bkp is " << b << endl;
  }

  for (vector<int>::reverse_iterator itr=deletions.rbegin(); itr!=deletions.rend(); itr++)
    result.push_back(*itr);

  if (verbose>0) {
    cerr << "result: ";
    for (uint k=0; k<hyp.size(); k++)
      cerr << result[k] << " ";
    cerr << "| ";
    for (uint k=hyp.size(); k<result.size(); k++)
      cerr << result[k] << " ";
    cerr << endl;
  }
  return result;
 }


/**
 * Determine Levenshtein alignment between two sequences and return the Levenshtein distance only.
 * @param hyp        a vector representing the tokens in the hypothesis
 * @param ref        a vector representing the tokens in the reference
 * @param ins_costs  if non-null, (*ins_costs)[i] contains insertion cost for hyp[i], 
 *        i in 0..hyp.size()-1
 * @param del_costs  if non-null, (*del_costs)[i] contains deletion cost for ref[i], 
 *        i in 0..ref.size()-1
 * @return Returns the minimal Levenshtein distance 
 */
template<class T>
  int Levenshtein<T>::LevenDist(const Tv &hyp, const Tv &ref,
	 const vector<int>* ins_costs, const vector<int>* del_costs) {

  LevenAlign(hyp, ref, ins_costs, del_costs);

  return costs;
}

/**
 * Determine Levenshtein alignment between two sequences.
 * Return the minimal Levenshtein distance which is obtained by covering only parts
 *  of the reference (complete coverage of hyp. is required)
 * @param hyp        a vector representing the tokens in the hypothesis
 * @param ref        a vector representing the tokens in the reference
 * @param ins_costs  if non-null, (*ins_costs)[i] contains insertion cost for hyp[i], 
 *        i in 0..hyp.size()-1
 * @param del_costs  if non-null, (*del_costs)[i] contains deletion cost for ref[i], 
 *        i in 0..ref.size()-1
 * @return Returns the minimal Levenshtein distance 
 */
template<class T>
  int Levenshtein<T>::LevenDistIncompleteRef(const Tv &hyp, const Tv &ref, 
                           const vector<int>* ins_costs, const vector<int>* del_costs) {

  LevenAlign(hyp, ref, ins_costs, del_costs);

  
  if (verbose>1) {
    cerr << "levAlig Q:\t";
    for (uint ii=0; ii<Q.size(); ii++) {
      cerr << Q[ii] << " ";
    }   
    cerr << endl;
  }
    
  return *min_element(Q.begin(), Q.end());
 }

/**
 * Dump the results of a Levenshtein alignment to a given stream.
 * @param hyp    a vector representing the tokens in the hypothesis
 * @param ref    a vector representing the tokens in the reference
 * @param align  the Levenshtein alignments 
 * @param os     output stream 
 */
template<class T>
 void Levenshtein<T>::dumpAlign(const Tv &hyp, const Tv &ref, const vector<int>& align, ostream& os)
{
   Uint next = 0;
   for (Uint i = 0; i < hyp.size(); ++i) {
      if (align[i] == -1)
	 os << hyp[i] << "/ ";
       else {
	  for (int j = next; j < align[i]; ++j)
	     os << " /" << ref[j] << ' ';
	  os << hyp[i] << '/' << ref[align[i]] << ' ';
	  next = align[i]+1;
       }
   }
   for (Uint j = next; j < ref.size(); ++j)
      os << "/" << ref[j] << ' ';
   os << endl;
}

}// ends Portage namespace
#endif
