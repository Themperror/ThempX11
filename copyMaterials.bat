@ECHO Off
SETLOCAL ENABLEDELAYEDEXPANSION 
SET "sourcedir=resources\materials"
SET "destdir=data\materials

FOR /f "delims=" %%a IN ('xcopy /y /L /s "%sourcedir%\*.mat"') DO (
 SET "destfile=%%a"
 SET "destfile=!destfile:*%sourcedir%=%destdir%!"
 IF /i "%%a" neq "!destfile!" (
  FOR %%m IN ("!destfile!") DO IF NOT EXIST "%%~dpm%%~na.mat" (
   MD "%%~dpm" 2>nul
	xcopy /i /y "%%a" "%%~dpm%%~na.mat*"
  )
 )
)
pause
GOTO :EOF