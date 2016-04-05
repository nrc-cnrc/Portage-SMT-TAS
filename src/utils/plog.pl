#!/usr/bin/perl -s -w

# @file plog.pl
# @brief Manage PortageII's accounting log
#
# @author Michel Simard, Eric Joanis
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

use strict;

=pod 

=head1 SYNOPSIS

=over 1

=item plog.pl {options} -create job_name

=item plog.pl {options} -update log_file status words_in [ words_out ] 

=item plog.pl {options} [ -extract|-stats ] [ period ]

=back

=head1 DESCRIPTION

This program is used to manage PortageII's accounting log.  Each log
entry corresponds to a specific translation "job", and contains the
following fields:

=over 1

=item NO        Job number

=item TIME      Date and time of the last logging operation, as an ISO 8601 timestamp YYYY-MM-DDTHH:MM:SSZ 

=item USER      User name (system) who created the log entry

=item JOB       Name of the job (user-given)

=item WORDS_IN  Submitted word count (non-negative integer number of source words submitted to translation)

=item WORDS_OUT Translated word count (non-negative integer number of source words actually translated, i.e. after confidence estimation filtering, if applicable)

=item STATUS    Current job status: one of "pending", "success" or "failure"

=item FILE      the name of the log file associated with the job

=item COMMENT   A user-specified comment

=back

Logging for a translation job is normally viewed as a two-step
process: first, create a log entry before the translation job begins;
second, update the entry to reflect the final status of the job
once it's finished.

The first form of this command (C<plog.pl -create>) is used to create
an entry for a new job: C<job_name> is the C<JOB> field (typically the
name of the source text file being translated). Values for the other
fields are allocated automatically: job numbers <NO> are allocated
sequentially, starting from 1; C<TIME> is the current time;
C<WORDS_IN> and C<WORDS_OUT> are zero; C<STATUS> is "pending"; C<FILE>
is the complete name of the file corresponding to this log entry;
C<COMMENT> is empty.  Upon successfully creating the entry, the
program outputs the C<FILE> field on the standard output.

The second form (C<plog.pl -update>) is used to update the C<STATUS>,
C<WORDS_IN> and C<WORDS_OUT> fields of the given entry. C<log_file> is
the file name returned by the C<plog.pl -create> command; C<status>
should either be "success" or "failure"; C<words_in> is the number of
source words submitted to translation; and C<words_out> is the actual
number of source-words translated (not filtered out by confidence
estimation) -- if not given, it is assumed to be the same as
C<words_in>.

Both the first and second forms accept an optional C<-comment> switch,
by which the caller may add a textual comment to the log entry.  Note
that a comment specified in the C<-update> form, even empty, will
overwrite a comment given with the C<-create> form.

The third form is used either to extract entries (C<-extract>) or to
compute global statistics on these entries (C<-stats>).  By default
(no C<period> argument), all log entries are considered. The C<period>
argument can be used to specify a time period: C<YYYY> for a complete
year (e.g. "2010"), C<YYYY/MM> for a month (e.g. "2005/02") or
C<YYYY/MM/DD> for a specific day (e.g. "1964/01/07").  With
C<-extract>, log entries are output to standard output in
tab-separated CSV format:

NO TIME USER JOB WORDS_IN WORDS_OUT STATUS FILE COMMENT

With C<-stats>, various statistics are computed and printed to
standard output.


=head1 OPTIONS

=over

=item -dir=D          Logging directory [$PORTAGE/logs/accounting]

=item -create         Create mode: create a new log entry

=item -update         Update mode: update the status of the given log file

=item -extract        Extract mode: extract entries for the given period

=item -comment=C      With C<-create> and C<-update>, add a textual comment to the entry

=item -header         With C<-extract>, output a CSV header

=item -raw            With C<-extract>, output raw (non-CSV) entries

=item -stats          Stats mode: display various statistics for the given period [default action]

=item -verbose        Be verbose

=item -h,-help        Print a help message and exit

=item -man,-H,-Help   Print the full manual

=back

=head1 AUTHOR

Michel Simard, Eric Joanis

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 Copyright 2010, Her Majesty in Right of Canada

=cut

