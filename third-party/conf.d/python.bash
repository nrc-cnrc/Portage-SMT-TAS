############ PYTHON ############
# Set PYTHON_HOME_OVERRIDE to override where Python is installed
if [[ $PYTHON_HOME_OVERRIDE ]]; then
   PYTHON_HOME=$PYTHON_HOME_OVERRIDE
elif [[ -d $PORTAGE/third-party/miniconda2 ]]; then
   PYTHON_HOME=$PORTAGE/third-party/miniconda2
else
   PYTHON_HOME=$PORTAGE/third-party/python2.7
fi

if [[ -d $PYTHON_HOME ]]; then
   export PYTHON=${PYTHON_HOME}/bin/python
   export PATH=$PYTHON_HOME/bin:$PATH
   export LIBRARY_PATH=$PYTHON_HOME/lib${LIBRARY_PATH:+:$LIBRARY_PATH}
   export LD_LIBRARY_PATH=$PYTHON_HOME/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
   export LD_RUN_PATH=$PYTHON_HOME/lib${LD_RUN_PATH:+:$LD_RUN_PATH}
   export CPLUS_INCLUDE_PATH=$PYTHON_HOME/include${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}
   export MANPATH=$PYTHON_HOME/share/man:$MANPATH
   #export PYTHONPATH=${PYTHONPATH:+$PYTHONPATH:}$PYTHON_HOME/lib/python2.7/site-packages
elif python --version 2>&1 | grep -q '2\.7'; then
   true
else
   echo Error: PortageII requires Python version 2.7
fi

unset PYTHON_HOME
