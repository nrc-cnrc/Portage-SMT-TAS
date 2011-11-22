#ifndef __ug_get_options_h
#define __ug_get_options_h

#include <boost/program_options.hpp>

namespace ugdiss 
{
  namespace po=boost::program_options;
  typedef po::options_description            progopts;
  typedef po::positional_options_description  posopts;
  typedef po::variables_map                   optsmap;

  // some boilerplate code to declutter my usual interpret_args() routine
  void 
  get_options(int ac, char* av[], progopts& o, posopts& a, optsmap& vm, 
              char const* cfgFileParam=NULL);

}
#endif
