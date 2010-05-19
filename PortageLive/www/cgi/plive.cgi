#!/usr/bin/perl -w
# @file plive.cgi
# @brief PORTAGE live CGI script
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

    plive.cgi

=head1 DESCRIPTION

This program and its companion F<plive-monitor.cgi> implemente a HTTP
interface to PORTAGE live.

Users can either submit translations through a text box, or through
file upload.  In the latter case, the translation job is performed in
the background, and the user is redirected to plive-monitor.cgi while
awaiting job completion.

=head1 SEE ALSO

plive-monitor.cgi.

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
use CGI qw(:standard);
use CGI::Carp qw/fatalsToBrowser/;
use XML::Twig;
use Time::gmtime;
use File::Temp qw(tempdir);
use File::Spec::Functions qw(splitdir catdir);

my $LANG = { iso2=>{ fr=>'fr', en=>'en' },
             iso3=>{ fr=>'fre', en=>'eng' },
             xml=>{ fr=>'FR_CA', en=>'EN_CA' },
             name=>{ fr=>'French', en=>'English' } };


## Site-specific configuration -- you may want to modify some of this stuff:


my $SRC_LANG = 'en';
my $TGT_LANG = 'fr';

my $PORTAGE_PATH = "/opt/Portage";
my $PORTAGE_BIN = "${PORTAGE_PATH}/bin";
my $PORTAGE_LIB = "${PORTAGE_PATH}/lib";
my $PORTAGE_MODEL_DIR = "${PORTAGE_PATH}/models/context"; # .$systems
my $CANOE_INI = "${PORTAGE_MODEL_DIR}/canoe.ini.cow";
my $CE_MODEL = ""; # "${PORTAGE_MODEL_DIR}/models/ce/ce_model";
my $TC_LM = "${PORTAGE_MODEL_DIR}/models/tc/tc-lm.${TGT_LANG}.tplm";
my $TC_MAP = "${PORTAGE_MODEL_DIR}/models/tc/tc-map.${TGT_LANG}.tppt";

# Make sure HTTP server has rw-access to this directory:
my $WORK_PATH = "/var/www/html/plive"; 

# Because text box translations are performed "on the spot", there is
# a risk for the page to time-out.  Hence this practical limit on
# job size:

my $MAX_TEXTBOX = 500;

## Site-specific configuration stops here -- normally...


$ENV{PORTAGE} = $PORTAGE_PATH;
$ENV{PATH} = join(":", $PORTAGE_BIN, $ENV{PATH});
$ENV{PERL5LIB} = (exists $ENV{PERL5LIB} 
                  ? join(":", $PORTAGE_LIB, $ENV{PERL5LIB}) 
                  : $PORTAGE_LIB);
$ENV{LD_LIBRARY_PATH} = (exists $ENV{LD_LIBRARY_PATH} 
                         ? join(":", $PORTAGE_LIB, $ENV{LD_LIBRARY_PATH}) 
                         : $PORTAGE_LIB);
push @INC, $PORTAGE_LIB;

$|=1;

## Main:

if (param('TranslateBox') or param('TranslateFile')) {
    processText();
} else {
    printForm();
}

exit 0;


## ------------------------------------------------------------------------
## Subroutines

## printForm()
#
# Produce the initial HTML form

sub printForm {
    print header(-type=>'text/html',
                 -charset=>'utf-8');

    print start_html("PORTAGELive");

    print NRCBanner();

    print h1("PORTAGELive"), 

    p();

    my @actions = qw(preview translate);

# Start a multipart form.
    
    my @filter_values = ();
    for (my $v = 0; $v <= 1; $v += 0.05) { push @filter_values, $v; }
    
    print start_multipart_form(),
    table({align=>'center', width=>400},
          Tr(td({colspan=>2, align=>'left',border=>0}, 
                p("Either type in some text, or select a plain-text file to translate. When you press the <em>Translate</em> button, the text will be translated by PORTAGELive."),
                br())),
          ## Text-box (textarea) interface:

          Tr(td({colspan=>2, align=>'left'}, 
                strong("Type some ".$LANG->{name}{$SRC_LANG}." text:"))),
          Tr(td({colspan=>2, align=>'left'}, 
                textarea(-name=>'textbox',
                         -value=>'',
                         -columns=>60, -rows=>10))),
          Tr(td({colspan=>2, align=>'center'},
                submit(-name=>'TranslateBox'),
                defaults('Clear Form'))),


          ## File-upload interface:

          Tr(td({colspan=>2, align=>'center'}, 
                h3("-- OR --"))),
          Tr(td({colspan=>2, align=>'left'}, 
                strong("Select a file:"),
                filefield(-name=>'filename',
                          -value=>'',
                          -default=>'',
                          -size=>60))),
          Tr({valign=>'top'},
             td(checkbox(-name=>'tmx',
                         -checked=>0,
                         -label=>'')),
             td(strong("TMX"), 
                "-- Check this box if input file is TMX.")),
          $CE_MODEL 
          ? Tr({valign=>'top'},
               td(scrolling_list(-name=>'filter',
                                 -default=>'no filtering',
                                 -values=>[ 'no filtering', map(sprintf("%0.2f", $_), @filter_values) ],
                                 -size=>1)),
               td(strong("Filter"),
                  "-- Set the filtering threshold on confidence."))
          : "",
          Tr({valign=>'top'},
             td(checkbox(-name=>'noss',
                         -checked=>0,
                         -label=>'')),
             td(strong("pre-segmented"),
                "-- Check this box if sentences are already newline-separated in the source text file.")),
          Tr({valign=>'top'},
             td(checkbox(-name=>'notok',
                         -checked=>0,
                         -label=>'')),
             td(strong("pre-tokenized"),
                "-- Check this box if word-tokens are already space-separated in the source text file.")),
          Tr({valign=>'top'},
             td({colspan=>2, align=>'center'},
                submit(-name=>'TranslateFile'),
                defaults('Clear Form'))));

    endform();

    print copyright();
}

