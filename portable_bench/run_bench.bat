@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo   LBS vs DQS Benchmark Harness (Portable Suite)
echo ============================================================
echo Machine: %COMPUTERNAME%
echo Current Directory: %CD%
echo.

if not exist "Sample.exe" (
    echo ERROR: Sample.exe not found in current directory!
    pause
    exit /b 1
)

echo Starting automated GPU benchmarks (--e2 --e3)...
echo Expected duration: approx 9.5 minutes. Please do not touch machine.
echo.

Sample.exe --e2 --e3

echo.
echo Benchmark run finished. Collecting results...

set "ZIP_NAME=results_%COMPUTERNAME%.zip"

echo Packaging CSVs into %ZIP_NAME%...
powershell -NoProfile -Command "$files = @(Get-ChildItem -Path . -Filter *.csv -Recurse | Select-Object -ExpandProperty FullName); if ($files.Count -gt 0) { Compress-Archive -Path $files -DestinationPath '%ZIP_NAME%' -Force; Write-Host 'Packaged ' $files.Count ' CSV files.' }"

if exist "%ZIP_NAME%" (
    echo.
    echo ============================================================
    echo SUCCESS: Results packaged successfully into:
    echo   %CD%\%ZIP_NAME%
    echo ============================================================
) else (
    echo.
    echo Results generated in Results folder.
)

echo.
echo All done. Press any key to exit.
pause >nul
