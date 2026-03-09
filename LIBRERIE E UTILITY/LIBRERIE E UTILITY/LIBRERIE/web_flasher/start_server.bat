@echo off
echo ============================================
echo   oraQuadra Nano - Web Flasher Server
echo ============================================
echo.

:: Verifica Python
where python >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERRORE] Python non trovato!
    echo Installa Python da: https://www.python.org/downloads/
    pause
    exit /b 1
)

:: Verifica che esista firmware.bin
if not exist "firmware.bin" (
    echo [ATTENZIONE] firmware.bin non trovato!
    echo Esegui prima compile_and_deploy.bat per compilare lo sketch.
    echo.
    echo Vuoi continuare comunque? (per test)
    pause
)

echo Server avviato su: http://localhost:8080
echo.
echo Apri Chrome/Edge e vai a http://localhost:8080
echo Premi Ctrl+C per fermare il server.
echo.
echo ============================================

:: Avvia server Python
python -m http.server 8080
