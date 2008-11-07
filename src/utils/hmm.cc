/**
 * @author Eric Joanis
 * @file hmm.cc HMM method implementations
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "portage_defs.h"
#include "hmm.h"
#include "error.h"
#include "str_utils.h"
#include <iostream>
#include <cfloat>

#include <boost/numeric/ublas/matrix_proxy.hpp>
namespace Portage {
   typedef boost::numeric::ublas::matrix_row<dMatrix> dMatrixRow;
   typedef boost::numeric::ublas::matrix_column<dMatrix> dMatrixCol;
}

using namespace Portage;
using namespace std;

void HMM::convertToLogModel()
{
   assert(parm_type == regular_probs);
   for ( Uint i = 0; i < N; ++i )
      Pi(i) = log(Pi(i));
   for ( Uint i = 0; i < N; ++i )
      for ( Uint j = 0; j < N; ++j )
         A(i,j) = log(A(i,j));
   for ( Uint i = 0; i < B_storage.size(); ++i )
      for ( Uint j = 0; j < N; ++j )
         for ( Uint k = 0; k < M; ++k )
            B_storage[i](j,k) = log(B_storage[i](j,k));
   parm_type = log_probs;
}

bool HMM::checkTransitionDistributions() const
{
   bool result(true);

   // Pi
   assert(Pi_storage.size() == N);
   double sum(0.0);
   for ( Uint i = 0; i < N; ++i ) {
      if ( parm_type == regular_probs && Pi(i) < 0.0 ) {
         error(ETWarn, "Negative probability Pi(%d) = %f", i, Pi(i));
         result = false;
      }
      sum += parm_type == log_probs ? exp(Pi(i)) : Pi(i);
   }
   if ( abs(sum - 1.0) > 1e-6 ) {
      error(ETWarn, "Sum_i(Pi(i)) = %f != 1.0", sum);
      result = false;
   }

   // A
   for ( Uint i = 0; i < N; ++i ) {
      double sum(0.0);
      for ( Uint j = 0; j < N; ++j ) {
         if ( parm_type == regular_probs && A(i,j) < 0.0 ) {
            error(ETWarn, "Negative probability A(%d,%d) = %f", i, j, A(i,j));
            result = false;
         }
         sum += parm_type == log_probs ? exp(A(i,j)) : A(i,j);
      }
      if ( abs(sum - 1.0) > 1e-6 ) {
         error(ETWarn, "Sum_j(A(%d,j)) = %f != 1.0", i, sum);
         result = false;
      }
   }

   return result;
} // HMM::checkTransitionDistributions()

bool HMM::checkEmissionDistributions(bool complete) const
{
   bool result(true);

   if ( emission_type == arc_emit ) {
      assert(B_storage.size() == N);
      for ( Uint i = 0; i < N; ++i ) {
         for ( Uint j = 0; j < N; ++j ) {
            double sum(0.0);
            for ( Uint k = 0; k < M; ++k ) {
               if ( parm_type == regular_probs && B(i,j,k) < 0.0 ) {
                  error(ETWarn, "Negative probability B(%d,%d,%d) = %f",
                        i, j, k, B(i,j,k));
                  result = false;
               }
               sum += parm_type == log_probs ? exp(B(i,j,k)) : B(i,j,k);
            }
            if ( complete ) {
               if ( abs(sum - 1.0) > 1e-6 ) {
                  error(ETWarn, "Sum_k(B(%d,%d,k)) = %f != 1.0",
                        i, j, sum);
                  result = false;
               }
            } else {
               if ( sum > 1.0 ) {
                  error(ETWarn, "Sum_k(B(%d,%d,k)) = %f > 1.0",
                        i, j, sum);
                  result = false;
               }
            }
         }
      }
   } else {
      assert(B_storage[0].size1() == N);
      assert(B_storage[0].size2() == M);
      for ( Uint i = 0; i < N; ++i ) {
         double sum(0.0);
         for ( Uint k = 0; k < M; ++k ) {
            if ( parm_type == regular_probs && B(i,k) < 0.0 ) {
               error(ETWarn, "Negative probability B(%d,%d) = %f",
                     i, k, B(i,k));
               result = false;
            }
            sum += parm_type == log_probs ? exp(B(i,k)) : B(i,k);
         }
         if ( complete ) {
            if ( abs(sum - 1.0) > 1e-6 ) {
               error(ETWarn, "Sum_k(B(%d,k)) = %f != 1.0", i, sum);
               result = false;
            }
         } else {
            if ( sum > 1.0 ) {
               //error(ETWarn, "Sum_k(B(%d,k)) = %f > 1.0", i, sum);
               //result = false; // Tolerate this too.
            } else if ( sum < 0.0 ) {
               error(ETWarn, "Sum_k(B(%d,k)) = %f < 0.0", i, sum);
               result = false;
            } else if ( sum <= 1e-6 ) {
               //error(ETWarn, "Sum_k(B(%d,k)) = %f very small", i, sum);
               //result = false;
            }
         }
      }
   }

   return result;
} // HMM::checkEmissionDistributions()

HMM::HMM(Uint N, Uint M, EmissionType emission_type, ParameterType parm_type)
   : N(N)
   , M(M)
   , emission_type(emission_type)
   , parm_type(parm_type)
   , Pi_storage(N)
   , A_storage(N,N)
   , B_storage(emission_type == arc_emit ? N : 1)
{
   assert(N > 0);
   assert(M > 0);

   // Initialize Pi
   if ( parm_type == log_probs ) {
      Pi(0) = 0.0;
      for ( Uint i = 1; i < N; ++i )
         Pi(i) = -INFINITY;
   } else {
      Pi(0) = 1.0;
      for ( Uint i = 1; i < N; ++i )
         Pi(i) = 0.0;
   }

   // Allocate B's sub-matrices
   for ( Uint i = 0; i < B_storage.size(); ++i )
      B_storage[i].resize(N,M);
}

HMM::~HMM() {
}

void HMM::write(ostream& os) const
{
   os << "Start HMM" << endl
      << "N = " << N << endl
      << "M = " << M << endl
      << "emission_type = " << Uint(emission_type)
      << " (" << ( emission_type == arc_emit ? "arc" : "state" )
      << ")" << endl;

   os << "Pi:";
   for ( Uint i = 0; i < N; ++i ) os << " " << Pi(i);
   os << endl;

   os << "A:" << endl;
   for ( Uint i(0); i < N; ++i ) {
      os << " i=" << i << ":";
      for ( Uint j(0); j < N; ++j )
         os << " " << A(i,j);
      os << endl;
   }

   os << "B:" << endl;
   for ( Uint i = 0; i < (emission_type == arc_emit ? N : 1); ++i ) {
      if ( emission_type == arc_emit )
         os << " i=" << i << endl;
      for ( Uint j = 0; j < N; ++j ) {
         os << "  j=" << j << ":";
         for ( Uint k = 0; k < M; ++k )
            os << " " << (emission_type == arc_emit ? B(i,j,k) : B(j,k));
         os << endl;
      }
   }

   os << "End HMM" << endl;
}

double HMM::probViterbi(const uintVector &O, uintVector &X_hat,
                        bool verbose) const
{
   assert (parm_type == regular_probs);
   Uint T = O.size();
   X_hat.resize(T+1, false);
   // O_t = O[t-1] because there is no O_0
   // X^_t = X_hat[t], for t in [0, T] (i.e., T+1 distinct values)
   // delta_j(t) = delta(t,j)
   dMatrix delta(T+1, N);
   // psi_j(t) = psi(t,j) -- for simplicity, we store but don't use psi_j(0)
   uintMatrix psi(T+1, N);

   double log_maxprob(0.0);

   // 1. Initialization
   for ( Uint j = 0; j < N; ++j ) {
      delta(0,j) = Pi(j);
      if ( verbose )
         cerr << "delta_" << j << "(0) = " << delta(0,j) << endl;
   }
   // 2. Induction
   for ( Uint t = 1; t <= T; ++t ) {
      double max_delta_at_t(-1.0);
      const HMMPrecision *delta_t_1 = &(delta(t-1,0)); // For faster access
      assert(&(delta_t_1[1]) == &(delta(t-1,1)));
      for ( Uint j = 0; j < N; ++j ) {
         // delta_j(t) = max_{0<=i<N} (delta_i(t-1) x A_ij x b_ijO_t)
         double max_prod(-1.0);
         int argmax_i(-1);
         /*
         for ( Uint i = 0; i < N; ++i ) {
            double prod = delta(t-1,i) * A(i,j) * B(i,j,O[t-1]);
            //if ( verbose ) cerr << " i=" << i << " prod=" << prod << endl;
            if ( prod > max_prod ) {
               max_prod = prod;
               argmax_i = i;
            }
         }
         */
         // Uglier, but significantly faster (checked using profiling)
         if ( emission_type == arc_emit ) {
            for ( Uint i = 0; i < N; ++i ) {
             //const double prod = delta(t-1,i) * A(i,j) * B_storage[i](j,O[t-1]);
               const double prod = delta_t_1[i] * A(i,j) * B_storage[i](j,O[t-1]);
               if ( prod > max_prod || argmax_i == -1 ) {
                  max_prod = prod;
                  argmax_i = i;
               }
            }
         } else if ( emission_type == state_entry_emit ) {
            const HMMPrecision B_val = B_storage[0](j,O[t-1]);
            for ( Uint i = 0; i < N; ++i ) {
             //const double prod = delta(t-1,i) * A(i,j) * B_storage[0](j,O[t-1]);
               const double prod = delta_t_1[i] * A(i,j) * B_val;
               if ( prod > max_prod || argmax_i == -1 ) {
                  max_prod = prod;
                  argmax_i = i;
               }
            }
         } else if ( emission_type == state_exit_emit ) {
            for ( Uint i = 0; i < N; ++i ) {
             //const double prod = delta(t-1,i) * A(i,j) * B_storage[0](i,O[t-1]);
               const double prod = delta_t_1[i] * A(i,j) * B_storage[0](i,O[t-1]);
               if ( prod > max_prod || argmax_i == -1 ) {
                  max_prod = prod;
                  argmax_i = i;
               }
            }
         } else assert(false);

         assert ( argmax_i != -1 );
         delta(t,j) = max_prod;
         psi(t,j) = argmax_i;
         if ( verbose )
            cerr << "delta_" << j << "(" << t << ") = " << max_prod
                 << " psi_" << j << "(" << t << ") = " << argmax_i
                 << endl;

         if ( max_prod > max_delta_at_t )
            max_delta_at_t = max_prod;
      }

      if ( max_delta_at_t < FLT_MIN ) {
         //error(ETWarn, "max_delta_at_t = %g < %g = FLT_MIN - rescaling",
         //      max_delta_at_t, FLT_MIN);
         // To avoid underflow, rescale and store the log scaling factor to
         // finish calculating the global logprob
         log_maxprob += log(max_delta_at_t);
         double scaling_factor(1.0/max_delta_at_t);
         for ( Uint j(0); j < N; ++j )
            delta(t,j) *= scaling_factor;
      }
   }
   // 3. Termination and path readout
   Uint argmax_i = 0;
   double maxprob = delta(T,0);
   for ( Uint i = 1; i < N; ++i ) {
      if ( delta(T,i) > maxprob ) {
         argmax_i = i;
         maxprob = delta(T,i);
      }
   }
   log_maxprob += log(maxprob);
   X_hat[T] = argmax_i;
   if ( verbose ) cerr << "X^_" << T << " = " << X_hat[T] << endl;
   for ( int t(T-1); t >= 0; --t ) {
      X_hat[t] = psi(t+1,X_hat[t+1]);
      if ( verbose ) cerr << "X^_" << t << " = " << X_hat[t] << endl;
   }

   if ( maxprob == 0.0 ) {
      write(cerr);
      error(ETWarn, "Possible underflow problem: probViterbi found prob of 0.0");
   }

   return log_maxprob;
} // HMM::Viterbi()

