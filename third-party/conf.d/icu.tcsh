############ ICU ############
if (! $?ICU_HOME_OVERRIDE) then
   set ICU_HOME=$PORTAGE/third-party/icu
else
   set ICU_HOME=$ICU_HOME_OVERRIDE
endif

if ( -d $ICU_HOME ) then
   setenv ICU $ICU_HOME
   setenv PATH $ICU_HOME/bin:$PATH
   setenv MANPATH $ICU_HOME/share/man:$MANPATH

   if (! $?LIBRARY_PATH) then
      setenv LIBRARY_PATH $ICU_HOME/lib
   else
      setenv LIBRARY_PATH $ICU_HOME/lib:$LIBRARY_PATH
   endif

   if (! $?LD_LIBRARY_PATH) then
      setenv LD_LIBRARY_PATH $ICU_HOME/lib
   else
      setenv LD_LIBRARY_PATH $ICU_HOME/lib:$LD_LIBRARY_PATH
   endif

   if (! $?LD_RUN_PATH) then
      setenv LD_RUN_PATH $ICU_HOME/lib
   else
      setenv LD_RUN_PATH $ICU_HOME/lib:$LD_RUN_PATH
   endif

   if (! $?CPLUS_INCLUDE_PATH) then
      setenv CPLUS_INCLUDE_PATH $ICU_HOME/include
   else
      setenv CPLUS_INCLUDE_PATH $ICU_HOME/include:$CPLUS_INCLUDE_PATH
   endif
endif

unset ICU_HOME
