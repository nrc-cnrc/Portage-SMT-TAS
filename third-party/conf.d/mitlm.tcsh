############ MITLM ############
if (! $?MITLM_HOME_OVERRIDE) then
   set MITLM_HOME=$PORTAGE/third-party/mitlm
else
   set MITLM_HOME=$MITLM_HOME_OVERRIDE
endif

if ( -d $MITLM_HOME ) then
   setenv PATH $MITLM_HOME/bin:$PATH
   setenv LIBRARY_PATH $MITLM_HOME/lib:$LIBRARY_PATH
endif

unset MITLM_HOME
