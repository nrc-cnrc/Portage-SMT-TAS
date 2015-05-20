#!/usr/bin/env perl

while (<>) {
   print "SENT $.\n";
   s/([][;])/ \1\n/g;
   s/:/ : /g;
   print;
}
