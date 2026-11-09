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
windeployqt.exe --release --no-translations --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler --qmldir qml --dir "%PACKAGE_DIR%" "%PACKAGE_DIR%\QmlCockpitSimulator.exe" || exit /b 1
copy /y "README.md" "%PACKAGE_DIR%\README.md" >nul || exit /b 1
copy /y "LICENSE" "%PACKAGE_DIR%\LICENSE" >nul || exit /b 1
copy /y "THIRD_PARTY_NOTICES.md" "%PACKAGE_DIR%\THIRD_PARTY_NOTICES.md" >nul || exit /b 1
copy /y "scripts\release-qt.conf" "%PACKAGE_DIR%\qt.conf" >nul || exit /b 1
xcopy /s /i /y "D:\Qt\Licenses" "%PACKAGE_DIR%\licenses\Qt" >nul || exit /b 1
xcopy /s /i /y "licenses" "%PACKAGE_DIR%\licenses" >nul || exit /b 1
mkdir "%PACKAGE_DIR%\licenses\Qt\sbom" || exit /b 1
for %%M in (qtbase qtdeclarative qtquick3d qtquicktimeline qtshadertools qtsvg qttasktree) do (
    copy /y "D:\Qt\6.12.0\msvc2022_64\sbom\%%M-6.12.0.spdx" "%PACKAGE_DIR%\licenses\Qt\sbom\" >nul || exit /b 1
)
exit /b 0
