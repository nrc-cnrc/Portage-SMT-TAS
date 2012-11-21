#!/usr/bin/perl -s
# $Id$
# @file ce_translate.pl
# @brief Confidence Estimation wrapper program
#
# @author Michel Simard
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

=pod

=head1 SYNOPSIS

Two forms:

 ce_translate.pl {options} canoe_ini ce_model source_text
 ce_translate.pl -train {options} canoe_ini ce_model source_text ref_text

=head1 DESCRIPTION

This program does confidence estimation (CE) for the PORTAGE
system. It can be used to obtain CE values for the translation of the
sentences of the given F<source_text> (first form) or to learn a new
CE model using the F<source_text> and its reference (human)
translation F<ref_text> as training data (second form).

In both forms: F<canoe_ini> is a PORTAGE decoder config file;
F<ce_model> is the name of a CE model file.  These files have a
C<.cem> extension, which may or may not be specified on the command
line.  Argument F<source_text> is a source-language text, in
one-segment-per-line format (but see options C<-tmx>, C<-sdlxliff> and
C<-ttx>).

In "prediction" mode (first form), F<ce_model> must be an existing CE
model file, as produced by program C<ce_train.pl> or this program with
the C<-train> option (second form).  In this mode, for each input line
in F<source_text>, the program outputs a line with two tab-separated
fields: a numerical CE value and the translation of the input.

In "training" mode (second form), the additional argument F<ref_text>
is also expected to be in one-segment-per-line format.  Argument
F<ce_model> must refer to an existing model file, unless option
C<-desc> is used to specify a model description file, from which the
new model will be generated. For a description of the syntax of that
file, do:

  ce.pl -help=desc

By default, the program creates a temporary directory, where it stores
a number of intermediate results, most notably tokenized and
lowercased versions of the text files, PORTAGE translations, and
individual value files for each feature used by the CE model.  The
temporary directory is normally deleted at the end of the process,
unless something goes wrong, or if the C<-dir=D> option is specified.

=head1 OPTIONS

=head2 General Options

=over

=item -dir=D          Store all work files in directory D [default is to use a temporary dir, deleted at the end]

=item -src=S          Source language [en]

=item -tgt=T          Target language [fr]

=item -tmem=T         Use TMem output file T

=item -n=N            Decode N-ways parallel

=item -nolc           Don't lowercase input text

