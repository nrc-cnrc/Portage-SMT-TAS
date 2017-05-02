#!/usr/bin/env perl

# @file translate.pl
# @brief Script to translate text.
#
# @author Darlene Stewart & Samuel Larkin (adapted from Michel Simard's ce_translate.pl)
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010-2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010-2013, Her Majesty in Right of Canada

=pod

=head1 NAME

translate.pl - Translate text

=head1 SYNOPSIS

 translate.pl -decode-only|-with-rescoring|-with-ce [options]
              [-f CANOE_INI] [SRC_TEXT]

=head1 DESCRIPTION

This program translates a source text file SRC_TEXT to the target language
according to the current trained models. If SRC_TEXT is not specified, the
source text is read from standard input (STDIN). With the C<-xml> option,
SRC_TEXT is a TMX format file from which the source text is extracted and to
which the translated text is replaced.

By default, the program creates a temporary working directory, where it stores
intermediate results, including tokenized and lowercased versions of the text
files, PortageII translations, and for C<-with-ce>, value files for the
features used by the CE model. Normally this temporary directory is deleted at
the end of the process, unless something goes wrong or if the C<-dir=D>option
or the C<-d> option is specified.

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

newline marks the end of a sentence [default with -notok or -xml];

=item -nl=w

two consecutive newlines mark the end of a paragraph, otherwise newline is just
whitespace (like a wrap marker) [general default].

=back

=item -[no]lc

Lowercase the input text. Use --nolc to skip lowercasing. [lc]

=item -[no]detok

Detokenize the text on output. Use --nodetok to skip detokenization. [detok]

=item -[no]tc | -tctp | -tc-plugin

If -tc, truecase the text on output using a text LM and map for the truecasing
model.
If -tctp, truecase using tightly packed LMs and map for the truecasing model.
For -tc or -tctp, the LM and map files, if not specified using -tclm and -tcmap,
are searched for in F<models/tc> relative to the F<canoe.ini> location.
If -notc, skip truecasing.
If -tc-plugin, call truecase_plugin to perform truecasing. This plugin should
be installed in the system's plugins/ directory, not system-wide! It will be
called like this:
   truecase_plugin TGT_LANG < in > out
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

=item -src=SRC_LANG

Use SRC_LANG as the source language. [en]

=item -tgt=TGT_LANG

Use TGT_LANG as the target language. [fr]

=item -src-country=SRC_COUNTRY

Use SRC_COUNTRY as the country code (2 uppercase chars) in the source locale. 
Needed only by truecasing using -tcsrclm. [CA]

=item -encoding=ENC

Use encoding ENC as the encoding of the input (SRC_TEXT) and output (OUTPUT).
Supported encodings are: 'utf-8', 'cp1252'. [utf-8]

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

=head2 XML Specific Options

=over 12

=item -xml

The input and output files are in XML format.

=item -xtags

Process and transfer tags, either in the XML file input, or in plain input text
with XMLish markup.

=item -hashtags

Process hashtags in such a way that the hashtag words get translated.  This
activates -xtags.

=item -wal=WAL

-wal=h: Use heuristic word alignment for -xtags and for -tcsrclm, even if work
alignment is available in the phrase table.  (For baseline evaluation only.)

-wal=pal: Use the word alignment from the a field in the phrase table (via the
PAL file).  In this case, it is an error if the word alignments are missing for
multi-word phrases.

-wal=mixed: Use the word alignment from the a field if found, falling back to
the heuristic otherwise.

[mixed]

=item -xsrc=XSRC

Use XSRC as the XML source language name. [uppercase SRC_LANG-SRC_COUNTRY]

=item -xtgt=XTGT

Use XTGT as the XML target language name. [uppercase TGT_LANG-SRC_COUNTRY]

=back

=head2 Confidence Estimation Specific Options

=over 12

=item -filter=T

Filter out translations below confidence threshold T (valid for C<-xml> mode only).
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
plugins directory specified with the C<-plugins> option.  All plugins
are expected to read from standard input and write to standard output,
and should require no command-line arguments.

=head1 CAVEATS

For -with-rescoring, in order for rat.sh to work correctly, either the
rescoring model must use absolute paths or its model paths must be of the
form C<models/subpath> and be accessible from the current directory
(i.e. C<./models/subpath> references the corresponding model).
This is the way PortageLive and the framework work.

=head1 SEE ALSO

canoe, canoe-parallel.sh, rat.sh, ce.pl, ce_translate.pl

=head1 AUTHOR

Darlene Stewart

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright (c) 2010-2013, Sa Majeste la Reine du Chef du Canada /
 Copyright (c) 2010-2013, Her Majesty in Right of Canada

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
      unshift @INC, "$bin_path/../utils", "$bin_path/../preprocessing", $bin_path;
   }
}

# With ACLs, -x and -r don't work properly unless we use filetest 'access'
# Downside: -x _ and -r _ no longer work properly in this mode, so we must
# repeat the file name (see perldoc filetest for details).
use filetest 'access';

use portage_utils;
printCopyright("translate.pl", 2010);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

