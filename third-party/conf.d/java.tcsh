############ JAVA ############
# Set this JAVA_HOME_OVERRIDE to override where Java is installed
if (! $?JAVA_HOME_OVERRIDE) then
   set _JAVA_HOME=$PORTAGE/third-party/jdk-1.7
else
   set _JAVA_HOME=$JAVA_HOME_OVERRIDE
endif

if ( -d $_JAVA_HOME ) then
   setenv PATH $_JAVA_HOME/bin:$PATH
   setenv MANPATH $_JAVA_HOME/share/man:$MANPATH
   setenv JAVA_HOME $_JAVA_HOME
else
   java -version |& egrep -q 'java version "1\.([6789]|\d\d)' || \
      echo Error: PortageII requires Java version 1.6 or more recent
endif

unset _JAVA_HOME
