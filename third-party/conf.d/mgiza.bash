############ MGIZA ############
# Set MGIZA_HOME_OVERRIDE to override where MGIZA is installed
MGIZA_HOME=${MGIZA_HOME_OVERRIDE:-$PORTAGE/third-party/mgiza}

if [[ -d $MGIZA_HOME ]]; then
   export PATH=$MGIZA_HOME:$PATH
fi

unset MGIZA_HOME
