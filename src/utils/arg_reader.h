/**
 * @author George Foster
 * @file arg_reader.h Process string arguments.
 * 
 * 
 * COMMENTS: 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef ARG_READER_H
#define ARG_READER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include "str_utils.h"

// Forward declaration instead of include speeds up compilation of files that
// don't use optional.
// Use #include <boost/optional/optional.hpp> in your .cc file if you need it.
namespace boost {
   template <class T> class optional;
}

namespace Portage {
   class iMagicStream;
   class oMagicStream;

/**
 * A command line reader.
 * Argument processing for programs. You specify what switches and args are
 * legal for your program, then this class takes care of processing the argv
 * list and converting strings to values, or dying in the attemp. There are a
 * few fancy things:
 * - Switches can be longer than one char: -zap will be interpreted as a
 *   switch named zap if one exists, otherwise as -z -a -p.
 * - Use -- to end switch processing: "-v -- -filename" will interpret
 *   -filename as a variable named filename, not a switch.
 * - You can ask for an istream/ostream pointer to be associated with either a
 *   filename if one is supplied, or cin/cout if "-" is supplied.
 * See prog.cc for example.
 */
class ArgReader {

   const static char switch_prefix;                        ///< switch prefix '-'
   const static char suffix_for_switches_that_take_args;   ///< suffix for switches that take args ':'
   const static string switchlist_terminator;              ///< switch list terminator "--"

   set<string> switches_noargs;	///< switches that don't take args
   set<string> switches_args;	///< switches that do take args
   int min_vars;                ///< minimum of required args
   int max_vars;                ///< maximum allowed args
   string help_message;         ///< help message
   string help_switch;          ///< switch to display help
   bool print_help_on_error;    ///< should we display help on error
   string alt_help_message;     ///< alternative help message
   string alt_help_switch;      ///< switch to display alternative message

   typedef map<string, vector<string> >  ARG_MAP;   ///< pair of switch and its values
   typedef ARG_MAP::const_iterator SwitchIter;      ///< an iterator of switch and its values
   ARG_MAP        switch_values;                    ///< map of switches and their values
   vector<string> vars;                             ///< list of floating args (args not belonging to a switch)
   string message_to_print_after_error;             ///< should we display help on error

   /// Clears the all args from ArgReader.
   void clear() {switch_values.clear(); vars.clear();}
   
public:

   /**
    * Constructor.
    * @param num_switches number of switches in switches
    * @param switches Leading switch_prefix char is optional; use trailing
    * suffix_for_switches_that_take_args char to indicate that switch requires
    * an argument.
    * @param min_vars minimum of floating args.
    * @param max_vars maximum of floating args.  Use -1 for no limit.
    * @param help_msg help message to be displayed
    * @param help_switch switch to display help message
    * @param print_help_on_error If false, just print a message saying you can
    * use -h for help.
    * @param alt_help_msg alternative help message
    * @param alt_help_switch switch to display alternative help message
    */
   ArgReader(Uint num_switches, const char* const switches[], 
	     int min_vars=0, int max_vars=0,
	     const char* help_msg="", const char* help_switch = "-h",
	     bool print_help_on_error=false,
	     const char* alt_help_msg="", const char* alt_help_switch = "");

   /// Destructor.
   ~ArgReader() {}

   /**
    * Reads a list of switches and arguments.
    * @param argc number of args in argv 
    * @param argv vector of args that should not contains the application name at argv[0]
    */
   void read(Uint argc, const char* const argv[]);

   /**
    * Dumps switch and var values to os.
    * @param os the ostream where to dump argReader
    */
   void dump(ostream& os);

   /**
    * Looks for the first value of a switch (from the list of switches with values).
    * @param sw switch that we want its first value
    * @param val will contains the first value of a switch sw if it exists 
    * @return true if sw is found or false otherwise
    */
   bool getSwitch(const char* sw, string* val = NULL) const;

   /**
    * Gets the number of floating args.
    * @return the number of floating args
    */
   Uint numVars() const { return vars.size(); }

   /**
    * Gets the ith floating arg (zero-based index).
    * @param i index of the required floating arg
    * @return the floating arg string value
    */
   const string& getVar(Uint i) const { return vars[i]; }

   /**
    * Gets all floating arg from start_index.
    * @param start_index  starting index
    * @param thosevars    returned vector of args
    */
   void getVars(Uint start_index, vector<string>& thosevars) const
   {
      for (Uint i = start_index; i < vars.size(); ++i)
        thosevars.push_back(vars[i]);
   }
   
   /** 
    * Gets value for switch sw.
    * Test whether a given switch was set and convert its value to a given
    * type if so. Abort with error if conversion fails.  
    * @param sw switch to test 
    * @param val place to store resulting value (not touched if switch isn't
    * set). T may be one of: bool, char, double, float, int, Uint, string,
    * ofstream, ifstream.
    */
   template<class T>
   void testAndSet(const char* sw, T& val)
   {
      string str;
      if (getSwitch(sw, &str) && !conv(str, val))
      {
         error(ETFatal, "can't convert -%s value (%s) to %s", sw, str.c_str(), typeName<T>().c_str());
      }
   }

