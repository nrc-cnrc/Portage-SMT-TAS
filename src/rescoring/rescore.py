#!/usr/bin/env python

# @file resecore.py
# @brief Driver script to train, or translate with, a rescoring model.
# 
# @author Darlene Stewart
# 
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

from __future__ import print_function, unicode_literals, division, absolute_import

import sys
import os.path
from argparse import ArgumentParser, RawDescriptionHelpFormatter, ArgumentTypeError
import codecs
from subprocess import call, check_output, CalledProcessError
from datetime import datetime
import time
import glob
import re
import shutil
import tempfile
import shlex


# If this script is run from within src/ rather than from the installed bin
# directory, we add src/utils to the Python module include path (sys.path)
# to arrange that portage_utils will be imported from src/utils.
if sys.argv[0] not in ('', '-c'):
   bin_path = os.path.dirname(sys.argv[0])
   if os.path.basename(bin_path) != "bin":
      sys.path.insert(1, os.path.normpath(os.path.join(bin_path, "..", "utils")))

from portage_utils import *

def prog_name():
   if sys.argv[0] in ('', '-c'):
      return "rescore.py"
   return os.path.basename(sys.argv[0])

def prog_dir():
   if sys.argv[0] in ('', '-c'):
      return "."
   return os.path.dirname(sys.argv[0])


def is_readable_file(f):
   return os.path.isfile(f) and os.access(f, os.R_OK)

def is_writable_file(f):
   return os.path.isfile(f) and os.access(f, os.W_OK)

def is_writable_dir(d):
   return os.path.isdir(d) and os.access(d, os.W_OK)


master_start_time = time.time()

def print_master_wall_time():
   info("Master-Wall-Time ", int(time.time() - master_start_time + .5), "s", sep='')


def run_command(cmd, descriptor="", time_it=False, *popenargs, **kwargs):
   # Put quotes around arguments that don't start with a single or double quote.
   quoted_cmd = ['"{0}"'.format(arg) if (' ' in arg and arg[0] not in "\"'")
                 else arg for arg in cmd]
   verbose("Running", descriptor, "command:", ' '.join(quoted_cmd))
   if time_it:
      (mon_fd, mon_file) = tempfile.mkstemp(prefix="mon.run_cmd.{0}.".format(cmd[0]), 
                                            dir=workdir)
      os.close(mon_fd)
   do_tm = "time-mem -mon {0} ".format(mon_file) if time_it else ""
   try:
      if kwargs.get('shell') or len(popenargs) > 7 and popenargs[7]:
         rc = call(do_tm + ' '.join(quoted_cmd), *popenargs, **kwargs)
      else:
         if not time_it:
            rc = call(cmd, *popenargs, **kwargs)
         else:
            rc = call(do_tm + ' '.join(quoted_cmd), shell=True, *popenargs, **kwargs)
   except os.error as e:
      print_master_wall_time()
      fatal_error(descriptor, "command failed (error:{0}): ".format(e), ' '.join(cmd))
   if rc is not 0:
      print_master_wall_time()
      fatal_error(descriptor, "command failed (rc={0}):".format(rc), ' '.join(cmd))


class Mode(object):
   none = 0
   train = 1
   trans = 2
   ffgen = 3

def pos_int(arg):
   """Positive integer type for argparse"""
   try:
      value = int(arg)
      if value < 1: raise ValueError()
   except ValueError:
      raise ArgumentTypeError("invalid positive int value: '{0}'".format(arg))
   return value

