@echo off
setlocal

echo == Detecting Visual Studio Environment ==

:: 1. Locate vswhere.exe (It is always in this standard location)
set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%vswhere%" (
    echo Error: Could not find vswhere.exe. Is Visual Studio installed?
    exit /b 1
)

:: 2. Use vswhere to find the installation path of the latest Visual Studio
::    -latest: Gets the newest version installed
::    -requires ...VC.Tools...: Ensures C++ tools are actually installed
::    -property installationPath: Returns just the folder path
for /f "usebackq tokens=*" %%i in (`"%vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "vsInstallPath=%%i"
)

if "%vsInstallPath%"=="" (
    echo Error: Could not detect a valid Visual Studio installation with C++ tools.
    exit /b 1
)

echo Found Visual Studio at: %vsInstallPath%

:: 3. Construct the path to vcvars64.bat
set "vcvars=%vsInstallPath%\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%vcvars%" (
    echo Error: Could not find vcvars64.bat at expected location.
    exit /b 1
)

:: 4. Initialize the environment
echo Initializing MSVC environment...
call "%vcvars%"

echo.
echo == Environment Set. Starting Build ==

:: 5. Run your build commands
:: Ensure the build folder is clean if you want a fresh build
:: if exist build rmdir /s /q build

:: Assuming qt-cmake is in your PATH. 
:: If not, you might need to pass the Qt path as an argument to this script.
call qt-cmake -S . -B build -G Ninja

echo.
echo == CMake done, start build ==

cmake --build build

echo.
echo == Build Complete ==