# processText()
#
# Does the translation

sub processText {
    my $src_file;               # Local copy of the source-text file
    my $work_name;              # User-recognizable jobname

    # Get the source text:
    if (param('filename')) {    # File upload
        $src_file=tmpFileName(param('filename')) || problem("Can't get tmpFileName()");
        my @src_file_parts = split(/[:\\\/]+/, param('filename'));
        $work_name = normalizeName($src_file_parts[-1]);

    } elsif (param('textbox')) { # Text box
        problem("Input text too large (limit = ${MAX_TEXTBOX}).  Try file upload instead.") 
            if length(param('textbox')) > $MAX_TEXTBOX;
        my $fh;
        ($fh, $src_file)=File::Temp::tempfile();
        print {$fh} param('textbox'), "\n";
        close $fh;

        $work_name = "Text-Box";
    } else {
        problem("No text or file to translate");
    }

    # Get some basic info on source text:
    my $line_count = param('tmx') 
        ? checkTMX($src_file) 
        : checkFile($src_file, param('notok'), param('noss'));
    
    # Prepare the ground for translate.pl:
    my $work_dir = makeWorkDir($work_name) 
        || problem("Can't make directory for $work_name");
    link $src_file, "$work_dir/Q.in" 
        || problem("Can't link to input file $src_file");
    my $outfilename = "PLive-${work_name}";
    my @tr_opt = ("-verbose",
                  "-ini=$CANOE_INI",
                  "-src=${SRC_LANG}",
                  "-tgt=${TGT_LANG}",
                  "-tctp",
                  "-tclm=${TC_LM}",
                  "-tcmap=${TC_MAP}",
                  "-dir=\"${work_dir}\"",
                  "-out=\"${work_dir}/P.ce-out\"");
    push @tr_opt, $CE_MODEL
        ? ("-with-ce", "-model=$CE_MODEL")
        : ("-decode-only");
    push @tr_opt, param('notok') ? "-notok": "-tok";
    push @tr_opt, param('noss') ? "-nl=s" : "-nl=p";
    push @tr_opt, ("-tmx", 
                   "-xsrc=".$LANG->{xml}{$SRC_LANG}, 
                   "-xtgt=".$LANG->{xml}{$TGT_LANG})
        if param('tmx');
    push @tr_opt, "-filter=".(param('filter')+0) 
        unless (param('filter') eq 'no filtering') 
        or (not defined param('filter'));
    my $tr_opt = join(" ", @tr_opt);
    my $tr_cmd = "${PORTAGE_PATH}/bin/translate.pl ${tr_opt} \"$work_dir/Q.in\" >& \"$work_dir/trace\"";

    my $tr_output = catdir($work_dir, param('tmx') ? "QP.tmx" : "P.txt");
    my $user_output = catdir($work_dir, $outfilename);
    param('trace', "$work_dir/trace");
    
    # Launch translation
    if (param('filename')) {
        monitor($work_name, $work_dir, $outfilename, $line_count);
    
        # Launch translate.pl in the background for uploaded files
        system("(if (${tr_cmd}); then ln -s ${tr_output} ${user_output}; fi)&") == 0 
            or problem("Call returned with error code: ${tr_cmd} (error = %d)", $?>>8);
    } else {
        # Perform job here for text box
        system(${tr_cmd}) == 0
            or problem("Call returned with error code: ${tr_cmd} (error = %d)", $?>>8);
        open(my $P, "<${tr_output}") or problem("Can't open output file ${tr_output}");
        my @p = readline($P);
        close $P;
        textBoxOutput(param('textbox'), @p);
    }

}

# monitor(work_name, work_dir, outfilename, line_count)
# 
# Redirect the user to a job monitoring page (see plive-monitor.cgi)
# - work_name:  user-recognizable name for the job (typically the uploaded source text file name)
# - work_dir: translate.pl's work directory
# - outfilename: target translation file name (within the work dir)
# - line_count: total number of text segments (sentences) to translate

