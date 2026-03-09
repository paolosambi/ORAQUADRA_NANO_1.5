@echo off
setlocal enabledelayedexpansion

echo ============================================
echo   oraQuadra Nano - Compilazione
echo ============================================
echo.

:: Configurazione
set SKETCH_PATH=..\oraQuadra_Nano_Ver_1.5.ino
set FQBN=esp32:esp32:esp32s3:CDCOnBoot=cdc,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=huge_app_2mb_spiffs,PSRAM=opi,UploadSpeed=921600
set OUTPUT_DIR=.\build

:: Verifica Arduino CLI
where arduino-cli >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERRORE] Arduino CLI non trovato!
    echo.
    echo Installa Arduino CLI da: https://arduino.github.io/arduino-cli/
    echo Oppure esegui: winget install Arduino.ArduinoCLI
    echo.
    pause
    exit /b 1
)

:: Crea cartella output
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo [1/4] Aggiorno indice delle board...
arduino-cli core update-index

echo.
echo [2/4] Verifico installazione ESP32...
arduino-cli core list | findstr "esp32:esp32" >nul
if %errorlevel% neq 0 (
    echo      Installo ESP32 core...
    arduino-cli core install esp32:esp32@2.0.17
) else (
    echo      ESP32 core gia installato
)

echo.
echo [3/4] Compilo lo sketch...
echo      Board: ESP32-S3 (3MB APP + 2MB SPIFFS, OPI PSRAM, 16MB Flash)
echo      Questo potrebbe richiedere alcuni minuti...
echo.

arduino-cli compile ^
    --fqbn %FQBN% ^
    --output-dir "%OUTPUT_DIR%" ^
    --warnings none ^
    "%SKETCH_PATH%"

if %errorlevel% neq 0 (
    echo.
    echo [ERRORE] Compilazione fallita!
    pause
    exit /b 1
)

echo.
echo      Compilazione sketch completata!
echo.
echo [4/4] Copio i file per il web flasher...

:: Copia firmware principale
for %%f in ("%OUTPUT_DIR%\*.ino.bin") do (
    copy "%%f" ".\firmware.bin" >nul
    echo      Copiato: %%~nxf -^> firmware.bin
)

:: Copia bootloader
for %%f in ("%OUTPUT_DIR%\*.ino.bootloader.bin") do (
    copy "%%f" ".\bootloader.bin" >nul
    echo      Copiato: %%~nxf -^> bootloader.bin
)

:: Copia partizioni
for %%f in ("%OUTPUT_DIR%\*.ino.partitions.bin") do (
    copy "%%f" ".\partitions.bin" >nul
    echo      Copiato: %%~nxf -^> partitions.bin
)

:: Copia boot_app0.bin dalla cartella ESP32 core
set ESP32_BOOT=%USERPROFILE%\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.17\tools\partitions\boot_app0.bin
if exist "%ESP32_BOOT%" (
    copy "%ESP32_BOOT%" ".\boot_app0.bin" >nul
    echo      Copiato: boot_app0.bin
) else (
    echo      [AVVISO] boot_app0.bin non trovato in ESP32 core
    echo               Cercando in percorsi alternativi...
    for /r "%USERPROFILE%\AppData\Local\Arduino15\packages\esp32" %%b in (boot_app0.bin) do (
        if exist "%%b" (
            copy "%%b" ".\boot_app0.bin" >nul
            echo      Copiato: boot_app0.bin da %%b
            goto :boot_found
        )
    )
    echo      [ERRORE] boot_app0.bin non trovato!
    :boot_found
)

:: Genera immagine LittleFS dalla cartella data (2MB = 0x200000 = 2097152 bytes)
set MKLITTLEFS=%USERPROFILE%\AppData\Local\Arduino15\packages\esp32\tools\mklittlefs\3.0.0-gnu12-dc7f933\mklittlefs.exe
set DATA_DIR=..\data
if exist "%MKLITTLEFS%" (
    if exist "%DATA_DIR%" (
        echo      Generando immagine LittleFS da cartella data...
        "%MKLITTLEFS%" -c "%DATA_DIR%" -s 2097152 -p 256 -b 4096 ".\littlefs_2mb.bin"
        if %errorlevel% equ 0 (
            echo      Creato: littlefs_2mb.bin (2MB con file audio)
        ) else (
            echo      [ERRORE] Generazione LittleFS fallita!
        )
    ) else (
        echo      [AVVISO] Cartella data non trovata
    )
) else (
    echo      [AVVISO] mklittlefs.exe non trovato
    echo               Cercando immagine pre-generata...
    set LITTLEFS_IMG=..\mklittlefs_2mb.bin
    if exist "!LITTLEFS_IMG!" (
        copy "!LITTLEFS_IMG!" ".\littlefs_2mb.bin" >nul
        echo      Copiato: mklittlefs_2mb.bin -^> littlefs_2mb.bin
    ) else (
        echo      [ERRORE] Nessuna immagine LittleFS disponibile
    )
)

:: Crea file merged.bin per web flasher (combina tutti i file)
echo      Creando merged.bin per web flasher...
python -c "import sys; ^
f=open('merged.bin','wb'); ^
f.seek(0x0); f.write(open('bootloader.bin','rb').read()); ^
f.seek(0x8000); f.write(open('partitions.bin','rb').read()); ^
f.seek(0xe000); f.write(open('boot_app0.bin','rb').read()); ^
f.seek(0x10000); f.write(open('firmware.bin','rb').read()); ^
f.seek(0x310000); f.write(open('littlefs_2mb.bin','rb').read()); ^
f.close(); print('      Creato: merged.bin')"

echo.
echo ============================================
echo   COMPILAZIONE COMPLETATA CON SUCCESSO!
echo ============================================
echo.
echo File generati:
echo   - firmware.bin     (firmware principale @ 0x10000)
echo   - bootloader.bin   (bootloader @ 0x0)
echo   - partitions.bin   (tabella partizioni @ 0x8000)
echo   - boot_app0.bin    (boot app @ 0xe000)
echo   - littlefs_2mb.bin     (filesystem audio 2MB @ 0x310000)
echo   - merged.bin       (file unico per web flasher)
echo   - build\           (tutti i file di compilazione)
echo.
echo Per flashare:
echo   - Web: avvia_flasher.bat (apre browser)
echo   - USB: flash_diretto.bat (usa esptool)
echo.
pause
