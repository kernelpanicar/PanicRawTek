@echo off
rem =============================================================================
rem  PANIC RAW TEK - compila y genera el instalador .exe para Windows
rem  Requisitos: Visual Studio 2022 (con "Desarrollo para escritorio con C++"),
rem              CMake (viene con Visual Studio) e Inno Setup 6 (jrsoftware.org)
rem =============================================================================
setlocal
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
    echo !! No encuentro CMake. Abri "Developer PowerShell for VS 2022" o instala CMake.
    pause & exit /b 1
)

echo ^>^> Configurando...
cmake -B build-win -G "Visual Studio 17 2022" -A x64 > build_config_log.txt 2>&1
if errorlevel 1 ( type build_config_log.txt & pause & exit /b 1 )

echo ^>^> Compilando Panic Raw Tek (VST3 + Standalone)... tarda unos minutos
cmake --build build-win --config Release --target PanicRawTek_VST3 PanicRawTek_Standalone -- /m /v:minimal > build_log.txt 2>&1
if errorlevel 1 (
    echo !! Fallo la compilacion. Errores:
    findstr /i /c:" error " build_log.txt
    pause & exit /b 1
)

for /f "tokens=4 delims=( " %%v in ('findstr /r /c:"^project(PanicRawTek VERSION" CMakeLists.txt') do set VER=%%v

set ISCC=
if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set ISCC="%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set ISCC="%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not defined ISCC (
    echo !! Falta Inno Setup 6. Bajalo de https://jrsoftware.org/isdl.php y volve a correr este archivo.
    pause & exit /b 1
)

echo ^>^> Generando instalador v%VER%...
%ISCC% /Q /DMyAppVersion=%VER% /DArtDir=..\..\build-win\PanicRawTek_artefacts\Release installer\windows\PanicRawTek.iss
if errorlevel 1 ( pause & exit /b 1 )

echo.
echo Listo: dist\Panic Raw Tek %VER% Windows Setup.exe
explorer dist
pause
