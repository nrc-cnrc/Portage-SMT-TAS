#!/usr/bin/env perl
# $Id$
# @file translate.pl
# @brief Script to translate text.
#
# @author Darlene Stewart (adapted from Michel Simard's ce_translate.pl)
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

=pod

=head1 NAME

translate.pl - Translate text

=head1 SYNOPSIS

 translate.pl -decode-only|-with-rescoring|-with-ce [options]
              [-f CANOE_INI] [SRC_TEXT]

=head1 DESCRIPTION

This program translates a source text file SRC_TEXT to the target language 
according to the current trained models. If SRC_TEXT is not specified, the 
source text is read from standard input (STDIN). With the C<-tmx> option,
SRC_TEXT is a TMX format file from which the source text is extracted and to 
which the translated text is replaced.

By default, the program creates a temporary working directory, where it stores
intermediate results, including tokenized and lowercased versions of the text
files, PORTAGE translations, and for C<-with-ce>, value files for the features
used by the CE model. Normally this temporary directory is deleted at the
end of the process, unless something goes wrong or if the C<-dir=D>option or
the C<-d> option is specified.

=head1 OPTIONS

=head2 Translation Options

=over 12

=item -decode-only

Decode only without rescoring or confidence estimation.
Use -f|-ini to specify the canoe.ini file describing the decoding models.

=item -with-rescoring

Decode with rescoring according to a rescoring model (using C<rat.sh>).
Use -model to specify the rescoring model, which defaults to F<rescore-model*>
in F<models/rescore> relative to the F<canoe.ini> location. See the CAVEATS
below regarding the rescoring model.

=item -with-ce

Decode with confidence estimation based on a CE model (using C<ce.pl> to predict
the confidence). See F<ce_translate.pl> for details about confidence estimation.
Use -model to specify the CE model, which defaults to F<*.cem> file in the
F<canoe.ini> location.

=item -n=N

Decode N-ways parallel (using C<canoe-parallel.sh> instead of C<canoe>)

=item -w=W

Make sure each parallel block processes at least W sentences, reducing
N if necessary.

=item -ini=CANOE_INI | -f=CANOE_INI

Use CANOE_INI as the canoe.ini file describing the translation system models.
[F<canoe.ini.cow> if exists or F<${PORTAGE}/models/canoe.ini.cow>]

=item -model=MODEL

Use MODEL as the the rescoring model file for C<-with-rescoring>
or the CE model file for C<-with-ce>.
[F<rescore-model*> file in F<models/rescore> relative to the F<canoe.ini>
location, or F<*.cem> file in F<canoe.ini> location]

=item -[no]tok

Tokenize the input text. Use --notok to skip tokenization only if the input
is pre-tokenized. [tok]

=item -nl=X

Use the following interpretation of newlines in the input text:

=over 8

=item -nl=p

newline marks the end of a paragraph;

=item -nl=s

newline marks the end of a sentence;

=item -nl=

two consecutive newlines mark the end of a paragraph, otherwise newline is just
whitespace. [default]

=back

=item -[no]lc

Lowercase the input text. Use --nolc to skip lowercasing. [lc]

=item -[no]detok

Detokenize the text on output. Use --nodetok to skip detokenization. [detok]

=item -[no]tc | -tctp

If -tc, truecase the text on output using a text LM and map for the truecasing 
model.
If -tctp, truecase using tightly packed LMs and map for the truecasing model.
For -tc or -tctp, the LM and map files, if not specified using -tclm and -tcmap,
are searched for in F<models/tc> relative to the F<canoe.ini> location.
If -notc, skip truecasing.
[notc]

=item -tclm=LM

Use target language model file LM for the truecasing model (valid for -tc or -tctp).
[target LM file located in F<models/tc> directory relative to the F<canoe.ini> location]

=item -tcmap=MAP

Use map file MAP for the truecasing model (valid for -tc or -tctp).
[F<*.map> file located in F<models/tc> directory relative to the F<canoe.ini> location]

=item -tcsrclm=SRCLM

If present, use source sentence-initial case normalization (NC1) language model 
file SRCLM to provide addition source language information for the truecasing 
model (valid for -tc or -tctp).
[source LM file located in F<models/tc> directory relative to the F<canoe.ini> location, if present]

=item -src=SRC

Use SRC as the source language. [en]

=item -tgt=TGT

Use TGT as the target language. [fr]

=item -encoding=ENC

Use encoding ENC as the encoding of the input (SRC_TEXT) and output (OUTPUT).
Supported encodings are: 'utf8', 'cp1252'. [utf8]

=item -out=OUTPUT

Use OUTPUT as the output file name. [STDOUT]

=item -dir=DIR

Use DIR as the working directory for all intermediate files.
DIR is not deleted at the end of processing, while the default temp directory is.
[temp directory F<./translate_work_XXXXXX> or F</tmp/translate_work_XXXXXX> 
if cannot mkdir in F<.>]

