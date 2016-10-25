#!/usr/bin/env perl
# @file xml2hashtag.pl
# @brief This code converts sequences of tokens within <hashtag>...</hashtag>
# into a proper, twitter style hashtag. Initial source-text markup is performed
# in tokenize_plugin, and tags are transfered via Portage's tag-transfer
# mechanism (translate.pl -xtags).
#
# @author Michel Simard
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada


use strict;
use warnings;

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
printCopyright 2015;
$ENV{PORTAGE_INTERNAL_CALL} = 1;


sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 < IN > OUT

 This program converts sequences of tokens within <hashtag>...</hashtag> into a
 proper, twitter style hashtag. Initial source-text markup is performed in
 tokenize_plugin, and tags are transfered via Portage's tag-transfer mechanism
 (translate.pl -xtags).
";
   exit @_ ? 1 : 0;
}

usage() if (@ARGV && $ARGV[0] eq "-h");

while (my $line = <STDIN>) {
    chop $line;
    while ($line =~ /<hashtag/) { # handle nested hashtags!!
        $line =~ s{<hashtag>([^<]*)</hashtag>}{hashtagify($1)}ge;
    }
    print "$line\n";
}

sub hashtagify {
    my ($s) = @_;

    $s =~ s/^\s+//;
    $s =~ s/\s+$//;
    $s =~ s/[\s\#]+/_/g;
    $s =~ s/^/\#/;
    $s =~ s/\#(\P{Alnum}+)_/$1 \#/; # remove initial punctuation, etc. in hashtags -- could be repeated

    return $s;
}
