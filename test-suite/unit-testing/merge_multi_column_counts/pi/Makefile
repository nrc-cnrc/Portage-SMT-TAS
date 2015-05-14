#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

MERGE := merge_multi_column_counts

-include Makefile.params

.DEFAULT_GOAL := all
.PHONY: all
all:  pi


.PHONY: clean
clean:


.PHONY:  pi
pi:  data/a data/b
	${MERGE} -PI - $+ | diff - ref/merged_pi -q
