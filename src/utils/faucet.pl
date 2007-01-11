#!/usr/bin/perl -w
# $Id$

# \file faucet.pl This script is used in conjunction with netpipes faucet
#                 program. It allows for the processing of a set of operation
#                 by a worker process spawn when a TCP message on a specific
#                 port.
#
# PROGRAMMER: Patrick Paul
#
# COMMENTS:
#
# Groupe de technologies langagières interactives / Interactive Language Techno ogies Group
# Institut de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada

use strict;

use Fcntl qw(:DEFAULT :flock);
use Getopt::Long;

# validate parameters
my $file_prefix = '';
my $host = '';
my $port = '';
my $num = '';
my $help = '';
my $file_content = "";

GetOptions ('file_prefix=s' => \$file_prefix,
            'num=i' => \$num,
            "help" => \$help,
            );

if($help ne ''){
    PrintHelp();
    exit (-1);
}

# initial validation
exit_with_error("Missing mandatory argument 'file_prefix' see --help\n")
    unless $file_prefix ne '';
exit_with_error("Missing mandatory argument 'num' see --help\n")
    unless $num ne '';
exit_with_error("File $file_prefix.todo doesn't exist\n")
    unless -e "$file_prefix.todo";
#exit_with_error("File $file_prefix.psub_cmd doesn't exist\n")
#    unless -e "$file_prefix.psub_cmd";


#
# Algorithm: We read stdin for the command type, We open the file, read
# the content, remove the first line, write the result, truncate the end
# of the file.
#

sysopen(FH, "$file_prefix.todo", O_RDWR | O_CREAT)
    or exit_with_error("can't open file $file_prefix.doto: $!\n");

# autoflush FH
my $ofh = select(FH);
$| = 1;
select ($ofh);

# after the flock call we are in a controlled area no other call to the
# faucet facility should go through
flock(FH, LOCK_EX) or exit_with_error("can't write-lock file: $!\n");

# Look for an add request and process it before processing the received command
if ( -f "$file_prefix.add" ) {
    my $add_count = 0 + `cat $file_prefix.add`;
    if ( $add_count > 0 ) {
        print STDERR "[" . localtime() . "] adding workers ($add_count)\n";
        LaunchOneMoreWorker();
        $add_count--;
        system "echo $add_count > $file_prefix.add";
    } else {
        system "rm -f $file_prefix.add";
    }
}

#STEP 1 - Read request on STDIN
my $cmd_rcvd = <STDIN>;

