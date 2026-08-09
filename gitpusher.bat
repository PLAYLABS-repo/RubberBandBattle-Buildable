@echo off
echo ==========================
echo   Git Auto Push Script
echo ==========================

cd /d "%~dp0"

echo.
if not exist ".git" (
    echo Initializing Git repository...
    git init
)

echo.
echo Setting remote repository...
git remote remove origin >nul 2>&1
git remote add origin hhttps://github.com/PLAYLABS-repo/Absolut-Engine-ULTRA.git

echo.
echo Switching to main branch...
git branch -M main

echo.
git add .

echo.
set /p msg=Enter commit message: 

git commit -m "%msg%"

echo.
echo Pulling latest changes...
git pull origin main --rebase

echo.
echo Pushing to origin main...
git push -u origin main

echo.
echo Done!
pause