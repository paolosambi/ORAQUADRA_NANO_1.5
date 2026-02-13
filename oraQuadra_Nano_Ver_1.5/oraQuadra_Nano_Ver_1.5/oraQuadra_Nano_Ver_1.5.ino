
// ORAQUADRA nano V1.0, By Paolo Sambinello, Alessandro Spagnoletti, Davide Gatti (www.survivalhacking.it)
// Video Tutorial completo: https://youtu.be/fNRnZvtF9N0 
// Versione ridotta e semplificato del progetto ORAQUADRA 2: https://youtu.be/DiFU6ITK8QQ / https://github.com/SurvivalHacking/Oraquadra2 
// Tutti i file necessari per la stampa 3D e info varie le trovate qui:  https://github.com/SurvivalHacking/OraquadraNano  
// Si può programmare senza compilazione andando a questo LINK con CHROME e collegando il dispositivo con un cavo USB al computer: https://davidegatti.altervista.org/installaEsp32.html?progetto=oraQuadraNano
// Qui potrete trovare il corretto modulo usato in questo progetto:  ESP32-4848S040  https://s.click.aliexpress.com/e/_onmPeo3  
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   ATTENZIONE !!!!!   Per una corretta compilazione dovrete selezionare nel gestore schede il core ESP32 in versione 2.0.17  /////
//                      e nel gestore librerie, la libreria  GFX Library for Arduino  in versione 1.6.0                        /////
//                      Nelle opzioni come Partition Scheme: HUGE APP e PSRAM: OPI PSRAM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// V1.1  21/10/2025 by Paolo Sambinello
// - Sistemata la funzionalità di utilizzo con Alexa, il dispositivo viene riconosciuto come lampadina alexa col nome ORAQUADRANANO
//   I comandi accettati sono:
//   ACCENDI ORAQUADRANANO
//   SPEGNI ORAQUADRANANO
//   ORAQUADRANANO COLORE (qui dire il colore che si vuole)
//
// - Implementato QR code per la configurazione del WIFI
// - Migliorata la modalità Snake
//
// V1.2  24/10/2025 by Paolo Sambinello
// - Aggiunte nuove modalità Mario e Tron
//
// V1.3  05/11/2025 by Paolo Sambinello
// - Aggiunta nuove modalità Galaga
// - Aggiunte nuova modalità orologio analogico con skin configurabile via WEB. E' necessario inserire una uSD formattata FAT32 per poter inserire le immagini degli orologi.
//   Le immagini da utilizzare devono essere delle JPG da 480x480, poi via web è possibile impostare la lunghezz e il colore delle lancette e molte altre cose.
//
// 
// V1.3.1  07/11/2025 by Paolo Sambinello 
// - Aggiunta modalità Ritorno al futuro 
// - Aggiunto easter egg a sorpresa 
//
//
// V1.5  24/01/2026 by Marco Camerani 
// - Attivato modulo interno I2S per audio
// - Riproduzione corretta dell'orario vocale 
// - Correzioni e miglioramenti generali

// - 09/02/2026 by Marco Camerani Paolo Sambinello
//   Aggiunta WEB RADIO, MP3 PLAYER, RADIO ALARM, CALENDARIO APPUNTAMENTI
//
//---------------------------------------------------------------------------------------------------------------------------------------
// Il quadrante è touch e se lo suddividete in 4 parti avrete 4 zone separate con diverse funzionalità.
//
// ----- Parte in alto a sinistra -----
// Cambio modalità:  FADE / LENTO / VELOCE / MATRIX / MATRIX2 / SNAKE / GOCCIA
//
// ----- Parte in alto a destra -----
// Una serie di combinazioni di colori delle varie modalità
//
// ----- Parte in basso a destra -----
// Fa lampeggiare la lettera E separatore tra le ore e minuti
//
// ----- Parte in basso a sinistra -----
// Selettore colore. Tenendolo premuto è possibile ciclre in tutti i colori per poter scegliere quello preferito.
//
// Una volta selezionato un modo o preset con il vostro colore personalizzato, verrà memorizzato e quando riavvierete il dispositivo, ritornerà nell'ultima configurazione che avevate impostato
//
// ----- Tenendo premute con 4 dita tutte e 4 le zone durante la fase di avvio, verranno resettate le impostazioni WIFI
//---------------------------------------------------------------------------------------------------------------------------------------
//
//
// Un grande ringraziamento a Paolo e Alessandro per il grandissimo lavoro di porting e conversione

//#define MENU_SCROLL //ATTIVA MENU SCROLL COMMENTA QUESTA RIGA PER DISATTIVARE
#define AUDIO  //ATTIVA AUDIO I2S LOCALE

// ================== ABILITA/DISABILITA EFFETTI ==================
#define EFFECT_FADE       // Effetto dissolvenza
#define EFFECT_SLOW       // Effetto lento
#define EFFECT_FAST       // Effetto veloce (visualizzazione diretta)
#define EFFECT_MATRIX     // Effetto pioggia Matrix
#define EFFECT_MATRIX2    // Effetto pioggia Matrix variante 2
#define EFFECT_SNAKE      // Effetto serpente
#define EFFECT_WATER      // Effetto goccia d'acqua
#define EFFECT_MARIO      // Effetto Mario Bros
#define EFFECT_TRON       // Effetto moto Tron
#define EFFECT_GALAGA     // Effetto astronave Galaga (cannone in angolo che spara alle lettere)
#define EFFECT_GALAGA2    // Effetto Galaga 2 (astronave volante come nell'emulatore HTML)
#define EFFECT_ANALOG_CLOCK // Orologio analogico con immagine di sfondo da SD card
#define EFFECT_FLIP_CLOCK   // Orologio a palette flip stile vintage anni '70
#define EFFECT_BTTF         // Quadrante DeLorean stile Ritorno al Futuro
#define EFFECT_LED_RING     // Orologio circolare con LED che rappresentano minuti e ore
#define EFFECT_WEATHER_STATION // Stazione meteo completa con tutti i dati e icone grafiche
// #define EFFECT_GEMINI_AI    // Integrazione Google Gemini AI con audio WiFi (ESP32C3) - TEMPORANEAMENTE DISABILITATA
#define EFFECT_CLOCK          // Orologio analogico con skin personalizzabili e subdial meteo (richiede BME280 e Open Meteo)
//#define EFFECT_MJPEG_STREAM   // Streaming video MJPEG via WiFi da server Python (con audio opzionale) - DISABILITATO
#define EFFECT_ESP32CAM       // Streaming video da ESP32-CAM Freenove (MJPEG diretto dalla camera)
#define EFFECT_FLUX_CAPACITOR // Flusso canalizzatore animato stile Ritorno al Futuro
#define EFFECT_CHRISTMAS      // Tema natalizio con albero di Natale e neve che si accumula
#define EFFECT_FIRE           // Effetto fuoco camino a schermo intero (Fire2012)
#define EFFECT_FIRE_TEXT      // Effetto lettere fiammeggianti (orario che brucia)
#define EFFECT_MP3_PLAYER     // Lettore MP3/WAV da SD card - ABILITATO (conflitto DMA risolto)
#define EFFECT_WEB_RADIO      // Interfaccia Web Radio a display con controlli touch
#define EFFECT_RADIO_ALARM    // Radiosveglia con selezione stazione WebRadio
// #define EFFECT_WEB_TV         // DISABILITATO - Troppo lag per ESP32
#define EFFECT_CALENDAR
#define EFFECT_LED_RGB        // LED RGB WS2812 (12 LED) su GPIO43 - colori per tema

// ================== INCLUSIONE LIBRERIE ==================
#include <Arduino.h>             // Libreria base per la programmazione di schede Arduino (ESP32).
#include <WiFi.h>                // Libreria per la gestione della connessione Wi-Fi.
#include <ESPmDNS.h>             // Libreria per il supporto del servizio mDNS (Multicast DNS), che permette di accedere al dispositivo tramite un nome host sulla rete locale.
#include <ArduinoOTA.h>          // Libreria per l'Over-The-Air (OTA) update, che consente di aggiornare il firmware del dispositivo via Wi-Fi.
#include <ezTime.h>              // Libreria per la gestione di data e ora, inclusa la sincronizzazione con server NTP (Network Time Protocol) e la gestione dei fusi orari.
#include <Espalexa.h>            // Libreria per l'integrazione con Amazon Alexa, permettendo il controllo del dispositivo tramite comandi vocali.
#include <EEPROM.h>              // Libreria per la lettura e scrittura nella memoria EEPROM (Electrically Erasable Programmable Read-Only Memory) del chip, utilizzata per memorizzare dati persistenti.
#include <U8g2lib.h>             //i Libreria per la gestione di display grafici monocromatici (anche se in questo codice sembra essere usato un display a colori).
#include <Arduino_GFX_Library.h> // Libreria generica per la gestione di display grafici, che supporta diversi tipi di display a colori.
#include <TAMC_GT911.h>          // Libreria specifica per la gestione del controller touch screen GT911.
#include <Wire.h>                // Libreria per la comunicazione tramite protocollo I2C (Inter-Integrated Circuit).
#include "SPI.h"                 // Libreria per la comunicazione tramite protocollo SPI (Serial Peripheral Interface).
#include <WiFiManager.h>         // Libreria per la gestione semplificata della connessione Wi-Fi, spesso tramite un captive portal (una pagina web che si apre automaticamente quando ci si connette a una rete Wi-Fi senza credenziali).
#include <HTTPClient.h>          // Libreria per effettuare richieste HTTP (Hypertext Transfer Protocol), utile per comunicare con server web.
#include <LittleFS.h>
#include <SD.h>                  // Libreria per l'utilizzo di schede SD tramite interfaccia SPI.
#include <JPEGDEC.h>             // Libreria per decodificare immagini JPEG.
#include <ESPAsyncWebServer.h>   // Libreria per web server asincrono ad alte prestazioni.
#include <AsyncTCP.h>            // Libreria per comunicazioni TCP asincrone (richiesta da ESPAsyncWebServer).
#ifdef EFFECT_LED_RGB
#include <Adafruit_NeoPixel.h>   // Libreria per LED RGB WS2812
#endif
#include "home_web_html.h"       // HTML per homepage con link a tutte le pagine

/////////////////TEST NUOVO I2S AUDIO////////////////////////////////////////////////////
#include <Audio.h>

Audio audio; // 'Audio' è la classe della libreria, 'audio' è il nome che usi nel codice

// ================== WEB RADIO CONTROL ==================
bool webRadioEnabled = false;  // Stato attuale della web radio (on/off)
String webRadioUrl = "http://icestreaming.rai.it/1.mp3";  // URL stream radio corrente
String webRadioName = "Rai Radio 1";  // Nome radio corrente
uint8_t webRadioVolume = 100;  // Volume radio (0-100)
int webRadioCurrentIndex = 0; // Indice radio corrente nella lista

// Struttura per una stazione radio
struct WebRadioStation {
  String name;
  String url;
};

// Lista radio (caricata da SD, max 50 stazioni)
#define MAX_RADIO_STATIONS 50
WebRadioStation webRadioStations[MAX_RADIO_STATIONS];
int webRadioStationCount = 0;

// Prototipi funzioni Web Radio (implementate più avanti nel file)
void startWebRadio();
void stopWebRadio();
void setWebRadioVolume(uint8_t vol);
void loadWebRadioSettings();
void saveWebRadioSettings();
void loadWebRadioStationsFromSD();
void saveWebRadioStationsToSD();
void initDefaultRadioStations();
bool addWebRadioStation(const String& name, const String& url);
bool removeWebRadioStation(int index);
void selectWebRadioStation(int index);
/////////////////////////////////////////////////////////////////////////////////////////

// ================== PROTEZIONE CREDITI ==================
#include "protection.h"
/////////////////////////////////////////////////////////////////////////////////////////

// ================== SISTEMA MULTILINGUA ==================
#include "language_config.h"     // Configurazione sistema multilingua (IT/EN)
#ifdef ENABLE_MULTILANGUAGE
// Definizione variabile globale lingua - deve essere qui nel file principale
// perché Arduino compila i .ino in ordine alfabetico
Language currentLanguage = LANG_IT;
#endif

#include "word_mappings.h"       // File di header locale (nello stesso progetto) che probabilmente contiene array o definizioni per mappare parole (stringhe di testo) a specifiche posizioni di LED sulla matrice del display.
#include "sh_logo_480x480new_black.h" // File di header locale che probabilmente contiene i dati binari (array di byte) per visualizzare un logo (in bianco e nero) sul display con risoluzione 480x480 pixel.
#include "qrcode_wifi.h"         // Libreria per la generazione di QR code per configurazione WiFi
#include <MyLD2410.h>             // Libreria MyLD2410 per il radar LD2410 24GHz FMCW con supporto luminosità
#include <Adafruit_BME280.h>      // Libreria per sensore temperatura/pressione/umidità BME280 (I2C)
                                  // IMPORTANTE: Installare da Gestione Librerie: "Adafruit BME280 Library"

// ================== CLASSE OFFSCREEN GFX PER DOUBLE BUFFERING ==================
// Classe condivisa per disegnare su un buffer in memoria invece che sul display
// Usata da: BTTF, LED Ring Clock, Analog Clock
class OffscreenGFX : public Arduino_GFX {
private:
  uint16_t *buffer;
  int16_t _width, _height;

public:
  OffscreenGFX(uint16_t *buf, int16_t w, int16_t h)
    : Arduino_GFX(w, h), buffer(buf), _width(w), _height(h) {}

  bool begin(int32_t speed = GFX_NOT_DEFINED) override { return true; }

  void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {
    buffer[y * _width + x] = color;
  }

  void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override {
    for (int16_t i = 0; i < h; i++) {
      if (y + i >= 0 && y + i < _height && x >= 0 && x < _width) {
        buffer[(y + i) * _width + x] = color;
      }
    }
  }

  void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override {
    for (int16_t i = 0; i < w; i++) {
      if (x + i >= 0 && x + i < _width && y >= 0 && y < _height) {
        buffer[y * _width + (x + i)] = color;
      }
    }
  }

  void flush() override {}
};

#ifdef EFFECT_BTTF
#include "bttf_types.h"          // Definizioni struct per modalità BTTF (Back to the Future)
#endif

// ESP32-audioI2S (Audio.h) gestisce tutto l'audio I2S - nessun include ESP8266Audio necessario

// ================== CONFIGURAZIONE ==================
// Definizione dei pin
#define I2C_SDA_PIN 19   // Definisce il pin 19 dell'ESP32 come il pin SDA (Serial Data) per la comunicazione I2C.
#define I2C_SCL_PIN 45   // Definisce il pin 45 dell'ESP32 come il pin SCL (Serial Clock) per la comunicazione I2C.
#define TOUCH_INT -1     // Definisce il pin per l'interrupt del touch screen. Un valore di -1 indica che questa funzionalità potrebbe non essere utilizzata o gestita in modo diverso (polling).
#define TOUCH_RST -1     // Definisce il pin per il reset del touch screen. Un valore di -1 indica che il reset potrebbe non essere controllato via software.
#define MAX_PATH_MEMORY 256  // Definisce la dimensione massima dell'array utilizzato per memorizzare il percorso del "serpente" di LED.

#ifdef AUDIO
#define I2S_BCLK      1     // Definisce il pin 1 dell'ESP32 come il pin BCLK (Bit Clock) per l'interfaccia I2S.
#define I2S_LRC       2     // Definisce il pin 2 dell'ESP32 come il pin LRC (Left/Right Clock o Word Select) per l'interfaccia I2S.
#define I2S_DOUT      40    // Definisce il pin 40 dell'ESP32 come il pin DOUT (Data Out) per l'interfaccia I2S.
#define I2S_PIN_ENABLE -1    // Definisce un pin per abilitare/disabilitare l'alimentazione o l'amplificatore audio collegato all'I2S. Un valore di -1 indica che non è utilizzato.
#define VOLUME_LEVEL  1   // Definisce il livello di volume iniziale per la riproduzione AUDIOaudio (1.0 è il volume massimo).
#endif

// Digital I/O used (I2S pins definiti sopra in #ifdef AUDIO)
#ifndef I2S_DOUT
#define I2S_DOUT      40
#define I2S_BCLK      1
#define I2S_LRC       2
#endif

// Configurazione annunci orari automatici (per audio WiFi esterno ESP32C3)
#define ANNOUNCE_START_HOUR 8   // Ora di inizio annunci automatici (dalle 8:00)
#define ANNOUNCE_END_HOUR   20  // Ora di fine annunci automatici (fino alle 19:59, alle 20:00 si blocca)
// Gli annunci orari automatici verranno riprodotti solo tra ANNOUNCE_START_HOUR e ANNOUNCE_END_HOUR
// Esempio: con 8 e 20, annuncia dalle 8:00 alle 19:59, silenzio dalle 20:00 alle 7:59

// Definizione dei pin per LD2410 Radar (UART)
// Pin IO1 e IO2 sono LIBERI - audio WiFi usa ESP32C3 esterno (nessun conflitto)
#define LD2410_RX_PIN 2       // Pin RX per LD2410 (collegato al TX del radar)
#define LD2410_TX_PIN 1       // Pin TX per LD2410 (collegato al RX del radar)
#define LD2410_BAUD_RATE 256000 // Baud rate per comunicazione UART con radar LD2410
// NOTA: Sensore luminosità LD2410 richiede libreria MyLD2410 (non disponibile con ld2410 standard)

// Configurazione SD Card
#define SD_CS_PIN   42  // Pin CS (Chip Select) per SD Card
#define SD_MOSI_PIN 47  // Pin MOSI (Master Out Slave In) per SD Card (condiviso con display)
#define SD_CLK_PIN  48  // Pin CLK (Clock) per SD Card (condiviso con display)
#define SD_MISO_PIN 41  // Pin MISO (Master In Slave Out) per SD Card (condiviso, inizializzare DOPO Audio WiFi)

// Configurazione PWM (Pulse Width Modulation) per il controllo della luminosità del display
#define GFX_BL 38            // Definisce il pin 38 dell'ESP32 come il pin utilizzato per controllare la retroilluminazione (BackLight) del display.
#define PWM_CHANNEL     0    // Definisce il canale PWM (da 0 a 15 sull'ESP32) che verrà utilizzato per controllare la luminosità.
#define PWM_FREQ        1000   // Definisce la frequenza del segnale PWM in Hertz (Hz). Una frequenza più alta di solito porta a uno sfarfallio meno percepibile.
#define PWM_RESOLUTION  8    // Definisce la risoluzione del segnale PWM in bit. 8 bit significa che ci sono 2^8 = 256 livelli di luminosità possibili (da 0 a 255).

// Configurazione touch screen
#define TOUCH_ROTATION ROTATION_NORMAL // Definisce l'orientamento del touch screen come "normale" (non ruotato). Altri valori potrebbero essere ROTATION_90, ROTATION_180, ROTATION_270.
#define TOUCH_MAP_X1 480      // Definisce il valore massimo letto sull'asse X dal sensore touch.
#define TOUCH_MAP_X2 0        // Definisce il valore minimo letto sull'asse X dal sensore touch.
#define TOUCH_MAP_Y1 480      // Definisce il valore massimo letto sull'asse Y dal sensore touch.
#define TOUCH_MAP_Y2 0        // Definisce il valore minimo letto sull'asse Y dal sensore touch. Questi valori vengono usati per mappare le coordinate del touch sulle coordinate del display.
#define TOUCH_DEBOUNCE_MS 300 // Definisce il tempo minimo in millisecondi che deve trascorrere tra due tocchi consecutivi per essere considerati distinti, evitando "rimbalzi" o letture multiple di un singolo tocco.

// Costanti per la pagina di setup (menu di configurazione)
#define SETUP_SCROLL_THRESHOLD 40      // Definisce la distanza minima in pixel che un tocco deve spostarsi per essere considerato uno "scroll" nel menu di setup.
#define SETUP_TIMEOUT 30000            // Definisce il tempo massimo in millisecondi (30 secondi) di inattività sulla pagina di setup prima che il sistema esca automaticamente.
#define SETUP_ITEMS_COUNT 5            // Definisce il numero totale di elementi presenti nel menu di setup.
#define LONG_PRESS_THRESHOLD 600      // Definisce la durata minima in millisecondi di un tocco continuo per essere considerato una "pressione lunga".

// Configurazione degli effetti di visualizzazione
#define SNAKE_SPEED 20          // Definisce la velocità iniziale dell'effetto "serpente" di LED, in millisecondi tra un movimento e l'altro (valore più basso = più veloce).
#define SNAKE_MIN_SPEED 20       // Definisce la velocità massima (più veloce) raggiungibile dall'effetto "serpente".
#define SNAKE_MAX_LENGTH 30      // Definisce la lunghezza massima che può raggiungere il "serpente" di LED.
#define SNAKE_ACCELERATION 5     // Definisce di quanto diminuisce l'intervallo di tempo (aumentando la velocità) del "serpente" ogni volta che "mangia una lettera".

// Configurazione dell'effetto "goccia d'acqua"
#define DROP_DURATION 3000      // Definisce la durata totale in millisecondi dell'effetto della "goccia d'acqua".
#define RIPPLE_SPEED 8.0         // Definisce la velocità con cui si espande il "cerchio" dell'onda generata dalla "goccia".
#define MAX_RIPPLE_RADIUS 20     // Definisce il raggio massimo in pixel che può raggiungere l'onda della "goccia".

// Configurazione dell'effetto Tron (moto luminose)
#define NUM_TRON_BIKES      3    // Numero di moto Tron che si muovono simultaneamente sullo schermo.
#define TRON_SPEED          150  // Velocità di movimento delle moto Tron in millisecondi tra un movimento e l'altro.
#define TRON_TRAIL_LENGTH   8    // Lunghezza massima della scia luminosa lasciata da ogni moto.

#define MAX_TIME_LEDS 100  // Definisce il numero massimo di LED che possono essere utilizzati per visualizzare l'ora corrente in una determinata modalità.

// Valori di default per la gestione automatica della luminosità in base all'ora del giorno
#define BRIGHTNESS_DAY_DEFAULT   250 // Livello di luminosità default durante il giorno (valore PWM).
#define BRIGHTNESS_NIGHT_DEFAULT 90  // Livello di luminosità default durante la notte (valore PWM).
#define HOUR_DAY   7    // Definisce l'ora del giorno (in formato 24 ore) a partire dalla quale considerare "giorno".
#define HOUR_NIGHT 19   // Definisce l'ora del giorno (in formato 24 ore) a partire dalla quale considerare "notte".

// Definizione delle costanti per lo stato di visualizzazione della lettera "E"
#define E_DISABLED 0 // La lettera "E" non viene visualizzata.
#define E_ENABLED 1  // La lettera "E" è visualizzata in modo statico.
#define E_BLINK 2    // La lettera "E" lampeggia.

// Configurazione del display (matrice di LED virtuale, anche se il display fisico è diverso)
#define MATRIX_WIDTH  16  // Larghezza della matrice virtuale di LED.
#define MATRIX_HEIGHT 16  // Altezza della matrice virtuale di LED.
#define NUM_LEDS 256    // Numero totale di LED nella matrice virtuale (larghezza * altezza).
#define GFX_DEV_DEVICE ESP32_4848S040_86BOX_GUITION // Definisce il tipo specifico di dispositivo grafico (display) utilizzato.
#define RGB_PANEL          // Indica che il display è un pannello RGB a colori.

