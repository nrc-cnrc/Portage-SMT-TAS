############ JAVA ############
# Set this JAVA_HOME_OVERRIDE to override where Java is installed
JAVA_HOME=${JAVA_HOME_OVERRIDE:-$PORTAGE/third-party/jdk-1.7}

if [[ -d $JAVA_HOME ]]; then
   export PATH=$JAVA_HOME/bin:$PATH
   export MANPATH=$JAVA_HOME/man:$MANPATH
   export JAVA_HOME
elif java -version 2>&1 | egrep -q 'java version "1\.([6789]|\d\d)'; then
   true
else
   echo Error: PortageII requires Java version 1.6 or more recent
fi


