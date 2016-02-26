#!/usr/bin/perl -w
# @file plive.cgi
# @brief PortageLive CGI script
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

    plive.cgi

=head1 DESCRIPTION

This program and its companion F<plive-monitor.cgi> implement an HTTP
interface to PortageLive.

Users can either submit translations through a text box, or through
file upload.  In the latter case, the translation job is performed in
the background, and the user is redirected to F<plive-monitor.cgi> while
awaiting job completion.

=head1 Translate from the command line.

./plive.cgi context=toy-regress-en2fr source_text="This is a test." translate_text="Translate Text"

=head1 CONFIGURATION

For this script to work for your application, you will likely have to
modify some internal variables. The most important of these are near
the top of the script.  Look for a comment saying "USER
CONFIGURATION".  Variables you are likely to want to change are
<PORTAGE_PATH> and <WEB_PATH>.

=head1 SEE ALSO

plive-monitor.cgi.

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

# NOTE
# HOW TO test this script from the command line:
# http://curl.haxx.se/docs/httpscripting.html
# curl --form "is_xml=1" --form "filename=@/root/PORTAGEshared/src/xliff/test_numbers_hyphens_2.docx.sdlxliff" --form "translate_file=Translate File" --form "context=reversed" 'http://localhost/cgi-bin/plive.cgi'

use strict;
use warnings;

use HTML::Entities;
use plive_lib;

## --------------------- USER CONFIGURATION ------------------------------
##

## Where PortageII files reside -- standard is /opt/PortageII
## NOTE: if you change this, also change it in plive-monitor.cgi
my $PORTAGE_PATH = "/opt/PortageII";

## Where PortageII executables reside -- standard is ${PORTAGE_PATH}/bin
## NOTE: if you change this, also change it in plive-monitor.cgi
my $PORTAGE_BIN = "${PORTAGE_PATH}/bin";

## Where PortageII code libraries reside -- standard is ${PORTAGE_PATH}/lib
## NOTE: if you change this, also change it in plive-monitor.cgi
my $PORTAGE_LIB = "${PORTAGE_PATH}/lib";

## Where the PortageII contexts (trained models) reside
## -- standard is ${PORTAGE_PATH}/models
my $PORTAGE_MODEL_DIR = "${PORTAGE_PATH}/models";

## The list of available contexts (trained models):
## -- if you leave an empty list, this script will find out by itself
##    what's available in $PORTAGE_MODEL_DIR
my @PORTAGE_CONTEXTS = ();

## PortageLive work directory
## NOTE: Make sure HTTP server has rw-access to this directory
## NOTE: if you change this, also change it in plive-monitor.cgi
my $WEB_PATH = "/var/www/html";

# plive's work directory location.
my $WORK_PATH = "${WEB_PATH}/plive";

# Because text box translations are performed "on the spot", there is
# a risk for the page to time-out.  Hence this practical limit on
# job size:
my $MAX_TEXTBOX = 3000;

## ---------------------- END USER CONFIGURATION ---------------------------
##
## below this line, you're on your own...


$ENV{PORTAGE} = $PORTAGE_PATH;
$ENV{PATH} = `source $PORTAGE_PATH/SETUP.bash; echo -n \$PATH`;
$ENV{PERL5LIB} = `source $PORTAGE_PATH/SETUP.bash; echo -n \$PERL5LIB`;
$ENV{LD_LIBRARY_PATH} = `source $PORTAGE_PATH/SETUP.bash; echo -n \$LD_LIBRARY_PATH`;
$ENV{PYTHONPATH} = `source $PORTAGE_PATH/SETUP.bash; echo -n \$PYTHONPATH`;
# disable cluster usage.
$ENV{PORTAGE_NOCLUSTER} = 1;
push @INC, $PORTAGE_LIB;

# We used to hard-code the environment variables here, but now we use
# SETUP.bash, as above.  Trade-off: using SETUP.bash means several system calls
# (via ` `), which is not as efficient.  Might be worth benchmarking some day...
#$ENV{PATH} = join(":", $PORTAGE_BIN, $ENV{PATH});
#$ENV{PERL5LIB} = (exists $ENV{PERL5LIB}
#                  ? join(":", $PORTAGE_LIB, $ENV{PERL5LIB})
#                  : $PORTAGE_LIB);
#$ENV{LD_LIBRARY_PATH} = (exists $ENV{LD_LIBRARY_PATH}
#                         ? join(":", $PORTAGE_LIB, $ENV{LD_LIBRARY_PATH})
#                         : $PORTAGE_LIB);