def get_args():
   """Parse the command line arguments.
   
   Return the parsed arguments in an argparse.Namespace object.
   """
   
   usage="rescore.py (-train|-trans|-ffgen) [options] MODEL SRC [REFS...]"
   help="""
   Train, or translate with, a rescoring model. First generate nbest lists and
   ffvals for a given source file, then generate external the feature files
   required for rescoring. Finally, unless in -ffgen mode, either train a model
   using reference texts (-train), or use an existing model to translate the
   source text (-trans). Note: parallelized decoding and feature generation are
   used.
   
   All intermediate files are written to a working directory, either specified
   by -w or created below the current directory with a name derived from the
   arguments.
   
   Note: any existing intermediate files will not be overwritten unless -F is
   specified; thus, when one adds extra features to an existing model, only
   the new features are calculated.
   """
   model_epilog="""Rescoring MODEL File Format:
   Use rescore_train -h for a description of the rescoring model syntax, and
   rescore_train -H for a list of features. If training, the final model is
   written to MODEL.rat. rescore.py automatically generates a file 'ffvals'
   containing all decoder feature values, so entries like 'FileFF:ffvals,i'
   can be used in MODEL to refer to columns in this file.
   rescore.py gives special interpretation to five magic tags within MODEL:
   <src>        will be replaced by SRC's basename. This can be used to
                specify feature arguments that depend on the current
                source file.
   <ffval-wts>  designates an automatically-generated rescoring model
                that contains decoder features and their weights. This
                is required by the nbest* features.
   <pfx>        will be replaced by the prefix for all intermediate files,
                WORKDIR/PFX (where WORKDIR is the work directory, and PFX is 
                the argument to -pfx). This can be used for features like the
                nbest* ones, which need to access intermediate files.
   <nbest>      will be replaced by the nbest list file generated by rescore.py.
   <NP>         will be replaced by the number of processors specified by -n.
                This is mainly intended for the SCRIPT features.
   """

   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, epilog=model_epilog,
                           formatter_class=RawDescriptionHelpFormatter, add_help=False)

   grp_mode = parser.add_argument_group("mode selection options (one required)")
   modes = grp_mode.add_mutually_exclusive_group(required=True)
   modes.add_argument('-train', '--train', dest="mode",
                      action='store_const', const=Mode.train,
                      help='''Select training mode. In training mode, one or
                           more reference texts must be provided.''')
   modes.add_argument('-trans', '--trans', dest="mode",
                      action='store_const', const=Mode.trans,
                      help='''Select translation mode. In translation mode, the
                           rescored translation is written to the file MSRC.rat
                           (where MSRC is the argument to -msrc).''')
   modes.add_argument('-ffgen', '--ffgen', dest="mode",
                      action='store_const', const=Mode.ffgen,
                      help='''Select feature file generation mode, i.e. stop
                           after feature file generation, don't rescore.''')

   parser.add_argument("model", type=str, metavar="MODEL",
                       help="Rescoring model (see below for detailed description).")
   parser.add_argument("src", type=str, metavar="SRC",
                       help="Source text file name.")
   parser.add_argument("refs", type=str, nargs="*", default=[], metavar="REFS",
                       help="File names of one or more reference translations.")

   grp_cp = parser.add_argument_group("decoding options")
   grp_cp.add_argument("-f", "--canoe-config", dest="canoe_config", type=str,
                       default="canoe.ini", metavar="CONFIG",
                       help='''Canoe config file. (To use a configmap file, 
                            specify it as the -f option in CP_OPTS.)
                            [%(default)s]''')
   grp_cp.add_argument("--cp-opts", type=str, action="append", default=[],
                       metavar="CP-OPTS",
                       help='''Options for canoe-parallel.sh (e.g. '-cleanup').
                            Only those options that come before the 'canoe'
                            keyword to canoe-parallel.sh are legal here.
                            CP-OPTS must be quoted if it includes whitespace.
                            Multiple --cp-opts options are permitted.''')
   grp_cp.add_argument("-p", "--cp-numpar", dest="cp_n", type=pos_int,
                       metavar="CP-NUMPAR",
                       help='''Maximum number of parallel decoding jobs for nbest
                            list generation (-n option to canoe-parallel.sh).
                            [None: canoe-parallel.sh -n default]''')
   grp_cp.add_argument("-c", "--cp-ncpus", dest="cp_ncpus", type=pos_int,
                       default=1, metavar="CP-NCPUS",
                       help='''Number of cpus per decoding job. [%(default)s]''')
   grp_cp.add_argument("-n", "--nbest-size", dest="nbsize", type=pos_int, default=1000,
                       help="Size of nbest lists. [%(default)s]")
   grp_cp.add_argument("-msrc", "--marked-src", dest="msrc", type=str,
                       help='''A version of the source file marked up with
                            rule-based translations, used for canoe input but
                            not for feature generation [SRC]''')
   grp_cp.add_argument("--check-dep", action='store_true', default=False,
                       help='''Check that the nbest is more recent than the
                            canoe CONFIG, and regenerate it if not. Similarly, 
                            feature functions are regenerated unless they are
                            more recent than the nbest list. This mimics "make"
                            because the nbest depends on the canoe CONFIG.
                            [don't regenerate existing files]''')

   grp_fg = parser.add_argument_group("feature generation options")
   grp_fg.add_argument("-F", "--force-overwrite", action='store_true', default=False,
                       help='''Force overwrite of existing feature function files.
                            [don't overwrite]''')
   grp_fg.add_argument("-pg", "--gfp-numpar", dest="gfp_n", type=pos_int,
                       metavar="GFP-NUMPAR",
                       help='''Maximum number of parallel jobs for feature generation. 
                            [-p CP-NUMPAR value if specified, else 3]''')
   grp_fg.add_argument("-cg", "--gfp-ncpus", dest="gfp_ncpus", type=pos_int,
                       metavar="GFP-NCPUS",
                       help='''Number of cpus per decoding job. [-c CP-NCPUS value]''')
   grp_fg.add_argument("--gfp-opts", type=str, action="append", default=[],
                       metavar="GFP-OPTS",
                       help='''Options for gen-features-parallel.pl (e.g. '-J 30').
                            Most options are automatically set by rescore.py.
                            GFP-OPTS must be quoted if it includes whitespace.
                            Multiple --gfp-opts options are permitted.''')
   grp_fg.add_argument("-sproxy", "--src-proxy", dest="sproxy", type=str,
                       default="", metavar="SPROXY",
                       help='''Use SPROXY instead of SRC when substituting for
                            <src> tag in MODEL.''')

   grp_rt = parser.add_argument_group("rescore -train/-trans options")
   grp_rt.add_argument("-a", "--algorithm", type=str, metavar="ALG+ARGS",
                       help='''In train mode, 
                            optimizer algorithm+arguments string, one of:
                            powell [rescore_train-opts], 
                            mira [C [I [E [B [H [O [D]]]]]]].
                            [powell]''')
   grp_rt.add_argument("-o", "--model-out", type=str, metavar="MODEL-OUT",
                       help='''In train mode (-train), write the final model to
                            MODEL_OUT [MODEL.rat]''')
   grp_rt.add_argument("-s", "--seed", type=int, default=0,
                       help='''Start seed for random number generator in mira.
                            [%(default)s]''')
   grp_rt.add_argument("--bleu-order", type=pos_int, default=4, metavar="ORDER",
                       help="N-gram order used when optimizing BLEU. [%(default)s]")
   grp_rt.add_argument("--rtrans-opts", type=str, action="append", default=[],
                       metavar="OPTS",
                       help='''Options for rescore_translate.
                            OPTS must be quoted if it includes whitespace.
                            Multiple --rtrans-opts options are permitted.''')

   grp_gen = parser.add_argument_group("general options")
   grp_gen.add_argument("-w", "--workdir", type=str,
                        help='''Work directory to use for rescoring.
                             [workdir-MSRC-NBbest]''')
   grp_gen.add_argument("-pfx", "--prefix", dest="pfx", type=str, default="",
                        help='''Prefix for all intermediate files (1best, nbest,
                             ffvals, pal, feature files) within the working
                             directory. This switch can be used to store results
                             from several different runs in the same working
                             directory. If PFX is non-empty, the output in
                             translation mode is named PFXrat. ["%(default)s"]''')
   # Use our standard help, verbose and debug support.
   grp_gen.add_argument("-h", "-help", "--help", action=HelpAction)
   grp_gen.add_argument("-v", "--verbose", action=VerboseMultiAction)
   grp_gen.add_argument("-d", "--debug", action=DebugAction)

   try:
      cmd_args = parser.parse_args()
   except IOError as e:
      fatal_error("cannot open: '{0}': {1}".format(e.filename, e))

   # Set some argument defaults
   
   if cmd_args.mode is Mode.train:
      cmd_args.model_out = cmd_args.model_out or os.path.basename(cmd_args.model) + '.rat'
   elif cmd_args.model_out is not None:
      fatal_error("-o is not valid only for --trans/--ffgen (valid only for --train).")

   if cmd_args.mode is Mode.train and len(cmd_args.refs) == 0:
      fatal_error("Reference texts are needed for training (--train).")
   
   if cmd_args.mode is Mode.train:
      cmd_args.algorithm = cmd_args.algorithm or 'powell'
   elif cmd_args.algorithm is not None:
      fatal_error("-a is not valid for --trans/--ffgen (valid only for --train).")

   cmd_args.msrc = cmd_args.msrc or cmd_args.src
   
   cmd_args.gfp_n = cmd_args.gfp_n or cmd_args.cp_n or 3
   cmd_args.gfp_ncpus = cmd_args.gfp_ncpus or cmd_args.cp_ncpus

   cmd_args.workdir = cmd_args.workdir or \
                      "workdir-{0}-{1}best".format(os.path.basename(cmd_args.msrc),
                                                   cmd_args.nbsize)

   verbose(" ".join(sys.argv))
   debug(cmd_args)
   
   # Validate file arguments
   ferror = False

   def file_error(f, type="", mode='r'):
      error(" ", type, " file '", f, "' is not a ", 
            "writable" if mode == 'w' else "readable", " file.", sep='')
      return True

   if not is_readable_file(cmd_args.model):
      ferror = file_error(cmd_args.model, "Rescoring MODEL")

   if not is_readable_file(cmd_args.src):
      ferror = file_error(cmd_args.src, "SRC")

   for f in cmd_args.refs:
      if not is_readable_file(f):
         ferror = file_error(f, "Reference")

   if not is_readable_file(cmd_args.canoe_config):
      ferror = file_error(cmd_args.canoe_config, "Canoe CONFIG")

   if not is_readable_file(cmd_args.msrc):
      ferror = file_error(cmd_args.msrc, "MSRC")

   if cmd_args.mode is Mode.train:
      if os.path.exists(cmd_args.model_out) and not is_writable_file(cmd_args.model_out) \
            or not is_writable_dir(os.path.dirname(cmd_args.model_out) or "."):
         ferror = file_error(cmd_args.model_out, "MODEL_OUT", 'w')
   
   if ferror: fatal_error("Aborting due to file errors.")

   return cmd_args

