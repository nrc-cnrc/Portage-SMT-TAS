#!/usr/bin/perl -w

use strict 'refs';
use warnings;
use CGI qw(:standard);
use CGI::Carp qw/fatalsToBrowser/;
use XML::Twig;
use Time::gmtime;
use File::Temp qw(tempdir);
use File::Spec::Functions qw(splitdir catdir);

my $PORTAGE_PATH = "/opt/Portage";
my $PORTAGE_BIN = "${PORTAGE_PATH}/bin";
my $PORTAGE_LIB = "${PORTAGE_PATH}/lib";
my $LOG_PATH = "/var/www/html/plive";
my $SRC_LANG_NAME = "French";
my $SRC_LANG = "fr";
my $TGT_LANG_NAME = "English";
my $TGT_LANG = "en";

my $MAX_TEXTBOX = 500;

$ENV{PORTAGE} = $PORTAGE_PATH;
$ENV{PATH} = join(":", $PORTAGE_BIN, $ENV{PATH});
$ENV{PERL5LIB} = exists $ENV{PERL5LIB} ? join(":", $PORTAGE_LIB, $ENV{PERL5LIB}) : $PORTAGE_LIB;
$ENV{LD_LIBRARY_PATH} = (exists $ENV{LD_LIBRARY_PATH} 
                         ? join(":", $PORTAGE_LIB, $ENV{LD_LIBRARY_PATH}) 
                         : $PORTAGE_LIB);
push @INC, $PORTAGE_LIB;

$|=1;

our ($v, $verbose, $Verbose, $mode, $host, $key);

$verbose = 0 unless defined $verbose;
$Verbose = 0 unless defined $Verbose;

if (param('TranslateBox') or param('TranslateFile')) {
    processText();
} else {
    printForm();
}

exit 0;


sub printForm {
    print header(-type=>'text/html',
                 -charset=>'utf-8');

    print start_html("PORTAGELive");

    print h1("PORTAGELive"), 

    p();

    my @actions = qw(preview translate);

# Start a multipart form.
    
    print start_multipart_form(),
    table({align=>'center', width=>400},
          Tr(td({colspan=>2, align=>'left',border=>0}, 
                p("Either type in some text, or select a plain-text file to translate. When you press the <em>Translate</em> button, the text will be translated by PORTAGELive."),
                br())),
          Tr(td({colspan=>2, align=>'left'}, 
                strong("Type some ${SRC_LANG_NAME} text:"))),
          Tr(td({colspan=>2, align=>'left'}, 
                textarea(-name=>'textbox',
                         -value=>'',
                         -columns=>60, -rows=>10))),
          Tr(td({colspan=>2, align=>'center'},
                submit(-name=>'TranslateBox'),
#                submit(-name=>'Preview'),
                defaults('Clear Form'))),
          Tr(td({colspan=>2, align=>'center'}, 
                h3("-- OR --"))),
          Tr(td({colspan=>2, align=>'left'}, 
                strong("Select a file:"),
                filefield(-name=>'filename',
                          -value=>'',
                          -default=>'',
                          -size=>60))),
          Tr(td({align=>'right'}, 
                "Check this box if sentences are already newline-separated in source text file:"),
             td(checkbox(-name=>'noss',
                         -checked=>0,
                         -label=>'Segmented'))),
          Tr(td({align=>'right'}, 
                "Check this box if word-tokens are already space-separated in source text file:"),
             td(checkbox(-name=>'notok',
                         -checked=>0,
                         -label=>'Tokenized'))),
          Tr(td({colspan=>2, align=>'center'},
                submit(-name=>'TranslateFile'),
#                submit(-name=>'Preview'),
                defaults('Clear Form'))));

    endform();

    print 
        hr(), 
        end_html();
}

