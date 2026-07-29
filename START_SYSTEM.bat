@echo off
REM Smart Token Management System - Universal Launcher
REM This script starts all components of the system

setlocal enabledelayedexpansion
set PROJECT_DIR=%~dp0
cd /d "%PROJECT_DIR%"

echo.
echo ============================================================
echo   Smart Token Management System - Universal Launcher
echo ============================================================
echo.
echo Starting all system components...
echo.

REM Check if npm is available
where npm >nul 2>nul
if errorlevel 1 (
    echo ERROR: Node.js/npm not found. Please install Node.js
    echo Download from: https://nodejs.org/
    pause
    exit /b 1
)

REM Check if Flutter is available
where flutter >nul 2>nul
if errorlevel 1 (
    echo WARNING: Flutter not found in PATH
    echo Some features may be unavailable
    echo.
)

echo [1/3] Starting Web Server on port 3000...
start /b /d "%PROJECT_DIR%" cmd /c "npm run dev"
timeout /t 2 /nobreak

echo [2/3] Opening Web Dashboard...
start "" "http://localhost:3000"

echo.
echo ============================================================
echo   System Running!
echo ============================================================
echo.
echo Web Dashboard: http://localhost:3000
echo.
echo Available Pages:
echo   - Home:              http://localhost:3000
echo   - Admin Dashboard:   http://localhost:3000/dashboard.html
echo   - Settings:          http://localhost:3000/settings.html
echo   - Reports:           http://localhost:3000/reports.html
echo   - TV Monitor:        http://localhost:3000/token-display.html
echo   - Registration:      http://localhost:3000/online-registration.html
echo   - Diagnostics:       http://localhost:3000/diagnostics.html
echo.
echo Default Credentials:
echo   Username: admin
echo   Password: admin123
echo.
echo To stop the server, press Ctrl+C in the npm window
echo.
echo ============================================================
pause
