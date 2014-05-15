@ECHO OFF

:: Check Windows version (XP Pro or later) and command line arguments (none)
IF NOT "%OS%"=="Windows_NT"    GOTO Syntax
IF NOT "%~1"==""               GOTO Syntax
WMIC.EXE Alias /? >NUL 2>&1 || GOTO Syntax

SETLOCAL ENABLEDELAYEDEXPANSION

:: Use WMIC to retrieve date and time
FOR /F "skip=1 tokens=1-6" %%A IN ('WMIC Path Win32_LocalTime Get Day^,Hour^,Minute^,Month^,Second^,Year /Format:table') DO (
	IF NOT "%%~F"=="" (
		SET /A SortDate = 10000 * %%F + 100 * %%D + %%A
		SET /A SortTime = 10000 * %%B + 100 * %%C + %%E
		SET SortTime=0000000!SortTime!
		SET SortTime=!SortTime:~-6!
	)
)

:: Display the results:
ECHO #define VERSION_TIMESTAMP "%SortDate%.%SortTime%"

:: Done
ENDLOCAL

GOTO:EOF