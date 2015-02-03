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
#include "errors.h"
#include <new>
#include <typeinfo>
#include <exception>

/// Replaces the normal main and adds a try block.
#define MAIN(argc, argv) main(int argc, const char* const argv[]) { try

#ifdef NO_EDUMP
/// Here, we still catch the bad_alloc since the core dump is of no use.
/// We now use error(ETFatal) instead of exit() because this way we can trap
/// the signal if the error handler has been overriden.
#define END_MAIN  \
   catch(std::bad_alloc& e)        {cerr << e.what() << " Most likely, you ran out of memory" << endl; Portage::showMemoryUsage(); cerr << endl; error(ETFatal, "std::bad_alloc %s The above log should help you troubleshoot the cause.", e.what());} }
#else
/// The actual default catch all exception block.
#define END_MAIN  \
   catch(std::length_error& e)     {error(ETFatal, "std::length_error: %s", e.what());}\
   catch(std::domain_error& e)     {error(ETFatal, "std::domain_error: %s", e.what());}\
   catch(std::out_of_range& e)     {error(ETFatal, "std::out_of_range: %s", e.what());}\
   catch(std::invalid_argument& e) {error(ETFatal, "std::invalid_argument: %s", e.what());}\
   catch(std::range_error& e)      {error(ETFatal, "std::range_error: %s", e.what());}\
   catch(std::overflow_error& e)   {error(ETFatal, "std::overflow_error: %s", e.what());}\
   catch(std::underflow_error& e)  {error(ETFatal, "std::underflow_error: %s", e.what());}\
   catch(std::bad_alloc& e)        {cerr << e.what() << " Most likely, you ran out of memory" << endl; Portage::showMemoryUsage(); cerr << endl; error(ETFatal, "std::bad_alloc %s The above log should help you troubleshoot the cause.", e.what());}\
   catch(std::bad_cast& e)         {error(ETFatal, "std::bad_cast: %s", e.what());}\
   catch(std::bad_typeid& e)       {error(ETFatal, "std::bad_typeid: %s", e.what());}\
   catch(std::bad_exception& e)    {error(ETFatal, "std::bad_exception: %s", e.what());}\
   catch(std::ios_base::failure& e){error(ETFatal, "std::ios_base::failure: %s", e.what());}\
   catch(std::exception& e)        {error(ETFatal, "std::exception: %s", e.what());}\
   catch(...)                      {error(ETFatal, "Unknown general exception");}\
}
#endif


#endif  // __EXCEPTION_DUMP_H__
