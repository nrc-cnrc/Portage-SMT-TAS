#!/usr/bin/env perl
#
# @file gen-features-parallel.pl 
# @brief Generate a set of feature-value files in parallel, as required by the
# rescoring model file MODEL.
#
# George Foster / Samuel Larkin
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

use strict;
use warnings;
use File::Temp;

BEGIN {
   # If this script is run from within src/ rather than being properly
   # installed, we need to add utils/ to the Perl library include path (@INC).
   if ( $0 !~ m#/bin/[^/]*$# ) {
      my $bin_path = $0;
      $bin_path =~ s#/[^/]*$##;
      unshift @INC, "$bin_path/../utils";
   }
}
use portage_utils;
printCopyright("gen-features-parallel.pl", 2008);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


my $SRC_TAG = "<src>";
my $FFVAL_WTS_TAG = "<ffval-wts>";
my $NBEST_TAG = "<nbest>";
my $PFX_TAG = "<pfx>";
my $NUM_PROC_TAG = "<NP>";

my @ARG_ORIGINAL = @ARGV;

my ($DEBUG, $VERBOSE, $NOEXEC, $FORCE_OVERWRITE);
my ($MODEL, $SFILE, $NBEST);
my $WORDFEAT = "";
my $SPROXY = undef;
my $PREF = undef;
my $ALIGFILE = undef;
my $CANOEFILE = undef;
my $JOBS_PER_FF = -1;
my $RESCORING_MODEL_OUT = undef;
my $N = 3;

