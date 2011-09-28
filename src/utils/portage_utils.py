#!/usr/bin/env python
# coding=utf-8

# @file portage_utils.py
# @brief Useful common Python classes and functions
# 
# @author Darlene Stewart
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada

from __future__ import print_function, unicode_literals, division, absolute_import

import sys
import argparse
import os

__all__ = ["printCopyright",
           "HelpAction", "VerboseAction", "DebugAction", 
           "set_debug","set_verbose", 
           "error", "fatal_error", "warn", "info", "debug", "verbose"
          ]

current_year = 2011

def printCopyright(program_name, start_year):
    """Print the standard NRC Copyright notice.
    
    The Crown Copyright will be asserted for start_year to latest release year.
    
    program_name: name of the program
    start_year: the first year of Copyright for the program;
    """
    if os.environ.get('PORTAGE_INTERNAL_CALL','0') == '0':
         print("\n{0}, NRC-CNRC, (c) {1}{2}, Her Majesty in Right of Canada".format(
               program_name, start_year,
               " - {0}".format(current_year) if current_year > start_year else ""),
               file=sys.stderr)
         print('Please run "portage_info -notice" for Copyright notices of 3rd party libraries.\n', 
               file=sys.stderr)


class HelpAction(argparse.Action):
    """argparse action class for displaying the help message to stderr.    
    e.g: parser.add_argument("-h", "-help", "--help", action=HelpAction)
    """
    def __init__(self, option_strings, dest, help="print this help message to stderr and exit"):
        super(HelpAction, self).__init__(option_strings, dest, nargs=0, 
                                         default=argparse.SUPPRESS,
                                         required=False, help=help)
    def __call__(self, parser, namespace, values, option_string=None):
        parser.print_help(file=sys.stderr)
        exit()

class VerboseAction(argparse.Action):
    """argparse action class for turning on verbose output.
    e.g: parser.add_argument("-v", "--verbose", action=VerboseAction)
    """
    def __init__(self, option_strings, dest, help="print verbose output to stderr [False]"):
        super(VerboseAction, self).__init__(option_strings, dest, nargs=0, 
                                            const=True, default=False, 
                                            required=False, help=help)

    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, True)
        set_verbose(True)

class DebugAction(argparse.Action):
    """argparse action class for turning on verbose output.
    e.g: parser.add_argument("-d", "--debug", action=DebugAction)
    """
    def __init__(self, option_strings, dest, help="print debug output to stderr [False]"):
        super(DebugAction, self).__init__(option_strings, dest, nargs=0, 
                                          const=True, default=False, 
                                          required=False, help=help)

    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, True)
        set_debug(True)


verbose_flag = False
debug_flag = False

def set_debug(flag):
    """Set value of the debug flag to control printing of debug messages."""
    global debug_flag
    debug_flag = flag

def set_verbose(flag):
    """Set value of the verbose flag to control printing of debug messages."""
    global verbose_flag
    verbose_flag = flag

def error(*args):
    """Print an error message to stderr."""
    print("Error:", *args, file=sys.stderr)
    return

def fatal_error(*args):
    """Print a fatal error message to stderr and exit with code 1."""
    print("Fatal error:", *args, file=sys.stderr)
    sys.exit(1)

def warn(*args):
    """Print an warning message to stderr."""
    print("Warn:", *args, file=sys.stderr)
    return

def info(*args, **kwargs):
    """Print information output to stderr."""
    print(*args, file=sys.stderr, **kwargs)

def debug(*args, **kwargs):
    """Print debug output to stderr if debug_flag (-d) is set."""
    if debug_flag:
        print("Debug:", *args, file=sys.stderr, **kwargs)

def verbose(*args, **kwargs):
    """Print verbose output to stderr if verbose_flag (-v) or debug_flag (-d) is set."""
    if verbose_flag or debug_flag:
        print(*args, file=sys.stderr, **kwargs)

if __name__ == '__main__':
    pass