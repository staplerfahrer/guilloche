rem @echo off
::echo %buildlevel%
::if %buildlevel% LEQ 1 goto ok
::echo Build error.
::goto end
::
:::ok
::@echo on
echo 0 0 0.875 0.125 0.9 0.0 0.4 72 0.1 0 8k sunburstAndCircles | guilloche.exe

:: :end


@REM gcc -O9 -g experiment.c -o experiment.exe
@REM experiment.exe