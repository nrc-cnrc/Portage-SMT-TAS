#!/usr/bin/env python

# @file tune.py
# @brief Generic MT tuning loop.
# 
# @author George Foster
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada

import sys
import os
import shutil
import errno
import gzip
import time
import re
from subprocess import call, check_output, CalledProcessError
from subprocess import Popen, PIPE, STDOUT
from optparse import OptionParser


usage="tune.py [options] src ref1 [ref2...] [ref1_al ref2_al...]"
help="""
Generic SMT tuning loop. Call with a source text and one or more reference
translations.
"""

# workaround for Java writing locale-sensitive numbers by default
os.environ['LC_ALL'] = 'C'

# constants

jav = "java"
jar=os.path.abspath(os.path.dirname(sys.argv[0]))+"/cherrycSMT.jar"

logdir = "logs"

decoder_log = logdir + "/log.decode"
eval_log = logdir + "/log.eval"
aggr_log = logdir + "/log.aggregate"
optimizer_log = logdir + "/log.optimize"
all_logs = (decoder_log, eval_log, aggr_log, optimizer_log)

history = "summary"
history_wts = "summary.wts"

# arguments

def maxmem():
   """This function should automatically calculate the maximum available memory."""
   return "16000"

parser = OptionParser(usage=usage, description=help)
parser.add_option("--postLattice", dest="postProcessorLattice", type="string", default=None,
                  help="Apply post processing to the lattice before optimizing [%default]")
parser.add_option("--post", dest="postProcessor", type="string", default=None,
                  help="Apply post processing the nbest lists before optimizing [%default]")
parser.add_option("--workdir", dest="workdir", type="string", default="foos",
                  help="Change the working directory [%default]")
parser.add_option("--clean", dest="clean", action="store_true", default=False,
                  help="Remove the working directory after successful completion [%default]")
parser.add_option("--debug", dest="debug", action="store_true", default=False,
                  help="write debug output to stderr [%default]")
parser.add_option("-v", dest="verbose", action="store_true", default=False,
                  help="write verbose output to stderr [%default]")
#parser.add_option("-r", dest="sparse", action="store_true", default=False,
#                  help="use sparse feature representation for nbest hyps; " + \
#                  "if SparseModel(s) are included, tune component weights [%default]")
parser.add_option("-f", dest="config", type="string", default="canoe.ini",
                  help="initial decoder configuration file, including weights [%default]")
parser.add_option("-o", dest="configout", type="string", default="canoe.tune",
                  help="output decoder configuration file [%default]")
parser.add_option("-n", dest="nbsize", type="int", default=100,
                  help="nbest list size [%default]")
parser.add_option("-p", dest="numpar", type="int", default=30,
                  help="number of parallel decoding jobs [%default]")
parser.add_option("-c", dest="numcpus", type="int", default=1,
                  help="number of cpus per decoding job [%default]")
parser.add_option("-j", dest="jmem", type="string", default=maxmem(),
                  help="java memory - depends on 'cpus' for tune.py job (eg 16000 for 4) [%default]")
parser.add_option("-d", dest="decodeopts", type="string", default="",
                  help="general decoding options [%default]")
parser.add_option("-a", dest="optcmd", type="string", default="powell",
                  help="optimizer algorithm and argument string, one of: " + \
                  "powell [switches], " \
                  "mira [C [I [E]]]], " \
                  "pro [alg [curwt [bleucol [orig [reg]]]]], " \
                  "svm [C [B [A]]], " \
                  "lmira [C decay bg density num_it], " \
                  "expsb [L] "\
                  "[%default]")
#parser.add_option("-b", dest="bestbleu", type="string", default="1",
#                  help="type of nbest BLEU calculation for mira: " + \
#                  "1 - smoothed sentence-level, 1x - same as 1, but scaled by " + \
#                  "source-sent len, 2 - oracle substitution, -1 internal BLEU [%default]")
parser.add_option("-m", dest="maxiters", type="int", default=15,
                  help="maximum number of iterations (decoder calls) [%default]")
parser.add_option("-l", dest="lastiter", action="store_true", default=False,
                  help="choose final weights from last iter rather than best [%default]")
parser.add_option("-s", dest="seed", type="int", default=0,
                  help="start seed for random number generator [%default]")
parser.add_option("--no_ag", dest="no_ag", action="store_true", default=False,
                  help="turn off n-best aggregation [%default]")
parser.add_option("--density", dest="density", type="float", default=-1,
                  help="density prune lattices in canoe (-1 for no pruning) [%default]")
parser.add_option("--bleuOrder", dest="bleuOrder", type="int", default=4,
                  help="(l)mira optimizes BLEU using this order of ngrams [%default]")
(opts, args) = parser.parse_args()

if len(args) < 2:
    parser.error("Expecting at least two arguments.")

workdir = opts.workdir
decoder_1best = workdir + "/out"
allnb = workdir + "/allnbests.gz"  # cumulative nbest lists
allbleus = workdir + "/allbleus.gz"
allnb_new = workdir + "/allnbests-new.gz"
nbpattern = workdir + "/nbest.%04d.%dbest.gz"

