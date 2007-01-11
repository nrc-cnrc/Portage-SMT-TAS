#!/usr/bin/perl -w
# $Id$

# \file worker.pl - This is a generic worker program that requests a command
#                   and executes it when done exits
#
# PROGRAMMER: Patrick Paul
#
# COMMENTS:
#
# Groupe de technologies langagières interactives / Interactive Language Technologies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

use strict;

use Fcntl qw(:DEFAULT :flock);
use Getopt::Long;


# validate parameters
my $host = '';
my $port = '';
my $help = '';
my $quota = 60; # in minutes
my $primary = 0;

GetOptions ("host=s"   => \$host,
            "port=i"   => \$port,
            help       => \$help,
            "quota=i"  => \$quota,
            primary    => \$primary,
            );

$primary and $quota = 0;

if($help ne ''){
    PrintHelp();
    exit (-1);
}

exit_with_error("Missing mandatory argument 'host' see --help\n") unless $host ne '';
exit_with_error("Missing mandatory argument 'port' see --help\n") unless $port ne '';

#
# Algorithm: Until we receive "***EMPTY***" request a command, run it,
#            acknowledge completion
#

my $me = `uname -n`;
chomp $me;
$me .= ":" . ($ENV{PBS_JOBID} || "");

if ( $primary ) { $me = "PRIMARY $me"; }

my $start_time = time;
my $reply_rcvd = `echo 'GET ($me)' | netcat $host $port`;

while($reply_rcvd !~ /^\*\*\*EMPTY\*\*\*/i and $reply_rcvd ne ""){
    print STDERR "[" . localtime() . "] ($me) Executing: $reply_rcvd";
    my $rc = system($reply_rcvd);
    print STDERR "[" . localtime() . "] ($me) Exit status $rc\n";
    if ( $quota > 0 and (time - $start_time) > $quota*60 ) {
        # Done my share of work, request a relaunch
        system("echo 'DONE-STOPPING ($me) $reply_rcvd' | netcat $host $port");
        last;
    } else {
        system("echo 'DONE ($me) $reply_rcvd' | netcat $host $port");
        $reply_rcvd = `echo 'GET ($me)' | netcat $host $port`;
    }
}
print STDERR "[" . localtime() . "] ($me) Done.\n";


sub PrintHelp{
print <<'EOF';
  worker.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada

  This script is a generic worker script. It is meant to be used in
  conjunction with:
    - /utils/faucet.pl
    - /utils/faucet_launcher.sh

  The motivation for this trio of scripts is to allow exclusive access to a
  file to guarantee consistent lock of a file while using NFS as the
  underlying filesystem. The problem with NFS is the caching feature of the
  clients and the delay between a write request completion and the actual
  write to physical disk in conjunction with another read access while the
  data is still not available.

  When multiple process tries to lock a file on an NFS mounted file, it
  seams that the locking mechanism guarantees consistency of the file being
  accessed.

  The idea here is that faucet_launcher.sh prepares things so that when a
  "GET" message arrives on given TCP port, the underlying "faucet" (see
  package netpipes) calls faucet.pl with the appropriate parameters to
  access a file extracting the topmost line which in the case of
  canoe-parallel is an actual call to canoe and execute that instruction.

  It would be quite easy to build on this and reuse the worker script to
  achieve other tasks in a parallel environment like on a cluster.

  Prerequesite: having a server on a host listening on a port that reply an
                executable command when it receives the message "GET". This
                same server should accept a command of the form
                "DONE WithSOmeSortOfMessageHere".

  Behaviour: When the program receives ***EMPTY*** this means that the server
             has nothing else to dispatch so we gracefully exits.

 syntax:
    perl worker.pl [options] --host=SomeHost --port=SomePort
 arguments:
    host=SomeHost: hostname to contact to get an instruction
    port=SomePort: The port on which to send requests
 options:
    -help     print this help message
    -quota T  The number of minutes this worker should work before
              requesting a relaunch from the faucet [60] (0 means never
              relaunch, i.e., work until there is no more work.)
    -primary  Indicates this worker is the primary one and should not be
              stopped on a quench request (implies -quota 0)

EOF
}

sub exit_with_error{
    my $error_string = shift;
    print STDERR $error_string;
    exit(-1);
}

