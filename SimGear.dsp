# Microsoft Developer Studio Project File - Name="SimGear" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

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
!MESSAGE "SimGear - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "SimGear - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\src" /I ".\src\Include" /I "\usr\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\src" /I ".\src\Include" /I "\usr\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib winspool.lib comdlg32.lib gdi32.lib shell32.lib glut32.lib wsock32.lib simgear.lib fnt.lib pui.lib sg.lib sl.lib ssg.lib mk4vc60_d.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"\lib" /libpath:"\lib\simgear" /libpath:"\lib\plib"

!ENDIF 

# Begin Target

# Name "SimGear - Win32 Release"
# Name "SimGear - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
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
# End Group
# Begin Group "Lib_sgmetar"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\metar\Antoi.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\Charcmp.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\Dcdmetar.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\Dcdmtrmk.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\Drvmetar.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\Fracpart.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\Local.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\Metar.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\MetarReport.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\MetarStation.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\Prtdmetr.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\Stspack2.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\metar\Stspack3.cpp

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmetar"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmetar"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgmisc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\misc\props.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmisc"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmisc"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\misc\props_io.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgmisc"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgmisc"

!ENDIF 

# End Source File
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
# Begin Group "Lib_sgscreen"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\screen\GLBitmaps.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgscreen"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgscreen"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\screen\GLBitmaps.h

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
# Begin Group "Lib_sgsky"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\sky\cloud.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\sky\dome.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\sky\moon.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\sky\oursun.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\sky\sky.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\sky\sphere.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\sky\stars.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgsky"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgsky"

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

SOURCE=.\simgear\timing\timezone.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\#

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\event.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\event.hxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\#

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\fg_timer.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\fg_timer.hxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\#

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\light.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\light.hxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\#

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\moonpos.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\moonpos.hxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\#

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\sunpos.cxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\sunpos.hxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\#

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\timing\timestamp.hxx

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgtiming"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgtiming"

!ENDIF 

# End Source File
# End Group
# Begin Group "Lib_sgxgl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\xgl\xgl.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxgl"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxgl"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\xgl\xglUtils.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_sgxgl"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_sgxgl"

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
# Begin Group "Lib_z"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\simgear\zlib\adler32.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\compress.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\crc32.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\deflate.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\deflate.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\gzio.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\infblock.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\infblock.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\infcodes.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\infcodes.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\inffast.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\inffast.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\inffixed.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\inflate.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\inftrees.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\inftrees.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\infutil.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\infutil.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\trees.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\trees.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\uncompr.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\zutil.c

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\simgear\zlib\zutil.h

!IF  "$(CFG)" == "SimGear - Win32 Release"

# PROP Intermediate_Dir "Release\Lib_z"

!ELSEIF  "$(CFG)" == "SimGear - Win32 Debug"

# PROP Intermediate_Dir "Debug\Lib_z"

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
