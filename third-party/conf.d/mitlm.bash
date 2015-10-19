############ MITLM ############
# Set MITLM_HOME_OVERRIDE to override where MITLM is installed
MITLM_HOME=${MITLM_HOME_OVERRIDE:-$PORTAGE/third-party/mitlm}

if [[ -d $MITLM_HOME ]]; then
   export PATH=$MITLM_HOME/bin:$PATH
   export LD_LIBRARY_PATH=$MITLM_HOME/lib:$LD_LIBRARY_PATH
fi

unset MITLM_HOME