=item -plugins=PLUGINS

Add PLUGINS to $PATH as the directory for the plugins (PLUGINS will be searched 
first for the executable for each plugin). (See PLUGINS section below.) 
[directory F<plugins> in the F<canoe.ini> location]

=item -xtra-decode-opts=OPTS

Specify additional options for the canoe decoder (valid for -decode-only or -with-ce).

=item -xtra-cp-opts=OPTS

Specify additional C<canoe-parallel.sh> options (valid when n > 1).

=item -xtra-rat-opts=OPTS

Specify additional C<rat.sh> options (valid for -with-rescoring only).

=back

=head2 TMX Specific Options

=over 12

=item -tmx

The input and output files are in TMX format.

=item -xsrc=XSRC

Use XSRC as the TMX source language name. [EN-CA]

=item -xtgt=XTGT

Use XTGT as the TMX target language name. [FR-CA]

=back

=head2 Confidence Estimation Specific Options

=over 12

=item -filter=T

Filter out translations below confidence threshold T (valid for C<-tmx> mode only).
[no]

=item -xtra-ce-opts=OPTS

Specify additional C<ce.pl> options (valid for -with-ce only).

=back

=head2 Other

=over 12

=item -verbose

Be verbose. (cumulative)

=item -quiet

Make terminal output as quiet as possible by redirecting STDERR to a log file.

=item -debug

Print debugging info.

=item -dryrun

Don't do anything, just print the commands.

=item -help

Print help message and exit.

=item -time

Produce detailed timing info.

=back

=head1 PLUGINS

Application-specific text pre- and post-processing is handled through
a simple "plugin" mechanism: this program calls executables (programs
or scripts) named F<preprocess_plugin>, F<predecode_plugin>,
F<postdecode_plugin> and F<postprocess_plugin> before tokenization,
before and after decoding and after detokenization, respectively.
Default implementations are provided for each of these, but these can
be overridden by providing alternate plugins in a directory called
F<plugins> in the same directory as the F<canoe.ini> file, or in a
plugins directory specified with the C<-plugins> option. All plugins
are expected to read from standard input and write to standard output,
and should require no command-line arguments.

=head1 CAVEATS

For -with-rescoring, in order for rat.sh to work correctly, either the
rescoring model must use absolute paths or its model paths must be of the
form C<models/subpath> and be accessible from the current directory 
(i.e. C<./models/subpath> references the corresponding model).
This is the way portageLive and the framework work.

=head1 SEE ALSO

canoe, canoe-parallel.sh, rat.sh, ce.pl, ce_translate.pl

=head1 AUTHOR

Darlene Stewart

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright (c) 2010, Sa Majeste la Reine du Chef du Canada /
 Copyright (c) 2010, Her Majesty in Right of Canada

=cut

## Developer's note: There is also a -skipto=S option, which can be
## used when debugging to skip to a specific processing stage.  This
## is implemented via Perl's goto mechanism.  Current stages are IN,
## PREP, TRANS, POST, CE, OUT and CLEANUP.

use strict;
use warnings;

BEGIN {
   # If this script is run from within src/ rather than being properly
   # installed, we need to add utils/ to the Perl library include path (@INC).
   if ( $0 !~ m#/bin/[^/]*$# ) {
      my $bin_path = $0;
      $bin_path =~ s#/[^/]*$##;
      unshift @INC, "$bin_path/../utils", $bin_path;
   }
}

use portage_utils;
printCopyright("translate.pl", 2010);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


use File::Temp qw(tempdir);
use File::Path qw(rmtree);
use File::Spec;
use File::Basename;
use Cwd;

use Time::HiRes qw( time );

# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
use Getopt::Long;

my $start_time = time;

my $verbose = 0;
my $saved_command_line = "$0 @ARGV";

Getopt::Long::GetOptions(
   'help'           => sub { displayHelp(); exit 0 },
   "verbose+"       => \$verbose,
   "quiet"          => \my $quiet,
   "debug"          => \my $debug,
   "dryrun"         => \my $dryrun,
   "time"           => \my $timing,
   
   "decode-only"    => \my $decode_only,
   "with-rescoring" => \my $with_rescoring,
   "with-ce"        => \my $with_ce,
   "n=i"            => \my $n,
   "w=i"            => \my $w,
   "ini|f=s"        => \my $canoe_ini,
   "model=s"        => \my $model,
   
   "tok!"           => \my $tok,
   "nl=s"           => \my $nl,
   "lc!"            => \my $lc,
   "detok!"         => \my $detok,
   
   "tc!"            => \my $tc,
   "tctp"           => \my $tctp,
   "tclm=s"         => \my $tclm,
   "tcmap=s"        => \my $tcmap,
   "tcsrclm=s"      => \my $tcsrclm,

   "src=s"          => \my $src,
   "tgt=s"          => \my $tgt,

   "encoding=s"     => \my $encoding,
   
   "out=s"          => \my $out,
   
   "dir=s"          => \my $dir,
   "plugins=s"      => \my $plugins,
   
   "xtra-decode-opts=s"  => \my $xtra_decode_opts,
   "xtra-cp-opts=s"      => \my $xtra_cp_opts,
   "xtra-rat-opts=s"     => \my $xtra_rat_opts,
   
   #TMX specific options
   "tmx"            => \my $tmx,
   "xsrc=s"         => \my $xsrc,
   "xtgt=s"         => \my $xtgt,

   #CE specific options
   "filter=f"       => \my $filter,
   "xtra-ce-opts=s" => \my $xtra_ce_opts,
   
   #Development options
   "skipto=s"    => \my $skipto,
) or (print(STDERR "ERROR: translate.pl aborted due to bad option.\nRun with -h for help.\n"), exit 1);

