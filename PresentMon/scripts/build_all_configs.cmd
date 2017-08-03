@echo off
setlocal

set /a errorcount=0

echo x64-debug
msbuild /nologo /verbosity:quiet /p:Configuration=debug /p:Platform=x64 "%~dp0..\PresentMon.sln"
if not "%errorlevel%"=="0" set /a errorcount=%errorcount% + 1

echo x64-release
msbuild /nologo /verbosity:quiet /p:Configuration=release /p:Platform=x64 "%~dp0..\PresentMon.sln"
if not "%errorlevel%"=="0" set /a errorcount=%errorcount% + 1

echo x86-debug
msbuild /nologo /verbosity:quiet /p:Configuration=debug /p:Platform=x86 "%~dp0..\PresentMon.sln"
if not "%errorlevel%"=="0" set /a errorcount=%errorcount% + 1

echo x86-release
msbuild /nologo /verbosity:quiet /p:Configuration=release /p:Platform=x86 "%~dp0..\PresentMon.sln"
if not "%errorlevel%"=="0" set /a errorcount=%errorcount% + 1

echo.
if %errorcount% GTR 0 (
    echo %errorcount% configs failed!
    exit /b 1
)

echo All configs passed!
exit /b 0
