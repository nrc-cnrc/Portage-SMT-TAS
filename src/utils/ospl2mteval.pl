#!/usr/bin/perl -s

# ospl2mteval.pl 
# 
# PROGRAMMER: George Foster
# 
# COMMENTS: 
#
# George Foster
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

use strict;
use warnings;

print STDERR "ospl2mteval.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada\n";

my $HELP = "
ospl2mteval.pl [-srclang l][-tgtlang l][-setid id][-sysid id] src tst ref1 ref2 ...
ospl2mteval.pl -c src_sgml tst

The 1st version converts one-sentence-per-line (OSPL) source, test, and
reference files into the SGML format required by the mteval script. The 2nd
(-c) version uses an SGML source file to help convert an OSPL test file into
SGML for mteval, which it writes to stdout. 

In the 1st version, there are two possibilities for the <src>, <tst>, and
<ref_i> args:
1) They can be single files, in which case all files must correspond line for
   line.
2) They can be directories, in which case all must contain the same number of
   files, with the same names, and files with the same name must correspond
   line for line.
In both cases, output is written to {src,tst,ref}-<setid>.sgml.

Options:

-srclang Use <l> as the source language tag [Chinese]
-tgtlang Use <l> as the target language tag [English]
-setid   Use <id> as the set tag and as base name for output files [eval]
-sysid   Use <id> for the sysid tag in the test output file [NRC]

";

our ($help, $h, $srclang, $tgtlang, $setid, $sysid, $c);

if ($help || $h) {
    print $HELP;
    exit 0;
}

$srclang = "Chinese" unless defined $srclang;
$tgtlang = "English" unless defined $tgtlang;
$setid = "eval" unless defined $setid;
$sysid = "NRC" unless defined $sysid;
 
my $src = shift or die "$HELP";
my $tst = shift or die "$HELP";

# --------------------------------------------------------------------
# 2nd version:

if ($c) {

    open(SRC, "<", $src) or die "Can't open $src for reading\n";
    undef $/;
    my $src_content = <SRC>;
    close SRC;
    $/ = "\n";

    open(TST, "<", $tst) or die "Can't open $tst for reading\n";


    if ($src_content =~ /<srcset\s+([^>]+)>/o) {
       my $attrs = $1;
       if ($attrs =~ /setid\s*=\s*\"([^\"]+)\"/o) {$setid = $1;}
       if ($attrs =~ /srclang\s*=\s*\"([^\"]+)\"/o) {$srclang = $1;}
    }

    while ($src_content =~ /<([^< ]+)([^<]*)>/go) {
       my $tag = $1;
       my $guts = $2;
       if ($tag eq "srcset") {
          print "<tstset setid=\"$setid\" srclang=\"$srclang\" trglang=\"$tgtlang\">\n";
       } elsif ($tag eq "/srcset") {
          print "</tstset>\n";
       } elsif ($tag eq "DOC") {
          print "<DOC$guts sysid=\"$sysid\">\n";
       } elsif ($tag eq "SEG" || $tag eq "seg") {
          print "<$tag$guts>";
          my $line = <TST>;
          chomp($line);
          print $line;
       } else {
          print "<$tag$guts>\n";
       }
    }

    exit 0;
}

# --------------------------------------------------------------------
# 1st version:

my @refs = @ARGV;
if ($#refs < 0) {die "$HELP";}

my $args_are_directories = 0;

if (-d $src) {
    $args_are_directories = -d $tst;
    foreach my $ref (@refs) {
	$args_are_directories = $args_are_directories && -d $ref;
    }
    if (!$args_are_directories) {
	die "Arguments must be either all files or all directories\n";
    }
}

my @src_files;
my @tst_files;
my @ref_sets;			# an array of arrays

if ($args_are_directories) {
    @src_files = list_files_in_dir($src);
    @tst_files = list_files_in_dir($tst);
    if ($#src_files != $#tst_files) {
	die "Directories $src and $tst contain different numbers of files\n";
    }
    foreach my $ref (@refs) {
	my @files = list_files_in_dir($ref);
	if ($#src_files != $#files) {
	    die "Directories $src and $ref contain different numbers of files\n";
	}
	push(@ref_sets, \@files);
    }
} else {
    $src_files[0] = $src;
    $tst_files[0] = $tst;
    foreach my $ref (@refs) {
	push(@ref_sets, [$ref]);
    }
}

open (SRC_OUT, ">src-$setid.sgm") or die "Can't open src$setid.sgm for writing\n";
open (TST_OUT, ">tst-$setid.sgm") or die "Can't open tst$setid.sgm for writing\n";
open (REF_OUT, ">ref-$setid.sgm") or die "Can't open ref$setid.sgm for writing\n";

print SRC_OUT "<srcset setid=\"$setid\" srclang=\"$srclang\">\n";
print TST_OUT "<tstset setid=\"$setid\" srclang=\"$srclang\" trglang=\"$tgtlang\">\n";
print REF_OUT "<refset setid=\"$setid\" srclang=\"$srclang\" trglang=\"$tgtlang\">\n";

for (my $i = 0; $i <= $#src_files; ++$i) {

    open(SRC, "<", $src_files[$i]) or die "Can't open $src_files[$i] for reading\n";
    open(TST, "<", $tst_files[$i]) or die "Can't open $tst_files[$i] for reading\n";
    my @REFS;
    for (my $j = 0; $j <= $#ref_sets; ++$j) {
	open ($REFS[$j], "<", $ref_sets[$j]->[$i]) or die "Can't open $ref_sets[$j]->[$i] for reading\n";
    }

    my $docno = $i+1;

    code_doc(\*SRC, \*SRC_OUT, "$docno", $src_files[$i]);
    code_doc(\*TST, \*TST_OUT, "$docno", $sysid);
    for (my $j = 0; $j <= $#REFS; ++$j) {
	code_doc($REFS[$j], \*REF_OUT, "$docno", $refs[$j]);
    }
}

print SRC_OUT "</srcset>\n";
print TST_OUT "</tstset>\n";
print REF_OUT "</refset>\n";

close SRC_OUT;
close TST_OUT;
close REF_OUT;


sub code_doc #(\*FROMFILE, \*TOFILE, docid, sysid)
{
    my $fromfile = shift;
    my $tofile = shift;
    my $docid = shift;
    my $sysid = shift;

    my $segid=1;

    print $tofile "<doc docid=\"$docid\" sysid=\"$sysid\">\n";
    while (my $line = <$fromfile>) {
	chomp $line;
	print $tofile "<seg id=", $segid++, "> ";
	print $tofile $line;
	print $tofile " </seg>\n";
    }
    print $tofile "</doc>\n";
}

sub list_files_in_dir # (dirname)
{
    my $dirname = shift;
    my @filelist;
    opendir(DIR, $dirname) or die "Can't opendir $dirname: $!";
    while (defined(my $file = readdir(DIR))) {
	next if $file =~ /^\.\.?$/;     # skip . and ..
	push(@filelist, "$dirname/$file");
    }
    closedir(DIR);
    return @filelist;
}
