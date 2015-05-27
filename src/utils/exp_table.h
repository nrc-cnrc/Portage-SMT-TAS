/**
 * @author Samuel Larkin
 * @file exp_table.h
 * @brief Lookup table for exp, sigmoid & tanh.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2014, Her Majesty in Right of Canada
 */


#ifndef  __EXP__TABLE__
#define __EXP__TABLE__


#include <assert.h>
#include <cmath>  // exp
#include <cfloat>  // DBL_MAX

namespace Portage {
class ExpTable {
   private:
      const double m_epsilon;
      double m_max_exp;
      int m_size;
      double* m_exp_table;
      double* m_sig_table;
      double* m_tanh_table;

   public:
      ExpTable(double max_exp, int size)
      : m_epsilon(1e-12)
      , m_max_exp(max_exp)
      , m_size(size)
      , m_exp_table(NULL)
      , m_sig_table(NULL)
      , m_tanh_table(NULL)
      {
         assert(size > 0);
         m_exp_table  = new double[m_size];
         m_sig_table  = new double[m_size];
         m_tanh_table = new double[m_size];

         for (int i=0; i<m_size; ++i) {
            const double power = (2*i/(double)m_size -1) * m_max_exp;
            m_exp_table[i]  = ::exp(power);  // MUST be ::exp or else we will use this class's exp.
            m_sig_table[i]  = m_exp_table[i]  / (m_exp_table[i] + 1);
            const double ex = m_exp_table[i] * m_exp_table[i];
            m_tanh_table[i] = (ex - 1) / (ex + 1);
            /*  DEBUGGING
            cerr << i
                 << ' ' << power
                 << ' ' << m_exp_table[i]
                 << ' ' << m_sig_table[i]
                 << ' ' << m_tanh_table[i]
                 << endl;
            */
         }
      }

      ~ExpTable() {
         delete[] m_exp_table;
         delete[] m_sig_table;
         delete[] m_tanh_table;
      }

      int getIndex(double x) const {
         return (int)((x + m_max_exp) * (m_size / m_max_exp / 2) + 0.5);
      }

      double exp(double x) const {
         if(x < (-m_max_exp+m_epsilon)) return 0;
         if(x >= (m_max_exp-m_epsilon)) return DBL_MAX;
         return m_exp_table[getIndex(x)];
      }

      double sig(double x) const {
         if(x < (-m_max_exp+m_epsilon)) return 0.0;
         if(x >= (m_max_exp-m_epsilon)) return 1.0;
         return m_sig_table[getIndex(x)];
      }

      double tanh(double x) const {
         if(x < (-m_max_exp+m_epsilon)) return -1.0;
         if(x >= (m_max_exp-m_epsilon)) return 1.0;
         return m_tanh_table[getIndex(x)];
      }
};
}

#endif //__EXP__TABLE__
