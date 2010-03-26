#!/usr/bin/perl -s -w 
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
one-segment-per-line format (but see options C<-tmx> and C<-ttx>).

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
unless the C<-dir=D> option is specified.

=head1 OPTIONS

=head2 General Options

=over 1

=item -dir=D          Store all work files in directory D [default is to use a temporary dir, deleted at the end]

=item -src=S          Source language [en]

=item -tgt=T          Target language [fr]

=item -tmem=T         Use TMem output file T

=item -ss             Input text is *not* pre-segmented into sentences

=item -notok          Input text is pre-tokenized (cancels -ss)

=item -nolc           Don't lowercase input text

=item -tclm=F         Perform truecasing on output, using LM F

=item -tcmap=M        Perform truecasing on output, using map M

=item -tctp           Truecasing with tightly packed LM and map

=item -path=P         Set search path for F<ce_model> feature arguments [same directory as F<ce-model>]

=back

=head2 Non-training Options

=over 1

=item -tmx            Input/output text in TMX format (not compatible with -train)

=item -ttx            Input/output text in TTX format (not compatible with -train)

=item -xsrc=S         TTX/TMX source language name [EN-CA]

=item -xtgt=T         TTX/TMX target language name [FR-CA]

=item -out=F          Output CE values to file F [stdout]

=item -filter=T       Filter out translations below confidence threshold T (-tmx mode only)

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

=back

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

our ($h, $help, $verbose, $debug, $desc, $tmem, $train,
     $test, $src, $tgt, $tmx, $ttx, $xsrc, $xtgt, $k, $norm, $dir, $path,
     $out, $filter, $dryrun, $ss, $notok, $nolc, 
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
$ss = 0 unless defined $ss;
$notok = 0 unless defined $notok;
$nolc = 0 unless defined $nolc;
$tclm = 0 unless defined $tclm;
$tcmap = 0 unless defined $tcmap;
$skipto = "" unless defined $skipto;

die "Can't both -train and -test" if $train and $test;
die "Can't train from TMX (yet)" if $train and $tmx;
die "Can't train from TTX (yet)" if $train and $ttx;
die "Can't have both -ttx and -tmx" if $tmx and $ttx; 

my $canoe_ini = shift || die "Missing argument: canoe_ini";
my $ce_model = shift || die "Missing argument: ce_model";
my $input_text = shift || die "Missing argument: input_text";
my $ref_text = "";
if ($train) {
    $ref_text = shift || die "Missing argument in training mode: ref_text";
} elsif ($test) {
    $ref_text = $test;
}

my $preprocess = 'perl -pe "s/[<>\\\\\\\\]/\\\\\\\\$&/g"';


# Make working directory

if ($dryrun) {
    $dir = "ce_work_temp_dir" unless $dir;
} elsif ($skipto) {
    die "Use -dir with -skipto" unless $dir;
    die "Unreadable directory $dir with -skipto" unless -d $dir;
} elsif ($dir) {
    if (not -d $dir) {
        mkdir $dir or die "Can't make directory $dir: errno=$!";
    }
} else {
    $dir = tempdir('ce_work_XXXXXX', TMPDIR=>1, CLEANUP=>1);
}

my $Q_txt = "${dir}/Q.txt";
my $Q_tok = "${dir}/Q.tok";
my $q_tok = "${dir}/q.tok";
my $T_txt = "${dir}/T.txt";
my $T_tok = "${dir}/T.tok";
my $t_tok = "${dir}/t.tok";
my $P_txt = "${dir}/P.txt";
my $P_tok = "${dir}/P.tok";
my $p_raw = "${dir}/p.raw";
my $p_tok = "${dir}/p.tok";
my $R_txt = "${dir}/R.txt";
my $R_tok = "${dir}/R.tok";
my $r_tok = "${dir}/r.tok";


goto $skipto if $skipto;

# Get source (and possibly reference/tmem target) text

IN:{
    if ($tmx) {
        call("ce_tmx.pl -verbose=${verbose} -src=${xsrc} -tgt=${xtgt} extract \"$dir\" \"$input_text\"");
    } elsif ($ttx) {
        call("ce_ttx2ospl.pl -verbose=${verbose} -dir=\"${dir}\" -src=${xsrc} -tgt=${xtgt} \"$input_text\"");
    } else {
        call("${preprocess} < \"${input_text}\" > \"${Q_txt}\"");
        if ($tmem) {
            call("${preprocess} < \"${tmem}\" > \"${T_txt}\"");
        }
    }

    if ($ref_text) {
        call("${preprocess} < \"${ref_text}\" > \"${R_txt}\"");
    }
}

# Tokenize and lowercase

PREP:{ 
    tokenize($src, $Q_txt, $Q_tok);
    lowercase($Q_tok, $q_tok);
    
    if ($tmem or $ttx) {
        tokenize($src, $T_txt, $T_tok);
        lowercase($T_tok, $t_tok);
    }
    
    if ($ref_text) {
        tokenize($src, $R_txt, $R_tok);
        lowercase($R_tok, $r_tok);
    }
}

# Translate

TRANS:{
    call("canoe -trace -ffvals -f ${canoe_ini} < \"${q_tok}\" > \"${p_raw}\"");
    call("ce_canoe2ffvals.pl -verbose=${verbose} -dir=\"${dir}\" \"${p_raw}\"");
}

POST:{
    truecase($tgt, ${p_tok}, ${P_tok});
    detokenize($tgt, ${P_tok}, ${P_txt});
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
        if ($tmx) {
            my $fopt = defined $filter ? "-filter=$filter" : "";
            call("ce_tmx.pl -verbose=${verbose} -src=${xsrc} -tgt=${xtgt} ${fopt} replace \"$dir\"");
        } else {
            my $ce_output = $out ? "> \"$out\"" : "";
            call("paste ${dir}/pr.ce \"${P_txt}\" ${ce_output}");
        }
    }
}

exit 0;

sub tokenize {
    my ($lang, $in, $out) = @_;
    if ($notok) {
        call("cp \"${in}\" \"${out}\"");
    } else {
        my $tokopt = " -lang=${lang}";
        $tokopt .= " -noss" unless $ss;
        call("utokenize.pl ${tokopt} \"${in}\" \"${out}\"");
    }
}

sub detokenize {
    my ($lang, $in, $out) = @_;
#     if ($notok) {
#         call("cp \"${in}\" \"${out}\"");
#     } else {
    call("udetokenize.pl -lang=${lang} < \"${in}\" > \"${out}\"");
#    }
}

sub lowercase {
    my ($in, $out) = @_;
    if ($nolc) {
        call("cp \"${in}\" \"${out}\"");
    } else {
        call("utf8_casemap -c l \"${in}\" \"${out}\"");
    }
}

sub truecase {
    my ($lang, $in, $out) = @_;
    if ($tcmap and $tclm) {
        if ($tctp) {
            call("truecase.pl --text=\"${in}\" --ucBOSEncoding=utf8 --tplm=${tclm} --tppt=${tcmap} > \"${out}\"");
        } else {
            call("truecase.pl --text=\"${in}\" --ucBOSEncoding=utf8 --lm=${tclm} --map=${tcmap} > \"${out}\"");
        }
    } else {
        call("cp \"${in}\" \"${out}\"");
    }
}

sub call {
    my ($cmd) = @_;
    verbose("[call: %s]\n", $cmd);
    if ($dryrun) {
        print $cmd, "\n";
    } else {
        system($cmd)==0 
            or die sprintf("Command \"%s\" failed: $?", $cmd);
    }
}

sub verbose { printf STDERR @_ if $verbose; }