double HMM::logViterbi(const uintVector &O, uintVector &X_hat,
                       bool verbose) const
{
   assert (parm_type == log_probs);
   Uint T = O.size();
   X_hat.resize(T+1, false);
   // O_t = O[t-1] because there is no O_0
   // X^_t = X_hat[t], for t in [0, T] (i.e., T+1 distinct values)
   // delta_j(t) = delta(t,j)
   dMatrix delta(T+1, N);
   // psi_j(t) = psi(t,j) -- for simplicity, we store but don't use psi_j(0)
   uintMatrix psi(T+1, N);

   // 1. Initialization
   for ( Uint j = 0; j < N; ++j ) {
      delta(0,j) = Pi(j);
      if ( verbose )
         cerr << "log-delta_" << j << "(0) = " << delta(0,j) 
              << " delta_" << j << "(0) = " << exp(delta(0,j))
              << endl;
   }
   // 2. Induction
   for ( Uint t = 1; t <= T; ++t ) {
      const HMMPrecision *delta_t_1 = &(delta(t-1,0)); // For faster access
      assert(&(delta_t_1[1]) == &(delta(t-1,1)));
      for ( Uint j = 0; j < N; ++j ) {
         // delta_j(t) = max_{0<=i<N} (delta_i(t-1) x A_ij x b_ijO_t)
         double max_prod(-INFINITY);
         int argmax_i(-1);
         /*
         for ( Uint i = 0; i < N; ++i ) {
            double prod = delta(t-1,i) + A(i,j) + B(i,j,O[t-1]);
            //if ( verbose ) cerr << " i=" << i << " prod=" << prod << endl;
            if ( prod > max_prod ) {
               max_prod = prod;
               argmax_i = i;
            }
         }
         */
         // Uglier, but significantly faster (checked using profiling)
         if ( emission_type == arc_emit ) {
            for ( Uint i = 0; i < N; ++i ) {
             //const double prod = delta(t-1,i) + A(i,j) + B_storage[i](j,O[t-1]);
               const double prod = delta_t_1[i] + A(i,j) + B_storage[i](j,O[t-1]);
               if ( prod > max_prod || argmax_i == -1 ) {
                  max_prod = prod;
                  argmax_i = i;
               }
            }
         } else if ( emission_type == state_entry_emit ) {
            const HMMPrecision B_val = B_storage[0](j,O[t-1]);
            for ( Uint i = 0; i < N; ++i ) {
             //const double prod = delta(t-1,i) + A(i,j) + B_storage[0](j,O[t-1]);
               const double prod = delta_t_1[i] + A(i,j) + B_val;
               if ( prod > max_prod || argmax_i == -1 ) {
                  max_prod = prod;
                  argmax_i = i;
               }
            }
         } else if ( emission_type == state_exit_emit ) {
            for ( Uint i = 0; i < N; ++i ) {
             //const double prod = delta(t-1,i) + A(i,j) + B_storage[0](i,O[t-1]);
               const double prod = delta_t_1[i] + A(i,j) + B_storage[0](i,O[t-1]);
               if ( prod > max_prod || argmax_i == -1 ) {
                  max_prod = prod;
                  argmax_i = i;
               }
            }
         } else assert(false);

         delta(t,j) = max_prod;
         psi(t,j) = argmax_i;
         if ( verbose )
            cerr << "log-delta_" << j << "(" << t << ") = " << max_prod
                 << " delta_" << j << "(" << t << ") = " << exp(max_prod)
                 << " psi_" << j << "(" << t << ") = " << argmax_i
                 << endl;

         if ( argmax_i == -1 ) {
            // This probably means we have an underflow problem.
            // Rerun self in verbose mode and then die
            if ( verbose ) {
               write(cerr);
               assert ( argmax_i != -1 );
            } else {
               logViterbi(O, X_hat, true);
            }
         }
      }
   }
   // 3. Termination and path readout
   Uint argmax_i = 0;
   double maxprob = delta(T,0);
   for ( Uint i = 1; i < N; ++i ) {
      if ( delta(T,i) > maxprob ) {
         argmax_i = i;
         maxprob = delta(T,i);
      }
   }
   X_hat[T] = argmax_i;
   if ( verbose ) cerr << "X^_" << T << " = " << X_hat[T] << endl;
   for ( int t(T-1); t >= 0; --t ) {
      X_hat[t] = psi(t+1,X_hat[t+1]);
      if ( verbose ) cerr << "X^_" << t << " = " << X_hat[t] << endl;
   }

   if ( maxprob == -INFINITY ) {
      write(cerr);
      error(ETWarn, "Massive underflow or bad HMM problem: logViterbi found "
                    "log-prob of -inf");
   }

   return maxprob;
} // HMM::logViterbi()

