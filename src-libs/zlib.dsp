# Microsoft Developer Studio Project File - Name="zlib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=zlib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "zlib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "zlib.mak" CFG="zlib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "zlib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "zlib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "zlib - Win32 Release"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "zlib-1.1.4/Release"
# PROP Intermediate_Dir "zlib-1.1.4/Release"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD CPP /nologo /MT /W3 /GX- /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "zlib - Win32 Debug"

# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "zlib-1.1.4/Debug"
# PROP Intermediate_Dir "zlib-1.1.4/Debug"
# PROP Target_Dir ""
LINK32=link.exe -lib
# ADD CPP /nologo /MTd /W3 /Gm /GX- /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ  /c
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "zlib - Win32 Release"
# Name "zlib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=".\zlib-1.1.4\adler32.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\compress.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\crc32.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\deflate.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\gzio.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\infblock.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\infcodes.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\inffast.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\inflate.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\inftrees.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\infutil.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\maketree.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\trees.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\uncompr.c"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\zutil.c"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=".\zlib-1.1.4\deflate.h"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\infblock.h"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\infcodes.h"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\inffast.h"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\inffixed.h"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\inftrees.h"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\infutil.h"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\trees.h"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\zconf.h"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\zlib.h"
# End Source File
# Begin Source File

SOURCE=".\zlib-1.1.4\zutil.h"
# End Source File
# End Group
# End Target
# End Project
