@echo off
setlocal

set "SOURCE_ROOT=%~dp0"
set "DEST_ROOT=C:\Users\Nipa1\Documents\Unreal Projects\MDs"

if not exist "%DEST_ROOT%" (
    mkdir "%DEST_ROOT%"
)

call :CopyDirectory "docs"
if errorlevel 1 exit /b %ERRORLEVEL%

call :CopyDirectory "GDD_Project_20260723_1144"
if errorlevel 1 exit /b %ERRORLEVEL%

call :CopyDirectory "Tasks"
if errorlevel 1 exit /b %ERRORLEVEL%

call :CopyFile "AGENTS.md"
if errorlevel 1 exit /b %ERRORLEVEL%

echo.
echo Copy completed: "%DEST_ROOT%"
exit /b 0

:CopyDirectory
set "ITEM=%~1"
if not exist "%SOURCE_ROOT%%ITEM%\" (
    echo Missing directory: "%SOURCE_ROOT%%ITEM%"
    exit /b 1
)

robocopy "%SOURCE_ROOT%%ITEM%" "%DEST_ROOT%\%ITEM%" /E /NFL /NDL /NJH /NJS /NP
if %ERRORLEVEL% GEQ 8 (
    echo Failed to copy directory: "%ITEM%"
    exit /b %ERRORLEVEL%
)
exit /b 0

:CopyFile
set "ITEM=%~1"
if not exist "%SOURCE_ROOT%%ITEM%" (
    echo Missing file: "%SOURCE_ROOT%%ITEM%"
    exit /b 1
)

copy /Y "%SOURCE_ROOT%%ITEM%" "%DEST_ROOT%\%ITEM%" >nul
if errorlevel 1 (
    echo Failed to copy file: "%ITEM%"
    exit /b 1
)
exit /b 0
