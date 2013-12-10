#!/usr/bin/env perl
#
# @file truecase.pl
# @brief Script to truecase target language text.
#
# Two truecasing workflows are supported. The first (old) workflow involves
# truecasing using just target language information to truecase the target 
# text; the second (new) workflow involves additionaly using source language
# information to improve the target text truecasing.
#
# In both workflows, the first step is to truecase the target language text 
# using a target language truecasing language model and vocabulary map. The 
# old workflow finishes with an optional step of very simple (and quite dumb) 
# beginning-of-sentence capitalization.
#
# The new workflow, on the other hand, performs two additional steps.
# It applies rules to transfer case information from the corresponding source
# language text to the target language text (i.e. to case OOVs and handle
# uppercase sequences). Finally, it applies beginning-of-sentence capitalization
# using source language case information and examining the target language text
# for sentence boundary markers.
#
# @author Rewritten by Darlene Stewart
#         Original by Akakpo Agbago under supervision of George Foster
#         with updates by Eric Joanis and Darlene Stewart
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2004, 2005, 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004, 2005, 2011, Her Majesty in Right of Canada

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
printCopyright("truecase.pl", 2004);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

use utf8;
use File::Basename;

use Time::HiRes qw( time );

use constant WITH_BASH => 1;

# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
use Getopt::Long;

my $start_time = time;

my $verbose = 0;
my $debug = 0;

Getopt::Long::GetOptions(
   # Common flags
   'help'            => sub { display_help(); exit 0 },
   'v|verbose+'      => \$verbose,
   'debug+'          => \$debug,
   'time'            => \my $timing,

   # Input argument
   'text=s'          => \my $in_file,     # target language text to truecase
   'src=s'           => \my $src_file,    # truecased source language text
   'pal=s'           => \my $pal_file,    # phrase alignments
   'wal=s'           => \my $wal,         # Which word-alignment source to use

   # Models arguments
   'lm=s'            => \my $lm_file,     # target language TC LM
   'map=s'           => \my $map_file,    # target language TC vocab map
   'tplm=s'          => \my $tplm,        # tightly packed target language TC LM
   'tppt=s'          => \my $tppt,        # tightly packed TC phrase table
   'lmorder=i'       => \my $lmOrder,     # LM order for target language TC LM
   'srclm=s'         => \my $srclm_file,  # source language NC1 LM

   # Flags for basic truecasing target language text using SRILM
   'uselmonly!'      => \my $use_lmOnly,
   'srilm!'          => \my $use_srilm,
   'viterbi!'        => \my $use_viterbi,

   # Beginning-of-sentence capitalization and encoding
   'bos!'            => \my $bos_cap,
   'encoding=s'      => \my $encoding,
   'srclang=s'       => \my $srclang,
   'locale=s'        => \my $locale,
   'ucBOSEncoding=s' => \my $ucBOSEncoding,  # deprecated

   "xtra-cm-opts=s"  => \my $xtra_cm_opts,   # extra options for casemark.py -r
   "xtra-bos-opts=s" => \my $xtra_bos_opts,  # extra options for boscap.py

   # Output arguments
   'out=s'           => \my $out_file,
)  or (print(STDERR "ERROR: truecase.pl aborted due to bad option.\nRun 'truecase.pl -h' for help.\n"), exit 1);

$timing = 0 unless defined $timing;

@ARGV < (defined $in_file ? 1 : 2)
   or die "ERROR: truecase.pl: too many arguments. Only one INPUT_FILE permitted.\n";
$in_file = $ARGV[0] unless defined $in_file or @ARGV == 0;
$in_file = '-' unless defined $in_file;

