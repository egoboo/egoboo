# Microsoft Developer Studio Project File - Name="egoboo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=egoboo - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "egoboo_vs6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "egoboo_vs6.mak" CFG="egoboo - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "egoboo - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "egoboo - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "egoboo - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"Release/egoboo-2.x.exe"

!ELSEIF  "$(CFG)" == "egoboo - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\enet\include" /I "C:\SDKs\SDL\include" /I "C:\SDKs\SDL_mixer" /I "C:\SDKs\SDL_image" /I "C:\SDKs\SDL_ttf" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/egoboo-2.x.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "egoboo - Win32 Release"
# Name "egoboo - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\camera.c
# End Source File
# Begin Source File

SOURCE=.\char.c
# End Source File
# Begin Source File

SOURCE=.\client.c
# End Source File
# Begin Source File

SOURCE=.\clock.c
# End Source File
# Begin Source File

SOURCE=.\configfile.c
# End Source File
# Begin Source File

SOURCE=.\egoboo_endian.c
# End Source File
# Begin Source File

SOURCE=.\egoboo_math.c
# End Source File
# Begin Source File

SOURCE=.\egoboo_setup.c
# End Source File
# Begin Source File

SOURCE=.\egoboo_strutil.c
# End Source File
# Begin Source File

SOURCE=.\enchant.c
# End Source File
# Begin Source File

SOURCE=.\file_common.c
# End Source File
# Begin Source File

SOURCE=.\file_linux.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\file_win.c
# End Source File
# Begin Source File

SOURCE=.\font.c
# End Source File
# Begin Source File

SOURCE=.\game.c
# End Source File
# Begin Source File

SOURCE=.\gltexture.c
# End Source File
# Begin Source File

SOURCE=.\graphic.c
# End Source File
# Begin Source File

SOURCE=.\graphic_fan.c
# End Source File
# Begin Source File

SOURCE=.\graphic_mad.c
# End Source File
# Begin Source File

SOURCE=.\graphic_prt.c
# End Source File
# Begin Source File

SOURCE=.\id_normals.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\input.c
# End Source File
# Begin Source File

SOURCE=.\link.c
# End Source File
# Begin Source File

SOURCE=.\log.c
# End Source File
# Begin Source File

SOURCE=.\Md2.c
# End Source File
# Begin Source File

SOURCE=.\menu.c
# End Source File
# Begin Source File

SOURCE=.\module.c
# End Source File
# Begin Source File

SOURCE=.\network.c
# End Source File
# Begin Source File

SOURCE=.\particle.c
# End Source File
# Begin Source File

SOURCE=.\passage.c
# End Source File
# Begin Source File

SOURCE=.\script.c
# End Source File
# Begin Source File

SOURCE=.\server.c
# End Source File
# Begin Source File

SOURCE=.\sound.c
# End Source File
# Begin Source File

SOURCE=.\sys_linux.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\sys_mac.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\sys_win32.c
# End Source File
# Begin Source File

SOURCE=.\ui.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\camera.h
# End Source File
# Begin Source File

SOURCE=.\char.h
# End Source File
# Begin Source File

SOURCE=.\client.h
# End Source File
# Begin Source File

SOURCE=.\clock.h
# End Source File
# Begin Source File

SOURCE=.\configfile.h
# End Source File
# Begin Source File

SOURCE=.\egoboo.h
# End Source File
# Begin Source File

SOURCE=.\egoboo_config.h
# End Source File
# Begin Source File

SOURCE=.\egoboo_endian.h
# End Source File
# Begin Source File

SOURCE=.\egoboo_math.h
# End Source File
# Begin Source File

SOURCE=.\egoboo_setup.h
# End Source File
# Begin Source File

SOURCE=.\egoboo_strutil.h
# End Source File
# Begin Source File

SOURCE=.\egoboo_typedef.h
# End Source File
# Begin Source File

SOURCE=.\enchant.h
# End Source File
# Begin Source File

SOURCE=.\font.h
# End Source File
# Begin Source File

SOURCE=.\gltexture.h
# End Source File
# Begin Source File

SOURCE=.\graphic.h
# End Source File
# Begin Source File

SOURCE=.\id_md2.h
# End Source File
# Begin Source File

SOURCE=.\input.h
# End Source File
# Begin Source File

SOURCE=.\link.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=.\Md2.h
# End Source File
# Begin Source File

SOURCE=.\menu.h
# End Source File
# Begin Source File

SOURCE=.\particle.h
# End Source File
# Begin Source File

SOURCE=.\passage.h
# End Source File
# Begin Source File

SOURCE=.\proto.h
# End Source File
# Begin Source File

SOURCE=.\script.h
# End Source File
# Begin Source File

SOURCE=.\server.h
# End Source File
# Begin Source File

SOURCE=.\sound.h
# End Source File
# Begin Source File

SOURCE=.\System.h
# End Source File
# Begin Source File

SOURCE=.\ui.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\egoboo1.ico
# End Source File
# Begin Source File

SOURCE=..\egoboo2.ico
# End Source File
# End Group
# End Target
# End Project
