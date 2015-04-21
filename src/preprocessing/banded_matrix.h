// $Id$
/**
 * @author Write your name here
 * @file banded_matrix.h
 * @brief A Compact representation of a banded matrix.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#ifndef __BANDED_MATRIX_H__
#define __BANDED_MATRIX_H__


#include "errors.h"
#include <limits>  // numeric_limits::max

namespace Portage {

//template<class T>
//struct Policy {
//   const T outOfBandValue;
//   Policy(const T& v = T()) : outOfBandValue(v) {}
//};

/**
 * A Compact representation of a banded matrix.
 * The matrix must be vertical aka size1 >= size2.
 */
//template<class T, int outValue=numeric_limits<typename T>::infinity()>
//template<class T, double outValue=4.0>
//template<class T, class P = Policy<T> >
template<class T>
class BandedMatrix {
   private:
      Uint _size1;  ///< simulated matrix size
      Uint _size2;  ///< simulated matrix size
      Uint b;       ///< window's size aroung the diagonal
      double m;     ///< slope of the diagonal

      vector<T> data;       ///< Matrix's data in a linear compact representation.
      T* pdata;
      // NOTE: index[x] is the lower bound column index (included) for row x
      //       index[x+1] is the up bound column index (excluded) for row x
      // row x indices are [index[x], index[x+1])
      vector<T*> index;   ///< indices of each row's start into matrix;
      vector<pair<Uint, Uint> > range;
      //const P policy;

   public:
      /// Default constructor.
      BandedMatrix()
      : _size1(0)
      , _size2(0)
      , b(0)
      , m(0.0)
      , pdata(NULL)
      {}

      /**
       * Constructs a matrix.
       * See resize for parameters' description.
       */
      BandedMatrix(Uint size1, Uint size2, Uint beamWidth) {
         resize(size1, size2, beamWidth);
      }

      /**
       * Resizes the matrix.
       * Restriction: size1 >= size2 or else the Matrix has more holes in it.
       * @param size1  number of rows.
       * @param size2  number of columns.
       * @param beamWidth  number lines around the diagonal.
       */
      void resize(Uint __size1, Uint __size2, Uint beamWidth) {
         assert(__size1 > 0);
         assert(__size2 > 0);
         assert(__size1 >= __size2);

         _size1 = __size1;
         _size2 = __size2;
         // What is our offset value
         b =  min(beamWidth, size2()-1);
         // What is the slope value
         if (size2() == 1) {
            // the slope is lock to 0 in the case of a vertical vector.
            m = 0;
         }
         else {
            m = max<double>(1, size2()-1) / max<double>(1, size1()-1);
         }

         // Calculate the row indices into the matrix/data.
         vector<Uint> tmp_index;   ///< indices of each row's start into matrix;
         tmp_index.resize(size1() + 1);
         tmp_index[0] = 0;
         index.resize(size1()+1);
         range.resize(size1());
         for (Uint i(0); i<size1(); ++i) {
            range[i].first = max<int>(0, diag(i)-getB());
            range[i].second = min(size2(), diag(i)+getB()+1);
            // This is the previous index endpoint plus the total number of element on that row.
            tmp_index[i+1] = tmp_index[i] + (maxRange(i) - minRange(i));
         }

         // Allocate the required memory
         const Uint size = tmp_index.back();
         //data.clear();
         if (data.size() < size) {
            data.resize(size);
         }

         pdata = &data[0];
         for (Uint i(0); i<index.size(); ++i) {
            // This is the previous index endpoint plus the total number of element on that row.
            index[i] = pdata + tmp_index[i];
         }
      }

      /**
       * Returns the matrix's number of rows.
       * @return number of rows.
       */
      inline Uint size1() const { return _size1; }

      /**
       * Returns the matrix's number of columns.
       * @return number of columns.
       */
      inline Uint size2() const { return _size2; }

      inline Uint getDataSize() const { return data.size(); }