use CGI qw(:standard);
use CGI::Carp qw/fatalsToBrowser/;
use XML::Twig;
use Time::gmtime;
use File::Temp qw(tempdir);
use File::Spec::Functions qw(splitdir catdir);

$|=1;

## Main:

my %CONTEXT = getContexts(@PORTAGE_CONTEXTS);

if (param('translate_text') or param('translate_file')) {
    processText();
}
else {
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

    print h1({align=>'center'}, "PORTAGELive"),

    p();

    my @actions = qw(preview translate);

    my %labels = (
          's' => 'one sentence per line -- Check this box if input text has one sentence per line.',
          'p' => 'one paragraph per line -- Check this box if input text has one paragraph per line.',
          'w' => 'blank lines mark paragraphs -- Check this box if input text has two consecutive newlines mark the end of a paragraph, otherwise newline is just whitespace.'
          );

# Start a multipart form.

    my @filter_values = ();
    for (my $v = 0; $v <= 1; $v += 0.05) { push @filter_values, $v; }
    my %context_labels = ();
    for my $c (sort keys %CONTEXT) { $context_labels{$c} = $CONTEXT{$c}->{label}; }

    my $context = param('context');
    print start_multipart_form(),
    table({align=>'center', width=>600},
          Tr(td(strong("Select a system:")),
             td(popup_menu(-name=>'context',
                           -values=>["", sort keys %CONTEXT],
                           -default => $context ? [$context, $context_labels{$context}] : ["", '-- Please pick one --'],
                           -labels=>{ ""=>'-- Please pick one --',
                                      %context_labels }))),
          Tr(td({colspan=>2, align=>'left', border=>0},
                p("Either type in some text, or select a text file to translate (plain text or TMX or SDLXLIFF).<BR /> Press the <em>Translate Text</em> or the <em>Translate File</em> button to have PORTAGELive <br /> translate your text or file."),
                br())),


          ## Text-box (textarea) interface:

          Tr(td({colspan=>2, align=>'left'},
                strong("Type in source-language text:"))),
          Tr(td({colspan=>2, align=>'left'},
                textarea(-name=>'source_text',
                         -value=>'',
                         -columns=>66,
                         -rows=>10))),
          Tr(td({align=>'center'},
                checkbox(-name=>'text_xtags',
                         -checked=>0,
                         -label=>'')),
             td(strong("xtags"),
                "-- Check this box if input text contains tags and you want to process & transfer them.")),
          Tr(td({align=>'center'},
                checkbox(-name=>'enable_phrase_table_debug',
                         -checked=>0,
                         -label=>'')),
             td(strong("phrase table"),
                "-- Check this box if you want to genereate a phrase table per source sentence for debugging purposes.")),
          Tr(td({colspan=>2, align=>'center'},
                submit(-name=>'translate_text', -value=>'Translate Text'))),


          ## File-upload interface:

          Tr(td({colspan=>2, align=>'center'},
                h3("-- OR --"))),
          Tr(td({align=>'right'},
                strong("Select a file:")),
             td(filefield(-name=>'filename',
                          -value=>'',
                          -default=>'',
                          -size=>60))),
          Tr({valign=>'top'},
             td({align=>'right'},
                checkbox(-name=>'is_xml',
                         -checked=>0,
                         -label=>'')),
             td(strong("TMX/SDLXLIFF"),
                "-- Check this box if input file is TMX or SDLXLIFF.")),
          Tr({valign=>'top'},
             td({align=>'right'},
                checkbox(-name=>'file_xtags',
                         -checked=>0,
                         -label=>'')),
             td(strong("xtags"),
                "-- Check this box if input file contains tags and you want to process & transfer them.")),
          Tr({valign=>'top'},
             td({align=>'right'},
                checkbox(-name=>'useConfidenceEstimation',
                         -checked=>0,
                         -label=>'')),
             td(strong("Confidence Estimation"),
                "-- Check this box if you want to use Confidence Estimation when a system provides Confidence Estimation.")),
          Tr({valign=>'top'},
             td({align=>'right'},
                scrolling_list(-name=>'filter',
                               -default=>'no filtering',
                               -values=>[ 'no filtering', map(sprintf("%0.2f", $_), @filter_values) ],
                               -size=>1)),
             td(strong("Filter"),
                "-- Set the filtering threshold on confidence (TMX/SDLXLIFF files only).")),
          Tr({valign=>'top'},
             td({colspan=>2, align=>'center'},
                submit(-name=>'translate_file', -value=>'Translate File'))),
          Tr(td({colspan=>2, align=>'center'},


          ## Advanced options.
          #
          hr())),

          Tr(td({colspan=>2, align=>'left'},
                strong("Advanced Options (plain text input):"))),
          Tr({valign=>'top'},
             td({align=>'left', colspan=>2},
                radio_group(-name=>'newline',
                   -values=>['s', 'p', 'w'],
                   -default=>'p',
                   -linebreak=>'true',
                   -labels=>\%labels,
                   ))),
          Tr({valign=>'top'},
             td({align=>'right'},
                checkbox(-name=>'notok',
                         -checked=>0,
                         -label=>'')),
             td(strong("pre-tokenized"),
                "-- Check this box if word-tokens are already space-separated in the source text file.")),
          Tr({valign=>'top'},
             td({colspan=>2, align=>'center'},
                defaults('Clear Form'))));

    endform();

    print copyright();
}

