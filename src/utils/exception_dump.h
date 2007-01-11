/**
 * @author Samuel Larkin
 * @file exception_dump.h Macros to trap exceptions.
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */
#ifndef __EXCEPTION_DUMP_H__
#define __EXCEPTION_DUMP_H__

#include <stdexcept>

/// Replaces the normal main and adds a try block.
#define MAIN(argc, argv) main(int argc, const char* const argv[]) try

/// The actual default catch all exception block.
#define END_MAIN  \
   catch(std::length_error& e)     {cerr << "std::length_error: " << e.what() << endl;}\
   catch(std::domain_error& e)     {cerr << "std::domain_error: " << e.what() << endl;}\
   catch(std::out_of_range& e)     {cerr << "std::out_of_range: " << e.what() << endl;}\
   catch(std::invalid_argument& e) {cerr << "std::invalid_argument: " << e.what() << endl;}\
   catch(std::range_error& e)      {cerr << "std::range_error: " << e.what() << endl;}\
   catch(std::overflow_error& e)   {cerr << "std::overflow_error: " << e.what() << endl;}\
   catch(std::underflow_error& e)  {cerr << "std::underflow_error: " << e.what() << endl;}\
   catch(std::bad_alloc& e)        {cerr << "std::bad_alloc: " << e.what() << endl;}\
   catch(std::bad_cast& e)         {cerr << "std::bad_cast: " << e.what() << endl;}\
   catch(std::bad_typeid& e)       {cerr << "std::bad_typeid: " << e.what() << endl;}\
   catch(std::bad_exception& e)    {cerr << "std::bad_exception: " << e.what() << endl;}\
   catch(std::ios_base::failure& e){cerr << "std::ios_base::failure: " << e.what() << endl;}\
   catch(std::exception& e)        {cerr << "std::exception: " << e.what() <<endl;}\
   catch(...)                      {cerr << "Unknown general exception" << endl;}

#endif  // __EXCEPTION_DUMP_H__
