CD /D %~dp0
echo Moving necessary files
xcopy /i /y "assimp-vc140-mt.dll" "bin\"
xcopy /i /y "config.ini" "bin\"
echo Compiling Models
call compileModels.bat
echo Compiling Textures
call compileTextures.bat
echo Done!
exit