   /**
    * Get value for switch sw.
    * Use this version when you don't want a default value, but rather you want
    * to be able to tell that it wasn't specified at all on the command line.
    * To use this method, you must \#include <boost/optional/optional.hpp>.
    * Note that this method should not be used with T=bool - use
    * testAndSetOrReset() with optional\<bool\> instead.
    * @param sw switch to test
    * @param val place to store resulting value - not touched if sw isn't set,
    *        which means if val was uninitialied, it will remain uninitialized
    */
   template<class T>
   void testAndSet(const char* sw, boost::optional<T>& val)
   {
      string str;
      if (getSwitch(sw, &str)) {
         T tmp_val;
         if ( conv(str, tmp_val) )
            val = tmp_val;
         else
            error(ETFatal, "can't convert -%s value (%s) to %s", sw, str.c_str(), typeName<T>().c_str());
      }
   }

   /**
    * Checks if sw has a value.
    * @param sw  switch to get its values
    * @param val return value
    */
   void testAndSet(const char* sw, bool& val);

   /**
    * Test for a dual -x/-nox switch.
    * Use with BOOL_TYPE=bool if a default is desired, or
    * BOOL_TYPE=optional\<bool\> if you want to be able to know when neither
    * switch was found on the command line.
    * @param set_sw   on switch
    * @param reset_sw off switch
    * @param val      will be set to true if set_sw is found, false if reset_sw
    *                 is found, and left untouched if neither is found.
    */
   template <class BOOL_TYPE>
   void testAndSetOrReset(const char* set_sw, const char* reset_sw,
                          BOOL_TYPE& val)
   {
      if ( getSwitch(set_sw) ) {
         val = true;
         if ( getSwitch(reset_sw) )
            error(ETWarn, "contradictory switches -%s and -%s both specified; ignoring -%s",
                  set_sw, reset_sw, reset_sw);
      } else if ( getSwitch(reset_sw) ) {
         val = false;
      }
   }

   /**
    * Returns all values for switch sw.
    * @param sw        switch to get its values
    * @param arg_list  returned values
    * @see void testAndSet(const char* sw, T& val)
    * @see bool getSwitch(const char* sw, string* val = NULL)
    */
   void testAndSet(const char* sw, vector<string>& arg_list);
   
   //@{
   /**
    * Opens a stream according to the value of sw.
    * @param sw  stream's name to open
    * @param ifs returned stream
    */
   void testAndSet(const char* sw, ifstream& ifs);
   void testAndSet(const char* sw, iMagicStream& ifs);
   //@}

   //@{
   /**
    * Opens a stream according to the value of sw.
    * @param sw  stream's name to open
    * @param ofs returned stream
    */
   void testAndSet(const char* sw, ofstream& ofs);
   void testAndSet(const char* sw, oMagicStream& ofs);
   //@}


   /**
    * Gets value of the floating arg at index var_index.
    * Test whether a given variable is present and convert its value to a
    * given type if so. Abort with error if conversion fails.
    * @param var_index index of variable to test
    * @param var_name name to use in error message
    * @param val place to store resulting value (not touched if var isn't
    * set). T may be one of: bool, char, double, float, int, Uint, string,
    * ofstream, ifstream.
    */
    template<class T>
    void testAndSet(Uint var_index, const char* var_name, T& val)
    {
        if (var_index < vars.size() && !conv(vars[var_index], val))
        {
            error(ETFatal, "can't convert %s value (%s) to %s", var_name, vars[var_index].c_str(), typeName<T>().c_str());
        }
    }

    //@{
    /**
     * Opens an input stream from the value of the floating arg at index var_index.
     * @param var_index  index of the floating arg
     * @param var_name   name of the floating arg when an error occurs
     * @param ifs        returned opened input stream
     */
    void testAndSet(Uint var_index, const char* var_name, ifstream& ifs);
    void testAndSet(Uint var_index, const char* var_name, iMagicStream& ifs);
    //@}

    //@{
    /**
     * Opens an output stream from the value of the floating arg at index var_index.
     * @param var_index  index of the floating arg
     * @param var_name   name of the floating arg when an error occurs
     * @param ofs        returned opened output stream
     */
    void testAndSet(Uint var_index, const char* var_name, ofstream& ofs);
    void testAndSet(Uint var_index, const char* var_name, oMagicStream& ofs);
    //@}

   //@{
   /**
    * Opens an input stream from the value of the floating arg at index var_index.
    * Test whether given var is present, and open an istream if so. Abort
    * with error if this can't be done.
    * @param var_index index of variable to test
    * @param var_name name to use in error message
    * @param is place to put value (not touched if var isn't set); if var is
    * "-", then *is is set to cin, else to ifs (opened with filename var)
    * @param ifs input file stream possibly associated with is
    */
   void testAndSet(Uint var_index, const char* var_name, std::istream** is, std::ifstream& ifs);
   void testAndSet(Uint var_index, const char* var_name, std::istream** is, iMagicStream& ifs);
   //@}


   //@{
   /**
    * Opens an output stream from the value of the floating arg at index var_index.
    * Test whether given var is present, and open an ostream if so. Abort
    * with error if this can't be done.
    * @param var_index index of variable to test
    * @param var_name name to use in error message
    * @param os place to put value (not touched if var isn't set); if var is
    * "-", then *is is set to cout, else to ofs (opened with filename var)
    * @param ofs output file stream possibly associated with os
    */
   void testAndSet(Uint var_index, const char* var_name, std::ostream** os, std::ofstream& ofs);
   void testAndSet(Uint var_index, const char* var_name, std::ostream** os, oMagicStream& ofs);
   //@}

};

} // ends Portage namespace
#endif
