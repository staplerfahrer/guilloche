rem @echo off
::echo %buildlevel%
::if %buildlevel% LEQ 1 goto ok
::echo Build error.
::goto end
::
:::ok
::@echo on
echo 0 0 0.875 0.125 1000 100 0.4 72 0 500 4k slinky | guilloche.exe

:: :end


@REM gcc -O9 -g experiment.c -o experiment.exe
@REM experiment.exe