BEGIN {
   # If this script is run from within src/ rather than being properly
   # installed, we need to add utils/ to the Perl library include path (@INC).
   if ( $0 !~ m#/bin/[^/]*$# ) {
      my $bin_path = $0;
      $bin_path =~ s#/[^/]*$##;
      unshift @INC, "$bin_path/../utils", $bin_path;
   }
}
use portage_utils;
printCopyright("plog.pl", 2010);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

my $PORTAGE = $ENV{PORTAGE};
my $DEFAULT_LOG = File::Spec->catdir($PORTAGE, "logs", "accounting");
my @LOG_FIELDS = qw(NO TIME USER JOB WORDS_IN WORDS_OUT STATUS FILE COMMENT);
my %LOG_DEFAULT = (NO=>0,
                   TIME=>undef,
                   USER=>undef,
                   JOB=>undef,
                   WORDS_IN=>0,
                   WORDS_OUT=>0,
                   STATUS=>"pending",
                   FILE=>"",
                   COMMENT=>"");
my $CSV_SEP = "\t";
my $LOG_LOCK = ".lock";

use Time::gmtime;
use File::Spec;
use File::Path qw(mkpath);
use File::Temp qw(tempfile);

##  Options
##
our($help, $h, $H, $Help, $HELP, $man, 
    $create, $update, $extract, $stats,
    $comment, 
    $dir, $header, $raw,
    $verbose);
our($debug, $wout);              # undocumented options

if ($help or $h) {
    system "podselect -section SYNOPSIS -section OPTIONS -section COPYRIGHT $0 | pod2text";
    exit 0;
}
if ($man or $HELP or $Help or $H) {
    -t STDOUT ? system "pod2usage -verbose 3 $0" : system "pod2text $0";
    exit 0;
}

die "Error: You can't specify more than one of -create, -update, -extract and -stats\n" 
    if defined($update) + defined($create) + defined($extract) + defined($stats) > 1;

$dir = $DEFAULT_LOG unless defined $dir;
$comment = undef unless defined $comment; # Yes, this is totally useless, except as documentation
die "Error: Option -comment only appicable with -create or -update" 
    if defined($comment) and not ($create or $update);

$header = 0 unless defined $header;
die "Error: Option -header only appicable with -extract" 
    if $header and not $extract;
$raw = 0 unless defined $raw;
die "Error: Option -raw only appicable with -extract" 
    if $raw and not $extract;
die "Error: Options -raw and -header can't be used together" 
    if $raw and $header;
$wout = 0 unless defined $wout;

$verbose = 0 unless defined $verbose;
$debug = 0 unless defined $debug;

die "Error: Can't access logging directory $dir\n" unless -d $dir and -r $dir;

## Command-line arguments are specific to actions (extract, create, update)
## and so are fetched later on

if ($create) {
    defined(my $job_name = shift) or die "Error: Missing argument: job_name\n";
    die "Error: Too many arguments\n" if shift;

    print STDOUT logCreate($dir, $job_name, $comment), "\n";

} elsif ($update) {
    defined(my $log_file = shift) or die "Error: Missing argument: log_file\n";
    defined(my $job_status = shift) or die "Error: Missing argument: status\n";
    defined(my $words_in = shift) or die "Error: Missing argument: words_in\n";
    my $words_out = shift;      # Don't care if it's undef -- handled by logUpdate()

    die "Error: Too many arguments\n" if shift;

    logUpdate($dir, $log_file, $job_status, $words_in, $words_out, $comment);

} elsif ($extract) {  
    my $period = shift || "";
    die "Error: Too many arguments\n" if shift;

    logExtract($dir, $period, $header, $raw);
} else {                        # -stats
    my $period = shift || "";
    die "Error: Too many arguments\n" if shift;

    logStats($dir, $period);
}

exit 0;

