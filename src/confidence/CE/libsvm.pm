#!/usr/bin/perl -s

# @file libsvm.pm 
# @brief Quick-and-dirty Perl API for libsvm
# 
# @author Michel Simard
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

package CE::libsvm;

=pod

=head1 NAME

CE::libsvm 

=head1 SYNOPSIS

 use CE::libsvm qw(train predict);

 # Get your training dataset, as a labeled CE::dataset:
 my $training_set = CE::dataset::new(); 
 ...

 # Train the model
 train("model.svm", $training_set, tmpdir=>"work-tmp");

 # Get your test dataset, as a CE::dataset, possibly unlabeled:
 my $test_set = CE::dataset::new(); 
 ...

 # Predict labels:
 my @labels = predict("model.svm", $test_set, tmpdir=>"work-tmp");


=head1 DESCRIPTION

Quick-and-dirty libsvm API for confidence estimation.  Uses temporary
files for communication with libsvm programs. Temporary files are
created using L<File::Temp/tempfile>.  If C<tmpdir> is not specified,
falls back to C<tempfile> default.

=head1 SEE ALSO

L<CE::dataset>, L<CE::data>.

=cut

use strict;
use warnings;
use File::Temp qw(tempfile);
use CE::dataset;
use CE::data;

use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

$VERSION     = 1.00;
@ISA         = qw(Exporter);
@EXPORT      = ();
@EXPORT_OK   = qw(train predict);
%EXPORT_TAGS = ();

# our $LIBSVM_PATH = "/home/simardmi/soft/libsvm-2.89";
our $verbose = 0;
our $debug = 0;

# Train model (filename) using dataset (see DataSet functions below)
sub train {
    my ($model, $dataset, %args) = @_;

    my $kfold = defined $args{kfold} ? $args{kfold}+0 : 0;

    my %tempargs = ();
    if (defined $args{tmpdir}) {
        $tempargs{DIR} = $args{tmpdir};
        $tempargs{UNLINK} = 0;
    } else {
        $tempargs{TMPDIR} = 1;
        $tempargs{UNLINK} = 1;
    }
    my ($datafh, $datafile) = tempfile("CE_libsvm_train_in_XXXXXX", %tempargs);
    writeDataset($dataset, $datafh);
    close $datafh;

    my $options = "-s 4 -h 0"; # -q";

    if ($kfold) {
        my $cmd = "svm-train  -v ${kfold} ${options} ${datafile}";
        debug("Calling: $cmd\n");

        system($cmd) == 0 
            or die sprintf("Error: Command \"%s\" failed: $?", $cmd);
    }
    my $cmd = "svm-train ${options} ${datafile} ${model} 1>&2"; # because svm-predict writes verbose-style output to stdout";
    debug("Calling: $cmd\n");

    system($cmd) == 0 
        or die sprintf("Error: Command \"%s\" failed: $?", $cmd);
}

# predict labels for dataset using model (filename)
sub predict {
    my ($model, $dataset, %args) = @_;

    my %tempargs = ();
    if (defined $args{tmpdir}) {
        $tempargs{DIR} = $args{tmpdir};
        $tempargs{UNLINK} = 0;
    } else {
        $tempargs{TMPDIR} = 1;
        $tempargs{UNLINK} = 1;
    }
    my ($datafh, $datafile) = tempfile("CE_libsvm_predict_in_XXXXXX", %tempargs);
    writeDataset($dataset, $datafh);
    close $datafh;

    my ($outfh, $outfile) = tempfile("CE_libsvm_predict_out_XXXXXX", %tempargs);
    close $outfh;

    my $options = "";
    my $cmd = "svm-predict ${options} ${datafile} ${model} ${outfile} 1>&2"; # because svm-predict writes verbose-style output to stdout
    debug("Calling: $cmd\n");

    system($cmd) == 0 
        or die sprintf("Error: Command \"%s\" failed: $?", $cmd);

    open(my $out, "<$outfile") or die "Error: Can't open ${outfile}";
    my @ce = ();
    while (my $line = <$out>) {
        push @ce, $line + 0;
    }
    close $out;

#    unlink $outfile;

    return @ce;
}

## Utilities

sub writeDataset {
    my ($dataset, $fh) = @_;

    for (my $i = 0; $i < $dataset->size(); ++$i) {
        my $data = $dataset->data($i);
        print {$fh} sprintf("%.6f", $data->label() ? $data->label() : 0);

        for (my $j = 1; $j <= $data->size(); ++$j) {
            print {$fh} sprintf(" %d:%.6f", $j, $data->value($j));
        }
        print {$fh} "\n";
    }    
}



sub verbose { printf STDERR @_ if $verbose }
sub debug { printf STDERR @_ if $debug }

1;

=pod

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

Technologies langagieres interactives / Interactive Language Technologies
Inst. de technologie de l'information / Institute for Information Technology
Conseil national de recherches Canada / National Research Council Canada
Copyright 2009, Sa Majeste la Reine du Chef du Canada /
Copyright 2009, Her Majesty in Right of Canada

=cut
