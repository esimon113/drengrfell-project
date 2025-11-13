@echo off
setlocal

REM Optional: Zielkonfiguration (Debug oder Release)
set CONFIG=Release

REM Delete old build directory
if exist build (
    echo Removing old build directory...
    rmdir /s /q build
) else (
    echo No existing build directory found.
)

echo Creating new build directory...
mkdir build
cd build

echo Running CMake generation...
cmake -DCMAKE_BUILD_TYPE=%CONFIG% ..

echo Building project (%CONFIG%)...
cmake --build . --config %CONFIG%
echo Build completed.

echo.
echo Starting program...

REM Check if executable exists
if exist %CONFIG%\drengrfell.exe (
    echo Found %CONFIG%\drengrfell.exe, starting it now...
    %CONFIG%\drengrfell.exe
) else (
    echo ERROR: drengrfell.exe not found in %CONFIG% folder!
)

endlocal
pause
