############ Perl ############
# Set PERL_HOME_OVERRIDE to override where Perl is installed

if (! $?PERL_HOME_OVERRIDE) then
   set PERL_HOME=`ls -d $PORTAGE/third-party/perl-5* |& tail -1`
else
   set PERL_HOME=$PERL_HOME_OVERRIDE
endif

if ( -d "$PERL_HOME" ) then
   setenv PATH $PERL_HOME/bin:$PATH
   setenv MANPATH $PERL_HOME/share/man:$MANPATH
else if ( { perl -e 'require 5.010' } ) then
   true
else
   echo "Error: PortageII requires Perl version 5.10 or more recent (>= 5.14 for Arabic)."
endif

unset PERL_HOME
