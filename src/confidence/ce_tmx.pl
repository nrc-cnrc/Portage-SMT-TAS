#!/usr/bin/env perl
# $Id: ce_tmx.pl,v 1.15 2012/09/07 18:13:00 joanise Exp $
# @file ce_tmx.pl
# @brief Handle TMX files for confidence estimation
#
# @author Michel Simard & Samuel Larkin
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

 ce_tmx.pl {options} extract dir [ xml_in ]
 ce_tmx.pl {options} replace dir
 ce_tmx.pl {options} check [ xml_in ]

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

=item -filter=T

In 'replace' mode, filter out TUs with confidence below T [don't]

=item -src=SL

Specify TMX source language (case insensitive within TMX file) [EN-CA]

=item -tgt=TL

Specify TMX target language (case insensitive within TMX file) [FR-CA]

=item -keeptags

Retain keeptags code (BPT, EPT, IT and PH elements) within target
language TUs (default is to delete it)

=item -score/-noscore

In 'replace' mode, do/don't insert confidence scores inside TUV's,
using TMX PROP elements of type Txt::CE. (default is -score)

=item -verbose=1

Be verbose

=item -Verbose=1

Be more verbose

=item -debug

Produce debugging info

=item -help,-h

Print this message and exit

=head1 SEE ALSO

ce.pl, ce_train.pl, ce_translate.pl, ce_gen_features.pl, ce_ttx2ospl.pl, ce_canoe2ffvals.pl.

=head1 AUTHOR

Michel Simard & Samuel Larkin

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
use encoding "UTF-8";
binmode(STDIN, ":encoding(UTF-8)");
binmode(STDOUT, ":encoding(UTF-8)");
binmode(STDERR, ":encoding(UTF-8)");

use Getopt::Long;
Getopt::Long::GetOptions(
   help           => sub { displayHelp(); exit 0 },
   "verbose=i"    => \my $verbose,
   "Verbose=i"    => \my $Verbose,
   debug          => \my $debug,
   "src=s"        => \my $src,
   "tgt=s"        => \my $tgt,
   "filter=s"     => \my $filter,
   keeptags       => \my $keeptags,
   score          => \my $score,
   noscore        => \my $noscore,
) or do { displayHelp(); exit 1 };

$|=1;

my $output_layers = ":raw:encoding(UTF8):utf8";
my $input_layers = ":utf8";

my $DEFAULT_SRC="EN-CA";
my $DEFAULT_TGT="FR-CA";

$verbose = 0 unless defined $verbose;
$Verbose = 0 unless defined $Verbose;
$debug = 0 unless defined $debug;

$keeptags = 0 unless defined $keeptags;
$filter = undef unless defined $filter;
$src = $DEFAULT_SRC unless defined $src;
$tgt = $DEFAULT_TGT unless defined $tgt;
die "Only one of -score and -noscore can be specified"
    if (defined($score) and defined($noscore));
$score = (defined($score) ? $score :
          defined($noscore) ? not $noscore :
          1);

my $action = shift or die "Missing argument: action";

if ($action eq 'extract') {
   my $dir = shift or die "Missing argument: dir";
   die "No such directory: $dir" unless -d $dir;

   my $xml_file = shift || "-";
   my $ix = newIx();
   my $parser = processFile(action => 'extract',
         xml_in => $xml_file,
         template_name => "${dir}/QP.template.xml",
         ix => $ix,
         src_lang => $src,
         tgt_lang => $tgt,
         keeptags => $keeptags);
   close $parser->{template} if(defined($parser->{template}));
   ixSave($ix, "${dir}/Q.txt", "${dir}/Q.ix");
}

elsif ($action eq 'replace') {
   my $dir = shift or die "Missing argument: dir";
   die "No such directory: $dir" unless -d $dir;

   my $ce_file = (-r "${dir}/pr.ce") ? "${dir}/pr.ce" : undef;
   my $ix = ixLoad("${dir}/P.txt", "${dir}/Q.ix", $ce_file);

   open(my $template, ">${output_layers}", "${dir}/QP.xml")
      or die "Can open output template file";
   processFile(action => 'replace',
         xml_in => "${dir}/QP.template.xml",
         template => $template,
         ix => $ix,
         filter => $filter,
         src_lang => $src,
         tgt_lang => $tgt,
         keeptags => $keeptags,
         score => $score);
   close $template;
}

elsif ($action eq 'check') {
   my $xml_file = shift || "-";
   my $info = processFile(action => 'check',
         xml_in => $xml_file,
         src_lang => $src,
         tgt_lang => $tgt);
   print $info->{seg_count}, "\n";
}

else {
   die "Unsupported action $action";
}


exit 0;

sub processFile {
   my %args = @_;

   my $parser = XML::Twig->new(
         start_tag_handlers => { xliff => \&processXLIFF, tmx => \&processTMX },
         );

   @{$parser}{keys %args} = values %args; # Merge args into parser
   $parser->{tu_count} = 0;
   $parser->{seg_count} = 0;
   $parser->{filter_count} = 0;
   $parser->{'tool-id'} = 'Portage-1.5.0';
   $parser->{'tool-name'} = 'Portage';
   $parser->{'tool-version'} = '1.5.0';
   $parser->{'tool-company'} = 'CNRC-NRC';

   verbose("[Processing file %s ...]\n", $parser->{xml_in});
   $parser->parsefile($parser->{xml_in});

   xmlFlush($parser);

   die "Unrecognized format.\n" unless(defined($parser->{InputFormat}));

   verbose("\r[%d%s segments processed. Done]\n",
         $parser->{seg_count},
         exists $parser->{total} ? "/".$parser->{total} : "" );
   verbose("[%d TUs filtered out]\n", $parser->{filter_count})
      if defined $parser->{filter};
   return $parser;
}


# If this xml file is a tmx, set the proper handlers for it.
sub processTMX {
   my ($parser, $elt) = @_;
   $parser->setTwigHandlers( {
      tu  => \&processTU,
      ph  => \&processNativeCode,
      bpt => \&processNativeCode,
      ept => \&processNativeCode,
      it  => \&processNativeCode,
      hi  => \&processHI,
      } );
   $parser->{InputFormat} = 'tmx';
   open(my $template, ">${output_layers}", $parser->{template_name})
      or die "Can open output template file";
}

use File::Copy;
# If this xml file is a sdlxliff, set the proper handlers for it.
sub processXLIFF {
   my ($parser, $elt) = @_;

   if ($parser->{action} eq 'extract' or $parser->{action} eq 'check') {
      $parser->setTwigHandlers( {
            'trans-unit' => \&processTransUnit,
            tag => \&processTag,
            x   => \&processX,
            g   => \&processG,
            } );
   }
   elsif ($parser->{action} eq 'replace') {
      $parser->setTwigHandlers( {
            'trans-unit' => \&processTransUnitReplace,
            header => \&processHeader,
            } );
   }
   else {
      die "Invalid action.\n";
   }
   # Create a template for sdlxliff
   verbose("About to copy\n");
   copy($parser->{xml_in}, $parser->{template_name}) if($parser->{action} eq 'extract');

   $parser->{InputFormat} = 'sdlxliff';
}



sub processNativeCode {
   my ($parser, $e) = @_;
   my $name = $e->name();
   my $text = normalize($e->text(no_recurse=>1));

   # Special treatment for \- \_ in compounded words.
   if (($name =~ /^ph$/i) and ($text =~ /^\s*\\([-_])\s*$/)) {
       # \_ is the RTF and Trados encoding for a non-breaking hyphen; replace it
       # by -, the regular hyphen, since it is generally used in ad-hoc ways,
       # when an author or translator doesn't like a particular line-splitting
       # choice their software has made.
       $e->set_text("-") if ($1 eq '_');
       # \- is the rtf and Trados encoding for an optional hyphen; remove it
       $e->set_text("") if ($1 eq '-');
       $e->erase();
   } elsif (not $parser->{keeptags}) {
       # Only applies within target language TUVs:
       my $tuv = $e->parent('tuv');
       my $lang = $tuv->att('xml:lang') if $tuv;
       $e->delete() if $lang and (lc($lang) eq lc($parser->{tgt_lang}));
   }
}

sub processHI {
   my ($parser, $hi) = @_;
   $hi->erase();
}

sub xmlFlush {
    my ($parser) = @_;
    if ($parser->{InputFormat} eq 'sdlxliff') {
       $parser->{action} eq 'replace'
          ? $parser->flush($parser->{template}, pretty_print=>'indented')
          : $parser->purge();
    }
    elsif ($parser->{InputFormat} eq 'tmx') {
       $parser->{action} eq 'check'
          ? $parser->purge()
          : $parser->flush($parser->{template}, pretty_print=>'indented');
    }
    else {
       die "xmlFlush on a undefined format!";
    }
}

# Grab tag in order to process <x/> & <p></p>
sub processTag {
   my ($parser, $tag) = @_;
   my $tag_id = $tag->{att}{id};
   verbose("found tag id=$tag_id\n");
   #debug($tag->xml_string, "\n");

   if ($tag->get_xpath('ph[@word-end="false" and string()=~/softbreakhyphen/]')) {
      #$parser->{tag}{$tag_id} = 0;
   }
   # Replace occurrences of <x/> with some id that refer to this tag with a non break hyphen.
   elsif ($tag->get_xpath('ph[@word-end="false" and string()=~/nonbreakhyphen/]')) {
      my $text = $tag->has_child('ph')->{att}{name};
      $parser->{tag}{$tag_id} = $text;
   }
}

# Used to extract or check 'trans-unit'.
sub processTransUnit {
   my ($parser, $trans_unit) = @_;

   # Get the docid for this translation pair
   my $trans_unit_id = $trans_unit->{att}{id};
   die "Each trans-unit should have its mandatory id." unless defined $trans_unit_id;
   debug("trans_unit_id: $trans_unit_id\n");

   # Extraction mode: find src-lang text segments and replace with placeholder ID
   # This translation unit is marked as not to be translated.
   # The optional translate attribute indicates whether the <trans-unit> is to be translated.
   return if ($trans_unit->{att}->{translate} and $trans_unit->{att}->{translate} eq "no");

   # Source text - The <seg-source> element is used to maintain a working copy
   # of the <source> element, where markup such as segmentation can be
   # introduced without affecting the actual <source> element content. The
   # content of the <seg-source> is generally the translatable text, typically
   # divided into segments through the use of <mrk mtype="seg"> elements.
   my $source = $trans_unit->first_child('seg-source');
   $source = $trans_unit->first_child('source') unless($source);
   die "No source for $trans_unit_id.\n" unless ($source);

   my $mrk_id = 0;
   my @mrks = $source->children('mrk[@mtype="seg"]') or die;
   foreach my $mrk (@mrks) {
      my $src_sub = $mrk->xml_string();
      my $id =  "$trans_unit_id.".(defined($mrk->{att}{mid}) ? $mrk->{att}{mid} : $mrk_id++);
      my $out = ixAdd($parser->{ix}, $src_sub, $id);
      #$parser->{seg_id} = $out;
      $mrk->set_text($out);  # for debugging
      $parser->{seg_count}++;
   }

   xmlFlush($parser);
}


sub processTransUnitReplace {
   my ($parser, $trans_unit) = @_;

   # Get the docid for this translation pair
   my $trans_unit_id = $trans_unit->{att}{id};
   die "Each trans-unit should have its mandatory id." unless defined $trans_unit_id;
   debug("trans_unit_id: $trans_unit_id\n");

   # Replacement mode: find placeholder ID, replace with text
   return if ($trans_unit->{att}->{translate} and $trans_unit->{att}->{translate} eq "no");

   my $source = $trans_unit->get_xpath('seg-source', 0);
   $source = $trans_unit->get_xpath('source', 0) unless($source);
   die "No source for $trans_unit_id.\n" unless ($source);

   my $mrk_id = 0;  # Fallback id.
   my @mrks = $source->children('mrk[@mtype="seg"]') or die "Can't find any segments";
   foreach my $smrk (@mrks) {
      my $mrk = $smrk->copy();
      my $src_sub = $mrk->xml_string();
      my $mid = (defined($mrk->{att}{mid}) ? $mrk->{att}{mid} : $mrk_id++);
      my $xid  = "$trans_unit_id.$mid";
      my $out = ixGetSegment($parser->{ix}, $xid);
      $parser->{seg_id} = $out;
      $mrk->set_text($out);  # for debugging
      ++$parser->{seg_count};
      my $alt_trans = $mrk->wrap_in('target', 'alt-trans' => {'tool-id' => $parser->{'tool-id'}, mid => $mid});

      # Confidence estimation:
      my $ce = $xid ? ixGetCE($parser->{ix}, $xid) : undef;

      # TODO: Where to add the confidence score?
      debug("Confidence estimation for $xid: CE=%s %s\n", defined $ce ? $ce : "undef", $parser->{filter} ? $parser->{filter} : "undef");
      if ($parser->{score} and defined($ce)) {
         $alt_trans->{att}->{'match-quality'} = sprintf("%.4f", $ce);
      }

      if (defined $parser->{filter} and defined($ce) and $ce < $parser->{filter}) {
         debug("Filtering out $xid\n");
         $alt_trans->del_att('match-quality');       # BOOM!
         ++$parser->{filter_count};
      }

      my $nodeDefinition = sprintf("alt-trans[\@mid=\"$mid\" and \@tool-id=\"%s\"]", $parser->{'tool-id'});
      if (my $previous_alt_trans = $trans_unit->get_xpath($nodeDefinition, 0)) {
         debug("replacing previous alt-trans\t$nodeDefinition\n");
         $alt_trans->replace($previous_alt_trans);
      }
      else {
         debug("pasting a new alt-trans\n");
         $alt_trans->paste(last_child => $trans_unit);
      }
   }

   xmlFlush($parser);
}


sub processX {
   my ($parser, $x) = @_;
   my $x_id = $x->{att}{id};
   debug("X id=$x_id\n");
   if (defined($parser->{tag}{$x_id})) {
      $x->set_text($parser->{tag}{$x_id});
   }
   $x->erase();
}


sub processG {
   my ($parser, $g) = @_;
   my $text = join(" ", map(normalize($_->text(no_recurse=>1)), $g));
   debug("G: $text\n");
   $g->set_text($text);
   $g->erase();
}


sub processHeader {
   my ($parser, $header) = @_;

   my @tools = $header->children('tool[@tool-id="Portage-1.5.0"]');

   unless (@tools) {
      XML::Twig::Elt->new(
            tool => {
            'tool-id' => $parser->{'tool-id'},
            'tool-name' => $parser->{'tool-name'},
            'tool-version' => $parser->{'tool-version'},
            'tool-company' => $parser->{'tool-company'}}
         )->paste(last_child => $header);
   }
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
            if (lc($lang) eq lc($parser->{src_lang})) {
                warn("Duplicate source-language tuv\n") if $new_tgt_tuv;
                $new_tgt_tuv = $tuv->copy();
                $new_tgt_tuv->set_att('xml:lang' => $parser->{tgt_lang});
                my $seg = $new_tgt_tuv->first_child('seg'); # There should be exactly one
                processSegment($parser, $seg) if $seg;

            # Keep a handle on the old tgt-lang TUV: it will be replaced
            } elsif (lc($lang) eq lc($parser->{tgt_lang})) {
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

            if (lc($lang) eq lc($parser->{tgt_lang})) {
                my $seg = $tuv->first_child('seg'); # There should be exactly one
                if (not $seg) {
                    warn "No SEG element in target language TUV";
                } else {
                    processSegment($parser, $seg) if $seg;
                }
            }
            $tu->set_att(changeid => 'MT!');
            $tu->set_att(changedate => timeStamp());
            $tu->set_att(usagecount => '0');
        }

        # Confidence estimation:
        my ($id, $ce);
        $id = $parser->{seg_id};
        $ce = $id ? ixGetCE($parser->{ix}, $id) : undef;

        debug("Confidence estimation for $id: CE=%s\n", defined $ce ? $ce : "undef");
        if ($parser->{score}
            and defined($ce)) {
            XML::Twig::Elt
                ->new(prop=>{type=>'Txt::CE'}, sprintf("%.4f", $ce))
                ->paste(first_child => $tu);
        }

        if (defined $parser->{filter}
            and defined($ce)
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

    my $old_text = join("", @src_seg);
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
   }
   elsif ($parser->{action} eq 'replace') {
      $out = ixGetSegment($parser->{ix}, $in);
      $parser->{seg_id} = $in;# Ugly side-effect
         warn("Can't find ID $in in index") unless $out;
   }

   return $out;
}


# Alias for processText which make the intent clearer.
sub getTranslatedText {
   my ($parser, $in) = @_;
   warn "getTranslatedText should be use in replace mode."  unless($parser->{action} eq 'replace');
   return processText($parser, $in);
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



########################################
# Database functions.
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
        debug("ixAdd: EXISTS id=%s, ce=%s, segment=\"%s\"\n", defined $id ? $id : "undef", defined $ce ? $ce : "undef", $segment);
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
sub displayHelp {
   -t STDOUT ? system "pod2usage -verbose 3 $0" : system "pod2text $0 >&2";
}

1;
