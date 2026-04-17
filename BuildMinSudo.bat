@setlocal
@echo off

cd /d "%~dp0"

REM ============================================================
REM Please modify the following three variables according to your
REM actual Visual Studio installation path.
REM ============================================================

REM Visual Studio installation directory
REM Common path examples:
REM   Build Tools:  D:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools
REM   Community:    C:\Program Files\Microsoft Visual Studio\2022\Community
REM   Enterprise:   C:\Program Files\Microsoft Visual Studio\2022\Enterprise
REM   Professional: C:\Program Files\Microsoft Visual Studio\2022\Professional
set "VSBASE=D:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
set "MSBUILD=%VSBASE%\MSBuild\Current\Bin\MSBuild.exe"
set "VCVARSALL=%VSBASE%\VC\Auxiliary\Build\vcvarsall.bat"


echo ============================================
echo   MinSudo Build Script
echo ============================================
echo.

if not exist "%MSBUILD%" (
    echo [ERROR] MSBuild not found: "%MSBUILD%"
    echo.
    echo Please check if VSBASE variable is correct, or modify MSBUILD variable directly.
    goto :fail
)

if not exist "%VCVARSALL%" (
    echo [ERROR] vcvarsall.bat not found: "%VCVARSALL%"
    echo.
    echo Please check if VSBASE variable is correct, or modify VCVARSALL variable directly.
    goto :fail
)

echo [1/5] Initializing x64 build environment...
call "%VCVARSALL%" x64
if errorlevel 1 goto :fail

echo.
echo [2/5] Restoring NuGet packages...
"%MSBUILD%" MinSudo\MinSudo.vcxproj /t:Restore /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto :fail

echo.
echo [3/5] Building MinSudo - x64 Release...
"%MSBUILD%" MinSudo\MinSudo.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto :fail

echo.
echo [4/5] Restoring NuGet packages for x86...
"%MSBUILD%" MinSudo\MinSudo.vcxproj /t:Restore /p:Configuration=Release /p:Platform=x86
if errorlevel 1 goto :fail

echo.
echo [5/5] Building MinSudo - x86 Release...
"%MSBUILD%" MinSudo\MinSudo.vcxproj /t:Build /p:Configuration=Release /p:Platform=x86
if errorlevel 1 goto :fail

echo.
echo ============================================
echo   Build Succeeded!
echo ============================================
echo.
echo   x64: Output\Binaries\Release\x64\MinSudo.exe
echo   x86: Output\Binaries\Release\x86\MinSudo.exe
echo.
goto :end

:fail
echo.
echo ============================================
echo   Build FAILED!
echo ============================================

:end
pause
@endlocal