// Configurazione degli effetti "MATRIX" (pioggia di LED)
#define MATRIX_START_Y_MIN  -7.0f // Valore minimo per la posizione Y iniziale delle "gocce" di LED (può essere negativo per farle apparire dall'alto).
#define MATRIX_START_Y_MAX   0.0f  // Valore massimo per la posizione Y iniziale delle "gocce" di LED.
#define MATRIX_TRAIL_LENGTH  10   // Lunghezza della "scia" luminosa dietro ogni "goccia".
#define NUM_DROPS           32   // Numero totale di "gocce" di LED da visualizzare contemporaneamente.
#define MATRIX_BASE_SPEED   0.30f // Velocità base di caduta delle "gocce".
#define MATRIX_SPEED_VAR    0.15f // Variazione casuale della velocità delle "gocce".
#define MATRIX2_BASE_SPEED  0.30f // Velocità base per una seconda variante dell'effetto "MATRIX".
#define MATRIX2_SPEED_VAR   0.15f // Variazione casuale della velocità per la seconda variante dell'effetto "MATRIX".

// Configurazione della memoria EEPROM
#define EEPROM_SIZE 1024         // Definisce la dimensione totale in byte della memoria EEPROM utilizzabile.
#define EEPROM_CONFIGURED_MARKER 0x55 // Un valore specifico (byte) scritto in una posizione della EEPROM per indicare se la configurazione iniziale è già stata eseguita.
#define EEPROM_VERSION_ADDR 500       // Indirizzo per versione layout EEPROM
#define EEPROM_VERSION_VALUE 0x02     // Versione 2: nuova disposizione indirizzi (cambiare questo valore per forzare reset)
#define EEPROM_PRESET_ADDR 1      // Indirizzo di memoria nella EEPROM dove è memorizzato il preset di configurazione corrente.
#define EEPROM_BLINK_ADDR 2       // Indirizzo di memoria nella EEPROM dove è memorato lo stato di lampeggio (probabilmente per qualche indicatore).
#define EEPROM_MODE_ADDR 3        // Indirizzo di memoria nella EEPROM dove è memorata la modalità di visualizzazione corrente.
#define EEPROM_WIFI_SSID_ADDR 10     // Indirizzo di memoria di inizio per memorizzare la SSID (nome) della rete Wi-Fi (32 byte riservati).
#define EEPROM_WIFI_PASS_ADDR 42     // Indirizzo di memoria di inizio per memorizzare la password della rete Wi-Fi (64 byte riservati).
#define EEPROM_WIFI_VALID_ADDR 106    // Indirizzo di memoria per un flag (1 byte) che indica se le credenziali Wi-Fi memorizzate sono valide.
#define EEPROM_WIFI_VALID_VALUE 0xAA // Valore (byte) che, se presente nell'indirizzo `EEPROM_WIFI_VALID_ADDR`, indica che le credenziali Wi-Fi sono valide.
#define EEPROM_SETUP_OPTIONS_ADDR 110 // Indirizzo di memoria per memorizzare le opzioni di configurazione del setup (probabilmente un byte che codifica diverse impostazioni).
#define EEPROM_WORD_E_VISIBLE_ADDR 107 // Indirizzo di memoria per memorizzare se la parola "E" deve essere visibile.
#define EEPROM_WORD_E_STATE_ADDR 108   // Indirizzo di memoria per memorizzare lo stato di visualizzazione della lettera "E" (es. abilitata, lampeggiante).
#define EEPROM_COLOR_R_ADDR 130       // Indirizzo di memoria per memorizzare la componente rossa (Red) del colore preferito dall'utente.
#define EEPROM_COLOR_G_ADDR 131       // Indirizzo di memoria per memorizzare la componente verde (Green) del colore preferito dall'utente.
#define EEPROM_COLOR_B_ADDR 132       // Indirizzo di memoria per memorizzare la componente blu (Blue) del colore preferito dall'utente.
#define EEPROM_CLOCK_DATE_ADDR 133    // Indirizzo di memoria per memorizzare lo stato di visualizzazione della data sull'orologio analogico (0=nascosta, 1=visibile).
#define EEPROM_AUDIO_DAY_ENABLED_ADDR 134   // Indirizzo per abilitare audio durante il giorno (1=on, 0=off)
#define EEPROM_AUDIO_NIGHT_ENABLED_ADDR 135 // Indirizzo per abilitare audio durante la notte (1=on, 0=off)
#define EEPROM_VOLUME_DAY_ADDR 136          // Indirizzo per volume audio giorno (0-100)
#define EEPROM_VOLUME_NIGHT_ADDR 137        // Indirizzo per volume audio notte (0-100)
#define EEPROM_RADAR_BRIGHTNESS_ADDR 175  // Indirizzo di memoria per abilitare/disabilitare controllo luminosità con radar LD2410
#define EEPROM_RADAR_BRIGHT_MIN_ADDR 176  // Indirizzo per luminosità minima radar (default 90)
#define EEPROM_RADAR_BRIGHT_MAX_ADDR 177  // Indirizzo per luminosità massima radar (default 255)
#define EEPROM_VUMETER_ENABLED_ADDR 178   // Indirizzo per abilitare/disabilitare VU meter durante audio (1=on, 0=off)
#define EEPROM_TTS_ANNOUNCE_ADDR 179      // Indirizzo per usare Google TTS invece di MP3 locali (1=TTS, 0=MP3)
#define EEPROM_TTS_VOICE_FEMALE_ADDR 180  // Indirizzo per voce TTS (1=femminile, 0=maschile)
#define EEPROM_DAY_START_HOUR_ADDR 4      // Indirizzo per ora inizio giorno (default 8)
#define EEPROM_NIGHT_START_HOUR_ADDR 5    // Indirizzo per ora inizio notte (default 22)
#define EEPROM_HOURLY_ANNOUNCE_ADDR 6     // Indirizzo per abilitare/disabilitare annuncio orario (1=on, 0=off)
#define EEPROM_DAY_START_MIN_ADDR 210     // Indirizzo per minuti inizio giorno (default 0)
#define EEPROM_NIGHT_START_MIN_ADDR 211   // Indirizzo per minuti inizio notte (default 0)
#define EEPROM_RANDOM_MODE_ADDR 212       // Indirizzo per abilitare/disabilitare cambio modalità random (1=on, 0=off)
#define EEPROM_RANDOM_INTERVAL_ADDR 213   // Indirizzo per intervallo cambio random in minuti (1-60)
#define EEPROM_BRIGHTNESS_DAY_ADDR 7      // Indirizzo per luminosità giorno (0-255)
#define EEPROM_BRIGHTNESS_NIGHT_ADDR 8    // Indirizzo per luminosità notte (0-255)
#define EEPROM_AUDIO_VOLUME_ADDR 9        // Indirizzo per volume audio ESP32C3 (0-100)
#define EEPROM_RADAR_SERVER_IP_ADDR 220   // Indirizzo per IP radar server remoto (4 byte: 220-223)
#define EEPROM_RADAR_SERVER_ENABLED 224   // Indirizzo per abilitare radar server remoto (1=on, 0=off)
#define EEPROM_RADAR_SERVER_VALID 225     // Marker validità config radar server (0xAA = valido)
#define EEPROM_RADAR_SERVER_VALID_VALUE 0xAA

// Indirizzi EEPROM per Web Radio
#define EEPROM_WEBRADIO_ENABLED_ADDR 216  // Web radio enabled (1=on, 0=off)
#define EEPROM_WEBRADIO_VOLUME_ADDR 217   // Volume web radio (0-21)
#define EEPROM_WEBRADIO_STATION_ADDR 218  // Indice stazione radio selezionata (0-49)

// Indirizzi EEPROM per calibrazione BME280 (offset temperatura e umidità)
// Gli offset sono salvati come int16 * 10 per mantenere un decimale (-100 = -10.0, +55 = +5.5)
#define EEPROM_MP3PLAYER_TRACK_ADDR 226    // Traccia corrente MP3 player (1 byte)
#define EEPROM_MP3PLAYER_PLAYING_ADDR 227  // Stato riproduzione MP3 (1 byte)
#define EEPROM_MP3PLAYER_VOLUME_ADDR 228   // Volume MP3 player (1 byte, 0-21)
#define EEPROM_MP3PLAYER_PLAYALL_ADDR 229  // Modalita' MP3: 0=singolo brano, 1=tutti i brani
#define EEPROM_ANNOUNCE_VOLUME_ADDR 230    // Volume annunci orari (1 byte, 0-21)
#define EEPROM_TOUCH_SOUNDS_VOLUME_ADDR 231  // Volume suoni touch (1 byte, 0-21)

#define EEPROM_BME280_TEMP_OFFSET_ADDR 355  // Offset temperatura (2 byte, int16)
#define EEPROM_BME280_HUM_OFFSET_ADDR 357   // Offset umidità (2 byte, int16)
#define EEPROM_BME280_CALIB_VALID_ADDR 359  // Marker validità calibrazione (1 byte, 0xBB = valido)
#define EEPROM_BME280_CALIB_VALID_VALUE 0xBB

String ino = __FILE__; // Variabile globale di tipo String che viene inizializzata con il nome del file corrente (.ino).

// ================== MODALITÀ DI VISUALIZZAZIONE ==================
enum DisplayMode {
  MODE_FADE = 0,   // Modalità di visualizzazione con effetto di dissolvenza.
  MODE_SLOW = 1,   // Modalità di visualizzazione lenta (definizione specifica dell'effetto non chiara qui).
  MODE_FAST = 2,   // Modalità di visualizzazione veloce (definizione specifica dell'effetto non chiara qui).
  MODE_MATRIX = 3, // Modalità di visualizzazione con l'effetto "matrice" (pioggia di LED).
  MODE_MATRIX2 = 4, // Una seconda modalità di visualizzazione con una variante dell'effetto "matrice".
  MODE_SNAKE = 5,  // Modalità di visualizzazione con l'effetto "serpente" di LED.
  MODE_WATER = 6,  // Modalità di visualizzazione con l'effetto "goccia d'acqua".
  MODE_MARIO = 7,  // Modalità con sprite Mario Bros che disegna l'orario.
  MODE_TRON = 8,   // Modalità con moto Tron che lasciano scie luminose e disegnano l'orario.
#ifdef EFFECT_GALAGA
  MODE_GALAGA = 9, // Modalità con astronave Galaga che spara per accendere le lettere.
#endif
#ifdef EFFECT_GALAGA2
  MODE_GALAGA2 = 17, // Modalità con astronave Galaga volante che insegue e spara le lettere.
#endif
#ifdef EFFECT_ANALOG_CLOCK
  MODE_ANALOG_CLOCK = 10, // Orologio analogico con immagine di sfondo e lancette.
#endif
#ifdef EFFECT_FLIP_CLOCK
  MODE_FLIP_CLOCK = 11,   // Orologio a palette flip stile vintage anni '70 (HH:MM).
#endif
#ifdef EFFECT_BTTF
  MODE_BTTF = 12,         // Quadrante DeLorean stile Ritorno al Futuro con tre pannelli data/ora.
#endif
#ifdef EFFECT_LED_RING
  MODE_LED_RING = 13,     // Orologio circolare con LED per minuti e ore più display digitale.
#endif
#ifdef EFFECT_WEATHER_STATION
  MODE_WEATHER_STATION = 14, // Stazione meteo completa con tutti i dati e icone grafiche.
#endif
#ifdef EFFECT_CLOCK
  MODE_CLOCK = 15,        // Orologio analogico con skin e subdial meteo (richiede BME280 + OpenWeatherMap).
#endif
#ifdef EFFECT_GEMINI_AI
  MODE_GEMINI_AI = 16,    // Interfaccia interattiva Google Gemini AI con scrolling.
#endif
#ifdef EFFECT_MJPEG_STREAM
  MODE_MJPEG_STREAM = 18, // Streaming video MJPEG via WiFi da server Python.
#endif
#ifdef EFFECT_ESP32CAM
  MODE_ESP32CAM = 19,     // Streaming video da ESP32-CAM Freenove.
#endif
#ifdef EFFECT_FLUX_CAPACITOR
  MODE_FLUX_CAPACITOR = 20, // Flusso canalizzatore animato stile Ritorno al Futuro.
#endif
#ifdef EFFECT_CHRISTMAS
  MODE_CHRISTMAS = 21,      // Tema natalizio con albero di Natale e neve che si accumula.
#endif
#ifdef EFFECT_FIRE
  MODE_FIRE = 22,           // Effetto fuoco camino a schermo intero.
#endif
#ifdef EFFECT_FIRE_TEXT
  MODE_FIRE_TEXT = 23,      // Effetto lettere fiammeggianti (orario che brucia).
#endif
#ifdef EFFECT_MP3_PLAYER
  MODE_MP3_PLAYER = 24,     // Lettore MP3/WAV da SD card.
#endif
#ifdef EFFECT_WEB_RADIO
  MODE_WEB_RADIO = 25,      // Interfaccia Web Radio con controlli touch.
#endif
#ifdef EFFECT_RADIO_ALARM
  MODE_RADIO_ALARM = 26,    // Radiosveglia con selezione stazione.
#endif
#ifdef EFFECT_WEB_TV
  MODE_WEB_TV = 27,         // Streaming TV da server Python via MJPEG.
#endif
#ifdef EFFECT_CALENDAR
  MODE_CALENDAR = 28,       // Calendario Google
#endif
  NUM_MODES = 29    // Costante che indica il numero totale di modalità di visualizzazione definite nell'enum.
};

// ================== STRUTTURE DATI ==================
// Struttura per memorizzare i LED dell'orario corrente per l'effetto SNAKE
struct TimeDisplay {
  uint16_t positions[MAX_TIME_LEDS];  // Array di tipo uint16_t (unsigned integer a 16 bit) per memorizzare gli indici (posizioni) dei LED che rappresentano l'ora corrente nell'effetto "serpente".
  uint16_t count;                    // Variabile di tipo uint16_t per tenere traccia del numero di LED effettivamente utilizzati per visualizzare l'ora.
};

// Struttura per memorizzare il percorso del "serpente"
struct SnakePath {
  uint16_t positions[MAX_PATH_MEMORY];  // Array per memorizzare le posizioni
uint16_t length;                      // Variabile di tipo uint16_t per memorizzare la lunghezza attuale del percorso del serpente.
  bool reversed;                        // Variabile di tipo bool (booleano, vero o falso) per indicare se il serpente sta percorrendo il suo percorso all'indietro.
};

// Strutture dati per gestire il serpente (l'effetto visivo)
struct SnakeSegment {
  uint16_t ledIndex;             // Variabile di tipo uint16_t per memorizzare la posizione di un singolo segmento del serpente come indice del LED nella matrice.
};

struct Snake {
  SnakeSegment segments[SNAKE_MAX_LENGTH];  // Array di strutture `SnakeSegment` per rappresentare tutti i segmenti del serpente. La dimensione massima è definita da `SNAKE_MAX_LENGTH`.
  uint8_t length;                // Variabile di tipo uint8_t (unsigned integer a 8 bit) per memorizzare la lunghezza attuale del serpente (numero di segmenti).
  uint16_t speed;                // Variabile di tipo uint16_t per memorizzare la velocità attuale del serpente, espressa come intervallo di tempo in millisecondi tra un movimento e l'altro.
  bool gameOver;                 // Variabile di tipo bool per indicare se il "gioco" del serpente è finito (ad esempio, se ha colpito se stesso).
  bool wordCompleted;            // Variabile di tipo bool per indicare se tutte le parole da illuminare sono state "mangiate" dal serpente.
  uint32_t lastMove;             // Variabile di tipo uint32_t (unsigned integer a 32 bit) per memorizzare il timestamp dell'ultimo movimento del serpente (in millisecondi).
  bool isVertical;               // Variabile di tipo bool per indicare se la direzione principale di movimento del serpente è verticale.
};

// Struttura dati per l'effetto "goccia d'acqua"
struct WaterDrop {
  float centerX;                 // Variabile di tipo float (numero in virgola mobile a precisione singola) per memorizzare la coordinata X del centro della goccia.
  float centerY;                 // Variabile di tipo float per memorizzare la coordinata Y del centro della goccia.
  float currentRadius;           // Variabile di tipo float per memorizzare il raggio attuale dell'onda che si propaga dalla goccia.
  uint32_t startTime;            // Variabile di tipo uint32_t per memorizzare il timestamp (in millisecondi) in cui è iniziato l'effetto della goccia.
  bool active;                   // Variabile di tipo bool per indicare se l'effetto della goccia è attualmente attivo.
  bool completed;                // Variabile di tipo bool per indicare se l'effetto della goccia ha completato la sua animazione.
  bool cleanupDone;              // Variabile di tipo bool per indicare se le operazioni di "pulizia" (reset di variabili) dopo il completamento dell'effetto sono state eseguite.
};

// Struttura dati per lo sprite Mario
struct MarioSprite {
  float x;                       // Posizione X corrente di Mario (in pixel).
  float y;                       // Posizione Y corrente di Mario (in pixel).
  float velocityX;               // Velocità orizzontale di Mario.
  float velocityY;               // Velocità verticale di Mario (per i salti).
  uint8_t animFrame;             // Frame corrente dell'animazione (0-3 per camminata).
  uint32_t lastAnimUpdate;       // Timestamp dell'ultimo aggiornamento dell'animazione.
  uint32_t lastMove;             // Timestamp dell'ultimo movimento.
  bool facingRight;              // true se Mario guarda a destra, false se guarda a sinistra.
  bool isJumping;                // true se Mario sta saltando.
  bool isOnGround;               // true se Mario è a terra.
  uint16_t pathIndex;            // Indice corrente nel percorso zig-zag (0-255).
  bool missionComplete;          // true quando Mario ha disegnato tutto l'orario.
};

// Struttura dati per gestire i colori RGB (Rosso, Verde, Blu)
struct RGB {
  uint8_t r; // Componente rossa del colore (valore da 0 a 255).
  uint8_t g; // Componente verde del colore (valore da 0 a 255).
  uint8_t b; // Componente blu del colore (valore da 0 a 255).

  // Costruttori
  RGB() : r(255), g(255), b(255) {} // Costruttore predefinito che inizializza il colore a bianco (tutte le componenti al massimo).
  RGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {} // Costruttore che permette di specificare i valori delle componenti RGB all'atto della creazione dell'oggetto.

  // Operatori utili per confrontare oggetti RGB
  bool operator==(const RGB& other) const { // Operatore di uguaglianza (==) che confronta due oggetti RGB e restituisce true se tutte le loro componenti sono uguali.
    return r == other.r && g == other.g && b == other.b;
  }

  bool operator!=(const RGB& other) const { // Operatore di disuguaglianza (!=) che confronta due oggetti RGB e restituisce true se almeno una delle loro componenti è diversa.
    return !(*this == other); // Utilizza l'operatore di uguaglianza per definire la disuguaglianza.
  }
};

// Struttura dati per la gestione dei colori (utilizzata in modo simile a RGB ma potenzialmente con un nome più generico)
struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  // Costruttori (identici a quelli di RGB)
  Color() : r(255), g(255), b(255) {}
  Color(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}

  // Conversione implicita a RGB
  operator RGB() const { // Operatore di conversione che permette di utilizzare un oggetto `Color` dove è atteso un oggetto `RGB`. Restituisce un nuovo oggetto `RGB` con le stesse componenti.
    return RGB(r, g, b);
  }
};

// Struttura dati per le lancette dell'orologio analogico (usata da EFFECT_CLOCK)
struct ClockHand {
  float angle;                   // Angolo corrente della lancetta (0-360 gradi, 0=ore 12).
  uint8_t length;                // Lunghezza della lancetta in pixel dal centro.
  uint8_t thickness;             // Spessore della lancetta in pixel.
  Color color;                   // Colore della lancetta.
};

// Struct per le parole dei minuti (decine e unità)
struct MinuteWords {
  const uint8_t* tens; // Puntatore a un array di byte (probabilmente indici nella mappa delle parole) per la decina dei minuti.
  const uint8_t* ones; // Puntatore a un array di byte per l'unità dei minuti.
};

// Struttura per un singolo fuoco d'artificio (effetto Capodanno)
struct Firework {
  int16_t x, y;                         // Posizione centro esplosione
  float radius;                         // Raggio corrente dell'esplosione
  float maxRadius;                      // Raggio massimo dell'esplosione
  float speed;                          // Velocità di espansione
  uint16_t color;                       // Colore RGB565
  bool active;                          // Se il fuoco è attivo
  uint32_t startTime;                   // Tempo di inizio esplosione
  uint8_t sparkles;                     // Numero di scintille
};

// Struttura per configurazione campo data (usata da EFFECT_CLOCK in 3_EFFETTI.ino)
struct DateField {
  bool enabled;      // Campo abilitato/disabilitato
  int x;             // Posizione X
  int y;             // Posizione Y
  uint16_t color;    // Colore testo RGB565
  uint8_t fontSize;  // Dimensione font (1-5)
};

// ================== DICHIARAZIONI FORWARD ==================

void testSpeakerSafely();
// Funzioni definite in 1_TOUCH.ino
void checkButtons();
void playTouchSound();

// Funzioni definite in 2_CHANGE_MODE.ino
void cleanupPreviousMode(DisplayMode previousMode);  // Cleanup risorse modalità precedente
void forceDisplayUpdate();
void applyPreset(uint8_t preset);
void handleModeChange();
bool isValidMode(DisplayMode mode);

// Funzioni definite in 3_EFFETTI.ino
bool updateIntro();
void displayWifiInit();
void displayWordWifi(uint8_t pos, const String& text);
void displayWifiDot(uint8_t n);
void updateFastMode();
void updateSlowMode();
void updateFadeMode();
void fadeWordPixels(const uint8_t* word, uint8_t step);
MinuteWords getMinuteWord(uint8_t minutes);
void updateMatrix();
bool areAllDropsInactive();
void updateMatrix2();
bool canDropInColumn(uint8_t col);
bool areAllLettersDrawn();
void displayWordToTarget(const uint8_t* word);
void displayWordToTarget_1(const uint8_t* word);
void displayWordToTarget_2(const uint8_t* word);
void displayMinutesToTarget(uint8_t minutes);
void updateWaterDrop();
void ledIndexToXY(uint16_t index, uint8_t &x, uint8_t &y);
float distance(float x1, float y1, float x2, float y2);
void updateSnake();
void updateMarioMode();
void eraseMarioSprite(int x, int y);
void drawMarioSprite(int x, int y, uint8_t animFrame);
void updateTron();
void updateGalagaMode();
void updateGalagaMode2();
void updateAnalogClock();
void displayWord(const uint8_t* word, const Color& color);
void showMinutes(uint8_t minute, const Color& color);

// Funzioni definite in 4_MENU.ino
bool checkSetupScroll();
void handleSetupTouch(int x, int y);
void hideSetupPage();
void updateSetupPage();
void initSetupMenu();
void initSetupOptions();
void saveSetupOptions();

// Funzioni definite in 10_FLIP_CLOCK.ino
void updateFlipClock();
void playFlipSound(uint8_t numClacks, uint8_t flapIndex);

