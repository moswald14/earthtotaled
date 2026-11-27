@echo off
where node.exe >nul 2>nul
if errorlevel 1 (
  echo This author tool needs Node.js. You can also choose Open ChatGPT in browser in the game.
  pause
  exit /b 1
)
node.exe "%~dp0author.cjs"
pause
