@echo off
echo Building Pixel release...

REM Clean
rmdir /s /q pixel_release 2>nul
del pixel-release.zip 2>nul

REM Compile
g++ -std=c++17 -O3 -s -o pixel.exe main.cpp Lexer.cpp Parser.cpp TypeChecker.cpp Interpreter.cpp -I.

REM Create folder and copy
mkdir pixel_release
mkdir pixel_release\lib
copy pixel.exe pixel_release\
xcopy /E /I lib pixel_release\lib

REM ZIP
powershell Compress-Archive -Path pixel_release\* -DestinationPath pixel-release.zip -Force

echo Done! pixel-release.zip created
pause