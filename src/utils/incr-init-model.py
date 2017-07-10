#!/usr/bin/env python
# @file incr-init-model.py
# @brief Initialize the incremental model, creating its canoe.ini.
#
# @author Darlene Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2017, Sa Majeste la Reine du Chef du Canada /
# Copyright 2017, Her Majesty in Right of Canada

from __future__ import print_function, unicode_literals, division, absolute_import

import sys
import os.path
from argparse import ArgumentParser, RawDescriptionHelpFormatter
import codecs
from subprocess import check_call, check_output, CalledProcessError
import shutil
# import re
from collections import OrderedDict
from tempfile import NamedTemporaryFile

# If this script is run from within src/ rather than from the installed bin
# directory, we add src/utils to the Python module include path (sys.path)
# to arrange that portage_utils will be imported from src/utils.
if sys.argv[0] not in ('', '-c'):
   bin_path = os.path.dirname(sys.argv[0])
   if os.path.basename(bin_path) != "bin":
      sys.path.insert(1, os.path.normpath(os.path.join(bin_path, "..", "utils")))

# portage_utils provides a bunch of useful and handy functions, including:
#   HelpAction, VerboseAction, DebugAction (helpers for argument processing)
#   printCopyright
#   info, verbose, debug, warn, error, fatal_error
#   open (transparently open stdin, stdout, plain text files, compressed files or pipes)
from portage_utils import *


def get_args():
   """Command line argument processing."""

   usage="incr-init-model.py [options] [--] base_canoe_ini"
   help="""
   Initialize the incremental model in the current directory, creating its
   canoe.ini.
   
   Initial incremental MixLM and incremental MixTM models are created, mixing
   the incrmental model with the main LM or TM, respectively.
   
   If the main LM is a regular LM, then the weights assigned in the MixLM are
   INCR_LM_WT for the incremental LM and 1-INCR_LM_WT for the main LM. If the
   main LM is already a MixLM, then the incremental LM is added to the MixLM as
   an extra component with INCR_LM_WT as its weight, and the weights of the
   other models are scaled by 1-INCR_LM_WT.
   
   For the MixTM, either 1 or 4 incremental model weights must be provided; if
   one weight is given, it is used for all four columns; if four weights are
   provided, then each column is given its own weight. The incremental TM is
   given INCR_TM_WT as its weight and the main TM is given 1-INCR_TM_WT as its
   weight."""

   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, add_help=False,
                           formatter_class=RawDescriptionHelpFormatter)

   # Use our standard help, verbose and debug support.
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction)
   parser.add_argument("-d", "--debug", action=DebugAction)

   parser.add_argument("-lm-wt", "--incr-lm-weight", dest="incr_cmpt_lm_wt",
                        metavar="INCR_LM_WT", type=float, default=0.1,
                       help="""incremental component LM model weight
                               between 0.0 and 1.0 [%(default)s]""")
   parser.add_argument("-tm-wt", "--incr-tm-weights", dest="incr_cmpt_tm_wts",
                       metavar="INCR_TM_WT", type=float, nargs='+', default=[0.1],
                       help="""incremental component TM model weights (1: same
                              for all columns, or 4: separate weight for each column)
                              between 0.0 and 1.0 [%(default)s]""")
   parser.add_argument("-force", "--force-init", dest="force_init",
                       action='store_true', default=False,
                       help="force model initialization even if files exist. [%(default)s]")
   parser.add_argument("-quiet", "--quiet", dest="quiet",
                       action='store_true', default=False,
                       help="silence warning about model already initialized. [%(default)s]")

   parser.add_argument("base_canoe_ini", type=str,
                       help="""pathname of the canoe config file on which to
                              base this document model""")

   cmd_args = parser.parse_args()

   if not 0.0 < cmd_args.incr_cmpt_lm_wt < 1.0:
      fatal_error("Float in the range (0..1) required for incr-lm-weight:",
                  cmd_args.incr_cmpt_lm_wt)
   if len(cmd_args.incr_cmpt_tm_wts) not in (1, 4):
      fatal_error("Require exactly 1 (same for all columns) or",
                  "4 TM weights to be provided:", cmd_args.incr_cmpt_tm_wts)
   for wt in cmd_args.incr_cmpt_tm_wts:
      if not 0.0 < wt < 1.0:
         fatal_error("Float in the range (0..1) required for incr-tm-weights:", wt)
   
   return cmd_args


