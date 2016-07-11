# @file model.pm
# @brief Confidence Estimation Model
# 
# @author Michel Simard
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

=pod

=head1 NAME

CE::model

=head1 SYNOPSIS

 use CE::model;

Create a model from scratch:

 my $ce1 = CE::model->new();
 $ce1->addFeature("p.clen");
 $ce1->addFeature("q.lm", lm=>"lm_file");
 ...
 $ce1->addFeature("pr.bleu", target=>1);  # every model needs one target feature

Or, preferably, create a model from a file description:

 my $ce2 = CE::model->new(ini_file=>"model.ini");

Train a model:

 $ce2->getData("training_data_dir");
 $ce2->train(norm=>"minmax", kfold=>5);

Use the model to estimate confidence

 $ce2->getData("test_data_dir");
 @ce_values = $ce2->predict();
 
Save the model to a file:

 $ce2->save("model_file"); # writes to file "model_file" -- it's a zip file, really

Load an existing model from a file "model_file"

 my $ce3 = CE::model->new(load=>"model_file");

or

 my $ce4 = CE::model->new();
 $ce4->load("model_file");

=head1 DESCRIPTION

A confidence estimation model.

=cut

package CE::model;

use Exporter;
use CE::base;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

$VERSION     = 1.00;
@ISA         = qw(Exporter CE::base);
@EXPORT      = ();
@EXPORT_OK   = qw(new load save getData setData train predict stats modelPath features addFeature readDesc writeDesc);
%EXPORT_TAGS = ();

use CE::normalize;
use CE::feature;
use CE::dataset;
use CE::data;
use CE::libsvm;

use strict;
use warnings;
use open IO=>':utf8';
use File::Spec qw(rel2abs);
use File::Temp qw(tempdir);

=pod

=head1 FUNCTIONS

=head2 new( [ %args ] )

Create a new CE::model object.  Optional args:

=over

=item C<ini_file=E<gt>"file">: Initialize model with description in
C<file>.  See section L<CE Model Description File> below for details
of this file.

=item C<load=E<gt>"file">

Load a pre-existing model from file, (see method C<save>). 

=item C<norm=E<gt>"method">

Use the given feature normalization method.  This is only effective
when called before calling C<train>. See L<CE::normalize>.

=item C<verbose=E<gt>1>: verbose mode

=back

=cut 

sub new {
    my ($this, %args) = @_;
    my $class = ref($this) || $this;
    my $self = {};
    bless $self, $class;
    $self->initialize(%args);
    return $self;
}

sub initialize {
    my ($this, %args) = @_;

    undef %$this;

    $this->SUPER::initialize(%args);

    $this->{features} = [ ];
    $this->{model_dir} = File::Spec->rel2abs("."); # internal knowledge of where we are
    $this->{model_path} = undef; # user-specified search path for feature argument files
    $this->{dataset} = undef;
    my $tmpdir = $this->{tmpdir} || File::Spec->tmpdir();

    my $dir = $this->{tmp_dir} = tempdir('ce_model_XXXXXX', DIR=>$tmpdir, CLEANUP=>1);
    $this->debug("[[Model temp dir: $dir]]\n");
    $this->{model_file} = "${dir}/model";
    $this->{norm_file} = "${dir}/norm";
    $this->{svm_file} = "${dir}/svm";

    $this->{norm_method} =  $args{norm} || "none";

    if (defined $args{ini_file}) {
        $this->readDesc($args{ini_file});
    } elsif (defined $args{load}) {
        $this->load($args{load});
    }

    return $this;
}

=pod

=head2 modelPath( [ $path ] )

Set/get the current model path.  $path is a colon-separated list of
directory names.

Some model features require file arguments, e.g. C<p.lm> which
requires an argument C<lm=lmfile> specifying a language model
file. These arguments are usually provided within a model description
file (see the section on L</"CE Model Description Files"> below). If
this argument is relative filename, then the runtime search for the
file is done in these directories, in the given order.

If modelPath is not explicitly specified, then the search proceeds
first in the data directory (see method L<getData> below), then
relative to the model description file (if the model was constructed
from such a file -- see constructor method L<new> above) or the model
file (if the model was loaded or saved onto a model file -- see
methods L<save> and L<load> below).

