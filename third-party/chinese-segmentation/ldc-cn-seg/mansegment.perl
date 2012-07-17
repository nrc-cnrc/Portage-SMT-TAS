#!/usr/bin/perl 
###############################################################################
# This software is being provided to you, the LICENSEE, by the Linguistic     #
# Data Consortium (LDC) and the University of Pennsylvania (UPENN) under the  #
# following license.  By obtaining, using and/or copying this software, you   #
# agree that you have read, understood, and will comply with these terms and  #
# conditions:                                                                 #
#                                                                             #
# Permission to use, copy, modify and distribute, including the right to      #
# grant others the right to distribute at any tier, this software and its     #
# documentation for any purpose and without fee or royalty is hereby granted, #
# provided that you agree to comply with the following copyright notice and   #
# statements, including the disclaimer, and that the same appear on ALL       #
# copies of the software and documentation, including modifications that you  #
# make for internal use or for distribution:                                  #
#                                                                             #
# Copyright 1999 by the University of Pennsylvania.  All rights reserved.     #
#                                                                             #
# THIS SOFTWARE IS PROVIDED "AS IS"; LDC AND UPENN MAKE NO REPRESENTATIONS OR #
# WARRANTIES, EXPRESS OR IMPLIED.  By way of example, but not limitation,     #
# LDC AND UPENN MAKE NO REPRESENTATIONS OR WARRANTIES OF MERCHANTABILITY OR   #
# FITNESS FOR ANY PARTICULAR PURPOSE.                                         #
###############################################################################
# mansegment.perl Version 1.1
# Run as: mansegment.perl [dictfile] < infile > outfile
# A Chinese segmenter for both GB and BIG5 as long as the cooresponding 
# word frequency dictionary is used.
#
# Written by Zhibiao Wu at LDC on April 12 1999
# Modified by Xiaoyi Ma at LDC, March, 2003
# Change of v1.1:
# - simplified code
# - regenerated database to be compatible with perl5
# Bug fixed, May, 2005
#
#
# Algorithm: Dynamic programming to find the path which has the highest 
# multiple of word probability, the next word is selected from the longest
# phrase.
#
# dictfile is a two column text file, first column is the frequency, 
# second column is the word. The program will change the file into a dbm 
# file in the first run. So be sure to remove the dbm file if you have a
# newer version of the text file.
##############################################################################

$wd = 2;

if ($0 =~ /\//) {
    $DICTPATH = $1 if ( $0 =~ /(.+)\/[^\/]+/ );
} else {
    $DICTPATH = ".";
}

if (@ARGV[0] ne "") {
    $dictfile = @ARGV[0];
} else {
    $dictfile = "$DICTPATH/Mandarin.fre";
}

$dict_db = $dictfile.".db";

@ARGV=();
$#ARGV = -1;

# read in frequency dictionary in associate array.

&read_dict();

# read in Mandarin files.

while (<>) {
    chomp;
    split(//,$_);
    $newline = "";
    $ch = 0;
    while (@_) {
	$c = shift(@_);
	if ($c eq " ") {
	    $newline .= $c;
	} elsif ($c =~ /[\x80-\xff]/) {
	    $cc = $c . shift(@_);
	    if ($ch == -1){
		$newline = $newline . " " . $cc;
	    } else {
		$newline = $newline . $cc;
	    }
	    $ch = 1;
	} else {
	    if ($ch == 1) {
		$newline = $newline . " " . $c;
	    } else {
		$newline .= $c;
	    }
	    $ch = -1;
	}
    }

    $_ = $newline;
    s/^ *//g;
    @segment = split;

    foreach (@segment) {
	&process($_);
	print " ";
    } 
    print "\n";
}

sub process {
    my ($sentence) = @_;

    return if ($sentence eq "");
    
    if (!($sentence =~ /[\x80-\xff]/)) {
	print $sentence;
	return;
    }

    $top = 1;
    $value{1} = 1;
    $position{1} = 0;
    $next{1} = -1;
    $result{1} = "";
    $nextid = 2;
    $len = length($sentence);

    # Take out the top most path in the stack and extend that path
    # into several new paths, and put those paths into the stack.
    while (($top != -1) && 
	   (!(($position{$top} == $len) && ($next{$top} == -1)))) {

 	#print STDERR  "$. $result{$top}\n";

	# find the first open path
	$current = $top;
	$father = $top;
	while (($current != -1 ) && ($position{$current} == $len)) {
	    $father = $current;
	    $current = $next{$current};
	}

	# remove this path
	if ($current == $top) {
	    $top = $next{$top};
	} else {
	    $next{$father} = $next{$current};
	}

	if ($current == -1) {
	    # no open path, finished, take the first path
	    $next{$top} = -1;
	} else {
	    $firstword = substr($sentence, $position{$current}, 2);

	    # if the first character doesn't have word phrase in the dict.
	    # $freq{m,} is the length of the word
           $i = $freq{"m,$firstword"};
           if ($i > $len - $position{$current}) {
               $i = $len - $position{$current};
           }
           if ($i < $wd) {
             $i = $wd;
           }
                                                                              
           while ($i>=$wd) {
             $word = substr($sentence, $position{$current}, $i);
             if ($i == $wd) {
               $freq{$word} = 1; # single character always counts as a word
             }
             if ($freq{$word}) {
               &pronode();
             }
                                                                              
             $i -= $wd;
	 }
	}
    }
   

    if ($top == -1) {
	print STDERR "Error: $. $sentence.\n";
    } else {
	$result{$top} =~ s/^ *//g;
	print $result{$top};
    }

    return;
}


sub pronode {

    $value{$nextid} = $value{$current} * $freq{$word} / $freq{total};
    $result{$nextid} = $result{$current} . " " . $word;
    $position{$nextid} = $position{$current} + $i;
    
    # check to see whether there is duplicated path
    # if there is a duplicate path, remove the small value path
    $index = $top;
    $father = $index;
    $needInsert = 1;
    while ($index != -1) {
	if ($position{$index} == $position{$nextid}) {
	    if ($value{$index} >= $value{$nextid}) {
		$needInsert = 0;
	    } else {
		if ($top == $index) {
		    $next{$nextid} = $next{$index};
		    $top = $nextid;
		    $needInsert = 0;
		} else {
		    $next{$father} = $next{$index};
		}
	    }
	    $index = -1;
	} else {
	    $father = $index;
	    $index = $next{$index};
	}
	
    }
    
    
    # insert the new path into the list
    if ($needInsert == 1) {
	$index = $top;
	while (($index != -1) && ($value{$index} > $value{$nextid})) {
	    $father = $index;
	    $index = $next{$index};
	}
	if ($top == $index) {
	    $next{$nextid} = $top;
	    $top = $nextid;
	} else {
	    $next{$father} = $nextid;
	    $next{$nextid} = $index;
	}
    }				# 
    
    $nextid++;

}

sub read_dict {
    open F,"<$dictfile" || die "Dictonary file $dictfile not found";
    while (<F>) {
	chomp;
	s/^ *//;
	split;
	$freq{$_[1]}  = $_[0];
	$header = substr($_[1],0,2);
	if ($freq{"m,$header"}) {
	    if ($freq{"m,$header"} < length($_[1])) {
		$freq{"m,$header"} = length($_[1]);
	    }
	} else {
	    $freq{"m,$header"} = length($_[1]);
	}
	$freq{total} += $_[0];
    }
    close(F);
}