if ($use_srilm) {
   not defined $tplm and not defined $tppt
      or die "ERROR: '-tplm' and '-tppt' cannot be used with '-srilm'.\n";
   defined $lm_file or die "ERROR: LM must be specified using '-lm'.\n";
   defined $map_file or defined $use_lmOnly 
      or die "ERROR: MAP must be specified using '-map' or '-useLMOnly' must be used.\n";
   not defined $map_file or not defined $use_lmOnly 
      or die "ERROR: only one of '-map' or '-useLMOnly' can be used.\n";
} else {
   not defined $use_viterbi 
      or die "ERROR: '-[no]viterbi' option can only be used with '-srilm'.\n";
   not defined $use_lmOnly 
      or die "ERROR: '-[no]useLMOnly' option can only be used with '-srilm'.\n";
   defined $tplm or defined $lm_file 
      or die "ERROR: LM must be specified using '-tplm' or '-lm'.\n";
   not defined $tplm or not defined $lm_file 
      or die "ERROR: only one of '-tplm' and '-lm' can be used.\n";
   defined $tppt or defined $map_file 
      or die "ERROR: MAP must be specified using '-tppt' or '-map'.\n";
   not defined $tppt or not defined $map_file 
      or die "ERROR: only one of '-tppt' and '-map' can be used.\n";
   defined $tplm and defined $tppt or not defined $tplm and not defined $tppt
      or die "ERROR: LM and MAP must both be tightly packed (use '-tplm' and '-tppt') ",
             "or both not be tightly packed (use '-lm' and '-map').\n";
}

if (defined $ucBOSEncoding) {
   warn "WARNING: '-ucBOSEncoding' is deprecated. Use '-bos' and '-encoding' instead!\n",
        "Run 'truecase.pl -h' for help.\n";
   not defined $src_file
      or die "ERROR: '-ucBOSEncoding' option cannot be used with '-src'.\n";
   not defined $bos_cap and not defined $encoding
      or die "ERROR: '-ucBOSEncoding' option cannot be used with '-bos' or '-encoding'.\n";
   $bos_cap = 1;
   $encoding = $ucBOSEncoding;
}

if (defined $src_file) {
   defined $pal_file
      or die "ERROR: '-src' requires PAL_FILE to be specified with '-pal' option.\n";
   defined $srclm_file
      or die "ERROR: '-src' requires SRC_NC1_LM_FILE to be specified with '-srclm' option.\n";
   if (defined $locale) {
      not defined $srclang
         or die "ERROR: '-srclang' option cannot be used with '-loc'.\n";
      not defined $encoding
         or die "ERROR: '-enc' option cannot be used with '-loc'.\n";
      ($srclang, my $rest) = split(/_/, $locale, 2);
      ($rest, $encoding) = split(/\./, $rest, 2);
   } else {
      $srclang = "en" unless defined $srclang;
      $encoding = "utf-8" unless defined $encoding;
      $locale = "${srclang}_CA.${encoding}";
   }
   $xtra_cm_opts = "" unless defined $xtra_cm_opts;
   $xtra_bos_opts = "" unless defined $xtra_bos_opts;

   $wal = "mixed" unless defined $wal;
   $wal eq "h" or $wal eq "pal" or $wal eq "mixed"
      or die "ERROR: unrecognized value for '-wal': $wal; valid values are h, pal, and mixed\n";
} else {
   not defined $pal_file
      or die "ERROR: '-pal' option can only be used with '-src'.\n";
   not defined $srclm_file
      or die "ERROR: '-srclm' option can only be used with '-src'.\n";
   not defined $locale
      or die "ERROR: '-loc' option can only be used with '-src'.\n";
   not defined $srclang
      or die "ERROR: '-srclang' option can only be used with '-src'.\n";
   not defined $xtra_cm_opts
      or die "ERROR: '-xtra_cm_opts' option can only be used with '-src'.\n";
   not defined $xtra_bos_opts
      or die "ERROR: '-xtra_bos_opts' option can only be used with both '-src' and '-bos'.\n";
   $encoding = "utf-8" unless defined $encoding;
}

$bos_cap = 0 unless defined $bos_cap;
$lmOrder = 3 if (!defined $lmOrder && $use_srilm);

print STDERR "truecase.pl: truecasing '$in_file' starting\n" if $verbose;

my @tmp_files = ();     # Temporary files collection
my $tmp_txt = "tc_tmp_text_$$";

# Make sure the input files exist
$in_file eq '-' or -f $in_file && -r _ 
   or die "ERROR: Input text file '$in_file' is not a readable file.\n";
