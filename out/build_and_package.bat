@echo off
set QT_DIR=C:\Qt\5.14.2\mingw73_32
set OUT_DIR=%~dp0

if exist "%OUT_DIR%\FuckRedSpiderQt.exe" (
    call "%QT_DIR%\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-opengl-sw "%OUT_DIR%\FuckRedSpiderQt.exe"
)
