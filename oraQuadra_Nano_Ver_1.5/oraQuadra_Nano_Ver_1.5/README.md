<p align="center">
  <img src="https://img.shields.io/badge/ESP32-S3-blue?style=for-the-badge&logo=espressif" alt="ESP32-S3">
  <img src="https://img.shields.io/badge/Arduino-IDE-00979D?style=for-the-badge&logo=arduino" alt="Arduino IDE">
  <img src="https://img.shields.io/badge/Version-1.5-green?style=for-the-badge" alt="Version 1.5">
  <img src="https://img.shields.io/badge/License-Open%20Source-orange?style=for-the-badge" alt="License">
</p>

<h1 align="center">OraQuadra Nano v1.5</h1>

<p align="center">
  <strong>Orologio interattivo a matrice LED 480x480 pixel basato su ESP32-S3</strong><br>
  24 modalita di visualizzazione | Touch Control | Alexa Integration | Web Server
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
- [Modalita di Visualizzazione](#modalita-di-visualizzazione)
- [Controllo Touch](#controllo-touch)
- [Integrazione Alexa](#integrazione-alexa)
- [Web Server](#web-server)
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
| **Display** | LCD touch 480x480 pixel a colori |
| **Modalita** | 24 effetti visuali, giochi arcade, orologi |
| **Touch** | Controllo a 4 zone interattive |
| **Web Server** | Configurazione completa via browser |
| **Alexa** | Integrazione vocale Amazon Alexa |
| **Audio I2S** | Annunci orari vocali tramite modulo I2S interno |
| **Sensori** | Temperatura, umidita, pressione (opzionali) |
| **Radar** | LD2410 24GHz per rilevamento movimento (opzionale)|
| **OTA** | Aggiornamento firmware via WiFi |
| **Lingue** | Supporto multilingua IT/EN todo|

---

## Hardware Richiesto

### Componente Principale

| Componente | Descrizione | Link |
|:---:|---|:---:|
| **ESP32-4848S040** | Display LCD 480x480 touch integrato (Guition 86BOX) | [AliExpress](https://s.click.aliexpress.com/e/_onmPeo3) |

### Sensori Opzionali

| Componente | Funzione |
|---|---|
| BME280 | Temperatura, pressione, umidita (I2C) (opzionale)|
| LD2410 | Radar 24GHz per movimento e luminosita automatica (opzionale)|
| Magnetometro | Bussola per Weather Station (opzionale)|
| ESP32-CAM Freenove | Streaming video MJPEG (opzionale)|
| SD Card | Skin personalizzate orologio analogico (FAT32) |

### Pinout

```
I2C:          SDA=19, SCL=45
Touch GT911:  Polling mode (INT=-1, RST=-1)
Audio I2S:    BCLK=1, LRC=2, DOUT=40
LD2410:       RX=2, TX=1 (UART 256000 baud)
SD Card:      CS=42, MOSI=47, CLK=48, MISO=41
Backlight:    Pin 38 (PWM)
```

---

## Modalita di Visualizzazione

### Effetti Base

| # | Modalita | Descrizione |
|:---:|---|---|
| 1 | **FADE** | Dissolvenza progressiva tra numeri |
| 2 | **SLOW** | Visualizzazione lenta dell'ora |
| 3 | **FAST** | Visualizzazione veloce diretta |
| 4 | **MATRIX** | Pioggia Matrix con 32 gocce LED |
| 5 | **MATRIX2** | Variante Matrix continua |
| 6 | **SNAKE** | Serpente che mangia le lettere |
| 7 | **WATER** | Effetto goccia d'acqua con onde |

### Modalita Arcade

| # | Modalita | Descrizione |
|:---:|---|---|
| 8 | **MARIO** | Mario Bros che disegna l'orario |
| 9 | **TRON** | 3 moto Tron con scie luminose |
| 10 | **GALAGA** | Astronave che spara alle lettere |
| 11 | **GALAGA2** | Variante con astronave volante |

### Orologi Specializzati

| # | Modalita | Descrizione |
|:---:|---|---|
| 12 | **ANALOG CLOCK** | Orologio analogico con skin JPG da SD |
| 13 | **FLIP CLOCK** | Split-flap stile anni '70 |
| 14 | **BTTF** | Quadrante DeLorean (Ritorno al Futuro) |
| 15 | **LED RING** | Orologio circolare LED + display digitale |
| 16 | **CLOCK** | Analogico con subdial meteo |

### Effetti Speciali

| # | Modalita | Descrizione |
|:---:|---|---|
| 17 | **FLUX CAPACITOR** | Flusso canalizzatore animato |
| 18 | **FIRE** | Effetto fuoco camino |
| 19 | **FIRE TEXT** | Lettere fiammeggianti |
| 20 | **CHRISTMAS** | Tema natalizio con neve |

### Integrazione

| # | Modalita | Descrizione |
|:---:|---|---|
| 21 | **WEATHER STATION** | Stazione meteo completa |
| 22 | **ESP32-CAM** | Streaming video da camera |

---

## Controllo Touch

Il display e diviso in **4 zone** con funzioni diverse:

```
+---------------------+---------------------+
|                     |                     |
|    CAMBIO MODO      |   PRESET COLORI     |
|   (alto-sinistra)   |    (alto-destra)    |
|                     |                     |
+---------------------+---------------------+
|                     |                     |
|  SELETTORE COLORE   |   LAMPEGGIO "E"     |
|  (basso-sinistra)   |   (basso-destra)    |
|                     |                     |
+---------------------+---------------------+
```

> **Reset WiFi**: Premere con 4 dita tutte le zone durante l'avvio.

---

## Integrazione Alexa

Il dispositivo viene riconosciuto come lampadina Alexa con nome **ORAQUADRANANO**.

**Comandi vocali supportati:**

```
"Alexa, accendi ORAQUADRANANO"
"Alexa, spegni ORAQUADRANANO"
"Alexa, ORAQUADRANANO colore [nome_colore]"
```

---

## Web Server

Accedere via browser a `http://[IP_DISPOSITIVO]:8080`

| Endpoint | Funzione |
|---|---|
| `/` | Homepage con monitor 16x16 |
| `/settings` | Configurazione generale |
| `/bttf` | Impostazioni Ritorno al Futuro |
| `/ledring` | Configurazione LED Ring |
| `/analogclock` | Skin e lancette orologio |
| `/radar` | Calibrazione LD2410 |

---

## Installazione

### Metodo 1: Programmazione Web (Consigliato)

> Nessuna compilazione richiesta!
DA IMPLEMENTARE 
1. Collegare il dispositivo via **USB**
2. Aprire **Chrome** e andare a: [Web Programmer](https://davidegatti.altervista.org/installaEsp32.html?progetto=oraQuadraNano)
3. Seguire le istruzioni a schermo

### Metodo 2: Compilazione Arduino IDE

#### Requisiti

| Componente | Versione |
|---|---|
| Arduino IDE | 1.8.x o 2.x |
| ESP32 Core | **2.0.17** (obbligatorio) |
| GFX Library for Arduino | **1.6.0** (obbligatorio) |

#### Librerie da Installare

```
Arduino_GFX_Library v1.6.0
ezTime
Espalexa
WiFiManager
ESPAsyncWebServer
AsyncTCP
JPEGDEC
TAMC_GT911
Adafruit BME280 Library
MyLD2410
U8g2
```

#### Impostazioni Board

| Parametro | Valore |
|---|---|
| Board | ESP32S3 Dev Module |
| Partition Scheme | **HUGE APP** |
| PSRAM | **OPI PSRAM** |
| Flash Size | 16MB |
| Upload Speed | 921600 |

#### Compilazione

1. Aprire `oraQuadra_Nano_Ver_1.5.ino`
2. Verificare le impostazioni della board
3. Compilare e caricare

---

## Struttura del Progetto

```
oraQuadra_Nano_Ver_1.5/
|
|-- oraQuadra_Nano_Ver_1.5.ino    # File principale
|-- 0_SETUP.ino                    # Inizializzazione
|-- 1_TOUCH.ino                    # Gestione touch
|-- 2_CHANGE_MODE.ino              # Cambio modalita
|-- 3_EFFETTI.ino                  # Effetti visuali
|-- 5_AUDIO.ino                    # Audio locale
|-- 7_AUDIO_WIFI.ino               # Audio WiFi
|-- 8_CLOCK.ino                    # Orologio analogico
|-- 10_FLIP_CLOCK.ino              # Flip clock
|-- 12_BTTF.ino                    # Back to the Future
|-- 18_LED_RING_CLOCK.ino          # LED Ring
|-- 19_WEATHER.ino                 # Dati meteo
|-- 20_WEATHER_STATION.ino         # Stazione meteo
|-- 26_WEBSERVER_SETTINGS.ino      # Web server
|-- 27_FLUX_CAPACITOR.ino          # Flux Capacitor
|-- 28_CHRISTMAS.ino               # Tema natalizio
|-- 29_FIRE.ino                    # Effetto fuoco
|-- 30_FIRE_TEXT.ino               # Testo fiammeggiante
|
|-- word_mappings.h                # Mappatura LED (IT)
|-- word_mappings_en.h             # Mappatura LED (EN)
|-- home_web_html.h                # HTML homepage
|-- settings_web_html.h            # HTML settings
|-- language_config.h              # Config multilingua
|
|-- data/                          # File audio MP3
|   |-- 0.mp3 - 59.mp3            # Numeri
|   |-- gennaio.mp3 - dicembre.mp3
|   +-- ...
|
+-- font/                          # Font DS-DIGI
```

---

## Configurazione

### Abilitare/Disabilitare Effetti

Nel file principale, commentare o decommentare le define:

```cpp
#define EFFECT_FADE           // Dissolvenza
#define EFFECT_SLOW           // Lento
#define EFFECT_FAST           // Veloce
#define EFFECT_MATRIX         // Matrix
#define EFFECT_MATRIX2        // Matrix 2
#define EFFECT_SNAKE          // Snake
#define EFFECT_WATER          // Goccia
#define EFFECT_MARIO          // Mario
#define EFFECT_TRON           // Tron
#define EFFECT_GALAGA         // Galaga
#define EFFECT_GALAGA2        // Galaga 2
#define EFFECT_ANALOG_CLOCK   // Orologio analogico
#define EFFECT_FLIP_CLOCK     // Flip clock
#define EFFECT_BTTF           // Back to the Future
#define EFFECT_LED_RING       // LED Ring
#define EFFECT_WEATHER_STATION // Stazione meteo
#define EFFECT_CLOCK          // Orologio con meteo
#define EFFECT_ESP32CAM       // Streaming camera
#define EFFECT_FLUX_CAPACITOR // Flux Capacitor
#define EFFECT_CHRISTMAS      // Natale
#define EFFECT_FIRE           // Fuoco
#define EFFECT_FIRE_TEXT      // Testo fiammeggiante
```

### Skin Orologio Analogico

1. Formattare una SD Card in **FAT32**
2. Copiare immagini **JPG 480x480 pixel** nella root
3. Inserire la SD nel dispositivo
4. Configurare via web in `/analogclock`

---

## Changelog

### v1.5 (24/01/2026) - Attuale by Marco Camerani
- **Attivato modulo interno I2S per audio**
- **Riproduzione corretta dell'orario vocale**
- Miglioramenti generali

### v1.3.1 (07/11/2025)
- Aggiunta modalita Ritorno al Futuro (BTTF)
- Easter egg a sorpresa

### v1.3 (05/11/2025)
- Aggiunta modalita Galaga
- Orologio analogico con skin da SD card
- Configurazione lancette via web

### v1.2 (24/10/2025)
- Aggiunte modalita Mario e Tron

### v1.1 (21/10/2025)
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
    <td align="center"><sub>Sviluppatore</sub></td>
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
| File STL 3D | [GitHub](https://github.com/SurvivalHacking/OraquadraNano) |
| Programmatore Web | [Web Programmer](https://davidegatti.altervista.org/installaEsp32.html?progetto=oraQuadraNano) |

---

## Supporto

Hai bisogno di aiuto?

- Apri una [Issue su GitHub](https://github.com/SurvivalHacking/OraquadraNano/issues)
- Visita [SurvivalHacking.it](https://www.survivalhacking.it)

---

<p align="center">
  <sub>Creato con passione da <a href="https://www.survivalhacking.it">Survival Hacking</a></sub>
</p>
