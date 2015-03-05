#!/usr/bin/perl -w
# @file plive-monitor.cgi
# @brief PortageLive monitor CGI script
#
# @author Michel Simard
#
# COMMENTS:
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

=pod

=head1 SYNOPSIS

    plive-monitor.cgi

=head1 DESCRIPTION

This program is a companion to F<plive.cgi>; together, they implement
an HTTP interface to PortageLive.

Translation jobs submitted to PortageLive through the F<plive.cgi>
script are sometimes run in the background.  This script allows the
user to monitor the progression of the job, and ultimately to
recuperate the result.

=head1 SEE ALSO

plive.cgi.

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

 Traitement multilingue de textes / Multilingual Text Processing
 Technologies de l'information et des communications /
   Information and Communications Technologies
 Conseil national de recherches Canada / National Research Council Canada
 Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 Copyright 2010, Her Majesty in Right of Canada

=cut

use strict;
use strict 'refs';
use warnings;
use utf8;
use plive_lib;

## --------------------- USER CONFIGURATION ------------------------------
##
## NOTE: if you change any of this, also change it in plive.cgi

## Where PortageII files reside -- standard is /opt/PortageII
my $PORTAGE_PATH = "/opt/PortageII";

## Where PortageII executables reside -- standard is ${PORTAGE_PATH}/bin
my $PORTAGE_BIN = "${PORTAGE_PATH}/bin";

## Where PortageII code libraries reside -- standard is ${PORTAGE_PATH}/lib
my $PORTAGE_LIB = "${PORTAGE_PATH}/lib";

## Top of web
my $WEB_PATH = "/var/www/html";

# Set this to 1 if you want the working directory to be cleaned up after successful runs.
my $DO_CLEAN_UP = 0;

## ---------------------- END USER CONFIGURATION ---------------------------
##
## below this line, you're on your own...

$ENV{PORTAGE} = $PORTAGE_PATH;
#$ENV{PATH} = join(":", $PORTAGE_BIN, $ENV{PATH});
#$ENV{PERL5LIB} = exists $ENV{PERL5LIB} ? join(":", $PORTAGE_LIB, $ENV{PERL5LIB}) : $PORTAGE_LIB;
#$ENV{LD_LIBRARY_PATH} = exists $ENV{LD_LIBRARY_PATH} ? join(":", $PORTAGE_LIB, $ENV{LD_LIBRARY_PATH}) : $PORTAGE_LIB;
push @INC, $PORTAGE_LIB;

use CGI qw(:standard);
use CGI::Carp qw/fatalsToBrowser/;
use File::Spec::Functions qw(splitdir catdir);

$|=1;

print header(-type=>'text/html; charset=utf8');

# Calculate the number of seconds from $start_time to the modification time of
# $file
sub time_delta($$) {
    my ($start_time, $file) = @_;
    my @stats = stat($file);
    $stats[9] - $start_time;
}

