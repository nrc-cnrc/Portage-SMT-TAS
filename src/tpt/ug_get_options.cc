#include "ug_get_options.h"
#include "tpt_error.h"
#include <fstream>

#include <string>
#include <iostream>

namespace ugdiss
{
  using namespace std;

  void 
  get_options(int ac, char* av[], progopts& o, posopts& a, optsmap& vm,
              char const* cfgFileParam)
  {
    try {
      // only get named parameters from command line
      po::store(po::command_line_parser(ac,av).options(o).run(),vm);

      if (cfgFileParam && vm.count(cfgFileParam))
        {
          string cfgFile = vm[cfgFileParam].as<string>();
          if (!cfgFile.empty())
            {
              if (!access(cfgFile.c_str(),F_OK))
                {
                  ifstream cfg(cfgFile.c_str());
                  po::store(po::parse_config_file(cfg,o),vm);
                }
              else
                {
                  cerr << "Error: File '" << cfgFile
                       << "' cannot be found!" << endl << exit_1;
                }
            }
        }
      
      // now get positional args too (if any), ignoring those set in the config
      // file
      if (a.max_total_count())
        po::store(po::command_line_parser(ac,av)
                  .options(o).positional(a).run(),vm);  
      po::notify(vm); // IMPORTANT
    } catch (po::error& e) {
      cerr << efatal << e.what() << endl << exit_1;
    }
  }
}
