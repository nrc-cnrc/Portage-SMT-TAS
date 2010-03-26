#!/usr/bin/perl

use vars qw($VERSION @ISA @EXPORT);
use Exporter;

# @file parser.en.pl 
# @brief The following is a portion of a perl program that
# detects dates and numbers in an English #text corpus, converts them to a
# standard format and marks them up. 
#

#Author:	Fatiha Sadat, LTRC-NRC
#Usage:		perl detectmarkup-eng2fr.date-number.NIST05.pl $infile $outfile 
#Date:		March 20, 2005
# just a dumb comment
# ----------------------------------------------------------------------------

BEGIN {
   # If this script is run from within src/ rather than being properly
   # installed, we need to add utils/ to the Perl library include path (@INC).
   if ( $0 !~ m#/bin/[^/]*$# ) {
      my $bin_path = $0;
      $bin_path =~ s#/[^/]*$##;
      unshift @INC, "$bin_path/../utils";
   }
}
use portage_utils;
printCopyright("parser.en.pl", 2005);
$ENV{PORTAGE_INTERNAL_CALL} = 1;

sub usage {
   local $, = "\n";
   print STDERR @_, "";
   $0 =~ s#.*/##;
   print STDERR "
Usage: $0 [options] [IN [OUT]]

  This program detects dates and numbers in an English #text corpus, converts
  them to a standard format and marks them up. 

Options:

  -h(elp)       print this help message
";
   exit 1;
}

use Getopt::Long;
# Note to programmer: Getopt::Long automatically accepts unambiguous
# abbreviations for all options.
my $verbose = 1;
GetOptions(
   help        => sub { usage },
) or usage;

$direng = "/export/projets/portage/models/parsing-dictionaries/English/";
#$dirf= "/export/projets/portage/corpora/NIST-05/processed-v1/";
#$dirmark = "/export/projets/portage/corpora/NIST-05/processed-v2/";
#$e = shift(@ARGV);
#$file = $dirf . $e;

$file = shift || "-";
open (FILE, $file) || die "$file : couldn't open file \n";

#$result1= $dirmark ."dates_" . $e;   
#$result2= $dirmark ."numbers_" .$e;   

#open (RESULT1, ">" . $result1) || die "$result1 : couldn't create file \n";
#open (RESULT2, ">" . $result2) || die "$result2 : couldn't create file \n";

#$result= $dirmark . $e . "_marked";   
$result = shift || "-";
open (RESULTMARKED, ">" . $result) || die "$result : couldn't create file \n";


# -- START the parsing script for English ... --

my %month = (
	january		=> 1,
	february	=> 2,
	march 		=> 3,
	april 		=> 4,
	may 		=> 5,
	june		=> 6,
	july		=> 7,
	august 		=> 8,
	september 	=> 9,
	october 	=> 10,
	november 	=> 11,
	december 	=> 12,
	);

my %monthabrev =( 
	jan	=> january,
	feb	=> february,
	mar	=> march,
	mars	=> march,
	apr	=> april,
	jun	=> june,
	jul	=> july,
	aug	=> august,
	sep	=> september,
	sept	=> september,
	"oct"	=> october,
	nov	=> november,
	dec	=> december,
	"jan."	=> january,
	"feb."	=> february,
	"mar."	=> march,
	"apr."	=> april,
	"jun."	=> june,
	"jul."	=> july,
	"aug."	=> august,
	"sep."	=> september,
	"sept."	=> september,
	"oct."	=> october,
	"nov."	=> november,
	"dec."	=> december,
	);

my %day = (
	sunday		=> 0,
	monday		=> 1,
	tuesday		=> 2,
	wednesday	=> 3,
	thursday	=> 4,
	friday		=> 5,
	saturday	=> 6,
	);
		
my %dayabrev =( 
	sun	=> sunday,
	mon	=> monday,
	tue	=> tuesday,
	tues	=> tuesday,
	wed	=> wednesday,
	wednes	=> wednesday,
	thu	=> thursday,
	thur	=> thursday,
	thurs	=> thursday,
	fri	=> friday,
	sat	=> saturday,
	);

my %specday  = (
	    afternoon	=> 1,
	    dawn	=> 2,
	    dusk	=> 3,
	    evening	=> 4,
	    evenings	=> 5,
	    midafternon	=> 6,  
	    yesterday	=> 7,
	    midday	=> 8,
	    midmorning	=> 9,
	    tomorrow	=> 10,
	    midnight	=> 11,
	    morning	=> 12,
	    mornings	=> 13,
	    night	=> 14,
	    nightfall	=> 15,
	    nights	=> 16,
	    noon	=> 17,
	    springtime	=> 18,
	    summertime	=> 19,
	    sundown	=> 20,
	    sunrise	=> 21,
	    sunset	=> 22,
	    wintertime	=> 23,
	    y2k		=> 24,
	    anniversary	=> 25,
	    today	=> 26,
	    afternoons	=> 27,
	    "9/11"	=> 28,
	    "mid-afternoon"	=> 29,
	    "mid-morning"	=> 30,
	    tonight		=> 31,
	    ); 
	    #christmas	=> 31,easter	=> 32, halloween	=> 33,

my $specdaypat = join("|", keys %specday);

my $monthpat = join("|", keys %month);
my $daypat = join("|", keys %day);

my $dayabrevpat1 = join("|", keys %dayabrev);
my $dayabrevpat2 = join("\.|", keys %dayabrev);
my $dayabrevpat = $dayabrevpat2 ."|". $dayabrevpat1;

my $monthabrevpat = join("|", keys %monthabrev);
#my $monthabrevpat2 = join("\.|", keys %monthabrev);
#my $monthabrevpat = $monthabrevpat2 ."|". $monthabrevpat1;

my %datesuf = (st => 1,
	      nd => 2,
	      rd => 3,
	      th => 4,
	      ); 
    
my $sufpat = join("|", keys %datesuf);

my @prefixdate = qw(previous coming last next current this that);
my $prefixdatepat = join ("|", @prefixdate); 

