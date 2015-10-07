############ PYTHON ############
# Set this variable to override where Python is installed
export PYTHON_HOME=${PYTHON_HOME_OVERRIDE:-$PORTAGE/third-party/Python2.7}
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
