#!/usr/bin/perl -sw

# * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# * Institut de technologie de l'information / Institute for Information Technology
# * Conseil national de recherches Canada / National Research Council Canada
# * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
# * Copyright 2006, Her Majesty in Right of Canada

use strict;

our($h, $help, $nbest, $ffvals, $f);

####################
# COPYRIGHT
print(STDERR "\nCalculateHypothesisProb.pl, Copyright (c) 2006, Sa Majeste la Reine du Chef du Canada / Her Majesty in Right of Canada\n\n");


################################################################################
# HELP HELP HELP
my $HELP = "\n$0 calculates P(T|S) for every hypothesis in a NBest list.\n\nUSAGE: $0 -nbest=<file> -f=<model> -ffvals=<file>.\n\t-nbest\tFile containing the hypotheses [nbest]\n\t-ffvals\tFile containing the hypotheses feature values [ffvals]\n\t-fFile containing the configuration use to generate the hypotheses and feature values [canoe.ini.cow]\n";
if (defined $h or defined $help)
{
    print(STDERR $HELP);
    exit 1;
}

##############################
# SET DEFAULT VALUES
$nbest  = "nbest"         unless defined($nbest);
$ffvals = "ffvals"        unless defined($ffvals);
$f      = "canoe.ini.cow" unless defined($f);


open(NBEST, "<$nbest") or die "unable to open nbest list";
open(FFVALS, "<$ffvals") or die "unable to open ffvals";
open(CANOE, "<$f") or die "Unable to open the config file";

########################################
# PARSE THE CONFIG FILE
my %weights=();  # Temporarily holds the models' weights
while(defined(my $cfg = <CANOE>))
{
   push(@{$weights{"d"}}, split(":", $1)) if($cfg =~ /\[weight-d\] (.*)/);
   push(@{$weights{"w"}}, split(":", $1)) if($cfg =~ /\[weight-w\] (.*)/);
   push(@{$weights{"s"}}, split(":", $1)) if($cfg =~ /\[weight-s\] (.*)/);
   push(@{$weights{"l"}}, split(":", $1)) if($cfg =~ /\[weight-l\] (.*)/);
   push(@{$weights{"t"}}, split(":", $1)) if($cfg =~ /\[weight-t\] (.*)/);
}
# This feature might not be used, requires a special treatment :D
@{$weights{"s"}} = () unless defined(@{$weights{"s"}});

my $NumFF = 0;
print(STDERR "Your models' weights:\n");
while ( my ($model, $w) = each (%weights))
{
   $NumFF += scalar(@{$w});
   print STDERR $model . ": " . join(" ", @{$w}) . "\n";
}
print(STDERR "Number of features detected: $NumFF\n");


########################################
# CALCULATE THE PROB FOR EACH HYPOTHESIS
my $ff;
my $hyp;
my @w;
push(@w, @{$weights{"d"}}, @{$weights{"w"}}, @{$weights{"s"}}, @{$weights{"l"}}, @{$weights{"t"}});
while(defined($ff = <FFVALS>) and defined($hyp = <NBEST>))
{
   my $prob = 0;
   my @ff1 = split("[[:space:]]", $ff);

   # Makes sure there is the same number of features as feature values
   die "Invalid number of features\n" if (scalar(@ff1) != $NumFF);  

   for(my $i=0; $i<$NumFF; $i++)
   {
      $prob +=  $ff1[$i] * $w[$i];
   }

   chomp($hyp);
   print exp($prob) . "\t$hyp\n";
}
