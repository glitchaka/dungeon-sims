@echo off
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_multiplayer.ps1 %*
if errorlevel 1 pause
