#!/usr/bin/perl -w -s
# @file ce_train.pl 
# @brief Learn a confidence estimation model
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

 ce_train.pl {options} model_file [ data_dir ]

=head1 DESCRIPTION

Train a confidence estimation (CE) model.  This program can either be
used to retrain the existing model in F<model_file>, or to create a
new model from a model description file, using option C<-ini> (see
below). For more information about model description files, do:

 ce_train.pl -help=desc

All required input files are assumed to exist in the F<data_dir>
directory (current directory if F<data_dir> is not specified). This
typically includes source, target and reference tokenized text file
(F<q.tok>, F<p.tok>, F<r.tok>), as well as feature functions extracted
from decoder information (eg F<p.score>, F<p.pmax>, F<p.plen>, etc.),
in other words any feature required by the model that is not
explicitly computed by the L<CE::feature> package.  For more
information about features, do

 ce_train.pl -help=features

The resulting model is saved to file F<model_file>.

=head1 OPTIONS

=over 1

=item -ini=ce_desc    

ce_spec specifies a model feature description file (For info: C<ce_train.pl -help=desc>). 

=item -path=P           

Colon-separated search path for feature-specific file arguments.
Default is to search for files relative to F<data_dir> first, then
relative to directory where F<model_file> resides.

=item -k=K            

Do k-fold cross validation [don't]

=item -max=M          

Only use M first training examples

=item -norm=N         

Use feature normalization method N: zeromean, minmax or none [zeromean].  

=item -verbose        

Be verbose

=item -debug          

Produce debugging info

=item -help,-h        

Print this message and exit

=back

=head1 SEE ALSO

ce.pl, ce_translate.pl.

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 Copyright 2009, Her Majesty in Right of Canada

=cut


use CE::model;
use CE::help;
use File::Spec;
use File::Temp qw(tempfile);

our($help, $h, $verbose, $debug, $ini, $path, $k, $norm, $min, $max);

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

$ini = "" unless defined $ini;
$path = "" unless defined $path;
$norm = 'zeromean' unless defined $norm;
$k = 0 unless defined $k;
$min = 1 unless defined $min;
$max = 0 unless defined $max;
$verbose = 0 unless defined $verbose;
$debug = 0 unless defined $debug;

my $model_name = shift || die "Missing argument: model_name";
my $data_dir = shift || ".";

my $model;
if ($ini) {
    $model = CE::model->new(ini_file=>$ini, verbose=>$verbose, debug=>$debug);
    # anticipate where model will reside to set model path:
    my (undef, $model_dir, undef) = File::Spec->splitpath($model_name);
    $model->modelPath(File::Spec->rel2abs($model_dir));
} else {
    $model = CE::model->new(load=>$model_name, verbose=>$verbose, debug=>$debug);
}

$model->modelPath($path) if $path;
my $dataset = $model->getData($data_dir, with_target=>1, min=>$min, max=>$max);

$model->train(norm=>$norm, kfold=>$k, tmpdir=>$data_dir);

$model->save($model_name);

$model->verbose("[Done.]\n");

exit 0;



sub kfold {
    my ($data, $K) = @_;
    $K = 5 unless defined $K;

    verbose("[Doing $K-fold cross-validation on %d training data items]\n", $#$data+1);
    my $sum_diff2 = 0;
    for (my $k = 0; $k < $K; ++$k) {
        verbose("[Fold no. %d/$K]\n", $k+1);
        my @D = ();
        my @T = ();
        for (my $i = 0; $i <= $#$data; ++$i) {
            if ($i % $K == $k) {
                push @T, $data->[$i];
            } else {
                push @D, $data->[$i];
            }
        }
        debug("Training on %d items\n", $#D+1);
        # my $svm = new Algorithm::SVM(Type=>'epsilon-SVR');
        my ($params_fh, $params_file) = tempfile("ce_train_kfold_${k}_XXXXXX", DIR=>".", UNLINK=>1);
        close $params_fh;
        train($params_file, \@D, tmpdir=>$data_dir);
        debug("Testing on %d items\n", $#T+1);
        my @y = predict($params_file, \@T);
        for (my $j = 0; $j <= $#T; ++$j) {
            my $v = dataLabel($T[$j]);
            $sum_diff2 += ($v-$y[$j])*($v-$y[$j]);
        }
    }
    verbose("[sqrt(Average squared diff) = %.6g]\n", sqrt($sum_diff2 / @$data));

    return $sum_diff2 / @$data;
}

