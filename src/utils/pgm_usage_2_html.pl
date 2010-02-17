#!/usr/bin/perl
# $Id$

# @file pgm_usage_2_html.pl 
# @brief Creates html page from a program's help message.
#
# @author Samuel Larkin
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
  This script is normally invoked by running \"make usage\" in the src/
  directory.

Options:

  -pgm <MODULE>/<NAME>    Creates a web page with the content of a program called <NAME>
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
   "index=s"   => \my $full_index,
) or usage;

die "You must provide either -pgm|-module|-main|-index!" if (not(defined($name) or defined($module_index) or defined($main_index) or defined($full_index)));
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

my $NRC_logo_path;
my $hierarchy;
if (defined($main_index)) {
   $title = $main_index;
   $NRC_logo_path = ".";
   $hierarchy = "";
}
elsif (defined($full_index)) {
   $title = $full_index;
   $NRC_logo_path = ".";
   $hierarchy = " <A HREF=\"index.html\">PORTAGEshared</A>";
}
else {
   $NRC_logo_path = "..";
   $hierarchy = "<A HREF=\"../index.html\">PORTAGEshared</A> /";
   if (defined($module_index)) {
      $title = "Module: $module_index";
      $hierarchy .= " $module_index /";
   }
   else {
      my @names = split(/\//, $name);
      $hierarchy .= " <A HREF=\"index.html\">$names[0]</A> / $names[1]";
      $name = $names[1];
      $title = $name;
   }
}

#print STDERR "\$title: $title\n";
#print STDERR "\$NRC_logo_path: $NRC_logo_path\n";
#print STDERR "\$hierarchy: $hierarchy\n";

# Here we define the header.
my $header = <<HEADER;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML>
<HEAD>
  <TITLE>PORTAGE shared - $title</TITLE>
  </HEAD>
  <BODY BGCOLOR="#FFFFFF" LINK="#0000ff" VLINK="#006600">

  <CENTER>
  <H1 id="logo"><img alt="Science at work for Canada" src="$NRC_logo_path/NRC_banner_e.jpg" /></H1>
  </CENTER>
  <H1>$title</H1>
  <H5>$hierarchy</H5>
HEADER

# Here we define the footer with the copyright.
my $footer = <<FOOTER;
  <BR><HR><BR>
<table width="100%" border="0" cellpadding="0" cellspacing="0"> 
<tr>
  <td width="20%" valign="bottom" align="right">
    <img src="$NRC_logo_path/iit_sidenav_graphictop_e.gif" height="54" alt="NRC-IIT - Institute for Information Technology" />
  </td>
  <td width="60%" valign="bottom" align="center" class="imgcell">
     <img src="$NRC_logo_path/mainf1.gif" alt="National Research Council Canada" width="286" height="44" />
  </td>
  <td valign="center" align="left" width="20%">
    <img src="$NRC_logo_path/mainWordmark.gif" alt="Government of Canada" width="93" height="44" />
  </td>
</tr>
<tr>
  <td valign="top" align="right">
    <img src="$NRC_logo_path/iit_sidenav_graphicbottom_e.gif" alt="NRC-IIT - Institute for Information Technology" />
  </td>
  <td valign="top" align="center">
Technologies langagi&egrave;res interactives / Interactive Language Technologies<BR>
Institut de technologie de l'information / Institute for Information Technology<BR>
Conseil national de recherches Canada / National Research Council Canada<BR>
Copyright &copy; 2004-2009, Sa Majest&eacute; la Reine du Chef du Canada / Her Majesty in Right of Canada
  </td>
</tr>
</table> 
FOOTER

print OUT $header;
# For a module we expect a list of files that we will format into a listing of
# programs.
if (defined($module_index)) {
   print OUT "<H2>Available programs in $module_index:</H2><BR>
   <TABLE CELLPADDING=\"5\" BORDER=\"1\">
   ";
   while (<IN>) {
      chomp;
      my $pgm_name = $_;
      next unless $pgm_name;
      print OUT "<TR ID=\"program brief description\"><TD ALIGN=\"left\" VALIGN=\"top\" NOWRAP><A ID=\"MODULE $module_index\" HREF=\"$pgm_name.html\">$pgm_name</A></TD>";

      my $code;
      {
         if ( -e "$pgm_name.cc" ) {
            $pgm_name .= ".cc";
         }
         local( $/, *CODE ) ;
         open( CODE, "head -10 $pgm_name | egrep '^[ ]*(#|\\*)' |" ) or die "sudden flaming death\n";
         $code = <CODE>;
         print STDERR "head -10 $pgm_name | egrep '^[ ]*(#|\\*)' |\n" if ($debug);
      }

      print STDERR $code if ($debug);
      if ($code =~ /[#\*] \@brief\s+(.*?)^\s*([#\*]\s*$|[#\*]\s*\@|\*\/)/ms) {
         my $oneliner = $1;
         $oneliner =~ s/[#\*\n]//g;
         chomp($oneliner);
         print OUT "<TD ALIGN=\"left\" VALIGN=\"top\">$oneliner</TD>";
      }
      print OUT "</TR>\n";
   }
   print OUT "</TABLE><BR>
   <A HREF=../index.html>back</A>
   ";
}
# Compile a list of all available module in Portage.
elsif (defined($main_index)) {
   print OUT "<H2>PORTAGEshared programs by module:</H2><BR>
   <UL>
   ";
   while (<IN>) {
      chomp;
      print OUT "<LI><A HREF=\"$_/index.html\">$_</A>\n";
   }
   # Add a link to the list of all available programs in portage.
   print OUT "</UL>
   <BR><BR>
   <H4><A HREF=\"list.html\">Alphabetical list of all programs</A></H4>\n";
}
# Compile an index of all the Portages' program into one list.
elsif (defined($full_index)) {
   my %list = ();
   print OUT "<TABLE CELLPADDING=\"5\" BORDER=\"1\">\n";
   while (<IN>) {
      # Extract the module's name.
      my $module_name;
      if (/ID="MODULE ([^"]+)"/) {
         $module_name = $1;
         print STDERR "$module_name\n" if ($debug);
      }
      else {
         print STDERR "<WARN>: Where is the module's name.\n";
      }

      # Fix the link to point to the file in the module's directory.
      if (not s/HREF="/HREF="$module_name\//) {
         print STDERR "<WARN>: Couldn't fix the link.\n";
      }

      # Grab the link and store it memory.
      if (/HREF="(.*html)"/) {
         my $key = $1;
         $key =~ s/.*\///;
         $key = lc($key);
         $list{$key} = $_;
      }
      else {
         print STDERR "<WARN>: Couldn't detect a link.\n";
      }
   }
   # Output the links in alphabetical order.
   foreach my $key (sort (keys(%list))) {
      print $list{$key};
   }

   # Add a nice back button.
   print OUT "</TABLE>
   <A HREF=index.html>back</A>
   \n";
}
# We are processing the help message from a program.
else {
   # Make sure the help message is verbatim.
   print OUT "<PRE>";
   while (<IN>) {
      # Replace all occurences of < by &lt which is required for html compatibility.
      s/</&lt;/g;
      # Special replace for programs that have the alternative help message.
      # Both help messages are separated by NRC_HELP_SEPARATOR_TAG. 
      s#NRC_HELP_SEPARATOR_TAG#</PRE><HR><BR><H3>Alternative help message</H3><BR><PRE>#;
      print OUT;
   }
   # Add the back and top button.
   print OUT "</PRE>
   <TABLE width=\"400\">
   <TD><A HREF=index.html>back</A></TD>
   <TD><A HREF=../index.html>top</A></TD>
   </TABLE>
   ";
}
print OUT $footer;