double HMM::statePosteriors(const uintVector &O, dMatrix &gamma,
                            bool verbose) const
{
   assert(parm_type == regular_probs);
   dMatrix alpha_hat;
   dVector c;
   double log_pr = ForwardProcedure(O, alpha_hat, c, true, verbose);

   dMatrix beta_hat;
   BackwardProcedure(O, c, beta_hat, verbose);
   
   statePosteriors(O, alpha_hat, beta_hat, gamma, verbose);
   return log_pr;
} // HMM::statePosteriors()

void HMM::statePosteriors(const uintVector &O,
                          const dMatrix &alpha_hat, const dMatrix &beta_hat,
                          dMatrix& gamma, bool verbose) const
{
   assert(parm_type == regular_probs);
   Uint T = O.size();
   assert(alpha_hat.size1() == T+1);
   assert(alpha_hat.size2() == N);
   assert(beta_hat.size1() == T+1);
   assert(beta_hat.size2() == N);
   gamma.resize(T+1, N, false);

   double numerators[N];
   for ( Uint t = 0; t <= T; ++t ) {
      double denom(0);
      for ( Uint i = 0; i < N; ++i )
         denom += numerators[i] = alpha_hat(t, i) * beta_hat(t, i);
      for ( Uint i = 0; i < N; ++i )
         gamma(t, i) = numerators[i] / denom;
   }

   if ( verbose ) {
      cerr << "ForwardBackward decoding results:" << endl;
      for ( Uint t = 0; t <= T; ++t ) {
         cerr << "  t = " << t << endl;
         for ( Uint i = 0; i < N; ++i ) {
            cerr << "    gamma_" << i << "(" << t << ") = "
                 << gamma(t,i) << endl;
         }
      }
   }
}

