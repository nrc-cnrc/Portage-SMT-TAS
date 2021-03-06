#!/usr/bin/env perl

# @file nbest2rescore.pl
# @brief Parses nbest lists.
#
# @author Michel Simard
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use warnings;
use JSON;

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
printCopyright("nbest2rescore.pl", 2005);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

use FileHandle;

# globals
my $phrase_re = qr/\"([^\\\"]*(?:(?:\\.)[^\\\"]*)*)\"/;
#my $align_re = qr/a=\[([^;\]]+;[^-;\]]+[-;][^;\]]+[^]]*)\]/;
my $align_re  = qr/a=\[([^\]]+)\]/;
my $ffvals_re = qr/v=\[([^\]]+)\]/;
my $legacy_re = qr/\(([^\)]+)\)/;

# command-line
sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: nbest2rescore.pl [options]

Read N-best translation files, as produced by the canoe decoder, and produce
output in a format compatible with the rescore programs (rescore_train,
rescore_translate, etc.). This presupposes that canoe was called with the
-nbest flag and at least one of -trace (produces alignment info) and -ffvals
(feature function values).

Options:
  -in=I		Specify input N-best file [stdin]
  -out=O	Specify output text file [stdout]
  -ffout=FF	Specify output feature functions file; implies -ff [none]
  -palout=PAL	Specify output phrase alignment file; implies -pal [none]
  -source=source Specify the source sentence file; [none]
  -target=target Specify an alternate target sentence file to use with -json; [none]
  -json=file     Output the phrase alignment in a json format to file.  requires -source [don't]

  -oov          Output OOV status in -palout file [don't]
  -wal          Output word-alignment info in the -palout file (implies -oov) [don't]
  -tagoov       Tag each OOV phrase in target text, eg <OOV>oov-phrase</OOV> [don't]

  -nbest=N	Only print N best translations for each source [0 means all]
  -format=F	Specify output format: rescore, phrase or xml [rescore]

  -append       Append to all output files [overwrite all output files]
  -legacy	Expect ancient canoe output (emulate parse_ffvals.pl) [don't]
  -canoe	Expect direct output from canoe, i.e. not a N-best list,
		and therefore with only one level of backslash's [don't]

Note: all output can be directed to stdout by specifying '-' as output file;
each translation then produces up to four lines of output: translation, phrase
alignment info, word alignment info, and ff values (in that order).
";
   exit @_ ? 1 : 0;
}

use Getopt::Long;
Getopt::Long::Configure("no_ignore_case");
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
   "in=s"      => \my $in,
   "out=s"     => \my $out,
   "ffout=s"   => \my $ffout,
   "palout=s"  => \my $palout,
   "source=s"  => \my $source,
   "target=s"  => \my $target,
   "json=s"    => \my $json,
   oov         => \my $oov,
   wal         => \my $wal,
   tagoov      => \my $tagoov,
   "nbest=i"   => \my $nbest,
   "format=s"  => \my $format,
   append      => \my $append,
   legacy      => \my $legacy,
   canoe       => \my $canoe,
) or usage "Error: Invalid option(s).";

$in = "-" unless defined $in;
$out = "-" unless defined $out;
$format = "rescore" unless defined $format;
$oov = 1 if $wal;

my $mode = ">";
$mode = ">>" if (defined($append));

die "Error: You must specify the source when using -json." if ($json and not $source);

die "Error: Don't know what to do with these extra arguments: @ARGV"
    if @ARGV;


# Do it.
open(my $in_stream, "<$in")
    or die "Error: nbest2rescore.pl can't open input nbest file <'$in': $!\n";
binmode($in_stream, ":encoding(utf-8)");

my $out_stream = new FileHandle;
zopen($out_stream, "$mode$out")
   or die "Error: nbest2rescore.pl can't open output file $mode'$out': $!\n";
binmode($out_stream, ":encoding(utf-8)");

my $ff_stream;
if ($ffout) {
   $ff_stream = new FileHandle;
   zopen($ff_stream, "$mode$ffout")
      or die "Error: nbest2rescore.pl can't open output ff file $mode'$ffout': $!\n";
   binmode($ff_stream, ":encoding(utf-8)");
}

my $pal_stream;
if ($palout) {
   $pal_stream = new FileHandle;
   zopen($pal_stream, "$mode$palout")
      or die "Error: nbest2rescore.pl can't open output phrase alignment file $mode'$palout': $!\n";
   binmode($pal_stream, ":encoding(utf-8)");
}

my $source_stream;
if ($source) {
   $source_stream = new FileHandle;
   zopen($source_stream, "<$source")
      or die "Error: nbest2rescore.pl can't open input source file '$source': $!\n";
   binmode($source_stream, ":encoding(utf-8)");
}

my $target_stream;
if ($target) {
   $target_stream = new FileHandle;
   zopen($target_stream, "<$target")
      or die "Error: nbest2rescore.pl can't open input target file '$target': $!\n";
   binmode($target_stream, ":encoding(utf-8)");
}

my $json_stream;
if ($json) {
   $json_stream = new FileHandle;
   zopen($json_stream, "$mode$json")
      or die "Error: nbest2rescore.pl can't open output phrase alignment json file $mode'$json': $!\n";
   # No need to open this file in UTF-8 since the JSON library takes care of this.
   #binmode($json_stream, ":encoding(utf-8)");
}

my @alignments;
my $count = 0;
my $max_ff_vals = 0;		# Ugly global var to keep track of
				# number of feature functions
while (my $line = <$in_stream>) {
    chomp $line;
    my $translation = parseTranslation($line, $legacy);
    printOutput($format, $translation, ++$count,
		$out_stream, $ff_stream, $pal_stream);
    push(@alignments, $translation) if ($json);
    last if ($nbest && $translation && ($count >= $nbest));
}

print $json_stream encode_json(\@alignments), "\n" if ($json);

close $in_stream;
close $out_stream;
close $ff_stream
    if $ff_stream;
close $pal_stream
    if $pal_stream;
if ($source) {
   warn("Warning: Source file too long!") if (defined(<$source_stream>));
   close $source_stream if $source;
}
if ($target) {
   warn("Warning: Target file too long!") if (defined(<$target_stream>));
   close $target_stream if $target;
}
close $json_stream if $json;

exit 0;



# Subs

my $sourceTooShort = undef;
my $targetTooShort = undef;
sub parseTranslation {
    my ($string, $legacy) = @_;

    my @phrases = ();
    my $translation = 0;

    my $line = $string;  	# copy
    chomp $line;		# remove newlines
#    print STDERR "INPUT: <<$line>>\n";
    $line =~ s/^\s+//o;         # remove leading and trailing spaces
    $line =~ s/\s+$//o;         # remove leading and trailing spaces
    $line =~ s/\\(.)/$1/go	# remove outer level of backslashification
	unless $canoe;		# unless we're reading direct output from canoe
#    print STDERR "DEBACKSLASHIFICATION: <<$line>>\n";
    # Loop on phrases:
    my @target = ();
    my $count = 0;

    my $re;
    if ($legacy) {
	$re = qr/^\s*${phrase_re}()(?:\s${legacy_re})?\s*(.*)$/;
    } else {
	$re = qr/^\s*${phrase_re}(?:\s${align_re})?(?:\s${ffvals_re})?\s*(.*)$/;
    }

    while ($line =~ /$re/o) {
	my $phrase = $1;
	my $alignment = $2;
	my $ffvals = $3;
	$line = $4;
	$count++;

#	print STDERR "PHRASE[$count]: <<$phrase>>\n";
#	print STDERR "ALIGN[$count]: <<$alignment>>\n";
#	print STDERR "FFVALS[$count]: <<$ffvals>>\n";

	# Process the phrase part
	$phrase =~ s/\\(.)/$1/go;	# remove inner level of backslashification
	my @words = split(/\s+/o, $phrase);
	if (!@words) {
	    warn "Warning: Empty phrase in line <<$string>>\n";
            @words = ("empty");
#	    return 0;
	}
	push @target, @words;

	my %record = ( phrase=>[ @words ] );

	# Process the data part
	if ($alignment) {
	    #$alignment =~ s/\\(.)/$1/go;	# remove inner level of backslashification
            my ($score, $range, $oovflag, $walign) = split(/;/, $alignment, 4);
            my ($begin, $end) = split(/-/, $range, 2);
            $walign = "" unless defined $walign;
	    $record{score} = $score;
	    $record{begin} = $begin;
	    $record{end} = $end;
            $record{oov} = $oovflag;
            $record{walign} = $walign;
	}
	if ($ffvals) {
	    $ffvals  =~ s/\\(.)/$1/go;	# remove inner level of backslashification
	    my @v = ();
	    @v = $legacy ? split(/\s*,\s*/o, $ffvals) : split(/;/o, $ffvals);
	    $record{ffvals} = [ @v ];
	    $max_ff_vals = @v if @v > $max_ff_vals; # Ugly global var to keep track of number of features
	}
	
	push @phrases, { %record };
    }

    $translation = { target=>[ @target ], phrases=>[ @phrases ] };

    if ($source) {
       # TODO: What about unbalanced source / input stream.  Should be unlikely but we should still check it.
       if (defined(my $s = <$source_stream>)) {
          chomp($s);
          $translation->{source} = [split(/ +/, $s)];
       }
       else {
          $translation->{source} = [split(/ +/, "Error: The source file is too short!")];
          warn("Warning: The source file is too short!") unless(defined($sourceTooShort));
          $sourceTooShort = 1;
       }
    }

    if ($target) {
       # TODO: What about unbalanced target / input stream.  Should be unlikely but we should still check it.
       if (defined(my $s = <$target_stream>)) {
          chomp($s);
          # Let's add the translation to the sentence.
          $translation->{target} = [split(/ +/, $s)];
          # Let's replace each phrase's target by the equivalent in target.
          my $targetWordIndex = 0;
          foreach my $phrase (@{$translation->{phrases}}) {
             my $pl = scalar(@{$phrase->{phrase}});
             my $end = $targetWordIndex + $pl;
             $phrase->{phrase} = [ @{$translation->{target}}[$targetWordIndex..$end-1] ];
             $targetWordIndex = $end;
          }
          die "You provide a -target that doesn't match the input file." unless($targetWordIndex == scalar(@{$translation->{target}}));
       }
       else {
          $translation->{target} = [split(/ +/, "Error: The target file is too short!")];
          warn("Warning: The target file is too short!") unless(defined($targetTooShort));
          $targetTooShort = 1;
       }
    }

    if ($line) {
	warn "Warning: Junk at end of line <<$string>>: <<$line>>\n";
	return 0;
    }

    return $translation;
}


# Output functions
sub printOutput {
    my ($how, @what) = @_;

    if ($how eq "xml") {
	printXML(@what);
    } elsif ($how eq "phrase") {
	printPhraseAlign(@what);
    } elsif ($how eq "rescore") {
	printRescore(@what);
    } else {
	die "Error: Don't know how to write output in \"$how\" format\n";
    }
}

sub printXML {
    my ($translation, $count, $outstream) = @_;

    return unless $translation;

    print $outstream "<translation>\n";
    print $outstream (" <target no=\"$count\">",
		      join(" ", @{$translation->{target}}),
		      "</target>\n");
    print $outstream " <phrases>\n";
    foreach my $phrase (@{$translation->{phrases}}) {
	print $outstream "  <phrase>\n";
	print $outstream "   <words>", join(" ", @{$phrase->{phrase}}), "</words>\n";
	print $outstream "   <cover>", $phrase->{begin}, " ", $phrase->{end}, "</cover>\n";
	print $outstream "   <score>", $phrase->{score}, "</score>\n";
	print $outstream "   <ffvals>", join(" ", @{$phrase->{ffvals}}), "</ffvals>\n"
	    if $phrase->{ffvals};
	print $outstream "  </phrase>\n";
    }
    print $outstream " </phrases>\n";
    print $outstream "</translation>\n";
}

sub printRescore {
    my ($translation, $count, $outstream, $outff, $outpal) = @_;

    return if ! $translation;

    # First, we print the target sentence
    if ($tagoov) {
       foreach my $phrase (@{$translation->{phrases}}) {
          my $s = join(" ", @{$phrase->{phrase}});
          if ($phrase->{oov} eq "O") {
             print $outstream "<OOV>$s</OOV> ";
          } else {
             print $outstream "$s ";
          }
       }
       print $outstream "\n";
    } else {
       print $outstream join(" ", @{$translation->{target}}), "\n";
    }

    # Then, we print individual phrase alignments
    if ($outpal) {
	my $pcount = 0;
	my $tgt_pos = 0;
	foreach my $phrase (@{$translation->{phrases}}) {
	    $pcount++;
	    my $psz = scalar @{$phrase->{phrase}};

	    print $outpal ' ' unless $pcount == 1;   # phrase separator
	    printf($outpal "%d:%d-%d:%d-%d%s%s",
		   $pcount, 	                     # phrase count
		   $phrase->{begin}, $phrase->{end}, # src range
		   $tgt_pos, $tgt_pos + $psz - 1,    # tgt range
                   $oov ? ":$phrase->{oov}" : "",    # oov status
                   $wal ? ":$phrase->{walign}" : "");# word-alignment info
	    $tgt_pos += $psz;	# update tgt position
	}
	print $outpal "\n";
    }

    # Then, we do the feature function values:
    if ($outff) {
	my @ff = (0) x $max_ff_vals; # Ugly global...
	if (@{$translation->{phrases}}) {
	    foreach my $phrase (@{$translation->{phrases}}) {
		for (my $i = 0; $i <= $#ff; $i++) {
		    $ff[$i] += $phrase->{ffvals}[$i];
		}
	    }
	}
	print $outff join("\t", @ff), "\n";
    }
}

sub printPhraseAlign {
    my ($translation, $count) = @_;

    return unless $translation;

    print "# target ($count): ", join(" ", @{$translation->{target}}), "\n";
    my $first = 1;
    foreach my $phrase (@{$translation->{phrases}}) {
	my $words = join(" ", @{$phrase->{phrase}});
	$words =~ s/[\(\)]/\\$&/go; # Escape what must
	printf("%s[%d, %d] (%s)", ($first ? "" : " "),
	       $phrase->{begin}, $phrase->{end}, $words);
	$first = 0;
    }
    print "\n";
}