sub logCreate {
    my ($dir, $job_name, $comment) = @_;
    $comment = "" unless defined $comment;

    verbose("[Creating log entry for \"${job_name}\" in $dir]\n");

    my $now = gmtime();         # Greenwich time
    my @pw = getpwuid($<);
    lockLog($dir) 
        or die "*** Error: All attempts to acquire exclusive access to $dir failed ",
               "-- please contact your administrator.\n";
    my $job = logNew(NO=>getNO($dir),
                     JOB=>$job_name,
                     USER=>$pw[0], # username
                     TIME=>timeStamp($now),
                     COMMENT=>$comment);


    my $todays_path = File::Spec->catdir(sprintf("%04d", $now->year()+1900), 
                                         sprintf("%02d", $now->mon()+1), 
                                         sprintf("%02d", $now->mday()));
    my $full_dirname = File::Spec->catdir($dir, $todays_path);

    umask 0000;                 # Ugly, but effective
    File::Path::mkpath($full_dirname, 0, 0777);    # like mkdir -p
    my ($log_fh, $full_filename) = File::Temp::tempfile(sprintf("%06d.%s.%s.XXXX",
                                                                logValue($job, 'NO'), 
                                                                logValue($job, 'TIME'), 
                                                                fileizeName(logValue($job, 'JOB'))), 
                                                        DIR=>$full_dirname, 
                                                        SUFFIX=>".log", 
                                                        UNLINK=>0);

    my (undef, undef, $just_the_filename) = File::Spec->splitpath($full_filename);

    logValue($job, 'FILE', File::Spec->catfile($todays_path, $just_the_filename));

    logWrite($job, $log_fh);
    close $log_fh;

    unlockLog($dir) or die "*** Error: Something went wrong while unlocking $dir: errno=$!\n";

    chmod 0644, $full_filename;

    verbose("[Done. Log file is \"%s\"]\n", logValue($job, 'FILE'));
    return logValue($job, 'FILE');
}


sub logUpdate {
    my ($dir, $log_file, $job_status, $words_in, $words_out, $comment) = @_;
    $words_in = 0 unless defined $words_in;
    $words_out = $words_in unless defined $words_out;

    verbose("[Updating log entry \"${log_file}\" status to $job_status]\n");

    die "Error: Status must be one of \"pending\", \"success\" or \"failure\" ; got \"$job_status\"\n"
        unless $job_status =~ /^(pending|success|failure)$/;

    die "Error: words_in must be a non-negative integer value; got \"$words_in\"\n"
        unless $words_in =~ /^[0-9]+$/;
    die "Error: words_out must be a non-negative integer value; got \"$words_out\"\n"
        unless $words_out =~ /^[0-9]+$/;
    die "Error: words_out must be less or equal than words_in\n"
        unless $words_out <= $words_in;

    my $full_filename = File::Spec->catfile($dir, $log_file);
    open(my $log_fh, "<$full_filename") 
        or die "Error: No read-access to log entry $full_filename\n";
    my $job = logRead($log_fh) 
        or die "Error: Can't read log entry from $full_filename\n";
    warn "**Warning: Extra material in log file $full_filename\n" 
        if defined readline($log_fh);
    close $log_fh;

    logValue($job, 'TIME', timeStamp(gmtime())); # Greenwich time;
    logValue($job, 'WORDS_IN', $words_in);
    logValue($job, 'WORDS_OUT', $words_out);
    logValue($job, 'STATUS', $job_status);
    logValue($job, 'COMMENT', $comment) 
        if defined $comment;    # Overwrite only if defined!

    open($log_fh, ">$full_filename") or die "Error: No write-access to log file $full_filename\n";
    logWrite($job, $log_fh) or die "Error: Can't write to log file $full_filename\n";
    close $log_fh;

    verbose("[Done.]\n");

    return $log_file;
}

sub logExtract {
    my ($dir, $period, $header, $raw) = @_;
    my $logWriter = $raw ? \&logWrite : \&logCSVWrite;

    verbose($period 
            ? "[Extracting log entries for period $period]\n"
            : "[Extracting all log entries]\n");

    die "Error: Invalid format for period \"$period\"\n"
        unless $period =~ m{^(\d\d\d\d(/\d\d(/\d\d)?)?)?$};

    my $full_dir = File::Spec->catdir($dir, $period);

    if (-d $full_dir) {
        logCSVHeader(*STDOUT) if $header;
        # We could just do "find ... -exec cat"; instead, we read and validate each entry:
        my $cmd = "find $full_dir -name '*.log' | sort -n |";
        open(my $log_files, $cmd) or die "Error: Can't do \"$cmd\"";

      LOG_FILES: while (my $log_file = <$log_files>) {
            open(my $log_fh, "<$log_file")
                or ( warn "**Warning: Can't open logfile $log_file"
                     and next LOG_FILES );
            my $log_entry = logRead($log_fh)
                or ( warn "**Warning: Can't read log entry from $log_file"
                     and next LOG_FILES );
            warn "**Warning: Extra material in log file $log_file\n" 
                if defined readline($log_fh);
            close $log_fh;
            &$logWriter($log_entry, *STDOUT);
        }
        close $log_files;

    } else {
        warn "**Warning: No log entries for period \"$period\"\n";
    }
    verbose("[Done.]\n");
}