// Funzioni definite in 12_BTTF.ino
void updateBTTF();

// Funzioni definite in 27_FLUX_CAPACITOR.ino
#ifdef EFFECT_FLUX_CAPACITOR
void updateFluxCapacitor();
#endif

// Funzioni definite in 18_LED_RING_CLOCK.ino
void updateLedRingClock();

// Funzioni definite in 20_WEATHER_STATION.ino
void initWeatherStation();
void updateWeatherStation();

// Funzioni definite in 35_CALENDAR.ino
bool initCalendarStation();
void updateCalendarStation();
void fetchGoogleCalendarData();
void drawCalendarEvents();
void handleCalendarTouch(int x, int y);
void checkCalendarAlarm();
void updateCalendarAlarmSound();
void stopCalendarAlarm();
void snoozeCalendarAlarm();
void loadCalSnoozeFromEEPROM();
void saveCalSnoozeToEEPROM();

// Funzioni definite in 36_WEBSERVER_CALENDAR.ino
void setup_calendar_webserver(AsyncWebServer* server);
void loadLocalEventsFromLittleFS();
void mergeLocalAndGoogleEvents();
bool pushEventToGoogle(uint16_t id);
bool deleteEventFromGoogleById(uint16_t id);

// Funzioni definite in 37_LED_RGB.ino e 38_WEBSERVER_LED_RGB.ino
#ifdef EFFECT_LED_RGB
void setup_led_rgb();
void updateLedRgb();
void saveLedRgbSettings();
void getLedColorForMode(DisplayMode mode, uint8_t &r, uint8_t &g, uint8_t &b);
void setup_ledrgb_webserver(AsyncWebServer* server);
extern bool    ledRgbEnabled;
extern uint8_t ledRgbBrightness;
extern bool    ledRgbOverride;
extern uint8_t ledRgbOverrideR;
extern uint8_t ledRgbOverrideG;
extern uint8_t ledRgbOverrideB;
extern uint8_t ledAudioReactive;
#endif

// Funzioni definite in 8_CLOCK.ino (EFFECT_CLOCK)
#ifdef EFFECT_CLOCK
void updateClockMode();
bool canUseEffectClock();  // Verifica se BME280 e OpenWeatherMap API sono disponibili
extern bool clockInitNeeded;  // Dichiarato in 8_CLOCK.ino, usato anche in 2_CHANGE_MODE.ino
#endif

// Funzioni definite in 21_MJPEG_STREAM.ino (EFFECT_MJPEG_STREAM)
#ifdef EFFECT_MJPEG_STREAM
void updateMjpegStream();
void cleanupMjpegStream();
void setMjpegStreamUrl(const String& url);
String getMjpegStreamUrl();
extern bool mjpegInitialized;  // Flag inizializzazione streaming
#endif

// Flag inizializzazione per modalità speciali (definiti nei rispettivi file .ino)
#ifdef EFFECT_LED_RING
extern bool ledRingInitialized;
#endif
#ifdef EFFECT_FLIP_CLOCK
extern bool flipClockInitialized;
#endif
#ifdef EFFECT_BTTF
extern bool bttfInitialized;
#endif
#ifdef EFFECT_WEATHER_STATION
extern bool weatherStationInitialized;
#endif
#ifdef EFFECT_FLUX_CAPACITOR
extern bool fluxCapacitorInitialized;
#endif
#ifdef EFFECT_CALENDAR
extern bool calendarStationInitialized;
extern bool calendarGridView;
extern bool calendarAlarmActive;
extern int calAlarmSnoozeMinutes;
#endif


// Funzioni definite in 14_CAPODANNO.ino
void updateCapodanno();
void checkCapodannoTrigger();

// Funzioni definite in 28_CHRISTMAS.ino
#ifdef EFFECT_CHRISTMAS
void initChristmas();
void updateChristmas();
extern bool christmasInitialized;
#endif

// Funzioni definite in 29_FIRE.ino
#ifdef EFFECT_FIRE
void initFire();
void updateFire();
extern bool fireInitialized;
#endif

// Funzioni definite in 30_FIRE_TEXT.ino
#ifdef EFFECT_FIRE_TEXT
void initFireText();
void updateFireText();
extern bool fireTextInitialized;
#endif

#ifdef EFFECT_MP3_PLAYER
void initMP3Player();
void updateMP3Player();
void cleanupMP3Player();
bool handleMP3PlayerTouch(int16_t x, int16_t y);
extern bool mp3PlayerInitialized;
#endif

#ifdef EFFECT_WEB_RADIO
void initWebRadioUI();
void updateWebRadioUI();
bool handleWebRadioTouch(int16_t x, int16_t y);
void setup_webradio_webserver(AsyncWebServer* server);
extern bool webRadioInitialized;
extern bool webRadioNeedsRedraw;
#endif

#ifdef EFFECT_RADIO_ALARM
// Struttura dati radiosveglia (definita in 33_RADIO_ALARM.ino)
struct RadioAlarmSettings {
  bool enabled;
  uint8_t hour;
  uint8_t minute;
  uint8_t stationIndex;
  uint8_t daysMask;
  uint8_t volume;
  uint8_t snoozeMinutes;
};
extern RadioAlarmSettings radioAlarm;
void initRadioAlarm();
void updateRadioAlarm();
void checkRadioAlarmTrigger();
void loadRadioAlarmSettings();  // Carica impostazioni da EEPROM
void setup_radioalarm_webserver(AsyncWebServer* server);
extern bool radioAlarmInitialized;
extern bool radioAlarmNeedsRedraw;
#endif

#ifdef EFFECT_MP3_PLAYER
void setup_mp3player_webserver(AsyncWebServer* server);
#endif

#ifdef EFFECT_WEB_TV
void initWebTV();
void updateWebTV();
void setup_webtv_webserver(AsyncWebServer* server);
extern bool webTVInitialized;
extern bool webTVNeedsRedraw;
#endif

// Struttura dati per le moto Tron
struct TronBike {
  uint8_t x;                     // Posizione X corrente della moto (coordinate matrice).
  uint8_t y;                     // Posizione Y corrente della moto (coordinate matrice).
  uint8_t direction;             // Direzione corrente: 0=su, 1=destra, 2=giù, 3=sinistra.
  Color color;                   // Colore della moto e della sua scia.
  bool active;                   // true se la moto è attiva.
  bool crashed;                  // true se la moto si è schiantata.
  uint32_t lastMove;             // Timestamp dell'ultimo movimento.
  uint32_t crashTime;            // Timestamp dello schianto (per gestire respawn).
  uint16_t trail[TRON_TRAIL_LENGTH];  // Array delle posizioni della scia luminosa.
  uint8_t trailLength;           // Lunghezza corrente della scia.
};

// Struttura dati per i proiettili di Galaga
struct GalagaBullet {
  float x;                       // Posizione X del proiettile (in pixel).
  float y;                       // Posizione Y del proiettile (in pixel).
  float velocityX;               // Velocità orizzontale del proiettile.
  float velocityY;               // Velocità verticale del proiettile.
  bool active;                   // true se il proiettile è attivo.
  uint32_t creationTime;         // Timestamp di creazione del proiettile.
};

// Struttura dati per le stelle dello sfondo
struct GalagaStar {
  float x;                       // Posizione X della stella (in pixel).
  float y;                       // Posizione Y della stella (in pixel).
  float speed;                   // Velocità di movimento della stella.
  uint8_t brightness;            // Luminosità della stella (0-255).
  uint8_t size;                  // Dimensione della stella (1-3 pixel).
};

// Struttura dati per il cannone Galaga
struct GalagaShip {
  float x;                       // Posizione X del cannone (in pixel) - fissa in basso a destra.
  float y;                       // Posizione Y del cannone (in pixel) - fissa in basso a destra.
  float angle;                   // Angolo di rotazione verso il target corrente.
  float targetAngle;             // Angolo verso cui il cannone sta ruotando.
  uint8_t animFrame;             // Frame corrente dell'animazione (0-1).
  uint32_t lastAnimUpdate;       // Timestamp dell'ultimo aggiornamento dell'animazione.
  uint32_t lastShot;             // Timestamp dell'ultimo colpo sparato.
  uint16_t currentTargetIndex;   // Indice della lettera target corrente.
  uint16_t targetList[NUM_LEDS]; // Array con gli indici delle lettere da colpire.
  uint16_t targetCount;          // Numero totale di lettere target.
  uint8_t passNumber;            // Numero del passaggio corrente (1, 2, 3...).
  bool readyToShoot;             // true quando il cannone è allineato al target.
  bool missionComplete;          // true quando tutte le lettere sono state colpite.
};

// Struttura dati per l'astronave volante Galaga2 (stile emulatore HTML)
struct Galaga2Ship {
  float x;                       // Posizione X dell'astronave (in pixel).
  float y;                       // Posizione Y dell'astronave (in pixel).
  uint8_t animFrame;             // Frame corrente dell'animazione sprite (0-1).
  uint32_t lastAnimUpdate;       // Timestamp dell'ultimo aggiornamento dell'animazione.
  uint16_t currentTargetIndex;   // Indice della lettera target corrente.
  uint16_t targetList[NUM_LEDS]; // Array con gli indici delle lettere da colpire (ordinate).
  uint16_t targetCount;          // Numero totale di lettere target.
  bool active;                   // true se l'astronave è attiva.
  bool missionComplete;          // true quando tutte le lettere sono state colpite.
};

// Struttura dati per la gestione del ciclo dei colori (effetto arcobaleno)
struct colorCycleState {
  bool isActive;                // Variabile di tipo bool per indicare se l'effetto di ciclo dei colori è attivo.
  uint32_t lastColorChange;     // Variabile di tipo uint32_t per memorizzare il timestamp (in millisecondi) dell'ultimo cambio di colore nel ciclo.
  uint16_t hue = 0;         // Variabile di tipo uint16_t per memorizzare la tonalità (Hue) nel modello di colore HSV (Hue, Saturation, Value), con valori da 0 a 360 gradi.
  uint8_t saturation = 255; // Variabile di tipo uint8_t per memorizzare la saturazione nel modello HSV, con valori da 0 (grigio) a 255 (colore pieno).
  bool fadingToWhite = false; // Variabile di tipo bool per indicare se il colore sta attualmente sfumando verso il bianco.
  bool showingWhite = false;  // Variabile di tipo bool per indicare se attualmente viene visualizzato il colore bianco.
};

colorCycleState colorCycle; // Dichiarazione di una variabile globale di tipo `colorCycleState` chiamata `colorCycle`.

// Struttura dati per l'effetto "Matrix" (pioggia di LED)
struct Drop {
  uint8_t x;         // Coordinata X della "goccia" nella matrice virtuale.
  float y;         // Coordinata Y della "goccia" (può essere float per un movimento più preciso).
  float speed;     // Velocità di caduta della "goccia".
  uint8_t intensity; // Intensità luminosa della "goccia".
  bool active;     // Flag per indicare se la "goccia" è attualmente attiva e in movimento.
  bool isMatrix2;  // Flag per distinguere tra la prima e la seconda variante dell'effetto "Matrix".
};

// Struttura dati per un elemento del menu di setup
struct SetupMenuItem {
  const char* label;           // Puntatore a una stringa costante (memorizzata nella memoria flash) che rappresenta l'etichetta visualizzata per questa opzione del menu.
  bool* valuePtr;              // Puntatore a una variabile di tipo bool che rappresenta lo stato (on/off) di questa opzione. Utilizzato per modificare direttamente il valore quando l'utente interagisce.
  uint8_t* modeValuePtr;       // Puntatore a una variabile di tipo uint8_t che rappresenta la modalità selezionata (per le opzioni che permettono di scegliere tra diverse modalità).
  bool isModeSelector;         // Variabile di tipo bool per indicare se questo elemento del menu è un selettore di modalità (invece di un semplice on/off).
};

// Struttura dati per memorizzare le opzioni di configurazione del setup
struct SetupOptions {
  bool autoNightModeEnabled;    // Stato (abilitato/disabilitato) della modalità notte automatica (basata sull'ora).
  bool touchSoundsEnabled;      // Stato (abilitato/disabilitato) dei suoni al tocco dello schermo.
  uint8_t touchSoundsVolume;    // Volume dei suoni touch (0-21).
  bool powerSaveEnabled;        // Stato (abilitato/disabilitato) della modalità di risparmio energetico.
  uint8_t defaultDisplayMode;   // Valore che rappresenta la modalità di visualizzazione predefinita all'avvio. Fa riferimento ai valori definiti nell'enum `DisplayMode`.
  bool wifiEnabled;             // Stato (abilitato/disabilitato) del Wi-Fi (potrebbe essere sempre true in questo progetto).
  bool ntpEnabled;              // Stato (abilitato/disabilitato) della sincronizzazione con server NTP (potrebbe essere sempre true se il Wi-Fi è attivo).
  bool alexaEnabled;            // Stato (abilitato/disabilitato) dell'integrazione con Amazon Alexa.
  bool radarBrightnessEnabled;  // Stato (abilitato/disabilitato) del controllo luminosità con radar LD2410.
  bool vuMeterShowEnabled;      // Stato (abilitato/disabilitato) della visualizzazione VU meter durante riproduzione audio.
};

// Struttura dati per rappresentare un pulsante nella pagina di setup
struct SetupButton {
  int x;           // Coordinata X dell'angolo superiore sinistro del pulsante.
  int y;           // Coordinata Y dell'angolo superiore sinistro del pulsante.
  int width;       // Larghezza del pulsante in pixel.
  int height;      // Altezza del pulsante in pixel.
  const char* label; // Puntatore a una stringa costante (etichetta) visualizzata sul pulsante.
  uint16_t color;    // Colore di sfondo del pulsante (in formato colore del display, es. RGB565).
  uint16_t textColor; // Colore del testo visualizzato sul pulsante.
};

// Array di pulsanti per la pagina di setup
#define NUM_SETUP_BUTTONS 5 // Definisce il numero totale di pulsanti nella pagina di setup.
SetupButton setupButtons[NUM_SETUP_BUTTONS] = { // Inizializzazione dell'array di strutture `SetupButton` con le proprietà di ciascun pulsante.
  {60, 120, 360, 50, "CAMBIA COLORE", BLUE, WHITE},
  {60, 190, 360, 50, "REGOLA LUMINOSITA", GREEN, BLACK},
  {60, 260, 360, 50, "IMPOSTAZIONI WIFI", CYAN, BLACK},
  {60, 330, 360, 50, "RESET IMPOSTAZIONI", RED, WHITE},
  {60, 400, 360, 50, "ESCI", YELLOW, BLACK}
};

// ================== DICHIARAZIONE VARIABILI GLOBALI ==================
// Hardware
Arduino_DataBus *bus = new Arduino_SWSPI( // Creazione di un oggetto puntatore alla classe `Arduino_DataBus` per la comunicazione con il display tramite SPI software (SWSPI).
    GFX_NOT_DEFINED /* DC */, 39 /* CS */, // DC (Data/Command), CS (Chip Select)
    48 /* SCK */, 47 /* MOSI */, GFX_NOT_DEFINED /* MISO */); // SCK (Serial Clock), MOSI (Master Out Slave In), MISO (Master In Slave Out) - MISO non definito perché il display è solo in uscita.

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel( // Creazione di un oggetto puntatore alla classe `Arduino_ESP32RGBPanel` per gestire il pannello RGB del display.
    18 /* DE */, 17 /* VSYNC */, 16 /* HSYNC */, 21 /* PCLK */, // DE (Data Enable), VSYNC (Vertical Sync), HSYNC (Horizontal Sync), PCLK (Pixel Clock)
    11 /* R0 */, 12 /* R1 */, 13 /* R2 */, 14 /* R3 */, 0 /* R4 */, // Pin per i bit del colore Rosso
    8 /* G0 */, 20 /* G1 */, 3 /* G2 */, 46 /* G3 */, 9 /* G4 */, 10 /* G5 */, // Pin per i bit del colore Verde
    //8 /* G0 */, 20 /* G1 */, 3 /* G2 */, 46 /* G3 */, 10 /* G4 */, 9 /* G5 */, // Pin per i bit del colore Verde
    4 /* B0 */, 5 /* B1 */, 6 /* B2 */, 7 /* B3 */, 15 /* B4 */, // Pin per i bit del colore Blu
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */, // Timing orizzontale
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */, // Timing verticale
    0 /* pclk_active_neg */, 12000000 /* prefer_speed */, false /* useBigEndian */, // Configurazione del clock dei pixel, velocità preferita, ordine dei byte
    0 /* de_idle_high */, 0 /* pclk_idle_high */, 0 /* bounce_buffer_size_px */); // Stato idle dei segnali DE e PCLK, dimensione del buffer per l'anti-rimbalzo (non usato qui).

Arduino_RGB_Display *gfx = new Arduino_RGB_Display( // Creazione di un oggetto puntatore alla classe `Arduino_RGB_Display`, che fornisce le funzioni di disegno sul display.
    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */, // Dimensioni del display, oggetto del pannello, rotazione, auto-flush
    bus, GFX_NOT_DEFINED /* RST */, st7701_type9_init_operations, sizeof(st7701_type9_init_operations)); // Bus di comunicazione, pin di reset (non usato), sequenza di inizializzazione specifica per il controller del display ST7701.

// Touch
TAMC_GT911 ts = TAMC_GT911(I2C_SDA_PIN, I2C_SCL_PIN, TOUCH_INT, TOUCH_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2)); // Creazione di un oggetto della classe `TAMC_GT911` per interagire con il touch screen tramite I2C. I valori `max` sono usati per assicurare che l'intervallo di mappatura sia corretto.
int touch_last_x = 0, touch_last_y = 0; // Variabili globali per memorizzare le coordinate dell'ultimo tocco rilevato.

// Networking
Timezone myTZ; // Oggetto della classe `Timezone` per gestire il fuso orario locale.
bool ntpSynced = false; // Flag per indicare se NTP è sincronizzato correttamente
uint32_t lastNtpRetry = 0; // Timestamp ultimo tentativo di risincronizzazione NTP
Espalexa espalexa; // Oggetto della classe `Espalexa` per gestire l'integrazione con Amazon Alexa (porta 80).
AsyncWebServer* clockWebServer = nullptr; // Web server asincrono per configurazione orologio (porta 8080).

// ================== VARIABILI GLOBALI CAPODANNO ==================
bool capodannoActive = false;           // Flag per indicare se l'effetto è attivo
uint32_t capodannoStartTime = 0;        // Timestamp di inizio dell'effetto
DisplayMode savedMode = MODE_FADE;      // Modalità salvata prima dell'attivazione (tipo DisplayMode)
bool capodannoTriggeredThisYear = false; // Flag per evitare riattivazioni multiple nello stesso anno
uint16_t capodannoLastYear = 0;         // Anno dell'ultimo trigger
#ifdef AUDIO
bool isPlaying = false;

// Prototipi funzioni audio (implementate in 5_AUDIO.ino)
void playTone(int frequency, int duration_ms);
void playFrequencySweep();
bool playTTS(const String& text, const String& language);
void cleanupAudio();
String myUrlEncode(const String& msg);
bool announceTime();
bool playLocalMP3(const char* filename);
bool playMP3Sequence(const String files[], int count);
bool concatenateMP3Files(const String files[], int count, const char* outputFile);
bool announceTimeLocal();
bool announceBootLocal();
#endif
// ================== PROTOTIPI FUNZIONI AUDIO (sempre disponibili) ==================
void checkTimeAndAnnounce(); // Verifica se è il momento di annunciare l'ora (funziona con audio I2S o WiFi)
bool announceTimeFixed(); // Annuncia l'ora corrente (wrapper per audio I2S o WiFi)
bool playLocalMP3(const char* filename); // Prototipo della funzione per riprodurre un file MP3 locale (disponibile sempre)

// ================== PROTOTIPI FUNZIONI AUDIO I2C (ESP32C3) ==================
void setup_audio_i2c(); // Inizializza comunicazione I2C con ESP32C3 audio slave
void playBootAnnounce(); // Annuncia data/ora al boot (chiamare dopo display inizializzato)
void updateAudioI2C(); // Aggiorna stato connessione I2C audio (opzionale)
bool announceTimeViaI2C(uint8_t hour, uint8_t minute); // Annuncia orario via ESP32C3
bool playBeepViaI2C(); // Riproduci beep.mp3 (feedback tocco)
bool playClackViaI2C(); // Riproduci clack.mp3 (flip clock)

////////////TEST NUOVO I2S AUDIO////////////
void audio_info(const char *info);
// --- Prototipo del Task Audio ---
void audioTask(void *pvParameters);
////////////TEST NUOVO I2S AUDIO////////////

// ================== PROTOTIPI FUNZIONI MAGNETOMETRO (QMC5883P) ==================
// NOTA: Il magnetometro è un QMC5883P (NON QMC5883L!) con indirizzo I2C 0x0C
// Libreria: Adafruit_QMC5883P (https://github.com/adafruit/Adafruit_QMC5883P)
void setup_magnetometer(); // Inizializza magnetometro QMC5883P su I2C
void updateMagnetometer(); // Legge magnetometro e calcola heading (azimuth)

// Variabili per controllo annunci orari (usate sia da audio I2S che audio WiFi)
static unsigned long lastAnnounceTime = 0; // Variabile statica per memorizzare il timestamp dell'ultimo annuncio dell'ora.
static uint8_t lastMinuteChecked = 255; // Variabile statica per tenere traccia dell'ultimo minuto in cui è stato verificato l'annuncio dell'ora (inizializzata a un valore non valido).

// VARIABILI GLOBALI PER GESTIONE LETTERA E
uint8_t word_E_state = 0;           // Variabile globale di tipo uint8_t per memorizzare lo stato di visibilità della lettera "E" (0=disabilitata, 1=abilitata, 2=lampeggiante).
DisplayMode currentMode = MODE_FAST; // Variabile globale di tipo `DisplayMode` per memorizzare la modalità di visualizzazione corrente, inizializzata a `MODE_FAST`.
DisplayMode userMode = MODE_FAST;    // Variabile globale di tipo `DisplayMode` per memorizzare la modalità di visualizzazione preferita dall'utente, inizializzata a `MODE_FAST`.
static DisplayMode prevMode = MODE_FAST;  // Variabile statica di tipo `DisplayMode` per memorizzare la modalità di visualizzazione precedente, inizializzata a `MODE_FAST`.

uint8_t currentPreset = 0;   // Variabile globale di tipo uint8_t per memorizzare il preset di configurazione corrente (probabilmente un indice).
uint8_t currentHour = 0;     // Variabile globale di tipo uint8_t per memorizzare l'ora corrente (in formato 24 ore).
uint8_t currentMinute = 0;   // Variabile globale di tipo uint8_t per memorizzare il minuto corrente.
uint8_t currentSecond = 0;   // Variabile globale di tipo uint8_t per memorizzare il secondo corrente.
uint8_t lastHour = 255;      // Variabile globale di tipo uint8_t per memorizzare l'ora precedente (inizializzata a un valore non valido per forzare l'aggiornamento all'inizio).
uint8_t lastMinute = 255;    // Variabile globale di tipo uint8_t per memorizzare il minuto precedente (inizializzata a un valore non valido).
uint8_t lastSecond = 255;    // Variabile globale di tipo uint8_t per memorizzare il secondo precedente (inizializzata a un valore non valido).
uint8_t lastAudioAnnounceHour = 255; // Valore 255 assicura che il primo trigger funzioni
uint8_t alexaOff = 0;        // Variabile globale di tipo uint8_t, probabilmente un flag per indicare se l'output del display è stato temporaneamente disattivato tramite Alexa (0=attivo, 1=disattivo).
bool alexaUpdatePending = false; // Flag per indicare che è necessario un aggiornamento da Alexa (gestione non bloccante).
uint32_t lastTouchTime = 0; // Variabile globale di tipo uint32_t per memorizzare il timestamp dell'ultimo tocco rilevato (in millisecondi).

