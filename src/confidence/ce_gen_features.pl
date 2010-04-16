#!/usr/bin/perl -w -s
# $Id$
# @file ce_gen_features.pl 
# @brief Generate feature values for confidence estimation
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

ce_gen_features.pl {options} ce_model [ data_dir ]

=head1 DESCRIPTION

Compute feature values for confidence estimation model F<ce_model> and data in directory F<data_dir> (current directory if this is not given). See C<ce.pl -help> for more information.

=head2 IMPORTANT NOTE

Normally, you shouldn't have to call this program, as CE model
features are normally computed implicitly be C<ce.pl> and
C<ce_train.pl>.  This program is only provided for debugging purposes.


=head1 OPTIONS

=over 1

=item -target         Also produce values for target feature

=item -new            With this option, argument F<ce_model> is assumed to be model description file; see C<ce.pl -help=desc> for more details. 

=item -verbose        Be verbose

=item -debug          Produce debugging info

=item -help,-h        Print this message and exit

=item -help=desc    Get help on model_desc syntax

=item -help=features  Get help on known features

=item -help=depend    Get help on feature dependencies

=back

=head1 SEE ALSO

 ce_gen_features.pl -help=features
 ce_gen_features.pl -help=desc
 ce_gen_features.pl -help=depend

ce.pl, ce_train.pl, ce_translate.pl.

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
printCopyright("ce_gen_features.pl", 2009);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


use CE::model;
use CE::help;

our($help, $h, $verbose, $debug, $target, $new, $path);

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

$target = 0 unless defined $target;
$new = 0 unless defined $new;
$verbose = 0 unless defined $verbose;
$debug = 0 unless defined $debug;

my $model_name = shift || die "Missing argument: ce_model";
my $data_dir = shift || ".";

my $model;

if ($new) {
    $model = CE::model->new(ini_file=>$model_name, verbose=>$verbose, debug=>$debug);
} else {
    $model = CE::model->new(load=>$model_name, verbose=>$verbose, debug=>$debug);
}
$model->getData($data_dir, with_target=>$target);

exit 0;