use ULexiTools qw(strip_xml_entities tokenize_file detokenize_file);

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
   "help"           => sub { displayHelp(); exit 0 },
   "h"              => sub { displayHelp(); exit 0 },
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
   "tc-plugin"      => \my $tc_plugin,

   "src=s"          => \my $src,
   "tgt=s"          => \my $tgt,

   "src-country=s"  => \my $src_country,
   "encoding=s"     => \my $encoding,

   "out=s"          => \my $out,

   "dir=s"          => \my $dir,
   "plugins=s"      => \my $plugins,

   "xtra-decode-opts=s"  => \my $xtra_decode_opts,
   "xtra-cp-opts=s"      => \my $xtra_cp_opts,
   "xtra-rat-opts=s"     => \my $xtra_rat_opts,

   #XML specific options
   "xml"            => \my $xml,
   "xtags"          => \my $xtags,
   "hashtags"       => \my $hashtags,
   "wal=s"          => \my $wal,
   "xsrc=s"         => \my $xsrc,
   "xtgt=s"         => \my $xtgt,

   # JSON
   "json-output"    => \my $json_output,

   #CE specific options
   "filter=f"       => \my $filter,
   "xtra-ce-opts=s" => \my $xtra_ce_opts,

   #Development options
   "skipto=s"       => \my $skipto,
) or (print(STDERR "Error: translate.pl aborted due to bad option.\nRun with -h for help.\n"), exit 1);

$quiet = 0 unless defined $quiet;
$debug = 0 unless defined $debug;
$dryrun = 0 unless defined $dryrun;
$timing = 0 unless defined $timing;
$xtags = 0 unless defined $xtags;

# To process hashtags, we use the xtags mechanism.
$xtags = 1 if defined $hashtags;

$wal = "mixed" unless defined $wal;
$wal eq "h" or $wal eq "pal" or $wal eq "mixed"
   or die "Error: unrecognized value for -wal: $wal; valid values are h, pal, and mixed\n";

if ( !$quiet || $verbose ) {
   print STDERR "$saved_command_line\n\n";
}

$decode_only = 0 unless defined $decode_only;
$with_rescoring = 0 unless defined $with_rescoring;
$with_ce = 0 unless defined $with_ce;
$decode_only + $with_rescoring + $with_ce == 1
   or die "Error: Missing or extra switch; specify exactly one of: ",
          "-decode-only, -with-rescoring, -with-ce.\nStopped";

$n = 1 unless defined $n;
$n >= 1 or die "Error: n must be a positive integer (n-ways-parallel)";
$n > 1 or !defined $xtra_cp_opts
   or die "Error: -xtra-cp-opts is valid only when using canoe parallel (n>1).\nStopped";

unless (defined $canoe_ini) {
   $canoe_ini = "canoe.ini.cow";
   $canoe_ini = "$ENV{PORTAGE}/models/canoe.ini.cow" unless -f $canoe_ini;
   -f $canoe_ini or die "Error: Unable to locate a canoe.ini.cow file; ",
                        "use -f or -ini to specify the canoe.ini file.\nStopped"
}
-f $canoe_ini && -r $canoe_ini or die "Error: canoe.ini file '$canoe_ini' is not a readable file.\nStopped";

my $models_dir = dirname($canoe_ini);
-d $models_dir or die "Error: models directory '$models_dir' is not a readable directory.\nStopped";

# Locate the rescoring or CE model, if needed.
my ($rs_model, $ce_model) = ("", "");
if ($with_rescoring) {
   # Locate the rescoring model.
   if (defined $model) {
      $rs_model = $model
   }
   else {
      my $rescoring_dir = "$models_dir/models/rescore";
      my @files = grep !(/\.ini$/ || /\.template$/), glob "$rescoring_dir/rescore-model*";
      @files > 0 or die "Error: Unable to locate a rescore-model file in '$rescoring_dir'; ",
                        -d $rescoring_dir ? "" : "'$rescoring_dir' does not exist; ",
                        "use -model to specify the rescoring model.\nStopped";
      @files == 1 or die "Error: Found multiple rescore-model files in '$rescoring_dir'; ",
                         "use -model to specify the rescoring model.\nStopped";
      $rs_model = shift @files;
   }
   -f $rs_model && -r $rs_model or die "Error: Rescoring model '$rs_model' is not a readable file.\nStopped";
} elsif ($with_ce) {
   # Locate the CE model.
   if (defined $model) {
      $ce_model = $model
   }
   else {
      my @files = grep !/^.*\/log\.[^\/]+/, glob "$models_dir/*.cem";
      @files > 0 or die "Error: Unable to locate a .cem file in '$models_dir'; ",
                        "use -model to specify the CE model.\nStopped";
      @files == 1 or die "Error: Found multiple .cem files in '$models_dir'; ",
                         "use -model to specify the CE model.\nStopped";
      $ce_model = shift @files;
   }
   -f $ce_model && -r $ce_model or die "Error: CE model '$ce_model' is not a readable file.\nStopped";
}
else {
   !defined $model or die "Error: -model option is invalid with -decode-only; "
                        . "use -f or -ini to specify the canoe.ini file.\nStopped";
}

$src = "en" unless defined $src;
$tgt = "fr" unless defined $tgt;

$src_country = "CA" unless defined $src_country;
$src_country = uc $src_country;

$tok = 1 unless defined $tok;
$lc = 1 unless defined $lc;
$detok = 1 unless defined $detok;

$nl = ($xml || !$tok ? "s" : "w") unless defined $nl;
$nl eq "w" or $nl eq "s" or $nl eq "p"
   or die "Error: -nl option must be one of: 's', 'p', 'w', or ''.\nStopped";
#$tok or $nl eq "s"
#   or die "Error: -notok requires -nl=s to be specified.\nStopped";
!$xml or $nl eq "s"
   or die "Error: -xml requires -nl=s.\nStopped";

!defined $tc || !defined $tctp
   or die "Error: Specify only one of: -notc, -tc, -tctp, -tc-plugin.\nStopped";
!defined $tc_plugin || !defined $tc && !defined $tctp
   or die "Error: Specify only one of: -notc, -tc, -tctp, -tc-plugin.\nStopped";
$tc = 0 unless defined $tc;
$tctp = 0 unless defined $tctp;
$tc = 1 if $tctp;
$tc or (!defined $tclm && !defined $tcmap && !defined $tcsrclm)
   or die "Error: Do not specify -tclm, -tctp or -tcsrclm with -notc.\nStopped";