void HMM::transitionPosteriors(
      const uintVector &O,
      const dMatrix &alpha_hat, const dMatrix &beta_hat,
      vector<dMatrix>& p, bool verbose) const
{
   const Uint T = O.size();
   p.resize(T);
   for ( Uint t = 0; t < T; ++t ) {
      transitionPosteriors(O, alpha_hat, beta_hat, t, p[t], verbose);
   }
}

void HMM::transitionPosteriors(const uintVector &O, 
                               const dMatrix &alpha_hat,
                               const dMatrix &beta_hat,
                               Uint t, dMatrix& p_t, bool verbose) const
{
   double p_t_denom(0.0);
   if ( p_t.size1() != N || p_t.size2() != N )
      p_t.resize(N,N,false);

   /* Cleaner design, but B(i,j,O[t]) is done too often and is expensive
    * This code is equivalent to the switch/case statement below.
   for ( Uint i = 0; i < N; ++i ) {
      for ( Uint j = 0; j < N; ++j ) {
         p_t_denom +=
            p_t(i,j) =
               alpha_hat(t,i) * A(i,j) * B(i,j,O[t]) * beta_hat(t+1,j);
      }
   }
    */
   // Uglier, but significantly faster (checked using profiling)
   switch (emission_type) {
      case arc_emit: {
         for ( Uint i = 0; i < N; ++i ) {
            const HMMPrecision alpha_hat_t_i = alpha_hat(t,i);
            for ( Uint j = 0; j < N; ++j ) {
               p_t_denom +=
                  p_t(i,j) =
                     alpha_hat_t_i * A(i,j)
                      * B_storage[i](j,O[t]) // * B(i,j,O[t])
                      * beta_hat(t+1,j);
            }
         }
      } break;
      case state_entry_emit: {
         // Uglier but faster - optimized to reduce matrix access
         HMMPrecision B_times_beta_hat[N];
         const HMMPrecision *beta_hat_t_1 = &beta_hat(t+1,0);
         for ( Uint j = 0; j < N; ++j )
            B_times_beta_hat[j] = B_storage[0](j,O[t]) * beta_hat_t_1[j];
         for ( Uint i = 0; i < N; ++i ) {
            const HMMPrecision alpha_hat_t_i = alpha_hat(t,i);
            const HMMPrecision *A_i = &A(i,0);
            HMMPrecision *p_t_i = &p_t(i,0);
            for ( Uint j = 0; j < N; ++j ) {
               p_t_denom +=
                  //p_t(i,j) =
                  p_t_i[j] =
                     alpha_hat_t_i  * A_i[j] * B_times_beta_hat[j];
                   //alpha_hat(t,i) * A(i,j)
                      //* B_storage[0](j,O[t]) // * B(i,j,O[t])
                      //* beta_hat(t+1,j);
            }
         }
      } break;
      case state_exit_emit: {
         for ( Uint i = 0; i < N; ++i ) {
            const HMMPrecision alpha_hat_t_i = alpha_hat(t,i);
            for ( Uint j = 0; j < N; ++j ) {
               p_t_denom +=
                  p_t(i,j) =
                     alpha_hat_t_i * A(i,j)
                      * B_storage[0](i,O[t]) // * B(i,j,O[t])
                      * beta_hat(t+1,j);
            }
         }
      } break;
      default:
         assert(false);
   }
   for ( Uint i = 0; i < N; ++i ) {
      HMMPrecision *p_t_i = &p_t(i,0);
      for ( Uint j = 0; j < N; ++j ) {
         // p_t(i,j) /= p_t_denom; // slower
         p_t_i[j] /= p_t_denom;    // faster
      }
   }
} // HMM::transitionPosteriors()


