#!/usr/bin/env perl

# @file PortageLiveAPI.pl
# @brief Simple wrapper to invoke PortageLive's API and return the translations.
#
# @author Samuel Larkin
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2013, Sa Majeste la Reine du Chef du Canada /
# Copyright 2013, Her Majesty in Right of Canada


use strict;
use warnings;

BEGIN{use encoding "UTF-8";}

BEGIN {
   # If this script is run from within src/ rather than being properly
   # installed, we need to add utils/ to the Perl library include path (@INC).
   if ( $0 !~ m#/bin/[^/]*$# ) {
      my $bin_path = $0;
      $bin_path =~ s#/[^/]*$##;
      unshift @INC, "$bin_path/../utils";
   }
}
use portage_utils;
printCopyright "prog.pl", 2013;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] -file FILE

  Invoke PortageLive's API and return the translations.

Options:

  -x(tags)
  -context CONTEXT
  -file FILE
  -filter CE_THRESHOLD
  -server SERVER
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
my $server = "iitfrdvm023110";
my $context = "toy-regress-afield";
GetOptions(
   help        => sub { usage },
   verbose     => sub { ++$verbose },
   quiet       => sub { $verbose = 0 },
   debug       => \my $debug,

   xtags        => \my $xtags,
   "context=s"  => \$context,
   "file=s"     => \my $file,
   "filter=f"       => \my $ce_threshold,

   "server=s"  => \$server,
) or usage "Error: Invalid option(s).";

0 == @ARGV or usage "Error: Superfluous argument(s): @ARGV";
die "Error: file required." unless(defined($file));

my $command = "curl";
if ($file =~ m/\.tmx$/i) {
   $command .= " --form tmx_filename=\@$file";
   $command .= " --form 'TranslateTMX=3'";
}
elsif ($file =~ m/\.sdlxliff$/i) {
   $command .= " --form sdlxliff_filename=\@$file";
   $command .= " --form 'TranslateSDLXLIFF=8'";
}
else {
   die "Error: Invalid file type.";
}
$command .= " --form 'ce_threshold=$ce_threshold'" if (defined($ce_threshold));
$command .= " --form 'context=$context'";
$command .= " --form 'xtags=on'" if (defined($xtags));
$command .= " http://$server/soap.php";

print STDERR "command: $command\n";
# Send the translation request.
my $monitor;
{
   open (IN, "$command |") or die "Error: opening piped output from command.";
   local $/ = undef;
   my $content = <IN>;
   # Extract the monitoring token.
   if ($content =~ m/<INPUT TYPE = "TEXT"   Name = "monitor_token" VALUE = "([^>]+)" \/>/) {
      $monitor = $1;
      $monitor =~ s#http://[^/]+/#http://$server/#;
      print STDERR "monitor token: $monitor\n";
   }
   else {
      die "Error: Unable to extract monitor token.";
   }
   close(IN);
}

# Check if the translation is complete.
use LWP::Simple;
my $content = "undef";
my $timeout = 600;
while ($content !~ m/Output file is ready/) {
   sleep 5;
   $content = get($monitor);
   die "Error: Couldn't get it!" unless defined $content;
   die "Error: Timeout\n" if(($timeout -= 5) < 0);
}

# Print out the translation.
if ($content =~ m/Right-click this link to save the file: <a href="([^>]+)">/) {
   my $doc = "http://$server$1";
   print STDERR "DOC: $doc\n";
   print get("$doc");
}
else {
   die "Error: Unable to determine translation file.";
}

#print STDERR "vimdiff <(sed 's/changedate=\"[^\"]*\"//g' soap.php.reply ) <(sed 's/changedate=\"[^\"]*\"//g' PORTAGEshared/test-suite/unit-testing/translate.pl/ref/ref.TestMemory_Unknown.xtags.CE.tmx )", "\n";