sub logStats {
    my ($dir, $period) = @_;

    my $plog_call = "$0 -extract -raw -dir=$dir -verbose=$verbose $period";
    open(my $plog_fh, "$plog_call |") or die "Error: Call failed: $plog_call\n";

    my $words_in = 0;
    my $words_out = 0;
    my $success_count = 0;
    my $failure_count = 0;
    my $pending_count = 0;
    my $first_time = undef;
    my $last_time = undef;
    my @job_no = ();

    while (my $job = logRead($plog_fh)) {
        push @job_no, logValue($job, 'NO');
        $first_time = logValue($job, 'TIME') 
            if (not defined $first_time) 
            or (logValue($job, 'TIME') lt $first_time); 
        $last_time = logValue($job, 'TIME') 
            if (not defined $last_time) 
            or (logValue($job, 'TIME') gt $last_time); 
        if (logValue($job, 'STATUS') eq 'success') {
            ++$success_count;
            $words_in += logValue($job, 'WORDS_IN');
            $words_out += logValue($job, 'WORDS_OUT');
        } elsif (logValue($job, 'STATUS') eq 'failure') {
            ++$failure_count;
        } elsif (logValue($job, 'STATUS') eq 'pending') {
            ++$pending_count;
        } else {
            warn "**Warning: Unexpected job status: ".logValue($job, 'STATUS')."\n";
        }
    }

    @job_no = sort {$a <=> $b} @job_no;
    for (my $i = 1; $i <= $#job_no; ++$i) {
        for (my $j = $job_no[$i-1] + 1; $j < $job_no[$i]; ++$j) {
            warn "**Warning: Missing job log no $j **\n";
        }
    }

    printf("Jobs:\n");
    printf("  First:      no. %d  (%s)\n", $job_no[0], $first_time) if @job_no;
    printf("  Last:       no. %d  (%s)\n", $job_no[-1], $last_time) if @job_no;
    printf("  Successful: %5d", $success_count);
    printf("  (%d total source words", $words_in);
    printf(", %d filtered out = %.2f%%", 
           $words_in - $words_out, 
           $words_in > 0 ? 100 * ($words_in - $words_out) / $words_in : 0)
        if $wout;
    print(")\n");
    printf("  Failed:     %5d\n", $failure_count);
    printf("  Pending:    %5d\n", $pending_count);
    printf("Total jobs:   %5d\n", int(@job_no));

    close $plog_fh;
}


## log pseudo-object

sub logNew {                    # constructor
    my %kv = @_;
    my $log = { %LOG_DEFAULT };
    for my $field (keys %kv) {
        debug("logNew: initializing field \"$field\" to value \"%s\"\n", $kv{$field});
        $log->{$field} = $kv{$field};
        # Turn off this error checking, because it gets called lockLog() and
        # unlockLog(), which means failing here is not permitted.
        #$log->{$field} = (exists($log->{$field}) # check validity of field names
        #                  ? $kv{$field}
        #                  : die "Error: logNew: unsupported log field \"$field\"");
    }
    return $log;
}

sub logValue {                  # get/set field values
    my ($log, $field, $value)=@_;
    # This is sometimes called between lockLog() and unlockLog(), not allowed to die.
    #die "Error: Invalid log field name $field" unless exists $log->{$field};
    $log->{$field} = $value if defined $value;
    return $log->{$field};
}

sub logWrite {                  # write to filehandle
    my ($log, $fh) = @_;

    my @fields = ();
    for my $f (@LOG_FIELDS) {
        my $v = logValue($log, $f);
        push @fields, makeCSVsafe($f."=".$v) if defined $v;
    }

    print {$fh} (join($CSV_SEP, @fields), "\n");
                 
}

sub logCSVWrite {                  # write to filehandle
    my ($log, $fh) = @_;

    my @fields = ();
    for my $f (@LOG_FIELDS) {
        my $v = logValue($log, $f);
        push @fields, makeCSVsafe(defined $v ? $v : "");
    }

    print {$fh} (join($CSV_SEP, @fields), "\n");
                 
}

sub logCSVHeader {                  # write header to filehandle
    my ($fh) = @_;

    print {$fh} join($CSV_SEP, @LOG_FIELDS),"\n";
}

