#!/usr/bin/perl
# $Id$

# pgm_usage_2_html.pl Creates html page from a program's help message.
#
# PROGRAMMER: Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2008, Sa Majeste la Reine du Chef du Canada /
# Copyright 2008, Her Majesty in Right of Canada

use strict;
use warnings;

print STDERR "pgm_usage_2_html.pl, NRC-CNRC, (c) 2006 - 2008, Her Majesty in Right of Canada\n";

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] [<in> [<out>]]

  Outputs to <out> an html page with the content given by <in>.

Options:

  -pgm <NAME>    Creates a web page with the content of a program called <NAME>
                 with it's help description given by <in>.
  -module <NAME> Creates list of available program in a module.  Expects a list
                 of all available programs as <in>.
  -main <TITLE>  Creates a web page with the list of all modules given by <in>.

  -h(elp)       print this help message
  -v(erbose)    increment the verbosity level by 1 (may be repeated)
  -d(ebug)      print debugging information
";
   exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
my $title;
GetOptions(
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,
   "pgm=s"     => \my $name,
   "module=s"  => \my $module_index,
   "main=s"    => \my $main_index,
) or usage;

die "You must provide either -pgm|-module|-main!" if (not(defined($name) or defined($module_index) or defined($main_index)));
die "You cannot make both main_index and module_index at the same time." if (defined($module_index) and defined($main_index));

my $in = shift || "-";
my $out = shift || "-";

0 == @ARGV or usage "Superfluous parameter(s): @ARGV";

#$verbose and print STDERR "Mildly verbose output\n";
#$verbose > 1 and print STDERR "Very verbose output\n";
#$verbose > 2 and print STDERR "Exceedingly verbose output\n";

if ( $debug ) {
   no warnings;
   print STDERR "
   in          = $in
   out         = $out
   verbose     = $verbose
   debug       = $debug
   pgm         = $name
   module      = $module_index
   main        = $main_index

";
}

open(IN, "<$in") or die "Can't open $in for reading: $!\n";
open(OUT, ">$out") or die "Can't open $out for writing: $!\n";

my $NRC_logo;
if (defined($main_index)) {
   $title = $main_index;
   $NRC_logo = "NRC_banner_e.jpg";
}
else {
   $NRC_logo = "../NRC_banner_e.jpg";
   if (defined($module_index)) {
      $title = $module_index;
   }
   else {
      $title = $name;
   }
}

my $header = <<HEADER;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML>
<HEAD>
  <TITLE>PORTAGE shared - $title</TITLE>
  </HEAD>
  <BODY BGCOLOR="#FFFFFF" LINK="#0000ff" VLINK="#006600">

  <H1 id="logo"><img alt="Science at work for Canada" height="90" width="450" src="$NRC_logo" /></H1>
  <H1>$title</H1>
HEADER

my $footer = <<FOOTER;
  <BR><HR><BR>
  <CENTER>
  Technologies langagi&egrave;res interactives / Interactive Language Technologies<BR>

  Institut de technologie de l'information / Institute for Information Technology<BR>
  Conseil national de recherches Canada / National Research Council Canada<BR>
  Copyright 2004-2008, Sa Majest&eacute; la Reine du Chef du Canada / Her Majesty in Right of Canada
  </CENTER>

  </BODY></HTML>
FOOTER

print OUT $header;
if (defined($module_index)) {
   print OUT "<H2>Available programs are:</H2><BR>
   <UL>
   ";
   while (<IN>) {
      chomp;
      my $pgm_web_page = $_;
      s/.html//;
      my $pgm_name     = $_;
      print OUT "<LI><A HREF=\"$pgm_web_page\">$pgm_name</A>\n";
   }
   print OUT "</UL><BR>
   <A HREF=../index.html>back</A>
   ";
}
elsif (defined($main_index)) {
   print OUT "<H2>Available modules are:</H2><BR>
   <UL>
   ";
   while (<IN>) {
      chomp;
      print OUT "<LI><A HREF=\"$_/index.html\">$_</A>\n";
   }
   print OUT "</UL><BR>\n";
}
else {
   print OUT "<PRE>";
   while (<IN>) {
      s/</&lt;/g;
      print OUT;
   }
   print OUT "</PRE>
   <A HREF=index.html>back</A>
   ";
}
print OUT $footer;
