<p align="center">
  <img src="https://img.shields.io/badge/ESP32-S3-blue?style=for-the-badge&logo=espressif" alt="ESP32-S3">
  <img src="https://img.shields.io/badge/Arduino-IDE-00979D?style=for-the-badge&logo=arduino" alt="Arduino IDE">
  <img src="https://img.shields.io/badge/Version-1.5-green?style=for-the-badge" alt="Version 1.5">
  <img src="https://img.shields.io/badge/License-Open%20Source-orange?style=for-the-badge" alt="License">
</p>

---

<h2 align="center">UN GRANDE RINGRAZIAMENTO A MARCO CAMERANI</h2>
<h3 align="center">SENZA IL SUO AIUTO QUESTA RELEASE NON SAREBBE NATA</h3>

---

<h1 align="center">OraQuadra Nano v1.5</h1>

<p align="center">
  <strong>Orologio interattivo a matrice LED 16x16 (256 LED virtuali) su display LCD 480x480 pixel basato su ESP32-S3</strong><br>
  33 modalita di visualizzazione | Touch Control | Alexa Integration | Web Server | Audio I2S | Web Radio | Radiosveglia | Calendario | YouTube Stats | News RSS | LED RGB WS2812 | Multi-Display ESP-NOW | Giochi Dual Display | Emulatore Arcade Z80 | BLE Gamepad
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
- [Calendario](#calendario)
- [LED RGB WS2812](#led-rgb-ws2812)
- [YouTube Stats](#youtube-stats)
- [News Feed RSS](#news-feed-rss)
- [Multi-Display ESP-NOW](#multi-display-esp-now)
- [Giochi Dual Display](#giochi-dual-display)
- [Testo Scorrevole](#testo-scorrevole)
- [Emulatore Arcade Z80](#emulatore-arcade-z80)
- [BLE Gamepad](#ble-gamepad)
- [OTA Firmware Update](#ota-firmware-update)
- [Installazione](#installazione)
- [Impostazioni Arduino IDE](#impostazioni-arduino-ide)
- [Struttura del Progetto](#struttura-del-progetto)
- [Configurazione](#configurazione)
- [Mappa EEPROM](#mappa-eeprom)
- [Changelog](#changelog)
- [Autori](#autori)
- [Supporto](#supporto)

---

## Caratteristiche

| Funzionalita | Descrizione |
|:---:|---|
| **Display** | LCD touch 480x480 pixel a colori con matrice virtuale 16x16 LED |
| **Modalita** | 33 effetti visuali, giochi arcade, orologi specializzati |
| **Touch** | Controllo a 4 zone interattive con debounce 300ms + mode selector overlay |
| **Web Server** | Configurazione completa via browser (porta 8080) |
| **Alexa** | Integrazione vocale Amazon Alexa (Espalexa) |
| **Audio I2S** | Annunci orari vocali tramite modulo I2S interno |
| **Web Radio** | Streaming radio internet con VU meter e lista stazioni configurabile |
| **Radiosveglia** | Sveglia programmabile con selezione stazione radio, snooze e giorni |
| **Sensori** | BME280 (temperatura, umidita, pressione) opzionale |
| **Radar** | LD2410 24GHz per rilevamento presenza e luminosita automatica |
| **OTA** | Aggiornamento firmware e LittleFS via WiFi (browser o ArduinoOTA) |
| **EEPROM** | Memorizzazione persistente impostazioni (1024 byte) |
| **SD Card** | Skin personalizzate, stazioni radio, brani MP3 (FAT32) |
| **Calendario** | Calendario con vista mensile/giornaliera, eventi locali e Google Calendar |
| **LED RGB** | 12 LED WS2812 con colori tematici, audio reactive e override manuale |
| **YouTube** | Statistiche canale YouTube in tempo reale (iscritti, views, video) |
| **News RSS** | Feed notizie da ANSA, BBC, CNN con categorie e navigazione touch |
| **Multi-Display** | Fino a 4 pannelli sincronizzati via ESP-NOW (2x2 grid, 960x960px) |
| **Giochi** | PONG, Breakout, Battaglia Navale per Dual Display (2 pannelli) |
| **Scroll Text** | Testo scorrevole con 19 font, effetti rainbow/fade, 10 messaggi |
| **Arcade** | Emulatore Z80 con 12 giochi (Pac-Man, Galaga, Donkey Kong, Frogger...) |
| **BLE Gamepad** | Supporto gamepad Bluetooth per Arcade (8BitDo, Xbox, generico) |
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
| LED WS2812 | Striscia 12 LED RGB indirizzabili | GPIO43 (data) |
| SD Card | Skin personalizzate orologio analogico | SPI (FAT32) |
| 2o ESP32-4848S040 | Secondo pannello per Multi-Display / Giochi | ESP-NOW WiFi |
| Gamepad BLE | Controller Bluetooth per Arcade (8BitDo, Xbox) | BLE HID |

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

LED WS2812:
  Data            Pin 43 (GPIO43 libero - Serial usa USB-CDC)
  Num LED         12

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

### Modalita Arcade Animate (MODE 7-9, 17)

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

### Effetti Speciali (MODE 19-20)

| # | Modalita | Descrizione | Define |
|:---:|---|---|---|
| 19 | **ESP32-CAM** | Streaming video da camera | `EFFECT_ESP32CAM` |
| 20 | **FLUX CAPACITOR** | Flusso canalizzatore animato | `EFFECT_FLUX_CAPACITOR` |

### Multimedia e Audio (MODE 21-23)

| # | Modalita | Descrizione | Define |
|:---:|---|---|---|
| 21 | **MP3 PLAYER** | Lettore MP3/WAV da SD card con interfaccia touch | `EFFECT_MP3_PLAYER` |
| 22 | **WEB RADIO** | Streaming radio internet con VU meter e lista stazioni | `EFFECT_WEB_RADIO` |
| 23 | **RADIO ALARM** | Radiosveglia con selezione stazione, snooze e programmazione giorni | `EFFECT_RADIO_ALARM` |

### Calendario e Info (MODE 25-27)

| # | Modalita | Descrizione | Define |
|:---:|---|---|---|
| 25 | **CALENDAR** | Calendario mensile/giornaliero con eventi locali e Google Calendar | `EFFECT_CALENDAR` |
| 26 | **YOUTUBE** | Statistiche canale YouTube (iscritti, visualizzazioni, video) | `EFFECT_YOUTUBE` |
| 27 | **NEWS** | Feed notizie RSS da ANSA, BBC, CNN con categorie navigabili | `EFFECT_NEWS` |

### Giochi Dual Display (MODE 28, 30-31) - Richiedono 2 pannelli

| # | Modalita | Descrizione | Define |
|:---:|---|---|---|
| 28 | **PONG** | Tennis a 2 giocatori su 960x480px (master=sx, slave=dx) | `EFFECT_PONG` |
| 30 | **BREAKOUT** | Gioco mattoncini su 960x480px (80 bricks, 5 livelli velocita) | `EFFECT_BREAKOUT` |
| 31 | **BATTLESHIP** | Battaglia Navale classica (5 navi, griglia 10x10, vista privata per giocatore) | `EFFECT_BATTLESHIP` |

### Testo e Multimedia Avanzato (MODE 29)

| # | Modalita | Descrizione | Define |
|:---:|---|---|---|
| 29 | **SCROLL TEXT** | Testo scorrevole con 19 font, effetti rainbow/fade, 10 messaggi | `EFFECT_SCROLLTEXT` |

### Emulatore Arcade (MODE 32)

| # | Modalita | Descrizione | Define |
|:---:|---|---|---|
| 32 | **ARCADE** | Emulatore Z80 con 12 giochi classici, audio sintetizzato, gamepad BLE | `EFFECT_ARCADE` |

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
| Alto-Sinistra | Cambio Modalita | Cicla tra le 33 modalita |
| Alto-Destra | Preset Colori | Cicla tra 14 preset colore |
| Basso-Sinistra | Selettore Colore | Tenere premuto per scorrere i colori |
| Basso-Destra | Lampeggio "E" | Attiva/disattiva lampeggio separatore |
| Centro-Sinistra | Mode Selector | Apre overlay griglia con tutte le modalita |

### Comandi Speciali

| Azione | Risultato |
|---|---|
| 4 dita all'avvio | Reset impostazioni WiFi |
| Long press (>600ms) | Azione speciale in alcune modalita |
| Mode Selector | Griglia scorrevole con tutte le 33 modalita (timeout 15s) |

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
| `/settings` | Configurazione generale (luminosita, orari, audio, API keys) |
| `/bttf` | Impostazioni Ritorno al Futuro (date, coordinate) |
| `/ledring` | Configurazione LED Ring (colori ore/minuti/secondi) |
| `/analogclock` | Skin e lancette orologio analogico |
| `/radar` | Calibrazione LD2410 e controllo luminosita automatica |
| `/webradio` | Gestione stazioni Web Radio con controlli di riproduzione |
| `/radioalarm` | Programmazione radiosveglia (orario, giorni, stazione, volume) |
| `/calendar` | Gestione eventi calendario (locali + Google Calendar) |
| `/ledrgb` | Controllo LED RGB WS2812 (on/off, luminosita, colore, audio reactive) |
| `/youtube` | Statistiche canale YouTube in tempo reale |
| `/news` | Feed notizie RSS con selezione fonte e categoria |
| `/dualdisplay` | Configurazione Multi-Display (griglia, ruoli, peer, scan, test) |
| `/pong` | Stato partita PONG (punteggi, game over) |
| `/scrolltext` | Gestione messaggi testo scorrevole (10 messaggi, font, effetti) |
| `/arcade` | Emulatore Arcade (lista giochi, avvio/stop, stato) |
| `/update` | Aggiornamento OTA firmware e LittleFS via browser |

### Servizi di Rete

| Servizio | Porta | Descrizione |
|---|---|---|
| Web Server | 8080 | Interfaccia configurazione |
| Espalexa | 80 | Discovery Amazon Alexa |
| OTA | 3232 | Aggiornamenti firmware ArduinoOTA |
| mDNS | - | `oraquadra.local` |
| ESP-NOW | - | Sincronizzazione Multi-Display (broadcast) |

---

## Sistema Audio

### Audio I2S Interno

Il sistema utilizza la libreria ESP32-audioI2S (Audio.h) per la riproduzione audio:

```
Formato:          MP3, AAC (streaming)
Sorgente:         LittleFS (/data/), SD Card, HTTP Stream
Sample Rate:      44100 Hz (audio), 24000 Hz (arcade)
Bit Depth:        16 bit
Canali:           Stereo
Volume Default:   80% (range 0-21)
```

### Funzionalita Audio

| Funzione | Descrizione |
|---|---|
| **Annunci Orari** | Riproduzione vocale dell'ora ogni ora intera |
| **Annuncio Boot** | Data e ora completa all'avvio del dispositivo |
| **Web Radio** | Streaming radio internet in tempo reale |
| **MP3 Player** | Riproduzione brani da SD card |
| **Google TTS** | Sintesi vocale via Google Translate (opzionale) |
| **VU Meter** | Indicatore visivo livello audio durante riproduzione |
| **Audio Arcade** | 3 motori di sintesi: Namco WSG, AY-3-8910, DK PCM |

### Annunci Orari

| Parametro | Valore Default |
|---|---|
| Orario Inizio | 08:00 |
| Orario Fine | 19:59 |
| Volume Annunci | Configurabile separatamente |
| Modalita | MP3 locali o Google TTS |

### File Audio Richiesti

La cartella `/data/` (LittleFS) deve contenere:
- `0.mp3` - `59.mp3` (numeri)
- `sonole.mp3`, `ore.mp3`, `e.mp3`, `minuti.mp3`
- `mezzanotte.mp3`, `mezzogiorno.mp3`
- `lunedi.mp3` - `domenica.mp3` (giorni)
- `gennaio.mp3` - `dicembre.mp3` (mesi)
- `saluto.mp3` (messaggio benvenuto boot)
- `H-2024.mp3` - `H-2030.mp3` (anni)

### File SD Card

La SD card (FAT32) puo contenere:
- Immagini JPG 480x480 per skin orologi
- File `radio.txt` con lista stazioni radio
- Brani MP3/WAV per il lettore musicale

---

## Web Radio

La Web Radio permette di ascoltare stazioni radio in streaming direttamente dal dispositivo con un'interfaccia touch moderna.

### Caratteristiche Web Radio

| Funzionalita | Descrizione |
|---|---|
| **Streaming** | Supporto stream MP3/AAC via HTTP |
| **VU Meter** | Indicatori livello audio stereo L/R animati |
| **Lista Stazioni** | Fino a 50 stazioni configurabili |
| **Controlli Touch** | Play/Stop, Vol+/-, Prev/Next stazione |
| **Persistenza** | Ultima stazione e volume salvati in EEPROM |

### Configurazione Stazioni

Le stazioni radio possono essere configurate via web (`/webradio`) o caricando un file `radio.txt` sulla SD card:

```
Rai Radio 1|http://icestreaming.rai.it/1.mp3
Rai Radio 2|http://icestreaming.rai.it/2.mp3
RTL 102.5|http://streaming.rtl.it/rtl1025
```

---

## Radiosveglia

La radiosveglia permette di programmare una sveglia che si attiva riproducendo una stazione radio selezionata.

### Caratteristiche Radiosveglia

| Funzionalita | Descrizione |
|---|---|
| **Programmazione** | Impostazione ora/minuti con controlli touch |
| **Giorni** | Selezione giorni della settimana (DOM-SAB) |
| **Stazione** | Scelta stazione dalla lista Web Radio |
| **Volume** | Volume sveglia indipendente (0-21) |
| **Snooze** | Posticipo configurabile (5-30 minuti) |
| **Test** | Prova immediata della sveglia |
| **Persistenza** | Impostazioni salvate in EEPROM |

### Controlli Radiosveglia

| Azione | Funzione |
|---|---|
| Toggle ON/OFF | Attiva/disattiva sveglia o ferma se sta suonando |
| Frecce Ora | Incrementa/decrementa ora |
| Frecce Minuti | Incrementa/decrementa minuti |
| Giorni | Tocca per attivare/disattivare ogni giorno |
| Frecce Stazione | Cambia stazione radio |
| Barra Volume | Regola volume sveglia |
| TEST | Attiva la sveglia per test |
| SNOOZE | Posticipa (durante sveglia) o cambia durata snooze |

---

## Calendario

Il calendario offre una vista mensile interattiva con supporto per eventi locali e sincronizzazione Google Calendar.

### Caratteristiche Calendario

| Funzionalita | Descrizione |
|---|---|
| **Vista Griglia** | Calendario mensile con evidenziazione giorni con eventi |
| **Vista Giorno** | Dettaglio eventi del giorno selezionato |
| **Eventi Locali** | Fino a 50 eventi salvati su LittleFS |
| **Google Calendar** | Sincronizzazione automatica ogni 60 secondi |
| **Navigazione Touch** | Cambio mese e selezione giorno tramite touch |
| **Persistenza** | Eventi locali salvati in `/calendar_events.json` |

### Configurazione Google Calendar

Configurare URL e API key del Google Calendar dalla pagina `/settings` del web server.

---

## LED RGB WS2812

Striscia di 12 LED RGB WS2812 indirizzabili singolarmente, con colori che seguono automaticamente il tema della modalita attiva.

### Caratteristiche LED RGB

| Funzionalita | Descrizione |
|---|---|
| **Colore Automatico** | Segue il tema della modalita di visualizzazione attiva |
| **Override Manuale** | Color picker per impostare un colore fisso personalizzato |
| **Audio Reactive** | Luminosita pulsante sincronizzata con la musica |
| **Luminosita** | Regolabile tramite slider (0-255) |
| **Persistenza** | Impostazioni salvate in EEPROM (indirizzi 900-907) |

---

## YouTube Stats

Visualizzazione in tempo reale delle statistiche di un canale YouTube tramite YouTube Data API v3.

### Caratteristiche YouTube

| Funzionalita | Descrizione |
|---|---|
| **Iscritti** | Contatore subscriber in tempo reale |
| **Visualizzazioni** | Totale views del canale |
| **Video** | Numero totale di video pubblicati |
| **Aggiornamento** | Fetch automatico ogni 5 minuti |
| **Smart Redraw** | Ridisegna solo quando i dati cambiano |
| **Configurazione** | API key e Channel ID da pagina `/settings` |

---

## News Feed RSS

Feed notizie in tempo reale da 3 fonti internazionali, senza necessita di API key.

### Fonti e Categorie

| Fonte | Paese | Categorie |
|---|---|---|
| **ANSA** | Italia | Generale, Cronaca, Politica, Economia, Sport, Tech, Cultura, Mondo |
| **BBC** | UK | Top, World, Business, Tech, Sport, Arts, Science |
| **CNN** | USA | Top, World, Business, Tech, Sport |

### Caratteristiche News

| Funzionalita | Descrizione |
|---|---|
| **RSS Nativo** | Nessuna API key richiesta, nessun limite di richieste |
| **Aggiornamento** | Fetch automatico ogni 10 minuti |
| **Tab Dinamici** | Layout 1 o 2 righe in base al numero di categorie |
| **4 Articoli** | Visualizzazione di 4 card articoli per categoria |
| **Touch** | Tap su tab per categoria, pill per ciclare fonte |
| **Parsing** | Gestione CDATA, HTML entities, tag XML |

---

## Multi-Display ESP-NOW

Sistema multi-pannello sincronizzato via ESP-NOW per creare display fino a 960x960 pixel (griglia 2x2).

### Caratteristiche Multi-Display

| Funzionalita | Descrizione |
|---|---|
| **Pannelli** | Fino a 4 pannelli in griglia 2x2 (960x960px virtuale) |
| **Protocollo** | ESP-NOW con magic byte 0xDC (~1-2ms latenza) |
| **Ruoli** | Standalone, Master, Slave (auto-configurabile) |
| **Sincronizzazione** | 30fps (ogni 33ms) con heartbeat e discovery |
| **Auto-Discovery** | I pannelli si trovano automaticamente sulla rete |
| **Touch Forward** | Il touch sullo slave viene inoltrato al master |
| **Virtual Canvas** | Coordinate virtuali `vFillRect()`, `vDrawCircle()`, etc. |
| **Per-Mode** | Abilitabile/disabilitabile per singola modalita |
| **Persistenza** | Config salvata in EEPROM (indirizzi 908-948) |

### Configurazione

1. Configurare i ruoli (Master/Slave) dalla pagina web `/dualdisplay`
2. Usare "Scan" per trovare gli altri pannelli
3. Aggiungere i peer tramite MAC address o discovery automatico
4. Il Master invia lo stato, gli slave lo applicano

### Requisiti

- Almeno 2 dispositivi ESP32-4848S040
- Stessa rete WiFi per la discovery iniziale
- ESP-NOW funziona anche senza WiFi dopo il pairing

---

## Giochi Dual Display

Tre giochi classici che utilizzano il sistema Multi-Display con 2 pannelli affiancati (960x480 pixel).

### PONG (MODE 28)

| Funzionalita | Descrizione |
|---|---|
| **Canvas** | 960x480px (2 pannelli x 1 riga) |
| **Giocatori** | 2 (Master=palette sx, Slave=palette dx) |
| **Velocita** | 5 livelli (touch in alto per cambiare) |
| **Punteggio** | Primo a 5 punti vince |
| **Touch** | Continuo per muovere la palette su/giu |
| **Uscita** | Touch angolo alto-sx (<80, <80) |

### Breakout (MODE 30)

| Funzionalita | Descrizione |
|---|---|
| **Canvas** | 960x480px (2 pannelli x 1 riga) |
| **Mattoncini** | 16 colonne x 5 righe = 80 bricks |
| **Colori** | Rosso(5pt), Arancio(4pt), Giallo(3pt), Verde(2pt), Blu(1pt) |
| **Vite** | 3 vite per partita |
| **Velocita** | 5 livelli (touch in alto per cambiare) |
| **Palette** | Si muove orizzontalmente (asse X) |

### Battaglia Navale (MODE 31)

| Funzionalita | Descrizione |
|---|---|
| **Griglia** | 10x10 celle per giocatore |
| **Navi** | 5 navi (Portaerei 5, Corazzata 4, Incrociatore 3, Sottomarino 3, Cacciatorpediniere 2) |
| **Fasi** | Piazzamento P1 -> P2 -> Turni alternati -> Game Over |
| **Privacy** | Ogni pannello mostra SOLO la propria vista (griglia privata) |
| **Vista Battaglia** | Schermo diviso: griglia attacco (grande) + difesa (piccola) |
| **Touch** | TAP per sparare/piazzare navi, tasto Ruota e Random |

### Requisiti Giochi

- `EFFECT_DUAL_DISPLAY` abilitato
- 2 pannelli configurati come Master + Slave
- Touch attivo su entrambi i pannelli

---

## Testo Scorrevole

Modalita di visualizzazione testo scorrevole con supporto per font multipli, effetti visivi e rotazione messaggi.

### Caratteristiche Scroll Text

| Funzionalita | Descrizione |
|---|---|
| **Messaggi** | Fino a 10 messaggi (200 caratteri ciascuno) |
| **Font** | 19 font: 1 Pixel scalabile + 18 U8g2 (fino a 54px) |
| **Direzione** | Sinistra o Destra |
| **Velocita** | 1-10 (da 1.0 a 19.0 px/frame) |
| **Effetti** | Rainbow (per carattere HSV), Fade (bande ai bordi) |
| **Data/Ora** | Opzionale, scorre insieme al testo |
| **Colore sfondo** | Personalizzabile (R, G, B) |
| **Rotazione** | Cambio automatico tra messaggi con intervallo configurabile |
| **Persistenza** | LittleFS `/scrolltext.json` + EEPROM backup (933-942) |

### Controlli Touch

| Zona | Funzione |
|---|---|
| Sinistra | Messaggio precedente |
| Centro | Pausa / Riprendi |
| Destra | Messaggio successivo |
| Basso centro | MODE>> (uscita) |

---

## Emulatore Arcade Z80

Emulatore di giochi arcade classici basato su CPU Z80, con audio sintetizzato in tempo reale e supporto gamepad Bluetooth.

### Giochi Supportati

| Gioco | Anno | Audio | Define |
|---|---|---|---|
| **Pac-Man** | 1980 | Namco WSG (3 canali wavetable) | `ENABLE_PACMAN` |
| **Galaga** | 1981 | Namco WSG + campione esplosioni | `ENABLE_GALAGA` |
| **Donkey Kong** | 1981 | i8048 PCM + campioni (walk, jump, stomp) | `ENABLE_DKONG` |
| **Frogger** | 1981 | AY-3-8910 (1 chip) | `ENABLE_FROGGER` |
| **Dig Dug** | 1982 | Namco WSG | `ENABLE_DIGDUG` |
| **1942** | 1984 | AY-3-8910 (2 chip) | `ENABLE_1942` |
| **Eyes** | 1982 | Namco WSG | `ENABLE_EYES` |
| **Mr. TNT** | 1983 | Namco WSG | `ENABLE_MRTNT` |
| **Liz Wiz** | 1985 | Namco WSG | `ENABLE_LIZWIZ` |
| **The Glob** | 1983 | Namco WSG | `ENABLE_THEGLOB` |
| **Crush Roller** | 1981 | Namco WSG | `ENABLE_CRUSH` |
| **Anteater** | 1982 | AY-3-8910 (1 chip) | `ENABLE_ANTEATER` |

### Architettura

| Componente | Descrizione |
|---|---|
| **CPU** | Emulatore Z80 + i8048 (per Donkey Kong) |
| **Display** | 224x288 originale -> 336x432 (1.5x upscale) centrato su 480x480 |
| **Threading** | Emulazione su Core 0 (60fps), Rendering su Core 1 (~25-30fps) |
| **Audio** | 3 motori: Namco WSG, AY-3-8910, DK PCM a 24kHz stereo |
| **Menu** | Griglia 4x3 con logo per ogni gioco |
| **Input** | Touch + gamepad BLE (OR logico, entrambi funzionano) |

### Controlli Touch Arcade

```
+--------+----------------------------+--------+
|  EXIT  |                            |  COIN  |
| <80,48 |       Area di gioco        | >400,48|
+--------+----------------------------+--------+
|        |                            |        |
|  LEFT  |         UP / DOWN          | RIGHT  |
|        |        (Y < > 240)         |        |
+--------+----------------------------+--------+
|  BACK  |          START             |  FIRE  |
| 0-159  |         160-319            | 320-479|
+--------+----------------------------+--------+
```

---

## BLE Gamepad

Supporto opzionale per gamepad Bluetooth Low Energy nell'emulatore Arcade.

### Caratteristiche

| Funzionalita | Descrizione |
|---|---|
| **Controller** | 8BitDo, Xbox BLE, gamepad HID generici |
| **Auto-detect** | Rileva automaticamente il formato dei report HID |
| **Formati** | Standard 8-byte, Xbox BLE 14-byte, Generic long |
| **Connessione** | Background task (non blocca il rendering) |
| **Auto-reconnect** | Riconnessione automatica se il gamepad si disconnette |
| **Mapping** | D-pad/stick -> direzioni, A/X -> Fire, B -> Extra, Select -> Coin, Start -> Start |

### Requisiti

- Libreria **NimBLE-Arduino** di h2zero (installare da Library Manager)
- Se non installata, il progetto compila comunque (gamepad disabilitato)
- Il gamepad deve supportare il profilo HID Service (UUID 0x1812)

---

## OTA Firmware Update

Aggiornamento firmware e file system LittleFS via browser web, senza necessita di cavo USB.

### Caratteristiche OTA

| Funzionalita | Descrizione |
|---|---|
| **Firmware** | Upload file `.bin` firmware via browser |
| **LittleFS** | Upload file `.bin` LittleFS (file audio, configurazioni) |
| **Pagina Web** | Interfaccia drag & drop a `/update` |
| **ArduinoOTA** | Supporto aggiornamento da Arduino IDE (porta 3232) |
| **Auto-restart** | Riavvio automatico dopo upload completato |

---

## Installazione

### Metodo 1: Programmazione Web (Consigliato)

> Nessuna compilazione richiesta!

1. Collegare il dispositivo via **USB**
2. Aprire **Chrome** e andare a: [Web Programmer](https://davidegatti.altervista.org/installaEsp32.html?progetto=oraQuadraNano)
3. Seguire le istruzioni a schermo

### Metodo 2: Compilazione Arduino IDE

#### Requisiti Obbligatori

> **CONDITIO SINE QUA NON**: Le seguenti versioni sono OBBLIGATORIE. Versioni diverse causeranno errori di compilazione, crash o malfunzionamenti del display.

| Componente | Versione | Note |
|---|---|---|
| Arduino IDE | 1.8.x o 2.x | |
| ESP32 Core | **2.0.17** | **OBBLIGATORIO** - Non usare versioni 3.x |
| GFX Library for Arduino | **1.6.0** | **OBBLIGATORIO** - Non usare versioni successive |

---

## Impostazioni Arduino IDE

> **IMPORTANTE**: Queste impostazioni sono ESSENZIALI. Impostazioni errate causeranno crash, riavvii continui o firmware troppo grande per la flash.

### Impostazioni Board (OBBLIGATORIE)

| Parametro | Valore | Conseguenze se errato |
|---|---|---|
| Board | **ESP32S3 Dev Module** | Non compila |
| Partition Scheme | **HUGE APP (3MB No OTA)** | Firmware troppo grande, upload fallisce |
| PSRAM | **OPI PSRAM** | Crash immediato, display nero, buffer PSRAM non allocati |
| Flash Size | **16MB (128Mb)** | Spazio insufficiente |
| Upload Speed | 921600 | Upload lento (non critico) |
| CPU Frequency | **240MHz** | Performance degradate, arcade lento |

### Perche HUGE APP?

Il firmware compilato con tutte le modalita abilitate supera i 2MB. La partizione HUGE APP riserva ~3MB per l'applicazione. Senza questa impostazione il firmware non entra nella flash.

### Perche OPI PSRAM?

Il progetto alloca buffers in PSRAM per:
- Frame buffer arcade (~100KB)
- Buffer upscale display (~96KB)
- Double buffer scroll text (~460KB)
- Buffer frame dual display
- Buffers audio e decodifica JPEG

Senza PSRAM abilitata, queste allocazioni falliscono e il dispositivo va in crash.

### Librerie da Installare

```
Arduino_GFX_Library v1.6.0    - Display grafico (VERSIONE ESATTA!)
ezTime                        - Gestione ora/data NTP
Espalexa                      - Integrazione Alexa
WiFiManager                   - Configurazione WiFi
ESPAsyncWebServer             - Web server asincrono
AsyncTCP                      - TCP asincrono
JPEGDEC                       - Decodifica immagini JPG
TAMC_GT911                    - Touch screen controller
Adafruit BME280 Library       - Sensore ambientale (opzionale)
MyLD2410                      - Radar LD2410 (opzionale)
U8g2                          - Font grafici (scroll text + flip clock)
ESP32-audioI2S (Audio.h)      - Streaming audio, MP3, Web Radio
Adafruit NeoPixel             - LED WS2812 (striscia RGB)
NimBLE-Arduino                - BLE Gamepad per Arcade (opzionale)
LittleFS (integrata)          - File system per eventi e configurazioni
esp_now.h (integrata)         - Comunicazione Multi-Display
```

> **Nota**: Le librerie marcate "opzionale" non impediscono la compilazione se assenti. Il progetto compila comunque con stub vuoti.

### Compilazione

1. Aprire `oraQuadra_Nano_Ver_1.5.ino`
2. Verificare le impostazioni della board (tabella sopra)
3. Caricare i file audio su LittleFS (Tools -> ESP32 Sketch Data Upload)
4. Compilare e caricare il firmware

---

## Struttura del Progetto

```
oraQuadra_Nano_Ver_1.5/
|
|-- oraQuadra_Nano_Ver_1.5.ino    # File principale (configurazione, define, struct)
|-- 0_SETUP.ino                    # Inizializzazione EEPROM, Display, WiFi, OTA, Alexa
|-- 0A_PROTECTION.ino              # Sistema protezione crediti
|-- 1_TOUCH.ino                    # Gestione touch screen 4 zone
|-- 2_CHANGE_MODE.ino              # Cambio modalita visualizzazione e cleanup
|-- 3_EFFETTI.ino                  # Effetti visuali base (Fade, Matrix, Snake, Water)
|-- 4_MENU.ino                     # Menu di configurazione
|-- 4_WEBSERVER_CLOCK.ino          # Web server orologio analogico
|-- 5_AUDIO.ino                    # Sistema audio I2S, VU meter, annunci, TTS
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
|-- 31_MP3_PLAYER.ino              # Lettore MP3 da SD card
|-- 32_WEB_RADIO.ino               # Web Radio streaming con VU meter
|-- 33_RADIO_ALARM.ino             # Radiosveglia programmabile
|-- 34_WEB_TV.ino                  # Web TV (disabilitato)
|-- 35_CALENDAR.ino                # Calendario mensile/giornaliero
|-- 36_WEBSERVER_CALENDAR.ino      # Web server calendario
|-- 37_LED_RGB.ino                 # Controllo LED RGB WS2812
|-- 38_WEBSERVER_LED_RGB.ino       # Web server LED RGB
|-- 39_YOUTUBE.ino                 # Statistiche YouTube
|-- 40_WEBSERVER_YOUTUBE.ino       # Web server YouTube
|-- 41_NEWS.ino                    # Feed notizie RSS
|-- 42_WEBSERVER_NEWS.ino          # Web server News
|-- 43_PONG.ino                    # Gioco PONG Dual Display
|-- 43_WEBSERVER_OTA.ino           # OTA firmware update via browser
|-- 44_DUAL_DISPLAY.ino            # Multi-Display ESP-NOW (sincronizzazione pannelli)
|-- 45_WEBSERVER_DUAL_DISPLAY.ino  # Web server Multi-Display
|-- 46_BREAKOUT.ino                # Gioco Breakout Dual Display
|-- 46_WEBSERVER_PONG.ino          # Web server PONG
|-- 47_MODE_SELECTOR.ino           # Overlay griglia selettore modalita
|-- 48_SCROLLTEXT.ino              # Testo scorrevole animato
|-- 49_WEBSERVER_SCROLLTEXT.ino    # Web server Scroll Text
|-- 50_BATTLESHIP.ino              # Gioco Battaglia Navale Dual Display
|-- 51_ARCADE.ino                  # Emulatore Arcade Z80 (render + menu)
|-- 52_WEBSERVER_ARCADE.ino        # Web server Arcade
|-- 53_BLE_GAMEPAD.ino             # Supporto gamepad BLE per Arcade
|
|-- arcade/                        # Motore emulazione arcade
|   |-- arcade_audio.h/.cpp        # Audio bridge (Namco WSG, AY-3-8910, DK PCM)
|   |-- arcade_bridge.h/.cpp       # Bridge emulazione <-> sistema
|   |-- arcade_config.h            # Configurazione arcade
|   |-- cpus/
|   |   |-- z80/Z80.h/.c           # Emulatore CPU Z80
|   |   +-- i8048/i8048.h/.c       # Emulatore CPU i8048 (Donkey Kong audio)
|   +-- machines/
|       |-- machineBase.h           # Classe base macchine arcade
|       |-- tileaddr.h              # Helper indirizzamento tile
|       |-- pacman/                  # ROM e dati Pac-Man
|       |-- galaga/                  # ROM e dati Galaga
|       |-- dkong/                   # ROM e dati Donkey Kong
|       |-- frogger/                 # ROM e dati Frogger
|       |-- digdug/                  # ROM e dati Dig Dug
|       |-- 1942/                    # ROM e dati 1942
|       |-- eyes/                    # ROM e dati Eyes
|       |-- mrtnt/                   # ROM e dati Mr. TNT
|       |-- lizwiz/                  # ROM e dati Liz Wiz
|       |-- theglob/                 # ROM e dati The Glob
|       |-- crush/                   # ROM e dati Crush Roller
|       +-- anteater/                # ROM e dati Anteater
|
|-- bttf_types.h                   # Struct per modalita BTTF
|-- bttf_web_html.h                # HTML pagina web BTTF
|-- home_web_html.h                # HTML homepage
|-- settings_web_html.h            # HTML pagina impostazioni
|-- calendar_web_html.h            # HTML pagina web calendario
|-- youtube_web_html.h             # HTML pagina web YouTube
|-- news_web_html.h                # HTML pagina web News
|-- dualdisplay_web_html.h         # HTML pagina web Multi-Display
|-- pong_web_html.h                # HTML pagina web PONG
|-- scrolltext_web_html.h          # HTML pagina web Scroll Text
|-- arcade_web_html.h              # HTML pagina web Arcade
|-- ota_web_html.h                 # HTML pagina web OTA update
|-- protection.h                   # Sistema protezione crediti
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
#define EFFECT_MP3_PLAYER     // Lettore MP3/WAV da SD card
#define EFFECT_WEB_RADIO      // Web Radio streaming con interfaccia touch
#define EFFECT_RADIO_ALARM    // Radiosveglia con selezione stazione
#define EFFECT_CALENDAR       // Calendario con eventi locali e Google Calendar
#define EFFECT_LED_RGB        // LED RGB WS2812 (12 LED) su GPIO43
#define EFFECT_YOUTUBE        // Statistiche canale YouTube
#define EFFECT_NEWS           // News Feed RSS (ANSA, BBC, CNN)
#define EFFECT_DUAL_DISPLAY   // Multi-Display ESP-NOW (fino a 4 pannelli)
#define EFFECT_PONG           // Gioco PONG (richiede DUAL_DISPLAY)
#define EFFECT_BREAKOUT       // Gioco Breakout (richiede DUAL_DISPLAY)
#define EFFECT_SCROLLTEXT     // Testo scorrevole animato
#define EFFECT_BATTLESHIP     // Battaglia Navale (richiede DUAL_DISPLAY)
#define EFFECT_ARCADE         // Emulatore Arcade Z80 (12 giochi)
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

---

## Mappa EEPROM

> **ATTENZIONE**: Gli indirizzi EEPROM sono assegnati rigorosamente. Non modificare questi range senza aggiornare tutti i riferimenti nel codice. Conflitti di indirizzo causano corruzione delle impostazioni.

### Range Principali

| Range | Funzione |
|---|---|
| **0-9** | Impostazioni base (marker, preset, blink, mode, orari, luminosita, volume) |
| **10-106** | Credenziali WiFi (SSID 32B + password 64B + flag validita) |
| **107-137** | Display e audio (colore RGB, data, audio giorno/notte, volumi) |
| **175-180** | Radar LD2410 (luminosita min/max, VU meter, TTS) |
| **210-231** | Temporizzazione (giorno/notte minuti, random mode, radio, MP3, annunci) |
| **270-295** | Flux Capacitor + LED Ring (colori, particelle, velocita) |
| **346-406** | BME280 calibrazione + define costanti |
| **500** | Versione layout EEPROM (0x02) |
| **520-562** | Orologio analogico (colori lancette, skin, smooth, rainbow) |
| **570-577** | Radiosveglia |
| **600-686** | BTTF (date destinazione/partenza, font, posizioni 3 pannelli) |
| **700-703** | `enabledModesMask` lower 32 bits (4 byte) - **NON TOCCARE** |
| **704** | `rainbowMode` - **NON TOCCARE** |
| **705** | `enabledModesMask` ext byte (bits 32-39) |
| **710-775** | Colori personalizzati modalita (22 mode x 3 byte RGB) |
| **780-801** | Preset personalizzati modalita (22 mode x 1 byte) |
| **900-907** | LED RGB WS2812 (on/off, luminosita, override, R, G, B, marker 0xAB, audioReactive) |
| **908-932** | Multi-Display ESP-NOW (enabled, panelXY, gridWH, role, 3x MAC 6B, marker 0xDD) |
| **933-942** | Scroll Text (marker 0xAE, count, current, rotation, bg RGB, showTime, showDate) |
| **943-948** | Per-mode dual display mask (5B + marker 0xDE) |
| **949-1023** | **LIBERO** (75 byte disponibili) |

### Zone Critiche - NON MODIFICARE

| Range | Motivo |
|---|---|
| 700-705 | `enabledModesMask` + `rainbowMode` - Sovrascrivere causa reset di tutte le modalita abilitate |
| 710-801 | Colori e preset per-mode - Conflitti corrompono colori/preset di tutti i modi |
| 908-948 | Multi-Display + Scroll Text + dual mask - Sovrascrivere disallinea la configurazione pannelli |
| 500 | Versione EEPROM - Cambiare forza reset completo impostazioni |

---

## Changelog

### v1.5.5 (23/02/2026) - Attuale
**by Paolo Sambinello & Marco Camerani**
- **Emulatore Arcade Z80** (MODE 32): 12 giochi arcade classici
  - Pac-Man, Galaga, Donkey Kong, Frogger, Dig Dug, 1942, Eyes, Mr. TNT, Liz Wiz, The Glob, Crush Roller, Anteater
  - Audio sintetizzato in tempo reale: Namco WSG, AY-3-8910, DK PCM (24kHz stereo)
  - Emulazione Z80 a 60fps su Core 0, rendering ~25-30fps su Core 1
  - Menu selezione gioco con griglia 4x3 e loghi
  - Controlli touch (D-pad, COIN, START, FIRE) + gamepad BLE
  - Pagina web `/arcade` con stato e controllo giochi
- **BLE Gamepad** per Arcade: Supporto gamepad Bluetooth
  - 8BitDo, Xbox BLE, HID generici con auto-detect formato
  - Connessione background su FreeRTOS task (non blocca il rendering)
  - Auto-reconnect su disconnessione
  - Libreria NimBLE-Arduino opzionale (compila senza)
- **Battaglia Navale** (MODE 31): Gioco classico Dual Display
  - 5 navi, griglia 10x10, vista privata per giocatore
  - Fasi: piazzamento + turni alternati + game over
  - Master gestisce logica, slave mostra vista locale
- **Breakout** (MODE 30): Gioco mattoncini Dual Display
  - 80 bricks (16x5), 5 colori/punti, 3 vite
  - 5 livelli velocita, palette orizzontale
  - Sincronizzazione stato bricks via ESP-NOW
- **Testo Scorrevole** (MODE 29): Testo animato con effetti
  - 10 messaggi x 200 char, 19 font (Pixel + 18 U8g2)
  - Effetti rainbow e fade, data/ora opzionale
  - Double buffer PSRAM, pagina web `/scrolltext`
  - EEPROM 933-942, LittleFS `/scrolltext.json`
- **PONG** (MODE 28): Tennis 2 giocatori Dual Display
  - 960x480px, 5 livelli velocita, primo a 5 punti
  - Delta rendering palette, touch continuo
- **Multi-Display ESP-NOW**: Sincronizzazione pannelli
  - Fino a 4 pannelli 2x2 (960x960px virtuale)
  - Protocollo ESP-NOW con discovery, heartbeat, touch forward
  - Virtual canvas con coordinate globali
  - Pagina web `/dualdisplay` con editor visuale griglia
  - EEPROM 908-948 per configurazione persistente
- **Mode Selector**: Overlay griglia selettore modalita
  - Griglia 4 colonne scorrevole con tutte le 33 modalita
  - Touch centro-sinistra per attivare, timeout 15 secondi
- **OTA Web**: Aggiornamento firmware/LittleFS via browser
  - Pagina `/update` con upload .bin firmware e LittleFS
  - Riavvio automatico dopo aggiornamento

### v1.5.4 (14/02/2026)
**by Paolo Sambinello & Marco Camerani**
- **News Feed RSS** (MODE 27): Feed notizie in tempo reale senza API key
  - 3 fonti: ANSA (8 categorie), BBC (7 categorie), CNN (5 categorie)
  - Layout tab dinamico (1 o 2 righe in base al numero di categorie)
  - Navigazione touch: tap su tab per categoria, pill per ciclare fonte
  - Parsing RSS con gestione CDATA e HTML entities
  - Pagina web `/news` con selezione fonte e categoria
- **YouTube Stats** (MODE 26): Statistiche canale YouTube in tempo reale
  - YouTube Data API v3 con aggiornamento ogni 5 minuti
  - Display: logo, nome canale, 3 card (iscritti, visualizzazioni, video)
  - Smart redraw: ridisegna solo quando i dati cambiano
  - API key e Channel ID configurabili da pagina `/settings`
  - Pagina web `/youtube` con status JSON
- **LED RGB WS2812**: Striscia LED RGB da 12 LED su GPIO43
  - Colori automatici che seguono il tema della modalita attiva
  - Override manuale colore tramite color picker
  - Modalita audio reactive: luminosita LED pulsante con la musica
  - Pagina web `/ledrgb` con controlli completi
  - Toggle on/off nella pagina Settings
  - Impostazioni persistenti in EEPROM (indirizzi 900-907)
- **Calendario** (MODE 25): Calendario interattivo con eventi
  - Vista griglia mensile + vista dettaglio giornaliero
  - Eventi locali salvati su LittleFS (max 50 eventi)
  - Sincronizzazione Google Calendar (compatibile formato DD/MM e DD/MM/YYYY)
  - Navigazione touch tra mesi e giorni
  - Pagina web `/calendar` per gestione eventi

### v1.5 (30/01/2026)
**by Paolo Sambinello & Marco Camerani**
- **Web Radio**: Streaming radio internet con interfaccia touch moderna
  - VU meter stereo animato (L/R)
  - Lista stazioni configurabile (fino a 50)
  - Controlli play/stop, volume, cambio stazione
  - Salvataggio stazioni su SD card
- **Radiosveglia**: Sveglia programmabile con stazione radio
  - Selezione giorni della settimana
  - Volume indipendente
  - Funzione snooze configurabile (5-30 min)
  - Test sveglia immediato
- **MP3 Player**: Lettore MP3/WAV da SD card migliorato
- **Audio migliorato**:
  - Nuovo sistema audio basato su libreria Audio.h (ESP32-audioI2S)
  - VU meter durante riproduzione
  - Annuncio boot con data e ora completa
  - Supporto Google TTS e MP3 locali
  - Gestione pause/resume web radio durante annunci
- Nuovi endpoint web: `/webradio`, `/radioalarm`
- Correzioni stabilita generale

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
