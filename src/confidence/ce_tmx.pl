#!/usr/bin/env perl
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
(<dir>/Q.txt), and produces a TMX file template (<dir>/QP.template.xml).

The second form inserts target-language translations from file
<dir>/P.txt into that template,  generating the output TMX file
<dir>/QP.xml.

The third form checks the validity of the file. If the file is valid,
the program outputs the number of translatable source-language
segments on the standard output and exits successfully; if the file is
not valid, it exits with a non-zero status and possibly a diagnostic
on the standard error stream.

=head1 OPTIONS

=over 1

=item -filter=T

In 'replace' mode, filter out translations with confidence below T [don't]

=item -src=SL

Specify TMX source language (case insensitive within TMX file) [EN-CA]

=item -tgt=TL

Specify TMX target language (case insensitive within TMX file) [FR-CA]

=item -keeptags

Retain keeptags code (BPT, EPT, IT and PH elements) within target
language TUs. (default is to delete it)

=item -score/-noscore

In 'replace' mode, do/don't insert confidence scores inside TUV's,
using TMX PROP elements of type Txt::CE. (default is -score)

=item -pretty_print

Indents the xml format to be more human readable. (default is not indented)

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

my $score = 0;
my $pretty_print = 'none';
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
   'score!'       => \$score,
   'pp|pretty_print' => sub { $pretty_print = 'indented' },
) or do { displayHelp(); exit 1 };

$|=1;

my $output_layers = ":raw:encoding(UTF8):utf8";
my $input_layers = ":utf8";

my $DEFAULT_SRC="EN-CA";
my $DEFAULT_TGT="FR-CA";

$verbose = 0 unless defined $verbose;
$Verbose = 0 unless defined $Verbose;
$debug = 0 unless defined $debug;
$pretty_print = 'indented' if ($debug);

$keeptags = 0 unless defined $keeptags;
$filter = undef unless defined $filter;
$src = $DEFAULT_SRC unless defined $src;
$tgt = $DEFAULT_TGT unless defined $tgt;

my $action = shift or die "Missing argument: action";

if ($action eq 'extract') {
   my $dir = shift or die "Missing argument: dir";
   die "No such directory: $dir" unless -d $dir;

   my $xml_file = shift || "-";
   my $ix = newIx();
   my $parser = processFile(action => 'extract',
         xml_in => $xml_file,
         xml_out_name => "${dir}/QP.template.xml",  # May or may not be open later.
         ix => $ix,
         src_lang => $src,
         tgt_lang => $tgt,
         keeptags => $keeptags);
   close $parser->{xml_out} if(defined($parser->{xml_out}));
   ixSave($ix, "${dir}/Q.txt", "${dir}/Q.tags", "${dir}/Q.ix");
}

elsif ($action eq 'replace') {
   my $dir = shift or die "Missing argument: dir";
   die "No such directory: $dir" unless -d $dir;

   my $ce_file = (-r "${dir}/pr.ce") ? "${dir}/pr.ce" : undef;
   my $tags_file = (-r "${dir}/P.tags") ? "${dir}/P.tags" : undef;
   my $ix = ixLoad("${dir}/P.txt", $tags_file, "${dir}/Q.ix", $ce_file);

   open(my $xml_out, ">${output_layers}", "${dir}/QP.xml")
      or die "Can open output xml_out file";
   my $parser = processFile(action => 'replace',
         xml_in => "${dir}/QP.template.xml",
         xml_out => $xml_out,
         ix => $ix,
         filter => $filter,
         src_lang => $src,
         tgt_lang => $tgt,
         keeptags => $keeptags,
         score => $score);
   close $xml_out;
   # TODO: should we rename tne output to its proper extension or leave it to PortageLive.php?
   #rename "${dir}/QP.xml",  "${dir}/QP." . $parser->{InputFormat};
}

