svn checkout http://sirikatawin32.googlecode.com/svn/trunk dependencies
cd dependencies
7z x -y -bd *.zip > nul
cd ..\build\cmake



REM "c:\Program Files (x86)\Microsoft Visual Studio 9\Common7\Tools\vsvars32.bat"




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

set ERRORLEV=0
"c:\Program Files (x86)\CMake 2.8\bin\cmake" . -G "Visual Studio 9 2008" -DCMAKE_INSTALL_PREFIX=SirikataNightly
IF ERRORLEVEL 1 SET ERRORLEV=1
set vsconsoleoutput=1
set vcconsoleoutput=1
echo ClearBuilding... > warninglogvc9.txt
echo ClearBuilding... > errorlogvc9.txt
echo ClearBuilding... > iwarninglogvc9.txt
echo ClearBuilding... > ierrorlogvc9.txt
del warninglogvc9.txt
del errorlogvc9.txt
del iwarninglogvc9.txt
del ierrorlogvc9.txt
echo Building... > warninglogvc9.txt
echo Building... > errorlogvc9.txt

VCExpress.exe /build %1 /log warninglogvc9.txt /out errorlogvc9.txt sirikata.sln
IF ERRORLEVEL 1 SET ERRORLEV=1
type errorlogvc9.txt
type warninglogvc9.txt
echo Building... > iwarninglogvc9.txt
echo Building... > ierrorlogvc9.txt

if "%errorlevel%"=="0" VCExpress.exe /build %1 /log iwarninglogvc9.txt /out ierrorlogvc9.txt INSTALL.vcproj 
IF ERRORLEVEL 1 SET ERRORLEV=1
type ierrorlogvc9.txt
type iwarninglogvc9.txt
if "%ERRORLEV%"=="0" if "%1" == "Release" 7z a SirikataNightly.zip SirikataNightly
if "%ERRORLEV%"=="0" exit 0
exit 1
