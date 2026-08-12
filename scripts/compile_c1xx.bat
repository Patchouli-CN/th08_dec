@echo off
call "%~dp0th08vars.bat"
cd /d "%~dp0"
CL.EXE pragma_var_order.cpp /o build/hackery.dll /link /DLL
if %errorlevel% neq 0 exit /b %errorlevel%
move /y "build\hackery.dll" "Program Files\MICROSOFT VISUAL STUDIO .NET\VC7\BIN\C1XX.DLL"
echo DONE