not defined $src_file or -f $src_file && -r _ 
   or die "ERROR: Source language file '$src_file' is not a readable file.\n";
not defined $pal_file or -f $pal_file && -r _ 
   or die "ERROR: PAL file '$pal_file' is not a readable file.\n";
not defined $lm_file or -f $lm_file && -r _ 
   or die "ERROR: Truecasing model '$lm_file' is not a readable file.\n";
not defined $map_file or -f $map_file && -r _ 
   or die "ERROR: Truecasing MAP '$lm_file' is not a readable file.\n";
not defined $tplm or -d $tplm && -x _ 
   or die "ERROR: Tightly packed truecasing model '$tplm' is not a readable directory.\n";
not defined $tppt or -d $tppt && -x _ 
   or die "ERROR: Tightly packed truecasing phrase table '$tppt' is not a readable directory.\n";
not defined $srclm_file or (-f $srclm_file && -r _) or (-d $srclm_file && -x _)
   or die "ERROR: Source NC1 language model '$srclm_file' is not readable.\n";

# Establish that the output file can be written.
$out_file = "" unless defined $out_file;
my $work_dir = dirname($out_file);
if ($out_file) {
   run("mkdir -p $work_dir");
   run("echo '' > $out_file");
}
my $gz = substr($out_file, -3, 3) eq ".gz" ? ".gz" : ""; # output to be gzipped?
my $gzip = $gz ? "| gzip" : "";

# Step 1: Basic truecasing using target language TC LM and map
my $cmd_part1;
if ($use_srilm) { # using disambig
   print STDERR "truecase.pl: using SRILM disambig",
                ($use_viterbi ? " and viterbi.\n" : ".\n") if $verbose;
   $cmd_part1 = "disambig -text $in_file -order $lmOrder -lm $lm_file"
                . ($use_lmOnly ? "" : " -map $map_file")
                . " -keep-unk"
                . ($use_viterbi ? "" : " -fb");
} else { # using canoe
   my ($ttable_type, $ttable_file, $model_file);
   if (defined $tplm) {
      print STDERR "truecase.pl: using canoe with TPT.\n" if $verbose;
      $ttable_type = "tppt";
      $model_file = $tplm;
      $ttable_file = $tppt;
   } else {
      print STDERR "truecase.pl: using canoe with on-the-fly phrase table.\n" if $verbose;
      $ttable_type = "multi-prob";
      $model_file = $lm_file;
      $ttable_file = "${work_dir}/tc_tmp_canoe_$$.tm";
      push @tmp_files, $ttable_file;
      convert_map_to_phrase_table($map_file, $ttable_file);
   }
   my $vb = $verbose > 1 ? $verbose - 1 : 0;
   $cmd_part1 = "set -o pipefail; "
                . "cat $in_file | canoe-escapes.pl -add "
                . "| canoe -f /dev/null -v $vb -load-first"
                . " -ttable-$ttable_type $ttable_file -lmodel-file $model_file"
                . (defined $lmOrder ? " -lmodel-order $lmOrder" : "")
                . " -ttable-limit 100 -regular-stack 100"
                . " -ftm 1.0  -lm 2.302585 -tm 0.0 -distortion-limit 0";
}

my $bos = $bos_cap && !defined $src_file ? " | boscap-nosrc.py -enc $encoding - - " : "";
my $tc_file = defined $src_file ? "${work_dir}/${tmp_txt}.tc$gz" : $out_file;
(push @tmp_files, $tc_file) if defined $src_file;
my $cmd = $cmd_part1
          . " | perl -n -e 's/^<s>\\s*//o; s/\\s*<\\/s>[ ]*//o; print;' $bos"
          . ($tc_file ? " $gzip > $tc_file" : "");
run($cmd, WITH_BASH);

# At this point we are done if we are just doing basic truecasing based on
# target language knowledge.

my $v = ($verbose > 1) ? "-v" : "";
my $d = ($debug > 1) ? "-d" : "";
my $t = $timing ? "-time" : "";