=cut 

sub modelPath {
    my ($this, $path) = @_;

    $this->{model_path} = $path if defined $path;

    return $this->{model_path};
}


=pod

=head2 features( )

Get model's list of features.

=cut 

sub features {
    my ($this, @features) = @_;

    $this->{features} = [ @features ] if @features;

    return @{$this->{features}};
}


=pod

=head2 addFeature( $name [, %args ] )

Add a feature to model.  Arguments are passed to CE::feature constructor.

=cut 

sub addFeature {
    my ($this, $name, %args) = @_;

    my $feature = CE::feature->new($name, %args);
    push @{$this->{features}}, $feature;

    return $feature;
}


=pod

=head2 getData( $directory [, %args ] )

Load model feature values from $directory; missing feature values will
be computed as needed (this method calls C<CE::feature::generate>).
Arguments that affect the behavior of this method:

=over

=item C<with_target=>1>

Each CE model contains a "target" feature, whose value is not normally
computed, except if this argument is set to a non-null value.

=back

=cut

sub getData {
    my ($this, $dir, %args) = @_;
    my $target_mode = $args{with_target};

    my $search_path = $this->modelPath() ? $this->modelPath() : join(":", $dir, $this->{model_dir});
    
    foreach my $f ($this->features()) {
        if ($target_mode or not $f->isTarget()) {
            $this->verbose("[Generating values for feature %s]\n", $f->name());
            $f->generate(dir=>$dir, path=>$search_path, %args);
        }
    }
    $this->{dataset} = $this->readDataset($dir, %args);
    return $this->{dataset};
}

=pod

=head2 setData( $dataset )

Set model's active dataset.

=cut

sub setData {
    my ($this, $dataset) = @_;

    $this->{dataset} = $dataset;
    return $this->{dataset};
}

=pod

=head2 train( [ %args ] )

Train CE model, using currently loaded data (C<CE::model::getData>
should have been called with optional argument C<with_target=E<gt>1>
prior to calling this method).

=cut 

sub train {
  my ($this, %args) = @_;

  $this->verbose("[Training model on all %d data points]\n", $this->{dataset}->size());

  $this->learnNormalize(%args);

  $CE::libsvm::verbose = $this->setVerbose();
  $CE::libsvm::debug = $this->setDebug();
  CE::libsvm::train($this->{svm_file}, $this->applyNormalize(%args), %args);
}


=pod

=head2 predict( [ %args ] )

Use model to estimate confidence for currenlty loaded dataset.  CE
values are returned as an array.  Note that dataset labels are not
affected.

=cut 

sub predict {
  my ($this, %args) = @_;

  $this->verbose("[Estimating confidence for %d data points]\n", $this->{dataset}->size());
  return CE::libsvm::predict($this->{svm_file}, $this->applyNormalize(%args), %args);
}


=pod

=head2 stats( )

Compare predicted confidence to labels.  Returns a hash with the
following key-value pairs: C<N> (sample size), C<min> (minimum
absolute error), C<max> (maximum absolute error), C<mean> (mean
error), C<mean_squared> (mean squared error) and C<mean_abs> (mean
absolute error).

=cut 

sub stats {
  my ($this, %args) = @_;

  my $D = $this->{dataset};
  die "Error: No data to get stats from" unless $D and not $D->empty();
  die "Error: Can't get stats from unlabeled data" unless defined $D->label(0);

  my @ce = $this->predict(%args);

  my $sum = 0;
  my $asum = 0;
  my $sum2 = 0; 
  my $N = 0;
  my $min = undef;
  my $max = undef;

  for (my $i = 0; $i <= $#ce; ++$i) {
      my $z = $ce[$i];
      my $y = $D->label($i);
      my $d = $z - $y;
      my $a = $d < 0 ? -$d : $d;
      
      $N++;
      $sum += $d;
      $asum += $a;
      $sum2 += $d*$d;
      $min = $d if not defined($min) or $d<$min;
      $max = $d if not defined($max) or $d>$max;
  }
  return (N=>$N, min=>$min, max=>$max, mean=>$sum/$N, mean_squared=>$sum2/$N, mean_abs=>$asum/$N);
}


