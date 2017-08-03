@echo off
setlocal
set csvpath=%1
set processes=
:gather_processes
    shift
    if "%1"=="" goto done_gathering_processes
    set processes=%processes% %1
    goto gather_processes
:done_gathering_processes

if "%csvpath%"=="" goto usage
if "%processes%"=="" goto usage
goto run
:usage
    echo usage: prune_processes.cmd path_to_csv.csv process_name [process2_name ...] 1>&2
    echo. 1>&2
    echo CSV will be output to stdout with data from non-listed processes excluded. 1>&2
    echo. 1>&2
    echo e.g.: prune_processes.cmd PresentMon.csv foo.exe bar.exe ^> PresentMon-pruned.csv 1>&2
    exit /b 1

:run
echo Pruning processes from %csvpath% 1>&2
for %%a in (%processes%) do (
    echo.     %%a 1>&2
)
for /f %%a in (%csvpath%) do (
    for /f "tokens=1 delims=," %%b in ("%%a") do (
        call :print_line "%%a" "%%b"
    )
)
exit /b 0

:print_line
    set line=%~1
    set process=%~2
    for %%c in (%processes%) do (
        if "%process%"=="%%c" (
            echo %line%
            exit /b 0
        )
    )
    exit /b 1
