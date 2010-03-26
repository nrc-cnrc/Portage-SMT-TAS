#!/usr/bin/perl -sw

# @file nbest2rescore.pl 
# @brief Parses nbest lists.
# 
# @author Michel Simard
# 
# COMMENTS:
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;
use warnings;

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

# globals
my $phrase_re = qr/\"([^\\\"]*(?:(?:\\.)[^\\\"]*)*)\"/;
my $align_re = qr/a=\[([^;\]]+;[^;\]]+;[^;\]]+[^]]*)\]/;
my $ffvals_re = qr/v=\[([^\]]+)\]/;
my $legacy_re = qr/\(([^\)]+)\)/;

# command-line
my $HELP = 
"Usage: nbest2rescore.pl [options]

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

  -ff		Output feature function values [don't, but implied by -ffout]
  -pal		Output phrase alignments [don't, but implied by -palout]
  -oov          Output OOV status in -palout file [don't]
  -tagoov       Tag each OOV phrase in target text, eg <OOV>oov-phrase</OOV> [don't]

  -nbest=N	Only print N best translations for each source [0 means all]
  -format=F	Specify output format: rescore, phrase or xml [rescore]

  -append       Append to all output files [overwrite all output files]
  -legacy	Expect ancient canoe output (emulate parse_ffvals.pl) [don't]
  -canoe	Expect direct output from canoe, i.e. not a N-best list,
		and therefore with only one level of backslash's [don't]

Note: all output can be directed to stdout by specifying '-' as output
file; each translation then produces up to three lines of output:
translation, alignment info, and ff values info (in that order).
";

our ($h, $help, $in, $out, $ff, $ffout, $pal, $palout, $nbest, $format, $legacy, $canoe, $oov, $tagoov, $append);

if (defined($h) or defined($help)) {
    print STDERR $HELP;
    exit 0;
}
$in = "-" unless defined $in;
$out = "-" unless defined $out;
$ff = $ffout unless defined $ff;
$ffout = 0 unless defined $ffout;
$pal = $palout unless defined $pal;
$palout = 0 unless defined $palout;
$nbest = 0 unless defined $nbest;
$format = "rescore" unless defined $format;
$legacy = 0 unless defined $legacy;
$canoe = 0 unless defined $canoe;
$oov = 0 unless defined $oov;
$tagoov = 0 unless defined $tagoov;
my $mode = ">";
$mode = ">>" if (defined($append));

die "Don't know what to do with all these arguments."
    if @ARGV;


# Do it.
open(my $in_stream, "<$in") 
    or die "Can't open input nbest file $in\n";
my $out_stream;
if ($out =~ /.gz/) {
   $out =~ s/(.*\.gz)\s*$/| gzip -cqf $mode $1/;
   open($out_stream, $out) 
      or die "Can't open output file $out\n";
}
else {
   open($out_stream, "$mode$out")
      or die "Can't open output file $out\n";
}

my ($ff_stream, $pal_stream);
if ($ff && $ffout) {
    if ($ffout =~ /.gz/) {
       $ffout =~ s/(.*\.gz)\s*$/| gzip -cqf $mode $1/;
       open($ff_stream, $ffout) 
          or die "Can't open output ff file $ffout\n";
    }
    else {
       open($ff_stream, "$mode$ffout") 
          or die "Can't open output ff file $ffout\n";
    }
}
if ($pal && $palout) {
    if ($palout =~ /.gz/) { 
       $palout =~ s/(.*\.gz)\s*$/| gzip -cqf $mode $1/; 
       open($pal_stream, $palout)
	   or die "Can't open output phrase alignment file $palout\n";
    }
    else {
       open($pal_stream, "$mode$palout")
 	  or die "Can't open output phrase alignment file $palout\n";
    }
}

my $count = 0;
my $max_ff_vals = 0;		# Ugly global var to keep track of
				# number of feature functions
while (my $line = <$in_stream>) {
    chomp $line;
    my $translation = parseTranslation($line, $legacy);
    printOutput($format, $translation, ++$count, 
		$out_stream, $ff_stream, $pal_stream);
    last if ($nbest && $translation && ($count >= $nbest));
}

close $in_stream;
close $out_stream;
close $ff_stream 
    if $ff_stream;
close $pal_stream
    if $pal_stream;

exit 0;



# Subs

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
	    warn "Empty phrase in line <<$string>>\n";
            @words = ("empty");
#	    return 0;
	}
	push @target, @words;

	my %record = ( phrase=>[ @words ] );

	# Process the data part
	if ($alignment) {
	    $alignment =~ s/\\(.)/$1/go;	# remove inner level of backslashification
	    my ($score, $begin, $end, $oovstatus) = split(/;/o, $alignment, 4);
	    $record{score} = $score;
	    $record{begin} = $begin;
	    $record{end} = $end;
            $record{oov} = $oovstatus;
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
    if ($line) {
	warn "Junk at end of line <<$string>>: <<$line>>\n";
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
	die "Don't know how to write output in \"$how\" format\n";
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
    my ($translation, $count, $outstream, $outff, $outpal, $outoov) = @_;

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

	    print $outpal ' ' unless $pcount == 1; # phrase separator
	    printf($outpal "%d:%d-%d:%d-%d%s", 
		   $pcount, 	# phrase count
		   $phrase->{begin}, $phrase->{end}, # src range
		   $tgt_pos, $tgt_pos + $psz - 1,    # tgt range
                   $oov ? ":$phrase->{oov}" : "");      # oov status
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