(defined $tclm && defined $tcmap) or (!defined $tclm && !defined $tcmap)
   or die "Error: Specify neither or both of -tclm and -tcmap.\nStopped";
(defined $tcsrclm && defined $tclm) or (!defined $tcsrclm)
   or die "Error: Do not specify -tcsrclm without -tclm specified.\nStopped";
!defined $tcsrclm or !$with_rescoring
   or die "Error: -tcsrclm cannot be used with -with-rescoring.\nStopped";

my $python_version = `python --version 2>&1`;
if ($python_version !~ /2\.7/) {
   chomp $python_version;
   die "Error: translate.pl requires Python 2.7. ",
       "Found $python_version instead. ", "Please check your installation.\n",
       "If you see this message in PortageLive's trace, place a symlink to the 2.7 python executable ",
       "in $ENV{PORTAGE}/bin/ and symlinks to libpython2.7.so* in $ENV{PORTAGE}/lib/, ",
       "and make sure the Apache process has sufficient permissions to use them.\n";
}
if ($python_version =~ /(libpython\S*):/) {
   print STDERR "translate.pl: $python_version";
   die "Error: translate.pl requires Python 2.7 and its library $1.\n",
       "If you see this message in PortageLive's trace, place a symlink to $1 in $ENV{PORTAGE}/lib/.\n";
}

# Locate the Truecasing model.
if ($tc and !defined $tclm) {
   my @tc_files = ();
   my $tc_dir = "$models_dir/models/tc";
   my $use_msg = "use -tclm, -tcmap and -tcsrclm to specify the truecasing files.\nStopped";
   -d $tc_dir
      or die "Error: '$tc_dir' does not exist; ", $use_msg;
   foreach my $ext ($tctp ? (".tplm", ".tppt") : (".binlm.gz", ".map")) {
      my @files = grep !/\/log\.[^\/]+$/ && !/[\/.-_]nc1[\.-_][^\/]+$/,
                  glob "$tc_dir/*{.,-,_}$tgt*$ext";
      @files > 0
         or die "Error: Unable to locate a $tgt TC $ext file in '$tc_dir'; ",
                "perhaps you need the " , $tctp ? "-tc" : "-tctp",
                " option instead of ", $tctp ? "-tctp" : "-tc",
                ".\nStopped";
      @files == 1
         or die "Error: Found multiple $tgt TC $ext files in '$tc_dir'; ", $use_msg;
      push @tc_files, @files;
   }
   ($tclm, $tcmap) = @tc_files;
   unless ($with_rescoring) {
      # New truecasing workflow using source info currently not compatible with rescoring.
      my $ext = $tctp ? ".tplm" : ".binlm.gz";
      my @files = grep !/\/log\.[^\/]+$/ && /[\/.\-_]nc1[\.\-_][^\/]+$/,
                  glob "$tc_dir/*{.,-,_}$src*$ext";
      @files <= 1
         or die "Error: Found multiple $src NC1 $ext files in '$tc_dir'; ", $use_msg;
      $tcsrclm = $files[0] unless @files == 0;
   }
}
if ($tc) {
   if ($tctp) {
      -d $tclm && -x $tclm
         or die "Error: Tightly packed truecasing $tgt model '$tclm' ",
                "is not a readable directory.\nStopped";
      -d $tcmap && -x $tcmap
         or die "Error: Tightly packed truecasing map '$tcmap' ",
                "is not a readable directory.\nStopped";
   }
   else {
      -f $tclm && -r $tclm
         or die "Error: Truecasing $tgt model '$tclm' is not a readable file.\nStopped";
      -f $tcmap && -r $tcmap
         or die "Error: Truecasing map '$tcmap' is not a readable file.\nStopped";
   }
   if (defined $tcsrclm) {
      if ($tcsrclm =~ /.tplm$/) {
         -d $tcsrclm && -x $tcsrclm
            or die "Error: Tightly packed truecasing $src model '$tcsrclm' ",
                   "is not a readable directory.\nStopped";
      }
      else {
         -f $tcsrclm && -r $tcsrclm
            or die "Error: Truecasing $src model '$tcsrclm' is not a readable file.\nStopped";
      }
   }
}

my $utf8 = 1;
if (defined $encoding) {
   my $lc_enc = lc $encoding;
   $utf8 = $lc_enc eq "utf8" || $lc_enc eq "utf-8";
   $utf8 or $lc_enc eq "cp1252"
      or die "Error: -encoding must be one of: 'utf8' or 'cp1252'.\nStopped";
}
else {
   $encoding = "utf-8";
}
if ($xtags && !$utf8) {
   die "Error: -xtags is not compatible with encoding $encoding: only utf8 is supported";
}

my $src_locale;
if (defined $tcsrclm) {
   $src_locale = "${src}_${src_country}.${encoding}";
   system("perl", "-e", "use POSIX qw(locale_h); exit 1 unless defined setlocale(LC_CTYPE,q($src_locale));") == 0
      or die "Error: Invalid locale '$src_locale'; ",
             "check -src (${src}), -src-country (${src_country}), -encoding (${encoding}); ",
             "if correct, locale '$src_locale' needs to be installed.\nStopped";
}

# Locate the Plugins directory
my $plugins_dir = defined $plugins ? $plugins : "$models_dir/plugins";
if (defined $plugins) {
   -d $plugins_dir
      or warn "Warning: plugins directory '$plugins_dir' is not a readable directory";
}

$decode_only or $with_ce or !defined $xtra_decode_opts
   or die "Error: -xtra-decode-opts is valid only with -decode-only or -with-ce.\nStopped";
$with_rescoring or !defined $xtra_rat_opts
   or die "Error: -xtra-rat-opts is valid only with -with-rescoring.\nStopped";