def get_algorithm_args(alg_plus_args):
   """Parse the -a algorithm+arguments string for train mode.

   The algorithm+arguments string may be one of:
      powell [rescore-opts]
      mira [C [I [E [B [H [O [D]]]]]]]

   alg_plus_args: algorithm+arguments string to parse
   returns: parsed arguments in an argparse.Namespace object.
   """

   def float_str(arg):
      try:
         value = float(arg)
      except ValueError:
         raise ArgumentTypeError("invalid float value: '{0}'".format(arg))
      return arg

   prog = '{0} ... -a'.format(prog_name())

   parser = ArgumentParser(prog=prog,add_help=False)
   subparsers = parser.add_subparsers(dest='algorithm')
   parser_powell = subparsers.add_parser('powell', add_help=False,
                      usage="{0} 'powell [rescore_train-opts]'".format(prog))
   parser_mira = subparsers.add_parser('mira', add_help=False,
                      usage="{0} 'mira [C [I [E [B [H [O [D]]]]]]]'".format(prog))
   parser_mira.add_argument('C', type=float_str, nargs='?', default='1e-02',
                            help='learning rate; 1e-4 recommended for B=1, 1e-8 for B=2')
   parser_mira.add_argument('I', type=int, nargs='?', default=30, 
                            help='number of iterations')
   parser_mira.add_argument('E', type=int, nargs='?', default=1, 
                            help='number of neg examples')
   parser_mira.add_argument('B', type=int, nargs='?', default=-4,
                            help='if >0, col to find BLEU in allbleus file')
   parser_mira.add_argument('H', choices=['true', 'false'], nargs='?',
                            default='true', help='hope update?')
   parser_mira.add_argument('O', choices=['Model', 'Oracle', 'Orange'], 
                            nargs='?', default='Oracle',
                            help='use Model, Oracle or Orange as background')
   parser_mira.add_argument('D', type=float_str, nargs='?', default='0.999',
                            help='Rate at which to decay Model or Oracle BG')

   alg_args, extra_args = parser.parse_known_args(shlex.split(alg_plus_args))

   if alg_args.algorithm == 'mira':
      if len(extra_args) is not 0:
         parser_mira.error("unrecognized arguments: " + ' '.join(extra_args))
   else:
      alg_args.rtrain_opts = extra_args

   return alg_args