sub logRead {
    my ($fh) = @_;
    my @LEGACY_FIELDS = qw(NO TIME USER JOB WORDS_IN STATUS FILE);

    defined (my $line = readline($fh)) 
        or return undef;

    chomp $line;
    my @fields = split(/$CSV_SEP/, $line);
    my %kv = ();

    # Legacy format:
    if ($fields[0] =~ /^\d+$/ and $#fields == $#LEGACY_FIELDS) {
        @kv{@LEGACY_FIELDS} = @fields;
        $kv{WORDS_OUT} = $kv{WORDS_IN};
    } 
    # Standard format:
    else { 
        for my $f (@fields) {
            if ($f !~ /^([^=]+)=(.*)$/) {
                die "Error: Invalid field ($f) in line ($line)";
                return undef;
            }
            $kv{$1} = $2;
        }
    }
    return logNew(%kv);
}



## Get next log entry number

sub getNO {
    my ($dir) = @_;

    my $no = 1;                # Default value
    
    die "Error: No such logging directory: $dir\n"  unless -d $dir;

    my $lastyear = dirGetLast($dir, qr/^\d\d\d\d$/);
    
    if ($lastyear) {
        my $logpath = File::Spec->catdir($dir, $lastyear);
        my $lastmonth = dirGetLast($logpath, qr/^\d\d$/);
        
        if ($lastmonth) {
            $logpath = File::Spec->catdir($logpath, $lastmonth);
            my $lastday = dirGetLast($logpath, qr/^\d\d$/);
        
            if ($lastday) {
                $logpath = File::Spec->catdir($logpath, $lastday);
                my $lastlog = dirGetLast($logpath, qr/^\d+\..*\.log$/);

                if ($lastlog) {
                    my ($lastno, undef) = split(/\./, $lastlog, 2);
                    $no = $lastno + 1;
                }
            }
        }
    }

    return $no;
}

sub dirGetLast {
    my ($dir, $pattern) = @_;
    $pattern = qr/.*/ unless defined $pattern; 

    opendir(my $D, $dir) or die "Error: Can't read $dir";
    my @subdirs = sort grep { /$pattern/ } readdir $D;
    closedir $D;

    return @subdirs ? pop @subdirs : undef;
}

## log lock mechanism (just for getting job no and creating log file)

sub lockLog {
    my ($dir, $attempts) = @_;

    $attempts = 100 unless defined $attempts;

    my $lockdir = File::Spec->catdir($dir, $LOG_LOCK);

    for my $a (1..$attempts) {
        if (mkdir $lockdir) {
            debug("Succeeded in acquiring lock $lockdir\n");
            return 1;
        }
        verbose("[Attempt no $a to acquire lock $lockdir failed.]\n");
        sleep 1;
    }
    return 0;
}

sub unlockLog {
    my ($dir) = @_;

    my $lockdir = File::Spec->catdir($dir, $LOG_LOCK);

    debug("Releasing lock $lockdir\n");
    return rmdir($lockdir);
    
}

## Various utilities

sub timeStamp {
    my ($tm) = @_;

    return sprintf("%04d-%02d-%02dT%02d:%02d:%02dZ", 
                   $tm->year()+1900, $tm->mon()+1, $tm->mday(),
                   $tm->hour(), $tm->min(), $tm->sec());
}

sub fileizeName {
    my ($name) = @_;

    #"File:/home/sam037/work/autobuild/plive/httpd/var/www/html/plive/CE_toy-regress-en2fr_2010-385-source-CCI_272OR_fr.short.txt_20160225T172037Z_IuBucc/Q.in; Context:/home/sam037/work/autobuild/plive/opt/PortageII/models/toy-regress-en2fr"
    # Keep the filename not too long.
    $name =~ s/^File:.*?(.{1,50}); Context:.*?(.{1,50})$/File:$1; Context:$2/;
    $name =~ s/[^-_\.0-9a-z]//gi;# remove all non-alphanum from job name

    return $name;
}

sub makeCSVsafe {
    my ($name) = @_;

    $name =~ s/[\n\r\f\b\t\"]//gi;# remove offending control chars, tabs and quotes

    return $name;
}

sub verbose { printf STDERR @_ if $verbose }
sub debug { printf STDERR @_ if $debug }

1;
