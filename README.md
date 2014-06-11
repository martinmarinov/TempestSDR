TempestSDR
=============

This project is a software toolkit for remotely eavesdropping video monitors using a Software Defined Radio (SDR) receiver. It exploits compromising emanations from cables carrying video signals.

Raster video is usually transmitted one line of pixels at a time, encoded as a varying current. This generates an electromagnetic wave that can be picked up by an SDR receiver. The software maps the received field strength of a pixel to a gray-scale shade in real-time. This forms a false colour estimate of the original video signal.

The toolkit uses unmodified off-the-shelf hardware which lowers the costs and increases mobility compared to existing solutions. It allows for additional post-processing which improves the signal-to-noise ratio. The attacker does not need to have prior knowledge about the target video display. All parameters such as resolution and refresh rate are estimated with the aid of the software. 

The software consists of a library written in C, a collection of plug-ins for various Software Define Radio (SDR) front-ends and a Java based Graphical User Interface (GUI). It is a multi-platform application, with all native libraries pre-compiled and packed into a single Java jar file.

Release
------------

 * [JTempestSDR.jar](https://raw.github.com/martinmarinov/TempestSDR/master/Release/JavaGUI/JTempestSDR.jar) is the self contained multi platform GUI executable. It should work with just a double click on most Windows/Ubuntu x86/x64 based machines.
 * [Download dlls] (https://github.com/martinmarinov/TempestSDR/tree/master/Release/dlls) contains the precompiled dll files for Linux/Windows x86/x64 which you can use in your own project under the GNU GPL license.

Double click on the JTempestSDR.jar file to launch. If it fails to launch, you will need to recompile the project.


Building the executable
------------

This is the Java wrapper and GUI. It builds all projects and supported plugins including the Java GUI itself. Go into the JavaGUI folder and type in

    make all

If it fails to find "jni.h", you should run one of the following commands:

    make all JAVA_HOME=path_to_jdk_installation

On Windows 8 x64 this could look like

    make all JAVA_HOME=C:/PROGRA~2/Java/jdk1.7.0_45
	
To force compilation for X64 or X32 (in case your compiler supports it), do the following

    make all JAVA_HOME=C:/PROGRA~2/Java/jdk1.7.0_45 ARCHNAME=X64

On Ubuntu with openjdk it could look like

    make all JAVA_HOME=/usr/lib/jvm/java-6-openjdk-amd64

Note: This will also compile the plugins. Some of them require additional libraries! You can disable the plugin compilation by editing the Makefile in JavaSDR.

### Windows

You need to have MinGW installed.

On Windows, this will also build the Mirics SDR plugin for the FlexiTV™ MSi3101 SDR USB Dongle. You first need to install the SDR driver from http://www.mirics.com/

In order to compile the plugin go to the plugin folder and type in

    make all MIRICS_HOME=path_to_mirics_installation
	
On Windows 8 x64 this could look like

    make all MIRICS_HOME=C:/PROGRA~1/MiricsSDR
	
If running Mirics Plugin, make sure the mir_sdr_api.dll is in the library path (or in the same directory as the executable).
You can find the dll in C:/Program Files/MiricsSDR/API/x86 or C:/Program Files/MiricsSDR/API/x64 depending on your architecture. 

If you don't intend to use the Mirics dongle, you can skip this step by editing the Makefile in the JavaGUI directory. Remove TSDRPlugin\_Mirics from line 89, changing it from

    PLUGINS += TSDRPlugin_Mirics TSDRPlugin_ExtIO

to

    PLUGINS += TSDRPlugin_ExtIO

### Linux and OS X

On Linux and OS X, compiling the GUI will also compile the UHD driver, so you will need to have UHD and the corresponding boost libraries installed (UHD will install them automatically). If you don't want the UHD drivers, then you can skip their compilation by removing the line 91 for Linux and line 93 for OS X from the Makefile in the JavaGUI directory. This is how those lines look like by default:

    PLUGINS += TSDRPlugin_UHD


Building the libraries
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

Folder Structure
------------

The different folders contain the different subprojects. The TempestSDR folder contains the main C library. The project aims to be crossplatform with plugin support for SDR frontends in different folders.

Requirements for Building
------------

The project is built with Eclipse with the CDT plugin (but this is not required). Currently it supports Windows, Linux and OS X. Some frontend plugins might not be crossplatform. You also need a Java Development Kit (JDK) installed.

### Windows

You need to have MinGW installed and gcc and make commands need to be in your path. Also javac and javah also need to be in your path.

### Linux

To be announced soon.