$quiet = 0 unless defined $quiet;
$debug = 0 unless defined $debug;
$dryrun = 0 unless defined $dryrun;
$timing = 0 unless defined $timing;

if ( !$quiet || $verbose ) {
   print STDERR "$saved_command_line\n\n";
}

$decode_only = 0 unless defined $decode_only;
$with_rescoring = 0 unless defined $with_rescoring;
$with_ce = 0 unless defined $with_ce;
$decode_only + $with_rescoring + $with_ce == 1
   or die "ERROR: Missing or extra switch; specify exactly one of: ",
          "-decode-only, -with-rescoring, -with-ce.\nStopped";

$n = 1 unless defined $n;
$n >= 1 or die "ERROR: n must be a positive integer (n-ways-parallel)";
$n > 1 or !defined $xtra_cp_opts 
   or die "ERROR: -xtra-cp-opts is valid only when using canoe parallel (n>1).\nStopped";

unless (defined $canoe_ini) {
   $canoe_ini = "canoe.ini.cow";
   $canoe_ini = "$ENV{PORTAGE}/models/canoe.ini.cow" unless -f $canoe_ini;
   -f $canoe_ini or die "ERROR: Unable to locate a canoe.ini.cow file; ",
                        "use -f or -ini to specify the canoe.ini file.\nStopped"
}
-f $canoe_ini && -r _ or die "ERROR: canoe.ini file '$canoe_ini' is not a readable file.\nStopped";

my $models_dir = dirname($canoe_ini);
-d $models_dir or die "ERROR: models directory '$models_dir' is not a readable directory.\nStopped";

# Locate the rescoring or CE model, if needed.
my ($rs_model, $ce_model) = ("", "");
if ($with_rescoring) {
   # Locate the rescoring model.
   if (defined $model) {
      $rs_model = $model
   } else {
      my $rescoring_dir = "$models_dir/models/rescore";
      my @files = grep !(/\.ini$/ || /\.template$/), glob "$rescoring_dir/rescore-model*";
      @files > 0 or die "ERROR: Unable to locate a rescore-model file in '$rescoring_dir'; ",
                        -d $rescoring_dir ? "" : "'$rescoring_dir' does not exist; ",
                        "use -model to specify the rescoring model.\nStopped";
      @files == 1 or die "ERROR: Found multiple rescore-model files in '$rescoring_dir'; ",
                         "use -model to specify the rescoring model.\nStopped";
      $rs_model = shift @files;
   }
   -f $rs_model && -r _ or die "ERROR: Rescoring model '$rs_model' is not a readable file.\nStopped";
} elsif ($with_ce) {
   # Locate the CE model.
   if (defined $model) {
      $ce_model = $model
   } else {
      my @files = grep !/^.*\/log\.[^\/]+/, glob "$models_dir/*.cem";
      @files > 0 or die "ERROR: Unable to locate a .cem file in '$models_dir'; ",
                        "use -model to specify the CE model.\nStopped";
      @files == 1 or die "ERROR: Found multiple .cem files in '$models_dir'; ",
                         "use -model to specify the CE model.\nStopped";
      $ce_model = shift @files;
   }
   -f $ce_model && -r _ or die "ERROR: CE model '$ce_model' is not a readable file.\nStopped";
} else {
   !defined $model or die "ERROR: -model option is invalid with -decode-only; "
                        . "use -f or -ini to specify the canoe.ini file.\nStopped";
}

$src = "en" unless defined $src;
$tgt = "fr" unless defined $tgt;

$tok = 1 unless defined $tok;
$lc = 1 unless defined $lc;
$detok = 1 unless defined $detok;

$nl = ($tmx || !$tok ? "s" : "") unless defined $nl;
$nl eq "" or $nl eq "s" or $nl eq "p"
   or die "ERROR: -nl option must be one of: 's', 'p', or ''.\nStopped";
$tok or $nl eq "s"
   or die "ERROR: -notok requires -nl=s to be specified.\nStopped";
!$tmx or $nl eq "s"
   or die "ERROR: -tmx requires -nl=s.\nStopped";

