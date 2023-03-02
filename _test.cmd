::@echo off
::echo %buildlevel%
::if %buildlevel% LEQ 1 goto ok
::echo Build error.
::goto end
::
:::ok
::@echo on
rem echo x 0 0 0.875 0.125 0.9 0.0 0.4 72 0.1 0 1k | a.exe x 
a.exe

:: :end