powellwts = workdir + "/powellwts."   # iter-specific wt record
optimizer_in = workdir + "/curmodel.ini"  # dummy model in
optimizer_in0 = workdir + "/curmodel0.ini"  # initial dummy model in, with weights
optimizer_out = workdir + "/curmodel.out"  # best model out
hypmem = workdir + "/hypmem.txt" # lmira fake lattice aggregation

src = args[0]
refs = args[1:]

alg = opts.optcmd.split()[0]
if opts.bleuOrder!=4 and not(alg!="mira" or alg!="lmira"):
   parser.error("bleuOrder only works with mira and lmira")

# allff is the aggregated feature-value file output from canoe, in sparse or
# dense format 
opts.sparse = False
if opts.sparse:
    allff = workdir + "/allsfvals.gz"
    parser.error("sparse features are not supported")
else:
    allff = workdir + "/allffvals.gz"

opts.bestbleu = "1"
if opts.bestbleu not in ("1", "1x", "2", "-1"):
    parser.error("bad value for -b switch: " + opts.bestbleu)

if alg not in ("powell", "mira", "pro", "svm", "lmira", "expsb"):
    parser.error("unknown optimization algorithm: " + alg)

if not os.path.isfile(src):
    parser.error("source file " + src + " doesn't exist")
for f in refs:
    if not os.path.isfile(f):
        parser.error("reference file " + f + " doesn't exist")
if not os.path.isfile(opts.config):
    parser.error("decoder config file " + opts.config + " doesn't exist")

if not opts.jmem.startswith("-"):
    opts.jmem = "-Xmx" + opts.jmem + "m"

# utilities

def print_timing(func):
    def wrapper(*arg):
        t1 = time.time()
        res = func(*arg)
        t2 = time.time()
        print >> sys.stderr, '%s took %0.3f s' % (func.func_name, (t2-t1))
        return res
    return wrapper

def warn(str):
   print >> sys.stderr, "Warning: ", str

def error(str, *args):
   print >> sys.stderr, "Error: ", str, args
   sys.exit(1)

def log(str, *files):
   """Write a line to a log file(s)."""
   for file in files:
      f = open(file, "a")
      print >> f, str
      f.close()

def init(*files):
   """Make files exist but be empty."""
   for file in files:
      f = open(file, "w")
      f.close()

def run(cmd):
   """Run a system command, and fail with an error if it fails."""
   if os.system(cmd) != 0:
      error("command failed: " + cmd)

def getOptimizedScore(find_what):
   """Extract from the optimize log the last iteration's best score.
   Must be from the jar's output.
   """
   with open(optimizer_log) as f:
      score = [ m.group(1) for l in list(f) for m in (find_what(l),) if m]
   if not len(score):
      error("Can't find any score in the optimize log file.")
   return score[-1]

class NBReader:
    """Manage lookahead for cumulative nbest/ffvals files, for aggregation."""
    def __init__(self, nbfile, fffile):
       self.nbfile = nbfile
       self.fffile = fffile
       self.hyp, self.val, self.pal = "","",""
    def get_next(self, index):
       if self.hyp != "":
           hyp, val, pal = self.hyp, self.val, self.pal
           self.hyp, self.val, self.pal = "","",""
           return (hyp, (val, pal))
       (s1, tab, hyp) = self.nbfile.readline().partition('\t')
       (s2, tab, val) = self.fffile.readline().partition('\t')
       if s1 != s2:
           error("inconsistent nbest/ffvals files")

       pal = ""
       if s1 != str(index):   # includes empty-file case
           self.hyp = hyp
           self.val = val
           self.pal = pal
           return None
       else:
           return (hyp, (val, pal))