// Variabili per configurazione orari giorno/notte (usati per luminosità e volume)
uint8_t dayStartHour = 8;    // Ora inizio giorno (default 8:00) - luminosità/volume alto
uint8_t dayStartMinute = 0;  // Minuti inizio giorno (default 0)
uint8_t nightStartHour = 22; // Ora inizio notte (default 22:00) - luminosità/volume basso
uint8_t nightStartMinute = 0; // Minuti inizio notte (default 0)

// Funzione helper per verificare se è notte (considera ore E minuti)
// Restituisce true se l'ora corrente è nel periodo notturno
bool checkIsNightTime(uint8_t hour, uint8_t minute) {
  // Converti tutto in minuti dall'inizio del giorno per confronto più facile
  uint16_t currentTimeMinutes = hour * 60 + minute;
  uint16_t dayStartMinutes = dayStartHour * 60 + dayStartMinute;
  uint16_t nightStartMinutes = nightStartHour * 60 + nightStartMinute;

  // Caso normale: giorno inizia prima della notte (es. giorno 8:00, notte 22:00)
  if (dayStartMinutes < nightStartMinutes) {
    // È notte se prima dell'inizio giorno O dopo l'inizio notte
    return (currentTimeMinutes < dayStartMinutes || currentTimeMinutes >= nightStartMinutes);
  } else {
    // Caso inverso: notte inizia prima del giorno (es. notte 2:00, giorno 8:00)
    // È notte se tra inizio notte e inizio giorno
    return (currentTimeMinutes >= nightStartMinutes && currentTimeMinutes < dayStartMinutes);
  }
}

// Variabili per luminosità giorno/notte (configurabili da web)
uint8_t brightnessDay = BRIGHTNESS_DAY_DEFAULT;     // Luminosità giorno (0-255)
uint8_t brightnessNight = BRIGHTNESS_NIGHT_DEFAULT; // Luminosità notte (0-255)
bool hourlyAnnounceEnabled = true;                  // Annuncio orario ogni ora abilitato
bool useTTSAnnounce = false;                        // Usa Google TTS invece di MP3 locali (default: MP3)
bool ttsVoiceFemale = true;                         // Voce TTS femminile (true) o maschile (false)
uint8_t audioVolume = 80;                           // Volume audio ESP32C3 (0-100, default 80%)
uint8_t announceVolume = 70;                        // Volume annunci orari (0-100, default 70)

// Variabili per audio giorno/notte (configurabili da web)
bool audioDayEnabled = true;                        // Audio abilitato durante il giorno
bool audioNightEnabled = false;                     // Audio abilitato durante la notte (default off)
uint8_t volumeDay = 80;                             // Volume audio giorno (0-100, default 80%)
uint8_t volumeNight = 30;                           // Volume audio notte (0-100, default 30%)
bool lastWasNightTime = false;                      // Ultimo stato giorno/notte per cambio volume automatico
uint8_t lastAppliedVolume = 80;                     // Ultimo volume applicato

// Variabili per cambio modalità random automatico
bool randomModeEnabled = false;                     // Cambio modalità random abilitato
uint8_t randomModeInterval = 5;                     // Intervallo cambio in minuti (default 5)
uint32_t lastRandomModeChange = 0;                  // Timestamp ultimo cambio modalità random

// Variabili per l'effetto "Snake"
Snake snake;                     // Variabile globale di tipo `Snake` (la struttura definita in precedenza) per gestire lo stato e il comportamento del serpente.
bool snakeInitNeeded = true;     // Variabile globale di tipo bool per indicare se è necessaria l'inizializzazione della struttura `snake` (ad esempio, all'avvio o al cambio modalità).

// Variabili per l'effetto "Water Drop"
WaterDrop waterDrop;             // Variabile globale di tipo `WaterDrop` per gestire lo stato e il comportamento della goccia d'acqua.
bool waterDropInitNeeded = true; // Variabile globale di tipo bool per indicare se è necessaria l'inizializzazione della struttura `waterDrop`.

// Variabili per l'effetto "Mario"
MarioSprite mario;               // Variabile globale di tipo `MarioSprite` per gestire lo stato e il comportamento di Mario.
bool marioInitNeeded = true;     // Variabile globale di tipo bool per indicare se è necessaria l'inizializzazione della struttura `mario`.

// Variabili per l'effetto "Tron"
TronBike tronBikes[NUM_TRON_BIKES];  // Array di moto Tron che si muovono sullo schermo.
uint8_t tronGrid[MATRIX_HEIGHT][MATRIX_WIDTH];  // Griglia per tracciare le posizioni occupate dalle scie.
bool tronInitialized = false;    // Variabile per indicare se l'effetto Tron è stato inizializzato.

#ifdef EFFECT_GALAGA
// Variabili per l'effetto "Galaga"
#define MAX_GALAGA_BULLETS 20    // Numero massimo di proiettili simultanei.
#define MAX_GALAGA_STARS 50      // Numero di stelle sullo sfondo.
GalagaShip galagaShip;           // Variabile globale per l'astronave Galaga.
GalagaBullet galagaBullets[MAX_GALAGA_BULLETS];  // Array di proiettili.
GalagaStar galagaStars[MAX_GALAGA_STARS];        // Array di stelle sullo sfondo.
bool galagaInitNeeded = true;    // Variabile globale per indicare se è necessaria l'inizializzazione.
#endif

#ifdef EFFECT_GALAGA2
// Variabili per l'effetto "Galaga2" (astronave volante stile emulatore HTML)
#define MAX_GALAGA2_STARS 50     // Numero di stelle sullo sfondo.
Galaga2Ship galaga2Ship;         // Variabile globale per l'astronave volante Galaga2.
GalagaStar galaga2Stars[MAX_GALAGA2_STARS];      // Array di stelle sullo sfondo (riutilizza struct).
bool galaga2InitNeeded = true;   // Variabile globale per indicare se è necessaria l'inizializzazione.
uint16_t galaga2RainbowHue = 0;  // Contatore per colore rainbow progressivo.
#endif

// ================== VARIABILI GLOBALI RADAR LD2410 ==================
// Radar LD2410 per rilevamento presenza e controllo luminosità (pin 1 e 2 UART)
// COMPATIBILE con audio WiFi esterno (ESP32C3) - nessun conflitto pin
HardwareSerial radarSerial(1);           // Usa Serial1 (UART1) per il radar LD2410
MyLD2410 radar(radarSerial);             // Oggetto MyLD2410 per comunicare con il radar
bool radarAvailable = false;             // Flag per indicare se il radar è disponibile
bool radarConnectedOnce = false;         // Flag per tracciare prima connessione riuscita
bool radarBrightnessControl = true;      // Flag per abilitare/disabilitare controllo luminosità con radar (SEMPRE ATTIVO)
uint8_t radarBrightnessMin = 90;         // Luminosità minima radar (buio) - default 90
uint8_t radarBrightnessMax = 255;        // Luminosità massima radar (luce forte) - default 255
bool presenceDetected = false;           // Flag per indicare se è stata rilevata presenza
uint8_t lastRadarLightLevel = 0;         // Ultimo valore luce letto dal radar (per web)
uint8_t lastAppliedBrightness = 255;     // Ultima luminosità applicata (per web)
uint32_t lastPresenceTime = 0;           // Timestamp dell'ultima presenza rilevata
uint32_t lastRadarRead = 0;              // Timestamp dell'ultima lettura del radar
const uint32_t RADAR_READ_INTERVAL = 200; // Intervallo lettura radar in ms (200ms = 5Hz per risposta rapida al rilevamento presenza)
const uint32_t PRESENCE_TIMEOUT = 30000;  // Timeout presenza in ms (30 secondi)


// Controllo luminosità basato su PRESENZA + ORA
// brightnessDay e brightnessNight sono già definite sopra (#define)
// Senza presenza: sempre 0 (display spento)
// Con presenza:
//   - Modalità automatica (radar): luminosità basata su sensore luce radar (getLightLevel)
//   - Modalità manuale: luminosità basata sull'ora (giorno/notte)

// ================== VARIABILI GLOBALI SENSORE BME280 ==================
// Sensore BME280 per temperatura, umidità e pressione interna
Adafruit_BME280 bme;             // Oggetto sensore BME280
bool bme280Available = false;    // Flag per indicare se il sensore è disponibile
float temperatureIndoor = 0.0;   // Temperatura interna dal BME280 (°C)
float humidityIndoor = 0.0;      // Umidità interna dal BME280 (%)
float pressureIndoor = 0.0;      // Pressione atmosferica dal BME280 (hPa)
// Offset di calibrazione BME280 (configurabili da web)
float bme280TempOffset = 0.0;    // Offset temperatura in °C (es: -2.5 per sottrarre 2.5 gradi)
float bme280HumOffset = 0.0;     // Offset umidità in % (es: +5.0 per aggiungere 5%)
// Fonte dati sensore ambiente: 0=nessuno, 1=BME280 interno, 2=radar remoto
uint8_t indoorSensorSource = 0;  // Indica da dove provengono i dati temperatura/umidità interna

// ================== VARIABILI GLOBALI METEO OUTDOOR (OpenWeatherMap) ==================
float temperatureOutdoor = 0.0;  // Temperatura esterna da OpenWeatherMap (°C)
float humidityOutdoor = 0.0;     // Umidità esterna da OpenWeatherMap (%)
bool outdoorTempAvailable = false;     // Flag se dati meteo outdoor disponibili
bool weatherIconAvailable = false;     // Flag se icona meteo disponibile
bool windDirectionAvailable = false;   // Flag se direzione vento disponibile
String weatherIconCode = "";           // Codice icona meteo (es. "01d", "10n")
float windDirection = 0.0;             // Direzione vento in gradi (0-360)
float windSpeed = 0.0;                 // Velocità vento in m/s
uint32_t lastWeatherUpdate = 0;        // Timestamp ultimo aggiornamento meteo
#define WEATHER_UPDATE_INTERVAL 600000 // Aggiorna ogni 10 minuti (600000 ms)

// ================== VARIABILI GLOBALI AUDIO WIFI (ESP32C3) ==================
// Audio gestito da ESP32C3 esterno via WiFi/UDP - nessun conflitto pin con radar
bool audioSlaveConnected = false;          // Flag per indicare se ESP32C3 audio è connesso
// localAudioActive rimosso - MP3 player ora usa stessa libreria Audio.h
unsigned long lastWiFiPing = 0;            // Timestamp ultimo ping WiFi a ESP32C3
#define WIFI_PING_INTERVAL 30000           // Intervallo ping WiFi in ms (30 secondi)
bool isAnnouncing = false;                 // Flag per indicare se è in corso un annuncio audio
unsigned long announceStartTime = 0;       // Timestamp inizio annuncio audio
unsigned long announceEndTime = 0;         // Timestamp fine annuncio (per bloccare suoni flip)
IPAddress audioServerIP;                   // IP dell'ESP32C3 sulla rete (usato da 7_AUDIO_WIFI.ino)

// ================== VARIABILI GLOBALI ALLARME BTTF ==================
bool bttfAlarmRinging = false;             // Flag per indicare se l'allarme BTTF è attivo
unsigned long bttfAlarmLastBeep = 0;       // Timestamp ultimo beep dell'allarme
unsigned long bttfAlarmStartTime = 0;      // Timestamp inizio allarme per timeout
#define BTTF_ALARM_BEEP_INTERVAL 1000      // Intervallo tra i beep dell'allarme (ms)
#define BTTF_ALARM_TIMEOUT 300000          // Timeout allarme: 5 minuti (300000 ms)

// Dichiarazioni extern per variabili definite in 12_BTTF.ino
#ifdef EFFECT_BTTF
extern bool alarmDestinationEnabled;       // Sveglia Destination Time abilitata
extern bool alarmLastDepartedEnabled;      // Sveglia Last Time Departed abilitata
#else
// Se EFFECT_BTTF non è definito, crea variabili dummy
bool alarmDestinationEnabled = false;
bool alarmLastDepartedEnabled = false;
#endif

// Comandi UDP voice assistant rimossi - non più necessari

#ifdef EFFECT_ANALOG_CLOCK
// Variabili per l'orologio analogico
JPEGDEC jpeg;                    // Oggetto per decodificare immagini JPEG.
bool analogClockInitNeeded = true; // Flag per inizializzazione.
bool clockImageLoaded = false;   // Flag per indicare se l'immagine di sfondo è stata caricata.
bool showClockDate = true;       // Flag per mostrare/nascondere la data (salvato in EEPROM).
float lastSecondAngle = 0;       // Angolo precedente della lancetta dei secondi.
float lastMinuteAngle = 0;       // Angolo precedente della lancetta dei minuti.
float lastHourAngle = 0;         // Angolo precedente della lancetta delle ore.
uint16_t* clockBackgroundBuffer = nullptr; // Buffer in PSRAM per salvare l'immagine di sfondo.

// Double buffering per orologio analogico
uint16_t* analogClockFrameBuffer = nullptr;  // Frame buffer per double buffering
OffscreenGFX* analogClockOffscreenGfx = nullptr;  // GFX offscreen

// Double buffering per flip clock
uint16_t* flipClockFrameBuffer = nullptr;  // Frame buffer per double buffering
OffscreenGFX* flipClockOffscreenGfx = nullptr;  // GFX offscreen

// Forward declarations per variabili definite in 8_CLOCK.ino (necessarie per cleanupPreviousMode)
#ifdef EFFECT_CLOCK
extern uint16_t* clockFaceBuffer;
extern uint16_t* clockSkinFrameBuffer;
extern OffscreenGFX* clockSkinOffscreenGfx;
#endif

// Variabili per monitoraggio SD Card
bool sdCardPresent = false;          // Stato corrente della SD Card
uint32_t lastSDCheckTime = 0;        // Ultimo controllo SD Card
const uint32_t SD_CHECK_INTERVAL = 500; // Controlla ogni 500ms (più reattivo)
uint32_t sdReinitTime = 0;           // Timestamp dell'ultima reinizializzazione
const uint32_t SD_REINIT_DELAY = 2000; // Attendi 2 secondi dopo reinizializzazione

// Colori configurabili per le lancette (RGB565)
uint16_t clockHourHandColor = BLACK;    // Colore lancetta ore (default: nero).
uint16_t clockMinuteHandColor = BLACK;  // Colore lancetta minuti (default: nero).
uint16_t clockSecondHandColor = RED;    // Colore lancetta secondi (default: rosso).

// Lunghezze lancette in pixel (salvati in file .cfg per ogni skin)
uint8_t clockHourHandLength = 80;       // Lunghezza lancetta ore (default: 80px)
uint8_t clockMinuteHandLength = 110;    // Lunghezza lancetta minuti (default: 110px)
uint8_t clockSecondHandLength = 120;    // Lunghezza lancetta secondi (default: 120px)

// Larghezze (spessore) lancette in pixel
uint8_t clockHourHandWidth = 6;         // Spessore lancetta ore (default: 6px)
uint8_t clockMinuteHandWidth = 4;       // Spessore lancetta minuti (default: 4px)
uint8_t clockSecondHandWidth = 2;       // Spessore lancetta secondi (default: 2px)

// Stili terminazione lancette (0=round, 1=square, 2=butt)
uint8_t clockHourHandStyle = 0;         // Stile lancetta ore (default: round)
uint8_t clockMinuteHandStyle = 0;       // Stile lancetta minuti (default: round)
uint8_t clockSecondHandStyle = 0;       // Stile lancetta secondi (default: round)

// Effetto smooth per lancetta dei secondi
bool clockSmoothSeconds = false;        // Se true, lancetta secondi scivola fluidamente invece di scattare

// Effetto Rainbow per le lancette dell'orologio analogico
bool clockHandsRainbow = false;         // Se true, le lancette cambiano colore con effetto rainbow

// Campi data configurabili indipendentemente per ogni skin (salvati in file .cfg)
// (struct DateField definita sopra, prima delle dichiarazioni forward)
DateField clockWeekdayField = {true, 190, 280, WHITE, 2};   // Giorno settimana (es: "Lun"), fontSize=2
DateField clockDayField = {true, 210, 320, WHITE, 3};        // Giorno numero (es: "25"), fontSize=3 (più grande)
DateField clockMonthField = {true, 200, 360, WHITE, 2};      // Mese (es: "Dic"), fontSize=2
DateField clockYearField = {true, 195, 400, WHITE, 2};       // Anno (es: "2025"), fontSize=2

#define CLOCK_CENTER_X 240       // Centro X dell'orologio (centro display 480x480).
#define CLOCK_CENTER_Y 240       // Centro Y dell'orologio.
#define HOUR_HAND_LENGTH 100     // Lunghezza lancetta ore.
#define MINUTE_HAND_LENGTH 140   // Lunghezza lancetta minuti.
#define SECOND_HAND_LENGTH 160   // Lunghezza lancetta secondi.
#define CLOCK_BUFFER_SIZE (480 * 480) // Dimensione buffer (480x480 pixel in RGB565).

// Indirizzi EEPROM per configurazione orologio (SPOSTATI a 520-559 per evitare conflitti)
#define EEPROM_CLOCK_HOUR_COLOR_ADDR   520  // 2 bytes per colore RGB565 lancetta ore
#define EEPROM_CLOCK_MINUTE_COLOR_ADDR 522  // 2 bytes per colore RGB565 lancetta minuti
#define EEPROM_CLOCK_SECOND_COLOR_ADDR 524  // 2 bytes per colore RGB565 lancetta secondi
#define EEPROM_CLOCK_DATE_Y_ADDR       526  // 2 bytes per posizione Y data
#define EEPROM_CLOCK_SKIN_NAME_ADDR    528  // 32 bytes per nome file skin attiva
#define EEPROM_CLOCK_SMOOTH_SECONDS_ADDR 560  // 1 byte per smooth seconds (on/off)
#define EEPROM_CLOCK_RAINBOW_ADDR 561         // 1 byte per rainbow hands (on/off)
#define EEPROM_CLOCK_SKIN_ADDR 562            // 1 byte per indice skin EFFECT_CLOCK (0-255)

// Nome file skin attiva (caricato da EEPROM)
char clockActiveSkin[32] = "orologio.jpg";  // Default
#endif

#ifdef EFFECT_BTTF
// ================== INDIRIZZI EEPROM PER BTTF ==================
// Indirizzi per date e coordinate BTTF (SPOSTATI a 600-700 per evitare conflitti)
#define EEPROM_BTTF_DEST_MONTH_ADDR   600  // 1 byte - mese destinazione (1-12)
#define EEPROM_BTTF_DEST_DAY_ADDR     601  // 1 byte - giorno destinazione (1-31)
#define EEPROM_BTTF_DEST_YEAR_ADDR    602  // 2 bytes - anno destinazione
#define EEPROM_BTTF_DEST_HOUR_ADDR    604  // 1 byte - ora destinazione (1-12)
#define EEPROM_BTTF_DEST_MINUTE_ADDR  605  // 1 byte - minuti destinazione (0-59)
#define EEPROM_BTTF_DEST_AMPM_ADDR    606  // 1 byte - AM/PM destinazione (0=AM, 1=PM)

#define EEPROM_BTTF_LAST_MONTH_ADDR   607  // 1 byte - mese partenza (1-12)
#define EEPROM_BTTF_LAST_DAY_ADDR     608  // 1 byte - giorno partenza (1-31)
#define EEPROM_BTTF_LAST_YEAR_ADDR    609  // 2 bytes - anno partenza
#define EEPROM_BTTF_LAST_HOUR_ADDR    611  // 1 byte - ora partenza (1-12)
#define EEPROM_BTTF_LAST_MINUTE_ADDR  612  // 1 byte - minuti partenza (0-59)
#define EEPROM_BTTF_LAST_AMPM_ADDR    613  // 1 byte - AM/PM partenza (0=AM, 1=PM)

// Indirizzi per font sizes BTTF
#define EEPROM_BTTF_MONTH_FONT_SIZE_ADDR   614  // 1 byte - dimensione font mesi (0-5)
#define EEPROM_BTTF_NUMBER_FONT_SIZE_ADDR  615  // 1 byte - dimensione font numeri (0-5)

// Indirizzi per coordinate individuali BTTF (ogni pannello ha coordinate proprie per ogni campo)
// PANEL 1 (DESTINATION TIME) - 24 bytes
#define EEPROM_BTTF_P1_MONTH_X_ADDR   616  // 2 bytes
#define EEPROM_BTTF_P1_MONTH_Y_ADDR   618  // 2 bytes
#define EEPROM_BTTF_P1_DAY_X_ADDR     620  // 2 bytes
#define EEPROM_BTTF_P1_DAY_Y_ADDR     622  // 2 bytes
#define EEPROM_BTTF_P1_YEAR_X_ADDR    624  // 2 bytes
#define EEPROM_BTTF_P1_YEAR_Y_ADDR    626  // 2 bytes
#define EEPROM_BTTF_P1_AMPM_X_ADDR    628  // 2 bytes
#define EEPROM_BTTF_P1_AMPM_Y_ADDR    630  // 2 bytes
#define EEPROM_BTTF_P1_HOUR_X_ADDR    632  // 2 bytes
#define EEPROM_BTTF_P1_HOUR_Y_ADDR    634  // 2 bytes
#define EEPROM_BTTF_P1_MIN_X_ADDR     636  // 2 bytes
#define EEPROM_BTTF_P1_MIN_Y_ADDR     638  // 2 bytes

// PANEL 2 (PRESENT TIME) - 24 bytes
#define EEPROM_BTTF_P2_MONTH_X_ADDR   640  // 2 bytes
#define EEPROM_BTTF_P2_MONTH_Y_ADDR   642  // 2 bytes
#define EEPROM_BTTF_P2_DAY_X_ADDR     644  // 2 bytes
#define EEPROM_BTTF_P2_DAY_Y_ADDR     646  // 2 bytes
#define EEPROM_BTTF_P2_YEAR_X_ADDR    648  // 2 bytes
#define EEPROM_BTTF_P2_YEAR_Y_ADDR    650  // 2 bytes
#define EEPROM_BTTF_P2_AMPM_X_ADDR    652  // 2 bytes
#define EEPROM_BTTF_P2_AMPM_Y_ADDR    654  // 2 bytes
#define EEPROM_BTTF_P2_HOUR_X_ADDR    656  // 2 bytes
#define EEPROM_BTTF_P2_HOUR_Y_ADDR    658  // 2 bytes
#define EEPROM_BTTF_P2_MIN_X_ADDR     660  // 2 bytes
#define EEPROM_BTTF_P2_MIN_Y_ADDR     662  // 2 bytes

