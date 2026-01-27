<p align="center">
  <img src="https://img.shields.io/badge/ESP32-S3-blue?style=for-the-badge&logo=espressif" alt="ESP32-S3">
  <img src="https://img.shields.io/badge/Arduino-IDE-00979D?style=for-the-badge&logo=arduino" alt="Arduino IDE">
  <img src="https://img.shields.io/badge/Version-1.5-green?style=for-the-badge" alt="Version 1.5">
  <img src="https://img.shields.io/badge/License-Open%20Source-orange?style=for-the-badge" alt="License">
</p>

<h1 align="center">OraQuadra Nano v1.5</h1>

<p align="center">
  <strong>Orologio interattivo a matrice LED 16x16 (256 LED virtuali) su display LCD 480x480 pixel basato su ESP32-S3</strong><br>
  24 modalita di visualizzazione | Touch Control | Alexa Integration | Web Server | Audio I2S
</p>

<p align="center">
  <a href="https://youtu.be/fNRnZvtF9N0"><img src="https://img.shields.io/badge/YouTube-Video%20Tutorial-FF0000?style=flat-square&logo=youtube" alt="YouTube"></a>
  <a href="https://github.com/SurvivalHacking/OraquadraNano"><img src="https://img.shields.io/badge/GitHub-Repository-181717?style=flat-square&logo=github" alt="GitHub"></a>
  <a href="https://www.survivalhacking.it"><img src="https://img.shields.io/badge/Web-SurvivalHacking.it-4285F4?style=flat-square&logo=google-chrome" alt="Website"></a>
</p>

---

## Indice

