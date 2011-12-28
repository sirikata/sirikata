REM build-win32-release.bat x.y.z
REM Builds a Windows binary release. This doesn't perform a complete build:
REM it depends on you already having setup dependencies properly.  You must
REM also have all the necessary tools: Visual Studio, the dependencies,
REM Python (for the symbol generation script), and 7zip for generating zip
REM files.

set VERSION=%1


REM This entire upper block is for determining where Visual Studio is. Search for TOPDIR for the real code (after a big empty block)

@SET VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0
@SET VCINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC
@SET FrameworkDir=C:\WINDOWS\Microsoft.NET\Framework
@SET FrameworkVersion=v2.0.50727
@SET Framework35Version=v3.5
@if "%VSINSTALLDIR%"=="" goto error_no_VSINSTALLDIR
@if "%VCINSTALLDIR%"=="" goto error_no_VCINSTALLDIR

@echo Setting environment for using Microsoft Visual Studio 2008 x86 tools.

@call :GetWindowsSdkDir

@if not "%WindowsSdkDir%" == "" (
	set "PATH=%WindowsSdkDir%bin;%PATH%"
	set "INCLUDE=%WindowsSdkDir%include;%INCLUDE%"
	set "LIB=%WindowsSdkDir%lib;%LIB%"
)


@rem
@rem Root of Visual Studio IDE installed files.
@rem
@set DevEnvDir=C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE

@set PATH=C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\BIN;C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools;C:\WINDOWS\Microsoft.NET\Framework\v3.5;C:\WINDOWS\Microsoft.NET\Framework\v2.0.50727;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\VCPackages;%PATH%
@set INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\INCLUDE;%INCLUDE%
@set LIB=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\LIB;%LIB%
@set LIBPATH=C:\WINDOWS\Microsoft.NET\Framework\v3.5;C:\WINDOWS\Microsoft.NET\Framework\v2.0.50727;C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\LIB;%LIBPATH%

@goto end

:GetWindowsSdkDir
@call :GetWindowsSdkDirHelper HKLM > nul 2>&1
@if errorlevel 1 call :GetWindowsSdkDirHelper HKCU > nul 2>&1
@if errorlevel 1 set WindowsSdkDir=%VCINSTALLDIR%\PlatformSDK\
@exit /B 0

:GetWindowsSdkDirHelper
@for /F "tokens=1,2*" %%i in ('reg query "%1\SOFTWARE\Microsoft\Microsoft SDKs\Windows" /v "CurrentInstallFolder"') DO (
	if "%%i"=="CurrentInstallFolder" (
		SET "WindowsSdkDir=%%k"
	)
)
@if "%WindowsSdkDir%"=="" exit /B 1
@exit /B 0

:error_no_VSINSTALLDIR
@echo ERROR: VSINSTALLDIR variable is not set. 
@goto end

:error_no_VCINSTALLDIR
@echo ERROR: VCINSTALLDIR variable is not set. 
@goto end

:end









set TOPDIR=%CD%
set BUILDDIR=%TOPDIR%\build\cmake
set INSTALLDIR=%TOPDIR%\sirikata_win32
set INSTALLPDBDIR=%TOPDIR%\sirikata_win32_pdb
set INSTALLSYMDIR=%TOPDIR%\sirikata_win32_sym

set CMAKE="C:\Program Files (x86)\CMake 2.8\bin\cmake.exe"
set PYTHON="C:\Python27\python.exe"
set SEVENZIP="C:\Program Files\7-Zip\7z.exe"


REM Main build and install
cd %BUILDDIR%
%CMAKE% -E remove_directory %INSTALLDIR%
%CMAKE% -E remove %BUILDDIR%\CMakeCache.txt
call %CMAKE% -G "Visual Studio 9 2008" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=%INSTALLDIR% %BUILDDIR%
REM not sure what you're supposed to use here, why doesn't VCExpress.exe support /clean?
vcbuild /clean Sirikata.sln
VCExpress.exe /build RelWithDebInfo /project ALL_BUILD /log sirikata.win32.release.warning.log /out sirikata.win32.release.error.log Sirikata.sln
VCExpress.exe /build RelWithDebInfo /project INSTALL Sirikata.sln

REM Breakpad symbols
%CMAKE% -E remove_directory %INSTALLSYMDIR%
%CMAKE% -E make_directory %INSTALLSYMDIR%
%CMAKE% -E remove_directory symbols
call %PYTHON% %TOPDIR%\tools\crashcollector\symbolstore.py
call xcopy /E symbols\* %INSTALLSYMDIR%

REM PDB files
%CMAKE% -E remove_directory %INSTALLPDBDIR%
%CMAKE% -E make_directory %INSTALLPDBDIR%
call copy RelWithDebInfo\*.pdb %INSTALLPDBDIR%

REM Archives
cd %TOPDIR%
%CMAKE% -E remove sirikata-%VERSION%-win32.zip sirikata-%VERSION%-win32.dbg.zip
call %SEVENZIP% a sirikata-%VERSION%-win32.zip sirikata_win32 
call %SEVENZIP% a sirikata-%VERSION%-win32.dbg.zip sirikata_win32_pdb sirikata_win32_sym

:finished
cd %TOPDIR%