elsif ($action eq 'check') {
   my $xml_file = shift || "-";
   my $info = processFile(action => 'check',
         xml_in => $xml_file,
         keeptags => $keeptags,
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
         pretty_print => $pretty_print,
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
   verbose("[%d translations filtered out]\n", $parser->{filter_count})
      if defined $parser->{filter};
   return $parser;
}


# If this xml file is a tmx, set the proper handlers for it.
sub processTMX {
   my ($parser, $elt) = @_;

   if($parser->{action} eq 'extract') {
      open($parser->{xml_out}, ">${output_layers}", $parser->{xml_out_name})
         or die "Can open output xml_out file";
      $parser->setTwigHandlers( { tu => \&processTU, } );
   }
   else {
      # NOTE: we should NOT change the source segment thsu we should NOT have to unwrapped element in the source segment.
      $parser->setTwigHandlers( { tu => \&replaceTU, } );
   }

   $parser->{InputFormat} = 'tmx';
}

use File::Copy;
# If this xml file is a sdlxliff, set the proper handlers for it.
# BOOKMARKS:
# * XLIFF version 1.2: http://docs.oasis-open.org/xliff/v1.2/os/xliff-core.html
# * xliff-core-1.2-strict.xsd: http://docs.oasis-open.org/xliff/v1.2/cs02/xliff-core-1.2-strict.xsd
sub processXLIFF {
   my ($parser, $elt) = @_;

   if ($parser->{action} eq 'extract' or $parser->{action} eq 'check') {
      $parser->setTwigHandlers( {
            'trans-unit' => \&processTransUnit,
            tag => \&processTag,
            g   => \&processG,
            x   => \&processX,
            bx  => sub { my( $t, $elt)= @_; },
            ex  => sub { my( $t, $elt)= @_; },
            bpt => sub { my( $t, $elt)= @_; },
            ept => sub { my( $t, $elt)= @_; },
            ph  => sub { my( $t, $elt)= @_; },
            it  => sub { my( $t, $elt)= @_; },
            'seg-source//mrk[@mtype="seg"]' => sub { my( $t, $elt)= @_; debug("\ttest MRK mid=%s\n", $elt->{att}->{mid}); },
            } );
   }
   elsif ($parser->{action} eq 'replace') {
      $parser->setTwigHandlers( {
            'trans-unit' => \&replaceTransUnit,
            header => \&processHeader,
            } );
   }
   else {
      die "Invalid action.\n";
   }
   # Create a template for sdlxliff
   copy($parser->{xml_in}, $parser->{xml_out_name}) if($parser->{action} eq 'extract');

   $parser->{InputFormat} = 'sdlxliff';
}



sub wrapSelfClosingTag {
   my ($parser, $elt) = @_;
   XML::Twig::Elt->new(tag_wrap => { content => $elt->sprint})->replace($elt);
}


sub wrapOpeningTag {
   my ($parser, $elt) = @_;
   XML::Twig::Elt->new(open_wrap => {
        id => $elt->{att}->{i},
        content => $elt->sprint,
        })->replace($elt);
}


sub wrapClosingTag {
   my ($parser, $elt) = @_;
   XML::Twig::Elt->new(close_wrap => {
        id => $elt->{att}->{i},
        content => $elt->sprint,
        })->replace($elt);
}


sub unWrapTag {
   my ($parser, $elt) = @_;
   my $content = $elt->{att}->{content};
   if ($content) {
      #print STDERR "CONTENT: ", $content, "\n";
      $elt->parse($content)->replace($elt);
   }
   else {
      die "Invalid wrapper tag format.\n";
   }
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
   }
   elsif (not $parser->{keeptags}) {
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
          ? $parser->flush($parser->{xml_out})
          : $parser->purge();
    }
    elsif ($parser->{InputFormat} eq 'tmx') {
       debug("  xmlflush tmx: %s\n", $parser->{action});
       $parser->{action} eq 'check'
          ? $parser->purge()
          : $parser->flush($parser->{xml_out});
    }
    else {
       die "xmlFlush on a undefined format!";
    }
}

