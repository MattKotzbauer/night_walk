@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl -FC -Zi /std:c++17 ..\driver\driver.cpp ..\driver\glad.c ^
/I"..\include" ^
/link /LIBPATH:"..\lib" ^
/NODEFAULTLIB:LIBCMT ^
/NODEFAULTLIB:MSVCRTD ^
user32.lib Gdi32.lib opengl32.lib glfw3.lib ^
shell32.lib msvcrt.lib vcruntime.lib ucrt.lib ^
Ole32.lib Mmdevapi.lib
popd
