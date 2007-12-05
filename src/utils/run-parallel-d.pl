#!/usr/bin/perl -w
# $Id$
#
# \file run-parallel-d.pl  This script is used in conjuction with netpipes'
#               netcat program, used in worker.pl, and run-parallel.sh.  It
#               accepts connections on a specific port and returns commands to
#               executed when asked.
#
# This script replaces our former faucet/faucet.pl: run-parallel-d.pl is the
# full deamon, receiving requests via a socket and handling them directly,
# without forking (an exclusive lock would be required around the whole fork,
# so there is no gain in speed and only a cost in complication) and without
# launching a new process.  Another big advantage of this deamon is that its
# variables will be in memory rather than on disk, as had to be the case with
# faucet.pl.  Turn around time for workers requesting jobs should now be
# measures in milliseconds rather than in seconds.
#
# PROGRAMMER: Eric Joanis, based on faucet.pl and faucet_launcher.sh, written
# by Patrick Paul and Eric Joanis
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005-2007, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005-2007, Her Majesty in Right of Canada

use strict;

require 5.002;
use Fcntl qw(:DEFAULT :flock);
use Getopt::Long;
use Socket;

#FILE_PREFIX=$1;
#JOBS_FILENAME=$FILE_PREFIX.jobs;

sub log_msg(@);
sub exit_with_error(@);
$0 =~ s#.*/##;

GetOptions (
   "help"               => sub { PrintHelp() }
) or exit_with_error "Type -help for help.\n";

my $file_prefix = shift;

# initial validation
exit_with_error("Missing mandatory argument 'FilePrefix' see --help")
    unless $file_prefix ne '';

# Return a random integer in the range [$min, $max).
#srand(0); # predictable for testing
srand(time() ^ ($$ + ($$ << 15))); # less predictable, for normal use
sub rand_in_range($$) {
   my ($min, $max) = @_;
   return int(rand ($max-$min)) + $min;
}

# Read job file and store in an array.
open JOBFILE, "$file_prefix.jobs"
   or exit_with_error "Can't open $file_prefix.jobs: $!";
my @jobs = <JOBFILE>;
close JOBFILE;

# State variables
my $add_count = 0;      # gets used when an ADD request comes in
my $quench_count = 0;   # gets used when a QUENCH request comes in
my $job_no = 0;         # next job number to launch, as an index into @jobs
my $done_count = 0;     # number of jobs done so far
my $num = @jobs;        # job count in a conveniently scalar variable
my @return_codes;       # return codes from all the jobs

# File that will contain all the return codes from the jobs
open (RCFILE, "> $file_prefix.rc")
   or exit_with_error("can't open $file_prefix.rc: $!");
select RCFILE; $| = 1; select STDOUT;

# This while(1) loop tries to open the listening socket until it succeeds
while ( 1 ) {
   my $port = rand_in_range 10000, 15000;
   my $proto = getprotobyname('tcp');
   socket(Server, PF_INET, SOCK_STREAM, $proto) or exit_with_error "$0 socket: $!";
   setsockopt(Server, SOL_SOCKET, SO_REUSEADDR, pack("l", 1))
      or exit_with_error "$0 setsockopt: $!";
   bind(Server, sockaddr_in($port, INADDR_ANY)) or do {
      if ( $!{"EADDRINUSE"} ) {
         log_msg "port $port already in use,", "trying another one";
         next;
      }
      die "$0 bind: $!";
   };
   listen(Server, SOMAXCONN) or exit_with_error "$0 listen: $!";

   log_msg "started listening on port $port";
   system("echo $port > $file_prefix.port");
   last;
}

