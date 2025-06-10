@echo off

if not defined _MARCH_DEV_ENV_ACTIVATED (
    set "PATH=%~dp0DevEnv;%PATH%"
    set "PROMPT=(MarchDev) %PROMPT%"
    set "_MARCH_DEV_ENV_ACTIVATED=1"
)