=pod

=head2 load( $filename )

Load from $filename a model previously saved with CE::model::save.
This method not only loads the model description (see
CE::model::readDesc), but also the learned parameters.

=cut 

sub load {
  my ($this, $filename, %args) = @_;

  my $tmpdir = $this->{tmp_dir};
  $filename .= ".cem" unless $filename =~ /\.cem$/;
#  system "unzip -o -d $tmpdir ${filename}";
  system "tar --extract --gzip --directory ${tmpdir} --file ${filename}";

  $this->readDesc($this->{model_file});

  my ($vol, $dir, $name) = File::Spec->splitpath($filename);
  $this->{model_dir} = File::Spec->rel2abs($dir);
}

=pod

=head2 save( $filename )

Save model into file $filename.  This method saves not only the model
description (see CE::model::writeDesc), but also the learned
parameters.  See also CE::model::load.

=cut 

sub save {
  my ($this, $filename, %args) = @_;

  $filename .= ".cem" unless $filename =~ /\.cem$/;

  my ($vol, $dir, $name) = File::Spec->splitpath($filename);
  $this->{model_dir} = File::Spec->rel2abs($dir);

  my $tmpdir = $this->{tmp_dir};
  $this->writeDesc($this->{model_file});

#  system "zip -j ${filename} ${tmpdir}/model ${tmpdir}/norm ${tmpdir}/svm";
  system "tar --create --gzip --directory ${tmpdir} --file ${filename} model norm svm";
}

=pod

=head2 readDesc( $filename )

Read model description from $filename.

=cut 

sub readDesc {
    my ($this, $file, %args) = @_;

    my ($vol, $dir, $name) = File::Spec->splitpath($file);
    $this->{model_dir} = File::Spec->rel2abs($dir);

    $this->verbose("[Reading model description file $file]\n");
    open(my $in, "<$file") or die "Error: Can't open CE model description file $file";

    $this->{features} = [ ];
    my $target_feature = undef;
    while (my $line = <$in>) {
        chomp $line;
        $line =~ s/^\#.*$//;
        $line =~ s/([^\\])\#.*$/$1/;
        my @fields = trim(split(/:/, $line));
        if (@fields) {
            my $name = shift @fields;
            my %args = ();
            foreach my $opt (@fields) {
                my ($opt_name, $opt_value) = trim(split(/=/, $opt, 2));
                $args{$opt_name} = $opt_value || 1;
                if ($opt_name eq 'target') {
                    die "Error: Non-unique CE target feature in $file" if $target_feature;
                    $target_feature = $name;
                }
            }
            push @{$this->{features}}, CE::feature->new($name, %args);
        }
    }
    close $in;

    die "Error: Missing CE target feature in $file" unless $target_feature;

    return $this;
}

=pod

=head2 writeDesc( $filename )

Write model description to $filename.

=cut 

sub writeDesc {
    my ($this, $filename, %args) = @_;

    $this->verbose("[Writing model description file $filename]\n");
    open(my $out, ">$filename") or die "Error: Can't open output model file $filename";

    for my $f (@{$this->{features}}) {
        print {$out} $f->{name};
        if (exists($f->{args}) and $f->{args}) {
            for my $opt_name (keys %{$f->{args}}) {
                print {$out} ":", $opt_name;
                my $opt_value = $f->{args}{$opt_name};
                print {$out} "=", $opt_value if $opt_value;
            }
        }
        print {$out} "\n";
    }

    close $out;

}