def convert_rescore_model(model_in, model_rat_out, model_out):
   """Transform the trained rescore-model from RAT syntax to normal syntax.
   
   This transfers the scores from the rat_out_model to orig_model, outputting
   the result in out-model.
   
   model_in: original input rescore_model file (with/without scores)
   model_rat_out: trained rescore_model file in RAT syntax
   model_out: output rescore_model file with scores from model_rat_out and
      feature names from model_in
   """
   ff_wt_re = re.compile(r'(.*?)(?:\s+((?:-?[0-9]*\.?[0-9]*)(?:[Ee]-?[0-9]+)?))?$')
   with open(model_in, 'r') as model_in_file:
      with open(model_rat_out, 'r') as model_rat_out_file:
         with open(model_out, 'w') as model_out_file:
            for line_num, model_in_line in enumerate(model_in_file, start=1):
               model_line = model_in_line.strip()
               model_match = ff_wt_re.match(model_line)
               wts_line = model_rat_out_file.readline().strip()
               wts_match = ff_wt_re.match(wts_line)
               # Sanity check
               if not model_match.group(1):
                  error("No feature in model at line", line_num)
                  debug("model_line:", model_line)
                  debug("model group 1 match:", model_match.group(1))
                  debug("model group 2 match:", model_match.group(2))
               if not wts_match.group(2):
                  error("Missing weight at line", line_num)
                  debug("wts_line:", wts_line)
                  debug("wts group 1 match:", wts_match.group(1))
                  debug("wts group 2 match:", wts_match.group(2))
               if model_line.startswith("#"):         # comment
                  wts_start = model_line
               elif model_line.startswith("FileFF:"): # FileFF:ffvals,N
                  wts_start = model_match.group(1)
               else:                                  # featName:...
                  wts_start = "FileFF:ff." + model_match.group(1).split(':', 1)[0]
               if not wts_match.group(1).startswith(wts_start):
                  warn("Possible model-weight mismatch at line", line_num)
                  debug("model_line:", model_line)
                  debug("model group 1 match:", model_match.group(1))
                  debug("model group 2 match:", model_match.group(2))
                  debug("wts_line:", wts_line)
                  debug("wts group 1 match:", wts_match.group(1))
                  debug("wts group 2 match:", wts_match.group(2))
               # Write out the original feature function name and its weight
               print(model_match.group(1), wts_match.group(2), file=model_out_file)
            if model_rat_out_file.readline():
               fatal_error(model_rat_out, "contains more lines than", model_in)


