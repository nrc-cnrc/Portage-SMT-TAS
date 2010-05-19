#!/usr/bin/perl -w

use strict 'refs';
use warnings;
use CGI qw(:standard);
use CGI::Carp qw/fatalsToBrowser/;
use File::Spec::Functions qw(splitdir catdir);

my $PORTAGE_PATH = "/opt/Portage";
my $PORTAGE_BIN = "${PORTAGE_PATH}/bin";
my $PORTAGE_LIB = "${PORTAGE_PATH}/lib";
my $WEB_PATH = "/var/www/html";

my $OUTPUT_TMX = 0;

$ENV{PORTAGE} = $PORTAGE_PATH;
$ENV{PATH} = join(":", $PORTAGE_BIN, $ENV{PATH});
$ENV{PERL5LIB} = exists $ENV{PERL5LIB} ? join(":", $PORTAGE_LIB, $ENV{PERL5LIB}) : $PORTAGE_LIB;
$ENV{LD_LIBRARY_PATH} = exists $ENV{LD_LIBRARY_PATH} ? join(":", $PORTAGE_LIB, $ENV{LD_LIBRARY_PATH}) : $PORTAGE_LIB;
push @INC, $PORTAGE_LIB;

$|=1;

our ($v, $verbose, $Verbose, $mode, $host, $key);

$verbose = 0 unless defined $verbose;
$Verbose = 0 unless defined $Verbose;

print header(-type=>'text/html');
        
if (my $dir = param('dir')
    and my $file = param('file')
    and my $start_time = param('time')) {
    my $filepath = catdir($WEB_PATH, $dir, $file);
    my $url = catdir("", $dir, $file);
    my $time = time() - $start_time;

    my $canoe_in = catdir($WEB_PATH, $dir, "q.tok");
    my $canoe_out = catdir($WEB_PATH, $dir, "p.raw");
    my $ce_out = catdir($WEB_PATH, $dir, "pr.ce");

    if (-r $filepath) {
        print start_html("PORTAGELive");

        print NRCBanner(),
              h1("PORTAGELive");

        print p("Output file is ready.  Right-click this link to save the file:",
                a({-href=>$url}, $file));

    } else {
        print start_html(-head=>meta({-http_equiv => 'refresh',
                                      -content => '5'}),
                         -title=>"PORTAGELive");

        print NRCBanner(),
              h1("PORTAGELive");

        print p("Processing file $file");

        if (not -r $canoe_in) {
            print p("Preparing input...");
        } elsif (not -r $canoe_out) {
            my $in_count = int(`wc --lines < $canoe_in`) + 0;
            print p("Preparing to translate ${in_count} segments...");
        } else {
            my $in_count = int(`wc --lines < $canoe_in`) + 0;
            my $out_count = int(`wc --lines < $canoe_out`) + 0;
            print p("Translated ${out_count} of ${in_count} segments...");
            if ($in_count == $out_count) {
                if (not -r $ce_out) {
                    print p("Estimating confidence...");
                } else {
                    print p("Preparing output...");
                }
            }
        }
        
    }
    print(p("Elapsed time: $time seconds."));
} else {
        print start_html("PORTAGELive");

        print NRCBanner(),
              h1("PORTAGELive");

        print p("Nothing to monitor.");
}
print copyright();

exit 0;

sub NRCBanner {
    return p({align=>'center'}, img({src=>'/images/NRC_banner_e.jpg'}));
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
                          "Her Majesty in Right of Canada"))));
}

        
sub copyright() {
    return (hr(),
            NRCFooter(),
            end_html());
}