!defined $tc || !defined $tctp
   or die "ERROR: Specify only one of: -notc, -tc, -tctp.\nStopped";
$tc = 0 unless defined $tc;
$tctp = 0 unless defined $tctp;
$tc = 1 if $tctp;
$tc or (!defined $tclm && !defined $tcmap && !defined $tcsrclm)
   or die "ERROR: Do not specify -tclm, -tctp or -tcsrclm with -notc.\nStopped";
(defined $tclm && defined $tcmap) or (!defined $tclm && !defined $tcmap)
   or die "ERROR: Specify neither or both of -tclm and -tcmap.\nStopped";
(defined $tcsrclm && defined $tclm) or (!defined $tcsrclm)
   or die "ERROR: Do not specify -tcsrc without -tclm specified.\nStopped";
!defined $tcsrclm or !$with_rescoring
   or die "ERROR: -tcsrclm cannot be used with -with-rescoring.\nStopped";

# Locate the Truecasing model.
if ($tc and !defined $tclm) {
   my @tc_files = ();
   my $tc_dir = "$models_dir/models/tc";
   my $use_msg = "use -tclm, -tcmap and -tcsrclm to specify the truecasing files.\nStopped";
   -d $tc_dir
      or die "ERROR: '$tc_dir' does not exist; ", $use_msg;
   foreach my $ext ($tctp ? (".tplm", ".tppt") : (".binlm.gz", ".map")) {
      my @files = grep !/\/log\.[^\/]+$/ && !/[\/.-_]nc1[\.-_][^\/]+$/, 
                  glob "$tc_dir/*{.,-,_}$tgt*$ext";
      @files > 0
         or die "ERROR: Unable to locate a $tgt TC $ext file in '$tc_dir'; ",
                "perhaps you need the " , $tctp ? "-tc" : "-tctp",  
                " option instead of ", $tctp ? "-tctp" : "-tc",
                ".\nStopped";
      @files == 1
         or die "ERROR: Found multiple $tgt TC $ext files in '$tc_dir'; ", $use_msg;
      push @tc_files, @files;
   }
   ($tclm, $tcmap) = @tc_files;
   unless ($with_rescoring) {
      # New truecasing workflow using source info currently not compatible with rescoring.
      my $ext = $tctp ? ".tplm" : ".binlm.gz";
      my @files = grep !/\/log\.[^\/]+$/ && /[\/.\-_]nc1[\.\-_][^\/]+$/, 
                  glob "$tc_dir/*{.,-,_}$src*$ext";
      @files <= 1
         or die "ERROR: Found multiple $src NC1 $ext files in '$tc_dir'; ", $use_msg;
      $tcsrclm = $files[0] unless @files == 0;
   }
}
if ($tc) {
   if ($tctp) {
      -d $tclm && -x _ 
         or die "ERROR: Tightly packed truecasing $tgt model '$tclm' ",
                "is not a readable directory.\nStopped";
      -d $tcmap && -x _ 
         or die "ERROR: Tightly packed truecasing map '$tcmap' ",
                "is not a readable directory.\nStopped";      
   } else {
      -f $tclm && -r _ 
         or die "ERROR: Truecasing $tgt model '$tclm' is not a readable file.\nStopped";
      -f $tcmap && -r _ 
         or die "ERROR: Truecasing map '$tcmap' is not a readable file.\nStopped";
   }
   if (defined $tcsrclm) {
      if ($tcsrclm =~ /.tplm$/) {
         -d $tcsrclm && -x _ 
            or die "ERROR: Tightly packed truecasing $src model '$tcsrclm' ",
                   "is not a readable directory.\nStopped";
      } else {
         -f $tcsrclm && -r _ 
            or die "ERROR: Truecasing $src model '$tcsrclm' is not a readable file.\nStopped";      
      }
   }
}

my $utf8 = 1;
if (defined $encoding) {
   my $lc_enc = lc $encoding;
   $utf8 = $lc_enc eq "utf8" || $lc_enc eq "utf-8";
   $utf8 or $lc_enc eq "cp1252" 
      or die "ERROR: -encoding must be one of: 'utf8' or 'cp1252'.\nStopped";
}

# Locate the Plugins directory
my $plugins_dir = defined $plugins ? $plugins : "$models_dir/plugins";
if (defined $plugins) {
   -d $plugins_dir 
      or warn "WARNING: plugins directory '$plugins_dir' is not a readable directory";
}

$decode_only or $with_ce or !defined $xtra_decode_opts
   or die "ERROR: -xtra-decode-opts is valid only with -decode-only or -with-ce.\nStopped";
$with_rescoring or !defined $xtra_rat_opts
   or die "ERROR: -xtra-rat-opts is valid only with -with-rescoring.\nStopped";
$xtra_cp_opts = "" unless defined $xtra_cp_opts;
$xtra_decode_opts = "" unless defined $xtra_decode_opts;
$xtra_rat_opts = "" unless defined $xtra_rat_opts;