if (defined $src_file) {
   # Step 2: Normalize the case of sentence-initial characters in the source text.
   # (needed by steps 3 and 4).
   my $nc1_file = "${work_dir}/${tmp_txt}.nc1$gz";
   my $nc1ss_file = $nc1_file;
   my $nc1sj_file = "${work_dir}/${tmp_txt}.nc1sj$gz";   # sentence joined
   push @tmp_files, $nc1_file, $nc1sj_file;
   my $tee_file = $gz ? ">(gzip > $nc1_file)" : "$nc1_file";
   run("normc1 $v -ignore 1 -extended -notitle -loc $locale $srclm_file $src_file "
       . "| tee $tee_file "
       . "| perl -pe 's/(.)\$/\$1 /; s/(.)\\n/\$1/' "
       . "$gzip > $nc1sj_file", WITH_BASH);
   my $pal_lc = run_with_output("zcat -f $pal_file | wc -l");
   my $nc1_lc = run_with_output("zcat -f $nc1_file | wc -l");
   $nc1_file = $nc1sj_file if $nc1_lc != $pal_lc;

   # Step 3: Transfer case from source language to target language text.
   my $cmark_file = "${work_dir}/${tmp_txt}.cmark$gz";
   my $cmark_out_file = "${work_dir}/${tmp_txt}.cmark.out$gz";
   my $cmark_log_file = "${work_dir}/${tmp_txt}.cmark.log";
   push @tmp_files, $cmark_file, $cmark_out_file, $cmark_log_file;
   my $cmark_tc_file = $bos_cap ? "${work_dir}/${tmp_txt}.cmark.tc$gz" : $out_file;
   (push @tmp_files, $cmark_tc_file) if defined $bos_cap;
   # Preload the C++ standard library to ensure that C++ exceptions and I/O 
   # are properly initialized when calling a C++ shared library from Python.
   my $ld_preload = "LD_PRELOAD=libstdc++.so:\$LD_PRELOAD";
   die "Encoding not defined" unless(defined($encoding));
   run("$ld_preload casemark.py $v $d $t -a -lm $srclm_file -enc $encoding $nc1_file $cmark_file", WITH_BASH);
   run("markup_canoe_output $v -wal $wal -n OOV $cmark_file $tc_file $pal_file "
       . "2> $cmark_log_file $gzip > $cmark_out_file");
   run("casemark.py $v $d $t -r -enc $encoding $xtra_cm_opts $cmark_out_file $cmark_tc_file");

   # Step 4: Capitalize beginning-of-sentence words, using hints from source language text.
   if ($bos_cap) {
      my $opts = ($nc1_lc != $pal_lc ? "-ssp " : "") . "-enc $encoding $xtra_bos_opts";
      if ($debug) {
         my $bos_file = "${work_dir}/${tmp_txt}.bos$gz";
         my $bos_out_file = "${work_dir}/${tmp_txt}.bos.out$gz";
         push @tmp_files, $bos_file, $bos_out_file;
         $opts .= "$d -srcbos $bos_file -tgtbos $bos_out_file";
      }
      run("boscap.py $v $t $opts $cmark_tc_file $src_file $nc1ss_file $pal_file $out_file");
   }
}

print STDERR "truecase.pl: truecasing '$in_file' done.\n" if $verbose;

# Delete all the temporary files.
unless ($debug) {
   foreach my $file (@tmp_files) {
      system("rm -rf $file") if $file;
   }
}

(print STDERR "truecase.pl: truecasing workflow took ", time - $start_time, " seconds.\n") if $timing;


sub convert_map_to_phrase_table
{
   # Convert the vocab map to a phrase table on the fly.
   my ($map_file, $pt_file) = @_;
   my $start = time if $timing;
   portage_utils::zin(*MAP, $map_file);
   open( TM,  ">", "$pt_file" ) 
      or die "ERROR: unable to open '$pt_file' for writing";

   # Need to be locale agnostic when converting map file (MAP) to the phrase table (TM).
   binmode(TM);
   binmode(MAP);
   while (<MAP>) {
      chomp;
      my @line = split( /\t/, $_ );
      my $from = shift( @line );
      my $to = shift( @line );
      my $prob = shift( @line );
      while (defined $to && $to ne '' and defined $prob && $prob ne '' ) {
         print TM "$from ||| $to ||| 1 $prob\n";
         $to = shift( @line );
         $prob = shift( @line );
      }
   }
   close( MAP );
   close( TM );
   (print STDERR "truecase.pl: converting map took ", time - $start, " seconds.\n") if $timing;
}