double HMM::ForwardProcedure(const uintVector &O, dMatrix &alpha_hat,
                             dVector &c, bool scale, bool verbose) const
{
   assert (parm_type == regular_probs);
   Uint T = O.size();
   alpha_hat.resize(T+1, N, false);
   c.resize(T+1, false);

   // alpha_hat_t_1 temporarily holds alpha_hat(t-1,_) for faster access.
   HMMPrecision alpha_hat_t_1_storage[N];
   HMMPrecision *alpha_hat_t_1 = alpha_hat_t_1_storage;
   // alpha_hat_t temporarily holds alpha_hat(t,_) for faster access.
   HMMPrecision alpha_hat_t_storage[N];
   HMMPrecision *alpha_hat_t = alpha_hat_t_storage;

   // 1. Initialization
   for ( Uint i = 0; i < N; ++i )
      alpha_hat_t_1[i] = alpha_hat(0, i) = Pi(i);
   c[0] = 1.0;

   bool bad_scale = false; // will be set to true if scaling fails at any step
   // 2. Induction
   for ( Uint t = 1; t <= T; ++t ) {
      double sum_alpha_t(0.0);
      for ( Uint j = 0; j < N; ++j ) {
       //alpha_hat(t,j) = 0.0;
         alpha_hat_t[j] = 0.0;
         //for ( Uint i = 0; i < N; ++i )
         //   alpha_hat(t, j) += alpha_hat(t-1,i) * A(i,j) * B(i,j,O(t-1));
         // Uglier, but significantly faster (checked using profiling)
         if ( emission_type == arc_emit ) {
            for ( Uint i = 0; i < N; ++i )
             //alpha_hat(t,j) += alpha_hat(t-1,i) * A(i,j) 
               alpha_hat_t[j] += alpha_hat_t_1[i] * A(i,j) 
                                  * B_storage[i](j,O(t-1)); // * B(i,j,O(t-1));
         } else if ( emission_type == state_entry_emit ) {
            const HMMPrecision B_val = B_storage[0](j,O(t-1));
            for ( Uint i = 0; i < N; ++i )
             //alpha_hat(t,j) += alpha_hat(t-1,i) * A(i,j) * B_val;
               alpha_hat_t[j] += alpha_hat_t_1[i] * A(i,j) * B_val;
                                //* B_storage[0](j,O(t-1)); // * B(i,j,O(t-1));
         } else if ( emission_type == state_exit_emit ) {
            for ( Uint i = 0; i < N; ++i )
             //alpha_hat(t,j) += alpha_hat(t-1,i) * A(i,j) 
               alpha_hat_t[j] += alpha_hat_t_1[i] * A(i,j) 
                                  * B_storage[0](i,O(t-1)); // * B(i,j,O(t-1));
         } else assert(false);

       //sum_alpha_t += alpha_hat(t, j);
         sum_alpha_t += alpha_hat_t[j];
      }
      // Scaling a la Rabiner
      if ( scale && sum_alpha_t == 0.0 ) bad_scale = true;
      c[t] = (scale && sum_alpha_t != 0.0) ? 1.0/sum_alpha_t : 1.0;
      if ( verbose ) cerr << "c_" << t << " = " << c[t] << endl;
      for ( Uint j = 0; j < N; ++j ) {
         if ( scale ) {
            if ( verbose )
               cerr << "alpha_" << j << "(" << t << ") = "
                  //<< alpha_hat(t,j) << "   ";
                    << alpha_hat_t[j] << "   ";
          //alpha_hat(t,j) *= c[t];
            alpha_hat_t[j] *= c[t];
         }
         if ( verbose )
            cerr << "alpha^_" << j << "(" << t << ") = "
               //<< alpha_hat(t,j) << endl;
                 << alpha_hat_t[j] << endl;
      }
      // Copy alpha_hat_t into alpha_hat(t,_), now that it's fully calculated
      for ( Uint j = 0; j < N; ++j )
         alpha_hat(t,j) = alpha_hat_t[j];
      // The next alpha_hat(t-1,_) is the current alpha_hat(t,_), so swap
      // alpha_hat_t_1 and alpha_hat_t
      swap(alpha_hat_t, alpha_hat_t_1);
   }

   // 3. Total : P(O|mu) = 1/prod_{t=0..T}(c[t]).
   // done in logs to avoid underflow
   double logprob(0.0);
   for ( Uint t = 0; t <= T; ++t )
      logprob += -log(c[t]);
   if ( !scale || bad_scale ) {
      double sum_alpha(0.0);
      for ( Uint i = 0; i < N; ++i )
         sum_alpha += alpha_hat(T,i);
      logprob += log(sum_alpha);
   }

   if ( verbose ) {
      double prod_c(1.0);
      for ( Uint t = 0; t <= T; ++t )
         prod_c *= c[t];
      double sum_alpha(0.0);
      for ( Uint i = 0; i < N; ++i )
         sum_alpha += alpha_hat(T,i);
      cerr << "Sum_{i=0..N-1}(alpha_hat_i(T)) = " << sum_alpha << endl
           << "Prod_{t=0..T}(c) = " << prod_c << endl
           << "Log_prob = " << logprob << endl
           << "Forward Procedure says P(O|mu) = " << sum_alpha/prod_c << endl;
   }

   return logprob;
} // HMM::ForwardProcedure()