def run_command(cmd, descriptor="", ret_output=False, ignore_error=False, *popenargs, **kwargs):
   """ Run a system command, returning the stdout output if requested.
   
   Specify shell=True to pass the command in a bash shell; shell=True is
   automatically added if cmd is a string instead of a list.
   
   cmd: command to be run, either as a string or list.
   descriptor: identifying string for verbose/error messages.
   ret_output: return the stdout output if True, otherwise return the rc (0).
   ignore_error: If True, don't produce a fatal error for a CalledProcessError.
   popenargs: additional positional arguments for popen.
   kwargs: additional keyword arguments for check_call or check_output
   """
   # Quote arguments that contain spaces but don't start with a ' or ".
   if type(cmd) is list:
      quoted_cmd = ['"{0}"'.format(arg) if (' ' in arg and arg[0] not in "\"'")
                    else arg for arg in cmd]
      cmd_str = ' '.join(quoted_cmd)
   else:
      cmd_str = cmd
      if len(popenargs) > 7:
         popenargs[7] = True
      else:
         kwargs['shell'] = True
   verbose("Running", descriptor, "command:", cmd_str)
   shell = kwargs.get('shell') or len(popenargs) > 7 and popenargs[7]
   call_cmd = check_output if ret_output else check_call
   try:
      results = call_cmd(cmd_str if shell else cmd, *popenargs, **kwargs)
   except CalledProcessError as e:
      if not ignore_error:
         fatal_error(descriptor, e)
      elif call_cmd is check_output:
         results = e.output
      else:
         results = e.returncode
   except os.error as e:
      fatal_error(descriptor, "command failed (error: {0}): ".format(e), cmd_str)
   return results


class Config(OrderedDict):
   """Manage the incremental configuration and its file, incremental.config.
   
   The config file is expected to contain lines of the form:
   KEY=VALUE
   Blank lines and comment lines (starting with #) are also permitted.
   """
   default_config_name = "incremental.config"
   
   __skip_tag__ = "__skip__"

   def __init__(self, base_canoe_ini_path, base_config_path=None):
      super(Config, self).__init__()
      self.base_canoe_ini_path = base_canoe_ini_path
      self.base_config_path = base_config_path
      # Read and parse the incremental config file.
      self.skip_index = 0;
      found_error = False
      if base_config_path:
         with open(base_config_path) as cf:
            for line in cf:
               if line.lstrip().startswith("#") or len(line.strip()) == 0:
                  key = "{}{}".format(self.__skip_tag__, self.skip_index)
                  self.skip_index += 1
                  value = line.rstrip()
               else:
                  try:
                     key, value = line.split('=', 1)
                     key = key.strip()
                     if key == "":
                        raise ValueError()
                  except ValueError:
                     error("Malformed line in incremental model config file: ", line)
                     found_error = True
                  value = value.strip()
               self[key] = value
      # Set default values for required parameters
      default_config = OrderedDict()
      default_config["SRC_LANG"] = ""
      default_config["TGT_LANG"] = ""
      default_config["INCREMENTAL_TM_BASE"] = "cpt.incremental"
      default_config["INCREMENTAL_LM_BASE"] = "lm.incremental"
      for key in default_config:
         if key not in self: self[key] = default_config[key]
      # Check for required fields:
      for key in ("SRC_LANG", "TGT_LANG"):
         if not self.get_unquoted_value(key):
            error("Missing/empty required parameter in incremental model config file:", key)
            found_error = True
      if found_error:
         fatal_error("Problem(s) found in incremental model config file:",
                     self.base_config_path)

   def get_unquoted_value(self, key):
      value = self[key]
      if len(value) > 1 and value[0] in "'\"" and value[0] == value[-1]:
         return value[1:-1]
      return value

   def get_local_canoe_ini_name(self):
      return os.path.basename(self.base_canoe_ini_path)

   def get_local_canoe_ini_orig_name(self):
      return os.path.basename(self.base_canoe_ini_path)+".orig"

   def get_local_config_name(self):
      if self.base_config_path:
         return os.path.basename(self.base_config_path)
      return self.default_config_name

   def get_incr_cmpt_lm_name(self):
      return "{b}_{t}".format(b=self.get_unquoted_value("INCREMENTAL_LM_BASE"),
                              t=self.get_unquoted_value("TGT_LANG"))

   def get_incr_cmpt_tm_name(self):
      return "{b}.{s}2{t}".format(b=self.get_unquoted_value("INCREMENTAL_TM_BASE"),
                                  s=self.get_unquoted_value("SRC_LANG"),
                                  t=self.get_unquoted_value("TGT_LANG"))

   def get_incr_mixlm_name(self):
      return "lm-incr.mixlm"

   def get_incr_mixtm_name(self):
      return "cpt-incr.mixtm"

   def write_local_config_file(self, force_init=False):
      local_config = self.get_local_config_name()
      if not force_init and os.path.exists(local_config):
         fatal_error("Incremental config file already exists:", local_config)
      with open(local_config, 'w') as cf:
         for key in self:
            if key.startswith(self.__skip_tag__):
               print(self[key], file=cf)
            else:
               print(key, '=', self[key], sep='', file=cf)


