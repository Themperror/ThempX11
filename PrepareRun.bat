CD /D %~dp0
if not exist "data\materials\" mkdir "data\materials"
echo Copying Materials
call copyMaterials.bat
echo Compiling Models
call compileModels.bat
echo Compiling Textures
call compileTextures.bat
echo Compiling Material Textures
call compilePBRTextures.bat
echo Copying Environment Maps
call compileEnvMaps.bat
echo Done!
exit