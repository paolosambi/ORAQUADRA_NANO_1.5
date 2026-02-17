// ================== LED RGB WS2812 ==================
// Anello WS2812 con 12 LED RGB collegato al GPIO9
// Cambia colore automaticamente in base al tema/modalità display attivo
// Controllo on/off, luminosità e override colore dalla pagina web /ledrgb

#ifdef EFFECT_LED_RGB

#include <Adafruit_NeoPixel.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "soc/gpio_sig_map.h"

// ===== Configurazione hardware =====
#define WS2812_PIN        43  // GPIO43 - su ESP32-S3 Serial usa USB-CDC, non UART0, quindi GPIO43 e' libero
#define WS2812_NUM_LEDS   12

// ===== EEPROM addresses (900-907) =====
// NOTA: 700-704 sono usati da enabledModesMask e rainbowMode!
#define EEPROM_LED_RGB_ENABLED    900
#define EEPROM_LED_RGB_BRIGHTNESS 901
#define EEPROM_LED_RGB_OVERRIDE   902
#define EEPROM_LED_RGB_R          903
#define EEPROM_LED_RGB_G          904
#define EEPROM_LED_RGB_B          905
#define EEPROM_LED_RGB_MARKER     906
#define EEPROM_LED_RGB_VALID      0xAC  // Cambiato per forzare reset default
#define EEPROM_LED_RGB_AUDIOREACT 907