# processText()
#
# Does the translation

sub processText {
    my $work_name;              # User-recognizable jobname
    my $work_dir;               # For translate.pl
    my @tr_opt = ();            # Translate.pl options

    my $context = param('context');
    if (not $context) {
        problem("No system (\"context\") specified.");
    }

    if (not -d "$PORTAGE_MODEL_DIR/$context") {
        problem("Invalid context (\"$context\") specified.");
    }

    # Create the work dir, get the source text in there:
    if (param('translate_file') and param('filename')) {  # File upload
        my $src_file = tmpFileName(param('filename'))
            || problem("Can't get tmpFileName()");
        my @src_file_parts = split(/[:\\\/]+/, param('filename'));
        $work_name = normalizeName(join("_", $context, $src_file_parts[-1]));
        $work_dir = makeWorkDir($work_name)
            || problem("Can't make work directory for $work_name");
        system("cp",  $src_file, "$work_dir/Q.in") == 0
            || problem("Can't copy input file $src_file into $work_dir/Q.in");

        # Do some basic checks on source text:
        if (param('is_xml')) {
            checkXML("$work_dir/Q.in");
        }
        else {
            checkFile("$work_dir/Q.in", param('notok'), param('noss'));
        }
    }
    elsif (param('translate_text') and param('source_text')) {  # Text box
        problem("Input text too large (limit = ${MAX_TEXTBOX}).  Try file upload instead.")
            if length(param('source_text')) > $MAX_TEXTBOX;
        $work_name = join("_", $context, "Text-Box");
        $work_dir = makeWorkDir($work_name)
            || problem("Can't make work directory for $work_name");
        open(my $fh, "> $work_dir/Q.in")
            || problem("Can't create input file Q.in in work directory $work_dir");
        print {$fh} param('source_text'), "\n";
        close $fh;

        push @tr_opt, "-xtags" if (param('text_xtags'));
    }
    else {
        problem("No text or file to translate");
    }

    $ENV{PATH} = "$PORTAGE_MODEL_DIR/$context/bin:$ENV{PATH}";
    $ENV{PERL5LIB} = "$PORTAGE_MODEL_DIR/$context/lib:$ENV{PERL5LIB}";
    $ENV{LD_LIBRARY_PATH} = "$PORTAGE_MODEL_DIR/$context/lib:$ENV{LD_LIBRARY_PATH}";

    # Prepare the ground for translate.pl:
    my $outfilename = "PLive-${work_name}";
    push @tr_opt, ("-verbose",
                  "-out=\"$work_dir/P.out\"",
                  "-dir=\"$work_dir\"");
    my $doCE = defined(param('useConfidenceEstimation')) && $CONTEXT{$context}->{ce_model};
    push @tr_opt, ($doCE ? "-with-ce" : "-decode-only");
    my $filter_threshold = (!defined param('filter') ? 0
                            : param('filter') eq 'no filtering' ? 0
                            : param('filter') + 0);
    if ($doCE and $filter_threshold > 0) {
        problem("Confidence-based filtering is only currently compatible with TMX or SDLXLIFF input.")
            unless param('is_xml');
        problem("Confidence-based filtering not available with system %s", $context)
            unless $CONTEXT{$context}->{ce_model};

        push @tr_opt, "-filter=$filter_threshold";
    }

    if (param('translate_file') and param('is_xml')) {
        push @tr_opt, ("-xml", "-nl=s");
        push @tr_opt, "-xtags" if (param('file_xtags'));
    }
    else {
        push @tr_opt, param('notok') ? "-notok": "-tok";
        my $newline = param('newline') || "p";
        push @tr_opt, "-nl=" . $newline;
    }

    unless (param('translate_file') and param('filename')) {
        if (param('enable_phrase_table_debug')) {
           push @tr_opt, "-xtra-decode-opts='-triangularArrayFilename $work_dir/P.triangArray.txt'";
        }
    }

    my $tr_opt = join(" ", @tr_opt);
    my $tr_cmd = $CONTEXT{$context}->{script} . " ${tr_opt} \"$work_dir/Q.in\" >& \"$work_dir/trace\"";

    my $tr_output = catdir($work_dir, param('is_xml') ? "QP.xml" : "P.txt");
    my $user_output = catdir($work_dir, $outfilename);
    param('trace', "$work_dir/trace");

    # Launch translation
    if (param('translate_file') and param('filename')) {
        monitor($work_name, $work_dir, $outfilename, $context);

        # Launch translate.pl in the background for uploaded files, in order to return to
	# webserver as soon as possible, and avoid potential timeout.

        close STDIN;
        close STDOUT;
        system("/bin/bash", "-c", "(if (${tr_cmd}); then ln -s ${tr_output} ${user_output}; fi; touch $work_dir/done)&") == 0
            or die("Call returned with error code: ${tr_cmd} (error = %d)", $?>>8);
    }
    else {
        # Perform job here for text box
        system(${tr_cmd}) == 0
            or problem("Call returned with error code: ${tr_cmd} (error = %d)", $?>>8);
        open(my $P, "<${tr_output}") or problem("Can't open output file ${tr_output}");
        my @p = readline($P);
        close $P;
        translationTextOutput(param('source_text'), $work_dir, @p);
    }
}