$xtra_cp_opts = "" unless defined $xtra_cp_opts;
$xtra_decode_opts = "" unless defined $xtra_decode_opts;
$xtra_rat_opts = "" unless defined $xtra_rat_opts;

# XML specific options
$xml = 0 unless defined $xml;
if ($xml) {
   if (!defined $xsrc) {
      $xsrc = "$src-$src_country";
      $xsrc =~ tr/a-z/A-Z/;
   }
   if (!defined $xtgt) {
      $xtgt = "$tgt-$src_country";
      $xtgt =~ tr/a-z/A-Z/;
   }
}
else {
   !defined $xsrc and !defined $xtgt and !defined $filter
      or warn "Warning: ignoring -xsrc, -xtgt and -filter, which are meaningful only with -xml.\n";
}

# CE specific options
if ($with_ce) {
   $xtra_ce_opts = "" unless defined $xtra_ce_opts;
}
else {
   !defined $filter
      or die "Error: -filter is valid only with -with-ce.\nStopped";
   !defined $xtra_ce_opts
      or die "Error: -xtra-ce-opts is valid only with -with-ce.\nStopped";
}

@ARGV <= 1 or die "Error: Too many arguments.\nStopped";
@ARGV > 0 or die "Error: Too few arguments. SRC_TEXT file required.\nStopped" if $xml;
my $input_text = @ARGV > 0 ? shift : "-";

unless (defined $out) {
   $out = "-";
}
else {
   system("echo '' >$out") == 0
      or die "Error: '$out' is not a writable file.\nStopped";
}

# Make working directory and the log file.
my $keep_dir = $dir;
my $plog_file;

open(SAVE_STDERR, ">&STDERR");   # later, STDERR may be redirected to a log file.

if ($dryrun) {
   $dir = "translate_work_temp_dir" unless $dir;
} elsif ($skipto) {
   $dir or die "Error: Use -dir with -skipto.\nStopped";
   -d $dir or die "Error: Unreadable directory '$dir' with -skipto.\nStopped";
   if ($quiet) {
      # Make terminal output as quiet as possible by redirecting STDERR.
      open(STDERR, ">>", "${dir}/log.translate.pl");
      print STDERR "\n---------- " . localtime() . "skipto $skipto ----------\n"
         or warn "Warning: Unable to redirect STDERR to append to '${dir}/log.translate.pl'";
   }
}
else {
   if ($dir) {
      if (not -d $dir) {
         mkdir $dir or die "Error: Can't make directory '$dir': errno=$!.\nStopped";
      }
   }
   else {
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
         or warn "Warning: Unable to redirect STDERR to '${dir}/log.translate.pl'";
   }
   $plog_file = plogCreate("File:${input_text}; Context:".File::Spec->rel2abs($models_dir));
}

# Past this point, do not call die() directly; call cleanupAndDie() instead.

# Further down, we assume $dir is an absolute path, in particular when decoding
# in parallel.
$dir = Cwd::realpath($dir);

$keep_dir = $dir if $debug;
verbose("Work directory: \"$dir\"");
verbose("models_dir: \"$models_dir\" (" . Cwd::realpath($models_dir) . ")");
verbose("Processing input text \"${input_text}\"");

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
      ".oppl = detokenized translation returned to one-paragraph per line\n",
      "pr.ce = confidence estimation\n",
      "\n",
      "Final translation output is in P.txt\n",
   );
   close README;
}

# Let's provide the plugins the location of the working directory if they ever
# need to create intermediate files.
$ENV{PORTAGELIVE_WORKDIR} = $dir;

# File names - the naming scheme required for use with CE is adhered to.
my $ostype = `uname -s`;
chomp $ostype;
my $ci = ($ostype eq "Darwin") ? "l" : ""; # for case insensitive file systems

my $Q_txt  = "${dir}/Q.txt";     # Raw source text
my $Q_tags = "${dir}/Q.tags";     # Raw source text with tags
# --> preprocessor plugin
my $Q_pre = "${dir}/Q.pre";     # Pre-processed (pre-tokenization) source
# --> tokenize
my $Q_tok = "${dir}/Q.tok";     # Tokenized source
my $Q_tok_tags_hashtags = "${dir}/Q.tok.tags.hashtags";
my $Q_tok_tags = "${dir}/Q.tok.tags";     # Tokenized source with tags
# --> lowercase
my $q_tok = "${dir}/q${ci}.tok";    # Lowercased tokenized source
# --> predecoder plugin
my $q_dec = "${dir}/q.dec";     # Decoder-ready source
# --> decoding
my $p_raw = "${dir}/p.raw";     # Raw decoder output (with trace)
# --> decoder output parsing
my $p_decoov = "${dir}/p.dec.oov";  # Raw decoder translation with OOV markup
my $p_json = "${dir}/p.json";  # pal + source info in a json format for phraseAlignment.html.
my $oov_html = "${dir}/oov.html";  # Html page with highlighted OOVs.
my $p_dec = "${dir}/p.dec";     # Raw decoder translation (OOV markup removed)
my $p_pal = "${dir}/p.pal";     # Phrase alignments used with truecasing
# --> postdecoder plugin
my $p_tokoov = "${dir}/p.tok.oov";  # Post-decoder plugin processed translation (with OOV markup)
my $p_tok = "${dir}/p${ci}.tok";    # Post-decoder plugin processed translation (OOV markup removed)
# --> truecasing
my $P_tok = "${dir}/P.tok";     # Truecased tokenized translation
my $P_tok_tags = "${dir}/P.tok.tags";     # Truecased tokenized translation with tags
# --> detokenization
my $P_dtk = "${dir}/P.dtk";     # Truecased detokenized translation
my $P_oppl = "${dir}/P.oppl";   # Truecased, detok'd, oppl'd translation
# --> postprocessor plugin
my $P_hashtagify = "${dir}/P.hashtagify";     # Restore hashtags.
my $P_txt = "${dir}/P.txt";     # translation

