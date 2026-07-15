@echo off
setlocal

cd /d "%~dp0"

set "ENV_NAME=%~1"
if "%ENV_NAME%"=="" set "ENV_NAME=wemos_d1_mini32"

set "UPLOAD_PORT=%~2"

where pio >nul 2>nul
if %errorlevel%==0 (
  set "PIO_CMD=pio"
) else (
  where platformio >nul 2>nul
  if %errorlevel%==0 (
    set "PIO_CMD=platformio"
  ) else (
    echo Erreur: ni pio ni platformio n'est disponible dans le PATH.
    echo Installez PlatformIO Core puis relancez ce script.
    exit /b 1
  )
)

echo Projet: %cd%
echo Environnement: %ENV_NAME%

if "%UPLOAD_PORT%"=="" (
  %PIO_CMD% run -e %ENV_NAME% -t uploadfs
) else (
  echo Port: %UPLOAD_PORT%
  %PIO_CMD% run -e %ENV_NAME% -t uploadfs --upload-port %UPLOAD_PORT%
)

if errorlevel 1 exit /b %errorlevel%

echo Upload LittleFS termine.
endlocal