@print_timing
def aggregate_nbests(ns):
    """Add novel contents (only) of nbest/ffvals files to cumulative lists."""

    if alg == "lmira":
       if opts.postProcessorLattice != None:
          latpattern = workdir + "/lat.%04d.gz"
          for i in range(ns):   # for each source sentence
             try:
                cmd = "zcat {f} | {p} | gzip > {f}.tmp && mv {f}.tmp {f}".format(f=latpattern%i, p=opts.postProcessorLattice)
                if opts.debug:
                   print >> sys.stderr, "lattice: ", latpattern%i
                   print >> sys.stderr, "cmd: ", cmd
                if call(cmd, shell=True) is not 0:
                   error("Problems while post processing the nbest lists with {}".format(' '.join(cmd)))
             except CalledProcessError, err:
                error("Problem running configtool", err)
             except OSError, err:
                error("configtool program not found ", err)
       return -1
    if alg == "lmira" or alg == "olmira":
        return -1

    logfile = open(aggr_log, 'a')

    if opts.sparse:
        allff_new = workdir + "/allsfvals-new.gz"
        ffpattern = workdir + "/nbest.%04d.%dbest.sfvals.gz"
    else:
        allff_new = workdir + "/allffvals-new.gz"
        ffpattern = workdir + "/nbest.%04d.%dbest.ffvals.gz"

    if opts.no_ag:
        open(allnb, 'w').close()
        open(allff, 'w').close()

    allnbfile = gzip.open(allnb)
    allfffile = gzip.open(allff)
    allnbfile_new = gzip.open(allnb_new, 'w')
    allfffile_new = gzip.open(allff_new, 'w')
    nbr = NBReader(allnbfile, allfffile)
    num_cur_hyps = 0
    num_new_hyps = 0

    if opts.postProcessor != None:
       try:
          nbpattern = workdir + "/post%04d"
          cmd = "zcat {w}/nbest.*best.gz | {p} | split -d -a4 -l {n} - {w}/post".format(w=workdir, p=opts.postProcessor, n=opts.nbsize)
          if opts.debug:
             print >> sys.stderr, "nbpattern: ", nbpattern
             print >> sys.stderr, "cmd: ", cmd
          if call(cmd, shell=True) is not 0:
             error("Problems while post processing the nbest lists with {}".format(' '.join(cmd)))
       except CalledProcessError, err:
          error("Problem running configtool", err)
       except OSError, err:
          error("configtool program not found ", err)
    else:
       nbpattern = workdir + "/nbest.%04d.%dbest.gz"

    for i in range(ns):   # for each source sentence

        print >> logfile, "sent ", i+1, ":",

        # record new hyps for this sent in dict: hyp -> [vals1, vals2, ..]

        if opts.postProcessor != None:
           nbname = nbpattern % (i)
           nbfile = open(nbname)
        else:
           nbname = nbpattern % (i, opts.nbsize)
           nbfile = gzip.open(nbname)
        if opts.debug:
           print >> sys.stderr, "Aggregating ", nbfile
        ffname = ffpattern % (i, opts.nbsize)
        fffile = gzip.open(ffname)

        nbset = {}
        for hyp in nbfile:
           vals = fffile.readline()
           pals = ""
           if (hyp == "\n"):  # lists are padded with blank lines
               break
           nbset.setdefault(hyp, []).append((vals, pals))
        nbfile.close()
        fffile.close()
        os.remove(nbname) # clean up; we're done with these files
        os.remove(ffname)

        # read/write existing hyps, removing them from nbset if they're there

        n = 0
        while True:
            hypval = nbr.get_next(i)
            if hypval == None:
                break
            n += 1
            print >> allnbfile_new, str(i) + "\t" + hypval[0],
            print >> allfffile_new, str(i) + "\t" + hypval[1][0],
            val_list = nbset.get(hypval[0])
            if val_list != None:
                try: val_list.remove(hypval[1])
                except ValueError: pass
                if len(val_list) == 0:
                    del nbset[hypval[0]]
        print >> logfile, n, "existing hyps +",

        # write remaining new hyps in nbset

        nn = 0
        for hyp,vals_pals in nbset.iteritems():
            for vp in vals_pals:
                nn += 1
                val = vp[0]
                print >> allnbfile_new, str(i) + "\t" + hyp,
                print >> allfffile_new, str(i) + "\t" + val,
        print >> logfile, nn, "novel hyps =", n+nn, "total"
        num_cur_hyps += n
        num_new_hyps += nn

    print >> logfile, "total size: ", num_cur_hyps, "existing hyps + ", \
             num_new_hyps, "novel hyps = ", num_cur_hyps + num_new_hyps

    logfile.close()
    allnbfile.close()
    allfffile.close()
    allnbfile_new.close()
    allfffile_new.close()
    os.rename(allnb_new, allnb)
    os.rename(allff_new, allff)

    return num_new_hyps


def normalize(wts, norm_wts):
    assert len(wts) == len(norm_wts)
    mx = max(abs(min(wts)), abs(max(wts)))
    if mx == 0: mx = 1.0
    for i in range(len(wts)):
        norm_wts[i] = wts[i] / mx

def avg_diff(wts1, wts2):
    ll = len(wts1) if len(wts1) else 1
    return sum(map(abs, [a[0]-a[1] for a in zip(wts1, wts2)])) / float(ll)

def sfvals2ffvals(num_features):
    """Convert allsfvals.gz file to allffvals.gz"""
    assert opts.sparse
    fi = gzip.open(workdir + "/allsfvals.gz", 'r')
    fo = gzip.open(workdir + "/allffvals.gz", 'w')
    for line in fi:
        (ind, tab, ffs) = line.partition('\t')
        newvals = ['0'] * len(wts)
        for tok in ffs.split():
            (f,v) = tok.split(':');
            newvals[int(f)] = v
        print >> fo, ind + '\t' + '\t'.join(newvals)
    fi.close()
    fo.close()

# wrapper functions

def decoderConfig2wts(config):
    """Convert a decoder config file into a weight vector."""
    if opts.sparse: configtool_cmd = "get-all-wts:x" 
    else: configtool_cmd = "rescore-model:x" 
    try:
       toks = check_output(["configtool", configtool_cmd, config]).split()
       return [float(toks[i]) for i in range(1, len(toks), 2)]
    except CalledProcessError, err:
       error("Problem running configtool", err)
    except OSError, err:
       error("configtool program not found ", err)