sub run
{
   my ($cmd, $with_bash) = @_;
   my $start = time if $timing;
   $with_bash = 0 unless defined $with_bash;
   print STDERR "COMMAND: $cmd\n" if $verbose;
   system($with_bash ? ("/bin/bash", "-c", $cmd) : $cmd) == 0
      or die "ERROR: truecase.pl failed (error $?) running '$cmd'.\n";
   (print STDERR "truecase.pl: Running ", (split(' ', $cmd, 2))[0], " took ", time - $start, " seconds.\n") if $timing;
}

sub run_with_output
{
   my ($cmd) = @_;
   my $output = `$cmd`;
   die "ERROR: truecase.pl failed (error $?) running '$cmd'.\n" if $?;
   chomp $output;     # remove last \n
   return $output;
}

sub display_help
{  
   -t STDERR ? system "pod2text -t -o $0 >&2" : system "pod2text $0 >&2";
}


=pod

=head1 NAME

truecase.pl - Truecase target language text

=head1 SYNOPSIS

truecase.pl [options] [-text] [INPUT_FILE]

=head1 DESCRIPTION

This script converts target language text to its truecase form.
Two truecasing workflows are supported. The first (old) workflow involves
truecasing using just target language information to truecase the target text;
the second (new) workflow involves additionaly using source language
information to improve the target text truecasing.

In both workflows, the first step is to truecase the target language text using
a target language truecasing language model and vocabulary map. The old
workflow finishes with an optional step of very simple (and quite dumb)
beginning-of-sentence capitalization. The new workflow, on the other hand,
performs two additional steps. It applies rules to transfer case information
from the corresponding source language text to the target language text
(i.e. to case OOVs and handle uppercase sequences). Finally, it applies
beginning-of-sentence capitalization using source language case information
and examining the target language text for sentence boundary markers.

=head1 OPTIONS

=head2 Input/Output

=over 12

=item -text INPUT_FILE

Input tokenized target language text file to truecase. Default is STDIN.

=item -out OUTPUT_FILE

Output tokenized file for truecased target language text. Default is STDOUT.

=item -src SRC_FILE

Corresponding truecased tokenized source language text. (new TC workflow)

=item -pal PAL_FILE

Phrase alignment file mapping between source and target language phrases.
Required with -src.

The PAL_FILE can be generated by calling canoe like this:

 canoe -f canoe.ini [-walign] -palign < SRC \
    | nbest2rescore.pl -canoe [-wal] -palout=PAL > TGT_TEXT

If the -walign and -wal switches are added and word-alignment information is
found in the phrase table, that word alignment is used, otherwise heuristic
word alignment is used (see -wal switch below).

=item -wal WAL

-wal h: Use heuristic word alignment for -xtags and for -tcsrclm, even if work
alignment is available in the phrase table.  (For baseline evaluation only.)

-wal pal: Use the word alignment from the a field in the phrase table (via the
PAL file).  In this case, it is an error if the word alignments are missing for
multi-word phrases.

-wal mixed: Use the word alignment from the a field if found, falling back to
the heuristic otherwise.

[mixed]

=back

=head2 Models

=over 12

=item -lm LM_FILE

Target language truecasing Language Model (NGram file) to use.
Can be in text or binlm format (with canoe), GZIP or uncompressed;
(use -tplm for tplm format). One of -lm or -tplm is required.

=item -map MAP_FILE

V1 to V2 vocabulary mapping model. One of -map or -tppt is required with canoe.

=item -lmOrder N

Effective N-gram order used by the target language TC LM model.

=item -tplm TPLM_FILE

Target language TC Language Model in TPLM format.
Valid with canoe only, not SRILM. -tplm and -tppt must be used together.