      /**
       * Find the diagonal value for x.
       * @param x  value
       * @return equivalent of m*x.
       */
      inline Uint diag(Uint x) const {
         // Do the Banker's rounding.
         // explanation: http://www.cplusplus.com/forum/articles/3638/
         const double v=getM()*x;
         const Uint  rv(v);
         if (v-rv == 0.5 && rv%2 == 0)
            return v;
         else
            return v+0.5f;
      }

      /**
       * Get the slope aka m in mx+b.
       * @return  slope.
       */
      inline double getM() const { return m; }

      /**
       * Get the beamsize aka B in mx+b.
       * @return  beamsize.
       */
      inline Uint getB() const { return b; }

      /**
       * Returns a copy of the value at x, y.
       * @param x  row number [0, size1)
       * @param y  column number [minRange(x), maxRange(x))
       * @return a copy of the value at x, y in the matrix if it is in the banded or -infinity if not.
       */
      inline
      T get(Uint x, Uint y) const {
         //if (!inRange(x, y)) return -numeric_limits<T>::infinity();
         //if (!inRange(x, y)) return policy.outOfBandValue;
         if (!inRange(x, y)) return T();

         return *(index[x] + getPosition(x, y));
      }

      /**
       * Set the value at x, y.
       * A value will only be set if (x, y) is part of the band otherwise it is silently ignored.
       * @param x  row number [0, size1)
       * @param y  column number [minRange(x), maxRange(x))
       * @param v  value to set at x, y.
       */
      inline
      void set(Uint x, Uint y, const T& v) {
         if (inRange(x, y)) {
            *(index[x] + getPosition(x, y)) = v;
         }
      }

      /**
       * Calculates the minimum index part of the band on a row.
       * @param x  row
       * @return minimum column index part of the band on row x.
       */
      inline Uint minRange(Uint x) const {
         // This must be calculated in signed value.
         return range[x].first;
      }

      /**
       * Calculates +1 maximum index part of the band on a row.
       * @param x  row
       * @return +1 maximum column index part of the band on row x.
       */
      inline Uint maxRange(Uint x) const {
         return range[x].second;
      }

      /**
       * Is the coordinate part of the band?
       * @param x  row number
       * @param y  column number
       * @return true iff the coordinate (x, y) is part of the band.
       */
      inline bool inRange(Uint x, Uint y) const {
         return (minRange(x) <= y && y < maxRange(x));
      }

      /**
       * Display the matrix.
       * @param out  stream to display the matrix.
       */
      void print(ostream& out) const {
         const streamsize old = out.precision();
         out.precision(5);
         out << "size1:" << size1() << "; size2:" << size2() << "; m:" << getM() << "; b:" << getB();
         out << "; data.size:" << getDataSize() << "/" << size1()*size2() << endl;
         //for (Uint i(0); i<index.size(); ++i) out << index[i] << endl;  // SAM DEBUGGING
         for (Uint i(0); i<size1(); ++i) {
            out << get(i, 0);
            for (Uint j(1); j<size2(); ++j) {
               out << "\t" << get(i, j);
            }
            out << endl;
         }
         out.precision(old);  // Reset the precision to what it was.
      }

   private:
      /**
       * Converts the (x, y) coordinates into and offset in the array representing the matrix.
       * @param x  row number
       * @param y  column number
       * @return the offset of (x, y) in the internal array representing the matrix.
       */
      inline
      Uint getPosition(Uint x, Uint y) const {
#ifdef DEBUG_BANDED_MATRIX
         assert(x < size1());
         assert(y < size2());
#endif
         const Uint y1 = y - minRange(x);

#ifdef DEBUG_BANDED_MATRIX
         const bool assertion(0 <= y1 && index[x] + y1 < index[x+1]);
         if (!assertion) {
            cerr << "\nx:" << x << "; y:" << y << "; y1:" << y1 << "; m:" << getM() << "; b:" << getB();
            cerr << "; diag:" << diag(x) << "; size1:" << size1() << "; size2:" << size2() << endl; // SAM DEBUGGING
            cerr << "range(" << minRange(x) << ", " << maxRange(x) << ")" << endl;
            cerr << index[x] << ", " << index[x+1] << endl;
         }
         assert(assertion);
#endif

         return y1;
      }
};

}  // ends namespace Portage

#endif