sub monitor {
    my ($work_name, $work_dir, $outfilename, $line_count) = @_;
# Output redirection to plive-monitor
    print header(-type=>'text/html',
                 -charset=>'utf-8');

    my @path = splitdir($work_dir);
    while (@path and $path[0] ne 'plive') { shift @path; }
    my $time = time();
    my $redirect="/cgi-bin/plive-monitor.cgi?time=${time}&file=${outfilename}&dir=".join("/",@path);
    print start_html(-title=>"PORTAGELive",
                     -head=>meta({-http_equiv => 'refresh',
                                  -content => "0;url=${redirect}"}));

    print 
        NRCBanner(),
        h1("PORTAGELive"),
        p("Processing $work_name: translating $line_count segments"),
        "\n";
    print copyright();

    close STDIN;
    close STDOUT;

}

# textBoxOutput($source, @target)
# 
# Produce an HTML page with source text and target translation 
# - $source: source-language text, as a single string of text
# - @target: target language translation, as a list of text segments

sub textBoxOutput {
    my ($source, @target) = @_;

    print header(-type=>'text/html',
                 -charset=>'utf-8');

    print start_html(-title=>"PORTAGELive");

    print 
        NRCBanner(),
        h1("PORTAGELive"),
        h2($LANG->{name}{$SRC_LANG}." source text:"),
        p($source),
        h2($LANG->{name}{$TGT_LANG}." target text:"),
        p(join("<br>", @target)),
        "\n";
    print copyright();

    close STDIN;
    close STDOUT;
}

# makeWorkDir(filename)
#
# Make a work directory to translate file filename
sub makeWorkDir {
    my ($filename) = @_;
    
    my $template = join("_","CE", $filename, timeStamp(),"XXXXXX");
    return tempdir($template,
                   DIR=>$WORK_PATH,
                   CLEANUP=>0);
}

# checkFile(src_file, notok, noss)
#
# Check validity of plain text file (either uploaded or extracted from text-box)
# - src_file
# - notok: no need to tokenize file, it's already tokenized
# - noss: no need to segment file into sentences, it's already done (newlines are segment boundaries)

sub checkFile {
    my ($src_file, $notok, $noss) = @_;

    # Check the MIME type and char set
    my $file_type = `file --brief --mime \"$src_file\"`;
    my ($mimetype, undef) = split(/[\s;]+/, $file_type, 2);
    problem("Please submit a plain text file")
        unless ($mimetype eq 'text/plain');
    my $charset = ($file_type =~ /charset=([^\s;]+)/) ? $1 : "unknown";
    problem("Please use either UTF-8 or ASCII character encoding")
        unless ($charset eq 'utf-8') or ($charset eq 'us-ascii');

    # Count words and sentences
    my $tokopt = "-lang=${SRC_LANG}";
    $tokopt .= $noss ? " -noss" : " -ss";
    $tokopt .= " -notok" if $notok;
    my $cmd = "${PORTAGE_BIN}/utokenize.pl $tokopt \"$src_file\" | wc --lines";
    my $segments = int(`$cmd`);
    
    problem("No text to translate in file")
        unless ($segments);

    return $segments;
}

# checkTMX(src_file)
#
# Check the validity of the source file as TMX file, return the number
# of translateble segments. This is based on ce_tmx.pl.

sub checkTMX {
    my ($infile) = @_;

    my $cmd = "ce_tmx.pl check \"${infile}\"";
    my ($fh, $fname) = File::Temp->tempfile("checkTMX-XXXXXX", UNLINK=>1);
    close $fh;

    system("$cmd > $fname")==0
        or problem("TMX check failed: $?");
    open($fh, "<$fname") 
        or problem("Can't open temp file $fname for reading");
    my $seg_count = <$fh>;
    close $fh;

    chomp $seg_count;
    return $seg_count;
}


# normalizeName(name)
#
# Remove anything that not alphanumeric, dash, underscore, dot or plus
sub normalizeName {
    my ($name) = @_;

    $name =~ s/[^-_\.+a-zA-Z0-9]//g;
    
    return $name;
}


# problem
# 
# Produce an HTML page describing a problem and exit
sub problem {
    my ($message, @args) = @_;

    print header(-type=>'text/html',
                 -charset=>'utf-8');
    print start_html(-title=>"PORTAGELive Problem");

    print 
        NRCBanner(),
        h1("PORTAGELive PROBLEM"),
        p(sprintf($message, @args)),
        "\n";

    if (param('trace') and -r param('trace')) {
        my $trace = param('trace');
        print(hr(),h1("Trace File"),
              pre(`cat $trace`));
    }

    print copyright();

    close STDIN;
    close STDOUT;

    exit 0;
}

# timeStamp()
#
# Return a ISO timestamp for "now"
sub timeStamp() {
    my $time = gmtime();
    
    return sprintf("%04d%02d%02dT%02d%02d%02dZ",
                   $time->year + 1900, $time->mon+1, $time->mday,
                   $time->hour, $time->min, $time->sec);
}


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