=item -tppt TPPT_FILE

V1 to V2 phrase table in TPPT format. (Use vocabMap2tpt.sh to create the TPPT).
Valid with canoe only, not SRILM. -tplm and -tppt must be used together.

=item -srclm SRC_NC1_LM_FILE

Source language sentence-initial case normalization (NC1) Language Model.
Can be in text or binlm format, GZIP or uncompressed, or tplm format.
Required with -src.

=back

=head2 Beginning-of-Sentence (BOS) Casing and Encoding

=over 12

=item -bos

Do beginning-of-sentence capitalization.

=item -encoding ENCODING

The source and target language text file encoding. Default is 'utf-8'.
Not valid with -locale.

=item -srclang SRC_LANG

The source language (in 2 character format). Default is 'en'.
Not valid with -locale.

=item -locale LOCALE

The locale for the source language. The encoding and source language can be
gleaned from the locale, so -encoding and -srclang cannot be used with -locale.
Valid only with -src.

=item -xtra-bos-opts OPTS

Specify additional C<boscap.py> options. Valid only with both -src and -bos.

=item -ucBOSEncoding ENC

DEPRECATED.

If defined, then uppercase the beginning of each sentence using the specified 
encoding (e.g. C<utf8>). Correctly uppercases accented characters.

=back

=head2 Using SRILM

=over 12

=item -useSRILM

Use SRILM disambig instead of canoe to perform the truecasing.
The default is to use canoe.

=item -useViterbi

Use Viterbi matching instead of forward-backward matching in SRILM disambig.

=item -useLMOnly

Use only the given NGram model with SRILM disambig; no V1-to-V2 mapping used.

=back

=head2 Other

=over 12

=item -xtra-cm-opts OPTS

Specify additional C<casemark.py -r> options. Valid only with -src.

=item -verbose

Print verbose information to STDERR.  (cumulative)

=item -debug

Print debug output to STDERR.

=item -help

Print this help and exit.

=item -time

Produce detailed timing info.

=back

=head1 EXAMPLES

=head2 BASIC

 truecase.pl -lm ngram.binlm.gz -map mapping.map.gz [-lmOrder 3] \
        [-bos] [-encoding utf-8] [-verbose] -text infile.txt \
        > outfile.tc

 truecase.pl -lm ngram.lm -useSRILM -useLMOnly [-lmOrder 3] \
        [-bos] [-enc cp1252] [-out outfile.tc] [-verbose] infile.txt

 truecase.pl -tplm lm.tplm -tppt mapping.tppt infile.txt

 cat infile.txt | truecase.pl -lm ngram.lm -map mapping.map -text - >outfile.tc

 cat infile.txt | truecase.pl -lm ngram.lm -map mapping.map >outfile.tc

=head2 USING SOURCE INFO

 truecase.pl -lm tc_en_lm.binlm.gz -map tc_en_map.map.gz [-lmOrder 3] \
        -src text_fr.tc -srclm nc1_fr_lm.binlm.gz -pal text.pal \
        [-bos] -srclang fr [-encoding utf-8] [-verbose] text_en.lc > text_en.tc

 truecase.pl [-verbose] -tplm tc_fr_lm.tplm -tppt tc_fr_map.tppt [-lmOrder 3] \
        -src text_fr.tc -srclm nc1_fr_lm.tplm -pal text.pal \
        [-bos] [-srclang en] [-encoding utf-8] text_en.lc > text_en.tc

=head1 CAVEATS

 The following may or may not be relevant.
 If you experience errors related to malformed UTF-8 when processing non-UTF8 
 data, the solution is to adjust the value of your "$LANG" Environment variable. 
 Example: if "LANG==en_CA.UTF-8", set "LANG=en_CA" and export it. 
 If the problem persists, try setting LC_ALL instead.

=head1 AUTHOR

=over 1

B<Programmer> - Rewritten by Darlene Stewart. Original by Akakpo Agbago

=back

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright (c) 2004, 2005, 2011, Sa Majeste la Reine du Chef du Canada /
 Copyright (c) 2004, 2005, 2011, Her Majesty in Right of Canada

=cut