void HMM::BackwardProcedure(const uintVector &O, const dVector& c,
                            dMatrix &beta_hat, bool verbose) const
{
   assert (parm_type == regular_probs);
   Uint T = O.size();
   assert ( c.size() == T + 1 );
   beta_hat.resize(T+1, N, false);

   // Make sure A being row-major means what we think, since we rely on it.
   assert(&((&A(1,0))[1]) == &A(1,1));

   // beta_hat_t_1 temporarily holds beta_hat(t+1,_) for faster access.
   HMMPrecision beta_hat_t_1_storage[N];
   HMMPrecision *beta_hat_t_1 = beta_hat_t_1_storage;
   // beta_hat_t temporarily holds beta_hat(t,_) for faster access.
   HMMPrecision beta_hat_t_storage[N];
   HMMPrecision *beta_hat_t = beta_hat_t_storage;

   // 1. Initialization
   for ( Uint i = 0; i < N; ++i ) {
      if ( verbose )
         cerr << "beta_" << i << "(" << T << ") = 1.0";
      beta_hat_t_1[i] = beta_hat(T,i) = 1.0 * c[T];
      if ( verbose )
         cerr << "   beta^_" << i << "(" << T << ") = " << beta_hat(T,i)
              << endl;
   }

   // 2. Induction
   HMMPrecision B_vals[emission_type==state_entry_emit ? N : 0];
   for ( int t = T-1; t >= 0; --t ) {
      // Faster access to B_storage values for state_entry_emit HMMs.
      if ( emission_type == state_entry_emit )
         for ( Uint j = 0; j < N; ++j )
            B_vals[j] = B_storage[0](j,O[t]);
      for ( Uint i = 0; i < N; ++i ) {
       //beta_hat(t,i) = 0.0;
         beta_hat_t[i] = 0.0;
         //for ( Uint j = 0; j < N; ++j )
         //   beta_hat(t,i) += A(i,j) * B(i,j,O[t]) * beta_hat(t+1,j);
         // Uglier, but significantly faster (checked using profiling)
         const HMMPrecision *A_i = &A(i,0);
         if ( emission_type == arc_emit ) {
            for ( Uint j = 0; j < N; ++j )
             //beta_hat(t,i) += A(i,j) 
               beta_hat_t[i] += A_i[j] 
                                * B_storage[i](j,O[t]) // * B(i,j,O[t])
                                * beta_hat_t_1[j];
                              //* beta_hat(t+1,j);
         } else if ( emission_type == state_entry_emit ) {
            for ( Uint j = 0; j < N; ++j )
             //beta_hat(t,i) += A(i,j) 
               beta_hat_t[i] += A_i[j]
                                * B_vals[j]
                              //* B_storage[0](j,O[t]) // * B(i,j,O[t])
                                * beta_hat_t_1[j];
                              //* beta_hat(t+1,j);
         } else if ( emission_type == state_exit_emit ) {
            const HMMPrecision B_val = B_storage[0](i,O[t]);
            for ( Uint j = 0; j < N; ++j )
             //beta_hat(t,i) += A(i,j) 
               beta_hat_t[i] += A_i[j] * B_val
                              //* B_storage[0](i,O[t]) // * B(i,j,O[t])
                                * beta_hat_t_1[j];
                              //* beta_hat(t+1,j);
         } else assert(false);
            /*
            if ( verbose )
               cerr << "A(" << i << "," << j << ")=" << A(i,j)
                    << " B(" << i << "," << j << "," << O[t] << ")="
                    << B(i,j,O[t])
                  //<< " prod = " << A(i,j) * B(i,j,O[t]) * beta_hat(t+1,j)
                    << " prod = " << A(i,j) * B(i,j,O[t]) * beta_hat_t_1[j]
                    << endl;
            */
         if ( verbose )
          //cerr << "beta_" << i << "(" << t << ") = " << beta_hat(t,i);
            cerr << "beta_" << i << "(" << t << ") = " << beta_hat_t[i];
         // Scaling a la Rabiner
       //beta_hat(t,i) *= c[t];
         beta_hat_t[i] *= c[t];
         if ( verbose )
          //cerr << "   beta^_" << i << "(" << t << ") = " << beta_hat(t,i)
            cerr << "   beta^_" << i << "(" << t << ") = " << beta_hat_t[i]
                 << endl;
      }
      // Copy beta_hat_t into beta_hat(t,_), now that it's fully calculated
      for ( Uint i = 0; i < N; ++i )
         beta_hat(t,i) = beta_hat_t[i];
      // The next beta_hat(t+1,_) is the current beta_hat(t,_), so swap
      // beta_hat_t_1 and beta_hat_t
      swap(beta_hat_t, beta_hat_t_1);
   }

   // 3. Total : P(O|mu) = sum_{i=0..N-1}(Pi(i)*beta_hat(0,i)) / prod(c[t])
   // (Again, we don't bother with this unecessary calculation.)
   if ( verbose ) {
      double sum(0.0);
      for ( Uint i = 0; i < N; ++i )
         sum += Pi(i) * beta_hat(0,i);
      cerr << "Sum_{i=0..N-1}(Pi_i*beta^_i(0)) = " << sum << endl;
      double prod_c(1.0);
      for ( Uint t = 0; t <= T; ++t )
         prod_c *= c[t];
      cerr << "Backward Procedure says P(O|mu) = " << sum / prod_c << endl;
   }
} // HMM::BackwardProcedure()

