/**
 * @author Samuel Larkin
 * @file exception_dump.h Macros to trap exceptions.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#ifndef __EXCEPTION_DUMP_H__
#define __EXCEPTION_DUMP_H__

#include <stdexcept>
#include "show_mem_usage.h"
#include <new>
#include <typeinfo>
#include <exception>

/// Replaces the normal main and adds a try block.
#define MAIN(argc, argv) main(int argc, const char* const argv[]) try

#ifdef NO_EDUMP
/// Here, we still catch the bad_alloc since the core dump is of no use and
/// this will allow us to have a fixed return code.
/// Returning 42 as exit code will indicate a memory problem and we will be
/// able to retry with more resources on the cluster.
#define END_MAIN  \
   catch(std::bad_alloc& e)        {cerr << e.what() << " Most likely, you ran out of memory" << endl; Portage::showMemoryUsage(); cerr << endl << "The above log should help you troubleshoot the cause of " << e.what() << endl; exit(42);}
#else
/// The actual default catch all exception block.
#define END_MAIN  \
   catch(std::length_error& e)     {cerr << "std::length_error: " << e.what() << endl; exit(24);}\
   catch(std::domain_error& e)     {cerr << "std::domain_error: " << e.what() << endl; exit(24);}\
   catch(std::out_of_range& e)     {cerr << "std::out_of_range: " << e.what() << endl; exit(24);}\
   catch(std::invalid_argument& e) {cerr << "std::invalid_argument: " << e.what() << endl; exit(24);}\
   catch(std::range_error& e)      {cerr << "std::range_error: " << e.what() << endl; exit(24);}\
   catch(std::overflow_error& e)   {cerr << "std::overflow_error: " << e.what() << endl; exit(24);}\
   catch(std::underflow_error& e)  {cerr << "std::underflow_error: " << e.what() << endl; exit(24);}\
   catch(std::bad_alloc& e)        {cerr << e.what() << " Most likely, you ran out of memory" << endl; Portage::showMemoryUsage(); cerr << endl << "The above log should help you troubleshoot the cause of " << e.what() << endl; exit(42);}\
   catch(std::bad_cast& e)         {cerr << "std::bad_cast: " << e.what() << endl; exit(24);}\
   catch(std::bad_typeid& e)       {cerr << "std::bad_typeid: " << e.what() << endl; exit(24);}\
   catch(std::bad_exception& e)    {cerr << "std::bad_exception: " << e.what() << endl; exit(24);}\
   catch(std::ios_base::failure& e){cerr << "std::ios_base::failure: " << e.what() << endl; exit(24);}\
   catch(std::exception& e)        {cerr << "std::exception: " << e.what() <<endl; exit(24);}\
   catch(...)                      {cerr << "Unknown general exception" << endl; exit(24);}
#endif


#endif  // __EXCEPTION_DUMP_H__