def main():
   printCopyright(prog_name(), 2013);
   os.environ['PORTAGE_INTERNAL_CALL'] = '1';   # add this if needed

   cmd_args = get_args()
   
   verbose("rescore.py starting on", datetime.today(), "\n")

   # Create the workdir, store its name globally so run_command can use it.
   global workdir
   if cmd_args.workdir:
      workdir = cmd_args.workdir
   else:
      workdir = "workdir-{0}-{1}best".format(os.path.basename(cmd_args.msrc),
                                             cmd_args.nbsize)
   if not os.path.isdir(workdir):
      try:
         os.mkdir(workdir)
      except os.error as e:
         fatal_error(" Cannot create workdir '", workdir, "': ", e, sep='')

   orig_pfx = cmd_args.pfx or os.path.basename(cmd_args.msrc) + "."
   pfx = os.path.join(workdir, cmd_args.pfx)

   # Step 1. Run canoe to generate Nbest lists, ffvals and pal files.
   verbose("Generating {0}best lists:".format(cmd_args.nbsize))

   # Like tune.py, filtering is expected to be done already (e.g. by the framework).

   # Check that the Nbest is more recent than the canoe.ini.
   nb_str = "{0}best".format(cmd_args.nbsize)
   nbest = None
   deleted_nb = 0
   for ext in ["", ".gz"]:
      nb = "{0}{1}{2}".format(pfx, nb_str, ext)
      if not os.path.exists(nb): continue
      if not is_readable_file(nb):
         ferror = file_error(nb, nb_str)
      if cmd_args.check_dep and \
            os.path.getmtime(nb) < os.path.getmtime(cmd_args.canoe_config):
         warn("Removing old Nbest file:", nb)
         os.unlink(nb)
         deleted_nb += 1
      else:
         nbest = nb
   if deleted_nb:
      if not nbest:
         warn("Regenerating Nbest since it was older than", cmd_args.canoe_config)
      else:
         warn("Not regenerating Nbest since there is an Nbest newer than", cmd_args.canoe_config)

   pal = "{0}pal.gz".format(pfx)
   ffvals = "{0}ffvals.gz".format(pfx)
   if nbest:
      info("NBest file", nbest, "exists - skipping", nb_str, 
           "and ffvals and pal generation.")
      if not os.path.exists(pal):
         pal = "{0}pal".format(pfx)
      if not os.path.exists(ffvals):
         ffvals = "{0}ffvals".format(pfx)
   else:
      if os.path.exists(pal):
         warn("Phrase alignment file ", pal, "exists already - overwriting it!")
         os.unlink(pal)
      if os.path.exists(ffvals):
         warn("ffvals file ", ffvals, "exists already - overwriting it!")
         os.unlink(ffvals)

      warned = False
      for ff in glob.iglob("{0}ff.*".format(pfx)):
         if not warned:
            warn("Generating a new Nbest list makes all previous ff files "
                 "irrelevant - deleting them.")
            warned = True
         os.unlink(ff)

      # Check the validity of the canoe config file.
      verbose("Checking canoe config")
      cmd = ["configtool", "check", cmd_args.canoe_config]
      run_command(cmd, "config file check")

      # Run canoe to produce the Nbest lists, pal, and ffval files.
      # Note: We need a filename prefix for -nbest for canoe-parallel.sh to
      # work correctly because `basename dir/` is "dir", so we use 'nb.'
      cmd = ["set", "-o", "pipefail", ";"]
      cmd.append("canoe-parallel.sh")
      if cmd_args.debug: cmd.append("-d")
      cmd.append("-lb-by-sent")
      if cmd_args.cp_n:
         cmd.extend(("-n", str(cmd_args.cp_n)))
      cmd.extend(("-psub", "-{0}".format(cmd_args.cp_ncpus)))
      cmd.extend(shlex.split(' '.join(cmd_args.cp_opts).encode('utf8')))
      cmd.extend(("canoe", "-append", "-v", str(cmd_args.verbose),
                  "-f", cmd_args.canoe_config,
                  "-nbest", "{0}nb..gz:{1}".format(pfx, cmd_args.nbsize),
                  "-ffvals", "-palign", 
                  "<", cmd_args.msrc))
      cmd.extend(("|", "nbest2rescore.pl", "-canoe", ">", "{0}1best".format(pfx)))
      run_command(cmd, "decoder", shell=True)
      
      nbest = "{0}{1}.gz".format(pfx, nb_str)
      # Rename nb.nbest.gz file to e.g. 1000best.gz;
      # also nb.ffvals.gz to ffvals.gz, and nb.pal.gz to pal.gz.
      if os.path.exists("{0}nb.nbest.gz".format(pfx)):
         os.rename("{0}nb.nbest.gz".format(pfx), nbest)
      if os.path.exists("{0}nb.ffvals.gz".format(pfx)):
         os.rename("{0}nb.ffvals.gz".format(pfx), ffvals)
      if os.path.exists("{0}nb.pal.gz".format(pfx)):
         os.rename("{0}nb.pal.gz".format(pfx), pal)

   # Sanity Checks
   if not os.path.exists(nbest):
      fatal_error("Failed to generate nbest file:", nbest)
   if not os.path.exists(ffvals):
      fatal_error("Failed to generate ffvals file:", ffvals)
   if not os.path.exists(pal):
      fatal_error("Failed to generate pal file:", pal)

   # Step 2. Generate feature values for Nbest lists
   verbose("Generating feature files:")

   # -trans mode might have .rat extension, -ffgen might not, -train won't
   # We always want .ff or .ff.rat, never .rat.ff
   model_base, model_ext = os.path.splitext(os.path.basename(cmd_args.model))
   if model_ext != ".rat":
      model_base += model_ext
      model_ext = ""
   model_rat_in = os.path.join(workdir, "{0}.ff{1}".format(model_base, model_ext))
   model_rat_out = "{0}.rat".format(model_rat_in)  # used only by -train

   cmd = ["gen-features-parallel.pl"]
   if cmd_args.debug: cmd.append("-d")
   if cmd_args.verbose: cmd.append("-v")
   if cmd_args.force_overwrite: cmd.append("-F")
   cmd.extend(("-N", str(cmd_args.gfp_n)))
   cmd.extend(("-rpopts", "-psub -{0}".format(cmd_args.gfp_ncpus)))
   cmd.extend(("-o", model_rat_in, "-c", cmd_args.canoe_config, "-a", pal, "-p", pfx))
   if cmd_args.sproxy: cmd.extend(("-s", cmd_args.sproxy))
   cmd.extend(shlex.split(' '.join(cmd_args.gfp_opts).encode('utf8')))
   cmd.extend((cmd_args.model, cmd_args.src, nbest))
   run_command(cmd, "feature generation")

   # Step 3. Train or translate with the rescoring model
   if cmd_args.mode is Mode.train:
      verbose("Training rescoring model:")
      alg_args = get_algorithm_args(cmd_args.algorithm.encode('utf8'))
      if alg_args.algorithm == 'mira':
         mira_data = "{0}matrix4mira".format(pfx)
         cmd = ["rescore_translate", "-p", pfx, "-dump-for-mira", mira_data,
                model_rat_in, cmd_args.src, nbest]
         run_command(cmd, "training", time_it=True)
         seed = str(cmd_args.seed * 10000 + 1)
         cmd = ["java", "-Xmx16000m", "-enableassertions", 
                "-jar", os.path.join(prog_dir(), "structpred.jar"),
                "MiraTrainNbestDecay", model_rat_in, mira_data+".allffvals.gz",
                mira_data+".allbleus.gz", mira_data+".allnbest.gz", 
                ','.join(cmd_args.refs), alg_args.C, str(alg_args.I),
                str(alg_args.E), str(alg_args.B), alg_args.H, alg_args.O,
                alg_args.D, str(cmd_args.bleu_order), seed, 
                ">", model_rat_out]
         run_command(cmd, "training", time_it=True, shell=True)
      else:
         cmd = ["rescore_train"]
         if cmd_args.verbose: cmd.append("-v")
         cmd.extend(("-n", "-bleu", "-p", pfx))
         cmd.extend(alg_args.rtrain_opts)
         cmd.extend((model_rat_in, model_rat_out, cmd_args.src, nbest))
         cmd.extend(cmd_args.refs)
         run_command(cmd, "training", time_it=True)

   elif cmd_args.mode is Mode.trans:
      verbose("Translating with rescoring model:")
      # We want the results outside the working dir
      cmd = ["rescore_translate"]
      if cmd_args.verbose: cmd.append("-v")
      cmd.extend(("-p", pfx))
      cmd.extend(shlex.split(' '.join(cmd_args.rtrans_opts).encode('utf8')))
      cmd.extend([model_rat_in, cmd_args.src, nbest])
      cmd.extend([">", "{0}rat".format(orig_pfx)])
      run_command(cmd, "translate", time_it=True, shell=True)

   # Transform the trained rescore-model from RAT syntax to normal syntax
   if cmd_args.mode is Mode.train:
      verbose("Transforming trained rescore-model to normal syntax:")
      convert_rescore_model(cmd_args.model, model_rat_out, cmd_args.model_out)

   # Evaluate the translation results, if references provided for -trans mode
   elif cmd_args.mode is Mode.trans:
      if len(cmd_args.refs) > 0:
         info("Evaluation of 1-best output from canoe:")
         cmd = ["bleumain", "-c", "-y", str(cmd_args.bleu_order), "{0}1best".format(pfx)]
         cmd.extend(cmd_args.refs)
         run_command(cmd, "canoe output evaluation")
   
         info("Evaluation of rescoring output:")
         cmd = ["bleumain", "-c", "-y", str(cmd_args.bleu_order), "{0}rat".format(orig_pfx)]
         cmd.extend(cmd_args.refs)
         run_command(cmd, "rescored output evaluation")

   verbose("rescore.py completed on", datetime.today(), "\n")
   print_master_wall_time()

if __name__ == '__main__':
   main()