def init_local_model(config, force_init=False):
   """Initialize a local copy of the PortageLive model.
   
   config: incremental Config object
   force_init: if true, force model initialization even if files/symlinks already exist.
   """
   base_canoe_ini = config.base_canoe_ini_path
   verbose("Initializing a local copy of the PortageLive model for:", base_canoe_ini)
   # Check that the base_canoe_ini exists
   if not os.path.exists(base_canoe_ini):
      fatal_error("Base canoe_ini file does not exist:", base_canoe_ini)

   base_model_dir = os.path.dirname(base_canoe_ini)
   local_canoe_ini = config.get_local_canoe_ini_name()
   if not force_init and os.path.exists(local_canoe_ini):
      fatal_error("Local canoe config file already exists:", local_canoe_ini)

   def make_symlink(src, dst):
      if os.path.exists(dst):
         if force_init:
            os.unlink(dst)
         else:
            fatal_error("symlink already exists:", dst)
      os.symlink(src, dst)

   # Create a symlink to the base_canoe_ini file as canoe.ini.cow.orig, for example.
   local_canoe_ini_orig = config.get_local_canoe_ini_orig_name()
   make_symlink(base_canoe_ini, local_canoe_ini_orig)

   # Create symlinks to other contents of the static model directory
   skip_items = frozenset((local_canoe_ini, config.get_local_config_name(), "README", "md5"))
   items = frozenset(os.listdir(base_model_dir)) - skip_items
   for item in items:
      make_symlink(os.path.join(base_model_dir, item), item)

   # Check that the model is good
   okay = run_command(["configtool", "check", local_canoe_ini_orig], 
                      ret_output=True, ignore_error=True)
   if okay != "ok":
      fatal_error


