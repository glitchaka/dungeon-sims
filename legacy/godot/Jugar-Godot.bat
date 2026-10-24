@echo off
cd /d "%~dp0"
if not exist "..\..\godot\Godot_v4.7.2-stable_win64.exe" (
    echo Falta el motor Godot en la carpeta godot de la raiz.
    echo Abre project.godot con tu instalacion de Godot.
    pause
    exit /b 1
)
start "Isla Viva Godot" "..\..\godot\Godot_v4.7.2-stable_win64.exe" --path "%~dp0."
