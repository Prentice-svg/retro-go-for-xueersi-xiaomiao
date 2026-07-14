@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\flash_xiaomiao.ps1" %*
if errorlevel 1 (
  echo.
  echo Retro-Go ????????????????
) else (
  echo.
  echo Retro-Go ?????
)
pause