my @eradesignator = qw(ad bc AD BC A.D. B.C);
my @hourcues = qw(oclock o'clock am pm);
my @datequantifiers = qw(many few a some last next);


# Loading the dictionary of English Date ordinals ry : first, second, ..., thirty-ninth
$dicodateord = $direng . "DictionaryEnglishDateOrdinals.txt";

open(ORD, $dicodateord) || die "$dicodateord : couldn't open file \n";
while(<ORD>) {
chop;
@a=split(/[\s\t\n]+/,$_);
$ordinal{$a[1]}++; 
}
close ORD;    

# Loading the Dictionary of English Dates quantities  : day, week, month, year, "seasons", minutes, seconds, ...
$dicodatequantity = $direng . "DictionaryEnglishDateQuantities.txt";

open(DQUANT, $dicodatequantity) || die "$dicodatequantity : couldn't open file \n";
while(<DQUANT>) {
chop;
@a=split(/[\s\t\n]+/,$_);
$dateform{$a[1]}++; 
}
close DQUANT;    


# Loading the Dictionaries for Alphabetical numbers and their sufixes (million, hundred, ..)                                
$diconumberalphab = $direng . "DictionaryEnglishNumbersAlphabet.txt";
open(NUMALPH, $diconumberalphab) || die "$diconumberalphab : couldn't open file \n";
while(<NUMALPH>) {
chop;
@a=split(/[\s\t\n]+/,$_);
$numalph{$a[1]}++;
}
close NUMALPH;


my %numsuf = (
	million => 0,
	hundred => 1,
	billion => 2,
	thousand => 3,	
	);

my $sufnumpat = join("|", keys %numsuf);

my $digitmonth = "0?[0-9]|1[0-2]";
my $digithour  = "[0-1]?[0-9]|2 [0-3]";
my $digitday   = "[0-2]?[0-9]|3[0-1]";  
my $digitminut = "[0-4]?[0-9]|5[0-9]";
my $am ="(\s*am)|(\s*a\.m\.)|oclock|o'clock"; 
my $pm ="(\s*pm)|(\s*p\.m\.)";
my $AMPM = "(\s*am)|(\s*a\.m\.)|(\s*pm)|(\s*p\.m\.)|oclock|o'clock";  

my @quantifydates = qw(month months years year day days week weeks decade decades century centuries millenium  milleniums hour  hours  hr hrs  minute minutes  second  seconds msec millisecond  milliseconds summer winter spring  autumn fal);
my $quantities= join ("|", @quantifydates); 

# DATA FOR detecting NUMBERS

my @percent = qw(percent pct % per-cent);
my $percentpat0= join ("|", @percent) ;      
my $percentpat1= "per cent";      
my $percentpat2= "per  cent";      
my $percentpat= $percentpat0."|".$percentpat1."|".$percentpat2;
#my $percentpat2= 'percent|pct|%|per cent';

#load dictionaries for currencies and measurement units 

$dicocurrencies = $direng . "DictionaryEnglishMoneyCurrencies.txt";
open(CURRENCY, $dicocurrencies) || die "$dicocurrencies   : couldn't open file \n";
while(<CURRENCY>) {
chop;
@a=split(/[\s\t\n]+/,$_);
if (($a[1] != /(\s+|\t|\n|\r)/)&& ($a[1] ne "")){
$a[1]=~s /\./ /g;$currency{$a[1]}++;}
}
close CURRENCY;

#---
$dicocurrenciesacronym = $direng . "DictionaryEnglishMoneyCurrenciesAcronyms.txt";
open(CURRENCYACRONYM, $dicocurrenciesacronym) || die "$dicocurrenciesacronym   : couldn't open file \n";
while(<CURRENCYACRONYM>) {
chop;
@a=split(/[\s\t\n]+/,$_);
if (($a[1] != /(\s+|\t|\n|\r)/)&& ($a[1] ne "")){$currency{$a[1]}++;}
}
close CURRENCYACRONYM;

#foreach $key(keys %currency){ print "\n$key\t$currency{$key}\n";}
my $currency = join ("|", keys %currency);
#print "$currency\n";

#---

$dicomeasure = $direng . "DictionaryEnglishMeasurementUnits.txt";
open(MEASURE, $dicomeasure) || die "$dicomeasure   : couldn't open file \n";
while(<MEASURE>) {
chop;
@a=split(/[\s\t\n]+/,$_);
if (($a[1] != /(\s+|\t|\n|\r)/)&& ($a[1] ne "")){$measuretab{$a[1]}++;}
}
close MEASURE;

my $measure = join ("|", keys %measuretab);
$patent ="$percentpat|$measure|$currency|\$"; 
$patent =~ s/\|\|\|\|/\|/g;

#print "\n$patent\n";

my %numalphabet1 = (	"one "	=> 1,
			"two "	=> 2,
			"three "	=> 3,
			"four "	=> 4,
			"five "	=> 5,
			"six "	=> 6,
			"seven "	=> 7,
			"eight "	=> 8,
			"nine "	=> 9,
			"ten "	=> 10,
			"ones "	=> 1,
			"twos "	=> 2,
			"threes "	=> 3,
			"fours "	=> 4,
			"fives "	=> 5,
			"sevens "	=> 7,
			"eights "	=> 8,
			"nines "	=> 9,
			one	=> 1,
			two	=> 2,
			three	=> 3,
			four	=> 4,
			five	=> 5,
			six	=> 6,
			seven	=> 7,
			eight	=> 8,
			nine	=> 9,
			ten	=> 10,
			twos	=> 2,
			threes	=> 3,
			fours	=> 4,
			fives	=> 5,
			sevens	=> 7,
			eights	=> 8,
			nines	=> 9,
			);
		#tens	=> 10,
		#	first	=> 1,
		#	second 	=> 2,
		#	third 	=> 3,
		#	fourth 	=> 4,
		#	fifth 	=> 5,
		#	sixth 	=> 6,
		#	seventh	=> 7,
		#	eighth 	=> 8,
		#	nineth 	=> 9,
		#	tenth 	=> 10,

my %numalphabet2  = (	eleven	=> 11, 
			twelve	=> 12,
			thirteen	=> 13, 	
			fourteen	=> 14,
			fifteen	=> 15,
			sixteen	=> 16,
			seventeen	=> 17,
			eighteen	=> 18,
			nineteen	=> 19,
			elevens	=> 11,
			twelves	=> 12,
			thirteens	=> 13,
			fourteens	=> 14,
			fifteens	=> 15,
			sixteens	=> 16,
			seventeens	=> 17,
			eighteens	=> 18,
			nineteens	=> 19,
			twenty	=> 20,
			thirty	=> 30,
			forty	=> 40,
			fifty	=> 50,
			sixty	=> 60,
			seventy	=> 70,
			eighty	=> 80,
			ninety	=> 90,
			twenties	=> 20,
			thirties	=> 30,
			forties	=> 40,
			fifties	=> 50,
			sixties	=> 60,
			seventies	=> 70,
			eighties	=> 80,
			nineties	=> 90,
			);
		#	eleventh  	=> 11,
		#	twelfth  	=> 12,
		#	thirteenth 	=> 13,
		#	fourteenth  	=> 14,
		#	fifteenth  	=> 15,
		#	sixteenth     	=> 16,
		#	seventeenth     => 17,
		#	eighteenth     	=> 18,
		#	nineteenth     	=> 19,
		#	twentieth 	=> 20,
		#	thirtieth  	=> 30,
		#	fourtieth  	=> 40,
		#	fiftieth	=> 50,
		#	sixtieth	=> 60,
		#	seventieth  	=> 70,
		#	eightieth  	=> 80,
		#	ninetieth  	=> 90,
		#	twentieth  	=> 20,
		#	thirtieth  	=> 30,
		#	fourtieth  	=> 40,
		#	fiftieth  	=> 50,
		#	sixtieth  	=> 60,
		#	seventieth  	=> 70,
		#	eightieth  	=> 80,
		#	ninetieth  	=> 90,
		
my %numalphabet3  = (	"hundred "	=> 100,
			"thousand "	=> 1000,
			"million "	=> 1000000,
			"billion "	=> 1000000000,
			"trillion "	=> 1000000000000,
			"quadrillion "	=> 1000000000000000,
			"quintillion "	=> 1000000000000000000,
			"hundreds "	=> 100,
			"thousands "	=> 1000,
			"millions "	=> 1000000,
			"billions "	=> 1000000000,
			"trillions "	=> 1000000000000,
			"quadrillions "	=> 1000000000000000,
			"quintillions "	=> 1000000000000000000,
			hundred	=> 100,
			thousand	=> 1000,
			million	=> 1000000,
			billion	=> 1000000000,
			trillion	=> 1000000000000,
			quadrillion	=> 1000000000000000,
			quintillion	=> 1000000000000000000,
			hundreds	=> 100,
			thousands	=> 1000,
			millions	=> 1000000,
			billions	=> 1000000000,
			trillions	=> 1000000000000,
			quadrillions	=> 1000000000000000,
			quintillions	=> 1000000000000000000,
			); 

my $numalphabetpat1= join ("|", keys %numalphabet1); 
my $numalphabetpat2= join ("|", keys %numalphabet2); 
my $numalphabetpat3= join ("|", keys %numalphabet3); 

$numpat = "$numalphabetpat1|$numalphabetpat2|$numalphabetpat3";
my @numavoidnew = qw(one two three four five six seven eight nine ten ones twos threes fours fives sevens eights nines tens);
my $numavoidpat= join ("|", @numavoidnew); 

#--------------------------
# start processing 
#--------------------------

#while(defined($line = lc(<FILE>)))   # my style of reading directly into $line from file

while(<FILE>) {

$line =lc($_); 
#chomp; 
   
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";
$count="a"; $correct ="";

#----------- The 1th Form -----------

while($line =~ m/($daypat|$dayabrevpat)?\s*(,)?\s*(($monthpat|$monthabrevpat)\s+($digitday)\s*($sufpat)?\s*(,)?\s+(\d\d+))/go){
#detecting Monday, June 12(th)?, 2004 of June 14th or June 31st, 98 or June 13 or June 13 1998 or Mon. June 12th 2004
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

	$mask = "<FAT_FORM>$count";
 	$memPatterns{$mask} = $3;       
 	$STNmask="<FAT_STN>$count";
 	$count++;                             
	$raw=$3; 
	$daystr = $5; 
	$instmonth = $4;
	$instwday =$1 if $1; 
	$yearstr=$8 if $8;
	
	if ($6) {$k="Month Day Suffix";}  else {$k="Month Day";}
	$k=$k." ," if $7;
	$k=$k." Year" if $8; $k=~ s/,$//g;
 

	$wdaystr ="NULL";
	
	if ($instmonth =~ /($monthpat)/) {
		$monthstr = $month{$instmonth}; 
	}elsif ($instmonth =~ /($monthabrevpat)/)  { 
		$monthstr = $month{$monthabrev{$instmonth}}; 
	}
	
	if ($raw =~/((.*)\s*(,)\s*)$/){	$raw =$2; } 
	if ($raw =~/^(\s*(,)\s*(.*))/){ $raw= $3; } 
			
	#print RESULT1 "$raw";
	#print RESULT1 "\t$wdaystr $yearstr:$monthstr:$daystr\n";
	
	$memSTN{$STNmask} = "$wdaystr $yearstr:$monthstr:$daystr";
	$line =~ s/($monthpat|$monthabrevpat)\s+($digitday)\s*($sufpat)?\s*(,)?\s+(\d\d+)/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
	#$line =~ s/($monthpat|$monthabrevpat)\s+($digitday)\s*($sufpat)?\s*(,)?\s+(\d\d+)?/<DATE pattern=\"$k\" standard\=\"$STNmask\">$mask<\/DATE>/;

} # End while

while($line =~ m/($daypat|$dayabrevpat)?\s*(,)?\s*(($monthpat|$monthabrevpat)\s+($digitday)\s*($sufpat))\s/go){
#detecting Monday, June 12(th)?, 2004 of June 14th or June 31st, 98 or June 13 or June 13 1998 or Mon. June 12th 2004
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

	$mask = "<FAT_FORM>$count";
 	$memPatterns{$mask} = $3;       
 	$STNmask="<FAT_STN>$count";
 	$count++;                             
	$raw=$3; 
	$daystr = $5; 
	$instmonth = $4;
	$instwday =$1 if $1; 
		
	$k="Month Day Suffix";
	
	$wdaystr ="NULL";
	
	if ($instmonth =~ /($monthpat)/) {
		$monthstr = $month{$instmonth}; 
	}elsif ($instmonth =~ /($monthabrevpat)/)  { 
		$monthstr = $month{$monthabrev{$instmonth}}; 
	}
	
	if ($raw =~/((.*)\s*(,)\s*)$/){	$raw =$2; } 
	if ($raw =~/^(\s*(,)\s*(.*))/){ $raw= $3; } 
			
	#print RESULT1 "$raw";
	#print RESULT1 "\t$wdaystr $yearstr:$monthstr:$daystr\n";
	
	$memSTN{$STNmask} = "$wdaystr $yearstr:$monthstr:$daystr";
	$line =~ s/($monthpat|$monthabrevpat)\s+($digitday)\s*($sufpat)/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
	#$line =~ s/($monthpat|$monthabrevpat)\s+($digitday)\s*($sufpat)?\s*(,)?\s+(\d\d+)?/<DATE pattern=\"$k\" standard\=\"$STNmask\">$mask<\/DATE>/;

} # End while

#HERE...
while($line =~ m/($daypat|$dayabrevpat)?\s*(,)?\s*(($monthpat|$monthabrevpat)\s+($digitday))/go){
#detecting Monday, June 12(th)?, 2004 of June 14th or June 31st, 98 or June 13 or June 13 1998 or Mon. June 12th 2004
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

	$mask = "<FAT_FORM>$count";
 	$memPatterns{$mask} = $3;       
 	$STNmask="<FAT_STN>$count";
 	$count++;                             
	$raw=$3; 
	$daystr = $5; 
	$instmonth = $4;
	$instwday =$1 if $1; 
		
	$k="Month Day";
	
	$wdaystr ="NULL";
	
	if ($instmonth =~ /($monthpat)/) {
		$monthstr = $month{$instmonth}; 
	}elsif ($instmonth =~ /($monthabrevpat)/)  { 
		$monthstr = $month{$monthabrev{$instmonth}}; 
	}
	
	if ($raw =~/((.*)\s*(,)\s*)$/){	$raw =$2; } 
	if ($raw =~/^(\s*(,)\s*(.*))/){ $raw= $3; } 
			
	#print RESULT1 "$raw";
	#print RESULT1 "\t$wdaystr $yearstr:$monthstr:$daystr\n";
	
	$memSTN{$STNmask} = "$wdaystr $yearstr:$monthstr:$daystr";
	$line =~ s/($monthpat|$monthabrevpat)\s+($digitday)/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
	
} # End while

#----------- The 5th Form -----------
#detecting Monday, 12th June, 2004 OR Friday 23rd january 2004 OR september 2003 

while($line =~ m/($daypat|$dayabrevpat)?\s*(,)?\s*(($digitday)\s*($sufpat)?\s+($monthpat|$monthabrevpat)\s*(,)?\s+(\d\d+))/go){
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

	$mask = "<FAT_FORM>$count";
 	$memPatterns{$mask} = $3;       
        $STNmask="<FAT_STN>$count";
        $count++;        

	($instmonth, $daystr) = ($6, $4);	
	$yearstr = $8 if $8;
	$raw = $3; 
	$instwday =$1 if $1;

	if ($5) {$k="Day Suffix Month";}  else {$k="Day Month";}
		$k=$k." ," if $7;
		$k=$k." Year" if $8;  $k=~ s/,$//g;
		
	$wdaystr ="NULL"; 	

	if ($instmonth =~ /($monthpat)/) {
	$monthstr = $month{$instmonth}; 
	} elsif ($instmonth =~ /($monthabrevpat)/)  { 
	$monthstr = $month{$monthabrev{$instmonth}};
	}

	if ($raw =~/((.*)\s*(,)\s*)$/){ $raw =$2; } 
	if ($raw =~/^(\s*(,)\s*(.*))/){ $raw= $3; } 
			
	#print RESULT1 "$raw";
	#print RESULT1 "\t$wdaystr $yearstr:$monthstr:$daystr\n";
	$memSTN{$STNmask} = "$wdaystr $yearstr:$monthstr:$daystr";
	$line =~ s/($digitday)\s*($sufpat)?\s+($monthpat|$monthabrevpat)\s*(,)?\s+(\d\d+)/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
	

} #end while 

while($line =~ m/($daypat|$dayabrevpat)?\s*(,)?\s*(($digitday)\s*($sufpat)?\s+($monthpat|$monthabrevpat))/go){
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

	$mask = "<FAT_FORM>$count";
 	$memPatterns{$mask} = $3;       
        $STNmask="<FAT_STN>$count";
        $count++;        

	($instmonth, $daystr) = ($6, $4);	
	$raw = $3;  $instwday =$1 if $1;

	if ($5) {$k="Day Suffix Month";}  else {$k="Day Month";}
	$k=~ s/,$//g;
		
	$wdaystr ="NULL"; 	

	if ($instmonth =~ /($monthpat)/) {
	$monthstr = $month{$instmonth}; 
	} elsif ($instmonth =~ /($monthabrevpat)/)  { 
	$monthstr = $month{$monthabrev{$instmonth}};
	}

	if ($raw =~/((.*)\s*(,)\s*)$/){ $raw =$2; } 
	if ($raw =~/^(\s*(,)\s*(.*))/){ $raw= $3; } 
			
	#print RESULT1 "$raw";
	#print RESULT1 "\t$wdaystr $yearstr:$monthstr:$daystr\n";
	$memSTN{$STNmask} = "$wdaystr $yearstr:$monthstr:$daystr";
	$line =~ s/($digitday)\s*($sufpat)?\s+($monthpat|$monthabrevpat)/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
	

} #end while 

#----------- The 6th Form -----------

while($line =~ m/(($monthpat|$monthabrevpat)\s*(,)?\s*($digitday)\s*(,)?\s+(\d\d+))/go){

$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

 	$mask = "<FAT_FORM>$count";
 	$memPatterns{$mask} = $1;       
        $STNmask="<FAT_STN>$count";
        $count++;                                             
	$wdaystr = "NULL"; 
	$daystr = $4; 
	$instmonth = $2;
	$yearstr = $6;
	$raw =$1;

	if ($3) {$k="Month ,";}  else {$k="Month";}
 	if ($5) {$k=$k."Day , Year";} else {$k=$k."Day Year";} 
 	$k =~ s/,$//g;
	 	
	if ($instmonth =~ /($monthpat)/) {
			$monthstr = $month{$instmonth}; 
		}elsif ($instmonth =~ /($monthabrevpat)/)  { 
			$monthstr = $month{$monthabrev{$instmonth}}; 
	}
	

	if ($raw =~/((.*)\s*(,)\s*)$/){ $raw =$2; } 
	if ($raw =~/^(\s*(,)\s*(.*))/){ $raw= $3; } 
		 		
	#print RESULT1 "$raw";
	print  RESULT1 "\t$wdaystr $yearstr:$monthstr:$daystr\n";
	$memSTN{$STNmask} = "$wdaystr $yearstr:$monthstr:$daystr";
#$line =~ s/($monthpat|$monthabrevpat)\s*(,)?\s*�($digitday)\s* (,)?\s* (\d\d+)/<DATE>$mask<\/DATE>/;
	$line =~ s/($monthpat|$monthabrevpat)\s*(,)?\s*($digitday)\s*(,)?\s+(\d\d+)/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
}

#----------- The 61th Form -----------

#detecting 20 march, 2003 
while($line =~ m/(($digitday)\s*(,)?\s*($monthpat|$monthabrevpat)\s*(,)?\s+(\d\d+))/go){

$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";
 	$mask = "<FAT_FORM>$count";
 	$memPatterns{$mask} = $1;       
        $STNmask="<FAT_STN>$count";
        $count++;                                             
	$raw= $1;
	
	$wdaystr = "NULL"; 
	$daystr = $2; 
	$instmonth = $4;
	$yearstr = $6;

	if ($3) {$k="Day , Month";}  else {$k="Day Month";}
 	if ($5) {$k=$k." , Year";} else {$k=$k." Year";} 
 	$k =~ s/,$//g;
	
	if ($instmonth =~ /($monthpat)/) {
			$monthstr = $month{$instmonth}; 
		}elsif ($instmonth =~ /($monthabrevpat)/)  { 
			$monthstr = $month{$monthabrev{$instmonth}}; 
	}
	
	
	if ($raw =~/((.*)\s*(,)\s*)$/){ $raw =$2; } 
	if ($raw =~/^(\s*(,)\s*(.*))/){ $raw= $3; } 
				
	#print RESULT1 "$raw";
	print  RESULT1 "\t$wdaystr $yearstr:$monthstr:$daystr\n";
	$memSTN{$STNmask} = "$wdaystr $yearstr:$monthstr:$daystr";
 
	$line =~ s/($digitday)\s*(,)?\s*($monthpat|$monthabrevpat)\s*(,)?\s+(\d\d+)/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;

}


#----------- The 7th Form -----------
#detecting "september 2003" , "june , 1997"

while($line =~ m/\s+(($monthpat|$monthabrevpat)\s*(,)?\s+(\d\d+))/go) {

$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";
	
	$mask = "<FAT_FORM>$count";
 	$memPatterns{$mask} = $1;       
        $STNmask="<FAT_STN>$count";
        $count++;        
	$raw=$1;
	$instmonth=$2; 
	$daystr="NULL";   $wdaystr="NULL";
	$yearstr=$4; 

	#print RESULT1 "$raw"; 
	
	if ($3) {$k="Month , Year";}  else {$k="Month Year";}
 	$k =~ s/,$//g;

	if ($instmonth =~ /($monthpat)/) {
		$monthstr = $month{$instmonth}; 
	} elsif ($instmonth =~ /($monthabrevpat)/)  { 
		$monthstr = $month{$monthabrev{$instmonth}};
	}

	#print RESULT1 "\tNULL $yearstr:$monthstr:NULL\n";
	$memSTN{$STNmask} = "$wdaystr $yearstr:$monthstr:$daystr";	
	$line =~ s/($monthpat|$monthabrevpat)\s*(,)?\s+(\d\d+)/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
		 
 } # End while


#----------- The 11th Form -----------

########## DETECTING TIME ....
while($line =~ m/\s+(($digithour)\s*(:$digitminut|:5[0-9])\s*(:$digitminut|:5[0-9])\s*($am|$pm))\s/go){
# detect time 12 : 55:30 ... 11:53 a.m. 
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

	$mask = "<FAT_FORM>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wraw=$2.$3.$4. ".". $5;  
	$wdaystr=$1;   
	$yearstr="NULL"; $monthstr="NULL"; $daystr="NULL";

	$k="Hour:Min:Second ampm"; $k =~ s/,$//g;
 	
	#print RESULT1 "$wdaystr"; 
	#print RESULT1 "\t$wraw $yearstr:$monthstr:$daystr\n";
	$memSTN{$STNmask} = "$wraw $yearstr:$monthstr:$daystr";
	$line =~ s/$wdaystr/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
	 
} # End while

while($line =~ m/\s+(($digithour)\s*(\.($digitminut|5[0-9]))\s*(\.($digitminut|5[0-9]))\s*($am|$pm))\s/go){
# detect time 12.55.30
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

	$mask = "<FAT_FORM>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wraw=$2.":".$4. ":".$6.".". $7;  
	$wdaystr=$1;   
	$yearstr="NULL"; $monthstr="NULL"; $daystr="NULL";
	$k="Hour.Min.Second ampm"; $k =~ s/,$//g;
 	
	#print RESULT1 "$wdaystr"; 
	#print RESULT1 "\t$wraw $yearstr:$monthstr:$daystr\n";
	$memSTN{$STNmask} = "$wraw $yearstr:$monthstr:$daystr";
	$line =~ s/$wdaystr/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
	 
} # End while


#----------- The 12th Form ----------- 11:53 a.m. 
 
while($line =~ m/\s+(($digithour)\s*(:$digitminut|:5[0-9])\s*($am|$pm))\s/go) {
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

	$mask = "<FAT_FORM>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$1;
	$wraw=$2.$3.":00.".$4;  
	$yearstr="NULL"; $monthstr="NULL"; $daystr="NULL";
	

	$k="Hour:Min ampm"; $k =~ s/,$//g;
 	
	#print RESULT1 "$wdaystr"; 
	#print RESULT1 "\t$wraw $yearstr:$monthstr:$daystr\n";
	$memSTN{$STNmask} = "$wraw $yearstr:$monthstr:$daystr";
	$line =~ s/$wdaystr/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
 		
} # End while

while($line =~ m/\s+(($digithour)\s*(\.($digitminut|5[0-9]))\s*($am|$pm))\s/go) {
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

	$mask = "<FAT_FORM>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	
	$wdaystr=$1;
	$wraw=$2.":". $4.":00.".$5;  
	$yearstr="NULL"; $monthstr="NULL"; $daystr="NULL";
	

	$k="Hour.Min ampm"; $k =~ s/,$//g;
 	
	#print RESULT1 "$wdaystr"; 
	#print RESULT1 "\t$wraw $yearstr:$monthstr:$daystr\n";
	$memSTN{$STNmask} = "$wraw $yearstr:$monthstr:$daystr";
	$line =~ s/$wdaystr/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
 		
} # End while



#----------- The 13th Form -----------

while($line =~ m/\s+(($digithour)\s*($am|$pm))\s/go) {
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

	$mask = "<FAT_FORM>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$k="Hour ampm"; $k =~ s/,$//g;
 	
	$wdaystr=$1;
	$wraw=$2.":00:00.".$3;  
	$yearstr="NULL"; $monthstr="NULL"; $daystr="NULL";
	#print RESULT1 "$wdaystr";
	#print RESULT1 "\t$wraw $yearstr:$monthstr:$daystr\n";
	$memSTN{$STNmask} = "$wraw $yearstr:$monthstr:$daystr";
	$line =~ s/$wdaystr/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
 

} #end while ...

# Added on June 23rd 2005
while($line =~ m/\s+(($digithour):(0[0-9]|[1-5][0-9]))\s+/go) {
$daystr = "NULL"; $monthstr = "NULL"; $yearstr = "NULL"; $wdaystr = "NULL"; 
$instwday =""; $instmonth = "";

	$mask = "<FAT_FORM>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$1;
	$wraw=$2.":".$3.":00";  
	$yearstr="NULL"; $monthstr="NULL"; $daystr="NULL";
	
	$k="Hour:Min ampm"; $k =~ s/,$//g;
	$memSTN{$STNmask} = "$wraw $yearstr:$monthstr:$daystr";
	$line =~ s/$wdaystr/<DATE standard\=\"$STNmask\">$mask<\/DATE>/;
 		
} # End while


#---------------------------------------------------------
#******* DETECT NUMBERS in DIGITS  ***********************
#--percentages, measurements, currencies, basic numbers 
#---------------------------------------------------------
#==========================================================================================================================================#
#============================== I- The number is delimited by \s+ and \s+, is located ANYWHERE IN THE SENTENCE ============================#
#==========================================================================================================================================#
#----------------------------------------
#----------- The 1st NUM Form -----------  using "$" 

while($line =~ m/\s+(((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th))\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$2;
	$number =$3.$4.$5.$6; $suf=$8;
 	$suffix = "$9";
	$number =~ s/,//g;
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
		
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...
		
while($line =~ m/\s+((\-|\+)?((\d+)(\s+\d+)*)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£))\s+/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
#print "PATENT IS $patent\t$1\n\n";
             
	$wdaystr=$3;
	#$number =$3;
	$suffix = $6;
	$number =$3; $number =~ s/\s+//g;
	

	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


while($line =~ m/\s+((\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$number =$3;
	$suffix = $8;$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#----------------

while($line =~ m/\s+((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$number =$4.$5.$6.$7; $suf=$9;
	$suffix= "$2";
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
	$number =~ s/,//g;
	
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


while($line =~ m/\s+((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(\-|\+)?((\d+)(\s+\d+)*))\s+/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$4;
	$suffix = $2;
	$number =$4; $number =~ s/\s+//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

while($line =~ m/\s+((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*))\s+/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$4; 
	$number =$4;
	$suffix = $2;    
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#----------------------------------------
#----------- The 2nd NUM Form ----------- % per cent pct %, with possible {minus, -, +} , quantities and currencies

while($line =~ m/\s+(((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th))\s*($patent))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$2;
	$number =$3.$4.$5.$6; $suf=$8;
	$suffix = "$9";
	
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
		
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


		
while($line =~ m/\s+((\-|\+)?((\d+)(\s+\d+)*)\s*($patent))\s+/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$suffix = $6;
	$number =$3; $number =~ s/\s+//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
	} #end while ...


while($line =~ m/\s+((\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)\s*($patent))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$number =$3;
	$suffix = $8;
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	

	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#----------------

while($line =~ m/\s+(($patent)\s*((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$number =$4.$5.$6.$7; $suf=$9;
	$suffix= "$2";
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


while($line =~ m/\s+(($patent)\s*(\-|\+)?((\d+)(\s+\d+)*))\s+/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$4;
	$suffix = $2;
	$number =$4; $number =~ s/\s+//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

while($line =~ m/\s+(($patent)\s*(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$4;
	$number =$4;
	$suffix = $2;    
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


#----------------------------------------
#----------- The 3rd NUM Form ----------- LA BASE DES CHIFFRES, without any extension  

# BASE of Numbers-1 : with {million, billion, etc ..} and  {st, nd, ed}

while($line =~ m/\s+((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$1;
	$number =$2.$3.$4.$5; $suf=$7;
	
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
		
	$suffix = "NULL";
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...

# BASE of Numbers-2 : with "-" ex. for phone numberes 3333-3333-3333 ...

while($line =~ m/\s+((\d+)((-(\d+))+))\s+/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$1; 
	$number =$2.$3;
	$suffix = "NULL";
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#consider BASE Numbers-2 : with "/" ex. 1988/98 
while($line =~ m/\s+((\d+)((\/(\d+))+))\s+/go) {

	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
        $wdaystr=$1; 
	$number =$2.$3;
	$suffix = "NULL";
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


# BASE of Numbers-3 : with spaces \s+   or juste concatened numbers               

while($line =~ m/\s+(\-|\+)?((\d+)((\s+\d+)*))\s+/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 
    	$wdaystr=$2; 
	$suffix = "NULL";
	$number =$2; $number =~ s/\s+//g;

	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

# BASE of Numbers-4 : with "," and "." 

while($line =~ m/\s+(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$2;
	$number =$2; $number =~ s/\s+//g;
	$suffix = "NULL";
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#==========================================================================================================================================#
#============================== II- The number is delimited by ^ and $, is the only entity located in a line===============================#
#==========================================================================================================================================#


#----------------------------------------
#----------- The 1st NUM Form -----------  using "$" 

#while($line =~ m/^(((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th))\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£))$/go) {
while($line =~ m/^(((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)(\.\s*)?$/go) {

	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$2;
	$number =$3.$4.$5.$6; $suf=$8;
	$suffix = "$9";
	
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


		
while($line =~ m/^((\-|\+)?((\d+)(\s+\d+)*)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£))(\.\s*)?$/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$number =$3;
	$suffix = $6;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


while($line =~ m/^((\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£))(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$number =$3;
	$suffix = $8;
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#----------------

while($line =~ m/^((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$3;
	$number =$4.$5.$6.$7; $suf=$9;
	$suffix= "$2";
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


while($line =~ m/^((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(\-|\+)?((\d+)(\s+\d+)*))(\.\s*)?$/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$4;
	$number =$4;
	$suffix = $2;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

while($line =~ m/^((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*))(\.\s*)?$/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$4;
	$number =$4;
	$suffix = $2;    
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#----------------------------------------
#----------- The 2nd NUM Form ----------- % per cent pct %, with possible {minus, -, +} , quantities and currencies

while($line =~ m/^(((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))\s*($patent)(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$2;
	$number =$3.$4.$5.$6; $suf=$8;
	$suffix = "$9";

	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }

	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


while($line =~ m/^((\-|\+)?((\d+)(\s+\d+)*)\s*($patent))(\.\s*)?$/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$3;
	$number =$3;
	$suffix = $6;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


while($line =~ m/^((\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)\s*($patent))(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$3;
	$number =$3;
	$suffix = $8;
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#----------------

while($line =~ m/^(($patent)\s*((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$3;
	$number =$4.$5.$6.$7; $suf=$9;
	$suffix= "$2";
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


while($line =~ m/^(($patent)\s*(\-|\+)?((\d+)(\s+\d+)*))(\.\s*)?$/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$4;
	$number =$4;
	$suffix = $2;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

while($line =~ m/^(($patent)\s*(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*))(\.\s*)?$/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
           
	$wdaystr=$4;
	$number =$4;
	$suffix = $2;    
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


#----------------------------------------
#----------- The 3rd NUM Form ----------- LA BASE DES CHIFFRES, without any extension  

while($line =~ m/^((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th))(\.\s*)?$/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$1;
	$number =$2.$3.$4.$5; $suf=$7;

	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
		
	$suffix = "NULL";
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...

# BASE of Numbers-2 : with "-" ex. 1988-98 or 2003-2005 or phone numbers 3333-3333-3333 ...

while($line =~ m/^((\d+)((-(\d+))+))(\.\s*)?$/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$1; 
	$number =$2.$3;
	$suffix = "NULL";
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#consider BASE Numbers-2 : with "/" ex. 1988/98 
while($line =~ m/^((\d+)((\/(\d+))+))(\.\s*)?$/go) {

	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
        $wdaystr=$1; 
	$number =$2.$3;
	$suffix = "NULL";
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...



# BASE of Numbers-3 : with spaces \s+   or juste concatened numbers               

while($line =~ m/^(\-|\+)?((\d+)((\s+\d+)*))(\.\s*)?$/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$2;
	$number =$2; $number =~ s/\s+//g;
	$suffix = "NULL";
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

# BASE of Numbers-4 : with "," and "." 

while($line =~ m/^(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$2;
	$number =$2;
	$suffix = "NULL";
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...






#==========================================================================================================================================#
#============================== III- The number is delimited by ^ and \s+, is located int the begining of a line ==========================#
#==========================================================================================================================================#

while($line =~ m/^(((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th))\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$2;
	$number =$3.$4.$5.$6; $suf=$8;
	$suffix = "$9";
	
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
	$number =~ s/,//g;	
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


		
while($line =~ m/^((\-|\+)?((\d+)(\s+\d+)*)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£))\s+/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
            
	$wdaystr=$3;
	$number =$3;
	$suffix = $6;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


while($line =~ m/^((\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$3;
	$number =$3;
	$suffix = $8;
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#----------------

while($line =~ m/^((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$3;
	$number =$4.$5.$6.$7; $suf=$9;
	$suffix= "$2";
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


while($line =~ m/^((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(\-|\+)?((\d+)(\s+\d+)*))\s+/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$4;
	$number =$4;
	$suffix = $2;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

while($line =~ m/^((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*))\s+/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$4;
	$number =$4;
	$suffix = $2;    
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...



#----------------------------------------
#----------- The 2nd NUM Form ----------- % per cent pct %, with possible {minus, -, +} , quantities and currencies

while($line =~ m/^(((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))\s*($patent)\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$2;
	$number =$3.$4.$5.$6; $suf=$8;
	$suffix = "$9";
	
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
		
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


		
while($line =~ m/^((\-|\+)?((\d+)(\s+\d+)*)\s*($patent))\s+/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$number =$3;
	$suffix = $6;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


while($line =~ m/^((\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)\s*($patent))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$number =$3;
	$suffix = $8;
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#----------------

while($line =~ m/^(($patent)\s*((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$number =$4.$5.$6.$7; $suf=$9;
	$suffix= "$2";
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


while($line =~ m/^(($patent)\s*(\-|\+)?((\d+)(\s+\d+)*))\s+/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$4;
	$number =$4;
	$suffix = $2;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

while($line =~ m/^(($patent)\s*(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*))\s+/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$4;
	$number =$4;
	$suffix = $2;    
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#----------------------------------------
#----------- The 3rd NUM Form ----------- LA BASE DES CHIFFRES, without any extension  


while($line =~ m/^((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th))\s+/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$1;
	$number =$2.$3.$4.$5; $suf=$7;
	
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
		
	$suffix = "NULL";
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...

# BASE of Numbers-2 : with "-" ex. for phone numberes 3333-3333-3333 ...

while($line =~ m/^((\d+)((-(\d+))+))\s+/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$1; 
	$number =$2.$3;
	$suffix = "NULL";
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#consider BASE Numbers-2 : with "/" ex. 1988/98 
while($line =~ m/^((\d+)((\/(\d+))+))\s+/go) {

	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
        $wdaystr=$1; 
	$number =$2.$3;
	$suffix = "NULL";
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


# BASE of Numbers-3 : with spaces \s+   or juste concatened numbers               

while($line =~ m/^(\-|\+)?((\d+)((\s+\d+)*))\s+/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$2; 
	$number =$2; $number =~ s/\s+//g;
	$suffix = "NULL";
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

# BASE of Numbers-4 : with "," and "." 

while($line =~ m/^(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)\s+/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$2;
	$number =$2;
	$suffix = "NULL";
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...



#==========================================================================================================================================#
#============================== IV- The number is delimited by \s+ and $ , is located int the end of a line ===============================#
#==========================================================================================================================================#

while($line =~ m/\s+(((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$2;
	$number =$3.$4.$5.$6; $suf=$8;
	$suffix = "$9";
	
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
		
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


		
while($line =~ m/\s+((\-|\+)?((\d+)(\s+\d+)*)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£))(\.\s*)?$/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$3;
	$number =$3;
	$suffix = $6;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


while($line =~ m/\s+((\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£))(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$number =$3;
	$suffix = $8;
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#----------------

while($line =~ m/\s+((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$3;
	$number =$4.$5.$6.$7; $suf=$9;
	$suffix= "$2";
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


while($line =~ m/\s+((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(\-|\+)?((\d+)(\s+\d+)*))(\.\s*)?$/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$4;
	$number =$4;
	$suffix = $2;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

while($line =~ m/\s+((\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*))(\.\s*)?$/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$4;
	$number =$4;
	$suffix = $2;    
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...



#----------------------------------------
#----------- The 2nd NUM Form ----------- % per cent pct %, with possible {minus, -, +} , quantities and currencies

while($line =~ m/\s+(((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))\s*($patent)(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$2;
	$number =$3.$4.$5.$6; $suf=$8;
	$suffix = "$9";
	
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
		
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


while($line =~ m/\s+((\-|\+)?((\d+)(\s+\d+)*)\s*($patent))(\.\s*)?$/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$3;
	$number =$3;
	$suffix = $6;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


while($line =~ m/\s+((\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)\s*($patent))(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$3;
	$number =$3;
	$suffix = $8;
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#----------------

while($line =~ m/\s+(($patent)\s*((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)))(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $3;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$3;
	$number =$4.$5.$6.$7; $suf=$9;
	$suffix= "$2";
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...


while($line =~ m/\s+(($patent)\s*(\-|\+)?((\d+)(\s+\d+)*))(\.\s*)?$/go) {
	$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$wdaystr=$4;
	$number =$4;
	$suffix = $2;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

while($line =~ m/\s+(($patent)\s*(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*))(\.\s*)?$/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $4;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$4;
	$number =$4;
	$suffix = $2;    
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...


#----------------------------------------
#----------- The 3rd NUM Form ----------- LA BASE DES CHIFFRES, without any extension  

while($line =~ m/\s+((\d+)(,\d+)*(\.\d+)?(,\d+)*\s*(-)?(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th))(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 

	$wdaystr=$1;
	$number =$2.$3.$4.$5; $suf=$7;
	
	if ($suf=~ /(hundred|thousand|million|billion|trillion|quadrillion|quintillion|hundreds|thousands|millions|billions|trillions|quadrillions|quintillions|st|nd|rd|th)/) {$number =$number." ".$suf; } #else {$number =$number.$suf; }
		
	$suffix = "NULL";
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;

} #end while ...

# BASE of Numbers-2 : with "-" ex. for phone numberes 3333-3333-3333 ...

while($line =~ m/\s+((\d+)((-(\d+))+))(\.\s*)?$/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$1; 
	$number =$2.$3;
	$suffix = "NULL";
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#consider BASE Numbers-2 : with "/" ex. 1988/98 
while($line =~ m/\s+((\d+)((\/(\d+))+))(\.\s*)?$/go) {

	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $1;       
	$STNmask="<FAT_STN>$count";
	$count++; 
        $wdaystr=$1; 
	$number =$2.$3;
	$suffix = "NULL";
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...



# BASE of Numbers-3 : with spaces \s+   or juste concatened numbers               

while($line =~ m/\s+(\-|\+)?((\d+)((\s+\d+)*))(\.\s*)?$/go) {
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$2; 
	$number =$2;$number =~ s/\s+//g;
	$suffix = "NULL";
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

# BASE of Numbers-4 : with "," and "." 

while($line =~ m/\s+(\-|\+)?((\d+)(,\d+)*(\.\d+)?(,\d+)*)(\.\s*)?$/go) {
$mask="";
	$mask = "<NUMEX>$count";
	$memPatterns{$mask} = $2;       
	$STNmask="<FAT_STN>$count";
	$count++; 
             
	$wdaystr=$2;
	$number =$2;
	$suffix = "NULL";
	$number =~ s/,//g;
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
} #end while ...

#HERE ADD THE NEW ASSUMPTIONS ....

#-------------------------------------------------------
#-------------------------------------------------------
# ****** DETECT NUMBERS with alphabets. ****************
#--percentages, measurements, currencies, basic numbers 
#-------------------------------------------------------

# the number is delimited by \s+ and \s+

while($line =~ m/\s+(($numpat)\s*(-)?(($numpat\s*)+))(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s+/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$2." ".$4;
		$suffix = $7; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}


while($line =~ m/\s+(£|\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?((($numpat)\s*)+))\s+/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$2;  
		$number =$3." ".$5;
		$suffix = $1; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		 if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}


while($line =~ m/\s+(($numpat)\s*(-)?(.*)($numpat)\s*(-)?)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|££)\s+/go) {
	$mask = "<NUMEX>$count"; 
            	$wdaystr=$1; 
		$number =$1;
		$suffix = $7; 
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
		if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*(-)?$/){
			$correct =$correct . " ".$k[$i];
			}
			else { $wdaystr=$correct; $i=@k;
			}	
		}
		$wdaystr=$correct; $wdaystr=~ s/ $//g;	
		$number=$correct;$number =~ s/,//g;
		## 
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			$memPatterns{$mask} = $correct;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	$number= $correct;    
			#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
	}
}


while($line =~ m/\s+(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?(.*)($numpat(-)?))\s+/go) {
	$mask = "<NUMEX>$count";
	$wdaystr=$2;  
	$number =$2;
	$suffix = $1;  
	$number =~ s/,//g;
	@k=split(/\s+/,$wdaystr);

	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
		$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
		if ($k[$i]=~ /^($numpat)\s*$/){
			$correct =$correct . " ".$k[$i];
		}
		else {$wdaystr=$correct; $i=@k;
		}	
	}
	$wdaystr=$correct; $wdaystr=~ s/ $//g;	
	$number=$correct;$number =~ s/,//g;
	## 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {

	
	$avoid=1;     
	}else {
	$memPatterns{$mask} = $correct;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$number= $correct;    
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
	} 
	}
}

while ($line=~ m/\s+(($numpat)\s*(-)?\s+(.*))\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s+/go){
 	$mask = "<NUMEX>$count";
	$wdaystr=$1; 
	$number =$2;
	$suffix = $5;  
	$number =~ s/,//g;
	@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
	$correct=$k[0];$correct=~ s/-//g;
	for ($i=1; $i<@k; $i++){
		$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
		if ($k[$i]=~ /^($numpat)\s*$/){
		$correct =$correct . " ".$k[$i];
		}
		else {$wdaystr=$k[$i-1]; $number=$k[$i-1];
		$i=@k;
		}
	}
	$wdaystr=~ s/ $//g;			
	  
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
	$avoid=1;     
		}else {
		$memPatterns{$mask} = $wdaystr;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $wdaystr;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}	
	}
}

while ($line=~ m/\s+(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?\s+(.*))\s+/go){
 		$mask = "<NUMEX>$count";
		$wdaystr=$2; 
		$number =$3;
		$suffix = $1;  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
		if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
		$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
		if ($k[$i]=~ /^($numpat)\s*$/){
			$correct =$correct . " ".$k[$i];
		}
			else {$wdaystr=$k[$i-1];$number=$k[$i-1];
				$i=@k;
			}
		}
	$wdaystr=~ s/ $//g;			
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
	 $avoid=1;   			}else {
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	$number= $wdaystr;    
			#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}			
}
}
#------ $end ..

# BASIC ...

while($line =~ m/\s+(($numpat)\s*(-)?((($numpat)\s*)+))\s+/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$2." ".$4;
		$suffix = "NULL"; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		 
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			#print "\n\nHERE:\t||$wdaystr||\n\n";		 
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}



while ($line=~ m/\s+(($numpat)\s*(-)?(.*)($numpat))\s+/go){
 		$mask = "<NUMEX>$count";
            	$wdaystr=$1; 
            	#print "KA:\t|$2|\t|$3|\t|$4|\t|$5|\n\n";  
		$number =$1;
		$suffix = "NULL";  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
		if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];
			for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}else {$wdaystr=$correct; $i=@k;
			}	
			}
		$wdaystr=$correct; $wdaystr=~ s/ $//g;		
		$number=$correct;$number =~ s/,//g;
	# 
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		#print "\n\nHERE:\t|||$wdaystr|||\n\n";		 
		$memPatterns{$mask} = $correct;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $correct;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		} 
	}
}
while ($line=~ m/\s+(($numpat)\s*(-)?\s+(.*))\s+/go){
 		$mask = "<NUMEX>$count";
		$wdaystr=$1;  
		$number =$2;
		$suffix = "NULL";  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$k[$i-1]; 
				$number=$k[$i-1];
				$i=@k;
			}
		}
		$wdaystr=~ s/ $//g;			
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		#print "\n\nHERE:\t||||$wdaystr||||\n\n";		 
		$memPatterns{$mask} = $wdaystr;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $wdaystr;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
		
	}
}

#----- 
#----- the number is located in the begining and the end of a line 

while($line =~ m/^(($numpat)\s*(-)?((($numpat)\s*)+))(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)(\.\s*)?$/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$2." ".$4;
		$suffix = $7; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		 
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}


while($line =~ m/^(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?((($numpat)\s*)+))(\.\s*)?$/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$2;  
		$number =$3." ".$5;
		$suffix = $1; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		 
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}

while($line =~ m/^(($numpat)\s*(-)?(.*)($numpat)\s*(-)?)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)(\.\s*)?$/go) {
$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$1;
		$suffix = $7;  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
		if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;	
		for ($i=1; $i<@k; $i++){
		$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
		if ($k[$i]=~ /^($numpat)\s*$/){
			$correct =$correct . " ".$k[$i];
		} else {$wdaystr=$correct; $i=@k;
		}	
		}
		$wdaystr=$correct;$wdaystr=~ s/ $//g;	
		$number=$correct;$number =~ s/,//g;
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		$memPatterns{$mask} = $correct;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $correct;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
	}
}

while($line =~ m/^(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?(.*)($numpat)\s*(-)?)(\.\s*)?$/go) {
	$mask = "<NUMEX>$count";
	$wdaystr=$2;  
	$number =$2;
	$suffix = $1;  
	$number =~ s/,//g;
	@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;	
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
			$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$correct; 
			$i=@k;
			}	
		}
		$wdaystr=$correct; $wdaystr=~ s/ $//g;	
		$number=$correct;$number =~ s/,//g;
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		$memPatterns{$mask} = $correct;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $correct;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
	}
}

while ($line=~ m/^(($numpat)\s*(-)?\s+(.*))\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)(\.\s*)?$/go){
 		$mask = "<NUMEX>$count";
		$wdaystr=$1;  
		$number =$2;
		$suffix = $5;  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$k[$i-1]; $number=$k[$i-1];
				$i=@k;
			}
		}
		$wdaystr=~ s/ $//g;			
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
			}else {
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	$number= $wdaystr;    
			#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
			}		
	}
}
while ($line=~ m/^(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?\s+(.*))(\.\s*)?$/go){
 		$mask = "<NUMEX>$count";
		$wdaystr=$2;  
		$number =$3;
		$suffix = $1;  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$k[$i-1]; $number=$k[$i-1];
				$i=@k;
			}
		}
		$wdaystr=~ s/ $//g;			
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
			}else {
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	$number= $wdaystr;    
			#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
			}	
			
	}
}
#-----

while($line =~ m/^(($numpat)\s*(-)?(((hundred|thousand|million|billion|trillion|quadrillion|quintillion| hundreds|thousands|millions|billions|trillions|quadrillions|quintillions)\s*)+))(\.\s*)?$/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$2." ".$4;
		$suffix = "NULL"; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		 
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}

while ($line=~ m/^(($numpat)\s*(-)?(.*)($numpat)\s*(-)?)(\.\s*)?$/go){
 		$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$1;
		$suffix = "NULL";  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;		
		for ($i=1; $i<@k; $i++){
		$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
		if ($k[$i]=~ /^($numpat)\s*$/){
			$correct =$correct . " ".$k[$i];
		}
		else {$wdaystr=$correct; $i=@k;
			}	
		}
		$wdaystr=$correct;$wdaystr=~ s/ $//g;	
		$number=$correct;$number =~ s/,//g;
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		$memPatterns{$mask} = $correct;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $correct;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
	}
}
while ($line=~ m/^(($numpat)\s*(-)?\s+(.*))(\.\s*)?$/go){
 		$mask = "<NUMEX>$count";
		$wdaystr=$1;  
		$number =$2;
		$suffix = "NULL";  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;		
		for ($i=1; $i<@k; $i++){
		$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
		if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$k[$i-1]; $number=$k[$i-1];
				$i=@k;
			}
			
		}
		$wdaystr=~ s/ $//g;			
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
			$avoid=1;   
		}else {
		$memPatterns{$mask} = $wdaystr;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $wdaystr;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}	
	}
}
#---- the number is located in the begining of a line

while($line =~ m/^(($numpat)\s*(-)?((($numpat)\s*)+))(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s+/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$2." ".$4;
		$suffix = $7; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		 
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}


while($line =~ m/^(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?((($numpat)\s*)+))\s+/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$2;  
		$number =$3." ".$5;
		$suffix = $1; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		 
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}



while($line =~ m/^(($numpat)\s*(-)?(.*)($numpat)\s*(-)?)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s+/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$1;
		$suffix = $7;  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
		if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;		
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$correct; $i=@k;
			}	
		}
		$wdaystr=$correct; $wdaystr=~ s/ $//g;	
		$number=$correct;$number =~ s/,//g;
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		$memPatterns{$mask} = $correct;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $correct;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		} 
	}
}
while($line =~ m/^(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?(.*)($numpat)\s*(-)?)\s+/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$2;  
		$number =$2;
		$suffix = $1;  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
	$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$correct; $i=@k;
			}	
		}
		$wdaystr=$correct; $wdaystr=~ s/ $//g;	
		$number=$correct;$number =~ s/,//g;
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {		$memPatterns{$mask} = $correct;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $correct;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		} 
	}
}

while ($line=~ m/^(($numpat)\s*(-)?\s+(.*))\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s+/go){
 		$mask = "<NUMEX>$count";
		$wdaystr=$1; 
		$number =$2;
		$suffix = $5;  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;		
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$k[$i-1]; $number=$k[$i-1];
				$i=@k;
			}
		}
		$wdaystr=~ s/ $//g;			
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
			}else {
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	$number= $wdaystr;    
			#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}	
	}
}

while ($line=~ m/^(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?\s+(.*))\s+/go){
 		$mask = "<NUMEX>$count";
		$wdaystr=$2; 
		$number =$3;
		$suffix = $1;  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$k[$i-1]; $number=$k[$i-1];
				$i=@k;
			}
		}
		$wdaystr=~ s/ $//g;			
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
			}else {
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	$number= $wdaystr;    
			#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
			}	
	}
}
# ----

while($line =~ m/^(($numpat)\s*(-)?((($numpat)\s*)+))\s+/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$1; 
		$number =$2." ".$4;
		$suffix = "NULL"; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
		$memPatterns{$mask} = $wdaystr;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        #print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}
while ($line=~ m/^(($numpat)\s*(-)?(.*)($numpat)\s*(-)?)\s+/go){
 		$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$1;
		$suffix = "NULL";  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;		
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$correct; $i=@k;
			}	
		}
		$wdaystr=$correct;
		$wdaystr=~ s/ $//g;	
		$number=$correct;$number =~ s/,//g;
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		$memPatterns{$mask} = $correct;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $correct;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		} 
}
}
while ($line=~ m/^(($numpat)\s*(-)?\s+(.*))\s+/go){
 		$mask = "<NUMEX>$count";
		$wdaystr=$1;  
		$number =$2;
		$suffix = "NULL";  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
		if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$k[$i-1]; $number=$k[$i-1];
				$i=@k;
			}
		}
		$wdaystr=~ s/ $//g;			
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		$memPatterns{$mask} = $wdaystr;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $wdaystr;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}			
	}
}

#----- the number is located in the end of a line 

while($line =~ m/\s+(($numpat)\s*(-)?((($numpat)\s*)+))(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)(\.\s*)?$/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$2." ".$4;
		$suffix = $7; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		 
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}


while($line =~ m/\s+(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?((($numpat)\s*)+))(\.\s*)?$/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$2;  
		$number =$3." ".$5;
		$suffix = $1; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		 
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}



while($line =~ m/\s+(($numpat)\s*(-)?(.*)($numpat)\s*(-)?)\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)(\.\s*)?$/go) {
	$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$1;
		$suffix = $7;  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;		
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$correct; $i=@k;
			}	
		}
		$wdaystr=$correct; $wdaystr=~ s/ $//g;	
		$number=$correct;$number =~ s/,//g;
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		$memPatterns{$mask} = $correct;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $correct;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		} 
	}
}

while($line =~ m/\s+(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?(.*)($numpat)\s*(-)?)(\.\s*)?$/go) {
	$mask = "<NUMEX>$count";
	$wdaystr=$2; 
	$number =$2;
	$suffix = $1;  
	$number =~ s/,//g;
	@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
		$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
		if ($k[$i]=~ /^($numpat)\s*$/){
			$correct =$correct . " ".$k[$i];
		}else {$wdaystr=$correct; $i=@k;
		}	
	}
	$wdaystr=$correct; $wdaystr=~ s/ $//g;	
	$number=$correct;$number =~ s/,//g;
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
	$avoid=1;   
	}else {
	$memPatterns{$mask} = $correct;       
	$STNmask="<FAT_STN>$count";
	$count++; 
	$number= $correct;    
	#print RESULT2 "$wdaystr";
	#print RESULT2 "\t$number\t$suffix\n";
	$memSTN{$STNmask} = "$number $suffix";
	$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
	} 
	}
}


while ($line=~ m/\s+(($numpat)\s*(-)?\s+(.*))\s*(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)(\.\s*)?$/go){
 		$mask = "<NUMEX>$count";
		$wdaystr=$1;  
		$number =$2;
		$suffix = $5;  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
		$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
		if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$k[$i-1]; $number=$k[$i-1];
				$i=@k;
			}
		}
		$wdaystr=~ s/ $//g;			
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		$memPatterns{$mask} = $wdaystr;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $wdaystr;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}	
	}
}

while ($line=~ m/\s+(\$|\$us|us\$|hk\$|\$hk|rmb|nt\$|£)\s*(($numpat)\s*(-)?\s+(.*))(\.\s*)?$/go){
 		$mask = "<NUMEX>$count";
		$wdaystr=$2; 
		$number =$3;
		$suffix = $1;  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$k[$i-1]; $number=$k[$i-1];
				$i=@k;
			}
		}
		$wdaystr=~ s/ $//g;			
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		$memPatterns{$mask} = $wdaystr;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $wdaystr;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}	
	}
}
#---

while($line =~ m/\s+(($numpat)\s*(-)?((($numpat)\s*)+))(\.\s*)?$/go) {
		$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$2." ".$4;
		$suffix = "NULL"; 
		$number =~ s/,//g;
		$wdaystr=~ s/ $//g;	
		$number =~ s/,//g;
		 
		if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;    
		}else {			
			$memPatterns{$mask} = $wdaystr;       
			$STNmask="<FAT_STN>$count";
			$count++; 
	        	#print RESULT2 "$wdaystr";
			#print RESULT2 "\t$number\t$suffix\n";
			$memSTN{$STNmask} = "$number $suffix";
			$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}
}

while ($line=~ m/\s+(($numpat)\s*(-)?(.*)\s*(-)?($numpat))(\.\s*)?$/go){
 		$mask = "<NUMEX>$count";
            	$wdaystr=$1;  
		$number =$1;
		$suffix = "NULL";  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;		
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$correct; $i=@k;
			}	
		}
		$wdaystr=$correct; $wdaystr=~ s/ $//g;	
		$number=$correct;$number =~ s/,//g;
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		$memPatterns{$mask} = $correct;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $correct;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}	
	} 
}

while ($line=~ m/\s+(($numpat)\s*(-)?\s+(.*))(\.\s*)?$/go){
 		$mask = "<NUMEX>$count";
		$wdaystr=$1; 
		$number =$2;
		$suffix = "NULL";  
		$number =~ s/,//g;
		@k=split(/\s+/,$wdaystr);
	if ($k[0]=~ /^($numpat)\s*$/){
		$correct=$k[0];$correct=~ s/-//g;
		for ($i=1; $i<@k; $i++){
			$k[$i]=~ s/-/ /g; $k[$i]=~ s/ $//g;
			if ($k[$i]=~ /^($numpat)\s*$/){
				$correct =$correct . " ".$k[$i];
			}
			else {$wdaystr=$k[$i-1]; $number=$k[$i-1];
				$i=@k;
			}
		}
		$wdaystr=~ s/ $//g;			
	# 
	if (($wdaystr =~ /^($numavoidpat\s*)$/) &&($wdaystr =! /^($numavoidpat\s*)($numpat)/)) {
		$avoid=1;   
		}else {
		$memPatterns{$mask} = $wdaystr;       
		$STNmask="<FAT_STN>$count";
		$count++; 
	        $number= $wdaystr;    
		#print RESULT2 "$wdaystr";
		#print RESULT2 "\t$number\t$suffix\n";
		$memSTN{$STNmask} = "$number $suffix";
		$line =~ s/$wdaystr/<NUM standard\=\"$STNmask\">$mask<\/NUM>/;
		}	
}
}
########### ELSE GENERAL - print the lines without any date ...

#----------- Write back the forms for the line -----------
#print "aa= $line\r\n";
   
foreach my $formMask (keys %memPatterns){  
   	$form = $memPatterns{$formMask};
	$line =~ s/$formMask/$form/g;

} # end foreach

undef %memPatterns;

foreach my $stnMask (keys %memSTN){  
   	$stn = $memSTN{$stnMask};
	$line =~ s/$stnMask/$stn/g;

} # end foreach

undef %memSTN;

#$line .= "\r\n";   # Add ligne break


$line =~ s/ , <\/DATE>/<\/DATE> , /g; 
$line =~ s/ <\/DATE>/<\/DATE>/g;
$line =~ s/ , <\/NUM>/<\/NUM> , /g; 
$line =~ s/ \">/\"> /g;

$line =~ s/<\/DATE>\./<\/DATE> \./g;
$line =~ s/<\/NUM>\./<\/NUM> \./g;

$line =~ s/<\/DATE>(\S+)/<\/DATE> $1/g;
$line =~ s/<\/NUM>(\S+)/<\/NUM> $1/g;
$line =~ s/\"> /\">/g;

print RESULTMARKED "$line";

	
} # end of while <FILE>




close (FILE);
#close (RESULT1);
#close (RESULT2);
close (RESULTMARKED);

#END ----
