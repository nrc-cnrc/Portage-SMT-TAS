#!/bin/sh
#! -*-perl-*-
eval 'exec perl -x -s -wS $0 ${1+"$@"}'
   if 0;

use warnings;

# @file ce.pl 
# @brief Compute confidence estimation
# 
# @author Michel Simard
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

=pod 

=head1 SYNOPSIS

ce.pl {options} ce_model [ data_dir ]

=head1 DESCRIPTION

Compute confidence estimation, using the given ce_model.  All input
files (i.e. files containing data from which CE model feature values
are computed, e.g. F<q.tok>, F<P.tok>, F<p.score>, etc.) are assumed
to exist in the F<data_dir> directory, or the current directory if
F<data_dir> is not specified.  Output is written in a file called
F<pr.ce> within that directory, unless option C<-output> is specified.

=head1 OPTIONS

=over 1

=item -output=O         

Output file name [F<data_dir/pr.ce>]

=item -path=P           

Colon-separated search path for feature-specific file arguments.
Default is to search for files relative to F<data_dir> first, then
relative to directory where F<ce_model> resides.

=item -stats          

Write prediction accuracy statistics to STDERR; this assumes the presence
in F<data_dir> of reference translations for the source text (file
F<r.tok>).

=item -verbose          

Be verbose

=item -debug          

Produce debugging info

=item -help, -h        

Print this message and exit

=item -help=desc, -help=features, -help=depend

Print additional help information: desc: model description files; features:
confidence estimation features; depend: feature dependencies.

=item -H

Print all additional help information available.

=back

=head1 SEE ALSO

ce_train.pl, ce_translate.pl.

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 Copyright 2009, Her Majesty in Right of Canada

=cut

use strict;

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
printCopyright("ce.pl", 2009);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


use CE::model;
use CE::help;

our($help, $h, $H, $verbose, $debug, $output, $path, $stats);

if ($help or $h) {
    $help = $h unless defined $help;
    if ($help eq '1') {
        -t STDOUT ? system "pod2usage -verbose 3 $0" : system "pod2text $0";
    } else {
        my $msg = CE::help::text($help); 
        if ($msg) {
            print STDERR $msg; 
        } else {
            print STDERR "No help on \"$help\"; ";
            print STDERR "known help keywords are: ", join(", ", CE::help::keywords()), ".\n";
        }
    }
    exit 0;
}
if ($H) {
    for (CE::help::keywords()) {
        print STDERR CE::help::text($_), "\n";
    }
    exit 0;
}

my $model_name = shift or die "Error: Missing argument: model";
my $data_dir = shift || ".";

$output = "${data_dir}/pr.ce" unless defined $output;
$path = "" unless defined $path;
$stats = 0 unless defined $stats;
$verbose = 0 unless defined $verbose;
$debug = 0 unless defined $debug;

my $model = CE::model->new(load=>$model_name, tmpdir=>$data_dir, 
                           verbose=>$verbose, debug=>$debug);
$model->modelPath($path) if $path;
$model->getData($data_dir, with_target=>$stats);
if ($stats) {
    my %S = $model->stats();
    print STDERR sprintf("CE stats: N=%d, min=%.2f, max=%.2f, mean=%.2f, mean_squared=%.2f, mean_abs=%.2f]\n",
                         $S{N}, $S{min}, $S{max}, $S{mean}, $S{mean_squared}, $S{mean_abs});
}

my @ce = $model->predict(tmpdir=>$data_dir);

$model->verbose("[Writing output to $output]\n");
open(my $out, ">$output") or die "Error: Can't open output file ${output}";
for my $y (@ce) {
    print {$out} $y, "\n";
}
close $out;
$model->verbose("[Done.]\n");

exit 0;

