@echo off
cd /d "%~dp0"
if not exist "dist\IslaViva-alpha-roads\isla_viva_native.exe" (
    echo Falta la alpha con mapa. Usa Jugar.bat para compilar el proyecto.
    pause
    exit /b 1
)
"dist\IslaViva-alpha-roads\isla_viva_native.exe" %*
if errorlevel 1 pause