# Grab tag in order to process <x/> & <p></p>
sub processTag {
   my ($parser, $tag) = @_;
   my $tag_id = $tag->{att}{id};
   veryVerbose("processing tag id=%s\n", $tag_id);
   #debug("%s\n", $tag->xml_string);

   if ($tag->get_xpath('ph[@word-end="false" and string()=~/softbreakhyphen/]')) {
      my $text = $tag->has_child('ph')->{att}{name};
      $parser->setTwigHandler("x[\@id=\"$tag_id\"]",
         sub {
            my ($parser, $x) = @_;
            veryVerbose("Special handler for %s\n", $tag_id);
            $x->delete();
         });
   }
   # Replace occurrences of <x/> with some id that refer to this tag with a non break hyphen.
   elsif ($tag->get_xpath('ph[@word-end="false" and string()=~/nonbreakhyphen/]')) {
      my $text = $tag->has_child('ph')->{att}{name};
      $parser->setTwigHandler("x[\@id=\"$tag_id\"]",
         sub {
            my ($parser, $x) = @_;
            veryVerbose("Special handler for %s\n", $tag_id);
            $x->set_text($text);
            $x->erase();
         });
   }
}

# Used to extract or check 'trans-unit'.
sub processTransUnit {
   my ($parser, $trans_unit) = @_;

   # Get the docid for this translation pair
   my $trans_unit_id = $trans_unit->{att}{id};
   die "Each trans-unit should have its mandatory id." unless defined $trans_unit_id;
   debug("trans_unit_id: %s\n", $trans_unit_id);

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
   my @mrks = $source->descendants('mrk[@mtype="seg"]') or warn "Can't find any mrk for $trans_unit_id\n\tcontent:", $source->xml_string, "\n";
   foreach my $mrk (@mrks) {
      my $mrk_text = $mrk->text();
      veryVerbose("\tMRK: %s\n", $mrk_text);
      my $id =  "$trans_unit_id." . (defined($mrk->{att}{mid}) ? $mrk->{att}{mid} : $mrk_id++);
      $parser->{seg_id} = ixAdd($parser->{ix}, $mrk_text, $mrk->xml_string(), $id);
      $parser->{seg_count}++;
   }

   xmlFlush($parser);
}


sub replaceTransUnit {
   my ($parser, $trans_unit) = @_;

   # Get the docid for this translation pair
   my $trans_unit_id = $trans_unit->{att}{id};
   die "Each trans-unit should have its mandatory id." unless defined $trans_unit_id;
   debug("trans_unit_id: %s\n", $trans_unit_id);

   # Replacement mode: find placeholder ID, replace with text
   return if ($trans_unit->{att}->{translate} and $trans_unit->{att}->{translate} eq "no");

   my $source = $trans_unit->first_child('seg-source');
   $source = $trans_unit->first_child('source') unless($source);
   die "No source for $trans_unit_id.\n" unless ($source);

   # Create a target element.
   my $target = $trans_unit->get_xpath('target', 0);
   $target->delete() if(defined($target));
   $target = $source->copy();
   $target->set_tag('target');  # rename it to target.
   $target->del_atts();  # Make sure there is no attributs.

   my $sdl_defs = $trans_unit->get_xpath("sdl:seg-defs", 0);
   unless (defined($sdl_defs)) {
      warn "Unable to find sdl:seg-defs for $trans_unit_id, adding one...";
      $sdl_defs = XML::Twig::Elt->new('sdl:seg-defs');
      $sdl_defs->paste(last_child => $trans_unit);
   }

   my $mrk_id = 0;  # Fallback id.
   my @mrks = $target->descendants('mrk[@mtype="seg"]') or warn "Can't find any mrk for $trans_unit_id\n\tcontent:", $target->xml_string, "\n";
   foreach my $mrk (@mrks) {
      my $mid = (defined($mrk->{att}{mid}) ? $mrk->{att}{mid} : $mrk_id++);
      my $xid = "$trans_unit_id.$mid";
      my $translatoin = getTranslation($parser, $xid);
      eval {
         #$mrk->set_text($translatoin);  # Escapes xml markup in translatoin which is not the behaviour we want for the tag project.
         $mrk->set_inner_xml($translatoin);  # Looks to be safe if $translatoin doesn't contain any markup/tags.
         1;
      }
      or die "XMLERROR: $translatoin\n$@\n";
      ++$parser->{seg_count};


      my $sdl_seg = $sdl_defs->get_xpath("sdl:seg[\@id=\"$mid\"]", 0);
      unless(defined($sdl_seg)) {
         warn "Unable to find sdl:seg for $trans_unit_id, adding one...";
         $sdl_seg = XML::Twig::Elt->new('sdl:seg-seg');
         $sdl_seg->paste(last_child => $sdl_defs);
      }

      $sdl_seg->del_atts();
      $sdl_seg->{att}->{id}              = $mid;
      $sdl_seg->{att}->{conf}            = 'Draft';
      $sdl_seg->{att}->{origin}          = 'mt';
      $sdl_seg->{att}->{'origin-system'} = $parser->{'tool-id'};

      # Confidence estimation:
      my $ce = $xid ? ixGetCE($parser->{ix}, $xid) : undef;

      # Confidence Estimation element example:
      # Not yet defined:
      #   <sdl:seg-defs><sdl:seg id="560" /></sdl:seg-defs>
      # Defined:
      #   <sdl:seg-defs><sdl:seg id="560" /></sdl:seg-defs>
      #      <sdl:seg conf="Draft" id="56" origin="mt" origin-system="Portage-1.5.0" percent="99" >
      #   </sdl:seg-defs>
      debug("Confidence estimation for %s: CE=%s %s\n", $xid, $ce, $parser->{filter});
      $sdl_seg->del_att('percent');       # Make sure there is no previous value for the attribut percent.
      if ($parser->{score} and defined($ce)) {
         $ce = 0 if ($ce < 0);
         $ce = 1 if ($ce > 1);
         $sdl_seg->{att}->{'percent'} = sprintf("%.0f", 100*$ce);  # %.0f will properly round numbers.
      }

      if (defined $parser->{filter} and defined($ce) and $ce < $parser->{filter}) {
         debug("Filtering out %s\n", $xid);
         $sdl_seg->del_atts();
         $sdl_seg->{att}->{id} = $mid;
         $mrk->delete();
         ++$parser->{filter_count};
      }
   }

   # If we didn't filter out all translations, we can add the target object to the tree.
   $target->paste(after => $source) if ($target->descendants('mrk[@mtype="seg"]'));

   xmlFlush($parser);
}


