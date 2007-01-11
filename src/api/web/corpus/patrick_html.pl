#! /usr/bin/perl  
#use strict;
use warnings;
                                                                                                                                                                                    
#my $HELP = "
#patric_html.pl $file1 $file2 > $outfile
#";
                                                                                                                                                                                    
#our ($help, $h);
                                                                                                                                                                                    
#if ($help || $h) {
#    print $HELP;
#    exit 0;
#}
                                                                                                                                                                                    
$in1 = shift or die; # $HELP;
$in2 = shift or die; # $HELP;
                                                                                                                                                                                    
open(IN1, "<$in1") or die "Can't open $in1 for reading\n";
open(IN2, "<$in2") or die "Can't open $in2 for reading\n";


print "<html>" . "\n";
print "<head>" . "\n";
print "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=GB2312\">" . "\n";
print "</head>" . "\n";
print "<body bgcolor=\"#FFFFFF\">" . "\n";
                                                                                                                                                                                   
print "<table width=\"100%\"border=\"0\" cellpadding=\"5\">" . "\n";
 

$color_one = "#CCFFFF";
$color_two = "#99FFFF";

$loop_count = 0;
$cur_color = $color_one;

while (<IN1>) { 
   if (($loop_count % 2 ) eq 0 ) {
        $cur_color = $color_one;
    }
   else{
        $cur_color = $color_two;
   }
   print "<tr>\n";
   print "<td bgcolor=\"$cur_color\" <b>" . ++$loop_count . "</b></td>\n";
   chomp $_;   
    print "<td bgcolor=\"$cur_color\" width=\"50%\"><b>" . $_ . "</b></td>\n";
    $line2=<IN2>;
    chop $line2;
    print "<td bgcolor=\"$cur_color\" width=\"50%\"><b>" .$line2. "</b></td>\n";
   print "</tr>\n";
#   ++$loop_count;     
 } 

close IN1;
close IN2;
print "</table>\n</body>\n</html>\n";
 
