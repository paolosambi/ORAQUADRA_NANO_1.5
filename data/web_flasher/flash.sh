#!/bin/bash

echo "============================================"
echo "  oraQuadra Nano - Flash Diretto Completo"
echo "============================================"
echo ""

# Verifica file necessari
if [ ! -f "firmware.bin" ]; then
    echo "[ERRORE] firmware.bin non trovato!"
    exit 1
fi

if [ ! -f "bootloader.bin" ]; then
    echo "[ERRORE] bootloader.bin non trovato!"
    exit 1
fi

if [ ! -f "partitions.bin" ]; then
    echo "[ERRORE] partitions.bin non trovato!"
    exit 1
fi

if [ ! -f "boot_app0.bin" ]; then
    echo "[ERRORE] boot_app0.bin non trovato!"
    exit 1
fi

FLASH_LITTLEFS=0
if [ -f "littlefs_2mb.bin" ]; then
    FLASH_LITTLEFS=1
else
    echo "[AVVISO] littlefs_2mb.bin non trovato - i file audio non saranno flashati"
fi

# Verifica Python
if ! command -v python3 &> /dev/null; then
    echo "[ERRORE] Python3 non trovato!"
    echo "Installa Python: brew install python3"
    exit 1
fi

echo "[1/3] Verifico esptool..."
if ! python3 -m pip show esptool &> /dev/null; then
    echo "     Installo esptool..."
    python3 -m pip install esptool
fi

# Installa pyserial se necessario
python3 -m pip show pyserial &> /dev/null || python3 -m pip install pyserial

echo ""
echo "[2/3] Cerco porte seriali disponibili..."
echo ""

# Lista le porte su Mac
ls /dev/cu.* 2>/dev/null || echo "  Nessuna porta trovata"

echo ""
echo "Su Mac le porte sono tipicamente:"
echo "  /dev/cu.usbserial-XXXX   (driver CH340/CP2102)"
echo "  /dev/cu.SLAB_USBtoUART   (driver Silabs)"
echo "  /dev/cu.wchusbserial*    (driver CH340)"
echo ""

read -p "Inserisci la porta (es. /dev/cu.usbserial-1410): " PORTA

echo ""
echo "[3/3] Flash COMPLETO in corso su $PORTA..."
echo "     Metti l'ESP32 in modalita BOOT se necessario"
echo "     (tieni premuto BOOT, premi RESET, rilascia BOOT)"
echo ""
echo "File da flashare:"
echo "  - bootloader.bin   @ 0x0"
echo "  - partitions.bin   @ 0x8000"
echo "  - boot_app0.bin    @ 0xe000"
echo "  - firmware.bin     @ 0x10000"
if [ $FLASH_LITTLEFS -eq 1 ]; then
    echo "  - littlefs_2mb.bin @ 0x310000"
fi
echo ""

if [ $FLASH_LITTLEFS -eq 1 ]; then
    python3 -m esptool --chip esp32s3 --port "$PORTA" --baud 921600 \
        --before default_reset --after hard_reset write_flash -z \
        --flash_mode dio --flash_freq 80m --flash_size 16MB \
        0x0 bootloader.bin \
        0x8000 partitions.bin \
        0xe000 boot_app0.bin \
        0x10000 firmware.bin \
        0x310000 littlefs_2mb.bin
else
    python3 -m esptool --chip esp32s3 --port "$PORTA" --baud 921600 \
        --before default_reset --after hard_reset write_flash -z \
        --flash_mode dio --flash_freq 80m --flash_size 16MB \
        0x0 bootloader.bin \
        0x8000 partitions.bin \
        0xe000 boot_app0.bin \
        0x10000 firmware.bin
fi

if [ $? -eq 0 ]; then
    echo ""
    echo "============================================"
    echo "  FLASH COMPLETATO CON SUCCESSO!"
    echo "============================================"
    echo ""
    echo "Premi RESET sull'ESP32 per avviare il firmware."
else
    echo ""
    echo "[ERRORE] Flash fallito!"
    echo "Verifica:"
    echo "  - La porta seriale e corretta"
    echo "  - L'ESP32 e in modalita BOOT"
    echo "  - Il cavo USB e un cavo dati"
fi