#Others:
my $pr_ce = "${dir}/pr.ce";     # confidence estimation
my $Q_filt = "${dir}/Q.filt";   # post-filtering source

# Skipto option
goto $skipto if $skipto;

# Get source text
IN:{
   if ($xml) {
      call("ce_tmx.pl -verbose=$verbose -src=$xsrc -tgt=$xtgt extract '$dir' '$input_text'");
      # $Q_txt can be empty if there is nothing in the original document.
      cleanupAndDie("XML file $input_text has no sentences in language $xsrc.\n") unless -e $Q_txt or $dryrun;
   }
   else {
      if ($xtags) {
         copy($input_text, $Q_tags);
      }
      else {
         copy($input_text, $Q_txt);
      }
   }
}

# Preprocess, tokenize and lowercase
PREP:{
   if ($xtags) {
      plugin("preprocess", $src, $Q_tags, $Q_pre);
      my $in = $Q_pre;
      if ($hashtags) {
         tokenize_hashtags($Q_pre, $Q_tok_tags_hashtags);
         $in = $Q_tok_tags_hashtags;
      }
      tokenize($src, $in, $Q_tok_tags);
      strip_entity($Q_tok_tags, $Q_tok);
   }
   else {
      plugin("preprocess", $src, $Q_txt, $Q_pre);
      tokenize($src, $Q_pre, $Q_tok);
   }
   lowercase($Q_tok, $q_tok);
}

# Translate
TRANS:{
   plugin("predecode", "$src-$tgt", $q_tok, $q_dec);

   if (defined $w and $n > 1) {
      my $sent_count = `wc -l < $q_dec` + 0;
      if ($w * $n > $sent_count) {
         $n = int($sent_count / $w);
         $n > 0 or $n = 1;
      }
   }

   # We need to modify our paths to run in the working directory.
   if ($canoe_ini !~ /^\//) {
      $dir =~ /^\//;
      $canoe_ini = ($&) ? Cwd::realpath($canoe_ini) : "../$canoe_ini";
      if ($with_rescoring) {
         $rs_model = ($&) ? Cwd::realpath($rs_model) : "../$rs_model";
      }
   }

   unless ($with_rescoring) {
      my $decoder = "canoe";
      if ($n > 1) {
         my $v = $verbose ? "-v" : "";
         $decoder = "cd $dir && canoe-parallel.sh $v -n $n $xtra_cp_opts canoe";
      }
      my $decoder_opts = $verbose ? "-v $verbose" : "";
      $decoder_opts .= " -quiet-empty-lines";
      $decoder_opts .= " -walign -palign";
      $decoder_opts .= " -ffvals" if $with_ce;
      $decoder_opts .= " $xtra_decode_opts";
      my $decoder_log = ($verbose > 1) ? "2> '${dir}/log.decode'" : "";
      call("$decoder $decoder_opts -f ${canoe_ini} < '${q_dec}' > '${p_raw}' ${decoder_log}");

      my $wal_opt = ($wal eq "h") ? "" : "-wal";
      call("set -o pipefail;" .
           " nbest2rescore.pl -canoe -tagoov -oov $wal_opt -palout='${p_pal}' < '${p_raw}'" .
           " | perl -pe 's/ +\$//;' > '${p_decoov}'");

      generateOOVsPage(${p_decoov}, ${oov_html});

      if ($with_ce) {
         # ce_canoe2ffvals.pl generates $p_dec from $p_raw, among other things
         call("ce_canoe2ffvals.pl -verbose=${verbose} -dir='${dir}' '${p_raw}'");
      }
   }
   else {  # $with_rescoring
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

      my $cwd = cwd();
      chdir($dir);
      my $cp_opts = "-n $n $xtra_cp_opts";
      my $rat_opts = $verbose ? "-v" : "";
      $rat_opts .= " $xtra_rat_opts";
      call("rat.sh $cp_opts trans $rat_opts -msrc q.dec -f ${canoe_ini} ${rs_model} q.tok");
      rename("q.dec.rat", "p.dec");
      chdir($cwd);
   }

   plugin("postdecode", $tgt, $p_decoov, $p_tokoov);
   call("perl -pe 's/<OOV>(.+?)<\\/OOV>/\\1/g;' < '${p_tokoov}' > '${p_tok}'");
}

# Truecase, detokenize, and postprocess
POST:{
   my $in = defined $tcsrclm ? $p_tokoov : $p_tok;

   # lang, in, out, source, pal
   truecase($tgt, $in, $P_tok, $Q_tok, $p_pal);
   $in = $P_tok;

   # Transfer tags from source to target.
   if ($xtags) {
      my $markup_verbose = $verbose > 2 ? "-vv" : $verbose == 2 ? "-v" : "";
      my $markup_log = $markup_verbose ? "2> '${dir}/log.markup'" : "";
      call("markup_canoe_output $markup_verbose -wal $wal -xtags $Q_tok_tags $in $p_pal > $P_tok_tags $markup_log");
      $in = $P_tok_tags;
   }
   else {
      if ($xml) {
         my $P_ee = "${dir}/P.ee";
         escape_entity($in, $P_ee);
         $in = $P_ee;
      }
   }

   detokenize($tgt, $in, $P_dtk);

   # Reconstruct paragraphs if necessary.
   if ($nl eq "p") {
      deparaline($P_dtk, $P_oppl);
      $in = $P_oppl;
   }
   else {
      $in = $P_dtk;
   }

   if ($hashtags) {
      reconstructHashtags($in, $P_hashtagify);
      $in = $P_hashtagify;
   }
   plugin("postprocess", $tgt, $in, $P_txt);

   # Generate pal.html's data.
   unless ($with_rescoring) {
      call("nbest2rescore.pl -canoe -source='${Q_tok}' -target='${P_tok}' -json='${p_json}' < '${p_raw}' > /dev/null");
      symlink(${dir}."/../../phraseAlignmentVisualization.html", ${dir}."/pal.html");
   }
}

