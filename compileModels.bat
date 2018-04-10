@ECHO Off
SETLOCAL ENABLEDELAYEDEXPANSION 
SET "sourcedir=resources\models"
SET "destdir=data\models"

FOR /f "delims=" %%a IN ('xcopy /y /L /s "%sourcedir%\*"') DO (
 SET "destfile=%%a"
 SET "destfile=!destfile:*%sourcedir%=%destdir%!"
 IF /i "%%a" neq "!destfile!" (
  FOR %%m IN ("!destfile!") DO IF NOT EXIST "%%~dpm%%~na.bin" (
   MD "%%~dpm" 2>nul
   "utils\ThempModelParser.exe" "%%a" "%%~dpm%%~na.bin"
  )
 )
)
GOTO :EOF