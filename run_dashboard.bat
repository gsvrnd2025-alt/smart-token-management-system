@echo off
title Smart Token Dashboard Launcher
echo.
echo ==================================================
echo   Starting local web server for Flutter Dashboard...
echo ==================================================
echo.

:: Start the server in the background using port 8080
start /b npx -y http-server -p 8080 "dashboard_flutter/build/web"

:: Give it 2 seconds to bind and initialize
timeout /t 2 >nul

echo.
echo ==================================================
echo   Launching Dashboard in Desktop Window Mode...
echo ==================================================
echo.

:: Launch in borderless desktop app mode
start "" chrome.exe --app="http://localhost:8080"

exit