def wts2decoderConfigStr(config, input_config = opts.config):
    """Helper for wts2decoderConfi"""
    if opts.sparse: configtool_cmd = "set-all-wts:-"
    else: configtool_cmd = "set-weights-rm:-"
    return ["configtool", configtool_cmd, input_config, config]

def wts2decoderConfig(wts, config):
    """Convert a weight vector into a decoder config file."""
    s = ""
    for w in wts:
        s += "x " + str(w) + "\n"
    #if opts.debug:
    #    print >> sys.stderr, ' '.join(wts2decoderConfigStr(config))
    #    print >> sys.stderr, "with input: ", s
    Popen(wts2decoderConfigStr(config), stdin=PIPE).communicate(s)
    if opts.debug:
       call(["cat", config])

def shardAnnotate(s, iter, shard) :
    """Append iteration and shard number to str"""
    return s+".i"+str(iter).zfill(2) +".s"+str(shard).zfill(2)

@print_timing    
def decode(wts):
    """Decode current source file using given weight vector."""
    wts2decoderConfig(wts, "decode-config")
    cmd = ["canoe-parallel.sh", "-cleanup", "-psub", "-" + str(opts.numcpus), \
           "-n", str(opts.numpar), "canoe", "-v", "1", "-f", "decode-config"]
    outcmd = ["nbest2rescore.pl", "-canoe"]
    if opts.sparse: 
       cmd.append("-sfvals")
    else: 
       cmd.append("-ffvals")
    if alg == "lmira":
       cmd.extend(["-palign", "-lattice", workdir+"/lat.gz"])
       if opts.density > 0 :
          cmd.extend(["-lattice-output-options", "overlay", "-lattice-density", str(opts.density)])
    elif alg == "olmira" :
        cmd.extend([""]) # intentionally blank, only need sentences
    else:
        cmd.extend(["-nbest", workdir+"/nbest.gz:"+str(opts.nbsize)])
    if opts.decodeopts != "":  # (may need to do more than split for general case)
        cmd.extend(opts.decodeopts.split())
    logfile = open(decoder_log, 'a')
    srcfile = open(src)
    outfile = open(decoder_1best, 'w')
    if opts.debug: print >> sys.stderr, ' '.join(cmd)
    if opts.debug: print >> sys.stderr, ' '.join(outcmd)
    cmd.append("|")
    cmd.extend(outcmd)
    if call(' '.join(cmd), stdin=srcfile, stdout=outfile, stderr=logfile, shell=True, close_fds=True) is not 0:
        error("decoder failed: {}".format(' '.join(cmd)))

@print_timing
def eval():
   """Evaluate decoder 1best and nbest output. Return 1best."""

   try:
      score_string="BLEU score"
      cmd = ["bleumain", "-y", str(opts.bleuOrder), decoder_1best] + refs

      if opts.debug: print >> sys.stderr, ' '.join(cmd)

      s = check_output(cmd, stderr=STDOUT).split("\n")
      logfile = open(eval_log, 'a')
      score = None
      for line in s:
          print >> logfile, line
          if line.startswith(score_string):
              score = line.split()[2]

      if alg not in ["powell", "lmira", "olmira"]:
          cmd = ["bestbleu", "-y", str(opts.bleuOrder), "-dyn", "-o", "nbest", allnb] + refs
          # if opts.bestbleu == "1": cmd[1:1] = ["-s", "2"]
          # if opts.bestbleu == "1x": cmd[1:1] = ["-x"]

          allbleus_file = gzip.open(allbleus, 'w')
          if opts.debug: print >> sys.stderr, ' '.join(cmd)
          for line in check_output(cmd, stderr=logfile).split("\n"):
              if line != "":
                  (s, bleu1, bleu2) = line.split(" ")
                  print >> allbleus_file, s, bleu1 if opts.bestbleu.startswith("1") else bleu2
          allbleus_file.close()

      logfile.close()
      return score
   except CalledProcessError, err:
      error("Problem running ", ' '.join(cmd), err)
   except OSError, err:
      error("Command not found ", ' '.join(cmd), err)


def optimizerModel2wts(model):
    """Convert an optimizer model file into a weight vector."""
    wts = []
    with open(model) as f:
        for line in f:
            wts.append(float(line.partition(' ')[2]))
    return wts

def olmiraModel2wts(model):
    """Convert a model file (any whitespace) into a weight vector."""
    wts = []
    with open(model) as f:
        for line in f:
            wts.append(float(line.split()[1]))
    return wts