// ===== Oggetto NeoPixel =====
Adafruit_NeoPixel ledStrip(WS2812_NUM_LEDS, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// ===== Variabili globali =====
bool    ledRgbEnabled       = true;
uint8_t ledRgbBrightness    = 80;
bool    ledRgbOverride      = false;
uint8_t ledRgbOverrideR     = 255;
uint8_t ledRgbOverrideG     = 255;
uint8_t ledRgbOverrideB     = 255;

// Throttle: aggiorna LED ogni 100ms (non serve di più)
static unsigned long lastLedUpdate = 0;
#define LED_UPDATE_INTERVAL 100

// ===== Audio reactive (lampeggio a tempo) =====
// 0 = Disattivato, 1 = Pulsazione (solo LED accesi), 2 = Accendi con audio (anche LED spenti)
uint8_t ledAudioReactive = 1;       // Default: pulsazione
static uint8_t ledAudioTarget  = 0; // Target livello simulato
static uint8_t ledAudioSmooth  = 0; // Livello smoothed
static uint8_t ledAudioLevel   = 0; // Livello finale 0-100
#define LED_AUDIO_ATTACK  12   // Velocità salita
#define LED_AUDIO_DECAY    4   // Velocità discesa
#define LED_AUDIO_MIN_BRI 30   // Luminosità minima durante audio (% della brightness impostata)

// ===== Carica impostazioni da EEPROM =====
void loadLedRgbSettings() {
  if (EEPROM.read(EEPROM_LED_RGB_MARKER) == EEPROM_LED_RGB_VALID) {
    ledRgbEnabled    = EEPROM.read(EEPROM_LED_RGB_ENABLED) != 0;
    ledRgbBrightness = EEPROM.read(EEPROM_LED_RGB_BRIGHTNESS);
    ledRgbOverride   = EEPROM.read(EEPROM_LED_RGB_OVERRIDE) != 0;
    ledRgbOverrideR  = EEPROM.read(EEPROM_LED_RGB_R);
    ledRgbOverrideG  = EEPROM.read(EEPROM_LED_RGB_G);
    ledRgbOverrideB  = EEPROM.read(EEPROM_LED_RGB_B);
    ledAudioReactive = EEPROM.read(EEPROM_LED_RGB_AUDIOREACT);
    if (ledAudioReactive > 2) ledAudioReactive = 1; // Sanitize
    Serial.printf("[LED RGB] Impostazioni caricate: enabled=%d, brightness=%d, override=%d, R=%d G=%d B=%d, audioReactive=%d\n",
                  ledRgbEnabled, ledRgbBrightness, ledRgbOverride, ledRgbOverrideR, ledRgbOverrideG, ledRgbOverrideB, ledAudioReactive);
  } else {
    Serial.println("[LED RGB] Nessuna impostazione salvata, uso valori predefiniti");
    saveLedRgbSettings(); // Salva i default
  }
}

// ===== Salva impostazioni in EEPROM =====
void saveLedRgbSettings() {
  EEPROM.write(EEPROM_LED_RGB_ENABLED, ledRgbEnabled ? 1 : 0);
  EEPROM.write(EEPROM_LED_RGB_BRIGHTNESS, ledRgbBrightness);
  EEPROM.write(EEPROM_LED_RGB_OVERRIDE, ledRgbOverride ? 1 : 0);
  EEPROM.write(EEPROM_LED_RGB_R, ledRgbOverrideR);
  EEPROM.write(EEPROM_LED_RGB_G, ledRgbOverrideG);
  EEPROM.write(EEPROM_LED_RGB_B, ledRgbOverrideB);
  EEPROM.write(EEPROM_LED_RGB_AUDIOREACT, ledAudioReactive);
  EEPROM.write(EEPROM_LED_RGB_MARKER, EEPROM_LED_RGB_VALID);
  EEPROM.commit();
  Serial.println("[LED RGB] Impostazioni salvate in EEPROM");
}

// ===== Setup LED RGB =====
void setup_led_rgb() {
  // === GPIO43 è UART0 TX (IOMUX) su ESP32-S3 ===
  // Con USB CDC Off, Serial usa UART0 su GPIO43.
  // Disconnettiamo SOLO il segnale TX dal pin (NON il driver UART0,
  // altrimenti Serial.print() crasherebbe).
  // Il driver UART0 continua a funzionare: il TX FIFO drena normalmente
  // anche senza pin connesso, quindi Serial.print() non si blocca.
  // L'output seriale testuale va perso ma l'ESP32 resta stabile.

  // Reset IOMUX + GPIO matrix: libera GPIO43 dalla funzione UART0 TX
  gpio_reset_pin((gpio_num_t)WS2812_PIN);
  esp_rom_gpio_connect_out_signal(WS2812_PIN, SIG_GPIO_OUT_IDX, false, false);

  ledStrip.begin();
  ledStrip.clear();
  ledStrip.show();
  loadLedRgbSettings();
  ledStrip.setBrightness(ledRgbBrightness);
  Serial.printf("[LED RGB] Inizializzato - 12 LED WS2812 su GPIO43\n");
  Serial.printf("[LED RGB] Stato: enabled=%d, brightness=%d, override=%d, audioReactive=%d\n",
                ledRgbEnabled, ledRgbBrightness, ledRgbOverride, ledAudioReactive);
  if (ledRgbOverride) {
    Serial.printf("[LED RGB] ATTENZIONE: Override attivo! Colore fisso: R=%d G=%d B=%d\n",
                  ledRgbOverrideR, ledRgbOverrideG, ledRgbOverrideB);
  }

  // LED spenti al boot - verranno accesi dal loop con updateLedRgb()
  // dopo che il display è pronto e il colore tema è corretto
  Serial.println("[LED RGB] Boot: LED spenti, accensione gestita dal loop");
}

// ===== Colore per modalità =====
// Ritorna il colore tema per la modalità corrente
// Per i modi che usano currentColor, converte dal Color struct
void getLedColorForMode(DisplayMode mode, uint8_t &r, uint8_t &g, uint8_t &b) {
  switch (mode) {
    // Modi che seguono il colore utente
    case MODE_FADE:
    case MODE_SLOW:
    case MODE_FAST:
    case MODE_MATRIX:
    case MODE_MATRIX2:
    case MODE_SNAKE:
    case MODE_WATER:
      r = currentColor.r;
      g = currentColor.g;
      b = currentColor.b;
      break;

    // Mario - Rosso Mario
    case MODE_MARIO:
      r = 230; g = 30; b = 30;
      break;

    // Tron - Azzurro Tron
    case MODE_TRON:
      r = 0; g = 150; b = 255;
      break;

#ifdef EFFECT_GALAGA
    case MODE_GALAGA:
      r = 0; g = 100; b = 255;
      break;
#endif

#ifdef EFFECT_GALAGA2
    case MODE_GALAGA2:
      r = 0; g = 100; b = 255;
      break;
#endif

#ifdef EFFECT_ANALOG_CLOCK
    case MODE_ANALOG_CLOCK:
      r = 255; g = 200; b = 100;
      break;
#endif

#ifdef EFFECT_FLIP_CLOCK
    case MODE_FLIP_CLOCK:
      r = 255; g = 180; b = 50;
      break;
#endif

#ifdef EFFECT_BTTF
    case MODE_BTTF:
      r = 0; g = 255; b = 0;
      break;
#endif

#ifdef EFFECT_LED_RING
    case MODE_LED_RING:
      r = 0; g = 200; b = 255;
      break;
#endif

#ifdef EFFECT_WEATHER_STATION
    case MODE_WEATHER_STATION: {
      // Colore dinamico basato sulle condizioni meteo attuali
      // Palette colori da MeteoVision con distinzione giorno/notte
      extern int weatherCode;
      extern bool weatherDataValid;
      extern String weatherIcon;
      if (weatherDataValid && weatherCode > 0) {
        bool isNight = weatherIcon.endsWith("n");
        if (weatherCode >= 200 && weatherCode <= 299) {
          // Temporale → viola
          r = 197; g = 0; b = 255;
        } else if (weatherCode >= 300 && weatherCode <= 399) {
          // Pioggerella → blu
          r = 0; g = 0; b = 255;
        } else if (weatherCode >= 500 && weatherCode <= 599) {
          // Pioggia → ciano
          r = 0; g = 180; b = 255;
        } else if (weatherCode >= 600 && weatherCode <= 699) {
          // Neve → ciano
          r = 0; g = 180; b = 255;
        } else if (weatherCode >= 700 && weatherCode <= 799) {
          // Nebbia / foschia → blu
          r = 0; g = 0; b = 255;
        } else if (weatherCode == 800) {
          // Cielo sereno
          if (isNight) {
            r = 128; g = 128; b = 128;  // Notte → grigio luna
          } else {
            r = 255; g = 227; b = 0;    // Giorno → giallo sole
          }
        } else if (weatherCode >= 801 && weatherCode <= 804) {
          // Nuvoloso
          if (isNight) {
            r = 128; g = 128; b = 128;  // Notte → grigio luna
          } else {
            r = 255; g = 227; b = 0;    // Giorno → giallo sole
          }
        } else {
          r = 100; g = 180; b = 255;
        }
      } else {
        // Dati meteo non disponibili → azzurro meteo default
        r = 100; g = 180; b = 255;
      }
      break;
    }
#endif

#ifdef EFFECT_CLOCK
    case MODE_CLOCK:
      r = 255; g = 220; b = 150;
      break;
#endif

#ifdef EFFECT_ESP32CAM
    case MODE_ESP32CAM:
      r = 200; g = 200; b = 200;
      break;
#endif

#ifdef EFFECT_FLUX_CAPACITOR
    case MODE_FLUX_CAPACITOR:
      r = 255; g = 160; b = 0;
      break;
#endif

#ifdef EFFECT_CHRISTMAS
    case MODE_CHRISTMAS:
      // Caso speciale: gestito direttamente in updateLedRgb()
      // Qui ritorniamo rosso come fallback
      r = 255; g = 0; b = 0;
      break;
#endif

#ifdef EFFECT_FIRE
    case MODE_FIRE:
      r = 255; g = 80; b = 0;
      break;
#endif

#ifdef EFFECT_FIRE_TEXT
    case MODE_FIRE_TEXT:
      r = 255; g = 60; b = 0;
      break;
#endif

#ifdef EFFECT_MP3_PLAYER
    case MODE_MP3_PLAYER:
      r = 150; g = 0; b = 255;
      break;
#endif

#ifdef EFFECT_WEB_RADIO
    case MODE_WEB_RADIO:
      r = 0; g = 200; b = 100;
      break;
#endif

#ifdef EFFECT_RADIO_ALARM
    case MODE_RADIO_ALARM:
      r = 255; g = 150; b = 0;
      break;
#endif

#ifdef EFFECT_CALENDAR
    case MODE_CALENDAR:
      r = 50; g = 120; b = 255;
      break;
#endif

#ifdef EFFECT_YOUTUBE
    case MODE_YOUTUBE:
      r = 255; g = 0; b = 0;  // Rosso YouTube
      break;
#endif

#ifdef EFFECT_NEWS
    case MODE_NEWS:
      r = 0; g = 180; b = 255;  // Azzurro news
      break;
#endif

#ifdef EFFECT_PONG
    case MODE_PONG:
      r = 255; g = 255; b = 255;  // Bianco classico Pong
      break;
#endif

    default:
      // Fallback: bianco caldo
      r = 255; g = 200; b = 100;
      break;
  }
}

// ===== Aggiornamento LED (chiamato nel loop) =====
static unsigned long lastLedDebug = 0;

void updateLedRgb() {
  unsigned long now = millis();
  if (now - lastLedUpdate < LED_UPDATE_INTERVAL) return;
  lastLedUpdate = now;

  // Debug periodico ogni 5 secondi
  if (now - lastLedDebug > 5000) {
    lastLedDebug = now;
    uint8_t dr, dg, db;
    getLedColorForMode((DisplayMode)currentMode, dr, dg, db);
    Serial.printf("[LED RGB] mode=%d, enabled=%d, override=%d, theme=(%d,%d,%d), overrideColor=(%d,%d,%d)\n",
                  currentMode, ledRgbEnabled, ledRgbOverride, dr, dg, db,
                  ledRgbOverrideR, ledRgbOverrideG, ledRgbOverrideB);
  }

  // Audio reactive solo in modalità audio (Radio, MP3, Radio Alarm)
  bool isAudioMode = false;
#ifdef EFFECT_MP3_PLAYER
  if ((DisplayMode)currentMode == MODE_MP3_PLAYER) isAudioMode = true;
#endif
#ifdef EFFECT_WEB_RADIO
  if ((DisplayMode)currentMode == MODE_WEB_RADIO) isAudioMode = true;
#endif
#ifdef EFFECT_RADIO_ALARM
  if ((DisplayMode)currentMode == MODE_RADIO_ALARM) isAudioMode = true;
#endif

  // Mode 2: LED controllati solo dall'audio (solo in modalità audio)
  if (ledAudioReactive == 2 && isAudioMode) {
    if (!audio.isRunning()) {
      ledStrip.clear();
      ledStrip.show();
      return;
    }
    // Audio attivo → continua per accendere e pulsare
  }
  // Mode 0 e 1, oppure mode 2 fuori da modalità audio: rispetta toggle on/off
  else if (!ledRgbEnabled) {
    ledStrip.clear();
    ledStrip.show();
    return;
  }

  // Se radar attivo e nessuna presenza, spegni LED
  {
    bool noPresence = false;
    if (radarServerEnabled) {
      // Radar remoto
      noPresence = !radarRemotePresence;
    } else if (radarConnectedOnce) {
      // Radar locale
      noPresence = !presenceDetected;
    }
    if (noPresence) {
      ledStrip.clear();
      ledStrip.show();
      return;
    }
  }

  // ===== Calcolo luminosità con audio reactive =====
  uint8_t activeBrightness = ledRgbBrightness;

  if (ledAudioReactive > 0 && isAudioMode && audio.isRunning()) {
    // Simula livello audio con variazione pseudo-casuale (come VU meter)
    if (random(100) < 35) {
      ledAudioTarget = random(30, 100);
    }
    // Smooth attack/decay
    if (ledAudioSmooth < ledAudioTarget) {
      int v = ledAudioSmooth + LED_AUDIO_ATTACK;
      ledAudioSmooth = (v > ledAudioTarget) ? ledAudioTarget : (uint8_t)v;
    } else {
      int v = ledAudioSmooth - LED_AUDIO_DECAY;
      ledAudioSmooth = (v < (int)ledAudioTarget) ? ledAudioTarget : (uint8_t)v;
    }
    ledAudioLevel = ledAudioSmooth + random(-4, 5);
    if (ledAudioLevel > 100) ledAudioLevel = 100;
    if (ledAudioLevel < 5) ledAudioLevel = 5;

    // Modula brightness: da LED_AUDIO_MIN_BRI% a 100% della brightness impostata
    uint16_t minBri = ((uint16_t)ledRgbBrightness * LED_AUDIO_MIN_BRI) / 100;
    uint16_t range  = ledRgbBrightness - minBri;
    activeBrightness = minBri + (uint8_t)(((uint16_t)range * ledAudioLevel) / 100);
  } else {
    // Reset livello quando audio non attivo
    ledAudioSmooth = 0;
    ledAudioTarget = 0;
    ledAudioLevel  = 0;
  }

  ledStrip.setBrightness(activeBrightness);

  // ===== Calcolo colore =====
  if (ledRgbOverride) {
    // Override: usa colore personalizzato per tutti i LED
    uint32_t color = ledStrip.Color(ledRgbOverrideR, ledRgbOverrideG, ledRgbOverrideB);
    for (int i = 0; i < WS2812_NUM_LEDS; i++) {
      ledStrip.setPixelColor(i, color);
    }
  }
#ifdef EFFECT_CHRISTMAS
  else if ((DisplayMode)currentMode == MODE_CHRISTMAS) {
    // Caso speciale: 6 LED rossi alternati a 6 LED verdi
    uint32_t red   = ledStrip.Color(255, 0, 0);
    uint32_t green = ledStrip.Color(0, 255, 0);
    for (int i = 0; i < WS2812_NUM_LEDS; i++) {
      ledStrip.setPixelColor(i, (i % 2 == 0) ? red : green);
    }
  }
#endif
  else {
    // Colore tema basato sulla modalità corrente
    uint8_t r, g, b;
    getLedColorForMode((DisplayMode)currentMode, r, g, b);
    uint32_t color = ledStrip.Color(r, g, b);
    for (int i = 0; i < WS2812_NUM_LEDS; i++) {
      ledStrip.setPixelColor(i, color);
    }
  }

  ledStrip.show();
}

#endif // EFFECT_LED_RGB