=item -notok          Input text is pre-tokenized (so don't retokenize it!)

=item -nl=X           Newlines in the input text:

=over

=item -nl=p           mark the end of a paragraph;

=item -nl=s           mark the end of a sentence [default if -tmx or -sdlxliff or -notok];

=item -nl=w           two consecutive newlines mark the end of a paragraph, otherwise newline is just whitespace (like wrap marker) [general default].

=back

=item -tclm=F         Perform truecasing on output, using LM F

=item -tcmap=M        Perform truecasing on output, using map M

=item -tctp           Truecasing with tightly packed LM and map

=item -path=P         Set search path for F<ce_model> feature arguments [same directory as F<ce-model>]

=item -plugin=P       Specify plugin directory; see note on plugins below. Default is to look for a directory F<plugins> in the same directory as the F<canoe_ini> file.

=back

=head2 Non-training Options

=over 1

=item -tmx            Input/output text in TMX format (not compatible with C<-train>)

=item -sdlxliff       Input/output text in SDLXLIFF format (not compatible with C<-train>)

=item -ttx            Input text in TTX format, output in plain text (not compatible with C<-train>) See caveats in C<ce_ttx2ospl.pl -help>.

=item -xsrc=S         TTX/TMX source language name [EN-CA]

=item -xtgt=T         TTX/TMX target language name [FR-CA]

=item -out=F          Output CE values to file F [stdout]

=item -filter=T       Filter out translations below confidence threshold T (C<-tmx> or C<-sdlxliff> mode only)

=item -test=R         Compute prediction accuracy statistics based on reference translation file F<R>.

=back


=head2 Training Options

=over 1

=item -train          Training mode

=item -desc=D         Use CE model description file D [none]

=item -k=K            do k-fold cross-validation [don't]

=item -norm=N         use feature normalization N (see C<ce_train.pl -help>)

=back


=head2 Other stuff

=over 1

=item -verbose        Be verbose

=item -debug          Print debugging info

=item -dryrun         Don't do anything, just print the commands

=item -help,-h        Print help message and exit

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
plugins directory specified with the C<-plugin> option.  All plugins
are expected to read from standard input and write to standard output,
and should require no command-line arguments.

=head1 SEE ALSO

ce.pl, ce_train.pl.

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 Copyright 2009, Her Majesty in Right of Canada

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
printCopyright("ce_translate.pl", 2009);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


use File::Temp qw(tempdir);
use File::Path qw(rmtree);
use File::Spec;
use File::Basename;

our($help, $h, $verbose, $debug);
our($desc, $tmem, $train, $plugin,
    $test, $src, $tgt, $tmx, $sdlxliff, $ttx, $xsrc, $xtgt, $k, $norm, $dir, $path,
    $out, $filter, $dryrun, $n, $nl, $notok, $nolc,
    $tclm, $tcmap, $tctp, $skipto);

if ($h or $help) {
    -t STDOUT ? system "pod2usage -verbose 3 $0" : system "pod2text $0";
    exit 0;
}

$verbose = 0 unless defined $verbose;
$debug = 0 unless defined $debug;
$train = 0 unless defined $train;
$ttx = 0 unless defined $ttx;
$tmx = 0 unless defined $tmx;
$sdlxliff = 0 unless defined $sdlxliff;
my $xml = $tmx or $sdlxliff;
$desc = "" unless defined $desc;
$tmem = "" unless defined $tmem;
$test = "" unless defined $test;
$k=undef unless defined $k;
$norm=undef unless defined $norm;
$dir="" unless defined $dir;
$path="" unless defined $path;
$out="" unless defined $out;
$src = "en" unless defined $src;
$tgt = "fr" unless defined $tgt;
$xsrc = "EN-CA" unless defined $xsrc;
$xtgt = "FR-CA" unless defined $xtgt;
$dryrun = 0 unless defined $dryrun;
$n = 0 unless defined $n;
$notok = 0 unless defined $notok;
$nl = ($xml || $notok ? "s" : "w") unless defined $nl;
$nolc = 0 unless defined $nolc;
$tclm = 0 unless defined $tclm;
$tcmap = 0 unless defined $tcmap;
$skipto = "" unless defined $skipto;
$plugin = "" unless defined $plugin;


die "Can't both -train and -test" if $train and $test;
die "Can't train from TMX (yet)" if $train and $tmx;
die "Can't train from SDLXLIFF (yet)" if $train and $sdlxliff;
die "Can't train from TTX (yet)" if $train and $ttx;
die "Can't have both -ttx and (-tmx or -sdlxliff)" if $xml and $ttx;

my $canoe_ini = shift || die "Missing argument: canoe_ini";
my $ce_model = shift || die "Missing argument: ce_model";
my $input_text = shift || die "Missing argument: input_text";
my $ref_text = "";
if ($train) {
    $ref_text = shift || die "Missing argument in training mode: ref_text";
} elsif ($test) {
    $ref_text = $test;
}

verbose("[Processing input text \"${input_text}\"]\n");

# Locate Plugins directory
my $plugin_dir;
if ($plugin) {
    $plugin_dir = $plugin;
} else {
    my ($vol, $dir, undef) = File::Spec->splitpath($canoe_ini);
    $plugin_dir = File::Spec->catpath($vol, $dir, "plugins");
}

# Make working directory

my $keep_dir = $dir;
my $plog_file;

if ($dryrun) {
   $dir = "ce_work_temp_dir" unless $dir;
} elsif ($skipto) {
   die "Use -dir with -skipto" unless $dir;
   die "Unreadable directory $dir with -skipto" unless -d $dir;
} else {
   if ($dir) {
      if (not -d $dir) {
         mkdir $dir or die "ERROR: Can't make directory '$dir': errno=$!.\nStopped";
      }
   } else {
      $dir = "";
      # Use eval to avoid death if unable to create the work directory .
      eval {$dir = tempdir('ce_work_XXXXXX', DIR=>".", CLEANUP=>0);};
      if (not -d $dir) {
         $dir = tempdir('ce_work_XXXXXX', TMPDIR=>1, CLEANUP=>0);
         # Prevent running in cluster mode if using /tmp as the work directory.
         $ENV{PORTAGE_NOCLUSTER} = 1;
      }
   }
   $plog_file = plogCreate("File:${input_text}; Context:".File::Spec->rel2abs(dirname($canoe_ini)))
      unless $train; # Don't log when training
}
$keep_dir = $dir if $debug;   # don't delete the directory in -debug mode.
verbose("[Work directory: \"${dir}\"]\n");

# File names
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
my $p_dec = "${dir}/p.dec";     # Raw decoder translation
# --> postdecoder plugin
my $p_tok = "${dir}/p${ci}.tok";     # Post-processed decoder translation
# --> truecasing
my $P_tok = "${dir}/P.tok";     # Truecased tokenized translation
# --> detokenization
my $P_dtk = "${dir}/P.dtk";     # Truecased detokenized translation
# --> postprocessor plugin
my $P_txt = "${dir}/P.txt";     # translation

my $R_txt = "${dir}/R.txt";     # Raw reference translation
# --> preprocessor plugin
my $R_pre = "${dir}/R.pre";     # Pre-processed (pre-tokenization) ref translation
# --> tokenize
my $R_tok = "${dir}/R.tok";     # Tokenized ref translation
# --> lowercase
my $r_tok = "${dir}/r${ci}.tok";     # Lowercased tokenized ref translation

my $T_txt = "${dir}/T.txt";     # Raw TMem target
# --> preprocessor plugin
my $T_pre = "${dir}/T.pre";     # Pre-processed (pre-tokenization) TMem target
# --> tokenize
my $T_tok = "${dir}/T.tok";     # Tokenized TMem target
# --> lowercase
my $t_tok = "${dir}/t${ci}.tok";     # Lowercased tokenized TMem target

#Others:
my $pr_ce = "${dir}/pr.ce";     # confidence estimation
my $Q_filt = "${dir}/Q.filt";   # post-filtering source


# Skipto option

goto $skipto if $skipto;

# Get source (and possibly reference/tmem target) text

IN:{
   if ($xml) {
      call("ce_tmx.pl -verbose=${verbose} -src=${xsrc} -tgt=${xtgt} extract '$dir' '$input_text'");
   } elsif ($ttx) {
      call("ce_ttx2ospl.pl -verbose=${verbose} -dir=\"${dir}\" -src=${xsrc} -tgt=${xtgt} \"$input_text\"");
   } else {
      copy($input_text, $Q_txt);
      if ($tmem) {
         copy($tmem, $T_txt);
      }
   }

   if ($ref_text) {
      copy($ref_text, $R_txt);
   }
}

# Preprocess, tokenize and lowercase

PREP:{
   plugin("preprocess", $src, $Q_txt, $Q_pre);
   tokenize($src, $Q_pre, $Q_tok);
   lowercase($Q_tok, $q_tok);

   if ($tmem or $ttx) {
      plugin("preprocess", $src, $T_txt, $T_pre);
      tokenize($tgt, $T_pre, $T_tok);
      lowercase($T_tok, $t_tok);
   }

   if ($ref_text) {
      plugin("preprocess", $src, $R_txt, $R_pre);
      tokenize($tgt, $R_pre, $R_tok);
      lowercase($R_tok, $r_tok);
   }
}

# Translate

TRANS:{
   plugin("predecode", $src, $q_tok, $q_dec);
   my $decoder = "canoe";
   if ( $n > 1 ) {
      $decoder = "canoe-parallel.sh -n $n canoe";
   }
   call("$decoder -trace -ffvals -f ${canoe_ini} < \"${q_dec}\" > \"${p_raw}\"");
   call("ce_canoe2ffvals.pl -verbose=${verbose} -dir=\"${dir}\" \"${p_raw}\"");
   # ce_canoe2ffvals.pl generates $p_dec from $p_raw, among other things
   plugin("postdecode", $tgt, $p_dec, $p_tok);
}

POST:{
   truecase($tgt, $p_tok, $P_tok);
   detokenize($tgt, $P_tok, $P_dtk);
   plugin("postprocess", $tgt, $P_dtk, $P_txt);
}

# Train/predict CE

CE:{
   my $ce_opt = "-verbose=${verbose}";
   $ce_opt .= " -path=\"${path}\"" if $path;
   if ($train) {
      $ce_opt .= " -ini=$desc" if $desc;
      $ce_opt .= " -k=$k" if $k;
      $ce_opt .= " -norm=$norm" if $norm;

      call("ce_train.pl ${ce_opt} ${ce_model} \"${dir}\"");
   } else {
      $ce_opt .= " -stats" if $test;
      call("ce.pl ${ce_opt} ${ce_model} \"${dir}\"");
   }
}

# Produce output

OUT:{
   unless ($train) {
      if ($xml) {
         my $fopt = defined $filter ? "-filter=$filter" : "";
         call("ce_tmx.pl -verbose=${verbose} -src=${xsrc} -tgt=${xtgt} -score ${fopt} replace \"$dir\"");
      } else {
         my $ce_output = $out ? "> \"$out\"" : "";
         call("paste ${dir}/pr.ce \"${P_txt}\" ${ce_output}");
      }
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


exit 0;


## Subroutines

sub copy {
   my ($in, $out) = @_;

   call("cp \"${in}\" \"${out}\"", $out);
}

sub plugin {
   my ($name, $lang, $in, $out) = @_;
   my $old_path = $ENV{PATH};
   $ENV{PATH} = "${plugin_dir}:".$ENV{PATH};
   my $actual_prog = callOutput("which ${name}_plugin"); # for the benefit of verbose
   call("${actual_prog} ${lang} < \"${in}\" > \"${out}\"", $out);
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
   return 0 + `$cmd`;
}

sub tokenize {
   my ($lang, $in, $out) = @_;
   if ($notok and $nl eq 's') {
      copy($in, $out);
   }
   else {
      if ($lang eq "en" or $lang eq "fr" or $lang eq "es") {
         # These languages are supported by utokenize.pl
         my $tokopt = " -lang=${lang}";
         $tokopt .= $nl eq 's' ? " -noss" : " -ss";
         $tokopt .= " -paraline" if $nl eq 'p';
         $tokopt .= " -pretok" if $notok;
         call("utokenize.pl ${tokopt} '${in}' '${out}'", $out);
      } else {
         # Other languages must provide sentsplit_plugin and tokenize_plugin.
         my $tok_input = $nl ne 's' ? "$in.ospl" : $in;
         my $ss_output = (!$notok) ? $tok_input : $out;
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
            plugin("sentsplit", $src, $ss_input, $ss_output);
         }
         if (!$notok) {
            plugin("tokenize", $src, $tok_input, $out);
         }
      }
   }
}

sub detokenize {
   my ($lang, $in, $out) = @_;
#     if ($notok) {
#        call("cp '${in}' '${out}'");
#     } else {
   my $old_path = $ENV{PATH};
   $ENV{PATH} = "${plugin_dir}:".$ENV{PATH};
   my $cmd = `which detokenize_plugin 2> /dev/null`;
   $ENV{PATH} = $old_path;
   chomp($cmd);
   if ( $cmd ) {
      plugin("detokenize", $tgt, $P_tok, $P_dtk);
   }
   else {
      call("udetokenize.pl -lang=${lang} < '${in}' > '${out}'", $out);
   }
#    }
}

sub lowercase {
   my ($in, $out) = @_;
   if ($nolc) {
      copy($in, $out);
   } else {
      call("utf8_casemap -c l '${in}' '${out}'", $out);
   }
}

sub truecase {
   my ($lang, $in, $out) = @_;
   if ($tcmap and $tclm) {
      if ($tctp) {
         call("truecase.pl --text='${in}' --ucBOSEncoding=utf8 --tplm=${tclm} --tppt=${tcmap} > '${out}'", $out);
      } else {
         call("truecase.pl --text='${in}' --ucBOSEncoding=utf8 --lm=${tclm} --map=${tcmap} > '${out}'", $out);
      }
   } else {
      copy($in, $out);
   }
}

sub call {
   my ($cmd, @outfiles) = @_;
   verbose("[call: %s]\n", $cmd);
   if ($dryrun) {
      print $cmd, "\n";
   } else {
      system($cmd) == 0
         or cleanupAndDie(explainSystemRC($?,$cmd,$0), @outfiles);
   }
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

   plogUpdate($plog_file, undef, 'failure');
   unlink @files unless $debug;
   die $message;
}

sub verbose { printf STDERR @_ if $verbose; }