@print_timing
def optimize(iter, wts):
    """Run the selected optimizer (according to opts.optcmd).
    iter -- current iteration in main loop
    wts -- weights from previous iteration; set to new wts on return
    return -- optimizer score
    """
    (alg, sep, args) = opts.optcmd.partition(' ')
    logfile = open(optimizer_log, 'a')
    if iter == 0 and alg != "powell":
       shutil.copyfile(optimizer_in0, optimizer_in)

    if alg == "powell":
        ret = optimizePowell(iter, wts, args, logfile)
    elif alg == "mira":
        ret = optimizeMIRA(iter, wts, args, logfile)
    elif alg == "pro":
        ret = optimizePRO(iter, wts, args, logfile)
    elif alg == "expsb":
        ret = optimizeExpSentBleu(iter, wts, args, logfile)
    elif alg == "svm":
        ret = optimizeSVM(iter, wts, args, logfile)
    elif alg == "lmira":
        ret = optimizeLMIRA(iter, wts, args, logfile)
    elif alg == "olmira":
        ret = optimizeOnlineLMIRA(iter, wts, args, logfile)
    else:
        assert 0   # has already been checked
    logfile.close()

    if alg != "powell":
       shutil.copyfile(optimizer_out, optimizer_in)

    return ret


def optimizePowell(iter, wts, args, logfile):
    """Optimize weights using Powell's over current aggregate nbest lists."""
    seed = str(opts.seed * 10000 + iter)
    wo_file = powellwts + str(iter+1)
    if opts.sparse: sfvals2ffvals(len(wts))
    cmd = ["time-mem", "rescore_train", "-n", "-r", "15", "-dyn", "-win", "5", "-s", seed, \
           "-wi", powellwts + str(iter), "-wo", wo_file] + args.split() + \
           [optimizer_in, optimizer_out, src, allnb] + refs
    print >> logfile, ' '.join(cmd)
    logfile.flush()
    if call(cmd, stdout=logfile, stderr=STDOUT) is not 0:
        error("optimizer failed with cmd: {}".format(' '.join(cmd)))
    wts[:] = optimizerModel2wts(optimizer_out)
    with open(wo_file) as f:
        s = float(f.readline().split()[2])
    return s

def optimizeMIRA(iter, wts, args, logfile):
    """Optimize weights using MIRA over current aggregate nbest lists."""
    C = "1e-02"  # learning rate; 1e-4 recommended fomr B=1, 1e-8 for B=2
    I = "30"     # number of iterations
    E = "1"      # number of neg examples
    B = "-4"     # if >0, col to find BLEU in allbleus file
    H = "true"   # hope update?
    O = "Oracle" # use Model, Oracle or Orange as background
    D = "0.999"  # Rate at which to decay Model or Oracle BG
    seed = "1"   # Random seed
    if(opts.seed>0): seed = str(opts.seed * 10000 + iter)
    args_vals = args.split()
    if len(args_vals) > 0: C = args_vals[0]
    if len(args_vals) > 1: I = args_vals[1]
    if len(args_vals) > 2: E = args_vals[2]
    if len(args_vals) > 3: B = args_vals[3]
    if len(args_vals) > 4: H = args_vals[4]
    if len(args_vals) > 5: O = args_vals[5]
    if len(args_vals) > 6: D = args_vals[6]
    if len(args_vals) > 7:
       print >> logfile, "warning: ignoring values past first 3 tokens in " + args
    refglob = ','.join(refs)
    cmd = ["time-mem", jav, opts.jmem, "-enableassertions", "-jar", jar, "MiraTrainNbestDecay", optimizer_in, \
           allff, allbleus, allnb, refglob, C, I, E, B, H, O, D, str(opts.bleuOrder), seed]
    outfile = open(optimizer_out, 'w')
    print >> logfile, ' '.join(cmd)
    logfile.flush()
    if call(cmd, stdout=outfile, stderr=logfile) is not 0:
        error("optimizer failed with cmd: {}".format(' '.join(cmd)))
    outfile.close()
    normalize(optimizerModel2wts(optimizer_out), wts)
    return getOptimizedScore(re.compile('Best BLEU found on it# \d+, score ([\d\.]+)').search)

def optimizeSVM(iter, wts, args, logfile):
    """Optimize weights as multiclass SVM training over current aggregate nbest lists."""
    C = "1e-03" # C-parameter, turn up for lower losses, less regularization
    B = "-1"    # if >0, col to find BLEU in allbleus file; if < 0, internal metric
    A = "cut"   # values are [cut=cutting plan] and [full=full multiclass on n-best list]
    args_vals = args.split();
    if len(args_vals) > 0 : C = args_vals[0]
    if len(args_vals) > 1 : B = args_vals[1]
    if len(args_vals) > 2 : A = args_vals[2]
    if len(args_vals) > 3 :
        print >> logfile, "warning: ignoring values past first 3 tokens in " + args
    refglob = ','.join(refs)
    cmd = ["time-mem", jav, opts.jmem, "-enableassertions", "-jar", jar, "SvmTrainNbest", optimizer_in, \
           allff, allbleus, allnb, refglob, C, B, A]
    outfile = open(optimizer_out, 'w')
    print >> logfile, ' '.join(cmd)
    logfile.flush()
    if call(cmd, stdout=outfile, stderr=logfile) is not 0:
        error("optimizer failed: {}".format(' '.join(cmd)))
    outfile.close()
    normalize(optimizerModel2wts(optimizer_out), wts)
    return getOptimizedScore(re.compile('Best obj found on it# \d+, score it   \d+ : BLEU = ([\d\.]+)').search)

