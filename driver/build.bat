@echo off
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl -FC -Zi /std:c++17 ..\driver\driver.cpp ..\driver\glad.c ^
/I"..\include" ^
/link /LIBPATH:"..\lib" ^
user32.lib Gdi32.lib opengl32.lib glfw3.lib ^
Ole32.lib Mmdevapi.lib winmm.lib
popd
