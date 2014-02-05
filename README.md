TempestSDR
=============

This project is in its infancy and is not operational! Please come back in Q2 of 2014 for more information about what it is and how to use :)
Keep in mind the project will be licensed under GNU GPL!!! So consider any of the binaries and source under the GNU GPL license until a formal notification is added.

Release
------------

 * [JTempestSDR.jar](https://raw.github.com/martinmarinov/TempestSDR/master/Release/JavaGUI/JTempestSDR.jar) is the self contained multi platform GUI executable. It should work with just a double click on most Windows/Linux x86/x64 based machines.
 * [Download dlls] (https://github.com/martinmarinov/TempestSDR/tree/master/Release/dlls) contains the precompiled dll files for Linux/Windows x86/x64 which you can use in your own project under the GNU GPL license.


Folder Structure
------------

The different folders contain the different subprojects. The TempestSDR folder contains the main C library. The project aims to be crossplatform with plugin support for SDR frontends in different folders.

Requirements for Building
------------

The project is built with Eclipse with the CDT plugin (but this is not required). Currently it supports Windows and Linux. Some frontend plugins might not be crossplatform. You also need a Java Development Kit (JDK) installed.

### Windows

You need to have MinGW installed and gcc and make commands need to be in your path. Also javac and javah also need to be in your path.

### Linux

To be announced soon.

Building
------------

All project could be built both with Eclipse and make as well.

### TempestSDR library

Enter the folder and type

    make all
	
This will produce the library which could be found in the bin subdir. The headers you need to interface with it are located in src/include.

### Plugins

Go into a plugin directory and type

    make all
	
This should work unless there is something specific for the plugin itself. Look for a README in this case.

### JavaGUI

This is the Java wrapper and GUI. It builds all projects and supported plugins including the Java GUI itself. Go into the JavaGUI folder and type in

    make all JAVA_HOME=path_to_jdk_installation

On Windows 8 x64 this could look like

    make all JAVA_HOME=C:/PROGRA~2/Java/jdk1.7.0_45
	
To force compilation for X64 or X32 (in case your compiler supports it), do the following

    make all JAVA_HOME=C:/PROGRA~2/Java/jdk1.7.0_45 ARCHNAME=X64

On Ubuntu with openjdk it could look like

    make all JAVA_HOME=/usr/lib/jvm/java-6-openjdk-amd64
	
### Mirics Plugin

This is a driver for the FlexiTV™ MSi3101 SDR USB Dongle for Windows. You first need to install the SDR driver from http://www.mirics.com/

In order to compile the plugin go to the plugin folder and type in

    make all MIRICS_HOME=path_to_mirics_installation
	
On Windows 8 x64 this could look like

    make all MIRICS_HOME=C:/PROGRA~1/MiricsSDR
	
If running Mirics Plugin, make sure the mir_sdr_api.dll is in the library path (or in the same directory as the executable).
You can find the dll in C:/Program Files/MiricsSDR/API/x86 or C:/Program Files/MiricsSDR/API/x64 depending on your architecture.
