@echo off
setlocal enabledelayedexpansion

echo ============================================
echo   oraQuadra Nano - Flash Diretto Completo
echo ============================================
echo.

:: Verifica file necessari
if not exist "firmware.bin" (
    echo [ERRORE] firmware.bin non trovato!
    echo Esegui prima compile.bat
    pause
    exit /b 1
)

if not exist "bootloader.bin" (
    echo [ERRORE] bootloader.bin non trovato!
    pause
    exit /b 1
)

if not exist "partitions.bin" (
    echo [ERRORE] partitions.bin non trovato!
    pause
    exit /b 1
)

if not exist "boot_app0.bin" (
    echo [ERRORE] boot_app0.bin non trovato!
    pause
    exit /b 1
)

if not exist "littlefs_2mb.bin" (
    echo [AVVISO] littlefs_2mb.bin non trovato - i file audio non saranno flashati
    set FLASH_LITTLEFS=0
) else (
    set FLASH_LITTLEFS=1
)

:: Verifica Python e installa esptool se necessario
where python >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERRORE] Python non trovato!
    echo Installa Python da: https://www.python.org/downloads/
    pause
    exit /b 1
)

echo [1/3] Verifico esptool...
python -m pip show esptool >nul 2>&1
if %errorlevel% neq 0 (
    echo      Installo esptool...
    python -m pip install esptool
)

echo.
echo [2/3] Cerco porte COM disponibili...
echo.

:: Lista le porte COM
python -c "import serial.tools.list_ports; ports = list(serial.tools.list_ports.comports()); [print(f'  {p.device} - {p.description}') for p in ports]" 2>nul

if %errorlevel% neq 0 (
    python -m pip install pyserial >nul 2>&1
    python -c "import serial.tools.list_ports; ports = list(serial.tools.list_ports.comports()); [print(f'  {p.device} - {p.description}') for p in ports]"
)

echo.
set /p PORTA="Inserisci la porta COM (es. COM3): "

echo.
echo [3/3] Flash COMPLETO in corso su %PORTA%...
echo      Metti l'ESP32 in modalita BOOT se necessario
echo      (tieni premuto BOOT, premi RESET, rilascia BOOT)
echo.
echo File da flashare:
echo   - bootloader.bin   @ 0x0
echo   - partitions.bin   @ 0x8000
echo   - boot_app0.bin    @ 0xe000
echo   - firmware.bin     @ 0x10000
if %FLASH_LITTLEFS%==1 (
    echo   - littlefs_2mb.bin     @ 0x310000
)
echo.

if %FLASH_LITTLEFS%==1 (
    python -m esptool --chip esp32s3 --port %PORTA% --baud 921600 ^
        --before default_reset --after hard_reset write_flash -z ^
        --flash_mode dio --flash_freq 80m --flash_size 16MB ^
        0x0 bootloader.bin ^
        0x8000 partitions.bin ^
        0xe000 boot_app0.bin ^
        0x10000 firmware.bin ^
        0x310000 littlefs_2mb.bin
) else (
    python -m esptool --chip esp32s3 --port %PORTA% --baud 921600 ^
        --before default_reset --after hard_reset write_flash -z ^
        --flash_mode dio --flash_freq 80m --flash_size 16MB ^
        0x0 bootloader.bin ^
        0x8000 partitions.bin ^
        0xe000 boot_app0.bin ^
        0x10000 firmware.bin
)

if %errorlevel% equ 0 (
    echo.
    echo ============================================
    echo   FLASH COMPLETATO CON SUCCESSO!
    echo ============================================
    echo.
    echo Premi RESET sull'ESP32 per avviare il firmware.
) else (
    echo.
    echo [ERRORE] Flash fallito!
    echo Verifica:
    echo   - La porta COM e corretta
    echo   - L'ESP32 e in modalita BOOT
    echo   - Il cavo USB e un cavo dati
)

echo.
pause