- [Caratteristiche](#caratteristiche)
- [Hardware Richiesto](#hardware-richiesto)
- [Specifiche Tecniche](#specifiche-tecniche)
- [Modalita di Visualizzazione](#modalita-di-visualizzazione)
- [Controllo Touch](#controllo-touch)
- [Integrazione Alexa](#integrazione-alexa)
- [Web Server](#web-server)
- [Sistema Audio](#sistema-audio)
- [Installazione](#installazione)
- [Struttura del Progetto](#struttura-del-progetto)
- [Configurazione](#configurazione)
- [Changelog](#changelog)
- [Autori](#autori)
- [Supporto](#supporto)

---

## Caratteristiche

| Funzionalita | Descrizione |
|:---:|---|
| **Display** | LCD touch 480x480 pixel a colori con matrice virtuale 16x16 LED |
| **Modalita** | 24 effetti visuali, giochi arcade, orologi specializzati |
| **Touch** | Controllo a 4 zone interattive con debounce 300ms |
| **Web Server** | Configurazione completa via browser (porta 8080) |
| **Alexa** | Integrazione vocale Amazon Alexa (Espalexa) |
| **Audio I2S** | Annunci orari vocali tramite modulo I2S interno |
| **Sensori** | BME280 (temperatura, umidita, pressione) opzionale |
| **Radar** | LD2410 24GHz per rilevamento presenza e luminosita automatica |
| **OTA** | Aggiornamento firmware via WiFi (ArduinoOTA) |
| **EEPROM** | Memorizzazione persistente impostazioni (1024 byte) |
| **SD Card** | Skin personalizzate orologio analogico (FAT32) |
| **mDNS** | Accessibile come `oraquadra.local` sulla rete |

---

## Hardware Richiesto

### Componente Principale

| Componente | Descrizione | Link |
|:---:|---|:---:|
| **ESP32-4848S040** | Display LCD 480x480 touch integrato (Guition 86BOX) | [AliExpress](https://s.click.aliexpress.com/e/_onmPeo3) |

### Sensori Opzionali

| Componente | Funzione | Interfaccia |
|---|---|---|
| BME280 | Temperatura, pressione, umidita | I2C (0x76 o 0x77) |
| LD2410 | Radar 24GHz per movimento e luminosita automatica | UART 256000 baud |
| Magnetometro | Bussola per Weather Station | I2C |
| ESP32-CAM Freenove | Streaming video MJPEG | WiFi |
| SD Card | Skin personalizzate orologio analogico | SPI (FAT32) |

---

## Specifiche Tecniche

### Configurazione Hardware

```
MCU:              ESP32-S3 con PSRAM OPI
Display:          LCD 480x480 RGB Panel (GFX_DEV_DEVICE ESP32_4848S040_86BOX_GUITION)
Matrice Virtuale: 16x16 = 256 LED virtuali
Touch:            GT911 Capacitive Touch Controller (polling mode)
```

### Pinout Completo

```
I2C Bus:
  SDA             Pin 19
  SCL             Pin 45

Touch GT911:
  INT             -1 (polling mode)
  RST             -1 (no software reset)

Audio I2S:
  BCLK            Pin 1
  LRC             Pin 2
  DOUT            Pin 40
  ENABLE          -1 (non usato)

Radar LD2410:
  RX (ESP32)      Pin 2 (collegato a TX radar)
  TX (ESP32)      Pin 1 (collegato a RX radar)
  Baud Rate       256000

SD Card (SPI):
  CS              Pin 42
  MOSI            Pin 47
  CLK             Pin 48
  MISO            Pin 41

Backlight:
  PWM Pin         Pin 38
  PWM Channel     0
  PWM Freq        1000 Hz
  PWM Resolution  8 bit (0-255)
```

### Configurazione Software

```
Flash Size:       16MB
Partition:        HUGE APP
PSRAM:            OPI PSRAM
ESP32 Core:       2.0.17 (obbligatorio)
GFX Library:      1.6.0 (obbligatorio)
EEPROM Size:      1024 byte
```

### Parametri Display

| Parametro | Valore |
|---|---|
| Luminosita Giorno (default) | 250 PWM |
| Luminosita Notte (default) | 90 PWM |
| Ora Inizio Giorno (default) | 07:00 |
| Ora Inizio Notte (default) | 19:00 |

---

## Modalita di Visualizzazione

### Effetti Base (MODE 0-6)

| # | Modalita | Descrizione | Define |
|:---:|---|---|---|
| 0 | **FADE** | Dissolvenza progressiva tra numeri | `EFFECT_FADE` |
| 1 | **SLOW** | Visualizzazione lenta dell'ora | `EFFECT_SLOW` |
| 2 | **FAST** | Visualizzazione veloce diretta | `EFFECT_FAST` |
| 3 | **MATRIX** | Pioggia Matrix con 32 gocce LED | `EFFECT_MATRIX` |
| 4 | **MATRIX2** | Variante Matrix continua | `EFFECT_MATRIX2` |
| 5 | **SNAKE** | Serpente che mangia le lettere (max 30 segmenti) | `EFFECT_SNAKE` |
| 6 | **WATER** | Effetto goccia d'acqua con onde (raggio max 20px) | `EFFECT_WATER` |

### Modalita Arcade (MODE 7-9, 17)

| # | Modalita | Descrizione | Define |
|:---:|---|---|---|
| 7 | **MARIO** | Mario Bros che disegna l'orario | `EFFECT_MARIO` |
| 8 | **TRON** | 3 moto Tron con scie luminose (8 LED trail) | `EFFECT_TRON` |
| 9 | **GALAGA** | Astronave che spara alle lettere | `EFFECT_GALAGA` |
| 17 | **GALAGA2** | Variante con astronave volante | `EFFECT_GALAGA2` |

### Orologi Specializzati (MODE 10-15)

| # | Modalita | Descrizione | Define |
|:---:|---|---|---|
| 10 | **ANALOG CLOCK** | Orologio analogico con skin JPG da SD | `EFFECT_ANALOG_CLOCK` |
| 11 | **FLIP CLOCK** | Split-flap stile anni '70 | `EFFECT_FLIP_CLOCK` |
| 12 | **BTTF** | Quadrante DeLorean (Ritorno al Futuro) | `EFFECT_BTTF` |
| 13 | **LED RING** | Orologio circolare LED + display digitale | `EFFECT_LED_RING` |
| 14 | **WEATHER STATION** | Stazione meteo completa | `EFFECT_WEATHER_STATION` |
| 15 | **CLOCK** | Analogico con subdial meteo (richiede BME280) | `EFFECT_CLOCK` |

### Effetti Speciali (MODE 19-23)

| # | Modalita | Descrizione | Define |
|:---:|---|---|---|
| 19 | **ESP32-CAM** | Streaming video da camera | `EFFECT_ESP32CAM` |
| 20 | **FLUX CAPACITOR** | Flusso canalizzatore animato | `EFFECT_FLUX_CAPACITOR` |
| 21 | **CHRISTMAS** | Tema natalizio con neve | `EFFECT_CHRISTMAS` |
| 22 | **FIRE** | Effetto fuoco camino (Fire2012) | `EFFECT_FIRE` |
| 23 | **FIRE TEXT** | Lettere fiammeggianti | `EFFECT_FIRE_TEXT` |

---

## Controllo Touch

Il display e diviso in **4 zone** con funzioni diverse:

```
+---------------------+---------------------+
|                     |                     |
|    CAMBIO MODO      |   PRESET COLORI     |
|   (alto-sinistra)   |    (alto-destra)    |
|      0-239 X        |     240-480 X       |
|      0-239 Y        |      0-239 Y        |
+---------------------+---------------------+
|                     |                     |
|  SELETTORE COLORE   |   LAMPEGGIO "E"     |
|  (basso-sinistra)   |   (basso-destra)    |
|      0-239 X        |     240-480 X       |
|     240-480 Y       |    240-480 Y        |
+---------------------+---------------------+
```

### Funzioni Touch

| Zona | Funzione | Comportamento |
|---|---|---|
| Alto-Sinistra | Cambio Modalita | Cicla tra le 24 modalita |
| Alto-Destra | Preset Colori | Cicla tra 14 preset colore |
| Basso-Sinistra | Selettore Colore | Tenere premuto per scorrere i colori |
| Basso-Destra | Lampeggio "E" | Attiva/disattiva lampeggio separatore |

### Comandi Speciali

| Azione | Risultato |
|---|---|
| 4 dita all'avvio | Reset impostazioni WiFi |
| Long press (>600ms) | Azione speciale in alcune modalita |

**Debounce:** 300ms tra tocchi consecutivi

---

## Integrazione Alexa

Il dispositivo viene riconosciuto come lampadina Alexa con nome **ORAQUADRANANO**.

### Comandi Vocali Supportati

```
"Alexa, accendi ORAQUADRANANO"       -> Accende il display
"Alexa, spegni ORAQUADRANANO"        -> Spegne il display
"Alexa, ORAQUADRANANO colore rosso"  -> Cambia colore
"Alexa, ORAQUADRANANO luminosita 50" -> Regola luminosita
```

### Colori Supportati

Rosso, Verde, Blu, Giallo, Arancione, Rosa, Viola, Bianco, Ciano, Magenta

---

## Web Server

Accedere via browser a `http://[IP_DISPOSITIVO]:8080` oppure `http://oraquadra.local:8080`

### Endpoint Disponibili

| Endpoint | Funzione |
|---|---|
| `/` | Homepage con monitor 16x16 in tempo reale |
| `/settings` | Configurazione generale (luminosita, orari, audio) |
| `/bttf` | Impostazioni Ritorno al Futuro (date, coordinate) |
| `/ledring` | Configurazione LED Ring (colori ore/minuti/secondi) |
| `/analogclock` | Skin e lancette orologio analogico |
| `/radar` | Calibrazione LD2410 e controllo luminosita automatica |

### Servizi di Rete

| Servizio | Porta | Descrizione |
|---|---|---|
| Web Server | 8080 | Interfaccia configurazione |
| Espalexa | 80 | Discovery Amazon Alexa |
| OTA | 3232 | Aggiornamenti firmware |
| mDNS | - | `oraquadra.local` |

---

## Sistema Audio

### Audio I2S Interno

Il sistema utilizza un modulo I2S interno per la riproduzione audio:

```
Formato:          MP3
Sorgente:         LittleFS (/data/)
Sample Rate:      44100 Hz
Bit Depth:        16 bit
Canali:           Stereo
Volume Default:   80%
```

### Annunci Orari

| Parametro | Valore Default |
|---|---|
| Orario Inizio | 08:00 |
| Orario Fine | 19:59 |
| Volume Giorno | 80% |
| Volume Notte | 30% |

### File Audio Richiesti

La cartella `/data/` deve contenere:
- `0.mp3` - `59.mp3` (numeri)
- File mesi e giorni per annunci completi

---

## Installazione

### Metodo 1: Programmazione Web (Consigliato)

> Nessuna compilazione richiesta!

1. Collegare il dispositivo via **USB**
2. Aprire **Chrome** e andare a: [Web Programmer](https://davidegatti.altervista.org/installaEsp32.html?progetto=oraQuadraNano)
3. Seguire le istruzioni a schermo

### Metodo 2: Compilazione Arduino IDE

#### Requisiti Obbligatori

| Componente | Versione | Note |
|---|---|---|
| Arduino IDE | 1.8.x o 2.x | |
| ESP32 Core | **2.0.17** | OBBLIGATORIO |
| GFX Library for Arduino | **1.6.0** | OBBLIGATORIO |

#### Librerie da Installare

```
Arduino_GFX_Library v1.6.0    - Display grafico
ezTime                        - Gestione ora/data NTP
Espalexa                      - Integrazione Alexa
WiFiManager                   - Configurazione WiFi
ESPAsyncWebServer             - Web server asincrono
AsyncTCP                      - TCP asincrono
JPEGDEC                       - Decodifica immagini JPG
TAMC_GT911                    - Touch screen controller
Adafruit BME280 Library       - Sensore ambientale
MyLD2410                      - Radar LD2410
U8g2                          - Font grafici
AudioFileSourceLittleFS       - Sorgente audio
AudioFileSourceBuffer         - Buffer audio
AudioGeneratorMP3             - Decoder MP3
AudioOutputI2S                - Output audio I2S
```

#### Impostazioni Board

| Parametro | Valore |
|---|---|
| Board | ESP32S3 Dev Module |
| Partition Scheme | **HUGE APP** |
| PSRAM | **OPI PSRAM** |
| Flash Size | 16MB |
| Upload Speed | 921600 |
| CPU Frequency | 240MHz |

#### Compilazione

1. Aprire `oraQuadra_Nano_Ver_1.5.ino`
2. Verificare le impostazioni della board
3. Caricare i file audio su LittleFS (Tools -> ESP32 Sketch Data Upload)
4. Compilare e caricare il firmware

---

## Struttura del Progetto

```
oraQuadra_Nano_Ver_1.5/
|
|-- oraQuadra_Nano_Ver_1.5.ino    # File principale (configurazione, define, struct)
|-- 0_SETUP.ino                    # Inizializzazione EEPROM, Display, WiFi, OTA, Alexa
|-- 1_TOUCH.ino                    # Gestione touch screen 4 zone
|-- 2_CHANGE_MODE.ino              # Cambio modalita visualizzazione
|-- 3_EFFETTI.ino                  # Effetti visuali base (Fade, Matrix, Snake, Water)
|-- 4_MENU.ino                     # Menu di configurazione
|-- 4_WEBSERVER_CLOCK.ino          # Web server orologio analogico
|-- 5_WEBSERVER_LED_RING.ino       # Web server LED Ring
|-- 7_AUDIO_WIFI.ino               # Audio WiFi esterno (ESP32C3)
|-- 8_CLOCK.ino                    # Orologio analogico con subdial meteo
|-- 10_FLIP_CLOCK.ino              # Flip clock vintage
|-- 11_FLIP_CLOCK_UTILS.ino        # Utility flip clock
|-- 12_BTTF.ino                    # Back to the Future mode
|-- 13_WEBSERVER_BTTF.ino          # Web server BTTF
|-- 14_CAPODANNO.ino               # Effetto capodanno
|-- 15_WEBSERVER_RADAR.ino         # Web server radar LD2410
|-- 18_LED_RING_CLOCK.ino          # LED Ring clock
|-- 19_WEATHER.ino                 # Dati meteo (Open Meteo API)
|-- 20_WEATHER_STATION.ino         # Stazione meteo completa
|-- 21_MJPEG_STREAM.ino            # Streaming MJPEG
|-- 22_WEBSERVER_MJPEG.ino         # Web server MJPEG
|-- 23_ESP32CAM_STREAM.ino         # Streaming ESP32-CAM
|-- 24_WEBSERVER_ESP32CAM.ino      # Web server ESP32-CAM
|-- 25_BLUETOOTH_AUDIO.ino         # Audio Bluetooth
|-- 25_MAGNETOMETER.ino            # Bussola magnetometro
|-- 26_WEBSERVER_SETTINGS.ino      # Web server impostazioni generali
|-- 27_FLUX_CAPACITOR.ino          # Flux Capacitor animato
|-- 28_CHRISTMAS.ino               # Tema natalizio
|-- 29_FIRE.ino                    # Effetto fuoco
|-- 30_FIRE_TEXT.ino               # Testo fiammeggiante
|-- 31_MP3_PLAYER.ino              # Lettore MP3 (disabilitato)
|
|-- bttf_types.h                   # Struct per modalita BTTF
|-- bttf_web_html.h                # HTML pagina web BTTF
|-- home_web_html.h                # HTML homepage
|-- language_config.h              # Configurazione multilingua
|-- language_strings.h             # Stringhe tradotte
|-- translations.h                 # Sistema traduzioni
|-- word_mappings.h                # Mappatura LED italiano
|-- word_mappings_en.h             # Mappatura LED inglese
|-- qrcode_wifi.h/.c               # Generazione QR code WiFi
|-- sh_logo_480x480new_black.h     # Logo avvio (PROGMEM)
|
|-- data/                          # File audio MP3 (LittleFS)
|   |-- 0.mp3 - 59.mp3            # Numeri
|   |-- gennaio.mp3 - dicembre.mp3 # Mesi
|   +-- ...
|
+-- font/                          # Font DS-DIGI per flip clock
```

---

## Configurazione

### Abilitare/Disabilitare Effetti

Nel file principale `oraQuadra_Nano_Ver_1.5.ino`, commentare o decommentare le define:

```cpp
// ================== ABILITA/DISABILITA EFFETTI ==================
#define EFFECT_FADE           // Effetto dissolvenza
#define EFFECT_SLOW           // Effetto lento
#define EFFECT_FAST           // Effetto veloce (visualizzazione diretta)
#define EFFECT_MATRIX         // Effetto pioggia Matrix
#define EFFECT_MATRIX2        // Effetto pioggia Matrix variante 2
#define EFFECT_SNAKE          // Effetto serpente
#define EFFECT_WATER          // Effetto goccia d'acqua
#define EFFECT_MARIO          // Effetto Mario Bros
#define EFFECT_TRON           // Effetto moto Tron
#define EFFECT_GALAGA         // Effetto Galaga (cannone)
#define EFFECT_GALAGA2        // Effetto Galaga 2 (astronave volante)
#define EFFECT_ANALOG_CLOCK   // Orologio analogico con skin da SD
#define EFFECT_FLIP_CLOCK     // Flip clock stile vintage
#define EFFECT_BTTF           // Back to the Future
#define EFFECT_LED_RING       // LED Ring clock
#define EFFECT_WEATHER_STATION // Stazione meteo completa
#define EFFECT_CLOCK          // Orologio con subdial meteo
#define EFFECT_ESP32CAM       // Streaming camera
#define EFFECT_FLUX_CAPACITOR // Flux Capacitor
#define EFFECT_CHRISTMAS      // Tema natalizio
#define EFFECT_FIRE           // Effetto fuoco
#define EFFECT_FIRE_TEXT      // Testo fiammeggiante
```

### Configurazione Audio

```cpp
#define AUDIO                 // Abilita audio I2S interno
#define ANNOUNCE_START_HOUR 8 // Ora inizio annunci (8:00)
#define ANNOUNCE_END_HOUR   20 // Ora fine annunci (19:59)
```

### Skin Orologio Analogico

1. Formattare una SD Card in **FAT32**
2. Copiare immagini **JPG 480x480 pixel** nella root
3. Inserire la SD nel dispositivo
4. Configurare via web in `/analogclock`

### Mappa Indirizzi EEPROM

| Indirizzo | Funzione |
|---|---|
| 0 | Marker configurazione (0x55) |
| 1 | Preset colore corrente |
| 3 | Modalita visualizzazione |
| 4 | Ora inizio giorno |
| 5 | Ora inizio notte |
| 6 | Annuncio orario abilitato |
| 7 | Luminosita giorno |
| 8 | Luminosita notte |
| 9 | Volume audio |
| 10-41 | SSID WiFi (32 byte) |
| 42-105 | Password WiFi (64 byte) |
| 106 | Flag validita WiFi |
| 130-132 | Colore RGB utente |
| 133 | Stato data orologio |
| 175 | Controllo luminosita radar |
| 500 | Versione layout EEPROM |

---

## Changelog

### v1.5 (24/01/2026) - Attuale
**by Marco Camerani**
- Attivato modulo interno I2S per audio
- Riproduzione corretta dell'orario vocale
- Correzioni e miglioramenti generali
- Supporto multilingua IT/EN (in sviluppo)

### v1.3.1 (07/11/2025)
**by Paolo Sambinello**
- Aggiunta modalita Ritorno al Futuro (BTTF)
- Easter egg a sorpresa

### v1.3 (05/11/2025)
**by Paolo Sambinello**
- Aggiunta modalita Galaga
- Orologio analogico con skin da SD card
- Configurazione lancette via web

### v1.2 (24/10/2025)
**by Paolo Sambinello**
- Aggiunte modalita Mario e Tron

### v1.1 (21/10/2025)
**by Paolo Sambinello**
- Integrazione Alexa funzionante
- QR code per configurazione WiFi
- Migliorata modalita Snake

### v1.0
- Release iniziale

---

## Autori

<table>
  <tr>
    <td align="center"><strong>Paolo Sambinello</strong></td>
    <td align="center"><strong>Alessandro Spagnoletti</strong></td>
    <td align="center"><strong>Davide Gatti</strong></td>
    <td align="center"><strong>Marco Camerani</strong></td>
  </tr>
  <tr>
    <td align="center"><sub>Sviluppatore Principale</sub></td>
    <td align="center"><sub>Sviluppatore</sub></td>
    <td align="center"><sub>Sviluppatore</sub></td>
    <td align="center"><sub>Audio I2S v1.5</sub></td>
  </tr>
</table>

<p align="center">
  <a href="https://www.survivalhacking.it"><strong>Survival Hacking</strong></a>
</p>

---

## Risorse

| Risorsa | Link |
|---|---|
| Video Tutorial | [YouTube](https://youtu.be/fNRnZvtF9N0) |
| Progetto ORAQUADRA 2 | [YouTube](https://youtu.be/DiFU6ITK8QQ) |
| Repository GitHub | [GitHub](https://github.com/SurvivalHacking/OraquadraNano) |
| File STL 3D | [GitHub](https://github.com/SurvivalHacking/OraquadraNano) |
| Programmatore Web | [Web Programmer](https://davidegatti.altervista.org/installaEsp32.html?progetto=oraQuadraNano) |
| Modulo ESP32-4848S040 | [AliExpress](https://s.click.aliexpress.com/e/_onmPeo3) |

---

## Supporto

Hai bisogno di aiuto?

- Apri una [Issue su GitHub](https://github.com/SurvivalHacking/OraquadraNano/issues)
- Visita [SurvivalHacking.it](https://www.survivalhacking.it)

---

<p align="center">
  <sub>Creato con passione da <a href="https://www.survivalhacking.it">Survival Hacking</a></sub><br>
  <sub>OraQuadra Nano v1.5 - ESP32-S3 Word Clock</sub>
</p>
