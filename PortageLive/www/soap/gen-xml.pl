#!/usr/bin/env perl

my $indoc = 0;
my $wrote_a_docline = 0;

while (<>) {
    if (/<documentation>/) { $indoc = 1; $wrote_a_docline = 0; }
    if (/<\/documentation>/) { $indoc = 0; }
    print;
    if ($indoc) {
        print "&lt;br/>" if $wrote_a_docline;
        $wrote_a_docline = 1;
    }
}