// PANEL 3 (LAST TIME DEPARTED) - 24 bytes
#define EEPROM_BTTF_P3_MONTH_X_ADDR   664  // 2 bytes
#define EEPROM_BTTF_P3_MONTH_Y_ADDR   666  // 2 bytes
#define EEPROM_BTTF_P3_DAY_X_ADDR     668  // 2 bytes
#define EEPROM_BTTF_P3_DAY_Y_ADDR     670  // 2 bytes
#define EEPROM_BTTF_P3_YEAR_X_ADDR    672  // 2 bytes
#define EEPROM_BTTF_P3_YEAR_Y_ADDR    674  // 2 bytes
#define EEPROM_BTTF_P3_AMPM_X_ADDR    676  // 2 bytes
#define EEPROM_BTTF_P3_AMPM_Y_ADDR    678  // 2 bytes
#define EEPROM_BTTF_P3_HOUR_X_ADDR    680  // 2 bytes
#define EEPROM_BTTF_P3_HOUR_Y_ADDR    682  // 2 bytes
#define EEPROM_BTTF_P3_MIN_X_ADDR     684  // 2 bytes
#define EEPROM_BTTF_P3_MIN_Y_ADDR     686  // 2 bytes

// Totale BTTF: 88 bytes (600-687)
// - Date: 14 bytes (600-613)
// - Font sizes: 2 bytes (614-615)
// - Coordinate individuali: 72 bytes (616-687)
#endif

#ifdef EFFECT_FLUX_CAPACITOR
// Indirizzi EEPROM per Flux Capacitor (270-280)
#define EEPROM_FLUX_COLOR_R_ADDR      270  // 1 byte - componente rosso
#define EEPROM_FLUX_COLOR_G_ADDR      271  // 1 byte - componente verde
#define EEPROM_FLUX_COLOR_B_ADDR      272  // 1 byte - componente blu
#define EEPROM_FLUX_PARTICLES_ADDR    273  // 1 byte - numero particelle
#define EEPROM_FLUX_ANIM_SPEED_ADDR   274  // 1 byte - velocità animazione (ms)
#define EEPROM_FLUX_PART_SPEED_ADDR   275  // 1 byte - velocità particelle (x100)
#define EEPROM_FLUX_VALID_ADDR        276  // 1 byte - marker validità (0xFC)
#define EEPROM_FLUX_VALID_MARKER      0xFC // Marker per indicare che le impostazioni sono valide
#endif

#ifdef EFFECT_LED_RING
// Indirizzi EEPROM per LED Ring (280-330)
#define EEPROM_LEDRING_HOURS_R_ADDR      280  // 1 byte - Red ore
#define EEPROM_LEDRING_HOURS_G_ADDR      281  // 1 byte - Green ore
#define EEPROM_LEDRING_HOURS_B_ADDR      282  // 1 byte - Blue ore
#define EEPROM_LEDRING_MINUTES_R_ADDR    283  // 1 byte - Red minuti
#define EEPROM_LEDRING_MINUTES_G_ADDR    284  // 1 byte - Green minuti
#define EEPROM_LEDRING_MINUTES_B_ADDR    285  // 1 byte - Blue minuti
#define EEPROM_LEDRING_SECONDS_R_ADDR    286  // 1 byte - Red secondi
#define EEPROM_LEDRING_SECONDS_G_ADDR    287  // 1 byte - Green secondi
#define EEPROM_LEDRING_SECONDS_B_ADDR    288  // 1 byte - Blue secondi
#define EEPROM_LEDRING_DIGITAL_R_ADDR    289  // 1 byte - Red display digitale
#define EEPROM_LEDRING_DIGITAL_G_ADDR    290  // 1 byte - Green display digitale
#define EEPROM_LEDRING_DIGITAL_B_ADDR    291  // 1 byte - Blue display digitale
#define EEPROM_LEDRING_HOURS_RAINBOW     292  // 1 byte - Rainbow ore (0/1)
#define EEPROM_LEDRING_MINUTES_RAINBOW   293  // 1 byte - Rainbow minuti (0/1)
#define EEPROM_LEDRING_SECONDS_RAINBOW   294  // 1 byte - Rainbow secondi (0/1)
#define EEPROM_LEDRING_VALID_ADDR        295  // 1 byte - marker validità (0xLR)
#define EEPROM_LEDRING_VALID_MARKER      0xAB // Marker per indicare che le impostazioni LED Ring sono valide
#endif

#ifdef EFFECT_FLIP_CLOCK
// Variabile per l'orologio flip clock
bool flipClockInitialized = false; // Variabile globale per indicare se il flip clock è stato inizializzato.

// Modalità test orario (per debug flip clock)
bool testModeEnabled = false;   // Flag per abilitare/disabilitare la modalità test orario
uint8_t testHour = 12;          // Ora base impostata manualmente per la modalità test (0-23)
uint8_t testMinute = 0;         // Minuti base impostati manualmente per la modalità test (0-59)
uint8_t testSecond = 0;         // Secondi base impostati manualmente per la modalità test (0-59)
unsigned long testModeStartTime = 0; // Tempo di inizio test mode
#endif

#ifdef EFFECT_WEATHER_STATION
// Variabile per la stazione meteo
bool weatherStationInitialized = false; // Variabile globale per indicare se la stazione meteo è stata inizializzata.
#endif

#ifdef EFFECT_CALENDAR
// Variabile per il calendario
bool calendarStationInitialized = false; // Variabile globale per indicare se il calendario è stato inizializzato.
#endif

#ifdef EFFECT_BTTF
// Variabile per la modalità Ritorno al Futuro
bool bttfInitialized = false; // Variabile globale per indicare se la modalità BTTF è stata inizializzata.
#endif

#ifdef EFFECT_FLUX_CAPACITOR
// Variabile per la modalità Flusso Canalizzatore
bool fluxCapacitorInitialized = false; // Variabile globale per indicare se la modalità Flux Capacitor è stata inizializzata.
#endif

#ifdef EFFECT_LED_RING
// Variabili per la modalità LED Ring
// Colori configurabili per ogni elemento dell'orologio LED Ring
uint8_t ledRingHoursR = 255, ledRingHoursG = 100, ledRingHoursB = 0;       // Colore ore (arancione default)
uint8_t ledRingMinutesR = 0, ledRingMinutesG = 150, ledRingMinutesB = 255;  // Colore minuti (blu default)
uint8_t ledRingSecondsR = 0, ledRingSecondsG = 255, ledRingSecondsB = 255;  // Colore secondi (ciano default)
uint8_t ledRingDigitalR = 0, ledRingDigitalG = 255, ledRingDigitalB = 200;  // Colore display digitale (verde-ciano)
// Flag per modalità Rainbow su ogni elemento
bool ledRingHoursRainbow = false;    // Rainbow per cerchio ore
bool ledRingMinutesRainbow = false;  // Rainbow per cerchio minuti
bool ledRingSecondsRainbow = false;  // Rainbow per cerchio secondi
// Hue rainbow per ogni elemento (per effetto indipendente)
uint16_t ledRingHoursHue = 0;
uint16_t ledRingMinutesHue = 120;
uint16_t ledRingSecondsHue = 240;
#endif

// Variabili per l'effetto "Matrix"
Drop drops[NUM_DROPS];       // Array globale di strutture `Drop` per memorizzare lo stato di ciascuna "goccia" nell'effetto matrice.
bool matrixInitialized = false; // Variabile globale di tipo bool per indicare se l'effetto matrice è stato inizializzato (ad esempio, con la posizione iniziale delle gocce).

// Variabili per la modalità "Slow" (definizione specifica dell'effetto non chiara qui)
bool slowInitialized = false; // Variabile globale di tipo bool per indicare se la modalità "Slow" è stata inizializzata.

// Flag per forzare ridisegno display da webserver (senza usare delay)
volatile bool webForceDisplayUpdate = false;
bool fadeDone = false;        // Variabile globale di tipo bool, probabilmente usata nella modalità "Slow" per indicare se una fase di dissolvenza è completata.
uint32_t fadeStartTime = 0;   // Variabile globale di tipo uint32_t per memorizzare il timestamp di inizio di una dissolvenza nella modalità "Slow".
const uint16_t fadeDuration = 3000; // Costante di tipo uint16_t per definire la durata di una dissolvenza in millisecondi (3 secondi).

// Variabili per Gemini AI
#ifdef EFFECT_GEMINI_AI
bool geminiInitialized = false; // Variabile globale per indicare se la modalità Gemini AI è stata inizializzata.
#endif

// Variabili per la modalità "Fade" (dissolvenza delle parole dell'ora)
enum FadePhase {
  // Fasi ITALIANO
  FADE_SONO_LE,        // Fase di dissolvenza per la parola "SONO LE".
  FADE_ORA,            // Fase di dissolvenza per la parola dell'ora.
  FADE_E,              // Fase di dissolvenza per la parola "E".
  FADE_MINUTI_NUMERO,  // Fase di dissolvenza per il numero dei minuti.
  FADE_MINUTI_PAROLA,  // Fase di dissolvenza per la parola "MINUTI".
  // Fasi INGLESE
  FADE_EN_IT_IS,       // "IT IS"
  FADE_EN_MINUTE_PREFIX, // FIVE/TEN/QUARTER/TWENTY/TWENTYFIVE/HALF
  FADE_EN_PAST_TO,     // PAST o TO
  FADE_EN_HOUR,        // Ora (ONE, TWO, THREE, ecc.)
  FADE_EN_OCLOCK,      // O'CLOCK (solo per :00)
  FADE_DONE            // Fase che indica che la sequenza di dissolvenza è completata.
};

FadePhase fadePhase = FADE_SONO_LE; // Variabile per memorizzare la fase attuale della dissolvenza, inizializzata a `FADE_SONO_LE`.
bool fadeInitialized = false;    // Variabile per indicare se la sequenza di dissolvenza è stata inizializzata.
uint8_t fadeStep = 0;           // Variabile per tenere traccia del passo corrente nell'animazione di dissolvenza.

#ifdef ENABLE_MULTILANGUAGE
// Variabili per il display inglese
EnglishTimeDisplay currentEnglishDisplay; // Memorizza lo stato corrente del display inglese
#endif
uint32_t lastFadeUpdate = 0;     // Variabile per memorizzare il timestamp dell'ultimo aggiornamento dell'animazione di dissolvenza.
static const uint8_t fadeSteps = 20;    // Costante statica di tipo uint8_t per definire il numero totale di passi nell'animazione di dissolvenza.
static const uint16_t wordFadeDuration = 500; // Costante statica di tipo uint16_t per definire la durata in millisecondi della dissolvenza di una singola parola.

// Buffer per il display e gestione dei pixel
uint16_t displayBuffer[NUM_LEDS]; // Array di tipo uint16_t per memorizzare lo stato del colore di ciascun LED nella matrice virtuale (probabilmente in formato RGB565).
bool pixelChanged[NUM_LEDS] = {false}; // Array di tipo bool per indicare se il colore di un pixel è cambiato dall'ultimo aggiornamento parziale del display.
uint32_t lastPartialUpdate = 0;    // Variabile globale di tipo uint32_t per memorizzare il timestamp dell'ultimo aggiornamento parziale del display.

bool targetPixels[NUM_LEDS] = {false};   // Array di tipo bool per memorizzare quali LED devono essere accesi in un determinato momento (usato probabilmente per effetti semplici on/off).
bool targetPixels_1[NUM_LEDS] = {false}; // Array simile a `targetPixels`, probabilmente usato per un secondo layer di animazione o per memorizzare uno stato intermedio.
bool targetPixels_2[NUM_LEDS] = {false}; // Altro array simile, per un terzo layer o stato.
bool activePixels[NUM_LEDS] = {false};   // Array di tipo bool per memorizzare quali LED sono attualmente attivi (accesi).
RGB snakeTrailColors[NUM_LEDS];          // Array per memorizzare i colori delle lettere "mangiate" dal serpente.

// Opzioni e stati per la pagina di setup
SetupOptions setupOptions;                // Variabile globale di tipo `SetupOptions` per memorizzare le impostazioni di configurazione.
SetupMenuItem setupMenuItems[SETUP_ITEMS_COUNT]; // Array di strutture `SetupMenuItem` per definire gli elementi del menu di setup.
uint8_t setupCurrentScroll = 0;         // Variabile globale di tipo uint8_t per memorizzare la posizione corrente dello scroll nel menu di setup.
unsigned long setupPageTimeout = 60000;  // Variabile globale di tipo unsigned long per definire il timeout (in millisecondi) per uscire automaticamente dalla pagina di setup (60 secondi).
bool exitingSetupPage = false;          // Variabile globale di tipo bool per indicare se si sta per uscire dalla pagina di setup.
bool setupPageActive = false;           // Variabile globale di tipo bool per indicare se la pagina di setup è attualmente attiva e visualizzata.
bool setupScrollActive = false;         // Variabile globale di tipo bool per indicare se lo scroll nel menu di setup è attivo (l'utente sta scorrendo).
bool checkingSetupScroll = false;       // Variabile globale di tipo bool per indicare se si sta verificando un movimento per determinare se è uno scroll.
uint32_t setupLastActivity = 0;         // Variabile globale di tipo uint32_t per memorizzare il timestamp dell'ultima interazione dell'utente con la pagina di setup.
int16_t colorTextX = 0;                // Variabile globale di tipo int16_t per memorizzare la coordinata X del testo relativo alla selezione del colore nella pagina di setup.
int16_t colorTextY = 240;               // Variabile globale di tipo int16_t per memorizzare la coordinata Y del testo relativo alla selezione del colore.
extern bool menuActive;                 // Dichiarazione di una variabile esterna (definita in un altro file) di tipo bool per indicare lo stato di attività del menu (potrebbe essere la stessa `setupPageActive`).

// Colore del testo dell'orologio
Color currentColor = Color(255, 255, 255);  // Variabile globale di tipo `Color` per memorizzare il colore attuale utilizzato per visualizzare l'orologio (inizializzato a bianco).
Color userColor = Color(255, 255, 255);     // Variabile globale di tipo `Color` per memorizzare il colore preferito dall'utente (inizializzato a bianco e potenzialmente caricato dalla EEPROM).
uint8_t BackColor = 40;                    // Variabile globale di tipo uint8_t per memorizzare un valore di "sfondo" per i colori (potrebbe essere un'intensità o un indice di colore).
Color TextBackColor = Color(40, 40, 40);    // Variabile globale di tipo `Color` per memorizzare il colore di sfondo del testo (inizializzato a un grigio scuro).
Color snake_E_color = Color(255, 255, 255); // Variabile globale di tipo `Color` per memorizzare il colore temporaneo per la visualizzazione e il lampeggio della lettera "E" nell'effetto "Snake".
Color hsvToRgb(uint8_t h, uint8_t s, uint8_t v); // Prototipo della funzione per convertire un colore dal modello HSV (Hue, Saturation, Value) al modello RGB (Red, Green, Blue).

// RGB leds[NUM_LEDS]; // Dichiarazione (commentata) di un array di strutture `RGB` per memorizzare il colore di ciascun LED (potrebbe essere stato sostituito da `displayBuffer`).
bool isFadeFirstEntry = true;      // Variabile globale di tipo bool per indicare se è la prima volta che si entra nella modalità "Fade" (potrebbe essere usata per un'inizializzazione specifica).
bool backgroundInitialized = false; // Variabile globale di tipo bool per indicare se lo sfondo del display è stato inizializzato.

// Strutture dati globali
TimeDisplay timeDisplay; // Variabile globale di tipo `TimeDisplay` per memorizzare i LED che rappresentano l'ora corrente.
SnakePath snakePath;     // Variabile globale di tipo `SnakePath` per memorizzare il percorso del serpente.

uint32_t debugTime = 0;            // Variabile globale di tipo uint32_t per scopi di debug (memorizzazione di timestamp).
static uint32_t menuDisplayTime = 0;   // Variabile statica di tipo uint32_t per memorizzare il timestamp dell'ultima visualizzazione del menu.
bool menuActive = false;        // Variabile globale di tipo bool per indicare lo stato di attività del menu (potrebbe essere la stessa `setupPageActive`).
bool resetCountdownStarted = false; // Variabile globale di tipo bool per indicare se è iniziato un conto alla rovescia per un reset.

// Touch e Ciclo colori
uint32_t touchStartTime = 0;          // Variabile globale di tipo uint32_t per memorizzare il timestamp dell'inizio di un tocco.

//===================================================================//
//                        GESTIONE COLORI                            //
//===================================================================//

// Funzione per aggiornare SOLO il colore rainbow (senza ridisegnare)
// Gli effetti useranno currentColor che viene aggiornato qui
void updateRainbowColor(uint32_t currentMillis) {
  const uint16_t interval = 100;  // Intervallo in ms tra cambi colore
  static uint32_t lastRainbowUpdate = 0;
  static uint16_t rainbowHue = 0;

  if (currentMillis - lastRainbowUpdate >= interval) {
    lastRainbowUpdate = currentMillis;

    // Incrementa hue per ciclo colori
    rainbowHue += 3;
    if (rainbowHue >= 360) {
      rainbowHue = 0;
    }

    // Calcola colore attuale
    uint8_t scaledHue = (rainbowHue % 360) * 255 / 360;
    Color c = hsvToRgb(scaledHue, 255, 255);  // Saturazione e luminosità al massimo
    currentColor = c;  // Aggiorna il colore globale che verrà usato dagli effetti
  }
}

// Funzione originale per il ciclo colori completo (con fasi bianco)
// Usata solo quando colorCycle.isActive è true (non da rainbowMode)
void updateColorCycling(uint32_t currentMillis) {
  const uint16_t interval = 100;
  static uint8_t whiteSteps = 0;
  static uint8_t rainbowLastHour = 255;
  static uint8_t rainbowLastMinute = 255;

  if (!colorCycle.isActive) return;

  // Rileva cambio orario e ridisegna lo sfondo se necessario
  if (rainbowLastHour != currentHour || rainbowLastMinute != currentMinute) {
    gfx->fillScreen(BLACK);
    gfx->setFont(u8g2_font_inb21_mr);
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      gfx->setTextColor(convertColor(TextBackColor));
      gfx->setCursor(pgm_read_word(&TFT_X[i]), pgm_read_word(&TFT_Y[i]));
      gfx->write(pgm_read_byte(&TFT_L[i]));
    }
    rainbowLastHour = currentHour;
    rainbowLastMinute = currentMinute;
  }

  if (currentMillis - colorCycle.lastColorChange >= interval) {
    colorCycle.lastColorChange = currentMillis;

    // Fase 1: Ciclo colore HSV
    if (!colorCycle.fadingToWhite && !colorCycle.showingWhite) {
      colorCycle.hue += 3;
      if (colorCycle.hue >= 360) {
        colorCycle.fadingToWhite = true;
      }
    }

    // Fase 2: Sfumatura verso bianco
    if (colorCycle.fadingToWhite) {
      if (colorCycle.saturation > 0) {
        colorCycle.saturation -= 5;
        if (colorCycle.saturation == 0) {
          colorCycle.fadingToWhite = false;
          colorCycle.showingWhite = true;
          whiteSteps = 0;
        }
      }
    }

    // Fase 3: Mostra bianco
    if (colorCycle.showingWhite) {
      whiteSteps++;
      if (whiteSteps >= 30) {
        colorCycle.showingWhite = false;
        colorCycle.saturation = 255;
        colorCycle.hue = 0;
      }
    }

    // Calcolo colore attuale
    uint8_t scaledHue = (colorCycle.hue % 360) * 255 / 360;
    Color c = hsvToRgb(scaledHue, colorCycle.saturation, 255);
    currentColor = c;

    // Aggiorna parole con il colore attuale
    if (currentHour == 0) {
      strncpy(&TFT_L[6], "MEZZANOTTE", 10);
      displayWord(WORD_MEZZANOTTE, currentColor);
    } else {
      strncpy(&TFT_L[6], "EYOREXZERO", 10);
      displayWord(WORD_SONO_LE, currentColor);
      const uint8_t* hourWord = (const uint8_t*)pgm_read_ptr(&HOUR_WORDS[currentHour]);
      displayWord(hourWord, currentColor);
    }

    if (currentMinute > 0) {
      displayWord(WORD_E, currentColor);
      showMinutes(currentMinute, currentColor);
      displayWord(WORD_MINUTI, currentColor);
    }
  }
}

void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, uint16_t &h, uint8_t &s, uint8_t &v) {
  float r_norm = r / 255.0f;
  float g_norm = g / 255.0f;
  float b_norm = b / 255.0f;

  float max_c = max(r_norm, max(g_norm, b_norm));
  float min_c = min(r_norm, min(g_norm, b_norm));
  float delta = max_c - min_c;

  // Valore massimo = luminosità (Value)
  v = round(max_c * 255.0f);

  // Saturazione (Saturation)
  if (max_c == 0.0f) {
    s = 0;
  } else {
    s = round((delta / max_c) * 255.0f);
  }

  // Tonalità (Hue)
  float hue_deg = 0.0f;
  if (delta > 0.0f) {
    if (max_c == r_norm) {
      hue_deg = 60.0f * fmodf((g_norm - b_norm) / delta, 6.0f);
    } else if (max_c == g_norm) {
      hue_deg = 60.0f * (((b_norm - r_norm) / delta) + 2.0f);
    } else { // max_c == b_norm
      hue_deg = 60.0f * (((r_norm - g_norm) / delta) + 4.0f);
    }

    if (hue_deg < 0.0f) {
      hue_deg += 360.0f;
    }
  }

  // Scala hue su 0–255 (compatibile con hsvToRgb)
  h = round(hue_deg * 255.0f / 360.0f);
}

// Funzione per convertire RGB a RGB565 (formato colore a 16 bit utilizzato da molti display)
uint16_t RGBtoRGB565(const RGB& color) {
  return ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3);
}

// Funzione per convertire Color a RGB565
uint16_t convertColor(const Color& color) {
  return ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3);
}

// Conversione da HSV a RGB
Color hsvToRgb(uint8_t h, uint8_t s, uint8_t v) {
  // Implementazione ottimizzata della conversione HSV a RGB
  uint8_t region, remainder, p, q, t;

  if (s == 0) {
    return Color(v, v, v); // Se la saturazione è 0, il colore è una scala di grigi (r=g=b=v).
  }

  region = h / 43;
  remainder = (h - (region * 43)) * 6;

  p = (v * (255 - s)) >> 8;
  q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
    case 0:
      return Color(v, t, p);
    case 1:
      return Color(q, v, p);
    case 2:
      return Color(p, v, t);
    case 3:
      return Color(p, q, v);
    case 4:
      return Color(t, p, v);
    default:
      return Color(v, p, q);
  }
}

// ================== FUNZIONI HELPER PER CARICARE JPG DA SD CARD ==================
// Usate da EFFECT_CLOCK e EFFECT_ANALOG_CLOCK per caricare skin JPEG
File jpegFile;
uint16_t* jpegTargetBuffer = nullptr;

