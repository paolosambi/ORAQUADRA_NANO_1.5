@echo off
echo ================================================
echo ELIMINA FILE "nul" (nome riservato Windows)
echo ================================================
echo.
echo Tentativo 1: Eliminazione con prefisso \\?\
del /F /Q "\\?\%CD%\nul" 2>nul
if not exist "nul" (
    echo [OK] File "nul" eliminato con successo!
    goto :success
)

echo Tentativo 2: Eliminazione con path UNC
del /F /Q "\\?\%CD%\nul" 2>nul
if not exist "nul" (
    echo [OK] File "nul" eliminato con successo!
    goto :success
)

echo Tentativo 3: Rinomina + Eliminazione
ren "\\?\%CD%\nul" "nul_temp.txt" 2>nul
del /F /Q "nul_temp.txt" 2>nul
if not exist "nul" (
    echo [OK] File "nul" eliminato con successo!
    goto :success
)

echo.
echo [ERRORE] Impossibile eliminare il file "nul"
echo Prova manualmente con:
echo   1. Apri PowerShell come Amministratore
echo   2. Esegui: Remove-Item -LiteralPath "\\?\%CD%\nul" -Force
echo.
pause
exit /b 1

:success
echo.
echo ================================================
echo OPERAZIONE COMPLETATA
echo ================================================
pause
exit /b 0