def optimizeExpSentBleu(iter, wts, args, logfile):
    """Optimize weights according to expected sum of Oranges"""
    L = "50"
    BFGS = "false"
    args_vals = args.split();
    if len(args_vals) > 0 : L = args_vals[0]
    if len(args_vals) > 1 : BFGS = args_vals[1]
    if len(args_vals) > 2 : 
        print >> logfile, "warning, ignoring values past first token in " + args
    refglob = ",".join(refs)
    cmd = ["time-mem", jav, opts.jmem, "-enableassertions", "-jar", jar, "ExpLinGainNbest", optimizer_in, \
           allff, allbleus, allnb, refglob, L, BFGS]
    outfile = open(optimizer_out, 'w')
    print >> logfile, ' '.join(cmd)
    logfile.flush()
    if call(cmd, stdout=outfile, stderr=logfile) is not 0:
        error("optimizer failed: {}".format(' '.join(cmd)))
    outfile.close()
    normalize(optimizerModel2wts(optimizer_out), wts)
    return getOptimizedScore(re.compile('([\d\.]+$)').search)

def optimizePRO(iter, wts, args, logfile):
    """Optimize weights using PRO over current aggregate nbest lists."""
    alg = "MaxentZero" # values are SVM, MIRA, Pagasos (=SVM), MegaM (not on cluster), 
                       # Maxent (regularizes to initializer), MaxentZero (regularizes to 0)
    curwt = "0.1"  # interp coeff on current wts vs prev
    b = "-2"       # if 1, column if bestbleus file; if < 0, internal metric
    o = "false"    # switch to true to switch off multiple samples
    r = "1e-4"     # regularization parameter, C for SVM, Pegasos, Maxent*; ignored for others
    args_vals = args.split()
    if len(args_vals) > 0: alg = args_vals[0]
    if len(args_vals) > 1: curwt = args_vals[1]
    if len(args_vals) > 2: b = args_vals[2]
    if len(args_vals) > 3: o = args_vals[3]
    if len(args_vals) > 4: r = args_vals[4]
    if len(args_vals) > 5:
        print >> logfile, "warning: ignoring values past fourth token in " + args
    refglob = ','.join(refs)
    cmd = ["time-mem", jav, opts.jmem, "-enableassertions", "-jar", jar, "ProTrainNbest", optimizer_in, \
           allff, allbleus, allnb, refglob, alg, o, curwt, \
           b, r, str(iter), optimizer_out]
    print >> logfile, ' '.join(cmd)
    logfile.flush()
    if call(cmd, stdout=logfile, stderr=logfile) is not 0:
        error("optimizer failed: {}".format(' '.join(cmd)))
    normalize(optimizerModel2wts(optimizer_out), wts)
    return getOptimizedScore(re.compile('Best BLEU found \(samp=\d+\) : ([\d\.]+)').search)

def optimizeLMIRA(iter, wts, args, logfile):
    """Optimize weights using lattice MIRA over current lattices."""
    C = "0.01"                          # learning rate
    decay = "0.999"                     # effective number of context sentences
    bg = "Oracle"                # BLEU background=Oracle|Model|Orange
    density = "50"                   # lattices will be f-b pruned to this density
    numIt = "30"                     # max number of iterations
    seed = "1"
    if(opts.seed>0): seed = str(opts.seed * 10000 + iter)
    args_vals = args.split()
    if len(args_vals) > 0: C = args_vals[0]
    if len(args_vals) > 1: decay = args_vals[1]
    if len(args_vals) > 2: bg = args_vals[2]
    if len(args_vals) > 3: density = args_vals[3]
    if len(args_vals) > 4: numIt = args_vals[4]
    if len(args_vals) > 5:
       print >> logfile, "warning: ignoring values past first 4 tokens in " + args
    refglob = ','.join(refs)
    cmd = ["time-mem", jav, opts.jmem, "-enableassertions", "-jar", jar, "MiraTrainLattice", optimizer_in, \
           workdir, refglob, src, hypmem, C, decay, bg, density, numIt, str(opts.bleuOrder), seed]
    outfile = open(optimizer_out, 'w')
    print >> logfile, ' '.join(cmd)
    logfile.flush()
    if call(cmd, stdout=outfile, stderr=logfile) is not 0:
        error("optimizer failed: {}".format(' '.join(cmd)))
    outfile.close()
    normalize(optimizerModel2wts(optimizer_out), wts)
    return getOptimizedScore(re.compile('Best BLEU found on it# \d+, score ([\d\.]+)').search)