# For SDLXLIFF, we need to map some x element to there value, more precisely,
# non breaking space.
# Prefered technique is to create a new handler when we see the proper id when
# processing <tag>.  See how we define a new handler when we see the <tag> for
# non breaking hyphen.
sub processX {
   my ($parser, $x) = @_;
   my $x_id = $x->{att}{id};
   veryVerbose("X id=%s\n", $x_id);
   if (defined($parser->{tag}{$x_id})) {
      $x->set_text($parser->{tag}{$x_id});
      $x->erase();
   }
}


# NOTE: erasing g elements is not necessary since during
sub processG {
   my ($parser, $g) = @_;
   my $text = join(" ", map(normalize($_->text(no_recurse=>1)), $g));
   veryVerbose("G id=%s: %s\n%s\n", $g->{att}{id},  $text, $g->xml_string);
   #$g->set_text($text);
   # Erase the element: the element is deleted and all of its children are pasted in its place.
   # TODO: disabled to extract tags.
   #$g->erase();
}


# Adds a tool description for Portage.
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

   $parser->{seg_id} = ""; # will be set later.

   # Extraction mode: find src-lang text segments and replace with placeholder ID
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
         if ($seg) {
            my $clean = XML::Twig->new(
                     twig_handlers => {
                        ph  => \&processNativeCode,
                     })
                  ->parse("<dummy>" . $seg->xml_string() . "</dummy>")
                  ->root;
            $parser->{seg_id} = ixAdd($parser->{ix}, $clean->text(), $clean->xml_string());
            $seg->set_text($parser->{seg_id});  # Mark translation's placeholder.
         }
      }
      # Keep a handle on the old tgt-lang TUV: it will be replaced
      elsif (lc($lang) eq lc($parser->{tgt_lang})) {
         $old_tgt_tuv = $tuv;
      }
   }

   if ($new_tgt_tuv) {
      if ($old_tgt_tuv) {
         $new_tgt_tuv->replace($old_tgt_tuv);
      }
      else {
         $new_tgt_tuv->paste(last_child => $tu);
      }
   }
   else {
      warn("Missing source-language version in TU");
   }
   xmlFlush($parser);

   $parser->{tu_count}++;
}

