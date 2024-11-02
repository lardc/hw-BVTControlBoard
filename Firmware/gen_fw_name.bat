@echo off
setlocal

del *.bin *.hex

rem # If no arguments — go to file rename
rem # else generate hex for TMS
if "%~1"=="" goto blank

set hex_cmd=%1 %2 %3 %4
set hex_cmd=%hex_cmd:"=%
%hex_cmd%

:blank

for /f "tokens=3 delims=-." %%i in ('git config --local remote.origin.url') do set git_proj=%%i
for /f %%i in ('git log -1 "--pretty=%%cd" "--date=format:%%Y.%%m.%%d_%%H.%%M"') do set git_date=%%i
set f_name=%git_proj%_%git_date%

if exist %git_proj%.binary (
	copy /y %git_proj%.binary %f_name%.bin 1>NUL
	echo File %f_name%.bin created
)
if exist %git_proj%.hex (
	copy /y %git_proj%.hex %f_name%.hex 1>NUL
	echo File %f_name%.hex created
)
