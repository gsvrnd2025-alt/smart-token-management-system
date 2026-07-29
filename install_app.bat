@echo off
title Installing Smart Token Dashboard...
echo.
echo =======================================================
echo   Installing Smart Token Management System Desktop App...
echo =======================================================
echo.

set "TARGET_DIR=%USERPROFILE%\AppData\Local\Programs\dashboard_flutter"

echo 1. Creating installation directory: "%TARGET_DIR%"
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"

echo 2. Copying application files...
xcopy /E /I /Y "%~dp0dashboard_flutter\build\windows\x64\runner\Release\*" "%TARGET_DIR%\" >nul

echo 3. Creating Start Menu shortcut...
powershell -Command "$wshell = New-Object -ComObject WScript.Shell; $shortcut = $wshell.CreateShortcut(\"$env:APPDATA\Microsoft\Windows\Start Menu\Programs\Smart Token Dashboard.lnk\"); $shortcut.TargetPath = \"%TARGET_DIR%\dashboard_flutter.exe\"; $shortcut.WorkingDirectory = \"%TARGET_DIR%\"; $shortcut.Description = \"Smart Token Management System Dashboard\"; $shortcut.Save()"

echo.
echo =======================================================
echo   Installation Successful!
echo   You can now launch the app from your Start Menu:
echo   "Smart Token Dashboard"
echo =======================================================
echo.
pause