def optimizeOnlineLMIRA(iter, wts, args, logfile):
    """Optimize weights using lattice MIRA over lattices generated online."""
    C = "0.01"                          # learning rate
    decay = "0.999"                     # effective number of context sentences
    bg = "Oracle"                   # BLEU background=Oracle|Model|Orange
    density = "1000"                    # density to which lattices will be pruned
    combineCounts = False             # should we combine BLEU counts at the end of each epoch?
    args_vals = args.split()
    if len(args_vals) > 0: C = args_vals[0]
    if len(args_vals) > 1: decay = args_vals[1]
    if len(args_vals) > 2: bg = args_vals[2]
    if len(args_vals) > 3: density = args_vals[3]
    if len(args_vals) > 4: combineCounts = args_vals[4] in ['true', 'True', 'TRUE']
    if len(args_vals) > 5:
       print >> logfile, "warning: ignoring values past first 3 tokens in " + args
    dec_cfg = "decode-config"
    miraConfig = [shardAnnotate(workdir+"/mira-config","xx",shard) for shard in range(opts.numpar)]
    #Create src filenames and refglobs for each shard
    refPrefixes = [(workdir+"/ref."+str(i)) for i in range(len(refs))]
    srcShards = list(shardAnnotate(workdir+"/src","xx",i) for i in range(opts.numpar))
    shardRefList = [[shardAnnotate(pref,"xx",i) for pref in refPrefixes] for i in range(opts.numpar)]
    shardRefglob = [",".join(shardRef) for shardRef in shardRefList]
    logfile.flush()
    #Shard corpus on first iteration, and set up shard-specific initial config files
    if iter==0 :
        #Shard source, save shard names
        cmd = ["time-mem", jav, opts.jmem, "-enableassertions", "-jar", jar, "ShardCorpus", src]
        cmd.extend(srcShards)
        print >> logfile, ' '.join(cmd)
        logfile.flush()
        if call(cmd, stdout=logfile, stderr=logfile) is not 0:
            error("src sharding failed: {}".format(' '.join(cmd)))
        #Shard refs, save ref names
        refShardList = [[shardAnnotate(pref,"xx",i) for i in range(opts.numpar)] for pref in refPrefixes]
        for i in range(len(refs)):
            cmd = ["time-mem", jav, opts.jmem, "-enableassertions", "-jar", jar, "ShardCorpus", refs[i]]
            cmd.extend(refShardList[i])
            print >> logfile, ' '.join(cmd)
            logfile.flush()
            if call(cmd, stdout=logfile, stderr=logfile) is not 0:
                error("ref sharding failed: {}".format(' '.join(cmd)))
        # create shard-specific initial config files
        for i in range(opts.numpar):
           run("configtool rep-sparse-models-local:." + str(i) + " " + dec_cfg + " > " + shardAnnotate(dec_cfg, "ni", i))
    #Write config files for each shard
    model=workdir+"/mira.model"
    avg=workdir+"/mira.avg.model"
    count=workdir+"/mira.count"
    lat = workdir+"/lat"
    portageIniWeights = optimizer_in
    for shard in range(opts.numpar):
        input_decodeConfig = shardAnnotate(dec_cfg, "ni", shard)
        decodeConfig = shardAnnotate(dec_cfg,"xx",shard)
        modelIn = "empty" if iter==0 else shardAnnotate(model,iter-1,"xx") 
        modelOut = shardAnnotate(model,iter,shard)
        bleuCountIn = "empty" if iter==0 else (shardAnnotate(count,iter-1,shard)
                                               if not combineCounts else shardAnnotate(count,iter-1,"xx"))
        bleuCountOut = shardAnnotate(count,iter,shard)
        sparse = "-sfvals" if opts.sparse else "-ffvals"
        decodeCmd = "canoe -f " + decodeConfig + " " + sparse + " -palign -lattice " + shardAnnotate(lat,"xx",shard)+".gz"
        weightCmd = " ".join(wts2decoderConfigStr(decodeConfig, input_decodeConfig))
        latticeTmpFile = shardAnnotate(lat,"xx",shard)+".0000.gz"
        srcFile = srcShards[shard]
        refFiles = shardRefglob[shard]
        with open(miraConfig[shard], 'w') as ofile:
            ofile.write("modelInFile = " + modelIn + "\n")
            ofile.write("modelOutFile = " + modelOut + "\n")
            ofile.write("portageIniWeights = " + portageIniWeights + "\n")
            ofile.write("bleuCountInFile = " + bleuCountIn + "\n")
            ofile.write("bleuCountOutFile = " + bleuCountOut + "\n")
            ofile.write("decodeCmd = " + decodeCmd + "\n")
            ofile.write("weightCmd = " + weightCmd + "\n")
            ofile.write("srcFile = " + srcFile + "\n")
            ofile.write("refFiles = " + refFiles + "\n")
            ofile.write("latticeTmpFile = " + latticeTmpFile + "\n")
            ofile.write("C = " + C + "\n")
            ofile.write("decay = " + decay + "\n")
            ofile.write("background = " + bg + "\n")
            ofile.write("density = " + density + "\n")
    #Run run-parallel
    rpcmds = workdir + "/rpcmds"
    with open(rpcmds,"w") as rpfile :
        for conf in miraConfig :
            rpfile.write(" ".join(["time-mem", jav, opts.jmem, "-enableassertions", "-jar", jar, "MiraLatticeStep", conf, str(iter),"\n"]))
    cmd = ["run-parallel.sh","-nolocal","-psub","-4","-psub", "-memmap 4", rpcmds,str(opts.numpar)]
    print >> logfile, ' '.join(cmd)
    logfile.flush()
    if call(cmd, stdout=logfile, stderr=logfile) is not 0:
        error("run-parallel failed: {}".format(' '.join(cmd)))
    #Merge configs into a combined file for next iteration
    cmd = [jav, opts.jmem, "-enableassertions", "-jar", jar, "CombineModels", shardAnnotate(model,iter,"xx")]
    cmd.extend([shardAnnotate(model,iter,shard) for shard in range(opts.numpar)])
    print >> logfile, ' '.join(cmd)
    logfile.flush()
    if call(cmd, stdout=logfile, stderr=logfile) is not 0:
        error("merge failed: {}".format(' '.join(cmd)))
    #Average combined file into a Portage-consumable framework
    cmd = [jav, opts.jmem, "-enableassertions", "-jar", jar, "AverageModel", portageIniWeights, shardAnnotate(model,iter,"xx"), optimizer_out]
    print >> logfile, ' '.join(cmd)
    logfile.flush()
    if call(cmd, stdout=logfile, stderr=logfile) is not 0:
        error("averaging failed: {}".format(' '.join(cmd)))
    normalize(olmiraModel2wts(optimizer_out), wts)
    #Combine background BLEU counts
    cmd = [jav, opts.jmem, "-enableassertions", "-jar", jar, "CombineBleuCounts", shardAnnotate(count,iter,"xx")]
    cmd.extend([shardAnnotate(count,iter,shard) for shard in range(opts.numpar)])
    print >> logfile, ' '.join(cmd)
    logfile.flush()
    if call(cmd, stdout=logfile, stderr=logfile) is not 0:
        error("bleu combination failed: {}".format(' '.join(cmd)))
    #Use combined background BLEU to get a score estimate
    cmd = [jav, opts.jmem, "-enableassertions", "-jar", jar, "ScoreCountFile", shardAnnotate(count,iter,"xx")]
    print >> logfile, ' '.join(cmd)
    logfile.flush()
    try:
       toks = check_output(cmd, stderr=logfile).split()
    except CalledProcessError, (num, errstr):
       error("ScoreCountFile failed for optimizeOnlineLMIRA", ' '.join(cmd), num, errstr)
    except OSError, err:
       error("Command not found ", ' '.join(cmd), err)
    return float(toks[0])