def create_starter_incr_cmpt_lm(incr_cmpt_lm_name, force_init=False):
   """"Create the starter incremental component LM.
   
   incr_cmpt_lm_name: name of the incremental component LM.
   force_init: if true, force model initialization even if models already exist.
   """
   if not force_init and os.path.exists(incr_cmpt_lm_name):
      fatal_error("Incremental component LM already exists:", incr_cmpt_lm_name)
   if not force_init and os.path.exists(incr_cmpt_lm_name+".tplm"):
      fatal_error("Incremental component TPLM already exists:", incr_cmpt_lm_name+".tplm")
   verbose("Creating the starter incremental component LM:", incr_cmpt_lm_name)
   with open(incr_cmpt_lm_name, 'w') as incr_cmpt_lm_fd:
      print("", file=incr_cmpt_lm_fd)
      print("\\data\\", file=incr_cmpt_lm_fd)
      print("ngram 1=3", file=incr_cmpt_lm_fd)
      print("ngram 2=3", file=incr_cmpt_lm_fd)
      print("ngram 3=2", file=incr_cmpt_lm_fd)
      print("", file=incr_cmpt_lm_fd)
      print("\\1-grams:", file=incr_cmpt_lm_fd)
      print("-0.301030\t</s>", file=incr_cmpt_lm_fd)
      print("-99\t<s>\t-99", file=incr_cmpt_lm_fd)
      print("0.000000\t__DUMMY__\t-99", file=incr_cmpt_lm_fd)
      print("", file=incr_cmpt_lm_fd)
      print("\\2-grams:", file=incr_cmpt_lm_fd)
      print("0.000000\t<s> __DUMMY__\t-99", file=incr_cmpt_lm_fd)
      print("-0.301030\t__DUMMY__ </s>\t-99", file=incr_cmpt_lm_fd)
      print("-0.301030\t__DUMMY__ __DUMMY__\t-99", file=incr_cmpt_lm_fd)
      print("", file=incr_cmpt_lm_fd)
      print("\\3-grams:", file=incr_cmpt_lm_fd)
      print("0.000000\t<s> __DUMMY__ __DUMMY__", file=incr_cmpt_lm_fd)
      print("0.000000\t__DUMMY__ __DUMMY__ </s>", file=incr_cmpt_lm_fd)
      print("\\end\\", file=incr_cmpt_lm_fd)
   verbose("Creating the starter incremental component TPLM:", incr_cmpt_lm_name+".tplm")
   run_command("arpalm2tplm.sh {} &> tp.{}.log".format(incr_cmpt_lm_name,incr_cmpt_lm_name))

def create_incr_mixlm(incr_mixlm_name, main_lm_name, incr_cmpt_lm_name, incr_cmpt_lm_wt, force_init=False):
   """ Create the incremental mixLM.
   
   incr_mixlm_name: name of the incremental mixLM file
   main_lm_name: pathname of the main LM file
   incr_cmpt_lm_name: name of the incremental component LM file
   incr_cmpt_lm_wt: incremental component LM model weight between 0.0 and 1.0
   force_init: if true, force model initialization even if models already exist.
   """
   if not force_init and os.path.exists(incr_mixlm_name):
      fatal_error("Incremental mixLM already exists:", incr_mixlm_name)
   verbose("Creating the incremental mixLM:", incr_mixlm_name)
   if not incr_cmpt_lm_name.endswith(".tplm"):
      incr_cmpt_lm_name+=".tplm"
   with open(incr_mixlm_name, 'w') as incr_mixlm_fd:
      if main_lm_name.endswith(".mixlm"):
         main_lm_dir = os.path.dirname(main_lm_name)
         with open(main_lm_name) as main_mixlm_fd:
            for line in main_mixlm_fd:
               name, wt = line.split()
               if os.path.isabs(name) or main_lm_dir == ".":
                  new_path = name
               else:
                  new_path = os.path.join(main_lm_dir, name)
               new_wt = float(wt) * (1 - incr_cmpt_lm_wt)
               print("{}\t{}".format(new_path, new_wt), file=incr_mixlm_fd)
            print("{}\t{}".format(incr_cmpt_lm_name, incr_cmpt_lm_wt), file=incr_mixlm_fd)
      else:
         print("{}\t{}".format(main_lm_name, 1 - incr_cmpt_lm_wt), file=incr_mixlm_fd)
         print("{}\t{}".format(incr_cmpt_lm_name, incr_cmpt_lm_wt), file=incr_mixlm_fd)

