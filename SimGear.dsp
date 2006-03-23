# Microsoft Developer Studio Project File - Name="SimGear" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=SimGear - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SimGear.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SimGear.mak" CFG="SimGear - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SimGear - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "SimGear - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_MBCS" /FD /c  /MT /I "." /I ".." /I ".\SimGear" /I "..\zlib-1.2.3" /I "..\OpenAL 1.0 Software Development Kit\include" /D "_USE_MATH_DEFINES" /D "_CRT_SECURE_NO_DEPRECATE" /D "HAVE_CONFIG_H"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD CPP /nologo /W3 /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_MBCS" /FR /FD /GZ /c  /MTd /I "." /I ".." /I ".\SimGear" /I "..\zlib-1.2.3" /I "..\OpenAL 1.0 Software Development Kit\include" /D "_USE_MATH_DEFINES" /D "_CRT_SECURE_NO_DEPRECATE" /D "HAVE_CONFIG_H"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "SimGear - Win32 Release"
# Name "SimGear - Win32 Debug"
# Begin Group "Lib_sgbucket"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\bucket\newbucket.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgbucket"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgbucket"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgdebug"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\debug\logstream.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgdebug"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgdebug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgephem"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\ephemeris\celestialBody.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\ephemeris.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\jupiter.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\mars.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\mercury.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\moonpos.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\neptune.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\pluto.hxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\saturn.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\star.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\stardata.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\uranus.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\ephemeris\venus.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgephem"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgephem"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\io\iochannel.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgio"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgio"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\io\lowlevel.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgio"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgio"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\io\sg_binobj.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgio"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgio"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\io\sg_file.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgio"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgio"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\io\sg_serial.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgio"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgio"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\io\sg_socket.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgio"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgio"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\io\sg_socket_udp.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgio"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgio"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgmagvar"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\magvar\coremag.hxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmagvar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmagvar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\magvar\coremag.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmagvar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmagvar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\magvar\magvar.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmagvar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmagvar"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgmath"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\math\interpolater.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmath"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmath"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\math\leastsqs.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmath"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmath"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\math\polar3d.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmath"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmath"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\math\sg_geodesy.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmath"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmath"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\math\sg_random.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmath"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmath"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\math\vector.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmath"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmath"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\math\fastmath.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmath"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmath"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\math\SGGeodesy.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmath"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmath"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgenvironment"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\environment\metar.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgenvironment"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgenvironment"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\environment\visual_enviro.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgenvironment"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgenvironment"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgmisc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\misc\sg_path.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmisc"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmisc"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\misc\sgstream.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmisc"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmisc"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\misc\strutils.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmisc"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmisc"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\misc\tabbed_values.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmisc"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmisc"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\misc\texcoord.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmisc"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmisc"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\misc\zfstream.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmisc"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmisc"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\misc\interpolator.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmisc"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmisc"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgnasal"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\nasal\code.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\code.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\codegen.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\data.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\gc.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\hash.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\lex.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\lib.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\mathlib.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\iolib.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\iolib.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\bitslib.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\misc.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\nasal.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\parse.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\parse.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\string.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\vector.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\thread-posix.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\nasal\thread-win32.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgnasal"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgnasal"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgprops"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\props\condition.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgprops"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgprops"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\props\props.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgprops"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgprops"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\props\props_io.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgprops"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgprops"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgroute"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\route\route.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgroute"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgroute"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\route\waypoint.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgroute"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgroute"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgmaterial"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\scene\material\mat.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmaterial"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmaterial"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\material\matlib.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmaterial"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmaterial"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\material\matmodel.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmaterial"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmaterial"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgmodel"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\scene\model\animation.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmodel"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmodel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\model\custtrans.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmodel"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmodel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\model\location.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmodel"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmodel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\model\model.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmodel"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmodel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\model\modellib.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmodel"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmodel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\model\personality.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmodel"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmodel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\model\placement.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmodel"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmodel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\model\placementtrans.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmodel"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmodel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\model\shadowvolume.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmodel"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmodel"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\model\shadanim.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmodel"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmodel"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgsky"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\scene\sky\cloud.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\sky\dome.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\sky\moon.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\sky\oursun.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\sky\sky.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\sky\sphere.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\sky\stars.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\sky\bbcache.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\sky\cloudfield.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\sky\newcloud.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgtgdb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\scene\tgdb\apt_signs.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtgdb"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtgdb"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\tgdb\leaf.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtgdb"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtgdb"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\tgdb\obj.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtgdb"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtgdb"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\tgdb\pt_lights.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtgdb"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtgdb"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\scene\tgdb\userdata.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtgdb"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtgdb"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgscreen"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\screen\texture.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgscreen"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgscreen"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\screen\GLBitmaps.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgscreen"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgscreen"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\screen\screen-dump.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgscreen"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgscreen"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\screen\tr.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgscreen"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgscreen"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\screen\extensions.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgscreen"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgscreen"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\screen\RenderTexture.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgscreen"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgscreen"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\screen\shader.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgscreen"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgscreen"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\screen\win32-printer.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgscreen"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgscreen"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgserial"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\serial\serial.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgserial"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgserial"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgsound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\sound\sample_openal.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsound"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsound"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\sound\soundmgr_openal.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsound"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsound"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\sound\xmlsound.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsound"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsound"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgstructure"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\structure\commands.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgstructure"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgstructure"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\structure\exception.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgstructure"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgstructure"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\structure\event_mgr.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgstructure"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgstructure"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\structure\subsystem_mgr.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgstructure"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgstructure"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgtiming"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\timing\geocoord.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\lowleveltime.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\sg_time.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\timestamp.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\timezone.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgxml"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\xml\asciitab.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\easyxml.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\hashtable.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\hashtable.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\iasciitab.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\latin1tab.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\nametab.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\utf8tab.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\xmldef.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\xmlparse.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\xmlparse.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\xmlrole.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\xmlrole.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\xmltok.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\xmltok.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xml\xmltok_impl.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxml"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxml"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\simgear\simgear_config.h.vc5

!IF  "$(CFG)" == "SimGear - Win32 Release"

# Begin Custom Build - Creating config.h
InputPath=.\simgear\simgear_config.h.vc5

".\simgear\simgear_config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
        copy .\simgear\simgear_config.h.vc5 .\simgear\simgear_config.h

# End Custom Build

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# Begin Custom Build - Creating config.h
InputPath=.\simgear\simgear_config.h.vc5

".\simgear\simgear_config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
        copy .\simgear\simgear_config.h.vc5 .\simgear\simgear_config.h

# End Custom Build

!ENDIF

# End Source File
# End Target
# End Project
