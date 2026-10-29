@echo off
title DeskBuddy Laptop Hardware Monitor
cd /d "%~dp0"

echo ============================================================
echo   DeskBuddy — Laptop Hardware Monitor Server (Port 34001)
echo ============================================================
echo.

where python >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [*] Python detected. Starting Python monitor...
    python deskbuddy_monitor.py
    if %ERRORLEVEL% NEQ 0 (
        echo.
        echo [!] Python exited with error, starting native PowerShell monitor...
        powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0deskbuddy_monitor.ps1"
    )
) else (
    echo [*] Python not detected. Starting native Windows PowerShell monitor...
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0deskbuddy_monitor.ps1"
)

pause
