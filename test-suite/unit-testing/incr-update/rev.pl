#!/bin/env perl
binmode(STDIN,":encoding(UTF-8)");
binmode(STDOUT,":encoding(UTF-8)");

while (<>) {
   chomp;
   print scalar reverse $_;
   print "\n";
}