# monitor(work_name, work_dir, outfilename, context)
#
# Redirect the user to a job monitoring page (see plive-monitor.cgi)
# - work_name:  user-recognizable name for the job (typically the uploaded source text file name)
# - work_dir: translate.pl's work directory
# - outfilename: target translation file name (within the work dir)
# - context: Name of the context (model) within which job is submitted

sub monitor {
    my ($work_name, $work_dir, $outfilename, $context) = @_;
# Output redirection to plive-monitor

    my @path = splitdir($work_dir);
    while (@path and $path[0] ne 'plive') { shift @path; }
    my $time = time();
    my $redirect="plive-monitor.cgi?time=${time}&file=${outfilename}&context=${context}&dir=".join("/",@path);
    $redirect .= $CONTEXT{$context}->{ce_model} ? "&ce=1" : "&ce=0";

    print redirect(-uri=>$redirect);
}

# translationTextOutput($source, @target)
#
# Produce an HTML page with source text and target translation
# - $source: source-language text, as a single string of text
# - @target: target language translation, as a list of text segments

sub translationTextOutput {
    my ($source, $workDir, @target) = @_;

    # strip out webroot since the traceFile argument to plive-monitor will have the webroot prepended later on.
    $workDir =~ s#^$WEB_PATH##;

    print header(-type=>'text/html',
                 -charset=>'utf-8');

    print start_html(-title=>"PORTAGELive");

    $source = HTML::Entities::encode_entities($source, '<>&');
    $source =~ s/\n/<br \/>/g;
    print
        NRCBanner(),
        h1("PORTAGELive"),
        div({-id=>'source'},
           h2("Source text:"),
           p($source)),
        "\n",
        div({-id=>'translation'},
           h2("Translation:"),
           p(join("<br />", map { HTML::Entities::encode_entities($_, '<>&') } @target)),
        "\n");
    print p(a({-id=>'translateMore', -href=>"plive.cgi?context=".param('context')}, "Translate more text"));

    my @debuggingTools = (
       a({-id=>'oov', -href=>"$workDir/oov.html"}, "Out-of-vocabulary words"),
       a({-id=>'pal', -href=>"$workDir/pal.html"}, "Phrase alignments"),
       a({-id=>'trace', -href=>"plive-monitor.cgi?traceFile=$workDir/trace"}, "Trace file"));
    push(@debuggingTools, a({-id=>'triangArray', -href=>"$workDir/P.triangArray.txt"}, "Phrase tables")) if (-r "$WEB_PATH/$workDir/P.triangArray.txt");

    print div({-id=>'debuggingToolsDiv', -style=>'font-size: 0.8em;'},
       h3("Debugging Tools"),
       ul({-id=>"debuggingToolsList"}, li({-type=>'circle'}, \@debuggingTools)));

    #my @params = param();
    #print "<PRE> @params </PRE>";
    #foreach my $param (@params) {
    #    print "<PRE> $param = ", param($param), "</PRE>";
    #}
    print copyright();
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

    # Check the MIME type and char set - only accept utf-8 plain text.

    # We prefix the file with a bunch of spaces to avoid collisions with mime
    # types whose magic number is plain text strings - e.g., a file starting
    # with "nut free" is identified as an Apple QuickTime movie by file.
    # Binary files are identified as "application/octet-stream" with these
    # extra spaces, so although their correct mime type is lost, the result
    # still works for our purposes here.
    my $file_type = `{ echo '                     '; cat \"$src_file\"; } | clean-utf8-text.pl | file --brief --mime -`;
    my ($mimetype, undef) = split(/[\s;]+/, $file_type, 2);
    problem("Please submit a plain text file (your file seems to be $mimetype)")
        unless ($mimetype =~ /text\/.*/);
    my $charset = ($file_type =~ /charset=([^\s;]+)/) ? $1 : "unknown";
    problem("Please use either UTF-8 or ASCII character encoding (your file seems to be $mimetype)")
        unless ($charset eq 'utf-8') or ($charset eq 'us-ascii');
}

