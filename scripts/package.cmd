@echo off
setlocal
cd /d "%~dp0.."
set "PACKAGE_DIR=%~1"
if not defined PACKAGE_DIR set "PACKAGE_DIR=dist\QmlCockpitSimulator"
if exist "%PACKAGE_DIR%" (
    echo Choose a new package directory.
    exit /b 1
)
set "PATH=D:\Qt\6.12.0\msvc2022_64\bin;%PATH%"
call "D:\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" || exit /b 1
if not exist "build\QmlCockpitSimulator.exe" exit /b 1
mkdir "%PACKAGE_DIR%" || exit /b 1
copy /y "build\QmlCockpitSimulator.exe" "%PACKAGE_DIR%\QmlCockpitSimulator.exe" >nul || exit /b 1
windeployqt.exe --release --no-translations --qmldir qml --dir "%PACKAGE_DIR%" "%PACKAGE_DIR%\QmlCockpitSimulator.exe" || exit /b 1
copy /y "README.md" "%PACKAGE_DIR%\README.md" >nul || exit /b 1
copy /y "scripts\release-qt.conf" "%PACKAGE_DIR%\qt.conf" >nul || exit /b 1
xcopy /s /i /y "D:\Qt\Licenses" "%PACKAGE_DIR%\licenses\Qt" >nul || exit /b 1
exit /b 0
