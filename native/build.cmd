@echo off
setlocal
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%i"
if not defined VS exit /b 1
call "%VS%\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
pushd "%~dp0"
if not exist "..\bin\x86\Release" mkdir "..\bin\x86\Release"
cl /nologo /LD /MT /EHsc /O2 /W4 /WX /I"..\..\script hook source code\ScriptHook" NoAutoPushHelper.cpp /Fo"..\bin\x86\Release\NoAutoPushHelper.obj" /link /MACHINE:X86 /DEF:NoAutoPushHelper.def /OUT:"..\bin\x86\Release\NoAutoPushHelper.asi" /IMPLIB:"..\bin\x86\Release\NoAutoPushHelper.lib" "..\..\script hook source code\ScriptHookDotNet\ScriptHook.lib"
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" goto done
cl /nologo /MT /EHsc /O2 /W4 /WX HelperTests.cpp /Fo"..\bin\x86\Release\HelperTests.obj" /Fe"..\bin\x86\Release\HelperTests.exe"
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" goto done
"..\bin\x86\Release\HelperTests.exe"
set "RESULT=%ERRORLEVEL%"
:done
popd
exit /b %RESULT%