# TMX specific options
$tmx = 0 unless defined $tmx;
if ($tmx) {
   $xsrc = "EN-CA" unless defined $xsrc;
   $xtgt = "FR-CA" unless defined $xtgt;
} else {
   !defined $xsrc and !defined $xtgt and !defined $filter
      or die "ERROR: -xsrc, -xtgt and -filter are valid only with -tmx.\nStopped"
}

# CE specific options
if ($with_ce) {
   $xtra_ce_opts = "" unless defined $xtra_ce_opts;
} else {
   !defined $filter
      or die "ERROR: -filter is valid only with -with-ce.\nStopped";
   !defined $xtra_ce_opts
      or die "ERROR: -xtra-ce-opts is valid only with -with-ce.\nStopped";
}

@ARGV <= 1 or die "ERROR: Too many arguments.\nStopped";
@ARGV > 0 or die "ERROR: Too few arguments. SRC_TEXT file required.\nStopped" if $tmx;
my $input_text = @ARGV > 0 ? shift : "-";

unless (defined $out) {
   $out = "-";
} else {
   system("echo '' >$out") == 0
      or die "ERROR: '$out' is not a writable file.\nStopped";
}

# Make working directory and the log file.
my $keep_dir = $dir;
my $plog_file;

open(SAVE_STDERR, ">&STDERR");   # later, STDERR may be redirected to a log file.

if ($dryrun) {
    $dir = "translate_work_temp_dir" unless $dir;
} elsif ($skipto) {
    $dir or die "ERROR: Use -dir with -skipto.\nStopped";
    -d $dir or die "ERROR: Unreadable directory '$dir' with -skipto.\nStopped";
    if ($quiet) {
       # Make terminal output as quiet as possible by redirecting STDERR.
       open(STDERR, ">>", "${dir}/log.translate.pl");
       print STDERR "\n---------- " . localtime() . "skipto $skipto ----------\n"
          or warn "WARNING: Unable to redirect STDERR to append to '${dir}/log.translate.pl'";
    }
} else {
    if ($dir) {
        if (not -d $dir) {
           mkdir $dir or die "ERROR: Can't make directory '$dir': errno=$!.\nStopped";
        }
    } else {
        $dir = "";
        # Use eval to avoid death if unable to create the work directory .
        eval {$dir = tempdir('translate_work_XXXXXX', DIR=>".", CLEANUP=>0);};
        if (not -d $dir) {
           $dir = tempdir('translate_work_XXXXXX', TMPDIR=>1, CLEANUP=>0);
           # Prevent running in cluster mode if using /tmp as the work directory.
           $ENV{PORTAGE_NOCLUSTER} = 1;
        }
    }
    if ($quiet) {
       # Make terminal output as quiet as possible by redirecting STDERR.
       open(STDERR, ">", "${dir}/log.translate.pl") 
          or warn "WARNING: Unable to redirect STDERR to '${dir}/log.translate.pl'";
    }
    $plog_file = plogCreate("File:${input_text}; Context:".File::Spec->rel2abs($models_dir));
}

$keep_dir = $dir if $debug;
verbose("[Work directory: \"${dir}\"]\n");
verbose("[models_dir: '$models_dir' (" . Cwd::realpath($models_dir) . ")]\n");
verbose("[Processing input text \"${input_text}\"]\n");

# Generate a README file for the working directory.
unless ($dryrun) {
   open (README, ">", "$dir/README")
      or cleanupAndDie("Unable to open '$dir/README' for writing.\n");
   print README (
      "Filenames starting with 'Q' or 'q' = query text = source language input.\n",
      "Filenames starting with 'P' or 'p' = product text = target language output.\n",
      "\n",
      "Filenames starting with an uppercase letter contain (possibly) truecased text\n",
      "i.e. before lowercasing or after truecasing has been applied.\n",
      "Filenames starting with a lowercase letter contain lowercase text\n",
      "i.e. after lowercasing and before truecasing has been applied.\n",
      "\n",
      "Extensions:\n",
      ".txt = raw source text / final translation (post-processed) output text\n",
      ".pre = pre-processed (pre-tokenization) source\n",
      ".filt = post-filtering source\n",
      ".tok = tokenized source / translation (after post-decoder plugin)\n",
      ".dec = decoder ready source / raw decoder translation (OOV markup removed)\n",
      ".dec.oov = raw decoder translation with OOV markup\n",
      ".tok.oov = tokenized translation with OOV markup (after post-decoder plugin)\n",
      ".pal = phrase alignments from the decoder used with truecasing\n",
      ".raw = raw decoder output (with trace) for -with-ce and truecasing\n",
      ".dtk = detokenized translation\n",
      "pr.ce = confidence estimation\n",
      "\n",
      "Final translation output is in P.txt\n",
   );
   close README;   
}

