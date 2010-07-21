#!/usr/bin/perl -s
# $Id$
# @file ce_tmx.pl 
# @brief Handle TMX files for confidence estimation
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

 ce_tmx.pl {options} extract dir [ tmx_in ]
 ce_tmx.pl {options} replace dir [ tmx_out ]
 ce_tmx.pl {options} check [ tmx_in ]

=head1 DESCRIPTION

TMX pre- and post-processing tools for translation with PORTAGE within
the confidence estimation framework.  

The first form extracts the
source-language text segments from a TMX file into a plain text file
(<dir>/Q.txt), and produces a TMX file template (<dir>/QP.template.tmx).

The second form inserts target-language translations from file
<dir>/P.txt into that template,  generating the output TMX file
<dir>/QP.tmx.  

The third form checks the validity of the file. If the file is valid,
the program outputs the number of translatable source-language
segments on the standard output and exits successfully; if the file is
not valid, it exits with a non-zero status and possibly a diagnostic
on the standard error stream.

=head1 OPTIONS

=over 1

=item -filter=T       In 'replace' mode, filter out TUs with confidence below T [don't] 

=item -src=SL         Specify TMX source language [EN-CA]

=item -tgt=TL         Specify TMX target language [FR-CA]

=item -verbose        Be verbose

=item -Verbose        Be more verbose

=item -debug          Produce debugging info

=item -help,-h        Print this message and exit

=head1 SEE ALSO

ce.pl, ce_train.pl, ce_translate.pl, ce_gen_features.pl, ce_ttx2ospl.pl, ce_canoe2ffvals.pl.

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 Copyright 2009, Her Majesty in Right of Canada

=cut

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
printCopyright("ce_tmx.pl", 2009);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


use strict;
use warnings;
use XML::Twig;
use Time::gmtime;

$|=1;

my $output_layers = ":raw:encoding(UTF8):utf8";
my $input_layers = ":utf8";

my $DEFAULT_SRC="EN-CA";
my $DEFAULT_TGT="FR-CA";

our ($h, $help, $verbose, $Verbose, $debug, $src, $tgt, $filter);

if ($h or $help) {
    -t STDOUT ? system "pod2usage -verbose 3 $0" : system "pod2text $0";
    exit 0;
}

$verbose = 0 unless defined $verbose;
$Verbose = 0 unless defined $Verbose;
$debug = 0 unless defined $debug;

$filter = undef unless defined $filter;
$src = $DEFAULT_SRC unless defined $src;
$tgt = $DEFAULT_TGT unless defined $tgt;

my $action = shift or die "Missing argument: action";

if ($action eq 'extract') {
    my $dir = shift or die "Missing argument: dir";
    die "No such directory: $dir" unless -d $dir;

    my $tmx_file = shift || "-";
    open(my $tmx_out, ">${output_layers}", "${dir}/QP.template.tmx")
        or die "Can open output TMX template file";
    my $ix = newIx();
    processTMX(action=>'extract', tmx_in=>$tmx_file, 
               tmx_out=>$tmx_out, ix=>$ix,
               src_lang=>$src, tgt_lang=>$tgt);
    close $tmx_out;
    ixSave($ix, "${dir}/Q.txt", "${dir}/Q.ix");
} 

elsif ($action eq 'replace') {
    my $dir = shift or die "Missing argument: dir";
    die "No such directory: $dir" unless -d $dir;

    my $tmx_file = shift || "-";
    my $ce_file = defined $filter ? "${dir}/pr.ce" : undef;
    my $ix = ixLoad("${dir}/P.txt", "${dir}/Q.ix", $ce_file);
    
    open(my $tmx_out, ">${output_layers}", "${dir}/QP.tmx")
        or die "Can open output TMX template file";
    processTMX(action=>'replace', tmx_in=>"${dir}/QP.template.tmx", 
               tmx_out=>$tmx_out, ix=>$ix, filter=>$filter,
               src_lang=>$src, tgt_lang=>$tgt);
    close $tmx_out;
} 

elsif ($action eq 'check') {
    my $tmx_file = shift || "-";
    my $info = processTMX(action=>'check', tmx_in=>$tmx_file, 
                       src_lang=>$src, tgt_lang=>$tgt);
    print $info->{seg_count}, "\n";
} 

else {
    die "Unsupported action $action";
}


exit 0;

sub processTMX {
    my %args = @_;

    my $parser = XML::Twig->new( twig_handlers=> { tu => \&processTU, 
                                                ph => \&processPH, 
                                                hi => \&processHI },
                                 );
                                 # keep_encoding=>0, 
                              # output_encoding=>'UTF-8' );

    @{$parser}{keys %args} = values %args; # Merge args into parser
    $parser->{tu_count} = 0;
    $parser->{seg_count} = 0;
    $parser->{filter_count} = 0;

    verbose("[Processing TMX file %s ...]\n", $parser->{tmx_in});
    $parser->parsefile($parser->{tmx_in});

    xmlFlush($parser);
    verbose("\r[%d%s segments processed. Done]\n", $parser->{seg_count}, 
            exists $parser->{total} ? "/".$parser->{total} : "" );
    verbose("[%d TUs filtered out]\n", $parser->{filter_count}) 
        if defined $parser->{filter};
    return $parser;
}

sub processPH {
   my ($parser, $ph) = @_;
   my $string = join(" ", map(normalize($_->text(no_recurse=>1)), $ph));

   # Special treatment for \- \_ in compounded words.
   if ($string =~ /\\([-_])/) {
      $ph->set_text($1);
      $ph->erase();
   }
}

sub processHI {
   my ($parser, $hi) = @_;
   $hi->erase();
}

sub xmlFlush {
    my ($parser) = @_;
    $parser->{action} eq 'check'
        ? $parser->purge()
        : $parser->flush($parser->{tmx_out}, pretty_print=>'indented');
}

sub processTU {
    my ($parser, $tu) = @_;

    my @tuvs = $tu->children('tuv');
    warn("Missing variants in TU") unless @tuvs;

    $parser->{seg_id} = ""; # will be set in processText()

    # Extraction mode: find src-lang text segments and replace with placeholder ID
    if ($parser->{action} eq 'extract' or $parser->{action} eq 'check') {
        my $new_tgt_tuv = 0;
        my $old_tgt_tuv = 0;
        foreach my $tuv (@tuvs) {
            my $lang = $tuv->{att}->{'xml:lang'};
            warn("Missing language attribute in TU") unless $lang;

            # Use the src-lang TUV as a template for the new tgt-lang TUV
            if ($lang eq $parser->{src_lang}) {
                warn("Duplicate source-language tuv\n") if $new_tgt_tuv;
                $new_tgt_tuv = $tuv->copy();
                $new_tgt_tuv->set_att('xml:lang' => $parser->{tgt_lang});
                my $seg = $new_tgt_tuv->first_child('seg'); # There should be exactly one
                processSegment($parser, $seg) if $seg;

            # Keep a handle on the old tgt-lang TUV: it will be replaced
            } elsif ($lang eq $parser->{tgt_lang}) {
                $old_tgt_tuv = $tuv;
            }
        }

        if ($new_tgt_tuv) {
            if ($old_tgt_tuv) {
                $new_tgt_tuv->replace($old_tgt_tuv);
            } else {
                $new_tgt_tuv->paste(last_child => $tu);
            }
        } else {
            warn("Missing source-language version in TU");
        }
        xmlFlush($parser);
    }

    # Replacement mode: find placeholder ID, replace with text
    elsif ($parser->{action} eq 'replace') {

        foreach my $tuv (@tuvs) {
            my $lang = $tuv->{att}->{'xml:lang'};
            warn("Missing language attribute in TU") unless $lang;

            if ($lang eq $parser->{tgt_lang}) {
                my $seg = $tuv->first_child('seg'); # There should be exactly one
                if (not $seg) {
                    warn "No SEG element in target language TUV";
                } else {
                    processSegment($parser, $seg) if $seg;
                }
            }
            $tu->set_att(changeid => 'MT!');
            $tu->set_att(changedate => timeStamp());
        }

        # CE-filtering:
        my ($id, $ce);
        $id = $parser->{seg_id};
        $ce = ($id and defined $parser->{filter}) ? ixGetCE($parser->{ix}, $id) : undef;
        debug("Filtering on $id: CE=%s\n", defined $ce ? $ce : "undef");
        if (defined $parser->{filter}
            and ($id = $parser->{seg_id})
            and defined($ce = ixGetCE($parser->{ix}, $id))
            and $ce < $parser->{filter}) {
            debug("Filtering out $id\n");
            $tu->delete();       # BOOM!
            $parser->{filter_count}++;
        } else {
            xmlFlush($parser);
        }
    }

    $parser->{tu_count}++;
}

sub processSegment {
    my ($parser, $seg) = @_;

    my @children = $seg->cut_children();
    my @src_seg = ();

    foreach my $child (@children) {
        if ($child->is_text()) {
            push @src_seg, $child->text();
        } else {
            my $name = $child->name();
            warn "Unknown element in a seg: $name\n" unless ($name =~ /^(bpt|ept|it|ph|ut)$/);
            foreach my $sub ($child->descendants(qr/sub/)) {
                my $src_sub = $sub->text();
                $sub->set_text(processText($parser, $src_sub));
                $parser->{seg_count}++;
                if ($Verbose) {
                    veryVerbose("[replacing sub source ``%s'' --> ``%s'']\n", 
                                $src_sub, $sub->text());
                } elsif ($verbose) {
                    verbose("\r[%d%s...]", $parser->{seg_count}, 
                            exists $parser->{total} ? "/".$parser->{total} : "" );
                }
            }
            $child->paste(last_child => $seg);
        }
    }

    my $old_text = join(" ", @src_seg);
    my $new_text = processText($parser, $old_text);
    $parser->{seg_count}++;
    if ($Verbose) {
        veryVerbose("[replacing sub source ``%s'' --> ``%s'']\n", 
                    $old_text, $new_text);
    } elsif ($verbose) {
        verbose("\r[%d%s...]", $parser->{seg_count}, 
                exists $parser->{total} ? "/".$parser->{total} : "" );
    }
    XML::Twig::Elt->new('#PCDATA', $new_text)->paste(first_child => $seg);
    return $seg;
}

sub processText {
    my ($parser, $in) = @_;
    my $out = "";

    if ($parser->{action} eq 'extract') {
        $out = ixAdd($parser->{ix}, $in);
        $parser->{seg_id} = $out; # Ugly side-effect
    } elsif ($parser->{action} eq 'replace') {
        $out = ixGetSegment($parser->{ix}, $in);
        $parser->{seg_id} = $in;# Ugly side-effect
        warn("Can't find ID $in in index") unless $out;
    }

    return $out;
}

sub timeStamp() {
    my $time = gmtime();
    
    return sprintf("%04d%02d%02dT%02d%02d%02dZ",
                   $time->year + 1900, $time->mon+1, $time->mday,
                   $time->hour, $time->min, $time->sec);
}

sub normalize {
    my ($text) = @_;
    
    $text =~ s/[\n\r\t\f]/ /g; # Newlines etc. are converted to spaces
    $text =~ s/ +/ /g;         # Multiple spaces are compressed;
    $text =~ s/^ +//g;          # Remove leading and trailing spaces
    $text =~ s/ +$//g;          # Remove leading and trailing spaces

    return $text;
}

sub newIx {
    return { id_seed=>0, id=>{}, segment=>{}, ce=>{} };
}

sub ixAdd {
    my ($ix, $segment, $id, $ce) = @_;
    $segment = normalize($segment);

    if (defined $id or not exists($ix->{id}{$segment})) {
        if (defined $id) {
            $id = normalize($id);
        } else {
            $id = ixNewID($ix);
        }
        debug("ixAdd: INSERTING id=%s, ce=%s, segment=\"%s\"\n", $id, defined $ce ? $ce : "undef", $segment);
        $ix->{id}{$segment} = $id;
        $ix->{segment}{$id} = $segment;
        $ix->{ce}{$id} = $ce if defined $ce;
    } else {
        debug("ixAdd: EXISTS id=%s, ce=%s, segment=\"%s\"\n", $id, defined $ce ? $ce : "undef", $segment);
    }
    return $ix->{id}{$segment};
}

sub ixNewID {
    my ($ix) = @_;

    my $n = $ix->{id_seed} + 1;
    while (exists $ix->{segment}{"seg_".$n}) {
        ++$n;
    }
    $ix->{id_seed} = $n;
    
    return "seg_".$n;
}

sub ixGetID {
    my ($ix, $segment) = @_;
    $segment = normalize($segment);
    return exists($ix->{id}{$segment}) ? $ix->{id}{$segment} : undef;    
}

sub ixGetSegment {
    my ($ix, $id) = @_;
    $id = normalize($id);
    return exists($ix->{segment}{$id}) ? $ix->{segment}{$id} : undef;
}

sub ixGetCE {
    my ($ix, $id) = @_;
    $id = normalize($id);
    return exists($ix->{ce}{$id}) ? $ix->{ce}{$id}+0 : undef;
}

sub ixSave {
    my ($ix, $seg_file, $id_file) = @_;

    open(my $seg_out, ">${output_layers}", $seg_file) 
        or die "Can't open output file $seg_file";
    open(my $id_out, ">${output_layers}", $id_file) 
        or die "Can't open output file $id_file";

    for my $id (sort keys %{$ix->{segment}}) {
        my $seg = $ix->{segment}{$id};
        print {$seg_out} portageEscape($seg), "\n";
        print {$id_out} $id, "\n";
    }

    close $seg_out;
    close $id_out;
}

sub ixLoad {
    my ($seg_file, $id_file, $ce_file) = @_;
    my $ix = newIx();

    open(my $seg_in, "<$input_layers", $seg_file)
        or die "Can open input file $seg_file";
    open(my $id_in, "<$input_layers", $id_file)
        or die "Can open input file $id_file";
    open(my $ce_in, "<$input_layers", $ce_file)
        or die "Can open input file $ce_file"
        if defined $ce_file;

    my $count = 0;
    verbose("[Reading index from $seg_file and $id_file]\n");
    verbose("[Reading CE from $ce_file]\n") if defined $ce_file;
    while (my $id = <$id_in>) {
        chop $id;
        my $seg = readline($seg_in);
        die "Not enough lines in text file $seg_file" unless defined $seg;
        chop $seg;
        my $ce = 0;
        if ($ce_file) {
            $ce = readline($ce_in);
            die "Not enough lines in CE file $ce_file" unless defined $ce;
            chop $ce;
        }
        debug("ixLoad: read $id <<$seg>> ($ce)\n");
        ixAdd($ix, $seg, $id, $ce);
        verbose("\r[%d lines...]", $count) if (++$count % 1 == 0);
    }
    verbose("\r[%d lines; done.]\n", $count);
    die "Too many lines in text file $seg_file" unless eof $seg_in;

    close $seg_in;
    close $id_in;
    close $ce_in if defined $ce_file;

    return $ix;
}

sub portageEscape {
    my ($s) = @_;
    $s =~ s/[\\<>]/\\\&/g;
    return $s;
}

sub verbose { printf STDERR (@_) if ($verbose or $Verbose) ; }
sub veryVerbose { printf STDERR (@_) if $Verbose; }
sub debug { printf STDERR (@_) if $debug; }

1;
