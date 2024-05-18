
@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
FOR /L %%i IN (1,1,1000000) DO (
    SET "data=gexiannan%%i"
    curl -v -X POST http://localhost:8080 -d "!data!"
)
