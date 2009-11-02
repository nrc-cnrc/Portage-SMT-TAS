/**
 * $Id$
 * @file ngram.h - 
 *   class representing n-grams with comparison function etc.
 *
 * PROGRAMMER: Nicola Ueffing
 * 
 * COMMENTS: this file contains the definitions of the functions as well because the compiler 
 * will output funny linker errors if a separate .cc or .cpp file is created (due to use of templates)
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef _NGRAM_H_
#define _NGRAM_H_

#include <vector>
#include <iostream>
#include "portage_defs.h"

namespace Portage {
  
  template<class T> class NGram {
    
  private:
    Uint       n;       // length of n-gram
    vector<T>  words;
    
  public:
    
    ~NGram();
    NGram();

    /**
     * Construct n-gram of length len, starting at position i in sentence sent 
     **/
    NGram(const vector<T> &sent, Uint i, Uint len); 
      
    inline bool operator<  (const NGram &ngr) const;
    inline bool operator== (const NGram &ngr) const;

    const vector<T>& getWords() const;

    friend ostream& operator<< (ostream &out, const NGram &ngr) {
      for (typename vector<T>::const_iterator itr = ngr.getWords().begin(); itr!=ngr.getWords().end(); itr++)
        out << *itr << " ";
      return out;
    }

  }; // class Ngram


//////////////////////////////////////////////////////////////////////

  template<class T>
  NGram<T>::~NGram() {
    words.clear();
  }
  
  template<class T>
  NGram<T>::NGram() : n(0), words(vector<T>()) {}
   
   
  template<class T>
  NGram<T>::NGram(const vector<T> &sent, Uint i, Uint len) : n(len) {
    if (i+n > sent.size()) cerr << "NGram:boom! " << i << "+" << n << ">" << sent.size() << endl;
    assert(i+n <= sent.size());
    words.assign(sent.begin()+i, sent.begin()+i+n);
  }
      
    
  template<class T>
  inline bool NGram<T>::operator< (const NGram<T> &ngr) const {
    if (n < ngr.n) return 1;
    if (n > ngr.n) return 0;
    
    for (Uint k=0; k<n; k++) {
      if (words[k] < ngr.words[k]) return 1;
      if (words[k] > ngr.words[k]) return 0;
    }
    return 0;
  }

  template<class T>
  inline bool NGram<T>::operator== (const NGram<T> &ngr) const  {
    if (n < ngr.n) return 0;
    if (n > ngr.n) return 0;
    
    for (Uint k=0;k<n;k++) {
      if (words[k] < ngr.words[k]) return 0;
      if (words[k] > ngr.words[k]) return 0;
    }
    return 1;
  }

  template<class T>
  const vector<T>& NGram<T>::getWords() const {
    return words;
  }

} // Portage

#endif // _NGRAM_H_