# Predict CE
CE:{
   if ($with_ce) {
      call("ce.pl -verbose=${verbose} ${xtra_ce_opts} ${ce_model} '${dir}'");
   }
}

# Produce output
OUT:{
   unless ($xml) {
      if ($with_ce) {
         my $ce_output = $out ne "-" ? "> '$out'" : "";
         call("paste ${dir}/pr.ce '${P_txt}' ${ce_output}");
      }
      else {
         # For now this mode only works if there is an one-sentence-per-line file.
         if ($json_output and -e "$Q_pre.ospl") {
            outputJson("$Q_pre.ospl", $P_txt, $out);
         }
         else {
            copy($P_txt, $out);
         }
      }
   }
   else {
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
        verbose("Cleaning up and deleting work directory $dir");
        rmtree($dir);
    }
}

(print STDERR "translate.pl: translation workflow took ", time - $start_time, " seconds.\n") if $timing;
exit 0; # All done!


## Subroutines

sub displayHelp {
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
   my $old_perl5lib_path = $ENV{PERL5LIB};
   $ENV{PERL5LIB} = "${plugins_dir}:".$ENV{PERL5LIB};
   my $actual_prog = `which ${name}_plugin`;
   chomp($actual_prog);
   print STDERR "Using plugin: ${actual_prog}\n";
   call("${actual_prog} ${lang} < '${in}' > '${out}'", $out);
   $ENV{PATH} = $old_path;
   $ENV{PERL5LIB} = $old_perl5lib_path;
}