if (defined $cmd_rcvd) {
    if ($cmd_rcvd =~ /^PING/i) {
        print "PONG";
        print STDERR "[" . localtime() . "] $cmd_rcvd";
    }
    elsif ($cmd_rcvd =~ /^GET/i) {
        if ( $cmd_rcvd !~ /GET \(PRIMARY/ and -f "$file_prefix.quench" ) {
            # dynamic quench request found, stop this (non-primary) worker
            my $quench_count = 0 + `cat $file_prefix.quench`;
            if ( $quench_count <= 1 ) {
                system "rm -f $file_prefix.quench";
            } else {
                my $remaining_quench = $quench_count - 1;
                system "echo $remaining_quench > $file_prefix.quench";
            }
            print STDERR "[" . localtime() . "] $cmd_rcvd";
            print STDERR "[" . localtime() . "] quenching ($quench_count)\n";
        } else {
            # extract first line from the todo file and write it to stdout
            my ($fetched_line,$remaining_lines) = GetFirstLine(*FH);
            my $line_no = $num - $remaining_lines;
            print $fetched_line;
            print STDERR "[" . localtime() . "] $cmd_rcvd";
            print STDERR "[" . localtime() . "] " .
                ($fetched_line eq "***EMPTY***\n"
                    ? "returning: $fetched_line"
                    : "starting ($line_no): $fetched_line");
        }
    }
    elsif ($cmd_rcvd =~ /^DONE/i) {
        # check if all done (todo file is empty & # of line in done file equals
        # $num) if so kill PPID and exit
        my $done_file_len = WriteProgressToFile("$file_prefix.done", $cmd_rcvd);
        my $todo_file_len = 0 + `wc -l < "$file_prefix.todo"`;
        print STDERR "[" . localtime() . "] $done_file_len/$num $cmd_rcvd";

        # If all done kill ppid
        if ($done_file_len >= $num and $todo_file_len == 0) {
            my $MY_PPID = getppid();
            print STDERR "[" . localtime() . "] ALL_DONE ($done_file_len/$num): Killing faucet\n";
            system("kill -1 $MY_PPID");
        }
        elsif ($done_file_len >= $num and $todo_file_len != 0) {
            print STDERR "[" . localtime() . "] This should not happen ([done_file_len=done_file_len,num=$num,todo_file_len=$todo_file_len\n)";
            print STDERR "[" . localtime() . "] faucet process still running\n";
        }
        elsif ($cmd_rcvd =~ /^DONE-STOPPING/i and $todo_file_len > 0) {
            # We're not done, but the worker is stopping, so launch another one
            LaunchOneMoreWorker();
        }
    }
    else {
        #report as an error
        print STDERR "[" . localtime() . "] UNKNOWN: received: $cmd_rcvd";
    }
}
else {
    print STDERR "[" . localtime() . "] EMPTY: received nothing\n";
}


close(FH) or exit_with_error("can't close file: $!\n");

sub LaunchOneMoreWorker {
    my $psub_cmd = `cat $file_prefix.psub_cmd`; chomp $psub_cmd;
    my $worker_id = `cat $file_prefix.next_worker_id`; chomp $worker_id;
    my $next_worker_id = $worker_id + 1;
    system ("echo $next_worker_id > $file_prefix.next_worker_id");
    #print STDERR "w_id $worker_id next_w_id $next_worker_id\n";
    $psub_cmd =~ s/__WORKER__ID__/$worker_id/g;
    print STDERR "[" . localtime() . "] Launching worker $worker_id\n";
    my $rc = system($psub_cmd);
    $rc == 0 or print STDERR "Error launching worker.  RC = $rc.  ",
                             "Command was:\n    $psub_cmd\n";
}

sub WriteProgressToFile {
    my $local_filename = shift;
    my $local_cmd = shift;
    my $line_cpt = 0;

    #open done file
    open(DONE_FH, ">> $local_filename")
        or exit_with_error("can't open file: $!\n");

    #append progress
    print DONE_FH $local_cmd or exit_with_error("can't write file: $!\n");

    #close file
    close(DONE_FH) or exit_with_error("can't close file: $!\n");

    $line_cpt = 5;
    $line_cpt = 0 + `wc -l < $local_filename`;

    return $line_cpt;
}

sub GetFirstLine {
    my $local_FH = shift;
    my $retv = "";

    my $first_line = <$local_FH>;

    my $remaining_lines = 0;
    if(defined $first_line){
        #get rid of lines with only a \n causing problem;
        while(<$local_FH>){
            if ($_ ne "\n") {
                $file_content .= $_;
                ++$remaining_lines;
            }
        }

        #rewind file
        seek($local_FH, 0, 0) or exit_with_error("can't rewind file : $!\n");

        #print remaining instructions back to beg of file
        print $local_FH $file_content or exit_with_error("can't write file: $!\n");

        #truncate the remaining of the file getting rid of 1 line overall
        truncate($local_FH, tell($local_FH)) or exit_with_error("can't truncate file: $!\n");

        $retv = $first_line;
    }
    else{
        #here we here we return an empty file token. (reserved word meaning eof)
        $retv = "***EMPTY***\n";
        $remaining_lines = -1;
    }

    return ($retv, $remaining_lines);
}

sub PrintHelp {
print <<'EOF';
  faucet.pl, Copyright (c) 2005 - 2006, Conseil national de recherches Canada / National Research Council Canada

  NAME: faucet.pl

  DESCRIPTION:

  This script allows exclusive and guaranteed lock of a file even over NFS.
  This script is intend to be run by the "faucet" process (see package
  netpipes).

  When receiving a "GET" on STDIN:
    - The scripts opens the file given as the file_prefix calling parameter;
    - Request the OS to place a lock on that file;
    - Extracts (remove) the first line from the file;
    - Write this line to STDOUT if the file was empty writes ***EMPTY***
      instead;
    - Close the file releasing the lock;

  When receiving a "DONE Message" on STDIN:
    - Write that line to a "done" file locking the file first;
    - If after writing the "DONE" message to the file, the number of lines in
      the .done file is greater or equal to the num parameter and the .todo
      file is empty, everything is done and we send a signal to the parent
      porcess to stop it from accepting new messages trigerring clean shutdown
      of the application

  When receiving a "PING" on STDIN:
    - Returns a "PONG" in reply.

 syntax:
    perl faucet.pl --file_prefix=SomeFilePrefix -num=10
 options:
    file_prefix=SomeFilePrefix: (mandatory) prefix of file to process:
                 file_prefix.todo must contain the jobs to run.
                 file_prefix.psub_cmd must contain the psub command that
                 will launch a new worker.
                 WARNING: this file gets eaten away as commands are given
                 to workers
    num=numeral: (mandatory) the number of line in the original set of
                 instructions

 Note: This script is intended to be run in conjunction with faucet see
       netpipes documentation (required for end condition to kill parent
       faucet process)

EOF
}

sub exit_with_error {
    my $error_string = shift;
    print STDERR "[" . localtime() . "]" .  $error_string;
    exit(-1);
}
