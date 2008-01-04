/**
 * @author George Foster
 * @file arg_reader.cc  Process string arguments.
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
#include "arg_reader.h"

using namespace Portage;

const char ArgReader::switch_prefix('-');
const char ArgReader::suffix_for_switches_that_take_args(':');
const string ArgReader::switchlist_terminator("--");

ArgReader::ArgReader(Uint num_switches, const char* const switches[], 
		     int min_vars, int max_vars,
		     const char* help_msg, const char* help_sw,
		     bool print_help_on_error,
		     const char* alt_help_msg, const char* alt_help_sw) :
   min_vars(min_vars), max_vars(max_vars),
   help_message(help_msg ? help_msg : ""),
   help_switch(help_sw ? help_sw : ""),
   print_help_on_error(print_help_on_error),
   alt_help_message(alt_help_msg ? alt_help_msg : ""),
   alt_help_switch(alt_help_sw ? alt_help_sw : "")
{
   set<string> all_switches;

   if (help_switch.size() > 1 && help_switch[0] == switch_prefix)
     help_switch = help_switch.substr(1);
   all_switches.insert(help_switch);

   if (alt_help_switch.size() > 1 && alt_help_switch[0] == switch_prefix)
     alt_help_switch = alt_help_switch.substr(1);
   all_switches.insert(alt_help_switch);

   for (Uint i = 0; i < num_switches; ++i)
   {
      string sw = switches[i];
      if (sw.size() > 1 && sw[0] == switch_prefix)
        sw = sw.substr(1);
      
      if (last(sw) == suffix_for_switches_that_take_args)
        switches_args.insert(chop(sw));
      else
        switches_noargs.insert(sw);

      if (!all_switches.insert(sw).second)
        error(ETFatal, "duplicate switch in ArgReader constructor: " + sw);
   }

   if (print_help_on_error)
      message_to_print_after_error = help_message;
   else if (this->help_switch != string(""))
      message_to_print_after_error = "use -" + help_switch + " for help";
   else
      message_to_print_after_error = "";
}

void ArgReader::read(Uint argc, const char* const argv[])
{
   clear();
   Uint i;
   
   for (i = 0; i < argc; ++i)
   {
      string sw = argv[i];
      if (sw == switchlist_terminator)
      {
         ++i;
         break;
      }
      else if (sw.size() > 1 && sw[0] == switch_prefix)
      {
         sw = sw.substr(1);
         if (sw == help_switch)
            error(ETHelp, help_message);
	 else if (sw == alt_help_switch)
	    error(ETHelp, alt_help_message);
         
         if (switches_args.find(sw) != switches_args.end())
         {
            // Is there at least one more argument
            if (i+1 < argc)
            {
               // Look-up next argument
               switch_values[sw].push_back(argv[++i]);
            }
            else
            {
               error(ETFatal, "missing argument for -%s switch\n%s", sw.c_str(), message_to_print_after_error.c_str());
            }
         }
         else if (switches_noargs.find(sw) != switches_noargs.end())
         {
           switch_values[sw].push_back("");
         }
         else
         {        // switch not found: check for switch glob
            string s1 = sw.substr(0, 1);
            if (switches_noargs.find(s1) != switches_noargs.end())
            {
               switch_values[s1].push_back("");
               for (Uint i = 1; i < sw.size(); ++i)
               {
                  s1 = sw.substr(i,1);
                  if (switches_noargs.find(s1) != switches_noargs.end())
                     switch_values[s1].push_back("");
                  else
                     error(ETFatal, "unknown switch: %s\n%s", sw.c_str(), message_to_print_after_error.c_str());
               } 
            }
            else if (switches_args.find(s1) != switches_args.end()) // eg: -x25
               switch_values[s1].push_back(sw.substr(1));
            else if (s1 == help_switch)
               error(ETHelp, help_message);
            else if (s1 == alt_help_switch)
               error(ETHelp, alt_help_message);
            else
               error(ETFatal, "unknown switch: %s\n%s", sw.c_str(), message_to_print_after_error.c_str());
         }
      }
      else
         break;
   }

   for (; i < argc; ++i)
      vars.push_back(argv[i]);

   if (max_vars >= 0 && vars.size() > (Uint)max_vars)
      error(ETFatal, "too many arguments: %s...\n%s", vars[max_vars].c_str(), message_to_print_after_error.c_str());
   else if (vars.size() < (Uint)min_vars)
      error(ETFatal, "too few arguments\n%s", message_to_print_after_error.c_str());
}


void ArgReader::dump(ostream& os)
{
   for (SwitchIter p = switch_values.begin(); p != switch_values.end(); ++p)
      os << p->first << ' ' << p->second.front() << endl;
   os << switchlist_terminator << endl;
   for (Uint i = 0; i < vars.size(); ++i)
      os << vars[i] << endl;
}

bool ArgReader::getSwitch(const char* s, string* val) const
{
   if (!s) return false;
   string sw = s;
   if (sw.size() && sw[0] == switch_prefix)
      sw = sw.substr(1);
   if (sw.size() && last(sw) == suffix_for_switches_that_take_args)
      chop(sw);
   
   SwitchIter res = switch_values.find(sw);
   if (res == switch_values.end()) return false;
   if (val)
      *val = res->second.front();
      
   return true;
}

void ArgReader::testAndSet(const char* sw, bool& val)
{
   string str;
   val = getSwitch(sw, &str);
}

void ArgReader::testAndSet(const char* sw, ifstream& ifs)
{
   string str;
   if (getSwitch(sw, &str))
   {
      ifs.open(str.c_str());
      if (!ifs)
         error(ETFatal, "can't read from %s", str.c_str());
   }
}

void ArgReader::testAndSet(const char* sw, ofstream& ofs)
{
   string str;
   if (getSwitch(sw, &str))
   {
      ofs.open(str.c_str());
      if (!ofs)
         error(ETFatal, "can't write to %s", str.c_str());
   }
}

void ArgReader::testAndSet(const char* sw, iMagicStream& ifs)
{
   string str;
   if (getSwitch(sw, &str))
   {
      ifs.open(str.c_str());
      if (!ifs)
         error(ETFatal, "can't read from %s", str.c_str());
   }
}

void ArgReader::testAndSet(const char* sw, oMagicStream& ofs)
{
   string str;
   if (getSwitch(sw, &str))
   {
      ofs.open(str.c_str());
      if (!ofs)
         error(ETFatal, "can't write to %s", str.c_str());
   }
}

void ArgReader::testAndSet(const char* sw, vector<string>& arg_list)
{
   if (!sw)
   {
      error(ETFatal, "You must provide a switch name, null not accepted");
   }
   
   string sArg = sw;
   if (sArg.size() && sArg[0] == switch_prefix)
      sArg = sArg.substr(1);
   if (sArg.size() && last(sArg) == suffix_for_switches_that_take_args)
      chop(sArg);
   
   SwitchIter res = switch_values.find(sArg);
   if (res != switch_values.end())
   {
      arg_list = res->second;
   }
}

void ArgReader::testAndSet(Uint var_index, const char* var_name, ifstream& ifs)
{
   if (var_index < vars.size())
   {
      ifs.open(vars[var_index].c_str());
      if (!ifs)
         error(ETFatal, "can't read from %s", vars[var_index].c_str());
   }
}

void ArgReader::testAndSet(Uint var_index, const char* var_name, ofstream& ofs)
{
   if (var_index < vars.size())
   {
      ofs.open(vars[var_index].c_str());
      if (!ofs)
         error(ETFatal, "can't write to %s", vars[var_index].c_str());
   }
}

void ArgReader::testAndSet(Uint var_index, const char* var_name, iMagicStream& ifs)
{
   if (var_index < vars.size())
   {
      ifs.open(vars[var_index].c_str());
      if (!ifs)
         error(ETFatal, "can't read from %s", vars[var_index].c_str());
   }
}

void ArgReader::testAndSet(Uint var_index, const char* var_name, oMagicStream& ofs)
{
   if (var_index < vars.size())
   {
      ofs.open(vars[var_index].c_str());
      if (!ofs)
         error(ETFatal, "can't write to %s", vars[var_index].c_str());
   }
}

void ArgReader::testAndSet(Uint var_index, const char* var_name, std::istream** is, std::ifstream& ifs)
{
   if (var_index < vars.size())
   {
      if (vars[var_index] == "-")
      {
         *is = &cin;
      }
      else
      {
         testAndSet(var_index, var_name, ifs);
         *is = &ifs;
      }
   }
}

void ArgReader::testAndSet(Uint var_index, const char* var_name, std::istream** is, iMagicStream& ifs)
{
   if (var_index < vars.size())
   {
      testAndSet(var_index, var_name, ifs);
      *is = &ifs;
   }
}

void ArgReader::testAndSet(Uint var_index, const char* var_name, std::ostream** os, std::ofstream& ofs)
{
   if (var_index < vars.size())
   {
      if (vars[var_index] == "-")
      {
         *os = &cout;
      }
      else
      {
         testAndSet(var_index, var_name, ofs);
         *os = &ofs;
      }
   }
}

void ArgReader::testAndSet(Uint var_index, const char* var_name, std::ostream** os, oMagicStream& ofs)
{
   if (var_index < vars.size())
   {
      testAndSet(var_index, var_name, ofs);
      *os = &ofs;
   }
}
