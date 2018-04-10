@ECHO Off
SETLOCAL ENABLEDELAYEDEXPANSION 
SET "sourcedir=resources\environmentmaps\"
SET "destdir=data\environmentmaps\

FOR /f "delims=" %%a IN ('xcopy /y /L /s "%sourcedir%*"') DO (
 SET "destfile=%%a"
 SET "destfile=!destfile:*%sourcedir%=%destdir%!"
 IF /i "%%a" neq "!destfile!" (
  FOR %%m IN ("!destfile!") DO IF NOT EXIST "%%~dpm%%~na.dds" (
   MD "%%~dpm" 2>nul
	xcopy /i /y "%%a" "%%~dpm%%~na.dds*"
  )
 )
)
pause
GOTO :EOF