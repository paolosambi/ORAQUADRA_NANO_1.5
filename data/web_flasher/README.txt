============================================
  oraQuadra Nano - Web Flasher
  Sistema di programmazione via browser
============================================

PREREQUISITI
------------
1. Arduino CLI installato
   - Download: https://arduino.github.io/arduino-cli/
   - Oppure: winget install Arduino.ArduinoCLI

2. Python installato (per il server locale)
   - Download: https://www.python.org/downloads/

3. Driver USB CH340/CP2102 installati

4. Browser compatibile: Chrome, Edge o Opera


COME USARE (Metodo Locale)
--------------------------
1. Esegui compile_and_deploy.bat
   - Compila lo sketch Arduino
   - Genera firmware.bin

2. Esegui start_server.bat
   - Avvia un server web locale sulla porta 8080

3. Apri Chrome e vai a: http://localhost:8080

4. Collega l'ESP32-S3 via USB

5. Clicca "Installa Firmware" e seleziona la porta COM


COME USARE (Hosting Online)
---------------------------
1. Compila lo sketch con compile_and_deploy.bat

2. Carica questi file su un hosting HTTPS:
   - index.html
   - manifest.json
   - firmware.bin

3. Apri l'URL del tuo hosting in Chrome


FILE CONTENUTI
--------------
- index.html          : Pagina web del flasher
- manifest.json       : Configurazione ESP Web Tools
- firmware.bin        : Firmware compilato (dopo compilazione)
- compile_and_deploy.bat : Script di compilazione
- start_server.bat    : Avvia server locale
- build/              : File di compilazione (dopo compilazione)


RISOLUZIONE PROBLEMI
--------------------
- "Porta COM non trovata": Installa i driver CH340/CP2102
- "Browser non supportato": Usa Chrome, Edge o Opera
- "Compilazione fallita": Verifica le librerie Arduino richieste
- "Connessione fallita": Prova un altro cavo USB (deve essere dati, non solo ricarica)


NOTE TECNICHE
-------------
- Board: ESP32-S3 Dev Module
- Partition: HUGE APP (per sketch grandi)
- PSRAM: OPI PSRAM
- Flash: 16MB
- Offset firmware: 0x10000 (65536)


============================================
Progetto oraQuadra di Paolo Sambinello
============================================
