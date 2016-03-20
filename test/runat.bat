echo off
if "%1" == "" goto usage
if "%2" == "" goto usage

if "%1" == "debug" goto debug:
if "%1" == "release" goto release:
if "%1" == "debug.boost" goto debug.boost:
if "%1" == "release.boost" goto release.boost:
goto usage

:debug
set DIR=debug\at
goto start

:release
set DIR=release\at
goto start

:debug.boost
set DIR=debug.boost\at
goto start

:release.boost
set DIR=release.boost\at
goto start


:start
call setup.bat %2
%DIR%\atrun -t run -s "%DIR%\at.exe -f cfg\at.cfg" -d . -c "ruby -I.\ Runner.rb 127.0.0.1 %2 definitions\server\fix40\*.def definitions\server\fix41\*.def definitions\server\fix42\*.def definitions\server\fix43\*.def definitions\server\fix44\*.def definitions\server\fix50\*.def definitions\server\fix50sp1\*.def definitions\server\fix50sp2\*.def" -i .\
goto quit

:usage
echo "Usage: runat [release | debug | release.boost | debug.boost] [port]"

:quit