@echo off
setlocal
set "INTDIR=%~1"
:collect_intdir
shift
if "%~1"=="" goto intdir_ready
set "INTDIR=%INTDIR% %~1"
goto collect_intdir
:intdir_ready
if not defined INTDIR exit /b 1
if not exist "%INTDIR%" mkdir "%INTDIR%"
if not exist "%INTDIR%\svn_version" mkdir "%INTDIR%\svn_version"
rem This workspace is a git checkout of the OpenMPT sources, so there is no
rem .svn metadata and no TortoiseSVN subwcrev tool. Exit quietly and let
rem common/version.cpp fall back to the checked-in build/svn_version/svn_version.h.
rem A genuine SVN working copy keeps the upstream behavior unchanged.
where subwcrev >nul 2>nul
if errorlevel 1 exit 0
subwcrev ..\.. ..\..\build\svn_version\svn_version.template.subwcrev.h "%INTDIR%\svn_version\svn_version.h"
if errorlevel 1 del "%INTDIR%\svn_version\svn_version.h" 2>nul
exit 0
