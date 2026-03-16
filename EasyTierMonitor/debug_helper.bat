@echo off
echo Checking for error log in common locations...
if exist "error.log" (
    echo Found error.log in current directory
    type error.log
) else if exist "..\error.log" (
    echo Found error.log in parent directory
    type ..\error.log
) else if exist "bin\x64\Release\error.log" (
    echo Found error.log in Release directory
    type "bin\x64\Release\error.log"
) else (
    echo error.log not found. Creating test directory structure...
    mkdir test_debug 2>/dev/null
    cd test_debug
)
