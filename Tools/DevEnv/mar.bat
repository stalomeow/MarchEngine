@echo off
setlocal

set engineDir=%~dp0..\..
set premakeDir=%engineDir%\Tools\Build\Premake
set premakeExe=%premakeDir%\bin\release\premake5.exe

if "%~1"=="" (
    set premakeArgs=vs2022
) else (
    set premakeArgs=%*
)

if not exist "%premakeExe%" (
    pushd "%premakeDir%"
    call Bootstrap.bat
    rmdir /s /q "%premakeDir%\build"
    popd
)

pushd "%engineDir%"
"%premakeExe%" --file="%engineDir%\Tools\Build\BuildEngine.lua" %premakeArgs%
popd