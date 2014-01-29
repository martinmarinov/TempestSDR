#!/bin/bash
# Define Windows mappings
C=

# Define Mirics home
MIRICS_HOME=$C\PROGRA~1\MiricsSDR

# For each cross compiler specify platform (LINUX or WINDOWS) and archtype (X86 or X64)
CCs=(ednoCC dveCC triCC)
OSNAMEs=(ednoOS, dveOS, triOS)
ARCHNAMEs=(ednoAR, dveAR, triAR)
JAVA_HOMEs=(ednoJH, dveJH, triJH)

make clean
make java

for i in "${!CCs[@]}"; do 
	CC=${CCs[$i]}
	OSNAME=${OSNAMEs[$i]}
	ARCHNAME=${ARCHNAMEs[$i]}
	JAVA_HOME=${JAVA_HOMEs[$i]}
	make all CC=$CC OSNAME=$OSNAME ARCHNAME=$ARCHNAME JAVA_HOME=$JAVA_HOME MIRICS_HOME=C:/PROGRA~1/MiricsSDR
	make cleandependent CC=$CC OSNAME=$OSNAME ARCHNAME=$ARCHNAME JAVA_HOME=$JAVA_HOME
done

make jar