// Callback per aprire file JPEG dalla SD card
void* jpegOpenFile(const char *szFilename, int32_t *pFileSize) {
  jpegFile = SD.open(szFilename, FILE_READ);
  if (jpegFile) {
    *pFileSize = jpegFile.size();
    return &jpegFile;
  }
  return NULL;
}

// Callback per chiudere file JPEG
void jpegCloseFile(void *pHandle) {
  File *f = static_cast<File*>(pHandle);
  if (f) {
    f->close();
  }
}

// Callback per leggere dal file JPEG
int32_t jpegReadFile(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  File *f = static_cast<File*>(pFile->fHandle);
  return f->read(pBuf, iLen);
}

// Callback per posizionarsi nel file JPEG
int32_t jpegSeekFile(JPEGFILE *pFile, int32_t iPosition) {
  File *f = static_cast<File*>(pFile->fHandle);
  f->seek(iPosition);
  return iPosition;
}

// Callback per disegnare i pixel decodificati del JPEG sul display
int jpegDrawCallback(JPEGDRAW *pDraw) {
  gfx->draw16bitRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1; // Continua il decode
}

// Callback per disegnare i pixel decodificati del JPEG nel buffer PSRAM
int jpegDrawToBufferCallback(JPEGDRAW *pDraw) {
  if (jpegTargetBuffer == nullptr) return 0; // Errore

  // Copia i pixel nel buffer PSRAM
  for (int16_t row = 0; row < pDraw->iHeight; row++) {
    uint16_t* dest = jpegTargetBuffer + ((pDraw->y + row) * 480) + pDraw->x;
    uint16_t* src = pDraw->pPixels + (row * pDraw->iWidth);
    memcpy(dest, src, pDraw->iWidth * 2);  // 2 bytes per pixel (RGB565)
  }
  return 1; // Continua il decode
}

// Funzione per caricare JPEG nel buffer PSRAM
bool loadJpegToBuffer(const char *filename, uint16_t* buffer, int maxWidth, int maxHeight) {
  if (buffer == nullptr) {
    Serial.println("Errore: buffer nullo!");
    return false;
  }

  jpegTargetBuffer = buffer;

  if (jpeg.open(filename, jpegOpenFile, jpegCloseFile, jpegReadFile, jpegSeekFile, jpegDrawToBufferCallback)) {
    Serial.printf("JPEG aperto: %dx%d\n", jpeg.getWidth(), jpeg.getHeight());

    int scale = 0;
    float ratio = (float)jpeg.getHeight() / maxHeight;

    if (ratio > 4) {
      scale = JPEG_SCALE_EIGHTH;
    } else if (ratio > 2) {
      scale = JPEG_SCALE_QUARTER;
    } else if (ratio > 1) {
      scale = JPEG_SCALE_HALF;
    }

    jpeg.decode(0, 0, scale);
    jpeg.close();
    jpegTargetBuffer = nullptr;

    Serial.println("JPEG caricato con successo nel buffer PSRAM!");
    return true;
  } else {
    Serial.printf("Errore apertura JPEG: %s\n", filename);
    jpegTargetBuffer = nullptr;
    return false;
  }
}

// Funzione per caricare e visualizzare un JPEG dalla SD card
bool drawJpegFromSD(const char *filename, int x, int y, int maxWidth, int maxHeight) {
  if (jpeg.open(filename, jpegOpenFile, jpegCloseFile, jpegReadFile, jpegSeekFile, jpegDrawCallback)) {
    Serial.printf("JPEG aperto: %dx%d\n", jpeg.getWidth(), jpeg.getHeight());

    int scale = 0;
    float ratio = (float)jpeg.getHeight() / maxHeight;

    if (ratio > 4) {
      scale = JPEG_SCALE_EIGHTH;
    } else if (ratio > 2) {
      scale = JPEG_SCALE_QUARTER;
    } else if (ratio > 1) {
      scale = JPEG_SCALE_HALF;
    }

    jpeg.decode(x, y, scale);
    jpeg.close();

    Serial.println("JPEG caricato con successo dalla SD!");
    return true;
  } else {
    Serial.printf("Errore apertura JPEG: %s\n", filename);
    return false;
  }
}

#ifdef EFFECT_ANALOG_CLOCK
void setup_clock_webserver(AsyncWebServer* server);
bool loadClockConfig(const char* skinName);
bool saveClockConfig(const char* skinName);
#endif

#ifdef EFFECT_BTTF
// Forward declarations BTTF (struct definiti in bttf_types.h)
void setup_bttf_webserver(AsyncWebServer* server);
void loadBTTFDatesFromEEPROM();
void saveBTTFDatesToEEPROM();
void loadBTTFCoordsFromEEPROM();
void saveBTTFCoordsToEEPROM();
#endif

#ifdef EFFECT_GEMINI_AI
// Forward declarations Gemini AI
void setup_gemini_webserver(AsyncWebServer* server);
void displayGeminiMode();  // Funzione per visualizzare modalità Gemini AI sul display
#endif

#ifdef EFFECT_MJPEG_STREAM
// Forward declarations MJPEG Stream
void setup_mjpeg_webserver(AsyncWebServer* server);
#endif

#ifdef EFFECT_ESP32CAM
// Forward declarations ESP32-CAM Stream
void setup_esp32cam_webserver(AsyncWebServer* server);
void updateEsp32camStream();
void cleanupEsp32camStream();
extern bool esp32camInitialized;
#endif

// Forward declarations Settings Webserver (sempre disponibile)
void setup_settings_webserver(AsyncWebServer* server);
DisplayMode getNextEnabledMode(DisplayMode current);
DisplayMode getPrevEnabledMode(DisplayMode current);
bool isModeEnabled(uint8_t mode);
extern uint32_t enabledModesMask;

#ifdef EFFECT_FLUX_CAPACITOR
// Forward declarations Flux Capacitor Webserver
void setup_flux_webserver(AsyncWebServer* server);
#endif

#ifdef EFFECT_LED_RING
// Forward declarations LED Ring Webserver
void setupLedRingWebServer(AsyncWebServer &server);
#endif

#ifdef EFFECT_MJPEG_STREAM
// Forward declarations Bluetooth Audio
void setup_bt_audio_webserver(AsyncWebServer* server);
bool initBtAudio();
void updateBtAudioReceive();
void cleanupBtAudio();
#endif

// Forward declarations Radar Remote Webserver
void setup_radar_webserver(AsyncWebServer* server);
void initRadarServerConnection();
void updateRadarServer();
void updateGasAlarmBeep();  // Gestione beep allarme gas
bool useRemoteRadar();
// Variabili esterne dal modulo radar remoto (15_WEBSERVER_RADAR.ino)
extern bool radarServerEnabled;
extern bool radarRemotePresence;
extern uint8_t radarRemoteBrightness;
extern bool radarServerConnected;
extern bool gasAlarmActive;  // Allarme gas attivo

// ================== INDICATORE ALLARME GLOBALE ==================
/**
 * Disegna un LED indicatore in alto a destra:
 * - ROSSO lampeggiante: allarme sta suonando
 * - GIALLO fisso: sveglia abilitata (in attesa)
 * - Nessun LED: sveglie disabilitate
 * Da chiamare in TUTTE le modalità di visualizzazione
 */
void drawAlarmIndicator() {
  // Posizione LED: esattamente nell'angolo in alto a destra
  const int LED_X = 480;  // Bordo destro (potrebbe essere parzialmente fuori schermo)
  const int LED_Y = 0;    // Bordo superiore (potrebbe essere parzialmente fuori schermo)
  const int LED_RADIUS = 4;

  // Debug rimosso per ridurre output seriale

  // ========== CANCELLAZIONE AREA LED ==========
  // SEMPRE cancella l'area prima di disegnare (risolve problema LED che rimane)
  gfx->fillCircle(LED_X, LED_Y, LED_RADIUS + 3, BLACK);

  // ========== PRIORITÀ 1: ALLARME STA SUONANDO ==========
  if (bttfAlarmRinging
  #ifdef EFFECT_CALENDAR
      || calendarAlarmActive
  #endif
  ) {
    // LED ROSSO lampeggiante ogni 500ms
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    unsigned long currentTime = millis();
    if (currentTime - lastBlink >= 500) {
      ledState = !ledState;
      lastBlink = currentTime;
    }

    if (ledState) {
      // LED ACCESO: cerchio rosso pieno con alone giallo
      gfx->fillCircle(LED_X, LED_Y, LED_RADIUS + 2, YELLOW);  // Alone esterno
      gfx->fillCircle(LED_X, LED_Y, LED_RADIUS, RED);         // Cerchio rosso
    } else {
      // LED SPENTO: cerchio scuro con bordo rosso
      gfx->fillCircle(LED_X, LED_Y, LED_RADIUS, 0x3000);      // Rosso scuro
      gfx->drawCircle(LED_X, LED_Y, LED_RADIUS, RED);         // Bordo rosso
    }
    return;  // Esce subito, priorità massima al rosso
  }

  // ========== PRIORITÀ 2: SVEGLIA ABILITATA (IN ATTESA) ==========
  if (alarmDestinationEnabled || alarmLastDepartedEnabled) {
    // LED GIALLO fisso = sveglia armata, in attesa di scattare
    gfx->fillCircle(LED_X, LED_Y, LED_RADIUS, YELLOW);       // Cerchio giallo pieno
    gfx->drawCircle(LED_X, LED_Y, LED_RADIUS + 1, ORANGE);   // Bordo arancione

    // Debug: conferma LED giallo
    static unsigned long lastDrawLog = 0;
    if (millis() - lastDrawLog > 10000) {
      Serial.printf("[ALARM LED] 🟡 LED GIALLO (sveglia abilitata) a (%d, %d)\n", LED_X, LED_Y);
      lastDrawLog = millis();
    }
    return;
  }

  // ========== NESSUNA SVEGLIA ATTIVA ==========
  // Non disegna nulla se nessuna sveglia è abilitata
}

/*void  testSpeakerSafely() {
  // Generiamo un'onda quadra a 440Hz per circa 1 secondo
  int16_t sample[2]; 
  
  for (int i = 0; i < 44100; i++) {
    // Creiamo il suono (valore 4000 per un volume udibile ma sicuro)
    int16_t val = (i % 100 < 50) ? 4000 : -4000;
    
    sample[0] = val; // Canale Sinistro
    sample[1] = val; // Canale Destro
    
    // Invia il suono all'hardware
    output->ConsumeSample(sample);
    
    // Un micro-ritardo ogni 100 campioni per non saturare il buffer
    if (i % 100 == 0) delayMicroseconds(100); 
  }
}*/

//////////////TEST NUOVO I2S AUDIO//////////////////////////////////////////
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
///////////////////////////////////////////////////////////////////////////
// ================== SETUP PRINCIPALE ==================
void setup() {

/////////////////TEST NUOVO I2S AUDIO//////////////////////////////////////
    xTaskCreatePinnedToCore(
        audioTask,      // Funzione del task assegnazione a core 0 esp32 della task per AUDIO
        "AudioTask",    // Nome
        16384,          // Stack size (aumentato per decoder MP3)
        NULL,           // Parametri
        2,              // Priorità alta
        NULL,           // Handle
        0               // CORE 0
    );
/////////////////////////////////////////////////////////////////////////////
  // RICAVO NOME SKETCH DA STAMPARE A VIDEO
  int lastSlash = ino.lastIndexOf("/");
  if (lastSlash == -1) {
      lastSlash = ino.lastIndexOf("\\");
  }
  ino = ino.substring(lastSlash + 1);  // MyClock.ino
  
  int dotIndex = ino.lastIndexOf(".");
  if (dotIndex > 0) {
      ino = ino.substring(0, dotIndex);  // MyClock
  }

  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\nOraQuadra2 Display 480x480 - Inizializzazione");
  
if (!LittleFS.begin(true)) {
  Serial.println("ERRORE CRITICO: LittleFS non inizializzabile!");
  while(1) delay(1000);
}
Serial.println("LittleFS inizializzato correttamente.");

  // Carica eventi calendario locali da LittleFS + snooze da EEPROM
  #ifdef EFFECT_CALENDAR
  loadLocalEventsFromLittleFS();
  loadCalSnoozeFromEEPROM();
  #endif

  Serial.printf("LittleFS: %d KB totali, %d KB usati\n",
                LittleFS.totalBytes()/1024, LittleFS.usedBytes()/1024);

  // Diagnostica PSRAM (per buffer audio e display)
  if (psramFound()) {
    Serial.printf("PSRAM: %d KB totali, %d KB liberi\n",
                  ESP.getPsramSize()/1024, ESP.getFreePsram()/1024);
  } else {
    Serial.println("PSRAM: Non disponibile");
  }

  // SETUP COMPONENTI IN SEQUENZA
  setup_eeprom();  // Inizializza e carica le impostazioni dalla EEPROM.
  setup_display(); // Inizializza il display.
  setup_touch();   // Inizializza il touch screen.

   // VERIFICA SE I 4 ANGOLI SONO PREMUTI ALL'AVVIO (per triggerare un reset delle impostazioni WiFi)
  int cornerCheckAttempts = 0;
  while (cornerCheckAttempts < 5) { // Facciamo più tentativi per stabilizzare il touch
    if (checkCornersAtBoot()) {
      // Mostra feedback e conferma
      gfx->fillScreen(YELLOW);
      gfx->setTextColor(BLACK);
      gfx->setCursor(80, 180);
      gfx->println(F("RESET WIFI AVVIO"));
      gfx->setCursor(120, 240);
      gfx->println(F("RILEVATO"));
      delay(2000);
      
      // Esegui il reset WiFi (funzione definita altrove)
      resetWiFiSettings();
      // Non ritorna mai da qui (il dispositivo si riavvia dopo il reset WiFi)
    }
    cornerCheckAttempts++;
    delay(50); // Piccola pausa tra i controlli
  }

  // Setup WiFi e servizi correlati (solo se necessario)
  setup_wifi();

  if (WiFi.status() == WL_CONNECTED) {
    setup_OTA();   // Inizializza l'aggiornamento Over-The-Air.
    setup_alexa(); // Inizializza l'integrazione con Alexa (server sulla porta 80).

    // Crea web server separato per configurazione (porta 8080)
    // Sempre disponibile per pagina impostazioni generali
    clockWebServer = new AsyncWebServer(8080);

    // Homepage con link a tutte le pagine (sempre disponibile)
    clockWebServer->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", HOME_HTML);
    });
    Serial.println("[WEBSERVER] Homepage disponibile su /");

    // Pagina impostazioni generali (sempre disponibile)
    setup_settings_webserver(clockWebServer);
    Serial.println("[WEBSERVER] Impostazioni generali disponibili su /settings");

    #ifdef EFFECT_ANALOG_CLOCK
    setup_clock_webserver(clockWebServer);
    Serial.println("[WEBSERVER] Configurazione orologio analogico disponibile su /clock");
    #endif

    #ifdef EFFECT_BTTF
    setup_bttf_webserver(clockWebServer);
    Serial.println("[WEBSERVER] Configurazione BTTF disponibile su /bttf");
    #endif

    #ifdef EFFECT_GEMINI_AI
    setup_gemini_webserver(clockWebServer);
    Serial.println("[WEBSERVER] Gemini AI disponibile su /gemini");
    #endif

    #ifdef EFFECT_MJPEG_STREAM
    setup_mjpeg_webserver(clockWebServer);
    Serial.println("[WEBSERVER] MJPEG Stream Control disponibile su /mjpeg");
    #endif

    #ifdef EFFECT_ESP32CAM
    setup_esp32cam_webserver(clockWebServer);
    Serial.println("[WEBSERVER] ESP32-CAM Config disponibile su /espcam");
    #endif

    #ifdef EFFECT_FLUX_CAPACITOR
    setup_flux_webserver(clockWebServer);
    Serial.println("[WEBSERVER] Flux Capacitor Config disponibile su /fluxcap");
    #endif

    #ifdef EFFECT_LED_RING
    setupLedRingWebServer(*clockWebServer);
    Serial.println("[WEBSERVER] LED Ring Config disponibile su /ledring");
    #endif

    #ifdef EFFECT_MJPEG_STREAM
    setup_bt_audio_webserver(clockWebServer);
    Serial.println("[WEBSERVER] Bluetooth Audio disponibile su /btaudio");
    #endif

    // Radar Remote - ricezione notifiche da radar server esterno
    setup_radar_webserver(clockWebServer);
    Serial.println("[WEBSERVER] Radar Remote disponibile su /radar/*");

    #ifdef EFFECT_MP3_PLAYER
    setup_mp3player_webserver(clockWebServer);
    Serial.println("[WEBSERVER] MP3 Player disponibile su /mp3player");
    #endif

    #ifdef EFFECT_WEB_RADIO
    setup_webradio_webserver(clockWebServer);
    Serial.println("[WEBSERVER] Web Radio disponibile su /webradio");
    #endif

    #ifdef EFFECT_RADIO_ALARM
    setup_radioalarm_webserver(clockWebServer);
    Serial.println("[WEBSERVER] Radio Alarm disponibile su /radioalarm");
    #endif

    #ifdef EFFECT_WEB_TV
    setup_webtv_webserver(clockWebServer);
    Serial.println("[WEBSERVER] Web TV disponibile su /webtv");
    #endif

    #ifdef EFFECT_CALENDAR
    setup_calendar_webserver(clockWebServer);
    Serial.println("[WEBSERVER] Calendario disponibile su /calendar");
    #endif

    #ifdef EFFECT_LED_RGB
    setup_ledrgb_webserver(clockWebServer);
    Serial.println("[WEBSERVER] LED RGB disponibile su /ledrgb");
    #endif

    clockWebServer->begin();
    Serial.println("[WEBSERVER] Server configurazione avviato su porta 8080");
    Serial.println("[WEBSERVER] Accedi a http://" + WiFi.localIP().toString() + ":8080/");

    // Tenta connessione al radar server remoto (dopo setup webserver)
    initRadarServerConnection();

    setup_NTP();   // Inizializza la sincronizzazione dell'ora tramite NTP.
    // Inizializza i flag dell'annuncio orario con l'ora attuale al boot
    lastAudioAnnounceHour = myTZ.hour();
    lastMinuteChecked = myTZ.minute();
    #ifdef AUDIO
    setup_audio(); // Inizializza il sistema audio (solo se la macro AUDIO è definita).
    #endif

      
  } else {
    Serial.println("WiFi non connesso, utilizzo ora predefinita");
    currentHour = 12;
    currentMinute = 0;
    currentSecond = 0;
  }

  // Inizializza SD Card (dopo setup audio per evitare conflitti su PIN 41)
  setup_sd();

  #ifdef EFFECT_BTTF
  // Carica configurazione allarmi BTTF dalla SD all'avvio
  extern bool loadBTTFConfigFromSD();
  if (loadBTTFConfigFromSD()) {
    Serial.println("[BTTF] Configurazione allarmi caricata da SD all'avvio");
  } else {
    Serial.println("[BTTF] Nessuna configurazione allarmi trovata, uso valori predefiniti");
  }
  #endif

  // Inizializza radar LD2410 (compatibile con audio WiFi esterno - nessun conflitto pin)
  setup_radar();

  // Inizializza sensore BME280 per temperatura/umidità/pressione interna (I2C)
  setup_bme280();

  // Inizializza audio I2C con ESP32C3 esterno (I2C condiviso con GT911 touchscreen)
  setup_audio_i2c();

  // Inizializza magnetometro QMC5883P (I2C condiviso con touch GT911)
  // NOTA: È un QMC5883P (non QMC5883L!), indirizzo 0x0C, libreria Adafruit_QMC5883P
  setup_magnetometer();

  #ifdef EFFECT_LED_RGB
  setup_led_rgb();
  #endif

  // Inizializzazione delle opzioni di setup (caricamento dalla EEPROM o valori predefiniti)
  initSetupOptions();
  initSetupMenu(); // Inizializza la struttura del menu di setup.

  // NON chiamare applyPreset all'avvio!
  // I valori di currentMode, currentColor, currentPreset sono già stati caricati dalla EEPROM
  // in loadSavedSettings(). Chiamare applyPreset sovrascriverebbe questi valori con i default del preset.
  // applyPreset(currentPreset);  // RIMOSSO - causa perdita impostazioni al riavvio

  Serial.printf("[SETUP] Impostazioni caricate: Mode=%d, Preset=%d, Color=R%d G%d B%d\n",
                currentMode, currentPreset, currentColor.r, currentColor.g, currentColor.b);

  // Inizializza modulo meteo OpenWeatherMap (dopo WiFi)
  if (WiFi.status() == WL_CONNECTED) {
    initWeather();
  }


///////////////////////////////////WEB RADIO INIT///////////////////////////////////////////////
    // Inizializza audio I2S per web radio
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21); // default 0...21
    Serial.println("[WEBRADIO] Audio I2S configurato");
    // Carica lista stazioni radio da SD card
    loadWebRadioStationsFromSD();
    // Carica impostazioni salvate (enabled, volume, stazione) e avvia radio se era attiva
    loadWebRadioSettings();

    // Carica impostazioni radiosveglia da EEPROM (DOPO le stazioni radio!)
    #ifdef EFFECT_RADIO_ALARM
    loadRadioAlarmSettings();
    Serial.printf("[RADIO-ALARM] Impostazioni caricate: %02d:%02d, %s\n",
                  radioAlarm.hour, radioAlarm.minute,
                  radioAlarm.enabled ? "ATTIVA" : "DISATTIVA");
    #endif
//////////////////////////////////////////////////////////////////////////////////////////////////////

  // Annuncio boot DOPO che audio I2S è inizializzato
  playBootAnnounce();


  // ========== IMPORTANTE: FORZA LUMINOSITÀ INIZIALE ==========
  // Assicura che il display sia SEMPRE acceso dopo il boot
  // anche se il radar non è disponibile/connesso
  // Questa impostazione viene applicata SEMPRE, con o senza WiFi/NTP
  Serial.println("\n========================================");
  Serial.println("IMPOSTAZIONE LUMINOSITÀ INIZIALE");
  Serial.println("========================================");
  // Usa dayStartHour/Minute e nightStartHour/Minute configurabili da web
  bool isNightTime = checkIsNightTime(currentHour, currentMinute);
  if (isNightTime) {
    ledcWrite(PWM_CHANNEL, brightnessNight);
    lastAppliedBrightness = brightnessNight;  // SYNC per loop
    Serial.printf("Luminosità NOTTE: %d (ora: %02d:%02d, notte dalle %02d:%02d)\n", brightnessNight, currentHour, currentMinute, nightStartHour, nightStartMinute);
  } else {
    ledcWrite(PWM_CHANNEL, brightnessDay);
    lastAppliedBrightness = brightnessDay;    // SYNC per loop
    Serial.printf("Luminosità GIORNO: %d (ora: %02d:%02d, giorno dalle %02d:%02d)\n", brightnessDay, currentHour, currentMinute, dayStartHour, dayStartMinute);
  }
  Serial.println("Display: ACCESO");
  Serial.println("========================================\n");

}

// ================== IMPLEMENTAZIONI WEB RADIO ==================

