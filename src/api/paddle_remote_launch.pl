#!/usr/bin/perl -w

# $Id$
# paddle_remote_launch.pl - Remotely launch all paddle_servers where specified
#                           in the constants.php file
#
# PROGRAMMER: Patrick Paul / Eric Joanis
#
# COMMENTS:
#
# Groupe de technologies langagieres interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada

use strict;

print STDERR "paddle_remote_launch.pl, NRC-CNRC, (c) 2005 - 2007, Her Majesty in Right of Canada\n";

sub usage {
    local $, = "\n";
    print STDERR "", @_, "";
    $0 =~ s#.*/##;
    print STDERR "
Usage: $0 [options] -p(ath) <serverpath> constants.php

    Launch all remote servers as specified in constants.php

Arguments:

    -p(ath) <serverpath>  Execute paddle_server from <serverpath>
                          Logs are also saved to <serverpath>
    constants.php         Location of the constants.php file to parse

Options:

    -c <config>           Use configuration <config> [std-config]
    -n(ot_really)         Only print what commands would be executed
    -h(elp)               Show this help message

";
    exit 1;
}


my $__EN_2_FR_SERVER_HOST__ = "0542-crtl";
my $__EN_2_FR_SERVER_PORT_NUM__ = "20002";
my $__FR_2_EN_SERVER_HOST__ = "0568-crtl";
my $__FR_2_EN_SERVER_PORT_NUM__ = "20002";
my $__CH_2_EN_SERVER_HOST__ = "0562-crtl";
my $__CH_2_EN_SERVER_PORT_NUM__ = "20002";


#my $server_loc = "/export/home/paulp/dev/eclipse/workspace/Portage_demo2/src/api/";
my $server_loc = ".";
my $server_img_name = "paddle_server";

my $tst_cfg = "";
#$tst_cfg = " -c test-config";

# Parse command line options
use Getopt::Long;
GetOptions(
    "c=s"       => \$tst_cfg,
    help        => sub { usage },
    not_really  => \my $not_really,
    "path=s"    => \$server_loc,
) or usage;
$server_loc or usage;
$tst_cfg and $tst_cfg = " -c $tst_cfg";

if ( $server_loc !~ /^\// ) {
   # server_loc not absolute, prepend `pwd` to it.
   my $cwd = `pwd`;
   chomp $cwd;
   $server_loc = "$cwd/$server_loc";
}

if ( ! -x ("$server_loc/$server_img_name") ) {
    usage "$server_loc$server_img_name is not executable";
}

# Read the constants.php file to replace default values;
while (<>) {
    /"__EN_2_FR_SERVER_HOST__",\s*"(.*?)"/ and $__EN_2_FR_SERVER_HOST__ = $1;
    /"__FR_2_EN_SERVER_HOST__",\s*"(.*?)"/ and $__FR_2_EN_SERVER_HOST__ = $1;
    /"__CH_2_EN_SERVER_HOST__",\s*"(.*?)"/ and $__CH_2_EN_SERVER_HOST__ = $1;
    /"__EN_2_FR_SERVER_PORT_NUM__",\s*"(.*?)"/ and $__EN_2_FR_SERVER_PORT_NUM__ = $1;
    /"__FR_2_EN_SERVER_PORT_NUM__",\s*"(.*?)"/ and $__FR_2_EN_SERVER_PORT_NUM__ = $1;
    /"__CH_2_EN_SERVER_PORT_NUM__",\s*"(.*?)"/ and $__CH_2_EN_SERVER_PORT_NUM__ = $1;
}

# make_cmd($host, $port, $src_lang, $tgt_lang) returns the command to execute
sub make_cmd ($$$$) {
   my ($host, $port, $src_lang, $tgt_lang) = @_;
   my $cmd = "ssh $host \"export LC_ALL=C; $server_loc/$server_img_name";
   if ( lc($host) eq "leclerc" or $host =~ /0575/ ) {
      $cmd .= "-32on64.sh";
   }
   $cmd .= " -p $port $tst_cfg $src_lang $tgt_lang " .
           " >> $server_loc/${src_lang}2$tgt_lang.log 2>&1 &\"";
   print $cmd, "\n";
   $cmd;
}

#start EN --> FR
my $cmd_en2fr = make_cmd($__EN_2_FR_SERVER_HOST__,
                         $__EN_2_FR_SERVER_PORT_NUM__, "en", "fr");
system($cmd_en2fr) unless $not_really;

#start FR --> EN
my $cmd_fr2en = make_cmd($__FR_2_EN_SERVER_HOST__,
                         $__FR_2_EN_SERVER_PORT_NUM__, "fr", "en");
system($cmd_fr2en) unless $not_really;

#start CH --> EN
my $cmd_ch2en = make_cmd($__CH_2_EN_SERVER_HOST__,
                         $__CH_2_EN_SERVER_PORT_NUM__, "ch", "en");
system($cmd_ch2en) unless $not_really;