# File names - the naming scheme required for use with CE is adhered to.
my $ostype = `uname -s`;
chomp $ostype;
my $ci = ($ostype eq "Darwin") ? "l" : ""; # for case insensitive file systems

my $Q_txt = "${dir}/Q.txt";     # Raw source text
# --> preprocessor plugin
my $Q_pre = "${dir}/Q.pre";     # Pre-processed (pre-tokenization) source
# --> tokenize
my $Q_tok = "${dir}/Q.tok";     # Tokenized source
# --> lowercase
my $q_tok = "${dir}/q${ci}.tok";    # Lowercased tokenized source
# --> predecoder plugin
my $q_dec = "${dir}/q.dec";     # Decoder-ready source
# --> decoding
my $p_raw = "${dir}/p.raw";     # Raw decoder output (with trace)
# --> decoder output parsing
my $p_decoov = "${dir}/p.dec.oov";  # Raw decoder translation with OOV markup
my $p_dec = "${dir}/p.dec";     # Raw decoder translation (OOV markup removed)
my $p_pal = "${dir}/p.pal";     # Phrase alignments used with truecasing
# --> postdecoder plugin
my $p_tokoov = "${dir}/p.tok.oov";  # Post-decoder plugin processed translation (with OOV markup)
my $p_tok = "${dir}/p${ci}.tok";    # Post-decoder plugin processed translation (OOV markup removed)
# --> truecasing
my $P_tok = "${dir}/P.tok";     # Truecased tokenized translation
# --> detokenization
my $P_dtk = "${dir}/P.dtk";     # Truecased detokenized translation
# --> postprocessor plugin
my $P_txt = "${dir}/P.txt";     # translation

#Others:
my $pr_ce = "${dir}/pr.ce";     # confidence estimation
my $Q_filt = "${dir}/Q.filt";   # post-filtering source

# Skipto option
goto $skipto if $skipto;

# Get source text
IN:{
   unless ($tmx) {
      copy($input_text, $Q_txt);
   } else {
      call("ce_tmx.pl -verbose=${verbose} -src=${xsrc} -tgt=${xtgt} extract '$dir' '$input_text'");
   }
}

# Preprocess, tokenize and lowercase
PREP:{
   plugin("preprocess", $src, $Q_txt, $Q_pre);
   tokenize($src, $Q_pre, $Q_tok);
   lowercase($Q_tok, $q_tok);
}

# Translate
TRANS:{
   plugin("predecode", $src, $q_tok, $q_dec);

   if (defined $w and $n > 1) {
      my $sent_count = `wc -l < $q_dec` + 0;
      if ($w * $n > $sent_count) {
         $n = int($sent_count / $w);
         $n > 0 or $n = 1;
      }
   }
   
   unless ($with_rescoring) {
      my $decoder = "canoe";
      if ($n > 1) {
         my $v = $verbose ? "-v" : "";
         $decoder = "canoe-parallel.sh $v -n $n $xtra_cp_opts canoe";
      }
      my $decoder_opts = $verbose ? "-v $verbose" : "";
      $decoder_opts .= " -palign" if (defined $tcsrclm or $with_ce);
      $decoder_opts .= " -ffvals" if $with_ce;
      $decoder_opts .= " $xtra_decode_opts";
      my $p_out = (defined $tcsrclm or $with_ce) ? $p_raw : $p_dec;
      call("$decoder $decoder_opts -f ${canoe_ini} < '${q_dec}' > '${p_out}'");
      if (defined $tcsrclm) {
         call("nbest2rescore.pl -canoe -tagoov -palout='${p_pal}' < '${p_raw}' " . 
              "| perl -pe 's/ +\$//;' > '${p_decoov}'");
      }
      if ($with_ce) {
         # ce_canoe2ffvals.pl generates $p_dec from $p_raw, among other things
         call("ce_canoe2ffvals.pl -verbose=${verbose} -dir='${dir}' '${p_raw}'");
      }
   } else {  # $with_rescoring
      # We run rat.sh from the working directory to have it create its working
      # directory inside ours to avoid name clashes.
      #
      # For rat.sh to work correctly, either the rescoring model must use
      # absolute paths or its model paths must be of the form "models/subpath"
      # and be accessible from the current directory (i.e. "./models/subpath""
      # references the corresponding model).
      # This is the way portageLive and portage.simple.framework.2 work.
      #
      # We link to the models directory from the working directory
      # so rat.sh can access its models as "models/subpath".
      symlink(Cwd::realpath("$models_dir/models"), "$dir/models");
      
      # We need to modify our paths to run in the working directory.
      if ($canoe_ini !~ /^\//) {
         $dir =~ /^\//;
         $canoe_ini = ($&) ? Cwd::realpath($canoe_ini) : "../$canoe_ini";
         $rs_model = ($&) ? Cwd::realpath($rs_model) : "../$rs_model";
      }
      my $cwd = cwd();
      chdir($dir);
      my $cp_opts = "-n $n $xtra_cp_opts";
      my $rat_opts = $verbose ? "-v" : "";
      $rat_opts .= " $xtra_rat_opts";
      call("rat.sh $cp_opts trans $rat_opts -msrc q.dec -f ${canoe_ini} ${rs_model} q.tok");
      rename("q.dec.rat", "p.dec");
      chdir($cwd);
   }
   
   if (not defined $tcsrclm) {
      plugin("postdecode", $tgt, $p_dec, $p_tok);   
   } else {
      plugin("postdecode", $tgt, $p_decoov, $p_tokoov);
      call("cat '${p_tokoov}' | perl -pe 's/<OOV>(.+?)<\\/OOV>/\\1/g;' >'${p_tok}'"); 
   }
}