def create_starter_incr_cmpt_tm(incr_cmpt_tm_name, force_init=False):
   """"Create the starter incremental component TM.
   
   incr_cmpt_tm_name: name of the incremental component TM.
   force_init: if true, force model initialization even if models already exist.
   """
   if not force_init and os.path.exists(incr_cmpt_tm_name):
      fatal_error("Incremental component TM already exists:", incr_cmpt_tm_name)
   if not force_init and os.path.exists(incr_cmpt_tm_name+".tppt"):
      fatal_error("Incremental component TPPT already exists:", incr_cmpt_tm_name+".tppt")
   verbose("Creating the starter incremental component TM:", incr_cmpt_tm_name)
   with open(incr_cmpt_tm_name, 'w') as incr_cmpt_tm_fd:
      print("__DUMMY__ ||| __DUMMY__ ||| 1 1.17549e-38 1 1.17549e-38 a=0 c=2", file=incr_cmpt_tm_fd)
      print("__DUMMY__ __DUMMY__ ||| __DUMMY__ __DUMMY__ ||| 1 1.17549e-38 1 1.17549e-38 a=0_1 c=1", file=incr_cmpt_tm_fd)
   verbose("Creating the starter incremental component TPPT:", incr_cmpt_tm_name+".tppt")
   run_command("textpt2tppt.sh {} &> tp.{}.log".format(incr_cmpt_tm_name,incr_cmpt_tm_name))

def create_incr_mixtm(incr_mixtm_name, main_tm_name, incr_cmpt_tm_name, incr_cmpt_tm_wts, force_init=False):
   """ Create the incremental mixTM.
   
   incr_mixtm_name: name of the incremental mixTM file
   main_tm_name: pathname of the main TM file
   incr_cmpt_tm_name: name of the incremental component TM file
   incr_cmpt_tm_wts: list of incremental component TM model weights (1 or 4),
      each between 0.0 and 1.0
   force_init: if true, force model initialization even if models already exist.
   """
   if not force_init and os.path.exists(incr_mixtm_name):
      fatal_error("Incremental mixTM already exists:", incr_mixtm_name)
   verbose("Creating the incremental mixTM:", incr_mixtm_name)
   if not incr_cmpt_tm_name.endswith(".tppt"):
      incr_cmpt_tm_name+=".tppt"
   if len(incr_cmpt_tm_wts) == 1:
      incr_cmpt_tm_wts.extend(incr_cmpt_tm_wts[0] for i in range(3))
   with open(incr_mixtm_name, 'w') as incr_mixtm_fd:
      print("Portage dynamic MixTM v1.0", file=incr_mixtm_fd)
      wts=(str(1.0 - incr_cmpt_tm_wts[i]) for i in range(4))
      print("{}\t{}".format(main_tm_name, ' '.join(wts)), file=incr_mixtm_fd)
      wts=(str(incr_cmpt_tm_wts[i]) for i in range(4))
      print("{}\t{}".format(incr_cmpt_tm_name, ' '.join(wts)), file=incr_mixtm_fd)

def create_incr_canoe_ini(config, lms, main_lm_idx, tms, main_tm_idx, force_init=False):
   """ Create the incremental canoe ini file.
   
   Assumes the incremental component LM and TM exist, as well as the
   incremental mixLM and mixTM files.
   
   config: incremental Config object
   lms: list of LM paths extracted from the base canoe ini file
   main_lm_idx: index of the main LM in lms
   tms: list of TM paths extracted from the base canoe ini file
   main_tm_idx: index of the main TM in tms
   force_init: if true, force model initialization even if models already exist.
   """
   incr_canoe_ini_name = config.get_local_canoe_ini_name()
   if not force_init and os.path.exists(incr_canoe_ini_name):
      fatal_error("Incremental canoe ini file already exists:", incr_canoe_ini_name)
   verbose("Creating the incremental canoe ini file:", incr_canoe_ini_name)
   new_lms = list(lms)
   new_lms[main_lm_idx] = config.get_incr_mixlm_name()
   new_tms = list(tms)
   new_tms[main_tm_idx] = config.get_incr_mixtm_name()
   model_args = "args:-lmodel-file {lms} -ttable-tppt -- -ttable {tms} -lock".format(
                lms=":".join(new_lms), tms=":".join(new_tms))
   with NamedTemporaryFile(prefix=incr_canoe_ini_name+".", dir=".",
                           delete=False) as incr_canoe_ini_tmp_fd:
      run_command(["configtool", "-p", model_args, config.get_local_canoe_ini_orig_name()],
                  stdout=incr_canoe_ini_tmp_fd)
      incr_canoe_ini_tmp_name = incr_canoe_ini_tmp_fd.name
   okay = run_command(["configtool", "check", incr_canoe_ini_tmp_name], 
                      ret_output=True, ignore_error=True)
   if okay.strip() != "ok":
      os.symlink(config.get_local_canoe_ini_orig_name(), incr_canoe_ini_name)
      fatal_error("Problem with incremental canoe config.", 
                  "Rolling back to original canoe ini file."
                  "Problem canoe config saved in:", incr_canoe_ini_tmp_name)
   os.rename(incr_canoe_ini_tmp_name, incr_canoe_ini_name)