# checkTMX(src_file)
#
# Check the validity of the source file as TMX file, return the number
# of translateble segments. This is based on ce_tmx.pl.

sub checkXML {
    my ($infile) = @_;

    my $cmd = "ce_tmx.pl check \"${infile}\"";

    system("$cmd > /dev/null") == 0
        or problem("TMX check failed: $?");
}


# getContexts()
#
# Gather information about available "contexts" (trained PortageII systems)
# -- if argument list is empty, find out what's available in $PORTAGE_MODEL_DIR
sub getContexts {
    my (@list) = @_;

    if (!@list) {
        opendir(my $dh, $PORTAGE_MODEL_DIR)
            or problem("Can't open directory $PORTAGE_MODEL_DIR");
        @list = readdir($dh);
        closedir $dh;
    }

    my %context = ();
    for my $model (@list) {
        if (my $info = getContextInfo($model)) {
            $context{$model} = $info;
        }
    }
    return %context;
}

# getContextInfo()
#
# Get basic info about model
sub getContextInfo {
    my ($name) = @_;

    my %info = ( name=>$name );

    my $D = "${PORTAGE_MODEL_DIR}/${name}";
    my $script = "$D/soap-translate.sh";
    $info{script} = $script;
    return undef # Can't find SOAP script $script
        unless -r $script;
    my $cmdline = `tail --lines=1 $script`;
    if ($cmdline =~ /(-decode-only|-with-ce|-with-rescoring)/) {
        return undef; # Invalid command line in soap-translate.sh; context probably from previous, incompatible version of Portagge
    }
    my $src = "";
    if ($cmdline =~ /-xsrc=([-a-zA-Z]+)/) { $src = $1 }
    elsif ($cmdline =~ /-src=(\w+)/) { $src = $1 }
    return undef unless $src; # Can't find source language in command line $cmdline
    my $tgt = "";
    if ($cmdline =~ /-xtgt=([-a-zA-Z]+)/) { $tgt = $1 }
    elsif ($cmdline =~ /-tgt=(\w+)/) { $tgt = $1 }
    return undef unless $tgt; # Can't find target language in command line $cmdline

    #$info{canoe_ini} = (-r "$D/canoe.ini.cow") ? "$D/canoe.ini.cow"
    #    : return undef; # Can't find canoe.ini file $D/canoe.ini.cow
    #$info{rescore_ini} = (-r "$D/rescore.ini") ? "$D/rescore.ini" : "";
    $info{ce_model} = (-r "$D/ce_model.cem") ? "$D/ce_model.cem" : "";

    $info{label} = "$info{name} ($src --> $tgt)" .
                   ($info{ce_model} ? " with CE" : "");
    return \%info;
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
    print start_html(-title=>"PORTAGELive Problem", -style => {-src => '/plive.css'});

    print
        NRCBanner(),
        "\n",
        div({-id=>'problemDescription'},
           h1("PORTAGELive PROBLEM"),
           p(sprintf($message, @args))),
        "\n";

    if (param('trace') and -r param('trace')) {
        my $traceFile = param('trace');
        print getTrace($traceFile);
    }

    print copyright();

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


sub copyright() {
    return (hr(),
            NRCFooter(),
            end_html());
}
