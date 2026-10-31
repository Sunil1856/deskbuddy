@echo off
title DeskBuddy Cloudflare Global Tunnel
cd /d "%~dp0"
echo ==============================================================================
echo   DeskBuddy Cloudflare Global Tunnel
echo   Forwarding Phone Server (http://192.0.0.8:8000) to World Wide Web...
echo ==============================================================================
echo.
echo Copy the "https://....trycloudflare.com" link below to open DeskBuddy
echo from ANY phone, laptop, college Wi-Fi, or cellular network in the world!
echo.
cloudflared.exe tunnel --url http://192.0.0.8:8000
pause
