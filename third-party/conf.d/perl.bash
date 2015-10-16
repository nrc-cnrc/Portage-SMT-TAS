############ Perl ############
# Set PERL_HOME_OVERRIDE to override where Perl is installed

if [[ $PERL_HOME_OVERRIDE ]]; then
   PERL_HOME=$PERL_HOME_OVERRIDE
else
   PERL_HOME=`ls -d $PORTAGE/third-party/perl-5* 2> /dev/null | tail -1`
fi

if [[ -d $PERL_HOME ]]; then
   export PATH=$PERL_HOME/bin:$PATH
   export MANPATH=$PERL_HOME/share/man:$MANPATH
elif perl -e 'require 5.010'; then
   true
else
   echo "Error: PortageII requires Perl version 5.10 or more recent (>= 5.14 for Arabic)."
fi

unset PERL_HOME
