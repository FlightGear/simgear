# Microsoft Developer Studio Project File - Name="Metar" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Metar - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Metar.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Metar.mak" CFG="Metar - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Metar - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Metar - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Metar - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /GX /Zi /O2 /I "..\..\..\lib" /I "..\lib" /I "..\..\lib" /I "..\..\..\..\lib" /I ".." /I "..\.." /I "..\..\.." /I "..\..\..\.." /I "..\include" /I "..\..\include" /I "..\..\..\include" /I "..\..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Metar - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /W3 /Gm /GX /Zi /Od /I "..\..\..\lib" /I "..\lib" /I "..\..\lib" /I "..\..\..\..\lib" /I ".." /I "..\.." /I "..\..\.." /I "..\..\..\.." /I "..\include" /I "..\..\include" /I "..\..\..\include" /I "..\..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "Metar - Win32 Release"
# Name "Metar - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Antoi.cpp
# End Source File
# Begin Source File

SOURCE=.\Charcmp.cpp
# End Source File
# Begin Source File

SOURCE=.\Dcdmetar.cpp
# End Source File
# Begin Source File

SOURCE=.\Dcdmtrmk.cpp
# End Source File
# Begin Source File

SOURCE=.\Fracpart.cpp
# End Source File
# Begin Source File

SOURCE=.\MetarReport.cpp
# End Source File
# Begin Source File

SOURCE=.\MetarStation.cpp
# End Source File
# Begin Source File

SOURCE=.\Prtdmetr.cpp
# End Source File
# Begin Source File

SOURCE=.\Stspack2.cpp
# End Source File
# Begin Source File

SOURCE=.\Stspack3.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\LOCAL.H
# End Source File
# Begin Source File

SOURCE=.\METAR.H
# End Source File
# Begin Source File

SOURCE=.\MetarReport.h
# End Source File
# Begin Source File

SOURCE=.\MetarStation.h
# End Source File
# End Group
# End Target
# End Project