sub readDataset {
    my ($this, $dir, %args) = @_;

    $dir = "." unless defined $dir;
    my $min = $args{min} || 1;
    my $max = $args{max} || 0;
    my $with_target = $args{with_target};
    my $dataset = CE::dataset->new();

    my $feature_index = 0;
    for my $feature ($this->features()) {
        next if $feature->isTarget() and not $with_target; # skip target feature

        my $filename = "${dir}/".$feature->name();
        $this->verbose("[Reading values for feature %s (%s) from %s]\n", 
                       $feature->name(), 
                       $feature->isTarget() ? "label" : $feature_index+1, 
                       $filename);
        open(my $in, "<$filename") or die "Error: Can't open feature value file $filename";

        my @values = ();
        my $count = 0;
        while (my $line = <$in>) {
            chop $line;
            ++$count;
            push @values, $line
                if ($min <= $count) and ((not $max) or ($count <= $max))
        }
        close $in;

        if ($dataset->empty()) {
            for my $value (@values) {
                $dataset->add(CE::data->new());
            }
        } else {
            die "Error: Wrong number of values in $filename"
                unless int(@values) == $dataset->size();
        }
        if ($feature->isTarget()) {
            for (my $j = 0; $j <= $#values; ++$j) {
                $dataset->label($j, $values[$j]);
            }
        } else {
            ++$feature_index;
            for (my $j = 0; $j <= $#values; ++$j) {
                $dataset->value($j, $feature_index, $values[$j]);
            }
        }
    }

    return $dataset;
}


#--------------------------------------------------------------------

sub learnNormalize {
    my ($this, %args) = @_;
    $this->{norm_method} = $args{norm} if defined $args{norm};

    die "Error: No data to learn normalization from"
        if $this->{dataset}->empty();
    my $f_count = $this->{dataset}->data(0)->size();

    my @norm = ();
    for (my $j = 1; $j <= $f_count; ++$j) {
        my $norm = CE::normalize->new($this->{norm_method});
        $norm->learn($this->{dataset}->getColumn($j));
        push @norm, $norm;
    }
    $this->writeNormalize(@norm);
}

sub applyNormalize {
    my ($this, %args) = @_;

    my @norm = $this->readNormalize();
    my $normdata = $this->{dataset}->copy();

    for (my $k = 0; $k < $this->{dataset}->size(); ++$k) {
        for (my $j = 1; $j <= $this->{dataset}->data($k)->size(); ++$j) {
            $normdata->value($k, $j, $norm[$j-1]->apply($normdata->value($k, $j)));
        }
    }

    return $normdata;
}

sub writeNormalize {
    my ($this, @norm) = @_;
    my $norm_file = $this->{norm_file};

    open(my $out, ">$norm_file") or die "Error: Can't open normalization file $norm_file";

    for my $norm (@norm) {
        print {$out} CE::normalize::toString($norm), "\n";
    }

    close $out;
}

sub readNormalize {
    my ($this) = @_;
    my $norm_file = $this->{norm_file};

    my @norm = ();

    open(my $in, "<$norm_file") or die "Error: Can't open normalization file $norm_file";

    while (my $line = <$in>) {
        chop $line;
        push @norm, CE::normalize->new($line);
    }

    close $in;

    return @norm;
}


## Utilities

sub trim { for my $s (@_) { $s =~ s/^\s+|\s+$//g; } return @_ }

CE::help::keyword("desc", __PACKAGE__."::CE Model Description File");

=pod

=head1 CE Model Description File

The model description file contains one line per feature,
each line of the form:

=over

C<FeatureName{:Args}+>

=back

Comments may appear within the file, preceded by the character "#"; in
particular, lines beginning with "#" are completely ignored.  

Feature arguments (e.g. language model file) are separated by the
character ":", and take the form C<arg=value>, or simply C<arg>,
which is taken to mean C<arg=1>.

Feature names should be unique.  If multiple features are specified
within a model description file with the same F<FeatureName>, then only
the first one encountered will be taken into account.

Any feature with an argument C<file=F> will be read from a file called
F<F>; in addition, if there is an argument C<field=N>, where C<N> is a
positive integer number, the feature is assumed to be the C<N>th
space-separated field in file F<F> (processing is performed with *nix
program C<cut>).

The model description should contain exactly one feature with a
C<target> argument, for example:

=over

=item C<pr.bleu:target>

=back 

Typically, values for this feature will only be computed when training
or testing a model, i.e. situations where a reference translation
(file F<r.tok>) is available.  Typical features are C<pr.bleu>,
C<pr.wer> and C<pr.u8>.

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

Technologies langagieres interactives / Interactive Language Technologies
Inst. de technologie de l'information / Institute for Information Technology
Conseil national de recherches Canada / National Research Council Canada
Copyright 2009, Sa Majeste la Reine du Chef du Canada /
Copyright 2009, Her Majesty in Right of Canada

=cut

1;
