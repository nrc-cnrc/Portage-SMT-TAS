/**
 * @author Samuel Larkin
 * @file randomDistribution.h  Contains three random distributions: normal, uniform and fix
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __RANDOM_DISTRIBUTION_H__
#define __RANDOM_DISTRIBUTION_H__

#include <boost/random.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/shared_ptr.hpp>

namespace Portage {

/// Random distribution base class to make it possible to use boost::random in
/// a polymorphic situation.
class rnd_distribution
{
   protected:
      typedef boost::taus88 generator;
   public:
      const char * const  name;  ///< A string representation of the distribution type

      /// Defaults constructor.
      /// @param name  distribution's name
      rnd_distribution(const char * const name)
      : name(name)
      {}
      /// Seed the random number generator.
      /// @param s  seed
      void seed(unsigned int s) {
         if (getGen() != NULL) {
            // IMPORTANT the random number generator requires a seed >= 16
            s += 16;
            assert(s >= 16);
            assert(11*s >= 16);
            const int int_s = s;
            const int v[] = { 7*int_s, int_s, 11*int_s, 3*int_s };
            const int* p = &v[0];
            getGen()->seed(p, v+sizeof(v)/sizeof(int));
            //cerr << "  Seeded: " << (*getGen()) << endl;  // Usefull for debugging
         }
      }
      /// Makes the object a callable entity that will return a random number.
      /// @return  Returns a random number according to the underlying distribution.
      double operator()() { return get(); };

      /// Get the underlying number generator.
      /// @return Returns the number generator.
      virtual boost::taus88* getGen() = 0;
      /// Return a random number.
      /// @return  Returns a random number according to the underlying distribution.
      virtual double get() = 0;
      /// Set a fix value for the distribution. Only valid for weight_distribution.
      /// @param v  new fix value
      virtual void set(double v) {}
};

/// A normal distribution.
/// Wraps a boost::normal_distribution object
class normal : public rnd_distribution
{
   protected:
      /// Definition of the distribution type
      typedef boost::normal_distribution<double> distribution;
      /// Definition of the random number generator
      typedef boost::variate_generator<generator, distribution>  random_gen;
      random_gen rnd;   ///< random number generator
   public:
      /**
       * Default constructor.
       * @param mean  mean value for the normal distribution
       * @param sigma sigma value for the normal distribution
       */
      normal(double mean, double sigma)
      : rnd_distribution("normal")
      , rnd(generator(), distribution(mean, sigma))
      {}
      virtual boost::taus88* getGen() { return &rnd.engine(); };
      virtual double get() { return rnd(); }
};

/// A uniform distribution.
/// Wraps a boost::uniform_real object
class uniform : public rnd_distribution
{
   protected:
      /// Definition of the distribution type
      typedef boost::uniform_real<double> distribution;
      /// Definition of the random number generator
      typedef boost::variate_generator<generator, distribution>  random_gen;
      random_gen    rnd;     ///< random number generator
   public:
      /**
       * Default constructor.
       * @param  min  minimum value for the uniform distribution
       * @param  max  maximum value for the uniform distribution
       */
      uniform(double min, double max)
      : rnd_distribution("uniform")
      , rnd(generator(), distribution(min, max))
      {}
      virtual boost::taus88* getGen() { return &rnd.engine(); };
      virtual double get() { return rnd(); }
};

/// A constant distribution.
/// Always returns the same value
class constant_distribution : public rnd_distribution
{
   protected:
      const double  value;   ///<  constant value
   public:
      /// Default constructor.
      /// @param value  constant value for this distribution
      constant_distribution(double value)
      : rnd_distribution("constant")
      , value(value)
      {}
      virtual boost::taus88* getGen() { return NULL; };
      virtual double get() { return value; }
};

/// A weight distribution.
/// Always returns the same value
class weight_distribution : public rnd_distribution
{
   protected:
      double  value;   ///<  weight value
   public:
      /// Default constructor.
      /// @param value  constant value for this distribution
      weight_distribution(double value)
      : rnd_distribution("weight")
      , value(value)
      {}
      virtual boost::taus88* getGen() { return NULL; };
      virtual double get() { return value; }
      virtual void set(double v) { value = v; }
};

/// Definition for a shared_pointed random number generator.
typedef boost::shared_ptr<rnd_distribution> ptr_rnd_gen;

};  // ends namespace Portage

#endif  // __RANDOM_DISTRIBUTION_H__