void HMM::BWCountExpectation(const uintVector &O, const dMatrix &alpha_hat,
                             const dMatrix &beta_hat,
                             dVector &Pi_counts, dMatrix &A_counts,
                             vector<dMatrix> &B_counts, bool verbose) const
{
   assert(parm_type == regular_probs);
   // Throughout this procedure, we use the Manning + Schuetze (hereafter, M+S)
   // equations, despite the fact that they don't reflect the Rabiner-style
   // rescalling.  This is correct because in all cases, the c_t constants that
   // should be added to the numerator exactly cancel out those that should be
   // added to the denominator.  So we systematically use alpha_hat and
   // beta_hat in lieu of alpha and beta.

   Uint T = O.size();
   assert(alpha_hat.size1() == T+1);
   assert(alpha_hat.size2() == N);
   assert(beta_hat.size1() == T+1);
   assert(beta_hat.size2() == N);

   // Make sure A being row-major means what we think, since we rely on it.
   assert(&((&A(1,0))[1]) == &A(1,1));

   // Pi counts = Pi reestimated since the transition to an initial state is
   // not a transition "from" any state.
   // M+S eqns 9.17 and 9.13
   Pi_counts.resize(N, false);
   double sum_Pi_counts(0.0);
   for ( Uint i = 0; i < N; ++i ) {
      Pi_counts[i] = alpha_hat(0,i) * beta_hat(0,i);
      sum_Pi_counts += Pi_counts[i];
   }
   for ( Uint i = 0; i < N; ++i )
      Pi_counts[i] /= sum_Pi_counts;

   // A counts = transition counts.
   // B counts = emission counts.
   // Not normalized here - the user will do that once they've taken into
   // account any tied variables or other kinds of tied parameters.  So
   // A_counts are only the numerator in M+S eqn 9.18, and B_counts are the
   // numerator in M+S eqn 9.19.
   A_counts.resize(N,N,false);
   A_counts.clear(); // zero all values

   B_counts.resize(emission_type == arc_emit ? N : 1);
   for ( Uint i = 0; i < B_counts.size(); ++i ) {
      B_counts[i].resize(N,M);
      B_counts[i].clear(); // zero all values
   }

   dMatrix p_t(N,N); // Reused for each t
   for ( Uint t = 0; t < T; ++t ) {
      // Calculate the transition posteriors p_t(i,j) in M+S eqn 9.16
      transitionPosteriors(O, alpha_hat, beta_hat, t, p_t, verbose);
      /* Cleaner design, but the switch/case statement is done too often!
      for ( Uint i = 0; i < N; ++i ) {
         for ( Uint j = 0; j < N; ++j ) {
            const double p_t_i_j = p_t(i,j);
            A_counts(i,j) += p_t_i_j;
            switch (emission_type) {
               case arc_emit:         B_counts[i](j,O[t]) += p_t_i_j; break;
               case state_entry_emit: B_counts[0](j,O[t]) += p_t_i_j; break;
               case state_exit_emit:  B_counts[0](i,O[t]) += p_t_i_j; break;
               default: assert(false);
            }
         }
      }
      */
      // Uglier, but significantly faster (checked using profiling)
      switch (emission_type) {
         case arc_emit: {
            for ( Uint i = 0; i < N; ++i ) {
               for ( Uint j = 0; j < N; ++j ) {
                  const double p_t_i_j = p_t(i,j);
                  A_counts(i,j) += p_t_i_j;
                  B_counts[i](j,O[t]) += p_t_i_j;
               }
            }
         } break;
         case state_entry_emit: {
            // Uglier but faster - optimized to reduce matrix access
            for ( Uint j = 0; j < N; ++j ) {
               HMMPrecision B_count_j_t(0.0);
               for ( Uint i = 0; i < N; ++i ) {
                  const double p_t_i_j = p_t(i,j);
                  A_counts(i,j) += p_t_i_j;
                  //B_counts[0](j,O[t]) += p_t_i_j;
                  B_count_j_t += p_t_i_j;
               }
               B_counts[0](j,O[t]) += B_count_j_t;
            }
         } break;
         case state_exit_emit: {
            for ( Uint i = 0; i < N; ++i ) {
               HMMPrecision B_counts_i_t(0.0);
               for ( Uint j = 0; j < N; ++j ) {
                  const double p_t_i_j = p_t(i,j);
                  A_counts(i,j) += p_t_i_j;
                  //B_counts[0](i,O[t]) += p_t_i_j;
                  B_counts_i_t += p_t_i_j;
               }
               B_counts[0](i,O[t]) += B_counts_i_t;
            }
         } break;
      }
   }

   if ( verbose ) {
      cerr << "CountExpectation results:" << endl;
      cerr << "Pi counts: " << joini(Pi_counts.begin(), Pi_counts.end()) << endl;
      cerr << "A counts:" << endl;
      for ( Uint i = 0; i < N; ++i ) {
         dMatrixRow row(A_counts, i);
         cerr << " i=" << i << ": " << joini(row.begin(), row.end()) << endl;
      }
      cerr << "B counts:" << endl;
      for ( Uint i = 0; i < B_counts.size(); ++i ) {
         if ( emission_type == arc_emit )
            cerr << " i=" << i << endl;
         for ( Uint j = 0; j < N; ++j ) {
            dMatrixRow row(B_counts[i], j);
            cerr << "  j=" << j << ": " << joini(row.begin(), row.end()) << endl;
         }
      }
   }
} // HMM::BWCountExpectation()