# initialize

try: os.mkdir(workdir)
except OSError, (num, errstr):
    if num == errno.EEXIST:
       warn("work directory already exists - will overwrite contents")
    else: raise
try: os.mkdir(logdir)
except OSError, (num, errstr):
    if num == errno.EEXIST: pass
    else: raise

wts = decoderConfig2wts(opts.config)
with open(optimizer_in, 'w') as f:
    for i in range(len(wts)):
        print >> f, "FileFF:" + workdir + "/allffvals.gz," + str(i+1)
with open(optimizer_in0, 'w') as f:
    for i in range(len(wts)):
        print >> f, "FileFF:" + workdir + "/allffvals.gz," + str(i+1) + " " + str(wts[i])
# Create the files that we need.
init(allnb, allff, powellwts + "0", history, history_wts, *all_logs)
num_srclines = sum(1 for line in open(src))

best_iter = 0
best_score = -1.0
best_wts = []
old_wts = []
opt_score = 0
aggr_nbest_size = 0

# main loop

for i in range(opts.maxiters):

    log("starting loop " + str(i+1), *all_logs)
    decode(wts)
    num_new_hyps = aggregate_nbests(num_srclines)
    score = eval()

    info = "decode-score=" + str(score) + \
           " prev-optimizer-score=" + str(opt_score) + \
           " prev-nbest-size=" + str(aggr_nbest_size) + \
           " avg-wt-diff=" + str(avg_diff(wts, old_wts))
    log(info, history)
    log(' '.join(map(str, wts)), history_wts)
    
    if (score > best_score):
        best_iter = i
        best_score = score
        best_wts[:] = wts

    if num_new_hyps == 0:
        print "Stopping - no new hypotheses found on iteration", i+1
        break
    elif i+1 == opts.maxiters:
        print "Maximum iterations (" + str(opts.maxiters) +") reached."
        break

    aggr_nbest_size += max(num_new_hyps,0)
    old_wts[:] = wts
    opt_score = optimize(i, wts)

# wrap up

print "Best score (from iter " + str(best_iter+1) + ") =", best_score
print "Best weights:", ' '.join([str(w) for w in best_wts])
if opts.lastiter:
    print "Using last-iteration weights for " + opts.configout
    best_wts = wts
wts2decoderConfig(best_wts, opts.configout)

if opts.clean and os.path.exists(workdir):
   shutil.rmtree(workdir)
