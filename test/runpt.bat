echo off
if "%1" == "" goto usage
if "%2" == "" goto usage

if "%1" == "debug" goto debug:
if "%1" == "release" goto release:
if "%1" == "debug.boost" goto debug.boost:
if "%1" == "release.boost" goto release.boost:
goto usage

:debug
set DIR=debug\pt
goto start

:debug.boost
set DIR=debug.boost\pt
goto start

:release
set DIR=release\pt
goto start

:release.boost
set DIR=release.boost\pt
goto start

:start
%DIR%\pt.exe -p %2 -c 500000
goto quit

:usage
echo "Usage: runpt [release | debug | release.boost | debug.boost ] [port]"

:quit