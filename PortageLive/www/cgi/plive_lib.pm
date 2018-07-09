# @file plive_lib.pm
# @brief Commons functions for plive.
#
# @author Samuel Larkin
#
# COMMENTS:
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2015, Sa Majeste la Reine du Chef du Canada /
# Copyright 2015, Her Majesty in Right of Canada

=pod

=head1 SYNOPSIS

    plive_lib.cgi

=head1 DESCRIPTION

This library contains common functions for plive's scripts.


=head1 Available Functions:

=over

=item getTrace

Returns a complex html element that contains the trace of Portage's execution.
The trace's content will be escaped and colorized.  Colors are user defined in
plive.css.

=over

=item *

ERROR: lines will be marked with the '.error' class;

=item *

COMMAND: lines will be marked with the '.command' class;

=item *

[call: lines will be marked with the '.call' class;

=item *

Using plugin: lines will be marked with the '.plugin' class;

=back

=item NRCBanner

Returns a html element containing NRC's logo.

=item NRCFooter

Returns a complex html element with NRC's logo and copyright notice.

=back


=head1 SEE ALSO

plive.cgi.
plive-monitor.cgi.

=head1 AUTHOR

Samuel Larkin

=head1 COPYRIGHT

 Traitement multilingue de textes / Multilingual Text Processing
 Technologies de l'information et des communications /
   Information and Communications Technologies
 Conseil national de recherches Canada / National Research Council Canada
 Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 Copyright 2015, Her Majesty in Right of Canada

=cut


package plive_lib;

use strict;
use warnings;
use CGI qw(:standard);
use CGI::Carp qw/fatalsToBrowser/;
use HTML::Entities;
require Exporter;

our (@ISA, @EXPORT, @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = (
      "getTrace",
      "NRCBanner",
      "NRCFooter"
      );
@EXPORT_OK = qw( getTrace NRCBanner NRCFooter );


sub NRCBanner {
    return
        p({align=>'center'},
          img({src=>'/images/NRC_banner_e.jpg'}));
}


sub NRCFooter {
    return table({border=>0, cellpadding=>0, cellspacing=>0, width=>'100%'},
                 Tr(td({width=>'20%', valign=>'center', align=>'right'},
                       img({src=>'/images/sidenav_graphic.png', height=>44,
                            alt=>'NRC-ICT'})),
                    "\n",
                    td({width=>'60%', valign=>'center', align=>'center'},
                       img({src=>'/images/mainf1.gif', height=>44, width=>286,
                            alt=>'National Research Council Canada'})),
                    "\n",
                    td({width=>'20%', valign=>'center', align=>'left'},
                       img({src=>'/images/mainWordmark.gif', height=>44, width=>93,
                            alt=>'Government of Canada'}))),
                 "\n",
                 Tr(td(),
                    "\n",
                    td({valign=>'top', align=>'center'},
                       small(
                          "PortageII cur", br(),
                          "Traitement multilingue de textes / Multilingual Text Processing", br(),
                          "Technologies de l'information et des communications / Information and Communications Technologies", br(),
                          "Conseil national de recherches Canada / National Research Council Canada", br(),
                          "Copyright 2004&ndash;2016, Sa Majest&eacute; la Reine du Chef du Canada / ",
                          "Her Majesty in Right of Canada", br(),
                          a({href=>"/portage_notices.php"}, "Third party Copyright notices")
                          ))),
                 "\n");
}


# getTrace
# @param trace_file the absolute path to the trace file.
sub getTrace {
   my ($trace_file) = @_;
	if ( $trace_file !~ m/trace$/ ) {
		 $trace_file = HTML::Entities::encode_entities($trace_file, '\&/\"\'<>');
		 return p()
       .h2("Not a valid trace file given")
       .h3($trace_file);
	}

   if (-r $trace_file) {
      open(TRACE, "$trace_file")
         || return h1("Can't open trace file $trace_file.");
      my $trace = do { local $/; <TRACE>; };
      close(TRACE);
      $trace = HTML::Entities::encode_entities($trace, '\&/\"\'<>');  # MUST be done before we add our spans.
      $trace =~ s#^(ERROR: .+)$#<span class="error">$1</span>#mg;
      $trace =~ s#^(.*fatal error: .+)$#<span class="error">$1</span>#mg;
      $trace =~ s#^(.+command not found.*)$#<span class="error">$1</span>#mg;
      $trace =~ s#^(COMMAND: .+)$#<span class="command">$1</span>#mg;
      $trace =~ s#^(\[call: .+)$#<span class="call">$1</span>#mg;
      $trace =~ s#^(Using plugin: .+)$#<span class="plugin">$1</span>#mg;

      return h2("Trace File")
         .pre($trace);
   }
   else {
	$trace_file = HTML::Entities::encode_entities($trace_file, '\&/\"\'<>');
      return p()
         .h2("No readable trace file")
         .h3($trace_file);
   }
}


1;
