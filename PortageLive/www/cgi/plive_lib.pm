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
require Exporter;

our (@ISA, @EXPORT, @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = (
      "NRCBanner",
      "NRCFooter"
      );
@EXPORT_OK = qw( NRCBanner NRCFooter );


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
                    td({width=>'60%', valign=>'center', align=>'center'},
                       img({src=>'/images/mainf1.gif', height=>44, width=>286,
                            alt=>'National Research Council Canada'})),
                    td({width=>'20%', valign=>'center', align=>'left'},
                       img({src=>'/images/mainWordmark.gif', height=>44, width=>93,
                            alt=>'Government of Canada'}))),
                 Tr(td(),
                    td({valign=>'top', align=>'center'},
                       small(
                          "Traitement multilingue de textes / Multilingual Text Processing", br(),
                          "Technologies de l'information et des communications / Information and Communications Technologies", br(),
                          "Conseil national de recherches Canada / National Research Council Canada", br(),
                          "Copyright 2004&ndash;2015, Sa Majest&eacute; la Reine du Chef du Canada / ",
                          "Her Majesty in Right of Canada", br(),
                          a({href=>"/portage_notices.html"}, "Third party Copyright notices")
                          ))));
}



1;
