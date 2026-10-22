@echo off
cd /d "%~dp0"
if not exist "godot\Godot_v4.7.2-stable_win64.exe" (
    echo Falta el ejecutable Godot en la carpeta godot.
    echo El proyecto editable esta en legacy\godot\project.godot
    pause
    exit /b 1
)
start "Isla Viva Godot" "godot\Godot_v4.7.2-stable_win64.exe" --path "%~dp0legacy\godot"
