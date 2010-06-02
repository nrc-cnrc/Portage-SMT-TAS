#!/usr/bin/perl -w
# @file plive-monitor.cgi
# @brief PORTAGE live monitor CGI script
#
# @author Michel Simard
#
# COMMENTS:
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

=pod

=head1 SYNOPSIS

    plive-monitor.cgi

=head1 DESCRIPTION

This program is a companion to F<plive.cgi>; together, they implement
an HTTP interface to PORTAGE live.

Translation jobs submitted to PORTAGE live through the F<plive.cgi>
script are sometimes run in the background.  This script allows the
user to monitor the progression of the job, and ultimately to
recuperate the result.

=head1 SEE ALSO

plive.cgi.

=head1 AUTHOR

Michel Simard

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 Copyright 2010, Her Majesty in Right of Canada

=cut

use strict;
use strict 'refs';
use warnings;

## --------------------- USER CONFIGURATION ------------------------------
##
## NOTE: if you change any of this, also change it in plive.cgi

## Where PORTAGE files reside -- standard is /opt/Portage 
my $PORTAGE_PATH = "/opt/Portage";

## Where PORTAGE executables reside -- standard is ${PORTAGE_PATH}/bin
my $PORTAGE_BIN = "${PORTAGE_PATH}/bin";

## Where PORTAGE code libraries reside -- standard is ${PORTAGE_PATH}/lib
my $PORTAGE_LIB = "${PORTAGE_PATH}/lib";

## Top of web
my $WEB_PATH = "/var/www/html"; 

# Set this to 1 if you want the working directory to be cleaned up after successful runs.
my $DO_CLEAN_UP = 0;

## ---------------------- END USER CONFIGURATION ---------------------------
##
## below this line, you're on your own...

$ENV{PORTAGE} = $PORTAGE_PATH;
$ENV{PATH} = join(":", $PORTAGE_BIN, $ENV{PATH});
$ENV{PERL5LIB} = exists $ENV{PERL5LIB} ? join(":", $PORTAGE_LIB, $ENV{PERL5LIB}) : $PORTAGE_LIB;
$ENV{LD_LIBRARY_PATH} = exists $ENV{LD_LIBRARY_PATH} ? join(":", $PORTAGE_LIB, $ENV{LD_LIBRARY_PATH}) : $PORTAGE_LIB;
push @INC, $PORTAGE_LIB;

use CGI qw(:standard);
use CGI::Carp qw/fatalsToBrowser/;
use File::Spec::Functions qw(splitdir catdir);

$|=1;

print header(-type=>'text/html');
        
## Script expects three parameters: file, dir and time
if (my $filename = param('file')     # The name of the file we are monitoring
    and my $work_dir = param('dir')  # The work directory
    and my $start_time = param('time')) { # When the job started

    my $filepath = catdir($WEB_PATH, $work_dir, $filename);
    my $url = catdir("", $work_dir, $filename);
    my $elapsed_time = time() - $start_time;

    my $canoe_in = catdir($WEB_PATH, $work_dir, "q.tok");
    my $canoe_out = catdir($WEB_PATH, $work_dir, param('ce') ? "p.raw" : "p.dec");
    my $ce_out = catdir($WEB_PATH, $work_dir, "pr.ce");
    my $job_done = catdir($WEB_PATH, $work_dir, "done");
    my $trace_file = catdir($WEB_PATH, $work_dir, "trace");
    
    # Background process is done
    if (-e $job_done) {
        print head($filename);

        if (-r $filepath) { # The output file exists, so presumably everything went well
            if ($DO_CLEAN_UP) {
                my @files = <${WEB_PATH}/${work_dir}/*>;
                for my $file (@files) {
                    unlink $file unless ($file =~ /${filename}/);
                }
            }
            print 
                p("Output file is ready.  Right-click this link to save the file:",
                a({-href=>$url}, $filename));
        } else { # The output file doesn't exist, so something went wrong
            print 
                p("Translation job terminated with no output"),
                hr(),
                getTrace($trace_file);
        } 
        print p("Elapsed time: ${elapsed_time} seconds.");
        print p(a({-href=>"plive.cgi"}, "Translate more text"));

    # Background process is still running:
    } else {
        print head($filename, 5); # Refresh in 5 seconds

        # What stage are we at:
        if (not -r $canoe_in) { # No decoder-ready file yet: still pre-processing
            print p("Preparing input...");
        } elsif (not -r $canoe_out) { # No decoder-output file yet: probably loading models
            my $in_count = int(`wc --lines < $canoe_in`) + 0;
            print p("Preparing to translate ${in_count} segments...");
        } else { # There is an output file, let's see how much of the input is processed:          
            my $in_count = int(`wc --lines < $canoe_in`) + 0;
            my $out_count = int(`wc --lines < $canoe_out`) + 0;   
            print p("Translated ${out_count} of ${in_count} segments...");

            if ($in_count == $out_count) { # Means decoding is done
                if (param('ce') and not -r $ce_out) { # we might be estimating confidence
                    print p("Estimating confidence...");
                } else {        # or just postprocessing
                    print p("Preparing output...");
                }
            }
        }
        
        print p("Elapsed time: ${elapsed_time} seconds.");
    }
} else {
        print head("Nothing to monitor.");
}

print tail();

exit 0;


## Subs

sub head {
    my($filename, $refresh) = @_;
    $refresh = 0 unless defined $refresh;

    my %start = (-title=>"PORTAGELive");
    $start{-head} = meta({-http_equiv=>'refresh',
                          -content =>$refresh})
        if $refresh > 0;

    return (start_html(%start),
            NRCBanner(),
            h1("PORTAGELive"),
            p("Processing file $filename"));
}

sub tail() {
    return (hr(),
            NRCFooter(),
            end_html());
}

sub NRCBanner {
    return 
        p({align=>'center'}, 
          img({src=>'/images/NRC_banner_e.jpg'}));
}

sub NRCFooter {
    return table({border=>0, cellpadding=>0, cellspacing=>0, width=>'100%'},
                 Tr(td({width=>'20%', valign=>'bottom', align=>'right'},
                       img({src=>'/images/iit_sidenav_graphictop_e.gif', height=>54,
                            alt=>'NRC-IIT - Institute for Information Technology'})),
                    td({width=>'60%', valign=>'bottom', align=>'center'},
                       img({src=>'/images/mainf1.gif', height=>44, width=>286,
                            alt=>'National Research Council Canada'})),
                    td({width=>'20%', valign=>'center', align=>'left'},
                       img({src=>'/images/mainWordmark.gif', height=>44, width=>93,
                            alt=>'Government of Canada'}))),
                 Tr(td({valign=>'top', align=>'right'},
                       img({src=>'/images/iit_sidenav_graphicbottom_e.gif',
                            alt=>'NRC-IIT - Institute for Information Technology'})),
                    td({valign=>'top', align=>'center'},
                       small(
                          "Technologies langagi&egrave;res interactives / Interactive Language Technologies", br(),
                          "Institut de technologie de l'information / Institute for Information Technology", br(),
                          "Conseil national de recherches Canada / National Research Council Canada", br(),
                          "Copyright 2004&ndash;2010, Sa Majest&eacute; la Reine du Chef du Canada / ",
                          "Her Majesty in Right of Canada", br(),
                          "<a href=\"/portage_notices.html\">Third party Copyright notices</a>"))));
}

        
sub getTrace {
    my ($trace_file) = @_;
    return (-r $trace_file     
            ? h1("Trace File").pre(`cat $trace_file`)
            : h1("No readable trace file"));
}

1;