# This for loop receives and handles connections until all work is done
my $paddr;
for ( ; $paddr = accept(Client, Server); close Client) {

   # Add one worker now if an add request is in progress
   if ( $add_count > 0 ) {
      log_msg "adding workers ($add_count)";
      LaunchOneMoreWorker();
      --$add_count;
   }

   my ($port, $iaddr) = sockaddr_in($paddr);
   my $name = gethostbyaddr($iaddr, AF_INET);
   log_msg "rcvd conn from $name [" . inet_ntoa($iaddr) . ":$port]";

   my $cmd_rcvd = <Client>;
   select Client; # Make the socket the default "file" to write to.
   if (defined $cmd_rcvd) {
      chomp $cmd_rcvd;
      if ($cmd_rcvd =~ /^PING/i) {
         log_msg $cmd_rcvd;
         print "PONG\n";
      } elsif ($cmd_rcvd =~ /^ADD (\d+)/i) {
         log_msg $cmd_rcvd;
         $add_count += $1;
         if ( $add_count > 0 ) {
            log_msg "adding workers ($add_count)";
            LaunchOneMoreWorker();
            --$add_count;
         }
         print "ADDED\n";
      } elsif ($cmd_rcvd =~ /^QUENCH (\d+)/i) {
         log_msg $cmd_rcvd;
         $quench_count += $1;
         print "QUENCHED\n";
      } elsif ($cmd_rcvd =~ /^KILL/i) {
         log_msg $cmd_rcvd;
         print "KILLED\n";
         close Client;
         exit 2;
      } elsif ($cmd_rcvd =~ /^GET/i) {
         log_msg $cmd_rcvd;
         if ( $cmd_rcvd !~ /GET \(PRIMARY/ and $quench_count > 0 ) {
            # dynamic quench in progress, stop this (non-primary) worker
            --$quench_count;
            log_msg "quenching ($quench_count)";
         } else {
            # send the next command for execution
            if ( $job_no < $num ) {
               print $jobs[$job_no];
               ++$job_no;
               log_msg "starting ($job_no):", $jobs[$job_no-1];
            } else {
               print "***EMPTY***\n";
               log_msg "returning: ***EMPTY***";
            }
         }
      } elsif ($cmd_rcvd =~ /^DONE/i) {
         ++$done_count;
         log_msg "$done_count/$num $cmd_rcvd";

         if ( $job_no < $done_count ) {
            log_msg "Something strange is going on: more jobs done",
                    "($done_count/$num) than started ($job_no)..."
         }

         # Write the return code of the job (shown as "(rc=NN)" on the "DONE"
         # command) to the .rc file
         my ($rc) = ($cmd_rcvd =~ /\(rc=(-?\d+)\)/);
         defined $rc or $rc = -127;
         push @return_codes, $rc;
         print RCFILE "$rc\n";

         if ( $done_count >= $num ) {
            # If all done, exit
            log_msg "ALL_DONE ($done_count/$num): Killing deamon";
            close Client;
            exit 0;
         } elsif ($cmd_rcvd =~ /^DONE-STOPPING/i and $job_no < $num) {
            # We're not done, but the worker is stopping, so launch
            # another one
            LaunchOneMoreWorker();
         }
      } else {
         #report as an error
         print "UNKNOWN COMMAND\n";
         log_msg "UNKNOWN COMMAND received: $cmd_rcvd";
      }
   } else {
      print "NO COMMAND\n";
      log_msg "EMPTY: received nothing";
   }
}


my $psub_cmd;
my $worker_id;
sub LaunchOneMoreWorker {
   if ( ! defined $psub_cmd || ! defined $worker_id ) {
      # Read the psub command and the next worker id, needed to launch
      # more workers.  But do so only the first time needed, after that
      # reuse the values kept in memory.
      $psub_cmd = `cat $file_prefix.psub_cmd`; chomp $psub_cmd;
      $worker_id = `cat $file_prefix.next_worker_id`; chomp $worker_id;
   }
   my $psub_cmd_copy = $psub_cmd;
   $psub_cmd_copy =~ s/__WORKER__ID__/$worker_id/g;
   log_msg "Launching worker $worker_id";
   my $rc = system($psub_cmd_copy);
   $rc == 0 or log_msg "Error launching worker.  RC = $rc.  ",
                       "Command was:\n    $psub_cmd_copy";
   ++$worker_id;
}

sub PrintHelp {
   print <<'EOF';
Usage: run-parallel-d.pl FilePrefix

  This deamon accepts connections on a randomly selected port and hands
  out the jobs in FilePrefix.jobs one at a time when GET requests are
  received.

Argument:

  FilePrefix - prefix of various argument files:

     FilePrefix.jobs - list of jobs to run, one per line
     Must exist before this script is called.

     FilePrefix.psub_cmd - command to run to launch more workers, with
     __WORKER__ID__ as a placeholder for the worker number.
     FilePrefix.worker - next worker number
     These two files may be created after this script has started, but must
     exist before requests involving new workers are made to this deamon.

     FilePrefix.port - will be created by this script and contain the port
     it listens on, as soon as this port is open for connections.

     FilePrefix.rc - will be created by this script, it will contain the
     return code from each job, in the order they finished (which is not
     necessarily the order they started).

Return status:

  0 - success
  1 - problem
  2 - exited because a KILL message was received

Valid messages -- response:

  GET  -- The first job not started yet is written back on the socket, or
          ***EMPTY*** if all jobs have started, to tell the worker to quit.

  DONE -- Increments the jobs-done counter.  When all jobs are done, exits and
          let run-parallel.sh clean up and exit.

  DONE-STOPPING -- Same as DONE, but the worker is exiting, a new worker is
          launched to replace it, and will find itself at the end of the PBS
          queue, thus allowing other jobs to run in between.  The new
          worker is launched using the command in FilePrefix.psub_cmd.

  ADD <N> -- Request to add more workers, results in the command in
          FilePrefix.psub_cmd being executed <N> times.

  QUENCH <N> -- Request to quench workers, results in the next <N> workers
          requesting jobs being told there are none left.

  KILL -- Request to stop the deamon, makes it exit now, letting
          run-parallel.sh clean up.

  PING -- PONG

EOF

   exit 0;
}

sub exit_with_error(@) {
   log_msg "$0:", @_;
   exit(1);
}

sub log_msg(@) {
   print STDERR "[" . localtime() . "] @_\n";
}