# Truecase, detokenize, and postprocess
POST:{
   truecase($tgt, defined $tcsrclm ? $p_tokoov : $p_tok, $P_tok, $Q_tok, $p_pal);
   detokenize($tgt, $P_tok, $P_dtk);
   plugin("postprocess", $tgt, $P_dtk, $P_txt);
}

# Predict CE
CE:{
   if ($with_ce) {
      call("ce.pl -verbose=${verbose} ${xtra_ce_opts} ${ce_model} '${dir}'");
   }
}

# Produce output
OUT:{
   unless ($tmx) {
       if ($with_ce) {
           my $ce_output = $out ne "-" ? "> '$out'" : "";
           call("paste ${dir}/pr.ce '${P_txt}' ${ce_output}");
       } else {
           copy($P_txt, $out);
       }
   } else {
       my $fopt = defined $filter ? "-filter=$filter" : "";
       my $sopt = $with_ce ? "-score" : "-noscore";
       call("ce_tmx.pl -verbose=${verbose} -src=${xsrc} -tgt=${xtgt} ${fopt} ${sopt} replace '$dir'");
   }
}

# Cleanup
CLEANUP:{
    my $words_in = sourceWordCount();
    my $words_out = defined($filter) ? sourceWordCount($filter) : $words_in;
    plogUpdate($plog_file, 'success', $words_in, $words_out);
    unless ($keep_dir) {
        verbose("[Cleaning up and deleting work directory $dir]\n");
        rmtree($dir);
    }
}

(print STDERR "translate.pl: translation workflow took ", time - $start_time, " seconds.\n") if $timing;
exit 0; # All done!


## Subroutines

sub displayHelp
{  
   -t STDERR ? system "pod2text -t -o $0 >&2" : system "pod2text $0 >&2";
}

sub copy {
   my ($in, $out) = @_;
   
   my $cmd = $out eq "-" ? qq(cat "$in") 
                         : $in eq "-" ? qq(cat >"$out") : qq(cp "$in" "$out");
   call($cmd, $out ne "-" ? $out : "");
}

sub plugin {
   my ($name, $lang, $in, $out) = @_;
   my $old_path = $ENV{PATH};
   $ENV{PATH} = "${plugins_dir}:".$ENV{PATH};
   my $actual_prog = `which ${name}_plugin`;
   chomp($actual_prog);
   print STDERR "Using plugin: ${actual_prog}\n";
   call("${actual_prog} ${lang} < '${in}' > '${out}'", $out);
   $ENV{PATH} = $old_path;
}

sub plogCreate {
   my ($job_name, $comment) = @_;
   my $plog_file;

   if ($dryrun) {
      $plog_file = "dummy-log";
   } else {
      my @plog_opt = qw(-create);
      push @plog_opt, "-verbose" if $verbose;
      push @plog_opt, "-comment=\"$comment\"" if defined $comment;
      my $cmd = "plog.pl ".join(" ", @plog_opt)." \"${job_name}\"";
      $plog_file = callOutput($cmd);
   }
    
   return $plog_file;
}

sub plogUpdate {
   my ($plog_file, $status, $words_in, $words_out, $comment) = @_;
   $words_in = 0 unless defined $words_in;
   $words_out = $words_in unless defined $words_out;

   return unless $plog_file;   # means "no logging"

   return if $dryrun;

   my @plog_opt = qw(-update);
   push @plog_opt, "-verbose" if $verbose;
   push @plog_opt, "-comment=\"$comment\"" if defined $comment;
   my $cmd = "plog.pl ".join(" ", @plog_opt)." '${plog_file}' $status $words_in $words_out";
   # Don't use call(): potential recursive loop!!
   system($cmd) == 0 or warn "WARNING: ", explainSystemRC($?,$cmd,$0);
}