sub plogCreate {
   my ($job_name, $comment) = @_;
   my $plog_file;

   if ($dryrun) {
      $plog_file = "dummy-log";
   }
   else {
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
   system($cmd) == 0 or warn "Warning: ", explainSystemRC($?,$cmd,$0);
}



sub sourceWordCount {
   my ($filter) = @_;
   return 0 if $dryrun;
   my $count_file;
   if (defined $filter) {
       open(my $tfh, "<${Q_pre}") or cleanupAndDie("Can't open text file ${Q_pre}");
       open(my $cfh, "<${pr_ce}") or cleanupAndDie("Can't open CE file ${pr_ce}");
       open(my $ffh, ">${Q_filt}") or cleanupAndDie("Can't open filtered source ${Q_filt}");

       while (my $t = <$tfh>) {
           my $y = readline($cfh);
           cleanupAndDie("Not enough lines in CE file ${pr_ce}") unless defined $y;
           print {$ffh} $t unless ($y < $filter);
       }
       warn "Warning: Too many lines in CE file ${pr_ce}" if readline($cfh);

       close $tfh;
       close $cfh;
       close $ffh;

       $count_file = $Q_filt;
   }
   else {
       $count_file = $Q_pre;
   }

   cleanupAndDie("Can't open temp file ${count_file} for reading.\n") unless (-r "${count_file}");

   if ($utf8) {
      open(COUNTSRC, "< :encoding(utf-8)", $count_file)
         or cleanupAndDie("Can't open source-word counting file $count_file for reading.\n");
      my $word_count = 0;
      # We follow MS Word: each asian character (Chinese, Japanese, Korean) is
      # counted as an individual word.  For other languages, we consider
      # whitespace as the word boundary.
      while (<COUNTSRC>) {
         s/(\p{Han}|
            \p{Hangul}|
            \p{Block: CJKUnifiedIdeographs}|
            \p{Block: CJKSymbolsAndPunctuation}|
            \p{Block: Katakana}|
            \p{Block: Katakana_Phonetic_Extensions}|
            \p{Block: Hiragana}
           )/ $1 /gx;
         $word_count += scalar(my @w = split);
      }
      return $word_count;
   }
   else {
      my $cmd = "wc -w < '${count_file}'";
      return 0 + `$cmd`; # Add "0 +" to return a number, not a string with a newline.
   }
}



sub tokenize {
   my ($lang, $in, $out) = @_;
   if (!$tok and $nl eq 's') {
      copy($in, $out);
   }
   else {
      if ($lang eq "en" or $lang eq "fr" or $lang eq "es" or $lang eq "da") {
         # These languages are supported by utokenize.pl
         if ($utf8) {
            my $start = time if $timing;
            my $ss = ($nl ne "s");
            verbose("call: tokenize_file($in, $out, $lang, nl=$nl, xtags=$xtags)");
            if (!$dryrun) {
               tokenize_file(
                   $in,
                   $out,
                   $lang,
                   0,          # $v
                   $ss,        # $p
                   $ss,        # $ss
                   !$ss,       # $noss
                   0,          # $notok
                   !$tok,      # $pretok
                   $nl eq "p", # $paraline
                   $xtags      # $xtags
               ) == 0
                  or cleanupAndDie("error calling tokenize_file()", $out);
               (print STDERR "translate.pl: Running tokenize_file() took ", time - $start, " seconds.\n") if $timing;
            }
         } else {
            my $tokopt = " -lang=${lang}";
            for ($nl) {
               $_ eq "s" and $tokopt .= " -noss";
               $_ eq "w" and $tokopt .= " -ss -p";
               $_ eq "p" and $tokopt .= " -ss -paraline -p";
            }
            $tokopt .= " -pretok" if !$tok;
            $tokopt .= " -xtags" if $xtags;
            my $u = $utf8 ? "u" : "";
            call("${u}tokenize.pl ${tokopt} '${in}' '${out}'", $out);
         }
      }
      else {
         # Other languages must provide sentsplit_plugin and tokenize_plugin.
         my $tok_input = $nl ne 's' ? "$in.ospl" : $in;
         my $ss_output = $tok ? $tok_input : $out;
         if ($nl ne 's') {
            my $ss_input = $nl eq 'w' ? "$in.oppl" : $in;
            if ($nl eq 'w') {
               open(PARA_INPUT, "< :encoding(utf-8)", $in)
                  or cleanupAndDie("Can't open $in for reading.\n");
               open(PARA_OUTPUT, "> :encoding(utf-8)", $ss_input)
                  or cleanupAndDie("Can't open $ss_input for writing.\n");
               require ULexiTools;
               while ($_ = ULexiTools::get_para(\*PARA_INPUT, 0)) {
                  next if /^\s*$/;
                  s/\s+/ /g;
                  s/\s*\z/\n/s;
                  print PARA_OUTPUT;
               }
               close PARA_INPUT;
               close PARA_OUTPUT;
            }
            if ($xtags) {
               call("sentsplit-with-tags-split.pl $ss_input $ss_output-text $ss_output-tags", "$ss_output-text", "$ss_output-tags");
               plugin("sentsplit", $src, "$ss_output-text", "$ss_output-textss");
               call("sentsplit-with-tags-combine.pl $ss_output-textss $ss_output-tags $ss_output", $ss_output);
            }
            else {
               plugin("sentsplit", $src, $ss_input, $ss_output);
            }
         }
         if ($tok) {
            if ($xtags) {
               call("tok-with-tags-split.pl $tok_input $out-text $out-tags", "$out-text", "$out-tags");
               plugin("tokenize", $src, "$out-text", "$out-texttok");
               call("tok-with-tags-combine.pl $out-texttok $out-tags $out", $out);
            }
            else {
               plugin("tokenize", $src, $tok_input, $out);
            }
         }
      }
   }
}



sub strip_entity {
   my ($in, $out) = @_;
   die "Error: You need to provide in and out" unless (defined($in) and defined($out));

   verbose("Stripping Entities");

   return if $dryrun;

   open(IN, "< :encoding(utf-8)", $in)
      or cleanupAndDie("Can't open $in for reading.\n");
   open(OUT, "> :encoding(utf-8)", $out)
      or cleanupAndDie("Can't open $out for writing.\n");

   while (<IN>) {
      strip_xml_entities($_);
      print OUT $_;
   }
   close(IN);
   close(OUT);
}



sub escape_entity {
   my ($in, $out) = @_;
   warn "Warning: escape_entity should be used in xml mode." unless ($xml);
   die "Error: You need to provide in and out" unless (defined($in) and defined($out));

   verbose("Escaping Entities");

   return if $dryrun;

   open(IN, "< :encoding(utf-8)", $in)
      or cleanupAndDie("Can't open $in for reading.\n");
   open(OUT, "> :encoding(utf-8)", $out)
      or cleanupAndDie("Can't open $out for writing.\n");

   while (<IN>) {
      s/&/&amp;/g;   # unescape ampersand
      s/>/&gt;/g;    # unescape greater than
      s/</&lt;/g;    # unescape less than
      print OUT $_;
   }
   close(IN);
   close(OUT);
}



sub detokenize {
   my ($lang, $in, $out) = @_;
   unless ($detok) {
      copy($in, $out);
   }
   else {
      my $old_path = $ENV{PATH};
      $ENV{PATH} = "${plugins_dir}:".$ENV{PATH};
      my $old_perl5lib_path = $ENV{PERL5LIB};
      $ENV{PERL5LIB} = "${plugins_dir}:".$ENV{PERL5LIB};
      my $cmd = `which detokenize_plugin 2> /dev/null`;
      $ENV{PATH} = $old_path;
      $ENV{PERL5LIB} = $old_perl5lib_path;
      chomp($cmd);
      if ( $cmd ) {
         plugin("detokenize", $tgt, $in, $out);
      }
      else {
         if ($utf8) {
            my $start = time if $timing;
            verbose("call: detokenize_file($in, $out, $lang)");
            if (!$dryrun) {
               detokenize_file($in, $out, $lang, 0, 0, 0, 0) == 0
                  or cleanupAndDie("error calling detokenize_file()", $out);
               (print STDERR "translate.pl: Running detokenize_file() took ", time - $start, " seconds.\n") if $timing;
            }
         } else {
            my $u = $utf8 ? "u" : "";
            call("${u}detokenize.pl -lang=${lang} < '${in}' > '${out}'", $out);
         }
      }
   }
}



sub reconstructHashtags {
   my ($in, $out) = @_;
   verbose("hashtagify hashtags from $in to $out");
   open IN, $in or cleanupAndDie("Can't open $in for reading: $!");
   binmode(IN, ":encoding(UTF-8)");
   open OUT, ">$out" or cleanupAndDie("Can't open $out for writing: $!", $out);
   binmode(OUT, ":encoding(UTF-8)");

   use hashtags;
   while (<IN>) {
      print OUT hashtagify($_);
   }
   close IN;
   close OUT;
}



sub tokenize_hashtags {
   my ($in, $out) = @_;
   verbose("tokenizing hashtags from $in to $out");
   open IN, $in or cleanupAndDie("Can't open $in for reading: $!");
   binmode(IN, ":encoding(UTF-8)");
   open OUT, ">$out" or cleanupAndDie("Can't open $out for writing: $!", $out);
   binmode(OUT, ":encoding(UTF-8)");

   use hashtags;
   while (<IN>) {
      print OUT markHashTags(tokenizeHashtags($_));
   }
   close IN;
   close OUT;
}



sub deparaline {
   my ($in, $out) = @_;
   verbose("Rebuilding paragraphs from OSPL file $in into OPPL file $out");
   open IN, $in or cleanupAndDie("Can't open $in for reading: $!");
   open OUT, ">$out" or cleanupAndDie("Can't open $out for writing: $!", $out);

   my $empty = '^\s*$'; # keep Eclipse Perl plugin happy by avoiding $/ in code.
   PARA: while (1) {
      $_ = <IN>;
      last if !defined $_;
      chomp;
      print OUT;
      SENT: while (1) {
         $_ = <IN>;
         if (!defined $_) {
            print OUT "\n";
            last PARA;
         }
         chomp;
         last SENT if /${empty}/;
         print OUT " ", $_;
      }
      print OUT "\n";
   }
   close IN;
   close OUT;
}



sub lowercase {
   my ($in, $out) = @_;
   unless ($lc) {
      copy($in, $out);
   }
   else {
      my $lc_prog = $utf8 ? "utf8_casemap -c l" : "lc-latin.pl";
      call("${lc_prog} '${in}' '${out}'", $out);
   }
}



sub truecase {
   my ($lang, $in, $out, $src_file, $pal) = @_;
   if ($tc_plugin) {
      plugin("truecase", $tgt, $in, $out);
   }
   elsif (!$tc) {
      copy($in, $out);
   }
   else {
      my ($lm_sw, $map_sw) = $tctp ? ("tplm", "tppt") : ("lm", "map");
      my $locale_opts = $src_locale ? "-locale ${src_locale}" : "-encoding ${encoding}";
      my $model_opts = "-$lm_sw '${tclm}' -$map_sw '${tcmap}'";
      my $src_opts = defined $tcsrclm ? "-src '${src_file}' -pal '${pal}' -srclm ${tcsrclm}" : "";
      my $enc = $utf8 ? "utf-8" : "cp1252";
      my $v = $verbose ? "-verbose" : "";
      my $d = $debug ? "-debug" : "";
      my $t = $timing ? "-time" : "";
      call("truecase.pl -wal $wal $v $d $t -text '${in}' -bos $locale_opts -out '${out}' $model_opts $src_opts", $out);
   }
}



sub call {
   my ($cmd, @outfiles) = @_;
   my $start = time if $timing;
   verbose("call: $cmd");
   if ($dryrun) {
      print $cmd, "\n";
   }
   else {
      system($cmd) == 0
         or cleanupAndDie(explainSystemRC($?,$cmd,$0), @outfiles);
   }
   (print STDERR "translate.pl: Running ", (split(' ', $cmd, 2))[0], " took ", time - $start, " seconds.\n") if $timing;
}



sub callOutput {
   my ($cmd) = @_;

   my ($fh, $fname) = File::Temp::tempfile("/tmp/callOutput-XXXXXX", UNLINK=>1);
   close $fh;

   system("$cmd > $fname") == 0 or cleanupAndDie(explainSystemRC($?,$cmd,$0));

   open($fh, "<$fname") or cleanupAndDie("Can't open temp file $fname for reading.\n");
   my @cmdout = <$fh>;
   close $fh;

   my $cmdout = join("", @cmdout);
   chomp $cmdout;

   return $cmdout;
}



# Routine to clean up (some) files, update the log file, and die.  Call this
# routine instead of die() once plogCreate() has been called.
sub cleanupAndDie {
   my ($message, @files) = @_;

   plogUpdate($plog_file, 'failure');
   if ($quiet) {
      # Notify the user via the original STDERR.
      print SAVE_STDERR "translate.pl fatal error: ", $message;
      print SAVE_STDERR "See '${dir}/log.translate.pl' for details.\n";
   }
   unlink @files;
   die "translate.pl fatal error: ", $message;
}


sub verbose { print STDERR "[", @_, "]\n" if $verbose; }



sub outputJson {
   my ($file_orig, $file_trans, $file_out) = @_;
   use JSON;
   open(ORIG, "<$file_orig") or die "Error: Can't open $file_orig";
   open(TRANS, "<$file_trans") or die "Error: Can't open $file_trans";
   open(OUT, ">$out") or die "Error: Can't open output $out";
   while (defined(my $orig = <ORIG>) and defined(my $trans = <TRANS>)) {
      chomp($orig);
      chomp($trans);
      # Do not perform UTF-8 encoding.
      print OUT to_json({original => $orig, translation => $trans}), "\n" if ($orig ne '' or $trans ne '');
   }
   # Make sure all content is read from both files.
   die "Error: Original text file too long" if (defined(<ORIG>));
   die "Error: Translation text file too long" if (defined(<ORIG>));
   close(ORIG);
   close(TRANS);
   close(OUT);
}



# Given a file containing sentences marked with OOVs (<OOV>words</OOV>),
# genereate an html page where the OOVs are colored red.
sub generateOOVsPage {
   my ($oov, $html) = @_;
   print STDERR "Generating oov.html\n";

   # TODO: We might want to be less harsh when we can't create the oov.html
   open(OOV, "<${oov}") or die "Error: can't open ${oov} to create the OOV html page. ($!)";
   open(HTML, ">${html}")   or die "Error: can't open ${html} to create the OOV html page. ($!)";

   print HTML "<!DOCTYPE html>\n<html>\n<head><style>.OOV { color: red;} </style></head>\n<body>\n";
   while (<OOV>) {
      s/&/\&amp;/g;
      s/</\&lt;/g;
      s/>/\&gt;/g;
      s/&lt;OOV&gt;/<span class="OOV">/g;
      s/&lt;\/OOV&gt;/<\/span>/g;
      s/$/<br \/>/;
      print HTML;
   }
   print HTML '</body></html>';

   close(HTML);
   close(OOV);
}
