/**
 * @author Samuel Larkin
 * @file feature_function_grammar.h  This is the grammar the parse feature
 * function when read a rescoring-model
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, 2006, Her Majesty in Right of Canada
 */

#ifndef __FEATURE_FUNCTION_GRAMMAR_H__
#define __FEATURE_FUNCTION_GRAMMAR_H__

#include "portage_defs.h"
#include "randomDistribution.h"
#include "errors.h"
#include <boost/version.hpp>
#if BOOST_VERSION >= 103800
   #include <boost/spirit/include/classic.hpp>
   #include <boost/spirit/include/classic_assign_actor.hpp>
   using namespace boost::spirit::classic;
#else
   #include <boost/spirit.hpp>
   #include <boost/spirit/actor/assign_actor.hpp>
   using namespace boost::spirit;
#endif
#include <boost/bind.hpp>
#include <string>


namespace Portage {

/// Grammar to parse feature function from a rescoring-model.
class feature_function_grammar : public grammar<feature_function_grammar>
{
protected:
   // Only usefull within the class
   mutable double       first;        ///< tmp placeholder when creating a random distribution
   mutable double       second;       ///< tmp placeholder when creating a random distribution
           bool         training;     ///< when training substitue weight by default distn and warn
public:
   // Must be accessible from outside
   mutable ptr_rnd_gen  rnd;          ///< The parsed random distribution object
   mutable std::string  ff;           ///<
   mutable std::string  name, arg;    ///< name and arguments of a feature function
   mutable double weight;

   /// Default constructor.
   feature_function_grammar(bool training)
   : first(0.0f)
   , second(0.0f)
   , training(training)
   , weight(0.0f)
   {}

   /// Resets the placeholders in the grammar in multiple usage scenarios.
   void clear() {
      debug("CLEARING");
      rnd.reset();
      ff.clear();
      name.clear();
      arg.clear();
      weight = 0.0f;
      first = second = 0.0f;
   }

   /// Creates a uniform random distribution.
   /// Grammar's semantic action.
   void checkDistn(char const*, char const*) const{
      debug("checking distribution");
      if (rnd.get() == NULL) {
         std::cerr << "Invalid distribution" << std::endl;
         assert(false);
      }
   }

   /// Creates a uniform random distribution.
   /// Grammar's semantic action.
   void makeUniform(char const*, char const*) const{
      debug("Creating uniform");
      rnd = ptr_rnd_gen(new uniform(first, second));
   }

   /// Creates a normal random distribution.
   /// Grammar's semantic action.
   void makeNormal(char const*, char const*) const{
      debug("Creating normal");
      rnd = ptr_rnd_gen(new normal(first, second));
   }

   /// Creates the weight distribution.
   /// Grammar's semantic action.
   void makeWeight(double weight) const{
      debug("Creating weight");
      rnd = ptr_rnd_gen(new weight_distribution(weight));
   }

   /// Creates the default random distribution which is a N(0.0,1.0).
   /// Grammar's semantic action.
   void makeDefault(char const*, char const*) const{
      debug("Creating default");
      rnd = ptr_rnd_gen(new normal(0.0f, 1.0f));
   }

   /// Creates the default random distribution which is a N(0.0,1.0).
   /// Grammar's semantic action.
   void makeDefault(double) const{
      makeDefault(NULL, NULL);
   }

   /// In training mode, when a weight is detect, warn the user.
   /// Grammar's semantic action.
   void warnDefault(double) const {
      if (training) {
         error(ETWarn, "Dropping weight and using default distribution for training");
         makeDefault(NULL, NULL);
      }
   }

   /// Signals an error on missing weight in translation mode.
   /// In translating mode, the model MUST contain a weight for each feature function.
   void errorNoWeight(char const*, char const*) const {
      debug("Error, when translating you need weights");
      error(ETFatal, "Translating requires feature function weights, check your rescoring model");
   }

   /// Actual grammar's definiton
   template <typename ScannerT>
   struct definition
   {    
      /// Default construtor.
      /// @param  self the grammar object itself
      definition(feature_function_grammar const& self)  
      {
         // Note: square braces are used to attach semantic action to a parsed
         // sequence of tokens.
         // Note: lexeme_d is to turn off white space skipping.

         // The grammar differs depending on training vs translating
         if (self.training) {
            // A weight can either be a normal distn or a uniform distn or a fix distn or be missing.
            weight = normal | uniform | fix | none;
         }
         else {
            // When translating, only a fix weight is allowed
            weight = fix | eps_p[bind(&feature_function_grammar::errorNoWeight, &self, _1, _2)];
         }

         // none means no weight or distn and we will instanciate the default distn with a semantic action.
         none = eps_p
                  [bind(&feature_function_grammar::makeDefault, &self, _1, _2)];

         // A fix weight is represented by a real number.
         // We then want to assign the parsed value to the variable weight in self with a semantic action.
         // We then want to create the default distn with a semantic action.
         // We then want to warn the user that we've created a default distn if he's training with rescore_train.
         fix = real_p
                  [assign_a(self.weight)]
                  [bind(&feature_function_grammar::makeDefault, &self, _1)]
                  [bind(&feature_function_grammar::warnDefault, &self, _1)];

         // For a normal the user writes "N(mean, sigma)".
         // We then want to instanciate a normal distn with the provided mean and sigma with a semantic action.
         normal = ('N' >> values)
                     [bind(&feature_function_grammar::makeNormal, &self, _1, _2)];

         // For a uniform the user writes "U(min, max)".
         // We then want to instanciate a unifrom distn with the parsed min an max with a semantic action.
         uniform = ('U' >> values)
                     [bind(&feature_function_grammar::makeUniform, &self, _1, _2)];

         // value is the common part between the uniform and normal syntax which is "(real, real)".
         // We then want to assign the real values parsed to some temp variables with a semantic action.
         values = '(' >> real_p[assign_a(self.first)] >> ',' >> real_p[assign_a(self.second)] >> ')';

         // The name of the feature function is composed of at least one alpha numeric caracter.
         // We then assign the feature's name to a variable called name with a semantic action.
         // lexeme_d means DO NOT skip white spaces for the rule inclosed in square braces
         // because this grammar expects the parser to eat the spaces for the rest.
         name = lexeme_d[(+alnum_p)][assign_a(self.name)];

         // A feature may have some argument or not.
         // If it does, the argument starts with ':' and can contain any thing except spaces.
         // If an argument is present, we then want to assign it to a variable arg with a semantic action.
         // lexeme_d means DO NOT skip white spaces for the rule inclosed in square braces
         // because this grammar expects the parser to eat the spaces for the rest.
         arg = ':' >> lexeme_d[(+(~ch_p(' ')))][assign_a(self.arg)] | eps_p;
         
         // The overall rule for a feature function declaration in rescoring-model is:
         // a name, an argument that we assign to a variable ff with a semantic action
         // followed by a weight and for robustness, we attach a semantic action
         // to the weight which will make sure there is valid distn
         // instanciated.
         r = (name >> arg)[assign_a(self.ff)] >> weight[bind(&feature_function_grammar::checkDistn, &self, _1, _2)];
      }
      /// Some rules placeholders
      rule<ScannerT>  r, weight, normal, uniform, values, none, fix, name, arg;
      /// Default starting point.
      rule<ScannerT> const& start() const { return r; };
   };

   void debug(const char* const msg) const {
#ifdef FF_GRAMMAR_DEBUG
      using namespace std;
      cerr << "  FFG: " << msg << endl;
#endif
   }
};  // ends namespace feature_function_grammar
};  // ends namespace Portage

#endif  // __FEATURE_FUNCTION_GRAMMAR_H__