## Script expects these parameters: file, dir, time, ce and context
if (my $filename = param('file')     # The name of the file we are monitoring
    and my $work_dir = param('dir')  # The work directory
    and my $start_time = param('time')  # The start time
    and my $context = param('context')) {  # What context (model)

    my $ce = int(param('ce'));  # Are we estimating confidence?

    my $filepath = catdir($WEB_PATH, $work_dir, $filename);
    my $url = catdir("", $work_dir, $filename);
    my $elapsed_time = time() - $start_time;

    my $full_work_dir = catdir($WEB_PATH, $work_dir);
    if ( ! -d $full_work_dir ) {
        print pageHead(0);
        print p("Can't find working directory $work_dir.");
    } else {
        my $input = catdir($WEB_PATH, $work_dir, "Q.in");
        my $canoe_in = catdir($WEB_PATH, $work_dir, "q.tok");
        my $p_raw = catdir($WEB_PATH, $work_dir, "p.raw");
        my $canoe_out = catdir($WEB_PATH, $work_dir, "p.dec");
        $canoe_out = $p_raw if (-r $p_raw);
        my $canoe_parallel_out = catdir($WEB_PATH, $work_dir, "canoe-parallel*", "out*");
        my $ce_out = catdir($WEB_PATH, $work_dir, "pr.ce");
        my $job_done = catdir($WEB_PATH, $work_dir, "done");
        my $trace_file = catdir($WEB_PATH, $work_dir, "trace");
        my $monitor_log = catdir($WEB_PATH, $work_dir, "monitor_log");

        my $trace_url = catdir("", $work_dir, "trace");

        if (-e $job_done) {
            print pageHead($filename, $context); # Background process is done
        } else {
            print pageHead($filename, $context, 5); # Refresh in 5 seconds - still running
        }

        #if ( -r $input ) {
        #    print br("Time delta from start to Q.in: " . time_delta($start_time, $input) . " seconds.");
        #}

        # Even if the background process is done, print the timing information for
        # each stage
        #print p("canoe_in: $canoe_in");
        if (not -r $canoe_in) { # No decoder-ready file yet: still pre-processing
            print br("Loading ...");
        } else {
            print br("Loading ... elapsed time: " . time_delta($start_time, $canoe_in) . " seconds.");
            #print p("canoe_out: $canoe_out");
            if (not -r $canoe_out) { # No decoder-output file yet: probably loading models
                my $in_count = int(`wc --lines < $canoe_in`) + 0;
                print br("Preparing to translate ${in_count} segments...");
            } else {
                my $in_count = int(`wc --lines < $canoe_in`) + 0;
                my $out_count = int(`wc --lines < $canoe_out`) + 0;
                if ($in_count != $out_count) { # Means decoding in progress
                    my $parallel_out_count = int(`cat $canoe_parallel_out 2> /dev/null | wc --lines`) + 0;
                    if ($parallel_out_count > $out_count) { $out_count = $parallel_out_count; }
                    print br("Translated ${out_count} of ${in_count} segments...");
                } else { # Means decoding is done
                    print br("Translated ${out_count} of ${in_count} segments... elapsed time: "
                            . time_delta($start_time, $canoe_out) . " seconds");
                    if (param('ce') and not -r $ce_out) { # we might be estimating confidence
                        print br("Estimating confidence...");
                    } else {        # or just postprocessing
                        if (param('ce')) {
                            print br("Estimating confidence... elapsed time: "
                                     . time_delta($start_time, $ce_out) . " seconds");
                        }
                        #print p("job_done: $job_done");
                        if (not -e $job_done) {
                            print br("Preparing output...");
                        } else {
                            print br("Preparing output... elapsed time: " . time_delta($start_time, $job_done) . " seconds");
                        }
                    }
                }
            }
        }
        if (not -e $job_done) {
            print p("Elapsed time so far: $elapsed_time seconds.");
        } else  { # Background process is done
            $elapsed_time = time_delta($start_time, $job_done);
            print p("Total job processing time: ${elapsed_time} seconds.");

            if (-r $filepath) { # The output file exists, so presumably everything went well
                if ($DO_CLEAN_UP) {
                    my @files = <${WEB_PATH}/${work_dir}/*>;
                    for my $file (@files) {
                        unlink $file unless ($file =~ /${filename}/);
                    }
                }
                print
                    p("Output file is ready.  Right-click this link to save the file:",
                    a({-href=>$url, -id=>"translations_file"}, $filename));
            } else { # The output file doesn't exist, so something went wrong
                print
                    p("Translation job terminated with no output.");
            }
            print p(a({-href=>"plive.cgi?context=$context"}, "Translate more text"));
            if (open MONITOR, ">$monitor_log") {
                my $wc_output = `wc --lines < $canoe_out 2> /dev/null`;
                my $out_count = $wc_output ? (int($wc_output)+0) : 0;
                print MONITOR "Translated ${out_count} segments in ${elapsed_time} seconds.";
                close MONITOR;
            }
        }
        print p("In case of problems, have a look at the job's ",
                 a({-href=>"plive-monitor.cgi?traceFile=$trace_url"}, "trace file"),
                 ".");
    }
}
elsif (my $traceFile = param('traceFile')) {
    my %start = (
       -title=>"PORTAGELive",
       -encoding=>'UTF8',
       -lang=>'fr-CA',
       -style => {-src => '/plive.css'}
    );

    print start_html(%start),
           NRCBanner(),
           h1("PORTAGELive");

    print getTrace($WEB_PATH . $traceFile);
}
else {
    print pageHead(0);
}

print pageTail();

exit 0;


## Subs

sub pageHead {
    my($filename, $context, $refresh) = @_;
    $refresh = 0 unless defined $refresh;

    my %start = (-title=>"PORTAGELive", -encoding=>'UTF8', -lang=>'fr-CA');
    $start{-head} = meta({-http_equiv=>'refresh',
                          -content =>$refresh})
        if $refresh > 0;

    return (start_html(%start),
            NRCBanner(),
            h1("PORTAGELive"),
            $filename
            ? p("Processing file $filename with system $context")
            : p("No job to monitor.  " , a({-href=>"plive.cgi"}, "Submit a job")));
}

sub pageTail {
    return (hr(),
            NRCFooter(),
            end_html());
}

1;
