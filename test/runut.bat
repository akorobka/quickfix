echo off
if "%1" == "" goto usage
if "%2" == "" goto usage

if "%1" == "debug" goto debug:
if "%1" == "release" goto release:
if "%1" == "debug.boost" goto debug.boost:
if "%1" == "release.boost" goto release.boost:
goto usage

:debug
set DIR=debug\ut
goto start

:release
set DIR=release\ut
goto start

:debug.boost
set DIR=debug.boost\ut
goto start

:release.boost
set DIR=release.boost\ut
goto start

:start
%DIR%\ut.exe -p %2 -f cfg\ut.cfg
goto quit

:usage
echo "Usage: runut [release | debug | release.boost | debug.boost] [port]"

:quit