def main():
   printCopyright("incr-init-model.py", 2017)
   os.environ['PORTAGE_INTERNAL_CALL'] = '1'   # add this if needed

   cmd_args = get_args()

   # The following allows stderr to handle non-ascii characters:
   sys.stderr = codecs.getwriter("utf-8")(sys.stderr)

   # Read and parse the incremental model config file
   config_pathname = os.path.join(os.path.dirname(cmd_args.base_canoe_ini),
                                  Config.default_config_name)
   if not os.path.exists(config_pathname):
      fatal_error("Incremental config file does not exist:", config_pathname)
   verbose("Reading incremental config file:", config_pathname)
   config = Config(cmd_args.base_canoe_ini, config_pathname)

   # Detect if model already initialized.
   local_canoe_ini = config.get_local_canoe_ini_name()
   if not cmd_args.force_init and os.path.exists(local_canoe_ini):
      if not cmd_args.quiet:
         warn("Local canoe ini", "({})".format(local_canoe_ini), "already exists, so",
              "incremental model appears to be initialized already; not re-initializing.")
      exit(0)

   init_local_model(config, cmd_args.force_init)
   
   local_canoe_ini_orig = config.get_local_canoe_ini_orig_name()

   wts_strs = run_command(["configtool", "weights", local_canoe_ini_orig], ret_output=True).split()
   wts = {}
   for i in range(0, len(wts_strs), 2):
      wts[wts_strs[i][1:]] = tuple(float(w) for w in wts_strs[i+1].split(':'))

   lms = run_command(["configtool", "list-lm", local_canoe_ini_orig], ret_output=True).split()
   lm_wts = wts["lm"]
   main_lm_idx = lm_wts.index(max(lm_wts))
   main_lm_name = lms[main_lm_idx]
   verbose("Main lm:", main_lm_name)

   tms = run_command(["configtool", "list-tm", local_canoe_ini_orig], ret_output=True).split()
   if len(tms) > 1:
      warn("Expected a single tm, but found", len(tms), "in", local_canoe_ini_orig,
           "; including only the first one in the mixTM for incremental.")
   main_tm_idx = 0
   main_tm_name = tms[main_tm_idx]
   verbose("Main tm:", main_tm_name)

   ntms = int(run_command(["configtool", "nt", local_canoe_ini_orig], ret_output=True))
   if len(tms) == 1:
      if ntms != 2:
         fatal_error("Incremental training supports only 4 column TMs, but",
                     main_tm_name, "has", ntms*2, "columns.")
   else:
      warn("Incremental training supports only 4 column TMs.", main_tm_name,
           "is expected to have 4 columns." )

   create_starter_incr_cmpt_lm(config.get_incr_cmpt_lm_name(), cmd_args.force_init)
   
   create_incr_mixlm(config.get_incr_mixlm_name(), main_lm_name,
                     config.get_incr_cmpt_lm_name(), cmd_args.incr_cmpt_lm_wt,
                     cmd_args.force_init)

   create_starter_incr_cmpt_tm(config.get_incr_cmpt_tm_name(), cmd_args.force_init)
   
   create_incr_mixtm(config.get_incr_mixtm_name(), main_tm_name,
                     config.get_incr_cmpt_tm_name(), cmd_args.incr_cmpt_tm_wts,
                     cmd_args.force_init)
   
   create_incr_canoe_ini(config, lms, main_lm_idx, tms, main_tm_idx, cmd_args.force_init)

   config.write_local_config_file(cmd_args.force_init)

if __name__ == "__main__":
   main()
