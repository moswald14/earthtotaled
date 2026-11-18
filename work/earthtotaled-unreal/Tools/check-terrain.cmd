@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
if not exist "%~dp0..\Saved" mkdir "%~dp0..\Saved"
cl /nologo /std:c++17 /EHsc /W4 "%~dp0terrain-check.cpp" /Fe:"%~dp0..\Saved\terrain-check.exe" /Fo:"%~dp0..\Saved\terrain-check.obj"
if errorlevel 1 exit /b 1
"%~dp0..\Saved\terrain-check.exe"