sub processText {
    # Prepare the ground for ce_translate
    my $src_file;               # Local copy of the source-text file
    my $work_name;              # User-recognizable jobname

    if (param('filename')) {    # File upload
        $src_file=tmpFileName(param('filename')) || problem("Can get tmpFileName()");
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

    my %preview = checkFile($src_file, param('notok'), param('noss'));
    problem("Can't work with this character set: %s.  Please use UTF-8 or plain ASCII.", $preview{charset}) 
        unless $preview{ok};

    my $line_count = $preview{"lines"} || problem("Nothing to translate in file");

    my $work_dir = makeDir($work_name) || problem("Can't make directory for $work_name");

    link $src_file, "$work_dir/Q.in" || problem("Can't link to input file $src_file");

    my $model_dir = "${PORTAGE_PATH}/models"; # .$systems
    my $canoe_ini = "${model_dir}/canoe.ini.cow";
    my $ce_model = "${model_dir}/ce/ce_model";
    my $tclm = "${model_dir}/tc/tc-lm.${TGT_LANG}.tplm";
    my $tcmap = "${model_dir}/tc/tc-map.${TGT_LANG}.tppt";

    my $outfilename = "PLive-${work_name}";

    my @ce_opt = ("-verbose",
                  "-src=${SRC_LANG}",
                  "-tgt=${TGT_LANG}",
                  "-tclm=${tclm}",
                  "-tcmap=${tcmap}",
                  "-tctp",
                  "-dir=\"${work_dir}\"",
                  "-out=\"${work_dir}/P.ce-out\"");
    push @ce_opt, "-notok" if param('notok');
    push @ce_opt, param('noss') ? "-nl=s" : "-nl=p";
    my $ce_opt = join(" ", @ce_opt);
    my $ce_cmd = "${PORTAGE_PATH}/bin/ce_translate.pl ${ce_opt} ${canoe_ini} ${ce_model} \"$work_dir/Q.in\" >& \"$work_dir/trace\"";

    my $P_txt = catdir($work_dir, "P.txt");
    my $output = catdir($work_dir, $outfilename);

    if (param('filename')) {
        monitor($work_name, $work_dir, $outfilename, $line_count);
    
        # Launch ce_translate in the background
        system("(if (${ce_cmd}); then ln -s ${P_txt} ${output}; fi)&") == 0 
            or die "Call returned with error code: ${ce_cmd} (error = ", $?>>8, ")";
    } else {
        system(${ce_cmd}) == 0
            or problem("Call returned with error code: ${ce_cmd} (error = %d)", $?>>8);
        open(my $P, "<${P_txt}") or problem("Can't open output file ${P_txt}");
        my @p = readline($P);
        textBoxOutput(param('textbox'), @p);
        close $P;
    }

}

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
        h1("PORTAGELive"),
        p("Processing $work_name: translating $line_count segments"),
        "\n";
    print hr();
    print end_html();

    close STDIN;
    # close STDERR;
    close STDOUT;

}

sub textBoxOutput {
    my ($source, @target) = @_;

    print header(-type=>'text/html',
                 -charset=>'utf-8');

    print start_html(-title=>"PORTAGELive");

    print 
        h1("PORTAGELive"),
        h2("${SRC_LANG_NAME} source text:"),
        p($source),
        h2("${TGT_LANG_NAME} target text:"),
        p(join("<br>", @target)),
        "\n";
    print hr();
    print end_html();

    close STDIN;
    # close STDERR;
    close STDOUT;
}

sub problem {
    my ($message, @args) = @_;

    print header(-type=>'text/html',
                 -charset=>'utf-8');
    print start_html(-title=>"PORTAGELive Problem");

    print 
        h1("PORTAGELive PROBLEM"),
        p(sprintf($message, @args)),
        "\n";
    print hr();
    print end_html();

    close STDIN;
    close STDOUT;

    exit 0;
}

sub makeDir {
    my ($filename) = @_;
    
    my $template = join("_","CE", $filename, timeStamp(),"XXXXXX");
    return tempdir($template,
                   DIR=>$LOG_PATH,
                   CLEANUP=>0);
}

sub checkFile {
    my ($src_file, $notok, $noss) = @_;
    my %I = (ok=>0);

    # Check the MIME type and char set
    my $file_type = `file --brief --mime \"$src_file\"`;
    ($I{mimetype}, undef) = split(/[\s;]+/, $file_type, 2);
    return %I unless ($I{mimetype} eq 'text/plain');

    $I{charset} = ($file_type =~ /charset=([^\s;]+)/) ? $1 : "unknown";
    return %I unless ($I{charset} eq 'utf-8') or ($I{charset} eq 'us-ascii');

    # Count words and sentences
    my $tokopt = "-lang=${SRC_LANG}";
    $tokopt .= $noss ? " -noss" : " -ss";
    $tokopt .= " -notok" if $notok;
    my $cmd = "${PORTAGE_BIN}/utokenize.pl $tokopt \"$src_file\" | wc";
    my $wc = `$cmd`;
    $wc =~ s/^ *//;
    $wc =~ s/ *$//;
    ($I{lines}, $I{words}, $I{chars}) = map($_+0, split(/ +/, $wc, 3));
    
    $I{ok} = $I{words};

    return %I;
}


sub normalizeName {
    my ($name) = @_;

    $name =~ s/[^-_\.+a-zA-Z0-9]//g;
    
    return $name;
}

sub timeStamp() {
    my $time = gmtime();
    
    return sprintf("%04d%02d%02dT%02d%02d%02dZ",
                   $time->year + 1900, $time->mon+1, $time->mday,
                   $time->hour, $time->min, $time->sec);
}


sub verbose { printf STDERR (@_) if ($verbose or $Verbose) ; }
sub veryVerbose { printf STDERR (@_) if $Verbose; }