my ($CMDS_FILE_HANDLE, $CMDS_FILE_NAME) = File::Temp::tempfile("gen-features-parallel-output.$$-XXXX", SUFFIX => ".commands", UNLINK=>1);


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
 Usage: gen-features-parallel.pl [-v][-n][-s sproxy][-p pref][-a pal-file]
        [-o RESCORING-MODEL][-N #nodes][-J #jobs_per_ff][-F]
        [-c canoe-file-with-weights] MODEL SFILE NBEST

 Generate a set of feature-value files in parallel, as required by the
 rescoring model file MODEL. For each line in MODEL that is in form
 FileFF:ff.FNAME[.ARGS], gen_feature_values will be run with the feature
 FNAME and arguments ARGS, writing results to the file ff.FNAME.ARGS (see
 example below). SFILE and NBEST are source and nbest files.

 On the cluster, this script will launch one job per feature. Otherwise, it
 generates features sequentially - to get parallel behaviour, just launch
 multiple instances of gen-features-parallel.sh with the same arguments
 (these will not overwrite each other's outputs).
 [This behaviour is broken, but if you use -n and pipe the output to
 run-parallel.sh, that works.  See, e.g., rat.sh.]

 Options:

 -v  Verbose output.
 -n  Don't execute commands, just write them to stdout. This turns off -v.
 -s  Use <sproxy> instead of SFILE when substituting for <src> in MODEL.
 -p  Generate feature files with prefix <pref>. [none]
 -a  Load phrase alignment from file <filename>. [none]
 -c  Get canoe weights from file <filename>. [none]
 -w  Write one feature for each target word instead of one per sentence.
     [sentence]
 -o  Produces a rescore_{train,translate} compatible model to RESCORING-MODEL.
 -N  Number of nodes to run job on. [3]
 -J  Number of jobs per feature function when running in parallel {expert mode}.
     [2*ceil(N/#computable FF)]
 -F  Force feature function files overwrite. [don't]

 For example, if the contents of MODEL are:

   FileFF:NgramFF.sfile
   FileFF:extern
   Consensus
   IBM2TgtGivenSrc:big-model
   SCRIPT:\"./yourscript <nbest>\"

 Then:
 - NgramFF is ignored because it is FileFF thus already created by the user.
 - Consensus causes the Consensus feature to be generated with a dummy
   argument and written to the file <pref>ff.Consensus.gz
 - IBM2TgtGivenSrc:big-model causes the IBM2TgtGivenSrc feature to be
   generated with argument \"big-model\" and written to file
   <pref>ff.IBM2TgtGivenSrc.big-model.gz. NOTE: each / in \"big-model\" will be
   replaced with _ in this file name, in which case you must also make this
   substitution in MODEL before passing it to rescore_{train,translate}.
 - FileFF:extern is ignored because it does not begin with \"ff.\".
 - SCRIPT indicates that you want to call an external script called
   yourscript located in the experiment main directory.  This script requires
   the nbest list and your SCRIPT MUST output the ff values uncompressed to
   standard out.

   Here are the tags available for the rescoring-model to make the
   model more generic which trigger some action in rat.sh.
   $SRC_TAG this is automatically substituted by SFILE's basename.
   $FFVAL_WTS_TAG this creates a rescoring-model from the canoe.ini and then
   substitute the tag with the newly created file.
   $PFX_TAG which is mandatory and gets substituted by the prefix
   constructed by rat.sh
   $NBEST_TAG this is automatically substituted by the nbest file's name.
   $NUM_PROC_TAG this is autotmatically replaced by the number of nodes requested
   by the user.

";
   exit 1;
}

# Rename an existing file to avoid accidentally re-using old data from an
# aborted or completed previous run.
sub rename_old {
   my $__FILENAME = shift;
   my $__SUFFIX = shift || "old";
   if ( -e $__FILENAME ) {
      my $__BACKUP_SUFFIX = "${__SUFFIX}01";
      my $__BACKUP_FILENAME = "$__FILENAME.$__BACKUP_SUFFIX";
      while ( -e $__BACKUP_FILENAME ) {
         $__BACKUP_SUFFIX++;
         $__BACKUP_FILENAME = "$__FILENAME.$__BACKUP_SUFFIX";
      }
      verbose("Moving existing file $__FILENAME to $__BACKUP_FILENAME");
      rename $__FILENAME, $__BACKUP_FILENAME;
   }
}

# Print error message and exit
sub error_exit {
   print STDERR map { "$_\n" } @_;
   print STDERR "Use -h for help.\n";
   exit 1;
}

# Print warning message to sderr
sub warn_user {
   print STDERR "WARNING: @_\n";
}

# Print debugging information if debugging is enabled
sub debug {
   if (defined($DEBUG)) {
      print STDERR map { "<D> $_\n" } @_;
   }
}

# Print verbose information if vervose is enabled
sub verbose {
   if (defined($VERBOSE)) {
      print STDERR map { "<V> $_\n" } @_;
   }
}

# Return the small value between two.
sub min ($$) { return $_[0] <= $_[1] ? $_[0] : $_[1] }


use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   help        => sub { usage },
   verbose     => sub {$VERBOSE="-v" },
   debug       => \$DEBUG,

   n           => \$NOEXEC,
   F           => \$FORCE_OVERWRITE,

   w           => sub { $WORDFEAT = "-w" },

   "s=s"       => \$SPROXY,
   "p=s"       => \$PREF,
   "a=s"       => \$ALIGFILE,
   "c=s"       => \$CANOEFILE,
   "o=s"       => \$RESCORING_MODEL_OUT,
   "N=i"       => \$N,
   "J=i"       => \$JOBS_PER_FF,
) or usage;


debug "ARGC: $#ARGV";
debug "ARGV: @ARGV";

# Get the mandatory arguments
$MODEL = shift or error_exit("Missing rescoring model");
$SFILE = shift or error_exit("Missing source file");
$NBEST = shift or error_exit("Missing Nbest list file");

# Verify that the mandatory arguments are accessible.
error_exit("Error: Cannot read MODEL file $MODEL.") unless (-r $MODEL);
error_exit("Error: Cannot read SFILE file $SFILE.") unless (-r $SFILE);
error_exit("Error: Cannot read NBEST file $NBEST.") unless (-r $NBEST);
error_exit("You should specify a number of nodes greater than 0") unless ($N > 0);

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

if ( $NOEXEC ) {
   my $VERBOSE=undef;
}


# Get the nbest list and source file's size for later consistency check.
my $WC_NBEST=`gzip -cqfd $NBEST | wc -l`;
chomp($WC_NBEST);
my $NUM_SRC_SENT = `wc -l < $SFILE`;
chomp($NUM_SRC_SENT);


# Make sure the rescore-model in and out are different.
if( defined($RESCORING_MODEL_OUT) and $RESCORING_MODEL_OUT eq $MODEL) {
   error_exit "Specify a different output name $MODEL $RESCORING_MODEL_OUT"
}

# Calculate the proper number of jobs per feature function if not specified by
# the user.
if ( $JOBS_PER_FF == -1 ) {
   # Computable feature functions are does that are not commented or FileFF
   my $L = `cat $MODEL | egrep -v '^\\s*#' | egrep -v "FileFF" | wc -l`;
   debug "L: $L";
   use POSIX qw(ceil);
   $L = 1 if ($L <= 0);
   $JOBS_PER_FF = 2 * ceil($N / $L);
   debug "JOBS_PER_FF: $JOBS_PER_FF";
}
error_exit "Number of jobs per feature function must be greater than 0" unless($JOBS_PER_FF > 0);


# Make it easy to recover the command line from saved logs by printing the
# command and its arguments.
if (defined($VERBOSE)) {
   print "$0 \\\n";
   print map { "   $_ \\\n" } @ARG_ORIGINAL;
   print "\n";
   printf "Starting on %s\n", `date`;
}


# Look for any feature function that would require the ffval weights
# and prepare a file accordingly.
my $CANOEWEIGHTFILE;
if ( system("cat $MODEL | grep -v '^#' | grep -m1 '$FFVAL_WTS_TAG' > /dev/null") == 0) {
   debug "A feature required the ffval-wts";
   if (defined($CANOEFILE)) {
      $CANOEWEIGHTFILE = "${PREF}_canoeweights.tmp";
      $CANOEWEIGHTFILE =~ s/(.*)\/([^\/]+)/$1\/_$2/;
      rename_old $CANOEWEIGHTFILE;
      `configtool rescore-model:ffvals.gz $CANOEFILE > $CANOEWEIGHTFILE`;
   }
   else {
      error_exit "Canoe weights file needed by at least one feature";
   }
}


# Extract the basename of the source file to replace the $SRC_TAG tag
$SPROXY = $SFILE unless(defined($SPROXY));
my $SPROXY_BASE = $SPROXY;
$SPROXY_BASE =~ s/.*\///;

# if the user asked for a rescoring-model.rat => delete previous
rename_old $RESCORING_MODEL_OUT if(defined($RESCORING_MODEL_OUT));


# Prepare the require streams
open(MODEL_FILE, $MODEL) or die "Unable to open $MODEL";
if (defined($RESCORING_MODEL_OUT)) {
   open(RESCORING_MODEL_OUT_FILE, ">$RESCORING_MODEL_OUT") or die "Unable to open $RESCORING_MODEL_OUT";
}


# Process the MODEL file and launch gen_feature_value jobs
# Build a process list for generating the feature function values
my @PARALLEL_FILE = ();   # List files/features that were run in parallel and need merging
my $start_time = time;
while (my $LINE = <MODEL_FILE>) {

   # Skip the empty lines
   if ($LINE =~ /^\s*$/) {
      debug "Removing empty line";
      next;
   }

   # We MUST clear the command array
   my @CMDS = ();

   # Skips commented lines
   if ($LINE =~ /^\s*#/) {
      # Keep the commented line in the outputed rescore-model
      print RESCORING_MODEL_OUT_FILE $LINE if (defined($RESCORING_MODEL_OUT));
      $LINE =~ s/^\s*#+//;
      warn_user "Skipping commented feature: $LINE";
      next;
   }

   # Parse the line to extract the feature's name, argument and weight
   # FEATURE[:ARGUMENTS] [WEIGHT]
   $LINE =~ /^([^:\t ]+)(:\S+|:".*")?(\s+[^"]+)?\s*$/;
   my $FEATURE = $1 or die "Invalid syntaxe, no FF found in: $LINE";
   my $ARGS    = $2;
   my $FILE    = $2;
   my $WEIGHT  = $3 || "";
   chomp($WEIGHT);
   $WEIGHT =~ s/^\s*//;

   # Skip lines that don't contain a feature function
   if ($FEATURE =~ /^\s*$/) {
      warn_user "Couldn't parse a feature function in $LINE\n";
      exit 7;
   }

   # Let's get over with the FileFF as soon as possible.
   if ($FEATURE eq "FileFF") {
      # Don't forget to write the feature to the rescoring-model
      if (defined($RESCORING_MODEL_OUT)) {
         # Just copy the input line defining the FileFF
         print RESCORING_MODEL_OUT_FILE "$LINE";
      }
      next;
   }

   # Generate feature function files for those feature that are not FileFF
   debug "";

   # Get the feature's argument and change the users' tags accordingly
   # if the user forgot to specify <pfx>, rat.sh still needs it thus we will
   # add it (second perl command in the pipe).
   if (defined($ARGS)) {
      # first thing is to strip out the :
      $ARGS =~ s/^://;
      $ARGS =~ s/^"//;
      $ARGS =~ s/"$//;
      $ARGS =~ s/$SRC_TAG/$SPROXY/g;
      $ARGS =~ s/$FFVAL_WTS_TAG$/$CANOEWEIGHTFILE#$PREF/g;
      $ARGS =~ s/$FFVAL_WTS_TAG/$CANOEWEIGHTFILE/g;
      $ARGS =~ s/$PFX_TAG/$PREF/g;
      $ARGS =~ s/$NBEST_TAG/$NBEST/g;
      $ARGS =~ s/$NUM_PROC_TAG/$N/g;
   }
   else {
      $ARGS = "no-arg";
   }

   # Get the output file name and strips ffvals-wts and pfx from the file name to make it shorter
   if (defined($FILE)) {
      # first thing is to strip out the :
      $FILE =~ s/^://;
      $FILE =~ s/^"//;
      $FILE =~ s/"$//;
      $FILE =~ s/$SRC_TAG/$SPROXY_BASE/g;
      $FILE =~ s/#?$FFVAL_WTS_TAG//g;
      $FILE =~ s/#?$PFX_TAG//g;
      $FILE =~ s/$NBEST_TAG/$NBEST/g;
      $FILE =~ s/$NUM_PROC_TAG/N/g;
      $FILE =~ s/:/./ unless $FILE =~ /FileFF/;
      # Replace slash, backslash and semi-colon by underscore
      $FILE =~ s#[/\\;&|<>]#_#g;
      $FILE =~ s/ //g;
   }

   # What is the output filename for this feature function
   chomp $FEATURE;
   my $FF_FILE = "ff.$FEATURE";
   $FF_FILE = "$FF_FILE.$FILE" if(defined($FILE));
   $FF_FILE = "$FF_FILE.gz" unless ($FF_FILE =~ /\.gz$/);
   $FF_FILE =~ s/[\/\"\'\$\\;&|<>*^]/#/go;  # These characters mustn't be part of the filename.

   # Create a rescoring-model for rescore_train and rescore_translate
   if (defined($RESCORING_MODEL_OUT)) {
      print RESCORING_MODEL_OUT_FILE "FileFF:$FF_FILE $WEIGHT\n";
   }


   # For debugging the feature function's parsing
   chomp($LINE);  # Makes for prettier debugging output
   debug "L: $LINE";
   debug "F: $FEATURE";
   debug "A: $ARGS";
   debug "f: $FF_FILE";
   debug "w: $WEIGHT";

   # From this point on we will refer to FF_FILE with the prefix.
   $FF_FILE = "${PREF}$FF_FILE";
   debug "p/f: $FF_FILE";

   # Check if this file was previously generated
   if ( -e $FF_FILE ) {
      my $nbest_size = `gzip -cqfd $NBEST | wc -l`;
      chomp($nbest_size);
      my $ff_size = `gzip -cqfd $FF_FILE | wc -l`;
      chomp($ff_size);
      if (defined($FORCE_OVERWRITE)) {
         warn_user "Force overwrite for: $FEATURE";
      }
      # What to do if the feature file has the same number of line as the nbest.
      elsif ($nbest_size == $ff_size) {
         # make sure the nbest is older then the feature file or else it is out-of-date
         if ( -M $NBEST > -M $FF_FILE) {
            warn_user "Feature file already exists and up-to-date for $FEATURE";
            next;
         }
         else {
            warn_user "Feature file is out-of-date for $FEATURE"
         }
      }
      else {
         # The feature file is most likely wrong, thus regenerate it.
         warn_user "Invalid feature file for $FEATURE, regenerating one";
         verbose "File $FF_FILE contains $ff_size / $nbest_size\n";
      }
   }

   # Does this feature requries the canoe.ini file and do we have it
   if ( $LINE =~ /$FFVAL_WTS_TAG/ and not defined($CANOEWEIGHTFILE) ) {
      warn_user "Canoe weights file needed for calculation of $FEATURE";
      warn_user "Due to missing info, SKIPPING $FEATURE";
      next;
   }

   # Special case for external/scripted feature function
   if ( $FEATURE eq "SCRIPT" ) {
      # Prepare the commands script
      # Grab the stdout of that script
      push @CMDS, "$ARGS | gzip > $FF_FILE";

      debug "Script feature, command is $CMDS[0]";

      # noexec stuff
      if ( defined($NOEXEC) ) {
         print map { "$_\n" } @CMDS;
      }
      else {
         # Queue the requested command
         print $CMDS_FILE_HANDLE map {"$_\n"} @CMDS;
      }

      next;
   }

   # Checking for alignment file requirement
   my $ALIGN_OPT = "";
   my $NEEDS_ALIGN = `feature_function_tool -a $FEATURE $ARGS | egrep "Alignment:"`;
   if ( $NEEDS_ALIGN =~ /TRUE/ ) {
      verbose "This feature needs alignment $FEATURE";
      if ( defined($ALIGFILE) ) {
         $ALIGN_OPT = "-a $ALIGFILE";
      }   
      else {
         warn_user "Alignment file needed for calculation of $FEATURE";
         warn_user "Due to missing info, SKIPPING $FEATURE";
         next;
      }
   }

   # Parallelize complexe features
   my $RANGE = "";
   my $COMPLEXITY = `feature_function_tool -c $FEATURE $ARGS | egrep "Complexity:" | cut -f2 -d' '`;
   chomp($COMPLEXITY);
   debug "COMPLEXITY: $COMPLEXITY";
   if ($COMPLEXITY == 0) {
      push @CMDS, "gen_feature_values $WORDFEAT $ALIGN_OPT -o $FF_FILE $VERBOSE $FEATURE $ARGS $SFILE $NBEST";
   }   
   elsif ($COMPLEXITY == 1 or $COMPLEXITY == 2) {
      # Add to a list of features that need their chunk files merged
      push(@PARALLEL_FILE, $FF_FILE);

      my $NUM_JOB = $JOBS_PER_FF * $COMPLEXITY;
      $NUM_JOB = 1 if ($N == 1);  # Don't split the job if the user wants to use only one cpu
      # Calculate the number of lines per block
      my $LINES_PER_BLOCK = ceil(( $NUM_SRC_SENT + $NUM_JOB - 1 ) / $NUM_JOB);
      debug "LINES_PER_BLOCK = $LINES_PER_BLOCK";

      $FF_FILE =~ s/.gz$//;    # Remove gz extension to be able to add chunk's id

      # For all but the last block
      foreach my $r (0 .. ($NUM_JOB - 1)) {
         my $INDEX = sprintf ".gfv%3.3d", $r;
         my $MIN = $r * $LINES_PER_BLOCK;
         my $MAX = min($MIN + $LINES_PER_BLOCK, $NUM_SRC_SENT);
         debug "min: $MIN max: $MAX";
         my $TMP_CMD = "gen_feature_values $WORDFEAT $ALIGN_OPT -min $MIN -max $MAX";
         $TMP_CMD = "$TMP_CMD -o $FF_FILE$INDEX.gz $VERBOSE $FEATURE $ARGS $SFILE $NBEST";
         push @CMDS, $TMP_CMD;
      }
   }
   else {
      warn "Unknown complexity: $COMPLEXITY\n";
   }

   # noexec stuff
   if ( defined($NOEXEC) ) {
      print map { "$_\n" } @CMDS;
      next;
   }

   # Queue the requested command
   print $CMDS_FILE_HANDLE map {"$_\n"} @CMDS;
}

close(MODEL_FILE);
close($CMDS_FILE_HANDLE);
close(RESCORING_MODEL_OUT_FILE) if (defined($RESCORING_MODEL_OUT));

printf STDERR "Processing $MODEL in %ds\n", time - $start_time;


# Now that we have the processes' list, we can execute them
if ( -s $CMDS_FILE_NAME ) {
   verbose "run-parallel.sh $VERBOSE $CMDS_FILE_NAME $N";
   my $RC = 0;
   if (not defined($NOEXEC)) {
      $RC = system("run-parallel.sh $VERBOSE $CMDS_FILE_NAME $N");
   }

   # Now merge feature functions' values that were generated horizontally.
   debug "FF to merge: @PARALLEL_FILE";
   foreach my $ff (@PARALLEL_FILE) {
      my $ALL_FF_PARTS = $ff;
      $ALL_FF_PARTS =~ s/.gz$//;
      $ALL_FF_PARTS = "$ALL_FF_PARTS.gfv???*";
      debug "Trying to merge $ALL_FF_PARTS";
      if ($RC == 0 or $WC_NBEST eq `zcat $ALL_FF_PARTS | wc -l`) {
         `zcat $ALL_FF_PARTS | gzip > $ff`;
         `rm $ALL_FF_PARTS`;
      }
   }

   if ( $RC != 0 ) {
      error_exit "problems with run-parallel.sh(RC=$RC) - quitting!",
                 "The failed outputs will not be merged or deleted to give",
                 "the user a chance to salvage as much data as possible.";
   }
}

# Finally, delete the commands' file
# Always delete the file, it might be empty.
# Deletion is done through automatic tempfile mechanism.
debug "Removing commands' file: $CMDS_FILE_NAME";

# Everything is fine if we get to this point
exit 0;