// Carica impostazioni web radio da EEPROM
void loadWebRadioSettings() {
  // NON avviare automaticamente al boot - l'utente decide quando attivare
  webRadioEnabled = false;  // Sempre spenta al boot

  webRadioVolume = EEPROM.read(EEPROM_WEBRADIO_VOLUME_ADDR);
  if (webRadioVolume > 100) webRadioVolume = 100;

  uint8_t stationIdx = EEPROM.read(EEPROM_WEBRADIO_STATION_ADDR);
  if (stationIdx < webRadioStationCount) {
    webRadioCurrentIndex = stationIdx;
    webRadioName = webRadioStations[stationIdx].name;
    webRadioUrl = webRadioStations[stationIdx].url;
  }

  // NON impostare volume qui - verra' impostato all'avvio della radio
  // audio.setVolume(webRadioVolume);

  // Carica anche volume annunci orari
  announceVolume = EEPROM.read(EEPROM_ANNOUNCE_VOLUME_ADDR);
  if (announceVolume > 100) announceVolume = 70;  // Default 70

  // Radio NON si avvia automaticamente - utente la attiva dalla pagina dedicata
  Serial.printf("[WEBRADIO] Caricato: volume=%d, station=%d (radio SPENTA al boot)\n",
                webRadioVolume, webRadioCurrentIndex);
  Serial.printf("[ANNOUNCE] Volume annunci: %d\n", announceVolume);
}

// Salva impostazioni web radio in EEPROM
void saveWebRadioSettings() {
  EEPROM.write(EEPROM_WEBRADIO_ENABLED_ADDR, webRadioEnabled ? 1 : 0);
  EEPROM.write(EEPROM_WEBRADIO_VOLUME_ADDR, webRadioVolume);
  EEPROM.write(EEPROM_WEBRADIO_STATION_ADDR, (uint8_t)webRadioCurrentIndex);
  EEPROM.commit();
  Serial.printf("[WEBRADIO] Salvato: enabled=%d, volume=%d, station=%d\n",
                webRadioEnabled, webRadioVolume, webRadioCurrentIndex);
}

// Salva volume annunci orari in EEPROM
void saveAnnounceVolume() {
  EEPROM.write(EEPROM_ANNOUNCE_VOLUME_ADDR, announceVolume);
  EEPROM.commit();
  Serial.printf("[ANNOUNCE] Volume salvato: %d\n", announceVolume);
}

// Avvia web radio
void startWebRadio() {
  if (webRadioEnabled) return;
  Serial.println("[WEBRADIO] Avvio streaming...");
  audio.setVolume(map(webRadioVolume, 0, 100, 0, 21));  // Converte 0-100 a 0-21
  audio.connecttohost(webRadioUrl.c_str());
  webRadioEnabled = true;
  
}

// Ferma web radio
void stopWebRadio() {
  if (!webRadioEnabled) return;
  Serial.println("[WEBRADIO] Stop streaming...");
  audio.stopSong();
  webRadioEnabled = false;
  
}

// Imposta volume web radio
void setWebRadioVolume(uint8_t vol) {
  if (vol > 100) vol = 100;
  webRadioVolume = vol;
  audio.setVolume(map(vol, 0, 100, 0, 21));  // Converte 0-100 a 0-21
  
}

// Seleziona stazione radio
void selectWebRadioStation(int index) {
  if (index < 0 || index >= webRadioStationCount) return;

  webRadioCurrentIndex = index;
  webRadioName = webRadioStations[index].name;
  webRadioUrl = webRadioStations[index].url;

  Serial.printf("[WEBRADIO] Selezionata: %s\n", webRadioName.c_str());

  if (webRadioEnabled) {
    audio.stopSong();
    delay(200);
    audio.connecttohost(webRadioUrl.c_str());
  }

  
}

// Inizializza radio di default
void initDefaultRadioStations() {
  webRadioStationCount = 0;
  addWebRadioStation("Rai Radio 1", "http://icestreaming.rai.it/1.mp3");
  addWebRadioStation("Rai Radio 2", "http://icestreaming.rai.it/2.mp3");
  addWebRadioStation("Rai Radio 3", "http://icestreaming.rai.it/3.mp3");
  addWebRadioStation("RTL 102.5", "http://shoutcast.rtl.it:3010/");
  addWebRadioStation("Virgin Radio", "http://icecast.unitedradio.it/Virgin.mp3");
}

// Aggiunge stazione radio
bool addWebRadioStation(const String& name, const String& url) {
  if (webRadioStationCount >= MAX_RADIO_STATIONS) return false;
  webRadioStations[webRadioStationCount].name = name;
  webRadioStations[webRadioStationCount].url = url;
  webRadioStationCount++;
  return true;
}

// Rimuove stazione radio
bool removeWebRadioStation(int index) {
  if (index < 0 || index >= webRadioStationCount) return false;
  for (int i = index; i < webRadioStationCount - 1; i++) {
    webRadioStations[i] = webRadioStations[i + 1];
  }
  webRadioStationCount--;
  if (webRadioCurrentIndex >= webRadioStationCount) {
    webRadioCurrentIndex = webRadioStationCount > 0 ? 0 : -1;
  }
  return true;
}

// Salva lista radio su SD
void saveWebRadioStationsToSD() {
  File file = SD.open("/webradio.txt", FILE_WRITE);
  if (!file) return;
  for (int i = 0; i < webRadioStationCount; i++) {
    file.print(webRadioStations[i].name);
    file.print("|");
    file.println(webRadioStations[i].url);
  }
  file.close();
}

// Carica lista radio da SD
void loadWebRadioStationsFromSD() {
  webRadioStationCount = 0;
  if (!SD.exists("/webradio.txt")) {
    initDefaultRadioStations();
    saveWebRadioStationsToSD();
    return;
  }

  File file = SD.open("/webradio.txt", FILE_READ);
  if (!file) {
    initDefaultRadioStations();
    return;
  }

  while (file.available() && webRadioStationCount < MAX_RADIO_STATIONS) {
    String line = file.readStringUntil('\n');
    line.trim();
    int sep = line.indexOf('|');
    if (sep > 0) {
      String name = line.substring(0, sep);
      String url = line.substring(sep + 1);
      addWebRadioStation(name, url);
    }
  }
  file.close();

  if (webRadioStationCount == 0) {
    initDefaultRadioStations();
  }
}

void setup_NTP() {
  Serial.println("Inizializzazione NTP...");

  // Configura server NTP (Network Time Protocol)
  setServer("pool.ntp.org");

  // Imposta il fuso orario locale - Italia con regola POSIX esplicita
  // CET-1CEST,M3.5.0,M10.5.0/3 = Europa centrale (GMT+1/+2 con DST)
  // DST inizia ultima domenica marzo, finisce ultima domenica ottobre
  myTZ.setPosix("CET-1CEST,M3.5.0,M10.5.0/3");

  // Attendi prima sincronizzazione (timeout aumentato a 15 secondi per reti lente)
  waitForSync(15);

  // Verifica corretta usando timeStatus() della libreria ezTime
  if (timeStatus() == timeSet) {
    ntpSynced = true;
    Serial.println("NTP sincronizzato. Ora corrente: " + myTZ.dateTime("H:i:s"));

    // Imposta l'ora corrente per le variabili del sistema
    currentHour = myTZ.hour();
    currentMinute = myTZ.minute();
    currentSecond = myTZ.second();

    Serial.printf("Impostazione ora: %02d:%02d:%02d\n",
                  currentHour, currentMinute, currentSecond);
  } else {
    ntpSynced = false;
    Serial.println("Sincronizzazione NTP non riuscita, utilizzo ora predefinita");
    Serial.println("Il sistema ritenterà la sincronizzazione automaticamente");
    // Imposta un'ora predefinita in caso di fallimento della sincronizzazione
    currentHour = 12;
    currentMinute = 0;
    currentSecond = 0;
  }

  // IMPORTANTE: Forza luminosità iniziale basata su giorno/notte
  // Questo assicura che il display sia sempre acceso dopo il boot
  // anche se il radar non è disponibile
  Serial.println("Impostazione luminosità dopo NTP...");
  if (checkIsNightTime(currentHour, currentMinute)) {
    ledcWrite(PWM_CHANNEL, brightnessNight);
    lastAppliedBrightness = brightnessNight;  // SYNC per loop
    Serial.printf("Luminosità NOTTE applicata: %d (ora: %02d:%02d, notte dalle %02d:%02d)\n", brightnessNight, currentHour, currentMinute, nightStartHour, nightStartMinute);
  } else {
    ledcWrite(PWM_CHANNEL, brightnessDay);
    lastAppliedBrightness = brightnessDay;    // SYNC per loop
    Serial.printf("Luminosità GIORNO applicata: %d (ora: %02d:%02d, giorno dalle %02d:%02d)\n", brightnessDay, currentHour, currentMinute, dayStartHour, dayStartMinute);
  }
}

// Funzione per ritentare sincronizzazione NTP (chiamata dal loop)
void retryNTPSync() {
  if (ntpSynced) return; // Già sincronizzato

  uint32_t now = millis();
  // Riprova ogni 30 secondi se non sincronizzato
  if (now - lastNtpRetry < 30000) return;
  lastNtpRetry = now;

  Serial.println("Tentativo risincronizzazione NTP...");

  // Forza una nuova sincronizzazione
  updateNTP();

  // Verifica se la sincronizzazione è riuscita
  if (timeStatus() == timeSet) {
    ntpSynced = true;
    currentHour = myTZ.hour();
    currentMinute = myTZ.minute();
    currentSecond = myTZ.second();
    Serial.println("NTP risincronizzato! Ora: " + myTZ.dateTime("H:i:s"));

    // Aggiorna luminosità in base all'ora corretta
    if (checkIsNightTime(currentHour, currentMinute)) {
      ledcWrite(PWM_CHANNEL, brightnessNight);
      lastAppliedBrightness = brightnessNight;
    } else {
      ledcWrite(PWM_CHANNEL, brightnessDay);
      lastAppliedBrightness = brightnessDay;
    }
  } else {
    Serial.println("Risincronizzazione NTP fallita, riprovo tra 30s");
  }
}

void setup_audio() {
  #ifdef AUDIO
  Serial.println("Inizializzazione sistema audio...");

  extern void setVolumeLocal(uint8_t volume);
  lastWasNightTime = checkIsNightTime(currentHour, currentMinute);
  uint8_t startupVolume = lastWasNightTime ? volumeNight : volumeDay;
  setVolumeLocal(startupVolume);
  lastAppliedVolume = startupVolume;

  Serial.printf("[AUDIO-LOCAL] Avvio: Volume impostato a %d%% (%s)\n",
                lastAppliedVolume, lastWasNightTime ? "NOTTE" : "GIORNO");
  #endif
}

// ================== FUNZIONE LETTURA SENSORE BME280 ==================
// Extern per dati radar remoto (definiti in 15_WEBSERVER_RADAR.ino)
extern bool radarServerConnected;
extern float radarRemoteTemperature;
extern float radarRemoteHumidity;

void readBME280Temperature() {
  // PRIORITÀ 1: BME280 interno
  if (bme280Available) {
    // Leggi valori grezzi dal sensore
    float rawTemp = bme.readTemperature();
    float rawHum = bme.readHumidity();
    pressureIndoor = bme.readPressure() / 100.0F;  // Converte Pa in hPa

    // Validazione: temperature ragionevoli tra -40°C e +85°C
    if (rawTemp < -40.0 || rawTemp > 85.0) {
      Serial.println("⚠ Temperatura BME280 fuori range, marcata come N.D.");
      bme280Available = false;  // Disabilita se fuori range
      // Prova fallback al radar
    } else if (rawHum < 0.0 || rawHum > 100.0) {
      // Validazione: umidità tra 0% e 100%
      Serial.println("⚠ Umidità BME280 fuori range, marcata come N.D.");
      bme280Available = false;  // Disabilita se fuori range
      // Prova fallback al radar
    } else if (pressureIndoor < 300.0 || pressureIndoor > 1100.0) {
      // Validazione: pressione ragionevole tra 300 e 1100 hPa
      Serial.println("⚠ Pressione BME280 fuori range, marcata come N.D.");
      bme280Available = false;  // Disabilita se fuori range
      // Prova fallback al radar
    } else {
      // Dati BME280 validi - applica offset di calibrazione
      temperatureIndoor = rawTemp + bme280TempOffset;
      humidityIndoor = rawHum + bme280HumOffset;

      // Limita umidità tra 0% e 100% dopo offset
      if (humidityIndoor < 0.0) humidityIndoor = 0.0;
      if (humidityIndoor > 100.0) humidityIndoor = 100.0;

      indoorSensorSource = 1;  // Fonte: BME280 interno
      return;
    }
  }

  // PRIORITÀ 2: Fallback al radar remoto (se BME280 non disponibile)
  if (radarServerConnected && radarRemoteTemperature > -40.0 && radarRemoteTemperature < 85.0) {
    // Usa dati dal radar remoto
    float rawTemp = radarRemoteTemperature;
    float rawHum = radarRemoteHumidity;

    // Applica offset di calibrazione anche ai dati del radar
    temperatureIndoor = rawTemp + bme280TempOffset;
    humidityIndoor = rawHum + bme280HumOffset;

    // Limita umidità tra 0% e 100% dopo offset
    if (humidityIndoor < 0.0) humidityIndoor = 0.0;
    if (humidityIndoor > 100.0) humidityIndoor = 100.0;

    // Pressione non disponibile dal radar
    pressureIndoor = 0.0;

    indoorSensorSource = 2;  // Fonte: Radar remoto
    return;
  }

  // Nessuna fonte disponibile
  indoorSensorSource = 0;
}

//////////////////////////TEST NUOVO I2S AUDIO////////////////////
// audio_eof_mp3() callback definito in 31_MP3_PLAYER.ino (dove MP3PlayerState e' visibile)

void audioTask(void *pvParameters) {
    while(1) {
        audio.loop();
        vTaskDelay(1);
    }
}
//////////////////////////////////////////////////////////////////

// ================== LOOP PRINCIPALE ==================
void loop() {
  static uint32_t lastUpdate = 0;     // Timestamp dell'ultimo aggiornamento dell'ora.
  static uint32_t lastUpdate_sec = 0; // Timestamp dell'ultimo aggiornamento dei secondi (potrebbe non essere strettamente necessario).
  static uint32_t lastButtonCheck = 0; // Timestamp dell'ultima verifica dei tocchi/pulsanti.
  static uint32_t lastEffectUpdate = 0; // Timestamp dell'ultimo aggiornamento degli effetti visivi.
  static bool lastEState = false;    // Stato precedente della lettera 'E' (probabilmente per il lampeggio).
  static uint8_t lastSecondFast = 255; // Ultimo secondo visualizzato in modalità FAST (per aggiornare solo al cambio di secondo).

  // Variabili static per controllo luminosità radar (fuori dai blocchi condizionali)
  //#ifndef AUDIO
  static uint32_t lastLightRead = 0;
  static uint32_t lastBrightnessUpdate = 0;
  static uint32_t lastBrightnessDebug = 0;
  //#endif

  uint32_t currentMillis = millis();  // Ottiene il tempo attuale in millisecondi dall'avvio.

  // ========== VERIFICA PROTEZIONE DISTRIBUITA ==========
  // Controllo periodico anti-tampering (ogni ~30 secondi)
  static uint32_t lastProtCheck = 0;
  if (currentMillis - lastProtCheck > 30000) {
    lastProtCheck = currentMillis;
    PROTECTION_CHECK_SILENT();
    PROTECTION_CHECK_RANDOM(currentMillis);
  }

  // Gestione OTA (Over-The-Air update) e network events (ezTime)
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle(); // Gestisce le eventuali richieste di aggiornamento OTA.
    events(); // Gestisce gli eventi della libreria ezTime (es. cambio di ora).
    retryNTPSync(); // Ritenta sincronizzazione NTP se fallita al boot
    espalexa.loop(); // Gestisce le comunicazioni con Alexa.
    updateRadarServer(); // Gestisce riconnessione radar server remoto
    updateGasAlarmBeep(); // Gestisce beep allarme gas (se attivo)

    // Controllo trigger radiosveglia
    #ifdef EFFECT_RADIO_ALARM
    checkRadioAlarmTrigger();
    #endif

    
    //audio.loop();


    // Gestione aggiornamenti Alexa NON BLOCCANTI
    if (alexaUpdatePending) {
      alexaUpdatePending = false;
      if (alexaOff == 1) {
        clearDisplay(); // Pulisce il display se Alexa ha spento
      } else {
        forceDisplayUpdate(); // Aggiorna il display con il nuovo colore
      }
    }

    // Gestione aggiornamenti da WebServer Settings NON BLOCCANTI
    if (webForceDisplayUpdate) {
      webForceDisplayUpdate = false;
      forceDisplayUpdate(); // Aggiorna il display con il nuovo preset/colore
    }

    // Salvataggio differito URL ESP32-CAM in EEPROM (dal web handler asincrono)
    #ifdef EFFECT_ESP32CAM
    extern void esp32camCheckPendingSave();
    esp32camCheckPendingSave();
    #endif
  } else {
    // WiFi disconnesso - tenta reconnect
    static uint32_t lastReconnectAttempt = 0;
    if (currentMillis - lastReconnectAttempt > 5000) { // Prova ogni 5 secondi
      lastReconnectAttempt = currentMillis;
      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
      WiFi.begin(); // Riconnette con le credenziali salvate
    }
  }

  // Check dei tocchi ogni 50ms
  if (currentMillis - lastButtonCheck > 50) {
    checkButtons(); // Funzione per leggere e gestire gli eventi del touch screen.
    lastButtonCheck = currentMillis;
  }

  // ========== CAMBIO MODALITA' RANDOM AUTOMATICO ==========
  // Se abilitato, cambia la modalità display ogni X minuti (scegliendo tra le modalità abilitate)
  if (randomModeEnabled && !setupPageActive && !capodannoActive) {
    uint32_t randomIntervalMs = (uint32_t)randomModeInterval * 60000UL;  // Converti minuti in millisecondi
    if (currentMillis - lastRandomModeChange >= randomIntervalMs) {
      lastRandomModeChange = currentMillis;

      // Trova una modalità random tra quelle abilitate
      DisplayMode newMode = getNextEnabledMode(currentMode);

      // Se ci sono più modalità abilitate, scegli random
      // Conta quante modalità sono abilitate
      uint8_t enabledCount = 0;
      for (int i = 0; i < NUM_MODES; i++) {
        if (isModeEnabled(i)) enabledCount++;
      }

      if (enabledCount > 1) {
        // Scegli una modalità random diversa da quella corrente
        uint8_t attempts = 0;
        do {
          uint8_t randomIndex = random(NUM_MODES);
          if (isModeEnabled(randomIndex) && randomIndex != (int)currentMode) {
            newMode = (DisplayMode)randomIndex;
            break;
          }
          attempts++;
        } while (attempts < 50);  // Max 50 tentativi per evitare loop infinito
      }

      // Cambia modalità
      if (newMode != currentMode) {
        Serial.printf("[RANDOM MODE] Cambio automatico: %d -> %d\n", (int)currentMode, (int)newMode);

        // *** CLEANUP DELLA MODALITA' PRECEDENTE ***
        DisplayMode previousMode = currentMode;
        cleanupPreviousMode(previousMode);

        currentMode = newMode;
        userMode = newMode;

        // Salva in EEPROM
        //EEPROM.write(EEPROM_MODE_ADDR, (uint8_t)newMode);
        //EEPROM.commit();

        // Forza reinizializzazione completa della nuova modalità
        forceDisplayUpdate();

        lastHour = 255;
        lastMinute = 255;
      }
    }
  }

  // Verifica timeout della pagina di setup
  if (setupPageActive && currentMillis - setupLastActivity > setupPageTimeout) {
    hideSetupPage(); // Nasconde la pagina di setup se non c'è stata attività per un certo periodo.
  }

  // ========== GESTIONE RADAR LD2410 PER RILEVAMENTO PRESENZA ==========
  //#ifndef AUDIO

  if (radarAvailable && currentMillis - lastRadarRead > RADAR_READ_INTERVAL) {
    lastRadarRead = currentMillis;

    // SEMPRE leggi i dati dal radar (MyLD2410)
    MyLD2410::Response response = radar.check();

    // Verifica se il radar è connesso (in basic o enhanced mode)
    if (radar.inBasicMode() || radar.inEnhancedMode()) {
      // Rileva prima connessione riuscita
      if (!radarConnectedOnce) {
        radarConnectedOnce = true;
        Serial.println("[RADAR] Connesso OK");
      }

      // SEMPRE aggiorna il livello luce per la pagina web
      lastRadarLightLevel = radar.getLightLevel();

      // Verifica se è stata rilevata presenza
      // LOGICA: Rileva SEMPRE sia movimento che target statici
      bool movementDetected = radar.movingTargetDetected();
      bool stationaryDetected = radar.stationaryTargetDetected();

      if (movementDetected || stationaryDetected) {
        // Presenza rilevata! Aggiorna SEMPRE il timestamp
        lastPresenceTime = currentMillis;

        if (!presenceDetected) {
          presenceDetected = true;

          // Se radar remoto è abilitato, NON gestire luminosità dal radar locale
          if (radarServerEnabled) {
            Serial.println("[RADAR LOCALE] Presenza rilevata, ma radar remoto abilitato - ignoro");
          } else {
            // Forza riaccensione immediata del display (solo se radar remoto non abilitato)
            uint8_t wakeupBrightness;
            if (radarBrightnessControl) {
              // Usa sensore luce radar per luminosità con range configurabile (min-max)
              uint8_t lightLevel = radar.getLightLevel();
              wakeupBrightness = map(constrain(lightLevel, 0, 255), 0, 255, radarBrightnessMin, radarBrightnessMax);
              Serial.printf("[RADAR] Presenza! Luce:%d Lum:%d (min:%d max:%d)\n", lightLevel, wakeupBrightness, radarBrightnessMin, radarBrightnessMax);
            } else {
              // Usa luminosità manuale giorno/notte
              wakeupBrightness = checkIsNightTime(currentHour, currentMinute) ? brightnessNight : brightnessDay;
              Serial.printf("[RADAR] Presenza! Lum manuale:%d\n", wakeupBrightness);
            }
            ledcWrite(PWM_CHANNEL, wakeupBrightness);
            lastAppliedBrightness = wakeupBrightness;
          }
        }
      }
    } else {
      // Radar non connesso - tenta riconnessione ogni 10 secondi
      static uint32_t lastReconnectAttempt = 0;
      if (currentMillis - lastReconnectAttempt > 10000) {
        lastReconnectAttempt = currentMillis;
        Serial.println("\n⚠ Radar non connesso - Tentativo riconnessione...");

        // Prova a reinizializzare
        if (radar.begin()) {
          Serial.println("✓ Radar riconnesso!");
          radarConnectedOnce = false; // Reset per mostrare il messaggio di connessione

          // Riattiva Enhanced Mode
          radar.enhancedMode(true);
        } else {
          Serial.println("✗ Riconnessione fallita. Verifica:");
          Serial.println("  1. Alimentazione radar (LED acceso?)");
          Serial.println("  2. TX radar -> Pin 2 (IO2)");
          Serial.println("  3. RX radar -> Pin 1 (IO1)");
          Serial.println("  4. Cavo di alimentazione 5V stabile");
        }
      }
    }

    // Verifica se è passato il timeout senza presenza
    if (presenceDetected && (currentMillis - lastPresenceTime > PRESENCE_TIMEOUT)) {
      presenceDetected = false;
      Serial.println("[RADAR] Timeout - OFF");
    }
  }

  // ========== CONTROLLO LUMINOSITÀ (3 MODALITÀ) ==========
  // PRIORITÀ: 3=REMOTO, 2=LOCALE, 1=MANUALE
  if (currentMillis - lastBrightnessUpdate > 200) {
    lastBrightnessUpdate = currentMillis;

    uint8_t targetBrightness;
    bool remoteRadarActive = useRemoteRadar();
    bool localRadarActive = radarAvailable && radarBrightnessControl && !remoteRadarActive;

    // MODALITÀ 3: RADAR REMOTO - gestito dagli handler HTTP
    if (remoteRadarActive) {
      // Non fare nulla qui - luminosità gestita da handleRadarBrightness()
      // Debug rimosso per performance
    }
    // RADAR REMOTO ABILITATO MA TIMEOUT - rispetta comunque lo stato presenza
    else if (radarServerEnabled && !radarRemotePresence) {
      // Radar server abilitato e nessuna presenza - mantieni display spento
      // Non sovrascrivere con luminosità manuale
      // Debug rimosso per performance
    }
    // MODALITÀ 2: RADAR LOCALE
    else if (localRadarActive) {
      bool shouldTurnOff = setupOptions.powerSaveEnabled && !presenceDetected;
      if (shouldTurnOff) {
        targetBrightness = 0;
      } else {
        targetBrightness = map(constrain(lastRadarLightLevel, 0, 255), 0, 255, radarBrightnessMin, radarBrightnessMax);
      }
      ledcWrite(PWM_CHANNEL, targetBrightness);
      lastAppliedBrightness = targetBrightness;
    }
    // MODALITÀ 1: MANUALE
    else {
      bool isNightTime = checkIsNightTime(currentHour, currentMinute);
      targetBrightness = isNightTime ? brightnessNight : brightnessDay;
      ledcWrite(PWM_CHANNEL, targetBrightness);
      lastAppliedBrightness = targetBrightness;
    }
  }
  //#endif

  // Gestione audio: audio.loop() e' gestito da audioTask su Core 0
  // EOF e cleanup sono gestiti dalla callback audio_eof_mp3()

  // ========== AGGIORNAMENTO AUDIO I2C (ESP32C3) ==========
  // Verifica periodicamente connessione I2C con ESP32C3 audio slave
  updateAudioI2C();

  // ========== AGGIORNAMENTO VOLUME AUTOMATICO GIORNO/NOTTE ==========
  extern bool setVolumeViaI2C(uint8_t volume); // Remoto (I2C)
  extern void setVolumeLocal(uint8_t volume);  // Locale (Interno)
  #ifdef EFFECT_RADIO_ALARM
  extern bool radioAlarmRinging;  // Sveglia sta suonando
  #endif