sub replaceTU {
   my ($parser, $tu) = @_;

   my @tuvs = $tu->children('tuv');
   warn("Missing variants in TU") unless @tuvs;

   $parser->{seg_id} = ""; # will be set later.

   # Replacement mode: find placeholder ID, replace with text
   foreach my $tuv (@tuvs) {
      my $lang = $tuv->{att}->{'xml:lang'};
      warn("Missing language attribute in TU") unless $lang;

      if (lc($lang) eq lc($parser->{tgt_lang})) {
         my $seg = $tuv->first_child('seg'); # There should be exactly one
         if (not $seg) {
            warn "No SEG element in target language TUV";
         }
         else {
            my $id = $seg->text();
            my $translation = getTranslation($parser, $id);
            eval {
               $seg->set_inner_xml($translation);
               1;
            }
            or die "XMLERROR: $translation\n$@\n";
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

   debug("Confidence estimation for %s: CE=%s\n", $id, $ce);
   if ($parser->{score}
         and defined($ce)) {
      XML::Twig::Elt
         ->new(prop=>{type=>'Txt::CE'}, sprintf("%.4f", $ce))
         ->paste(first_child => $tu);
   }

   if (defined $parser->{filter}
         and defined($ce)
         and $ce < $parser->{filter}) {
      debug("Filtering out %s\n", $id);
      $tu->delete();       # BOOM!
         $parser->{filter_count}++;
   }
   else {
      xmlFlush($parser);
   }

    $parser->{tu_count}++;
}


# Returns from the database, the translation for id.
sub getTranslation {
   my ($parser, $id) = @_;
   my $translation = "";

   die "getTranslation should be use in replace mode."  unless($parser->{action} eq 'replace');

   $translation = ixGetSegment($parser->{ix}, $id);
   $parser->{seg_id} = $id;# Ugly side-effect
   warn("Can't find ID $id in index") unless defined($translation);

   return $translation
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
    return { id_seed=>0, id=>{}, segment=>{}, tagged=>{}, ce=>{} };
}

sub ixAdd {
   my ($ix, $segment, $tagged, $id, $ce) = @_;
   $segment = normalize($segment);
   $tagged  = normalize($tagged) if defined $tagged;

   if (defined $id or not exists($ix->{id}{$segment})) {
      if (defined $id) {
         $id = normalize($id);
      }
      else {
         $id = ixNewID($ix);
      }
      veryVerbose("ixAdd: INSERTING id=%s, ce=%s, segment=\"%s\" : tagged=\"%s\"\n", $id, $ce, $segment, $tagged);
      $ix->{id}{$segment} = $id;
      $ix->{segment}{$id} = $segment;
      $ix->{tagged}{$id}  = $tagged;
      $ix->{ce}{$id}      = $ce if defined $ce;
   }
   else {
      veryVerbose("ixAdd: EXISTS id=%s, ce=%s, segment=\"%s\" : tagged=\"%s\"\n", $id, $ce, $segment, $tagged);
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

# Not used.
sub ixGetID {
    my ($ix, $segment) = @_;
    $segment = normalize($segment);
    return exists($ix->{id}{$segment}) ? $ix->{id}{$segment} : undef;
}

sub ixGetSegment {
   my ($ix, $id) = @_;
   $id = normalize($id);
   my $tagged = $ix->{tagged}{$id};
   if (defined($tagged)) {
      return $tagged;
   }
   else {
      return exists($ix->{segment}{$id}) ? $ix->{segment}{$id} : undef;
   }
}

sub ixGetCE {
    my ($ix, $id) = @_;
    $id = normalize($id);
    return exists($ix->{ce}{$id}) ? $ix->{ce}{$id}+0 : undef;
}

sub ixSave {
   my ($ix, $seg_file, $tag_file, $id_file) = @_;

   open(my $seg_out, ">${output_layers}", $seg_file)
      or die "Can't open output file $seg_file";
   open(my $tag_out, ">${output_layers}", $tag_file)
      or die "Can't open output file $tag_file"
      if defined $tag_file;
   open(my $id_out, ">${output_layers}", $id_file)
      or die "Can't open output file $id_file";

   for my $id (sort keys %{$ix->{segment}}) {
      my $seg = $ix->{segment}{$id};
      my $tag = $ix->{tagged}{$id};
      print {$seg_out} $seg, "\n";
      print {$id_out} $id, "\n";
      if (defined($tag_file)) {
         if (defined($tag)) {
            print {$tag_out} 
               XML::Twig->new(
                  twig_handlers => {
                     bpt => \&wrapOpeningTag,
                     ept => \&wrapClosingTag,
                     ph  => \&wrapSelfClosingTag,
                     it  => \&wrapSelfClosingTag,
                     hi  => \&wrapSelfClosingTag,
                  })
               ->parse("<dummy>$tag</dummy>")
               ->root
               ->xml_string, "\n";
         }
         else {
            print {$tag_out} "PORTAGE_NULL\n";
         }
      }
   }

   close $seg_out;
   close $tag_out if defined $tag_file;
   close $id_out;
}

# TODO: Is there any point in loading back the segment version when all we need
# is the tagged translation?
sub ixLoad {
   my ($seg_file, $tag_file, $id_file, $ce_file) = @_;
   my $ix = newIx();

   open(my $seg_in, "<$input_layers", $seg_file)
      or die "Can open input file $seg_file";
   open(my $tag_in, "<$input_layers", $tag_file)
      or die "Can open input file $tag_file"
      if defined $tag_file;
   open(my $id_in, "<$input_layers", $id_file)
      or die "Can open input file $id_file";
   open(my $ce_in, "<$input_layers", $ce_file)
      or die "Can open input file $ce_file"
      if defined $ce_file;

   my $count = 0;
   verbose("[Reading index from $seg_file and $id_file]\n");
   verbose("[Reading CE from $tag_file]\n") if defined $tag_file;
   verbose("[Reading CE from $ce_file]\n") if defined $ce_file;
   my $tag = undef;
   while (defined (my $id = <$id_in>)) {
      chomp $id;
      my $seg = readline($seg_in);
      die "Not enough lines in text file $seg_file" unless defined $seg;
      chomp $seg;
      if ($tag_file) {
         $tag = readline($tag_in);
         die "Not enough lines in text file $tag_file" unless defined $tag;
         chomp $tag;
         if ($tag =~ m/^PORTAGE_NULL$/) {
            $tag = undef;
         }
         else {
            # Remove Portage's wrapping tags.
            if ($tag =~ m/open_wrap|close_wrap|tag_wrap/) {
               $tag = XML::Twig->new(
                     twig_handlers => {
                        open_wrap  => \&unWrapTag,
                        close_wrap => \&unWrapTag,
                        tag_wrap   => \&unWrapTag,
                     })
                  ->parse("<dummy>$tag</dummy>")
                  ->root
                  ->xml_string;
            }
         }
      }
      my $ce = 0;
      if ($ce_file) {
         $ce = readline($ce_in);
         die "Not enough lines in CE file $ce_file" unless defined $ce;
         chomp $ce;
      }
      veryVerbose("ixLoad: read $id <<%s>> {{%s}} ($ce)\n", $seg, $tag);
      ixAdd($ix, $seg, $tag, $id, $ce);
      verbose("\r[%d lines...]", $count) if (++$count % 1 == 0);
   }
   verbose("\r[%d lines; done.]\n", $count);
   die "Too many lines in text file $seg_file" unless eof $seg_in;
   if (defined($tag_file)) {
      die "Too many lines in text file $tag_file" unless eof $tag_in;
   }

   close $seg_in;
   close $tag_in if defined $tag_file;
   close $id_in;
   close $ce_in if defined $ce_file;

   return $ix;
}

sub verbose { printf STDERR (map { $_ = defined $_ ? $_ : "undef" } @_) if ($verbose or $Verbose) ; }
sub veryVerbose { printf STDERR (map { $_ = defined $_ ? $_ : "undef" } @_) if $Verbose; }
sub debug { printf STDERR (map { $_ = defined $_ ? $_ : "undef" } @_) if $debug; }
sub displayHelp {
   -t STDOUT ? system "pod2usage -verbose 3 $0" : system "pod2text $0 >&2";
}

1;
