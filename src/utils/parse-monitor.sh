#!/usr/bin/perl 
# $Id$
#
# @file parse-monitor.sh 
# @brief Extract useful information from the output of monitor-process.sh
#
# @author Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2005, Her Majesty in Right of Canada
#

use strict;
use warnings;

print STDERR "parse-monitor.sh, NRC-CNRC, (c) 2005 - 2009, Her Majesty in Right of Canada\n";

sub usage {
    local $, = "\n";
    print STDERR @_, "";
    $0 =~ s#.*/##;
    print STDERR "
Usage: $0 [-h(elp)] [-v(erbose)] [in]

  monitor-process.sh produces a line every 5 seconds using ps.  This script
  reads this output and produces a summary, showing in particular highest
  memory usage, total CPU, and optionally data to construct a time vs memory
  graph.

Caveat:

  This script is not very reliable.  It works fine if your process is not
  parallel, and nothing goes wrong with ps, and there is no ambiguity in
  process names, and etc.  In short, if you use this script, expect glitches!
  It is recommended you pipe its output to less, so you can more easily ignore
  the tons of warnings it sometimes dumps to STDERR.

Options:

  -graph        print data to construct time vs memory graphs
  -xgraph       produce xgraph data and runs xgraph on it
  -only <pid> or -only <pid>:<pid>:<pid>:...
                only print information about specified pid's.
  -period <p>   assume the polling period was every <p> seconds [5]
  -quiet        don't print summaries (only useful with -graph)
  -mb           display the memory in megabytes instead of gigabytes
  -cpu-window <N> smoothing window for the CPU figure in number of <p>
                second periods [120]

  -h(elp):      print this help message
  -d(ebug):     print debugging information

";
  #-v(erbose):   increment the verbosity level by 1 (may be repeated)
    exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
my $period = 5;
my $smoothing_window = 120;
GetOptions(
    help        => sub { usage },
    verbose     => sub { ++$verbose },
    quiet       => sub { $verbose = 0 },
    debug       => \my $debug,
    mb          => \my $mb,
    graph       => \my $graph,
    xgraph      => \my $xgraph,
    "only=s"    => \my @only,
    "period=i"  => \$period,
    "cpu-window=i" => \$smoothing_window,
) or usage;

if ( $debug ) {
    no warnings;
    print STDERR "
    verbose     = $verbose
    debug       = $debug
    mb          = $mb
    graph       = $graph
    xgraph      = $xgraph
    only        = @only
    cpu-window  = $smoothing_window
    period      = $period

";
}

sub print_process_info ();

my $pid = 0;
my $command_line;
my $max_memory = 0;
my $max_ram = 0;
my @previous_start_time; # (hour, min)
my $clock_skew; # in minutes
my $process_start;
my $process_stop;
my $previous_time_elapsed = 0; # to detect and correct for date changes.
# array of array refs: [ $clock_time, $CPU_time, $percent_CPU, $RAM_usage ]
my @graph_data;

# Handle gzipped logs
@ARGV = map { /\.gz$/ ? "gunzip -cdqf $_ |" : $_ } @ARGV;

while (<>) {
    # Split the ps line into eleven fields:
    # 0 user, 1 pid, 2 %CPU, 3 %MEM, 4 virtual memory, 5 resident memory,
    # 6 tty, 7 status, 8 start time, 9 CPU time, 10 command
    # The first 10 are blank separated, the last is the rest of the line.
    my @fields = split /\s+/, $_, 11;
    next if ( scalar(@fields) < 11 );   # not a ps line ... 
    next if / tail -f /;                # ignore watching processes
    next if /parse-monitor/;            # ignore self
    next if / (?:ps|date|rsync|gvim) /;
    next if / \[canoe\]$/;

    my $first_line = 0;
    if ( $pid == 0 ) {
        $pid = $fields[1];
        $command_line = $fields[10];
        $first_line = 1;
    }
    if ( $pid != $fields[1] ) {
        print_process_info;
        $pid = $fields[1];
        $command_line = $fields[10];
        $max_memory = 0;
        $max_ram = 0;
        @previous_start_time = ();
        undef $clock_skew;
        undef $process_start;
        undef $process_stop;
        @graph_data = ();
        $first_line = 1;
        $previous_time_elapsed = 0;
    }

    # Parse the process start time: HH:MM if started the same day, date
    # otherwise
    my ($start_hour, $start_minute);
    my $use_clock_start_time = 0;
    if ( ($start_hour, $start_minute) = $fields[8] =~ /(\d+):(\d+)/ ) {
        @previous_start_time = ($start_hour, $start_minute);
    } else {
        if ( @previous_start_time ) {
            ($start_hour, $start_minute) = @previous_start_time;
        } else {
            $use_clock_start_time = 1;
        }
    }

    # Get the next line (should be the current date) and calculate the time
    # elapsed since the start time
    while (<>) {
        next if / tail -f /;                # ignore watching processes
        next if /parse-monitor/;            # ignore self
        next if / (?:ps|date|rsync|gvim) /;
        next if / \[canoe\]$/;
        last;
    }
    if ( ! defined $_ ) {
        print_process_info;
        last;
    }
    my ($cur_hour, $cur_minute, $cur_sec) = /\d+\s+(\d+):(\d+):(\d+) E[SD]T/
    or do {
        warn "Discarding line with invalid time ($.): $_";
        next;
    };
    if ( $use_clock_start_time ) {
        ($start_hour, $start_minute) = ($cur_hour, $cur_minute);
        @previous_start_time = ($cur_hour, $cur_minute);
    }

    if ( ! defined $process_start ) { $process_start = $_ }
    $process_stop = $_;

    my $time_elapsed = $cur_hour * 60 + $cur_minute
                     - $start_hour * 60 - $start_minute;
    $time_elapsed = $time_elapsed * 60 + $cur_sec;      # convert to seconds

    # Process the CPU time used (turn it into a number of seconds)
    my ($cpu_min, $cpu_sec) = $fields[9] =~ /(\d+):(\d+)/
        or warn "Discarding line with invalid CPU time at $.\n" and next;
    my $cpu_time = $cpu_min * 60 + $cpu_sec;

    if ( $first_line ) {
        # Assume we're running at 100% CPU until the first reading, and that
        # the clock skew is equal to the CPU time used until then, rounded up;
        # this is the most reliable estimator of the real start time we have.
        $clock_skew = $cpu_time - $time_elapsed;
    }
    $time_elapsed += $clock_skew;    # always = $cpu_time on the first line.

    # Check if we rolled over midnight since the process started
    #if ( $time_elapsed < 0 ) { $time_elapsed += 24 * 60 * 60 }
    while ( $time_elapsed < $previous_time_elapsed ) {
        $time_elapsed += 24 * 60 * 60;
    }
    $previous_time_elapsed = $time_elapsed;

    # Process the memory information
    if ( $fields[4] > $max_memory ) {
        $max_memory = $fields[4];
    }
    if ( $fields[5] > $max_ram ) {
        $max_ram = $fields[5];
    }

    # The %CPU field ($fields[2]) is sometimes above 100%, but since we don't
    # do multi-threaded programming, this is not believable, so assume >100
    # means =100.
    if ( $fields[2] > 100 ) {
        $fields[2] = 100
    }

    # Store the time elapsed, CPU time used, memory, and RAM usage in the graph
    # data.
    push @graph_data, [ $time_elapsed, $cpu_time, $fields[2], $fields[4], $fields[5] ];
}
print_process_info if @graph_data;

# Return an elapsed time in HH:MM:SS format, filling only needed parts
sub hms_time($) {
    my $t = shift;
    my $s = $t % 60;
    my $m = int($t / 60);
    if ( $m < 60 ) {
        sprintf("%2d:%02d", $m, $s);
    } else {
        sprintf("%d:%02d:%02d", int($m / 60), $m % 60, $s);
    }
} 

# Print all the information we have about a process.
sub print_process_info () {
    if ( ! @graph_data ) {
        warn "No graph data to print in print_process_info!\n";
        return;
    }

    if ( @only and ! grep { $_ == $pid } @only ) {
        return;
    }

    if ( $verbose > 0 ) {
        chomp $process_start;
        chomp $process_stop;
        print "================= $pid =================\n", $command_line;
        print "Started: $process_start\t\t";
        printf "Max Mem: %7dKB (%.4g%s)\n", $max_memory,
            ($max_memory / 1024 / ($mb ? 1 : 1024)),
            ($mb ? "MB" : "GB");
        print "Stopped: $process_stop\t\t";
        printf "Max RAM: %7dKB (%.4g%s)\n", $max_ram,
            ($max_ram / 1024 / ($mb ? 1 : 1024)),
            ($mb ? "MB" : "GB");
        printf "Total CPU / Clock Time: %s / %s (%.1f%%)\n\n",
            hms_time($graph_data[-1][1]),
            hms_time($graph_data[-1][0]),
            (100 * $graph_data[-1][1] / ($graph_data[-1][0]||1));
    }

    if ( $graph ) {
        print "Elapsed  CPU sec  %CPU  %CPU  RAM (KB)\n";
        for (my $i = 0; $i <= $#graph_data; $i++) {
            my $current_CPU;
            if ( $i < $smoothing_window ) {
                $current_CPU = $graph_data[$i][2];
            } else {
                my $j = $i - $smoothing_window;
                $current_CPU = 100.0 *
                               ($graph_data[$i][1] - $graph_data[$j][1]) /
                               ($graph_data[$i][0] - $graph_data[$j][0]);
                $current_CPU = 100 if $current_CPU > 100;
            }
            printf "%7d %8d %5.1f %5.1f %9d\n", @{$graph_data[$i]}[0,1],
                $current_CPU, @{$graph_data[$i]}[2,3];
        }
        #foreach my $graph_line_data (@graph_data) {
        #    printf "%7d %8d %5.1f %9d\n", @{$graph_line_data};
        #}
    }

    if ( $xgraph ) {
        open XGRAPH, "| `which xgraph`" or do {
            warn "Can't run xgraph: $!\n";
            return;
            #warn "Can't run xgraph - trying stdout: $!\n";
            #open XGRAPH, ">&STDOUT" 
            #    or die "Can't even print to STDOUT - giving up: $!";
        };

        my $time_unit = "min";
        my $seconds_per_time_unit = 60;
        if ( scalar @graph_data > 0 ) {
            if ( $graph_data[-1][0] < 600 ) {
                # total time less than 10 minutes, show time in seconds
                $time_unit = "sec";
                $seconds_per_time_unit = 1;
            } elsif ( $graph_data[-1][0] > 10 * 60 * 60 ) {
                # total time above 10 hours, show time in hours
                $time_unit = "hour";
                $seconds_per_time_unit = 60 * 60;
            }
        }

        my ($K) = $command_line =~ /(-K\s+\d+)/;
        defined $K or $K = "????";
        print XGRAPH
            "TitleText: $pid: $K\n\n",
            "XUnitText: Time ($time_unit)\n",
            "YUnitText: GB / scaled %\n";
        # Curve 1: current CPU usage data
        print XGRAPH "\n\"CPU (", ($smoothing_window * $period / 60), " min avg)\n";
        for (my $i = 0; $i <= $#graph_data; $i++) {
            my $current_CPU;
            if ( $i < $smoothing_window ) {
                $current_CPU = $graph_data[$i][2];
            } else {
                my $j = $i - $smoothing_window;
                $current_CPU = 100 *
                               ($graph_data[$i][1] - $graph_data[$j][1]) /
                               ($graph_data[$i][0] - $graph_data[$j][0]);
                $current_CPU = 100 if $current_CPU > 100;
            }
            print XGRAPH ($graph_data[$i][0]/$seconds_per_time_unit), " ",
                         ($current_CPU / 100 * $max_memory / 1024 / 1024), "\n";
        }
        # Curve 2: total memory usage data
        print XGRAPH "\n\"Memory (GB)\n";
        foreach my $graph_line_data (@graph_data) {
            print XGRAPH ($graph_line_data->[0] / $seconds_per_time_unit), " ",
                         ($graph_line_data->[3] / 1024 / 1024),
                         "\n";
        }
        # Curve 3: RAM memory (i.e., resident memory) usage data
        print XGRAPH "\n\"RAM (GB)\n";
        foreach my $graph_line_data (@graph_data) {
            print XGRAPH ($graph_line_data->[0] / $seconds_per_time_unit), " ",
                         ($graph_line_data->[4] / 1024 / 1024),
                         "\n";
        }
        # Curve 4: cumulative CPU usage data
        print XGRAPH "\n\"CPU (cumul)\n";
        foreach my $graph_line_data (@graph_data) {
            print XGRAPH ($graph_line_data->[0] / $seconds_per_time_unit), " ",
                         ($graph_line_data->[2] / 100 * $max_memory / 1024 / 1024),
                         "\n";
        }
        print XGRAPH "\n";
        close XGRAPH;
    }
}
