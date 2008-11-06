/**
 * @author Samuel Larkin
 * @file test_feature_function_grammar.cc  Test the grammar for parsing a feature
 * function when reading a rescoring-model
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include <feature_function_grammar.h>
#include <iostream>

using namespace std;
using namespace boost::spirit;

/**
 * Semantic action for the boost::grammar that outputs the object to STDOUT.
 * @param  t a typed data to simply be outputed on STDPUT
 */
template <typename T>
void print(T t) {
   cout << "\t" << t << endl;
}
/**
 * A second semantic action for the boost::grammar that outputs the object to
 * STDOUT.
 * @param  t a typed data to simply be outputed on STDPUT
 */
template <typename T>
void dummy(T t) {
   cout << "\tusing dummy: " << t << endl;
}
/**
 * A third semantic action for the boost::grammar that simply outputs a string
 * to STDOUT.
 * @param  str  pointer to the start of the string
 * @param  end  pointer to the end of the string
 */
void printString(char const* str, char const* end)
{
   string  s(str, end);
   cout << "\t" << s << endl;
}

int main (int argc, char* argv[])
{
   if (argc > 1) {
	   cerr << "Prgram that tests the grammar used for parsing the feature functions." << endl;
		exit(1);
	}

   // Different syntaxes for a feature function in a rescoring-model
   const char* const src [] = {
      "File2FF" ,
      "File2FF -0.32143",
      "File2FF N(6.0,1.0)" ,
      "File2FF U(1.0,11.0)" ,
      "File2FF:somefile.../argument,1",
      "File2FF:somefile.../argument,1 0.343" ,
      "File2FF:somefile.../argument,1 N(6.0,1.0)",
      "File2FF:somefile.../argument,1 U(1.0,11.0)",
      "nbestNgramPost:1#1",
      "nbestNgramPost:1#1 3",
      "nbestNgramPost:1#1 N(-5,0.04)",
      "nbestNgramPost:1#1 U(-5,-3)",
      "FileFF:ff.nbestNgramPost.1#1",
      "FileFF:ff.nbestNgramPost.1#1 3",
      "FileFF:ff.nbestNgramPost.1#1 N(-5,0.04)",
      "FileFF:ff.nbestNgramPost.1#1 U(-5,-3)",
      "FileFF:ff.HMMTgtGivenSrc.hmm.en_given_ch",
      "FileFF:ff.HMMTgtGivenSrc.hmm.en_given_ch -9",
      "FileFF:ff.HMMTgtGivenSrc.hmm.en_given_ch N(1, 0.2)",
      "FileFF:ff.HMMTgtGivenSrc.hmm.en_given_ch U(-4.5, 12.452)"
      };

   for (unsigned int i(0); i<sizeof(src)/sizeof(const char* const); ++i) {
      Portage::feature_function_grammar gram(false);
      parse_info<> pi = parse(src[i], gram, space_p);
      if (pi.full) {
         cout << "\033[1;32m=>Full src parsed successfully\033[0m" << endl;
      }
      else {
         if (pi.hit) 
            cout << "\033[1;33m=>Partially parsed src\033[0m" << endl;
         else 
            cout << "\033[1;31m=>Failed to parse\033[0m" << endl;
      }
      cout << "SRC: " << src[i] << endl;
      cout << "ff: "        << gram.ff << endl;
      cout << "name: "      << gram.name << endl;
      cout << "arg: "       << gram.arg << endl;
      cout << "weight: "    << gram.weight << endl;
      if (gram.rnd.get() != NULL) cout << "random: " << gram.rnd->name << endl;
      cout << "Remaining: " << pi.stop << endl;

      if (gram.rnd.get() != NULL) {
         cout << "Some Random values: " << endl;
         for (unsigned int r(0); r<3; ++r)
            cout << (*gram.rnd)() << endl;
      }

      cout << endl;
   }

   return 0;
}
