#!/usr/bin/env perl

while (<STDIN>) {

  s/^canoe starting on .+//go;
  s/^Done in [0-9]+s//go;
  s/(\.\.\. kept: [0-9]+ in )[0-9]+s./$1/go;
  s/^LM loading completed.+//go;
  s/^Loaded data structures.+//go;
  s/^Translated 10 sentences.+//go;
  s/^(createModel|runDecoder|doOutput).+//go;
  s/^canoe done on.+//go;
  s/^[.\\_\/]{10}$/........../go;

  print;
}
