/**
 * @author Eric Joanis
 * @file test_hmm.cc  Test harness for hmm.h
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "hmm.h"
#include "file_utils.h"
#include "str_utils.h"
#include "arg_reader.h"
#include "errors.h"
#include <stdexcept>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
test_hmm [-v]\n\
\n\
   Run HMM test suite\n\
   For regression testing, do: test_hmm 2>&1 | diff test_hmm.out -\n\
\n\
";

static bool verbose = false;
void getArgs(int argc, const char* const argv[]);
void test_matrix_iterators();

int main(int argc, char** argv)
{
   getArgs(argc, argv);

   // Crazy soft drink machine HMM from Manning & Schuetze, page 321.
   enum CrazyStates { CP, IP };
   enum CrazyAlphabet { cola, ice_t, lem };
   Uint N = 2;
   Uint M = 3;
   HMM crazy(N, M, HMM::state_exit_emit, HMM::regular_probs);
   crazy.A(CP,CP) = .7;
   crazy.A(CP,IP) = .3;
   crazy.A(IP,CP) = .5;
   crazy.A(IP,IP) = .5;
   crazy.checkTransitionDistributions();

   crazy.B(CP,cola) = .6;
   crazy.B(CP,ice_t) = .1;
   crazy.B(CP,lem) = .3;

   crazy.B(IP,cola) = .1;
   crazy.B(IP,ice_t) = .7;
   crazy.B(IP,lem) = .2;
   crazy.checkEmissionDistributions(true);

   crazy.write(cerr);
   cerr << endl;

   Uint T = 3;
   uintVector O(T);
   O[0] = lem;
   O[1] = ice_t;
   O[2] = cola;

   uintVector X_hat;
   double logprob = crazy.Viterbi(O, X_hat, true);
   cerr << "probViterbi logprob " << logprob << " prob " << exp(logprob)
        << " sequence: " << joini (X_hat.begin(), X_hat.end()) << endl;
   cerr << endl;

   HMM crazy_copy(crazy);
   crazy_copy.convertToLogModel();
   cerr << "Converting copy to log model" << endl;
   crazy_copy.write(cerr);
   double logprob2 = crazy_copy.Viterbi(O, X_hat, true);
   cerr << "logViterbi logprob " << logprob2 << " prob " << exp(logprob2)
        << " sequence: " << joini (X_hat.begin(), X_hat.end()) << endl;
   cerr << endl;

   dMatrix alpha_hat;
   dVector c;
   crazy.ForwardProcedure(O, alpha_hat, c, true, true);
   cerr << endl;

   dMatrix beta_hat;
   crazy.BackwardProcedure(O, c, beta_hat, true);
   cerr << endl;

   dVector Pi_counts;
   dMatrix A_counts;
   vector<dMatrix> B_counts;
   crazy.BWCountExpectation(O, alpha_hat, beta_hat,
                            Pi_counts, A_counts, B_counts, true);
   cerr << endl;

   dMatrix gamma;
   crazy.statePosteriors(O, gamma, true);

   crazy.BWReestimate(Pi_counts, A_counts, B_counts, true);
   cerr << "Reestimated model:" << endl;
   crazy.write(cerr);

   //test_matrix_iterators();
}

// arg processing

void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {"v"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 0, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
}


// test function to understand how ublas::matrix iterators work.
void test_matrix_iterators() {
   cerr << endl << "Matrix Iterator Tests" << endl;

   Uint I(5), J(7);
   uintMatrix M(I,J);
   for ( Uint i = 0; i < I; ++i )
      for ( Uint j = 0; j < J; ++j )
         M(i,j) = i * 100 + j; // mnemonic value - easy to read later

   cerr << "M.begin1() to end1():";
   for ( uintMatrix::iterator1 it1 = M.begin1(); it1 != M.end1(); ++it1 )
      cerr << " " << *it1;
   cerr << endl;

   cerr << "M.begin2() to end2():";
   for ( uintMatrix::iterator2 it2 = M.begin2(); it2 != M.end2(); ++it2 )
      cerr << " " << *it2;
   cerr << endl;

   cerr << "it1 = M.begin1() to end1():" << endl;
   for ( uintMatrix::iterator1 it1 = M.begin1(); it1 != M.end1(); ++it1 ) {
      cerr << "  it2 = it1.begin() to end():";
      for ( uintMatrix::iterator2 it2 = it1.begin(); it2 != it1.end(); ++it2 )
         cerr << " " << *it2;
      cerr << endl;
   }

   cerr << "it2 = M.begin2() to end2():" << endl;
   for ( uintMatrix::iterator2 it2 = M.begin2(); it2 != M.end2(); ++it2 ) {
      cerr << "  it1 = it2.begin() to end():";
      for ( uintMatrix::iterator1 it1 = it2.begin(); it1 != it2.end(); ++it1 )
         cerr << " " << *it1;
      cerr << endl;
   }

   //cerr << "M.begin1()[3].begin()[4]: ";
   //cerr << M.begin1()[3].begin()[4] << endl;

   {
      uintMatrix::iterator1 it1 = M.begin1() + 3;
      //it1 += 3;
      cerr << "it1 = M.begin1() + 3: " << *it1 << endl;
      uintMatrix::iterator2 it2 = it1.begin() + 4;
      //it2 += 4;
      cerr << "  it2 = it1.begin() + 4: " << *it2 << endl;
      it1 = it2.begin() + 1;
      cerr << "  it1 = it2.begin() + 1: " << *it1 << endl;
      it2 = it1.begin() + 2;
      cerr << "  it2 = it1.begin() + 2: " << *it2 << endl;
   }

   cerr << "*((M.begin1() + 3).begin() + 4): "
        << *((M.begin1() + 3).begin() + 4) << endl;
}
