#!/usr/bin/make -f
# vim:noet:ts=3:nowrap:filetype=make

SHELL = bash

-include Makefile.params

.DEFAULT_GOAL := all
.PHONY: all
all:  testsuite


.PHONY: clean
clean:


.PHONY:  testsuite


# Let's see if we can call our test program's help message via -h.
.PHONY:  help_short
testsuite:  help_short
help_short:
	[[ `./perl_interpreter.pl -h` == "Help Message" ]] \
	|| ! echo "Error: We are expecting a help message." >&2



# Let's see if we can call our test program's help message via -help.
.PHONY:  help_long
testsuite:  help_long
help_long:
	[[ `./perl_interpreter.pl -help` == "Help Message" ]] \
	|| ! echo "Error: We are expecting a help message." >&2



# Let's make sure that our test script invokes the user's Perl and not the
# default Perl interpreter.
.PHONY:  interpreter_default
testsuite:  interpreter_default
interpreter_default:
	[[ `which perl` == '/usr/bin/perl' ]] \
	|| [[ `./perl_interpreter.pl` != `/usr/bin/perl -e 'print "$$^V\n"'` ]] \
	|| ! echo "Error: We shouldn't be using the default perl." >&2



# Let's see if we are invoking the same interpreter as the user has defined in
# his environment.
.PHONY:  interpreter_user_defined
testsuite:  interpreter_user_defined
interpreter_user_defined:
	[[ `which perl` == '/usr/bin/perl' ]] \
	|| [[ `./perl_interpreter.pl` == `perl -e 'print "$$^V\n"'` ]] \
	|| ! echo "Error: We should be invoking the user defined Perl interpreter." >&2
