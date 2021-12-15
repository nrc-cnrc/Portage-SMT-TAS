############ MGIZA ############
if (! $?MGIZA_HOME_OVERRIDE) then
   set MGIZA_HOME=$PORTAGE/third-party/mgiza
else
   set MGIZA_HOME=$MGIZA_HOME_OVERRIDE
endif

if ( -d $MGIZA_HOME ) then
   setenv PATH $MGIZA_HOME/bin:$PATH
endif

unset MGIZA_HOME