sub sourceWordCount {
   my ($filter) = @_;
   my $count_file;
   if (defined $filter) {
       open(my $tfh, "<${Q_pre}") or die "Can't open text file ${Q_pre}";
       open(my $cfh, "<${pr_ce}") or die "Can't open CE file ${pr_ce}";
       open(my $ffh, ">${Q_filt}") or die "Can't open filtered source ${Q_filt}";

       while (my $t = <$tfh>) {
           my $y = readline($cfh);
           die "Not enough lines in CE file ${pr_ce}"
               unless defined $y;
           print {$ffh} $t unless ($y < $filter);
       }
       warn "Too many lines in CE file ${pr_ce}" if readline($cfh);

       close $tfh;
       close $cfh;
       close $ffh;

       $count_file = $Q_filt;
   } else {
       $count_file = $Q_pre;
   }

   cleanupAndDie("Can't open temp file ${count_file} for reading.\n") unless (-r "${count_file}");
   my $cmd = "wc -w < '${count_file}'";
   return 0 + `$cmd`; # Add "0 +" to return a number, not a string with a newline.
}

sub tokenize {
   my ($lang, $in, $out) = @_;
   if (!$tok and $nl eq 's') {
      copy($in, $out);
   }
   else {
      my $old_path = $ENV{PATH};
      $ENV{PATH} = "${plugins_dir}:".$ENV{PATH};
      my $cmd = `which tokenize_plugin 2> /dev/null`;
      $ENV{PATH} = $old_path;
      chomp($cmd);
      if ( $cmd ) {
         plugin("tokenize", $src, $Q_pre, $Q_tok);
      }
      else {
         my $tokopt = " -lang=${lang}";
         $tokopt .= $nl eq 's' ? " -noss" : " -ss";
         $tokopt .= " -paraline" if $nl eq 'p';
         $tokopt .= " -pretok" if !$tok;
         my $u = $utf8 ? "u" : "";
         call("${u}tokenize.pl ${tokopt} '${in}' '${out}'", $out);
      }
   }
}

sub detokenize {
   my ($lang, $in, $out) = @_;
   unless ($detok) {
      copy($in, $out);
   }
   else {
      my $old_path = $ENV{PATH};
      $ENV{PATH} = "${plugins_dir}:".$ENV{PATH};
      my $cmd = `which detokenize_plugin 2> /dev/null`;
      $ENV{PATH} = $old_path;
      chomp($cmd);
      if ( $cmd ) {
         plugin("detokenize", $tgt, $P_tok, $P_dtk);
      }
      else {
         my $u = $utf8 ? "u" : "";
         call("${u}detokenize.pl -lang=${lang} < '${in}' > '${out}'", $out);
      }
   }
}

sub lowercase {
   my ($in, $out) = @_;
   unless ($lc) {
      copy($in, $out);
   } else {
      my $lc_prog = $utf8 ? "utf8_casemap -c l" : "lc-latin.pl";
      call("${lc_prog} '${in}' '${out}'", $out);
   }
}

sub truecase {
   my ($lang, $in, $out, $src, $pal) = @_;
   unless ($tc) {
      copy($in, $out);
   } else {
      my ($lm_sw, $map_sw) = $tctp ? ("tplm", "tppt") : ("lm", "map");
      my $model_opts = "-$lm_sw '${tclm}' -$map_sw '${tcmap}'";
      my $src_opts = defined $tcsrclm ? "-src '${src}' -pal '${pal}' -srclm ${tcsrclm}" : "";
      my $enc = $utf8 ? "utf-8" : "cp1252";
      my $v = $verbose ? "-verbose" : "";
      my $d = $debug ? "-debug" : "";
      my $t = $timing ? "-time" : "";
      call("truecase.pl $v $d $t -text '${in}' -bos -enc ${enc} -out '${out}' $model_opts $src_opts", $out);
   }
}

sub call {
   my ($cmd, @outfiles) = @_;
   my $start = time if $timing;
   verbose("[call: %s]\n", $cmd);
   if ($dryrun) {
      print $cmd, "\n";
   } else {
      system($cmd) == 0
         or cleanupAndDie(explainSystemRC($?,$cmd,$0), @outfiles);
   }
   (print STDERR "translate.pl: Running ", (split(' ', $cmd, 2))[0], " took ", time - $start, " seconds.\n") if $timing;
}

sub callOutput {
   my ($cmd) = @_;
    
   my ($fh, $fname) = File::Temp->tempfile("callOutput-XXXXXX", UNLINK=>1);
   close $fh;

   system("$cmd > $fname") == 0 or cleanupAndDie(explainSystemRC($?,$cmd,$0));

   open($fh, "<$fname") or cleanupAndDie("Can't open temp file $fname for reading.\n");
   my @cmdout = <$fh>;
   close $fh;

   my $cmdout = join("", @cmdout);
   chomp $cmdout;

   return $cmdout;
}

sub cleanupAndDie {
   my ($message, @files) = @_;

   plogUpdate($plog_file, 'failure');
   if ($quiet) {
      # Notify the user via the original STDERR.
      print SAVE_STDERR "ERROR: ", $message;
      print SAVE_STDERR "See '${dir}/log.translate.pl' for details.\n";      
   }
   unlink @files;
   die "ERROR: ", $message;
}

sub verbose { printf STDERR @_ if $verbose; }
