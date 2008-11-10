#!/usr/bin/perl -w
# $Id$
#
# @file load-balancing.pl 
# @brief Splits the input into blocks where the first block should be the
# hardest to translate and the last block, the easiest.
#
# @author Samuel Larkin
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use POSIX qw(ceil floor);
use strict;

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [-h(elp)] [-v(erbose)]
          -node=<number> -job=<number>
          [-output <PATTERN>]
          [-input <ifile>]
          [-ref <rfile>]

   Sorts by decreasing order of source sentences' length then the N/J longest
   sentences are striped into N similar size jobs and the rest are split into
   (J - N) jobs.
   Output:
      [ PATTERN.0000, PATTERN.node )
      [ PATTERN.node, PATTERN.job )
   if a reference is available:   
      [ PATTERN.ref.0000, PATTERN.ref.node )
      [ PATTERN.ref.node, PATTERN.ref.job )

Options:
   -node      number of nodes
   -job       number of jobs
   -input     input file [-]
   -ref       reference file
   -output    output filename pattern [input]
";
   exit 1;
}

use Getopt::Long;

my $verbose = 0;
my $input   = "-";
my $output  = "input";
GetOptions(
   "node|n=i"       => \my $node,
   "job|j=i"        => \my $job,
   "input|i=s"      => \$input,
   "output|o=s"     => \$output,
   "ref|r=s"        => \my $ref,
   "help|h|?"       => sub { usage },
   "debug|d"        => \my $DEBUG,
   "verbose|v"      => sub { ++$verbose },
) or usage;


##################################################
# CHECKING PARAMETERS
die "You must specify the node's parameter" if (!defined($node));
die "You must specify the job's parameter" if (!defined($job));


##################################################
# READING THE INPUT AND REFERENCE
open(INPUT, "<$input") or die "Unable to open $input for reading\n";
if (defined($ref)) {
   open(REF, "<$ref") or die "Unable to open $ref for reading\n";
}

my $line;
my %src_sents = ();
my $index=0;
while (defined($line = <INPUT>))
{
   chomp($line);
   my $word_size = scalar @{[ split /\s+/, $line ]};
   my $char_size = length($line);
   $line = $index . "\t" . $line;
   $src_sents{$line}{'word'} = $word_size;
   $src_sents{$line}{'char'} = $char_size;
   $src_sents{$line}{'index'} = ++$index;
   if (defined($ref)) {
      my $tmp = <REF>;
      if (defined($tmp)) {
         chomp($tmp);
         $src_sents{$line}{'ref'} = $tmp;
      }
      else {
         printf(STDERR "Missing reference\n");
         exit -2;
      }
   }
}
close(INPUT);
close(REF) if (defined($ref));


##################################################
# SORTING IN DECREASING ORDER OF SOURCE SENTENCE LENGTH
my @sorted = sort { $src_sents{$b}{'word'} <=> $src_sents{$a}{'word'} } keys %src_sents; 

# FOR DEBUGGING
if (0) {
   foreach my $k (@sorted) {
      print $src_sents{$k}{'word'} . "\t" . $k . "\n";
   }
}


##################################################
# CHECK ARGUMENTS LOGIC
# Silently reajust $head and $tail to some appropriate sizes that will fit in $index
if ($node > $index) {
   $node = $index;
   warn "You are asking for more blocks than there are lines => reajusting node=$job";
}
$job=$node if ($job<$node);


##################################################
# PROCESS THE FIRST HALF
my $processed=0;
my $file_index=0;
my $midpoint = floor($index*($node/$job));  # find the midpoint between two halves
printf STDERR "midpoint $midpoint\n" if (defined($DEBUG));
# First half will be stripped
for (my $m=0; $m<$node; ++$m)
{
   my $filename = sprintf("$output.%4.4d", $file_index);
   open(OUT, ">$filename") or die "Unable to open $filename to write output\n";
   if (defined($ref)) {
      my $reffile = sprintf("$output.ref.%4.4d", $file_index);
      open(REF, ">$reffile") or die "Unable to open $reffile to write output\n";
   }
   for (my $i=$m; $i<$midpoint; $i=$i+$node)
   {
      my $k = $sorted[$i];
      print OUT "$k\n";
      print REF "$src_sents{$k}{'ref'}\n" if (defined($ref));
      ++$processed;
   }
   close(OUT);
   close(REF) if (defined($ref));
   ++$file_index;
}


##################################################
# PROCESS THE SECOND HALF
# Second half is chop in decreasing size
my $rest = $index - $midpoint;  # What's left to split between jobs
$job = $job - $node;            # How many jobs left for the rest
if ($rest > 0) {
   die "ASSERTION: there should be some jobs left" unless ($job > 0);

   # The first few blocks may contain more sentences per block.
   my $block_size = ceil($rest/$job);
   my $t=0;
   for (; $t<($rest%$job); ++$t)
   {
      my $filename = sprintf("$output.%4.4d", $file_index);
      open(OUT, ">$filename") or die "Unable to open $filename to write output\n";
      if (defined($ref)) {
         my $reffile = sprintf("$output.ref.%4.4d", $file_index);
         open(REF, ">$reffile") or die "Unable to open $reffile to write output\n";
      }
      for (my $i=0; $i<$block_size; ++$i)
      {
         my $k = $sorted[$processed++];
         print OUT "$k\n";
         print REF "$src_sents{$k}{'ref'}\n" if (defined($ref));
      }
      close(OUT);
      close(REF) if (defined($ref));
      ++$file_index;
   }

   # The last blocks may be smaller than the first.
   $block_size = floor($rest/$job);
   for (; $t<$job; ++$t)
   {
      my $filename = sprintf("$output.%4.4d", $file_index);
      open(OUT, ">$filename") or die "Unable to open $filename to write output\n";
      if (defined($ref)) {
         my $reffile = sprintf("$output.ref.%4.4d", $file_index);
         open(REF, ">$reffile") or die "Unable to open $reffile to write output\n";
      }
      for (my $i=0; $i<$block_size; ++$i)
      {
         my $k = $sorted[$processed++];
         print OUT "$k\n";
         print REF "$src_sents{$k}{'ref'}\n" if (defined($ref));
      }
      close(OUT);
      close(REF) if (defined($ref));
      ++$file_index;
   }
}

die "OUPS: load-balancing pb: $processed $index" if ($processed != $index);