void HMM::BWReestimate(const dVector &Pi_counts, const dMatrix &A_counts,
                              const vector<dMatrix> &B_counts,
                              bool verbose)
{
   assert(parm_type == regular_probs);
   assert(Pi_counts.size() == N);
   assert(A_counts.size1() == N);
   assert(A_counts.size2() == N);
   assert(B_counts.size() == (emission_type == arc_emit ? N : 1));
   for ( Uint i = 0; i < B_counts.size(); ++i ) {
      assert(B_counts[i].size1() == N);
      assert(B_counts[i].size2() == M);
   }

   if ( verbose ) cerr << "Reestimating Pi" << endl;
   double Pi_sum(0.0);
   for ( Uint i = 0; i < N; ++i )
      Pi_sum += Pi_counts[i];
   for ( Uint i = 0; i < N; ++i )
      Pi(i) = Pi_counts[i] / Pi_sum;

   if ( verbose ) cerr << "Reestimating A" << endl;
   for ( Uint i = 0; i < N; ++i ) {
      double a_i_sum(0.0);
      for ( Uint j = 0; j < N; ++j )
         a_i_sum += A_counts(i,j);
      for ( Uint j = 0; j < N; ++j )
         A(i,j) = A_counts(i,j) / a_i_sum;
   }

   if ( verbose ) cerr << "Reestimating B" << endl;
   if ( emission_type == arc_emit ) {
      for ( Uint i = 0; i < N; ++i )
         for ( Uint j = 0; j < N; ++j )
            for ( Uint k = 0; k < M; ++k )
               B(i,j,k) = B_counts[i](j,k) / A_counts(i,j);
   } else {
      assert(B_counts[0].size1() == N);
      assert(B_counts[0].size2() == M);
      for ( Uint i = 0; i < N; ++i ) {
         double b_i_sum(0.0);
         for ( Uint k = 0; k < M; ++k )
            b_i_sum += B_counts[0](i,k);
         if ( verbose ) cerr << "b_" << i << "_sum = " << b_i_sum << endl;
         for ( Uint k = 0; k < M; ++k )
            B(i,k) = B_counts[0](i,k) / b_i_sum;
      }
   }
} // HMM::BWReestimate()

double HMM::BWForwardBackwardCount(const uintVector &O, dVector &Pi_counts,
                                   dMatrix &A_counts, vector<dMatrix> &B_counts,
                                   bool verbose) const
{
   assert(parm_type == regular_probs);
   dMatrix alpha_hat;
   dVector c;
   double logprob = ForwardProcedure(O, alpha_hat, c, true, verbose);

   dMatrix beta_hat;
   BackwardProcedure(O, c, beta_hat, verbose);

   BWCountExpectation(O, alpha_hat, beta_hat,
                      Pi_counts, A_counts, B_counts, verbose);
   
   return logprob;
} // HMM::BWForwardBackwardCount()
