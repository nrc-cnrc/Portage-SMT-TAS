############ PYTHON ############
if (! $?PYTHON_HOME_OVERRIDE) then
   if ( -d $PORTAGE/third-party/miniconda2 ) then
      set PYTHON_HOME=$PORTAGE/third-party/miniconda2
   else
      set PYTHON_HOME=$PORTAGE/third-party/python-2.7
   endif
else
   set PYTHON_HOME=$PYTHON_HOME_OVERRIDE
endif

if ( -d $PYTHON_HOME ) then
   setenv PYTHON $PYTHON_HOME/bin/python
   setenv PATH $PYTHON_HOME/bin:$PATH
   setenv MANPATH $PYTHON_HOME/share/man:$MANPATH

   if (! $?LIBRARY_PATH) then
      setenv LIBRARY_PATH $PYTHON_HOME/lib
   else
      setenv LIBRARY_PATH $PYTHON_HOME/lib:$LIBRARY_PATH
   endif

   if (! $?LD_LIBRARY_PATH) then
      setenv LD_LIBRARY_PATH $PYTHON_HOME/lib
   else
      setenv LD_LIBRARY_PATH $PYTHON_HOME/lib:$LD_LIBRARY_PATH
   endif

   if (! $?LD_RUN_PATH) then
      setenv LD_RUN_PATH $PYTHON_HOME/lib
   else
      setenv LD_RUN_PATH $PYTHON_HOME/lib:$LD_RUN_PATH
   endif

   if (! $?CPLUS_INCLUDE_PATH) then
      setenv CPLUS_INCLUDE_PATH $PYTHON_HOME/include
   else
      setenv CPLUS_INCLUDE_PATH $PYTHON_HOME/include:$CPLUS_INCLUDE_PATH
   endif
else
   python2 --version |& grep -q '2\.7' || \
      echo Error: PortageII requires Python version 2.7
endif

unset PYTHON_HOME