bool currentIsNight = checkIsNightTime(currentHour, currentMinute);

if (currentIsNight != lastWasNightTime) {
  // Cambio fascia oraria rilevato
  lastWasNightTime = currentIsNight;
  uint8_t targetVolume = currentIsNight ? volumeNight : volumeDay;

  // Non sovrascrivere il volume se la sveglia sta suonando o c'e' un annuncio in corso
  bool skipVolumeChange = isAnnouncing;
  #ifdef EFFECT_RADIO_ALARM
  skipVolumeChange = skipVolumeChange || radioAlarmRinging;
  #endif

  if (targetVolume != lastAppliedVolume && !skipVolumeChange) {

    #ifdef AUDIO
      // Eseguito SEMPRE al cambio fascia oraria, indipendentemente dallo slave
      setVolumeLocal(targetVolume);
    #else
      // Eseguito solo se NON è definita AUDIO e lo slave è connesso
      if (audioSlaveConnected) {
        setVolumeViaI2C(targetVolume);
      }
    #endif

    lastAppliedVolume = targetVolume;
    Serial.printf("[AUDIO] Cambio fascia oraria: %s - Volume: %d%%\n",
                  currentIsNight ? "NOTTE" : "GIORNO", targetVolume);
  }
}

  // ========== AGGIORNAMENTO MAGNETOMETRO QMC5883P (ogni 100ms) ==========
  // Legge heading dal magnetometro QMC5883P per la bussola nella Weather Station
  static unsigned long lastMagUpdate = 0;
  if (currentMillis - lastMagUpdate >= 100) {  // 10 Hz
    lastMagUpdate = currentMillis;
    updateMagnetometer();
  }

  // ========== GESTIONE ALLARME BTTF ==========
  #ifdef EFFECT_BTTF
  // Riproduce beep.mp3 in loop se l'allarme è attivo
  updateBTTFAlarmSound();
  #endif

  // ========== GESTIONE ALLARME CALENDARIO ==========
  #ifdef EFFECT_CALENDAR
  checkCalendarAlarm();          // Controlla se scatta un allarme
  updateCalendarAlarmSound();    // Gestisce beep continuo + lampeggio
  #endif

  // ========== CONTROLLO ANNUNCIO ORARIO (ogni 10 secondi) ==========
  // Annunci orari tramite ESP32C3 audio WiFi esterno
  static unsigned long lastTimeCheck = 0;
  if (millis() - lastTimeCheck > 10000 && WiFi.status() == WL_CONNECTED) {
    lastTimeCheck = millis();
    checkTimeAndAnnounce(); // Verifica se è il momento di annunciare l'ora vocalmente.
  }

  // ========== LETTURA SENSORE AMBIENTE (BME280 o Radar, ogni 30 secondi) ==========
  static uint32_t lastBME280Read = 0;
  // Chiama sempre la funzione - gestisce internamente BME280 interno o fallback a radar
  if (currentMillis - lastBME280Read >= 30000) {  // Ogni 30 secondi
    readBME280Temperature();
    lastBME280Read = currentMillis;
  }

  // ========== AGGIORNAMENTO METEO OPENWEATHERMAP (ogni 10 minuti) ==========
  if (WiFi.status() == WL_CONNECTED) {
    updateWeatherData();  // La funzione gestisce internamente l'intervallo
  }

  // ========== GESTIONE MODE SWITCH SICURO (dal webserver task) ==========
  {
    extern volatile bool pendingModeSwitch;
    if (pendingModeSwitch) {
      pendingModeSwitch = false;
      Serial.printf("[LOOP] Mode switch sicuro: da %d a FADE\n", currentMode);
      cleanupPreviousMode(currentMode);
      currentMode = MODE_FADE;
      forceDisplayUpdate();
    }
  }

  #ifdef EFFECT_CALENDAR
    static unsigned long lastCheck = 0;

    // Flag sync/push dalla pagina web (definiti in 36_WEBSERVER_CALENDAR.ino)
    extern volatile bool forceGoogleSync;
    extern volatile int16_t pushToGoogleId;
    extern String lastSyncTime;
    extern int mergedEventCount;

    // Gestione push evento verso Google (richiesto dalla pagina web)
    if (pushToGoogleId >= 0) {
      int16_t id = pushToGoogleId;
      pushToGoogleId = -1;
      Serial.printf("[CALENDAR] Push evento ID %d verso Google...\n", id);
      bool pushOk = pushEventToGoogle((uint16_t)id);
      mergeLocalAndGoogleEvents();
      char buf[20];
      sprintf(buf, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
      lastSyncTime = String(buf);
      if (pushOk) Serial.println("[CALENDAR] Push completato con successo");
      else Serial.println("[CALENDAR] Push fallito");
    }

    // Gestione sync forzata dalla pagina web
    if (forceGoogleSync) {
      forceGoogleSync = false;
      Serial.println("[CALENDAR] Sync Google forzata dalla pagina web...");
      fetchGoogleCalendarData();
      char buf[20];
      sprintf(buf, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
      lastSyncTime = String(buf);
      mergeLocalAndGoogleEvents();
      lastCheck = millis();
      if (currentMode == MODE_CALENDAR) {
        drawCalendarEvents();
      }
      Serial.println("[CALENDAR] Sync forzata completata.");
    }

    // Fetch Google: in calendario ogni 2 min, in background ogni 5 min (per allarmi)
    if (currentMode == MODE_CALENDAR) {
      if (millis() - lastCheck > 120000 || lastCheck == 0) {
        fetchGoogleCalendarData();
        lastCheck = millis();
        mergeLocalAndGoogleEvents();
        drawCalendarEvents();
        Serial.println("[CALENDAR] Dati aggiornati e visualizzati in diretta.");
      }
    } else {
      // Sync in background ogni 5 minuti per tenere aggiornati gli allarmi
      if (millis() - lastCheck > 300000 || lastCheck == 0) {
        Serial.println("[CALENDAR] Sync background per allarmi...");
        fetchGoogleCalendarData();
        lastCheck = millis();
        mergeLocalAndGoogleEvents();
        Serial.printf("[CALENDAR] Sync background completata: %d eventi\n", mergedEventCount);
      }
    }
    #endif

  // Aggiornamento orario ogni secondo se connesso al WiFi
  if (WiFi.status() == WL_CONNECTED && currentMillis - lastUpdate > 1000) {
    currentHour = myTZ.hour();     // Ottiene l'ora dal fuso orario.
    currentMinute = myTZ.minute();   // Ottiene i minuti.
    currentSecond = myTZ.second();   // Ottiene i secondi.
    lastUpdate = currentMillis;    // Aggiorna il timestamp dell'ultimo aggiornamento.

    // NOTA: Luminosità gestita dal radar LD2410 - nessun controllo manuale necessario
  } else if (currentMillis - lastUpdate > 1000) {
    // Senza WiFi, incrementa manualmente i secondi (per un orologio di fallback)
    uint8_t oldSecond = currentSecond;
    currentSecond++;
    if (currentSecond >= 60) {
      currentSecond = 0;
      currentMinute++;
      if (currentMinute >= 60) {
        currentMinute = 0;
        currentHour = (currentHour + 1) % 24; // Incrementa l'ora (0-23).
      }
    }

    // Regola la luminosità anche in modalità offline (solo se radar remoto NON attivo)
    #ifdef AUDIO
    if (!radarServerEnabled || radarRemotePresence) {
      // Solo se radar server disabilitato o c'è presenza
      if(checkIsNightTime(currentHour, currentMinute)) {
        ledcWrite(PWM_CHANNEL, brightnessNight);
      } else {
        ledcWrite(PWM_CHANNEL, brightnessDay);
      }
    }
    #endif

    lastUpdate = currentMillis;
  }

  // ===== CONTROLLO TRIGGER CAPODANNO (PRIORITÀ ASSOLUTA) =====
  // Controlla se è 01/01 alle 00:00 per attivare l'effetto speciale
  checkCapodannoTrigger();

  // Se l'effetto Capodanno è attivo, sovrasta TUTTO e non fa altro
  if (capodannoActive) {
    if (currentMillis - lastEffectUpdate > 30) {  // Aggiorna ogni 30ms (~33 FPS)
      updateCapodanno();
      lastEffectUpdate = currentMillis;
    }
    return;  // Esce dal loop senza eseguire altro
  }

  // ===== GESTIONE RAINBOW MODE =====
  // Aggiorna il colore rainbow (ciclo colori) se abilitato
  // NON blocca gli effetti, aggiorna solo currentColor
  extern bool rainbowModeEnabled;
  if (rainbowModeEnabled) {
    updateRainbowColor(currentMillis);  // Aggiorna solo il colore, non ridisegna
  }

  // ===== GESTIONE COLOR CYCLE (ciclaggio colori touch basso-sinistra) =====
  // Aggiorna il colore quando l'utente tiene premuto l'angolo basso-sinistra in WordClock
  if (colorCycle.isActive) {
    updateColorCycling(currentMillis);
  }

  if (!setupPageActive && !resetCountdownStarted) {
    // Gestione del display in base alla modalità corrente (se non è attiva la pagina di setup e non è iniziato un reset)
    if (alexaOff == 0) {  // Se il display non è stato spento tramite Alexa.

      // Reset flag inizializzazione quando si esce da una modalità
      #ifdef EFFECT_WEATHER_STATION
      if (currentMode != MODE_WEATHER_STATION) {
        weatherStationInitialized = false;  // Reset per reinizializzare al prossimo ingresso
      }
      #endif

      #ifdef EFFECT_CALENDAR
      if (currentMode != MODE_CALENDAR) {
        calendarStationInitialized = false;  // Reset per reinizializzare al prossimo ingresso
        calendarGridView = true;  // Torna a vista griglia al prossimo ingresso
      }
      #endif

      // Se allarme gas o calendario attivo, non aggiornare display (mantieni schermata allarme)
      if (!gasAlarmActive
      #ifdef EFFECT_CALENDAR
          && !calendarAlarmActive
      #endif
      ) {

      switch (currentMode) {
        case MODE_MATRIX:
          if (currentMillis - lastEffectUpdate > 20) {
            updateMatrix();       // Aggiorna l'effetto "Matrix".
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();  // Gestisce il lampeggio della lettera 'E' (se abilitato).
          break;
          
        case MODE_MATRIX2:
          if (currentMillis - lastEffectUpdate > 20) {
            updateMatrix2();      // Aggiorna la seconda variante dell'effetto "Matrix".
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();
          break;
          
        case MODE_SNAKE:
          if (currentMillis - lastEffectUpdate > 20) {
            updateSnake();        // Aggiorna l'effetto "Snake".
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();
          break;
          
        case MODE_WATER:
          if (currentMillis - lastEffectUpdate > 20) {
            updateWaterDrop();    // Aggiorna l'effetto "Goccia d'acqua".
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();
          break;

        case MODE_MARIO:
          if (currentMillis - lastEffectUpdate > 20) {
            updateMarioMode();    // Aggiorna l'effetto "Mario Bros".
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();
          break;

        case MODE_TRON:
          if (currentMillis - lastEffectUpdate > 20) {
            updateTron();         // Aggiorna l'effetto "Tron".
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();
          break;

#ifdef EFFECT_GALAGA
        case MODE_GALAGA:
          if (currentMillis - lastEffectUpdate > 20) {
            updateGalagaMode();   // Aggiorna l'effetto "Galaga".
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();
          break;
#endif

#ifdef EFFECT_GALAGA2
        case MODE_GALAGA2:
          if (currentMillis - lastEffectUpdate > 50) {
            updateGalagaMode2();   // Aggiorna l'effetto "Galaga2" (astronave volante).
            lastEffectUpdate = currentMillis;
          }
          blinkWord_E();
          break;
#endif

#ifdef EFFECT_ANALOG_CLOCK
        case MODE_ANALOG_CLOCK: {
          // Intervallo di aggiornamento: 33ms (~30fps) se smooth seconds attivo, altrimenti 100ms
          unsigned long analogClockInterval = clockSmoothSeconds ? 33 : 100;
          if (currentMillis - lastEffectUpdate > analogClockInterval) {
            updateAnalogClock();   // Aggiorna l'orologio analogico.
            lastEffectUpdate = currentMillis;
          }
          // Non c'è blinkWord_E perché l'orologio analogico non mostra le lettere
          break;
        }
#endif

#ifdef EFFECT_FLIP_CLOCK
        case MODE_FLIP_CLOCK:
          if (!flipClockInitialized) {
            initFlipClock();
          }
          updateFlipClock();      // Aggiorna orologio a palette flip.
          break;
#endif

#ifdef EFFECT_BTTF
        case MODE_BTTF:
          if (currentMillis - lastEffectUpdate > 1000) {  // Aggiorna ogni secondo
            updateBTTF();          // Aggiorna quadrante DeLorean Ritorno al Futuro.
            lastEffectUpdate = currentMillis;
          }
          // Non c'è blinkWord_E perché il quadrante BTTF non usa le parole
          break;
#endif

#ifdef EFFECT_LED_RING
        case MODE_LED_RING:
          if (currentMillis - lastEffectUpdate > 200) {  // Aggiorna frequentemente per fluidità
            updateLedRingClock();  // Aggiorna orologio LED Ring.
            lastEffectUpdate = currentMillis;
          }
          // Non usa blinkWord_E perché ha il proprio display digitale
          break;
#endif

#ifdef EFFECT_WEATHER_STATION
        case MODE_WEATHER_STATION:
          // Prima volta o ritorno alla modalità: inizializza tutto
          if (!weatherStationInitialized) {
            initWeatherStation();  // Disegna sfondo, bordi, labels e dati
          }
          updateWeatherStation();  // Aggiorna solo i dati che cambiano
          break;
#endif

#ifdef EFFECT_CALENDAR
        case MODE_CALENDAR:
          // Prima volta o ritorno alla modalità: inizializza tutto
          if (!calendarStationInitialized) {
            initCalendarStation();  // Disegna sfondo, bordi, labels e dati
          }
          updateCalendarStation();  // Aggiorna solo i dati che cambiano
          break;
#endif

#ifdef EFFECT_GEMINI_AI
        case MODE_GEMINI_AI:
          displayGeminiMode();  // Aggiorna interfaccia Gemini AI con scrolling.
          // Non usa blinkWord_E perché ha la propria interfaccia completa
          break;
#endif

#ifdef EFFECT_CLOCK
        case MODE_CLOCK:
          updateClockMode();  // Aggiorna orologio analogico con skin e subdial meteo.
          break;
#endif

#ifdef EFFECT_MJPEG_STREAM
        case MODE_MJPEG_STREAM:
          updateMjpegStream();  // Aggiorna streaming video MJPEG via WiFi.
          // Non usa blinkWord_E perché mostra video fullscreen
          break;
#endif

#ifdef EFFECT_ESP32CAM
        case MODE_ESP32CAM:
          updateEsp32camStream();  // Aggiorna streaming video da ESP32-CAM Freenove.
          // Non usa blinkWord_E perché mostra video fullscreen
          break;
#endif

#ifdef EFFECT_FLUX_CAPACITOR
        case MODE_FLUX_CAPACITOR:
          updateFluxCapacitor();  // Aggiorna animazione Flux Capacitor.
          // Non usa blinkWord_E perché ha la propria animazione fullscreen
          break;
#endif

#ifdef EFFECT_CHRISTMAS
        case MODE_CHRISTMAS:
          updateChristmas();  // Aggiorna tema natalizio con neve e albero.
          // Non usa blinkWord_E perché ha la propria animazione fullscreen
          break;
#endif

#ifdef EFFECT_FIRE
        case MODE_FIRE:
          updateFire();  // Aggiorna effetto fuoco camino.
          // Non usa blinkWord_E perché ha la propria animazione fullscreen
          break;
#endif

#ifdef EFFECT_FIRE_TEXT
        case MODE_FIRE_TEXT:
          updateFireText();  // Aggiorna effetto lettere fiammeggianti.
          // Non usa blinkWord_E perché ha la propria animazione
          break;
#endif

#ifdef EFFECT_MP3_PLAYER
        case MODE_MP3_PLAYER:
          updateMP3Player();  // Gestisce il lettore MP3/WAV
          break;
#endif

#ifdef EFFECT_WEB_RADIO
        case MODE_WEB_RADIO:
          updateWebRadioUI();  // Gestisce interfaccia Web Radio
          break;
#endif

#ifdef EFFECT_RADIO_ALARM
        case MODE_RADIO_ALARM:
          updateRadioAlarm();  // Gestisce interfaccia Radiosveglia
          break;
#endif

#ifdef EFFECT_WEB_TV
        case MODE_WEB_TV:
          updateWebTV();  // Gestisce interfaccia Web TV streaming
          break;
#endif

        case MODE_FADE:
          if (currentMillis - lastEffectUpdate > 20) {
            updateFadeMode();     // Aggiorna l'animazione di dissolvenza.
            lastEffectUpdate = currentMillis;           
          }
          blinkWord_E();                    
          break;
          
        case MODE_SLOW:
          if (currentMillis - lastEffectUpdate > 20) {
            updateSlowMode();     // Aggiorna la modalità "Slow".
            lastEffectUpdate = currentMillis;           
          }
          blinkWord_E();                    
          break;
          
        case MODE_FAST:
          // Aggiorna la modalità "Fast" solo al cambio di ora o minuto
          if (lastHour != currentHour || lastMinute != currentMinute) {
            updateFastMode();     // Aggiorna la visualizzazione veloce dell'ora.
          } else {
            blinkWord_E();
          }
          break;
      }
      } // Fine if (!gasAlarmActive)
    }
  }

  // ========== INDICATORE LED ALLARME GLOBALE ==========
  // Disegna il LED rosso in alto a destra se un allarme è attivo
  // Chiamato qui per funzionare in TUTTE le modalità, ogni ciclo del loop
  drawAlarmIndicator();

  // ========== AGGIORNAMENTO LED RGB WS2812 ==========
  // Aggiorna colore anello LED in base al tema/modalità corrente
  #ifdef EFFECT_LED_RGB
  updateLedRgb();
  #endif

  // Permette al watchdog timer dell'ESP32 di non resettare il dispositivo
  yield();
}

// GESTIONE LETTERA E con lampeggio basato sui secondi
void blinkWord_E(){
  if (currentMinute > 0) { // Lampeggia 'E' solo se ci sono i minuti.
    static bool E_on = false;
    static bool E_off = false;     
    uint8_t pos = 116; // Posizione della lettera 'E' nella mappa delle parole.
    // E Blink
    if (word_E_state == 1 ) { // Se lo stato di 'E' è impostato per il lampeggio.
      if (currentSecond % 2 == 0) { // Se il secondo è pari.         
        E_off = false;
        if(!E_on){
          E_on = true;    
          gfx->setTextColor(convertColor(currentColor));       
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));             
        }                  
      } else { // Se il secondo è dispari.
        E_on = false;
        if(!E_on){
          E_off = true;
          gfx->setTextColor(convertColor(TextBackColor));
          gfx->setCursor(pgm_read_word(&TFT_X[pos]), pgm_read_word(&TFT_Y[pos]));
          gfx->write(pgm_read_byte(&TFT_L[pos]));
        }
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

// Funzione per verificare se tutti e quattro gli angoli del touch screen sono premuti all'avvio
bool checkCornersAtBoot() {
  // Leggi lo stato del touch screen
  ts.read();
  
  // Se non ci sono almeno 4 tocchi, ritorna falso
  if (ts.touches < 4) return false;
  
  // Variabili per tenere traccia se ogni angolo è stato toccato
  bool tl = false, tr = false, bl = false, br = false; // Top-Left, Top-Right, Bottom-Left, Bottom-Right
  
  // Controlla tutti i punti di contatto rilevati
  for (int i = 0; i < ts.touches && i < 10; i++) {
    // Mappa le coordinate del touch screen alle coordinate del display (0-479)
    int x = map(ts.points[i].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
    int y = map(ts.points[i].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);
    
    // Verifica se il punto di contatto rientra in una delle zone degli angoli (con un margine di 100 pixel)
    if (x < 100 && y < 100) tl = true;
    else if (x > 380 && y < 100) tr = true;
    else if (x < 100 && y > 380) bl = true;
    else if (x > 380 && y > 380) br = true;
  }
  
  // Ritorna vero solo se tutti e quattro gli angoli sono stati toccati contemporaneamente
  return (tl && tr && bl && br);
}

/////////////////////////////////////////////////MENU///////////////////////////////////////////////// 
