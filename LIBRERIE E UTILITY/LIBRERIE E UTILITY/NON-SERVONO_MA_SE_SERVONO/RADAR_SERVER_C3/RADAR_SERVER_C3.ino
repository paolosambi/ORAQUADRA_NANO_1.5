// ============================================================================
// RADAR SERVER HLK-LD2410B - ESP32-C3 VERSION
// Server autonomo per gestione presenza e luminosita
// Controlla dispositivi client via HTTP
// ============================================================================
// Autore: Sambinello Paolo
// Hardware: ESP32-C3 + HLK-LD2410B + Sensore Luce (opzionale)
// ============================================================================

#include <WiFi.h>
#include <WiFiManager.h>  // WiFiManager by tzapu - https://github.com/tzapu/WiFiManager
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BME280.h>
#include <Adafruit_NeoPixel.h>
#include "MyLD2410.h"

// ========== CONFIGURAZIONE WIFI ==========
// WiFiManager gestisce automaticamente le credenziali WiFi
// Al primo avvio o se non riesce a connettersi, crea un Access Point
// chiamato "RadarServer-Setup" per la configurazione
#define WIFI_AP_NAME "RadarServer-Setup"
#define WIFI_PORTAL_TIMEOUT 180       // Timeout portale configurazione in secondi (3 minuti)

// Flag per forzare il reset WiFi (impostato via web o pulsante)
bool shouldResetWiFi = false;

// ========== CONFIGURAZIONE DISPOSITIVO ==========
const char* hostname = "radar-server";
#define DEBUG_MODE

// ========== DEBUG MACRO ==========
// Su ESP32-C3 USB-CDC può bloccare se non connessa, usiamo Serial con check
#ifdef DEBUG_MODE
  #define DEBUG_PRINT(x) if(Serial) Serial.print(x)
  #define DEBUG_PRINTLN(x) if(Serial) Serial.println(x)
  #define DEBUG_PRINTF(format, ...) if(Serial) Serial.printf(format, ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(format, ...)
#endif

// ========== CONFIGURAZIONE HARDWARE ESP32-C3 ==========
#define SERIAL_BAUD_RATE 115200
#define LD2410_BAUD_RATE 256000

// Pin Seriale per HLK-LD2410B (Serial1 su ESP32-C3)
// ESP32-C3 ha solo UART0 (USB/Debug) e UART1 (disponibile)
#define RADAR_RX_PIN 20  // GPIO20 - RX per radar (collega a TX del radar)
#define RADAR_TX_PIN 21 // GPIO21 - TX per radar (collega a RX del radar)

// Pin LED WS2812 di stato
#define WS2812_PIN 1      // GPIO1 - LED WS2812 esterno
#define WS2812_COUNT 1    // Numero di LED

// Pin impulso cambio presenza
#define PULSE_PIN 3  // GPIO3 - Impulso su cambio presenza
#define PULSE_DURATION_MS 2000  // Durata impulso 1 secondo

// Pin Relè (HIGH LEVEL TRIGGER - si attiva con HIGH)
#define RELAY_PIN 4  // GPIO4 - Relè
#define RELAY_ACTIVE HIGH   // HIGH = relè attivo (HIGH LEVEL TRIGGER)
#define RELAY_INACTIVE LOW  // LOW = relè disattivo

// Pin I2C per Display OLED (ESP32-C3)
#define I2C_SDA 8   // GPIO8 - SDA
#define I2C_SCL 9   // GPIO9 - SCL

// ========== CONFIGURAZIONE DISPLAY OLED ==========
#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define OLED_ADDR 0x3C
#define OLED_RESET -1

// Display OLED (senza mirror - orientamento normale)
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// Timing aggiornamento display
#define DISPLAY_UPDATE_MS 500
unsigned long lastDisplayUpdate = 0;
bool displayInitialized = false;
bool displayOn = true;  // Stato display (on/off)

// ========== CONFIGURAZIONE BME280 ==========
#define BME280_ADDR 0x76  // Indirizzo I2C BME280 (può essere 0x77)
Adafruit_BME280 bme;
bool bme280Initialized = false;
float bmeTemperature = 0.0;
float bmeHumidity = 0.0;
float bmePressure = 0.0;
// Offset calibrazione (salvati in memoria)
float tempOffset = 0.0;   // Offset temperatura (es. -1.5 per sottrarre 1.5°C)
float humOffset = 0.0;    // Offset umidità (es. +3.0 per aggiungere 3%)
#define BME280_UPDATE_MS 2000
unsigned long lastBME280Update = 0;

// ========== LED WS2812 ==========
Adafruit_NeoPixel statusLed(WS2812_COUNT, WS2812_PIN, NEO_GRB + NEO_KHZ800);
uint32_t currentLedColor = 0;
bool otaInProgress = false;
#define LED_UPDATE_MS 100  // Aggiorna più frequentemente
unsigned long lastLedUpdate = 0;

// Radar sensor serial configuration (Serial1 per ESP32-C3)
HardwareSerial radarSerial(1);  // UART1
#define ENHANCED_MODE

// ========== COSTANTI TIMING ==========
#define RADAR_CACHE_UPDATE_MS 50   // Più frequente per non perdere dati (dataLifespan=500ms)
#define PRESENCE_CHECK_MS 500
#define CLIENT_NOTIFY_MS 300  // Notifica ogni 300ms per reattività
#define LIGHT_SENSOR_UPDATE_MS 500
#define MAX_RADAR_RETRIES 5

// ========== OGGETTI GLOBALI ==========
WebServer server(80);
Preferences preferences;

#ifdef DEBUG_MODE
MyLD2410 sensor(radarSerial, true);
#else
MyLD2410 sensor(radarSerial);
#endif

// ========== STRUTTURA CACHE RADAR ==========
struct RadarCache {
  bool presenceDetected;
  bool movingTargetDetected;
  bool stationaryTargetDetected;
  bool radarOnline;
  int detectedDistance;
  int movingTargetDistance;
  int stationaryTargetDistance;
  int movingTargetSignal;
  int stationaryTargetSignal;
  String statusString;
  unsigned long lastUpdate;
} radarCache;

// ========== STRUTTURA CLIENT DEVICE ==========
#define MAX_CLIENTS 10
struct ClientDevice {
  String ip;
  String name;
  uint16_t port;  // Porta HTTP del client (default 80)
  bool active;
  bool notifyPresence;
  bool notifyBrightness;
  unsigned long lastSeen;
  bool online;              // Stato online/offline
  unsigned long lastPing;   // Timestamp ultimo ping
  int failedPings;          // Contatore ping falliti consecutivi
};
ClientDevice clients[MAX_CLIENTS];
int clientCount = 0;

// Configurazione ping e cleanup
#define PING_TIMEOUT_MS 1000        // Timeout ping 1 secondo
#define AUTO_PING_INTERVAL 60000    // Ping automatico ogni 60 secondi
#define MAX_FAILED_PINGS 3          // Rimuovi dopo 3 ping falliti
unsigned long lastAutoPing = 0;
bool autoPingEnabled = true;

// ========== VARIABILI STATO ==========
bool radarInitialized = false;
bool presenceDetected = false;
bool previousPresenceState = false;
unsigned long lastPresenceTime = 0;
unsigned long presenceTimeout = 10000;  // 10 secondi timeout assenza (come progetto funzionante)
bool manualPresenceOverride = false;   // Se true, ignora radar e usa presenceDetected manuale

// Isteresi presenza - evita oscillazioni spurie
int presenceConfirmCount = 0;          // Contatore letture consecutive con presenza
#define PRESENCE_CONFIRM_THRESHOLD 6   // Richiede 6 letture consecutive per confermare presenza (3 sec)

// Cooldown dopo cambio stato - evita oscillazioni rapide
unsigned long lastStateChangeTime = 0;
#define STATE_CHANGE_COOLDOWN_MS 30000  // 30 secondi di cooldown dopo ogni cambio stato (evita oscillazioni monitor)

// Filtro segnale minimo - ignora letture con segnale debole (rumore)
#define MIN_SIGNAL_THRESHOLD 15  // Segnale minimo per considerare presenza valida (0-100)

// Luminosita ambiente
int ambientLight = 0;
int minBrightness = 10;
int maxBrightness = 255;
int calculatedBrightness = 128;

// Controllo automatico
bool autoPresenceEnabled = true;
bool autoBrightnessEnabled = true;

// Timing
unsigned long lastRadarCacheUpdate = 0;
unsigned long lastPresenceCheck = 0;
unsigned long lastClientNotify = 0;
unsigned long lastLightSensorUpdate = 0;

// Impulso cambio presenza
bool pulseActive = false;
unsigned long pulseStartTime = 0;

// Stato Relè
bool relayState = false;        // Stato attuale relè (true = ON, false = OFF)
bool relayAutoMode = false;     // Se true, relè segue la presenza
bool relayInverted = false;     // Se true, relè ON quando assenza (es. luce notturna)
bool relayPulseMode = true;     // Se true, relè funziona come pulsante (2 sec su cambio presenza)
bool relayPulseActive = false;  // Impulso relè in corso
unsigned long relayPulseStartTime = 0;
#define RELAY_PULSE_DURATION_MS 2000  // Durata impulso relè: 2 secondi

// ========== FUNZIONI RELE ==========
void initRelay() {
  pinMode(RELAY_PIN, OUTPUT);
  // All'avvio, forza relè OFF - sarà l'auto mode a gestirlo dopo la prima lettura radar
  // LOW LEVEL TRIGGER: HIGH = OFF, LOW = ON
  relayState = false;
  digitalWrite(RELAY_PIN, RELAY_INACTIVE);
  DEBUG_PRINTF("Relay inizializzato su GPIO%d - Stato: OFF (attesa lettura radar)\n", RELAY_PIN);
}

void setRelay(bool state) {
  relayState = state;
  // LOW LEVEL TRIGGER: LOW = attivo, HIGH = disattivo
  digitalWrite(RELAY_PIN, relayState ? RELAY_ACTIVE : RELAY_INACTIVE);
  DEBUG_PRINTF("Relay: %s\n", relayState ? "ON" : "OFF");
}

void toggleRelay() {
  setRelay(!relayState);
}

// Avvia impulso relè (pulsante 2 secondi)
void startRelayPulse() {
  if (!relayPulseMode) return;  // Solo se pulse mode attivo

  setRelay(true);  // Attiva relè
  relayPulseActive = true;
  relayPulseStartTime = millis();
  DEBUG_PRINTLN("IMPULSO RELE PIN 10: ON (2 sec)");
}

// Aggiorna impulso relè (spegne dopo 2 secondi)
void updateRelayPulse() {
  if (!relayPulseActive) return;

  if (millis() - relayPulseStartTime >= RELAY_PULSE_DURATION_MS) {
    setRelay(false);  // Disattiva relè
    relayPulseActive = false;
    DEBUG_PRINTLN("IMPULSO RELE PIN 10: OFF");
  }
}

// Aggiorna relè in base alla presenza (se auto mode attivo)
void updateRelayAuto() {
  if (!relayAutoMode) return;
  if (relayPulseActive) return;  // Non interferire durante impulso

  bool targetState;
  if (relayInverted) {
    // Inverted: relè ON quando NON c'è presenza
    targetState = !presenceDetected;
  } else {
    // Normale: relè ON quando c'è presenza
    targetState = presenceDetected;
  }

  if (relayState != targetState) {
    setRelay(targetState);
    // Salva stato
    preferences.begin("radar", false);
    preferences.putBool("relayState", relayState);
    preferences.end();
  }
}

// ========== FUNZIONI RADAR ==========
void initRadar() {
  // Display: inizializzazione Radar
  if (displayInitialized) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Radar...");
    display.display();
  }

  // Inizializza Serial1 per ESP32-C3 con pin custom
  radarSerial.begin(LD2410_BAUD_RATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  delay(2000);

  int retries = 0;
  radarInitialized = false;

  while (retries < MAX_RADAR_RETRIES && !radarInitialized) {
    // Aggiorna display con tentativo
    if (displayInitialized) {
      // Pulisce la riga prima di scrivere per evitare sovrapposizioni
      display.fillRect(0, 10, OLED_WIDTH, 10, SSD1306_BLACK);
      display.setCursor(0, 10);
      display.print("Tentativo ");
      display.print(retries + 1);
      display.print("/");
      display.print(MAX_RADAR_RETRIES);
      display.display();
    }

    if (sensor.begin()) {
      radarInitialized = true;
      DEBUG_PRINTLN("Radar HLK-LD2410B inizializzato con successo");
    } else {
      retries++;
      DEBUG_PRINTF("Tentativo %d/%d inizializzazione radar\n", retries, MAX_RADAR_RETRIES);
      delay(1000);
    }
  }

  // Display: risultato
  if (displayInitialized) {
    display.setCursor(0, 20);
    display.println(radarInitialized ? "Radar OK" : "Radar ERRORE");
    display.display();
    delay(500);
  }

  if (!radarInitialized) {
    DEBUG_PRINTLN("ERRORE: Radar HLK-LD2410B non risponde");
    autoPresenceEnabled = false;
  }

#ifdef ENHANCED_MODE
  if (radarInitialized) {
    bool enhOk = sensor.enhancedMode();
    DEBUG_PRINTF("Radar Enhanced Mode: %s\n", enhOk ? "OK" : "ERRORE");

    // Verifica che sia effettivamente in enhanced mode
    delay(100);
    for (int i = 0; i < 10; i++) {
      sensor.check();
      delay(50);
    }
    DEBUG_PRINTF("Radar inEnhancedMode(): %d\n", sensor.inEnhancedMode());
  }
#endif

  // Attiva sensore luce integrato nel radar
  if (radarInitialized) {
    if (sensor.requestAuxConfig()) {
      DEBUG_PRINTLN("Sensore luce radar configurato");
      DEBUG_PRINTF("Luce iniziale: %d\n", sensor.getLightLevel());
    } else {
      DEBUG_PRINTLN("ERRORE: requestAuxConfig() fallito");
    }
  }

  // Inizializza cache
  radarCache.presenceDetected = false;
  radarCache.movingTargetDetected = false;
  radarCache.stationaryTargetDetected = false;
  radarCache.radarOnline = false;
  radarCache.detectedDistance = 0;
  radarCache.lastUpdate = 0;
}

void updateRadarCache() {
  unsigned long currentTime = millis();
  if (currentTime - lastRadarCacheUpdate < RADAR_CACHE_UPDATE_MS) return;

  // Aggiorna radar
  bool checkResult = sensor.check();

  // Debug periodico stato radar (ogni 3 secondi)
  static unsigned long lastRadarDebug = 0;
  static int dataCount = 0;
  if (checkResult) dataCount++;

  if (currentTime - lastRadarDebug > 3000) {
    if (Serial) Serial.printf("[RADAR] frames=%d enhanced=%d status=%d light=%d\n",
                  dataCount, sensor.inEnhancedMode(),
                  sensor.getStatus(), sensor.getLightLevel());
    dataCount = 0;
    lastRadarDebug = currentTime;
  }

  radarCache.presenceDetected = sensor.presenceDetected();
  radarCache.movingTargetDetected = sensor.movingTargetDetected();
  radarCache.stationaryTargetDetected = sensor.stationaryTargetDetected();
  radarCache.radarOnline = sensor.inEnhancedMode() || sensor.getFirmwareMajor() > 0;

  if (radarCache.presenceDetected) {
    radarCache.detectedDistance = sensor.detectedDistance();
  } else {
    radarCache.detectedDistance = 0;
  }

  if (radarCache.movingTargetDetected) {
    radarCache.movingTargetDistance = sensor.movingTargetDistance();
    radarCache.movingTargetSignal = sensor.movingTargetSignal();
  } else {
    radarCache.movingTargetDistance = 0;
    radarCache.movingTargetSignal = 0;
  }

  if (radarCache.stationaryTargetDetected) {
    radarCache.stationaryTargetDistance = sensor.stationaryTargetDistance();
    radarCache.stationaryTargetSignal = sensor.stationaryTargetSignal();
  } else {
    radarCache.stationaryTargetDistance = 0;
    radarCache.stationaryTargetSignal = 0;
  }

  radarCache.statusString = sensor.statusString();
  radarCache.lastUpdate = currentTime;
  lastRadarCacheUpdate = currentTime;
}

// ========== GESTIONE IMPULSO CAMBIO PRESENZA ==========
void startPulse() {
  digitalWrite(PULSE_PIN, HIGH);
  pulseActive = true;
  pulseStartTime = millis();
  DEBUG_PRINTLN("IMPULSO PIN 3: ON");
}

void updatePulse() {
  if (!pulseActive) return;

  if (millis() - pulseStartTime >= PULSE_DURATION_MS) {
    digitalWrite(PULSE_PIN, LOW);
    pulseActive = false;
    DEBUG_PRINTLN("IMPULSO PIN 3: OFF");
  }
}

// ========== GESTIONE PRESENZA ==========
void updatePresence() {
  // Se override manuale attivo, non aggiornare da radar
  if (manualPresenceOverride) {
    return;
  }

  if (!autoPresenceEnabled || !radarInitialized) {
    return;
  }

  unsigned long currentTime = millis();
  if (currentTime - lastPresenceCheck < PRESENCE_CHECK_MS) return;

  // Filtra letture con segnale debole (probabilmente rumore)
  bool movingValid = radarCache.movingTargetDetected &&
                     (radarCache.movingTargetSignal >= MIN_SIGNAL_THRESHOLD);
  bool stationaryValid = radarCache.stationaryTargetDetected &&
                         (radarCache.stationaryTargetSignal >= MIN_SIGNAL_THRESHOLD);

  bool currentPresence = movingValid || stationaryValid;

  // Debug stato ogni 3 secondi
  static unsigned long lastDebug = 0;
  if (currentTime - lastDebug > 3000) {
    unsigned long cooldownRemaining = 0;
    if ((currentTime - lastStateChangeTime) < STATE_CHANGE_COOLDOWN_MS) {
      cooldownRemaining = STATE_CHANGE_COOLDOWN_MS - (currentTime - lastStateChangeTime);
    }
    if (Serial) Serial.printf("[PRESENZA] det:%d curr:%d m:%d(%d) s:%d(%d) hyst:%d/%d cool:%lums\n",
                  presenceDetected,
                  currentPresence,
                  radarCache.movingTargetDetected, radarCache.movingTargetSignal,
                  radarCache.stationaryTargetDetected, radarCache.stationaryTargetSignal,
                  presenceConfirmCount, PRESENCE_CONFIRM_THRESHOLD,
                  cooldownRemaining);
    lastDebug = currentTime;
  }

  // Verifica cooldown - blocca cambi di stato troppo rapidi
  bool cooldownActive = (currentTime - lastStateChangeTime) < STATE_CHANGE_COOLDOWN_MS;

  if (currentPresence) {
    // Incrementa contatore isteresi (max al threshold per evitare overflow)
    if (presenceConfirmCount < PRESENCE_CONFIRM_THRESHOLD) {
      presenceConfirmCount++;
    }

    // Solo se abbiamo abbastanza letture consecutive E cooldown scaduto, conferma presenza
    if (presenceConfirmCount >= PRESENCE_CONFIRM_THRESHOLD) {
      lastPresenceTime = currentTime;

      // Notifica cambio stato - PRESENZA ARRIVATA (solo se non in cooldown)
      if (!previousPresenceState && !cooldownActive) {
        presenceDetected = true;
        lastStateChangeTime = currentTime;  // Avvia cooldown
        if (Serial) {
          Serial.println("========================================");
          Serial.println("[PRESENZA] >>> PRESENZA RILEVATA <<<");
          Serial.println("========================================");
        }
        notifyClients("presence", "true");
        startPulse();
        startRelayPulse();  // Impulso relè 2 sec su arrivo presenza
      } else if (!cooldownActive) {
        presenceDetected = true;
      }
    }
  } else {
    // Nessuna presenza rilevata - resetta contatore isteresi
    presenceConfirmCount = 0;

    // Timeout assenza (solo se non in cooldown)
    unsigned long elapsed = currentTime - lastPresenceTime;
    if (elapsed > presenceTimeout && !cooldownActive) {
      if (previousPresenceState) {
        lastStateChangeTime = currentTime;  // Avvia cooldown
        notifyClients("presence", "false");
        startPulse();
        startRelayPulse();  // Impulso relè per spegnere monitor
      }
      presenceDetected = false;
    }
  }

  previousPresenceState = presenceDetected;
  lastPresenceCheck = currentTime;
}

// ========== GESTIONE LUMINOSITA ==========
void updateLightSensor() {
  unsigned long currentTime = millis();
  if (currentTime - lastLightSensorUpdate < LIGHT_SENSOR_UPDATE_MS) return;

  // Leggi luce dal sensore integrato nel radar LD2410 (0-255)
  if (radarInitialized) {
    ambientLight = sensor.getLightLevel();
  }

  // Calcola luminosita consigliata (0-255)
  if (autoBrightnessEnabled) {
    // Mappa luce ambiente (0-255) su luminosita
    // Buio = brightness basso, Luce = brightness alto
    calculatedBrightness = map(ambientLight, 0, 255, minBrightness, maxBrightness);
    calculatedBrightness = constrain(calculatedBrightness, minBrightness, maxBrightness);
  }

  // Stampa luminosità su terminale
  #ifdef DEBUG_MODE
  static unsigned long lastLightPrint = 0;
  if (currentTime - lastLightPrint >= 1000) {
    DEBUG_PRINTF("LUCE: %d/255 | Brightness: %d\n", ambientLight, calculatedBrightness);
    lastLightPrint = currentTime;
  }
  #endif

  lastLightSensorUpdate = currentTime;
}

// ========== GESTIONE DISPLAY OLED ==========
void initDisplay() {
  // Inizializza I2C con pin custom per ESP32-C3
  Wire.begin(I2C_SDA, I2C_SCL);

  // Inizializza display OLED (senza mirror)
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    displayInitialized = true;
    DEBUG_PRINTLN("Display OLED inizializzato");

    // Schermata di avvio
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 4);
    display.println("RADAR SERVER");
    display.setCursor(35, 16);
    display.println("ESP32-C3");
    display.display();
    delay(1500);
  } else {
    displayInitialized = false;
    DEBUG_PRINTLN("ERRORE: Display OLED non trovato!");
  }
}

// ========== GESTIONE BME280 ==========
void initBME280() {
  // Display: inizializzazione BME280
  if (displayInitialized) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Sensore BME280...");
    display.display();
  }

  // Il BME280 usa lo stesso bus I2C già inizializzato per il display
  if (bme.begin(BME280_ADDR)) {
    bme280Initialized = true;
    DEBUG_PRINTLN("Sensore BME280 inizializzato");
    // Prima lettura
    bmeTemperature = bme.readTemperature();
    bmeHumidity = bme.readHumidity();
    DEBUG_PRINTF("Temp: %.1f C, Umidita: %.1f%%\n", bmeTemperature, bmeHumidity);

    // Display: risultato OK
    if (displayInitialized) {
      display.setCursor(0, 10);
      display.print("T:");
      display.print((int)bmeTemperature);
      display.print("C H:");
      display.print((int)bmeHumidity);
      display.println("%");
      display.display();
      delay(500);
    }
  } else {
    bme280Initialized = false;
    DEBUG_PRINTLN("ERRORE: Sensore BME280 non trovato! Prova indirizzo 0x77");

    // Display: non trovato (non è un errore critico)
    if (displayInitialized) {
      display.setCursor(0, 10);
      display.println("(non presente)");
      display.display();
      delay(300);
    }
  }
}

void updateBME280() {
  if (!bme280Initialized) return;

  unsigned long currentTime = millis();
  if (currentTime - lastBME280Update < BME280_UPDATE_MS) return;

  // Leggi valori raw e applica offset calibrazione
  bmeTemperature = bme.readTemperature() + tempOffset;
  bmeHumidity = bme.readHumidity() + humOffset;
  bmePressure = bme.readPressure() / 100.0F;  // Converti in hPa

  // Limita umidità tra 0-100%
  bmeHumidity = constrain(bmeHumidity, 0.0, 100.0);

  #ifdef DEBUG_MODE
  static unsigned long lastBMEPrint = 0;
  if (currentTime - lastBMEPrint >= 5000) {
    DEBUG_PRINTF("BME280 - Temp: %.1f C (offset: %.1f), Umidita: %.1f%% (offset: %.1f), Press: %.1f hPa\n",
                 bmeTemperature, tempOffset, bmeHumidity, humOffset, bmePressure);
    lastBMEPrint = currentTime;
  }
  #endif

  lastBME280Update = currentTime;
}

// ========== GESTIONE LED WS2812 ==========
void initStatusLed() {
  // Reset pin prima di inizializzare
  pinMode(WS2812_PIN, OUTPUT);
  digitalWrite(WS2812_PIN, LOW);
  delay(10);

  statusLed.begin();
  statusLed.setBrightness(100);  // Luminosità aumentata (0-255)
  statusLed.clear();
  statusLed.show();
  delay(50);

  // ARANCIO durante avvio
  statusLed.setPixelColor(0, statusLed.Color(255, 128, 0));
  statusLed.show();
  currentLedColor = statusLed.Color(255, 128, 0);
  DEBUG_PRINTLN("LED WS2812 inizializzato");
}

void setLedColor(uint32_t color) {
  currentLedColor = color;
  statusLed.setPixelColor(0, color);
  statusLed.show();
}

void updateStatusLed() {
  unsigned long currentTime = millis();
  if (currentTime - lastLedUpdate < LED_UPDATE_MS) return;

  // Priorità colori:
  // 1. OTA in corso -> CIANO lampeggiante
  // 2. NESSUNA PRESENZA -> ROSSO FISSO
  // 3. Impulso attivo -> VERDE
  // 4. WiFi disconnesso -> BLU
  // 5. Radar offline -> GIALLO
  // 6. BME280 errore -> VIOLA
  // 7. Presenza attiva (tutto OK) -> SPENTO

  uint32_t newColor = 0;  // Spento di default

  if (otaInProgress) {
    // Lampeggio ciano durante OTA
    static bool otaBlink = false;
    otaBlink = !otaBlink;
    newColor = otaBlink ? statusLed.Color(0, 255, 255) : 0;
  }
  else if (!presenceDetected) {
    // ROSSO FISSO quando non c'è presenza (priorità massima!)
    newColor = statusLed.Color(255, 0, 0);
  }
  else if (pulseActive) {
    // VERDE durante impulso cambio presenza
    newColor = statusLed.Color(0, 255, 0);
  }
  else if (WiFi.status() != WL_CONNECTED) {
    // BLU se WiFi disconnesso
    newColor = statusLed.Color(0, 0, 255);
  }
  else if (radarInitialized && !radarCache.radarOnline) {
    // GIALLO se radar offline
    newColor = statusLed.Color(255, 255, 0);
  }
  else if (!bme280Initialized) {
    // VIOLA se BME280 errore
    newColor = statusLed.Color(255, 0, 255);
  }
  // Altrimenti SPENTO (presenza attiva, tutto OK)

  // Aggiorna LED se cambiato
  if (newColor != currentLedColor) {
    setLedColor(newColor);
    if (Serial) {
      if (newColor == statusLed.Color(255, 0, 0)) {
        Serial.println("[LED] ROSSO (no presenza)");
      } else if (newColor == 0) {
        Serial.println("[LED] SPENTO (presenza OK)");
      } else if (newColor == statusLed.Color(0, 255, 0)) {
        Serial.println("[LED] VERDE (impulso)");
      }
    }
  }

  lastLedUpdate = currentTime;
}

void updateDisplay() {
  if (!displayInitialized) return;

  unsigned long currentTime = millis();
  if (currentTime - lastDisplayUpdate < DISPLAY_UPDATE_MS) return;

  // Spegni display se non c'è presenza
  if (!presenceDetected) {
    if (displayOn) {
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      displayOn = false;
      DEBUG_PRINTLN("Display SPENTO (no presenza)");
    }
    lastDisplayUpdate = currentTime;
    return;
  }

  // Riaccendi display se c'è presenza
  if (!displayOn) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    displayOn = true;
    DEBUG_PRINTLN("Display ACCESO (presenza rilevata)");
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Riga 1: Stato presenza
  display.setCursor(0, 0);
  display.print("PRESENZA ");
  display.print(radarCache.detectedDistance);
  display.print("cm");

  // Riga 2: Luminosità, Brightness e Client connessi
  display.setCursor(0, 12);
  display.print("L:");
  display.print(ambientLight);
  display.print(" B:");
  display.print(calculatedBrightness);
  // Conta client online
  int onlineClients = 0;
  for (int i = 0; i < clientCount; i++) {
    if (clients[i].active && clients[i].online) onlineClients++;
  }
  display.print(" C:");
  display.print(onlineClients);

  // Riga 3: IP Address
  display.setCursor(0, 24);
  if (WiFi.status() == WL_CONNECTED) {
    display.print(WiFi.localIP().toString());
  } else {
    display.print("No WiFi");
  }

  // Stato radar e wifi
  display.setCursor(100, 0);
  display.print("R:");
  display.print(radarCache.radarOnline ? "OK" : "NO");
  display.setCursor(100, 12);
  display.print("W:");
  display.print(WiFi.status() == WL_CONNECTED ? "OK" : "NO");

  // BME280: Temperatura e Umidità a (100, 24)
  display.setCursor(100, 24);
  if (bme280Initialized) {
    // Alterna tra temperatura e umidità ogni secondo
    if ((currentTime / 1000) % 2 == 0) {
      display.print((int)bmeTemperature);
      display.print("C");
    } else {
      display.print((int)bmeHumidity);
      display.print("%");
    }
  } else {
    display.print("--");
  }

  display.display();
  lastDisplayUpdate = currentTime;
}

// ========== GESTIONE CLIENT ==========
void addClient(String ip, String name, uint16_t port = 80) {
  // Cerca se esiste gia
  for (int i = 0; i < clientCount; i++) {
    if (clients[i].ip == ip) {
      clients[i].name = name;
      clients[i].port = port;
      clients[i].active = true;
      clients[i].lastSeen = millis();
      clients[i].online = true;       // Se si registra, è online
      clients[i].failedPings = 0;
      saveClients();
      return;
    }
  }

  // Aggiungi nuovo client
  if (clientCount < MAX_CLIENTS) {
    clients[clientCount].ip = ip;
    clients[clientCount].name = name;
    clients[clientCount].port = port;
    clients[clientCount].active = true;
    clients[clientCount].notifyPresence = true;
    clients[clientCount].notifyBrightness = true;
    clients[clientCount].lastSeen = millis();
    clients[clientCount].online = true;      // Nuovo client è online
    clients[clientCount].lastPing = millis();
    clients[clientCount].failedPings = 0;
    clientCount++;
    saveClients();
    DEBUG_PRINTF("Client aggiunto: %s (%s:%d)\n", name.c_str(), ip.c_str(), port);
  }
}

void removeClient(String ip) {
  for (int i = 0; i < clientCount; i++) {
    if (clients[i].ip == ip) {
      // Sposta gli altri client
      for (int j = i; j < clientCount - 1; j++) {
        clients[j] = clients[j + 1];
      }
      clientCount--;
      saveClients();
      DEBUG_PRINTF("Client rimosso: %s\n", ip.c_str());
      return;
    }
  }
}

void notifyClients(String event, String value) {
  if (clientCount == 0) {
    return;
  }

  for (int i = 0; i < clientCount; i++) {
    if (!clients[i].active) continue;

    // Controlla tipo di notifica
    if (event == "presence" && !clients[i].notifyPresence) continue;
    if (event == "brightness" && !clients[i].notifyBrightness) continue;
    // Temperatura e umidità seguono le stesse regole di brightness
    if (event == "temperature" && !clients[i].notifyBrightness) continue;
    if (event == "humidity" && !clients[i].notifyBrightness) continue;

    HTTPClient http;
    String url = "http://" + clients[i].ip + ":" + String(clients[i].port) + "/radar/" + event + "?value=" + value;

    DEBUG_PRINTF("NOTIFY -> %s:%d /%s=%s ", clients[i].ip.c_str(), clients[i].port, event.c_str(), value.c_str());

    http.begin(url);
    http.setTimeout(2000);
    int httpCode = http.GET();

    if (httpCode > 0) {
      clients[i].lastSeen = millis();
      DEBUG_PRINTF("OK(%d)\n", httpCode);
    } else {
      DEBUG_PRINTF("FAIL(%d)\n", httpCode);
    }

    http.end();
  }
}

void saveClients() {
  preferences.begin("radar", false);
  preferences.putInt("clientCount", clientCount);

  for (int i = 0; i < clientCount; i++) {
    String prefix = "c" + String(i);
    preferences.putString((prefix + "ip").c_str(), clients[i].ip);
    preferences.putString((prefix + "name").c_str(), clients[i].name);
    preferences.putUShort((prefix + "port").c_str(), clients[i].port);
    preferences.putBool((prefix + "pres").c_str(), clients[i].notifyPresence);
    preferences.putBool((prefix + "bri").c_str(), clients[i].notifyBrightness);
  }

  preferences.end();
}

void loadClients() {
  preferences.begin("radar", true);
  clientCount = preferences.getInt("clientCount", 0);

  for (int i = 0; i < clientCount && i < MAX_CLIENTS; i++) {
    String prefix = "c" + String(i);
    clients[i].ip = preferences.getString((prefix + "ip").c_str(), "");
    clients[i].name = preferences.getString((prefix + "name").c_str(), "Device");
    clients[i].port = preferences.getUShort((prefix + "port").c_str(), 80);  // Default 80
    clients[i].notifyPresence = preferences.getBool((prefix + "pres").c_str(), true);
    clients[i].notifyBrightness = preferences.getBool((prefix + "bri").c_str(), true);
    clients[i].active = true;
    clients[i].lastSeen = millis();
    // Inizializza campi online/ping (stato sconosciuto all'avvio)
    clients[i].online = false;       // Sconosciuto finché non verificato
    clients[i].lastPing = 0;
    clients[i].failedPings = 0;
  }

  preferences.end();
  DEBUG_PRINTF("Caricati %d client\n", clientCount);
}

// ========== mDNS AUTO-DISCOVERY ==========
unsigned long lastDiscoveryTime = 0;
unsigned long discoveryInterval = 300000;  // Scansione ogni 5 minuti (300000ms)
bool autoDiscoveryEnabled = true;

// Verifica device tramite HTTP per ottenere info
bool probeDevice(String ip, uint16_t port, String &deviceName) {
  HTTPClient http;
  String url = "http://" + ip + ":" + String(port) + "/api/status";

  http.begin(url);
  http.setTimeout(2000);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    // Cerca il nome nel JSON (semplice parsing)
    int nameIdx = payload.indexOf("\"name\"");
    if (nameIdx == -1) nameIdx = payload.indexOf("\"deviceName\"");
    if (nameIdx > 0) {
      int colonIdx = payload.indexOf(":", nameIdx);
      int quoteStart = payload.indexOf("\"", colonIdx);
      int quoteEnd = payload.indexOf("\"", quoteStart + 1);
      if (quoteStart > 0 && quoteEnd > quoteStart) {
        deviceName = payload.substring(quoteStart + 1, quoteEnd);
      }
    }
    http.end();
    return true;
  }

  // Prova endpoint alternativo
  http.end();
  url = "http://" + ip + ":" + String(port) + "/radar/test?value=probe";
  http.begin(url);
  http.setTimeout(2000);
  httpCode = http.GET();
  http.end();

  return (httpCode == 200);
}

// Scansiona la rete per device con servizio _radardevice._tcp
int discoverClients() {
  DEBUG_PRINTLN("\n[mDNS] ========== AVVIO DISCOVERY ==========");

  int found = 0;
  int added = 0;

  // Cerca servizi _radardevice._tcp
  DEBUG_PRINTLN("[mDNS] Cercando servizi _radardevice._tcp...");
  int n = MDNS.queryService("radardevice", "tcp");

  DEBUG_PRINTF("[mDNS] queryService ritornato: %d\n", n);

  if (n == 0) {
    DEBUG_PRINTLN("[mDNS] Nessun servizio mDNS trovato");
    DEBUG_PRINTLN("[mDNS] Assicurati che i device abbiano:");
    DEBUG_PRINTLN("[mDNS]   MDNS.addService(\"radardevice\", \"tcp\", port);");
    return 0;
  }

  DEBUG_PRINTF("[mDNS] Trovati %d servizi!\n", n);

  for (int i = 0; i < n; i++) {
    IPAddress deviceIP = MDNS.IP(i);
    String ip = deviceIP.toString();
    String hostname = MDNS.hostname(i);
    uint16_t port = MDNS.port(i);

    DEBUG_PRINTF("[mDNS] [%d] Host: %s, IP: %s, Porta: %d\n",
                 i, hostname.c_str(), ip.c_str(), port);

    // Salta IP invalidi
    if (ip == "0.0.0.0" || ip == "" || deviceIP == IPAddress(0,0,0,0)) {
      DEBUG_PRINTF("[mDNS] [%d] IP non valido, skippo\n", i);
      continue;
    }

    // Usa hostname come nome device, oppure prova a ottenere info via HTTP
    String deviceName = hostname;
    if (deviceName.length() == 0 || deviceName == "0") {
      deviceName = "Device_" + ip;
    }

    // Rimuovi .local dal hostname se presente
    deviceName.replace(".local", "");

    // Prova a verificare che il device risponda
    DEBUG_PRINTF("[mDNS] [%d] Verifico device %s:%d...\n", i, ip.c_str(), port);
    String probedName = deviceName;
    bool isAlive = probeDevice(ip, port, probedName);

    if (isAlive && probedName.length() > 0) {
      deviceName = probedName;
    }

    DEBUG_PRINTF("[mDNS] [%d] Device %s - Nome: %s\n",
                 i, isAlive ? "RISPONDE" : "non risponde", deviceName.c_str());

    // Verifica se il client è già registrato
    bool exists = false;
    for (int j = 0; j < clientCount; j++) {
      if (clients[j].ip == ip) {
        exists = true;
        // Aggiorna nome e porta se cambiati
        clients[j].name = deviceName;
        clients[j].port = port;
        clients[j].active = true;
        clients[j].lastSeen = millis();
        DEBUG_PRINTF("[mDNS] Client %s gia' registrato, aggiornato\n", ip.c_str());
        break;
      }
    }

    // Aggiungi nuovo client se non esiste
    if (!exists && clientCount < MAX_CLIENTS) {
      clients[clientCount].ip = ip;
      clients[clientCount].name = deviceName;
      clients[clientCount].port = port;
      clients[clientCount].active = true;
      clients[clientCount].notifyPresence = true;
      clients[clientCount].notifyBrightness = true;
      clients[clientCount].lastSeen = millis();
      clientCount++;
      added++;
      DEBUG_PRINTF("[mDNS] >>> NUOVO CLIENT: %s (%s:%d)\n",
                   deviceName.c_str(), ip.c_str(), port);
    }

    found++;
  }

  // Salva se sono stati aggiunti nuovi client
  if (added > 0) {
    saveClients();
    DEBUG_PRINTF("[mDNS] Salvati %d nuovi client\n", added);
  }

  DEBUG_PRINTLN("[mDNS] ========== DISCOVERY COMPLETATA ==========");
  DEBUG_PRINTF("[mDNS] Risultato: %d trovati, %d nuovi aggiunti\n", found, added);
  return added;
}

// Network scan - cerca device sulla sottorete locale provando IP comuni
int networkScan() {
  DEBUG_PRINTLN("\n[SCAN] ========== AVVIO NETWORK SCAN ==========");

  // Ottieni IP locale e subnet
  IPAddress localIP = WiFi.localIP();
  DEBUG_PRINTF("[SCAN] IP locale: %s\n", localIP.toString().c_str());

  // Scansiona la sottorete (es. 192.168.1.1 - 192.168.1.254)
  String baseIP = String(localIP[0]) + "." + String(localIP[1]) + "." + String(localIP[2]) + ".";

  int found = 0;
  int added = 0;
  int scanned = 0;

  // Porte da provare (80 e 8080 sono le più comuni)
  uint16_t ports[] = {80, 8080};
  int numPorts = 2;

  // Timeout connessione in millisecondi (50ms è sufficiente per rete locale)
  const int CONNECT_TIMEOUT_MS = 50;

  DEBUG_PRINTLN("[SCAN] Scansione rete in corso...");
  DEBUG_PRINTF("[SCAN] Timeout connessione: %dms per porta\n", CONNECT_TIMEOUT_MS);

  unsigned long startTime = millis();

  for (int host = 1; host <= 254; host++) {
    String ip = baseIP + String(host);

    // Salta il nostro IP
    if (ip == localIP.toString()) continue;

    // Salta IP già registrati
    bool alreadyRegistered = false;
    for (int j = 0; j < clientCount; j++) {
      if (clients[j].ip == ip) {
        alreadyRegistered = true;
        break;
      }
    }
    if (alreadyRegistered) continue;

    scanned++;

    // Prova le porte
    for (int p = 0; p < numPorts; p++) {
      uint16_t port = ports[p];

      // Prova connessione con timeout breve
      WiFiClient client;

      // ESP32: connect(host, port, timeout_ms)
      if (client.connect(ip.c_str(), port, CONNECT_TIMEOUT_MS)) {
        client.stop();

        // Verifica se è un device radar compatibile
        String deviceName = "Device_" + ip;
        if (probeDevice(ip, port, deviceName)) {
          DEBUG_PRINTF("[SCAN] TROVATO: %s:%d (%s)\n", ip.c_str(), port, deviceName.c_str());

          // Aggiungi client
          if (clientCount < MAX_CLIENTS) {
            clients[clientCount].ip = ip;
            clients[clientCount].name = deviceName;
            clients[clientCount].port = port;
            clients[clientCount].active = true;
            clients[clientCount].notifyPresence = true;
            clients[clientCount].notifyBrightness = true;
            clients[clientCount].lastSeen = millis();
            clientCount++;
            added++;
            found++;
          }
          break;  // Trovato su questa porta, passa al prossimo IP
        }
      }

      // Yield per non bloccare il watchdog
      yield();
    }

    // Feedback ogni 50 host
    if (host % 50 == 0) {
      unsigned long elapsed = (millis() - startTime) / 1000;
      DEBUG_PRINTF("[SCAN] Progresso: %d/254 (%ld sec)\n", host, elapsed);
    }
  }

  if (added > 0) {
    saveClients();
  }

  unsigned long totalTime = (millis() - startTime) / 1000;
  DEBUG_PRINTLN("[SCAN] ========== NETWORK SCAN COMPLETATO ==========");
  DEBUG_PRINTF("[SCAN] Tempo: %ld secondi\n", totalTime);
  DEBUG_PRINTF("[SCAN] Scansionati: %d host, Trovati: %d, Nuovi: %d\n", scanned, found, added);
  return added;
}

// ========== PING E GESTIONE ONLINE/OFFLINE ==========

// Ping singolo client - ritorna true se online
bool pingClient(int index) {
  if (index < 0 || index >= clientCount) return false;

  HTTPClient http;
  String url = "http://" + clients[index].ip + ":" + String(clients[index].port) + "/radar/test?value=ping";

  http.begin(url);
  http.setTimeout(PING_TIMEOUT_MS);
  int httpCode = http.GET();
  http.end();

  clients[index].lastPing = millis();

  if (httpCode == 200) {
    clients[index].online = true;
    clients[index].failedPings = 0;
    clients[index].lastSeen = millis();
    return true;
  } else {
    clients[index].failedPings++;
    if (clients[index].failedPings >= MAX_FAILED_PINGS) {
      clients[index].online = false;
    }
    return false;
  }
}

// Ping tutti i client
int pingAllClients() {
  DEBUG_PRINTLN("\n[PING] Verifica stato client...");

  int online = 0;
  int offline = 0;

  for (int i = 0; i < clientCount; i++) {
    bool isOnline = pingClient(i);
    if (isOnline) {
      online++;
      DEBUG_PRINTF("[PING] %s (%s:%d) - ONLINE\n",
                   clients[i].name.c_str(), clients[i].ip.c_str(), clients[i].port);
    } else {
      offline++;
      DEBUG_PRINTF("[PING] %s (%s:%d) - OFFLINE (fail: %d/%d)\n",
                   clients[i].name.c_str(), clients[i].ip.c_str(), clients[i].port,
                   clients[i].failedPings, MAX_FAILED_PINGS);
    }
    yield();  // Evita watchdog
  }

  DEBUG_PRINTF("[PING] Risultato: %d online, %d offline\n", online, offline);
  return online;
}

// Rimuovi client offline (con più di MAX_FAILED_PINGS falliti)
int cleanupOfflineClients() {
  DEBUG_PRINTLN("\n[CLEANUP] Rimozione client offline...");

  int removed = 0;
  int i = 0;

  while (i < clientCount) {
    if (!clients[i].online && clients[i].failedPings >= MAX_FAILED_PINGS) {
      DEBUG_PRINTF("[CLEANUP] Rimosso: %s (%s)\n",
                   clients[i].name.c_str(), clients[i].ip.c_str());

      // Sposta gli altri client
      for (int j = i; j < clientCount - 1; j++) {
        clients[j] = clients[j + 1];
      }
      clientCount--;
      removed++;
      // Non incrementare i, ricontrolla la stessa posizione
    } else {
      i++;
    }
  }

  if (removed > 0) {
    saveClients();
    DEBUG_PRINTF("[CLEANUP] Rimossi %d client offline\n", removed);
  } else {
    DEBUG_PRINTLN("[CLEANUP] Nessun client da rimuovere");
  }

  return removed;
}

// Ping automatico periodico (chiamare nel loop)
void autoPingClients() {
  if (!autoPingEnabled || clientCount == 0) return;

  if (millis() - lastAutoPing > AUTO_PING_INTERVAL) {
    pingAllClients();
    lastAutoPing = millis();
  }
}

void loadSettings() {
  preferences.begin("radar", true);
  autoPresenceEnabled = preferences.getBool("autoPresence", true);
  autoBrightnessEnabled = preferences.getBool("autoBright", true);
  presenceTimeout = preferences.getULong("presTimeout", 5000);  // Default 5 sec
  minBrightness = preferences.getInt("minBright", 10);
  maxBrightness = preferences.getInt("maxBright", 255);
  // Calibrazione BME280
  tempOffset = preferences.getFloat("tempOffset", 0.0);
  humOffset = preferences.getFloat("humOffset", 0.0);
  // Relè
  relayAutoMode = preferences.getBool("relayAuto", false);
  relayInverted = preferences.getBool("relayInv", false);
  relayState = preferences.getBool("relayState", false);
  preferences.end();
  DEBUG_PRINTF("Calibrazione caricata - TempOffset: %.1f, HumOffset: %.1f\n", tempOffset, humOffset);
  DEBUG_PRINTF("Relay - Auto: %d, Inverted: %d, State: %d\n", relayAutoMode, relayInverted, relayState);
}

void saveSettings() {
  preferences.begin("radar", false);
  preferences.putBool("autoPresence", autoPresenceEnabled);
  preferences.putBool("autoBright", autoBrightnessEnabled);
  preferences.putULong("presTimeout", presenceTimeout);
  preferences.putInt("minBright", minBrightness);
  preferences.putInt("maxBright", maxBrightness);
  // Calibrazione BME280
  preferences.putFloat("tempOffset", tempOffset);
  preferences.putFloat("humOffset", humOffset);
  // Relè
  preferences.putBool("relayAuto", relayAutoMode);
  preferences.putBool("relayInv", relayInverted);
  preferences.putBool("relayState", relayState);
  preferences.end();
}

// ========== FORWARD DECLARATIONS ==========
void resetWiFiSettings();  // Dichiarata qui, definita dopo setupWiFi()

// ========== API WEB ==========
void setupWebServer() {
  // CORS headers per tutte le risposte
  server.enableCORS(true);

  // Pagina Web principale con impostazioni
  server.on("/", HTTP_GET, []() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Radar Server C3</title>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    *{box-sizing:border-box}
    body{font-family:Arial,sans-serif;margin:0;padding:15px;background:#1a1a2e;color:#eee}
    .container{max-width:800px;margin:0 auto}
    h1{color:#00d4ff;text-align:center;margin-bottom:20px}
    h2{color:#0099cc;margin:0 0 15px 0;font-size:18px;border-bottom:1px solid #0099cc;padding-bottom:5px}
    .card{background:#16213e;padding:15px;border-radius:10px;margin-bottom:15px}
    .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:10px}
    .stat{background:#0f3460;padding:10px;border-radius:5px;text-align:center}
    .stat-value{font-size:24px;font-weight:bold;color:#00d4ff}
    .stat-label{font-size:12px;color:#888;margin-top:5px}
    .green{color:#00ff88}.red{color:#ff4444}.yellow{color:#ffaa00}.blue{color:#00d4ff}
    .row{display:flex;align-items:center;margin:10px 0;flex-wrap:wrap}
    .row label{width:140px;font-size:14px}
    .row input,.row select{flex:1;padding:8px;border:1px solid #0099cc;border-radius:5px;background:#0f3460;color:#eee;min-width:100px}
    .row input:focus{outline:none;border-color:#00d4ff}
    button{background:#0099cc;color:white;border:none;padding:10px 20px;border-radius:5px;cursor:pointer;margin:5px;font-size:14px}
    button:hover{background:#00bbee}
    button.danger{background:#cc3333}
    button.danger:hover{background:#ee4444}
    button.success{background:#33cc33}
    .switch{position:relative;width:50px;height:26px;display:inline-block}
    .switch input{opacity:0;width:0;height:0}
    .slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#555;border-radius:26px;transition:.3s}
    .slider:before{position:absolute;content:"";height:20px;width:20px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.3s}
    input:checked+.slider{background:#00ff88}
    input:checked+.slider:before{transform:translateX(24px)}
    .client{background:#0f3460;padding:10px;margin:5px 0;border-radius:5px;display:flex;justify-content:space-between;align-items:center}
    .msg{padding:10px;border-radius:5px;margin:10px 0;display:none}
    .msg.ok{background:#1a4d1a;display:block}
    .msg.err{background:#4d1a1a;display:block}
    .msg.success{background:#1a4d1a;display:block}
    .msg.error{background:#4d1a1a;display:block}
    /* Spinner animation */
    .loader{display:inline-block;width:20px;height:20px;border:3px solid #0099cc;border-radius:50%;border-top-color:#00d4ff;animation:spin 1s linear infinite;vertical-align:middle;margin-right:10px}
    @keyframes spin{to{transform:rotate(360deg)}}
    .scan-progress{background:#0f3460;padding:15px;border-radius:8px;margin:10px 0}
    .scan-progress .header{display:flex;align-items:center;margin-bottom:10px}
    .scan-progress .status{color:#00d4ff;font-weight:bold}
    .progress-bar{height:6px;background:#1a1a2e;border-radius:3px;overflow:hidden}
    .progress-bar .fill{height:100%;background:linear-gradient(90deg,#0099cc,#00d4ff);width:0%;transition:width 0.3s}
    .scan-log{font-family:monospace;font-size:12px;color:#888;max-height:100px;overflow-y:auto;margin-top:10px;padding:5px;background:#1a1a2e;border-radius:4px}
    .tabs{display:flex;border-bottom:2px solid #0099cc;margin-bottom:15px}
    .tab{padding:10px 20px;cursor:pointer;border-radius:5px 5px 0 0;background:#0f3460}
    .tab.active{background:#0099cc}
    .tab-content{display:none}
    .tab-content.active{display:block}
  </style>
</head>
<body>
<div class="container">
  <h1>Radar Server ESP32-C3</h1>

  <div class="tabs">
    <div class="tab active" onclick="showTab(0)">Stato</div>
    <div class="tab" onclick="showTab(1)">Impostazioni</div>
    <div class="tab" onclick="showTab(2)">Calibrazione</div>
    <div class="tab" onclick="showTab(3)">Client</div>
  </div>

  <!-- TAB STATO -->
  <div class="tab-content active" id="tab0">
    <div class="card">
      <h2>Presenza</h2>
      <div class="grid">
        <div class="stat"><div class="stat-value" id="presence">--</div><div class="stat-label">Stato</div></div>
        <div class="stat"><div class="stat-value" id="distance">--</div><div class="stat-label">Distanza (cm)</div></div>
        <div class="stat"><div class="stat-value" id="moving">--</div><div class="stat-label">Movimento</div></div>
        <div class="stat"><div class="stat-value" id="stationary">--</div><div class="stat-label">Stazionario</div></div>
      </div>
    </div>
    <div class="card">
      <h2>Ambiente (BME280)</h2>
      <div class="grid">
        <div class="stat"><div class="stat-value" id="temp">--</div><div class="stat-label">Temperatura C</div></div>
        <div class="stat"><div class="stat-value" id="hum">--</div><div class="stat-label">Umidita %</div></div>
        <div class="stat"><div class="stat-value" id="press">--</div><div class="stat-label">Pressione hPa</div></div>
        <div class="stat"><div class="stat-value" id="light">--</div><div class="stat-label">Luce</div></div>
      </div>
    </div>
    <div class="card">
      <h2>Sistema</h2>
      <div class="grid">
        <div class="stat"><div class="stat-value" id="uptime">--</div><div class="stat-label">Uptime</div></div>
        <div class="stat"><div class="stat-value" id="rssi">--</div><div class="stat-label">WiFi dBm</div></div>
        <div class="stat"><div class="stat-value" id="heap">--</div><div class="stat-label">Heap KB</div></div>
        <div class="stat"><div class="stat-value" id="ip">--</div><div class="stat-label">IP</div></div>
      </div>
    </div>
  </div>

  <!-- TAB IMPOSTAZIONI -->
  <div class="tab-content" id="tab1">
    <div class="card">
      <h2>Controllo Automatico</h2>
      <div class="row">
        <label>Auto Presenza:</label>
        <label class="switch"><input type="checkbox" id="autoPresence" onchange="toggleSetting('presence')"><span class="slider"></span></label>
      </div>
      <div class="row">
        <label>Auto Brightness:</label>
        <label class="switch"><input type="checkbox" id="autoBrightness" onchange="toggleSetting('brightness')"><span class="slider"></span></label>
      </div>
    </div>
    <div class="card">
      <h2>Parametri</h2>
      <div class="row">
        <label>Timeout Assenza (ms):</label>
        <input type="number" id="presenceTimeout" min="1000" max="300000" step="1000">
      </div>
      <div class="row">
        <label>Brightness Min:</label>
        <input type="number" id="minBrightness" min="0" max="255">
      </div>
      <div class="row">
        <label>Brightness Max:</label>
        <input type="number" id="maxBrightness" min="0" max="255">
      </div>
      <button onclick="saveSettings()">Salva Impostazioni</button>
      <div class="msg" id="msgSettings"></div>
    </div>
    <div class="card">
      <h2>Azioni</h2>
      <button onclick="api('/api/notify/test')">Test Notifica</button>
      <button onclick="api('/api/notify/presence')">Invia Presenza</button>
      <button onclick="api('/api/notify/brightness')">Invia Brightness</button>
      <button class="danger" onclick="if(confirm('Riavviare?'))location='/api/restart'">Riavvia</button>
    </div>
    <div class="card">
      <h2>WiFi</h2>
      <div class="grid">
        <div class="stat"><div class="stat-value" id="wifiSSID">--</div><div class="stat-label">SSID</div></div>
        <div class="stat"><div class="stat-value" id="wifiMAC">--</div><div class="stat-label">MAC</div></div>
      </div>
      <button class="danger" onclick="if(confirm('Reset WiFi? Il dispositivo entrera in modalita configurazione.'))location='/api/wifi/reset'">Reset WiFi</button>
    </div>
  </div>

  <!-- TAB CALIBRAZIONE -->
  <div class="tab-content" id="tab2">
    <div class="card">
      <h2>Calibrazione BME280</h2>
      <div class="grid">
        <div class="stat"><div class="stat-value" id="rawTemp">--</div><div class="stat-label">Temp Raw</div></div>
        <div class="stat"><div class="stat-value" id="calTemp">--</div><div class="stat-label">Temp Calibrata</div></div>
        <div class="stat"><div class="stat-value" id="rawHum">--</div><div class="stat-label">Umid Raw</div></div>
        <div class="stat"><div class="stat-value" id="calHum">--</div><div class="stat-label">Umid Calibrata</div></div>
      </div>
      <div class="row">
        <label>Offset Temperatura:</label>
        <input type="number" id="tempOffset" step="0.1" min="-10" max="10">
      </div>
      <div class="row">
        <label>Offset Umidita:</label>
        <input type="number" id="humOffset" step="0.1" min="-20" max="20">
      </div>
      <button onclick="saveCalibration()">Salva Calibrazione</button>
      <button class="danger" onclick="resetCalibration()">Reset a 0</button>
      <div class="msg" id="msgCal"></div>
    </div>
  </div>

  <!-- TAB CLIENT -->
  <div class="tab-content" id="tab3">
    <div class="card">
      <h2>Registra Nuovo Client</h2>
      <div class="row">
        <label>IP:</label>
        <input type="text" id="newClientIp" placeholder="192.168.1.100">
      </div>
      <div class="row">
        <label>Porta:</label>
        <input type="number" id="newClientPort" placeholder="80" value="80" min="1" max="65535">
      </div>
      <div class="row">
        <label>Nome:</label>
        <input type="text" id="newClientName" placeholder="Lampada Sala">
      </div>
      <button onclick="addClient()">Aggiungi Client</button>
    </div>
    <div class="card">
      <h2>Client Registrati</h2>
      <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:10px">
        <span id="clientStats">-</span>
        <div>
          <button onclick="pingAllClients()" style="background:#2980b9">Ping Tutti</button>
          <button onclick="cleanupOffline()" class="danger">Rimuovi Offline</button>
        </div>
      </div>
      <div id="pingResult" style="color:#888;font-size:12px;margin-bottom:5px"></div>
      <div id="clientList">Caricamento...</div>
    </div>
    <div class="card">
      <h2>Ricerca Device</h2>
      <p>Cerca device compatibili sulla rete</p>
      <div id="scanButtons" style="display:flex;gap:10px;flex-wrap:wrap;margin-bottom:10px">
        <button onclick="discoverMDNS()" style="background:#27ae60">mDNS Discovery</button>
        <button onclick="networkScan()" style="background:#2980b9">Network Scan</button>
        <button onclick="fullDiscovery()" style="background:#8e44ad">Cerca Tutto</button>
      </div>
      <div id="scanProgress" class="scan-progress" style="display:none">
        <div class="header">
          <div class="loader"></div>
          <span id="scanStatus" class="status">Ricerca in corso...</span>
        </div>
        <div class="progress-bar"><div id="progressFill" class="fill"></div></div>
        <div id="scanLog" class="scan-log"></div>
      </div>
      <div id="discoveryResult" class="msg"></div>
      <div class="row" style="margin-top:10px">
        <label>Auto-scan mDNS:</label>
        <span id="autoDiscoveryStatus">-</span>
        <button onclick="toggleAutoDiscovery()">Toggle</button>
      </div>
      <p style="font-size:0.8em;color:#888;margin-top:10px">
        mDNS: veloce (~2 sec)<br>
        Network Scan: scansiona 254 IP (~30-60 sec)
      </p>
    </div>
  </div>
</div>

<script>
let tabs=document.querySelectorAll('.tab');
let contents=document.querySelectorAll('.tab-content');
function showTab(n){tabs.forEach((t,i)=>{t.classList.toggle('active',i==n);contents[i].classList.toggle('active',i==n)})}

function api(url){return fetch(url).then(r=>r.json())}

// Aggiorna solo i valori di sola lettura (non i campi input)
function updateStatus(){
  api('/api/status').then(d=>{
    document.getElementById('presence').innerHTML=d.radar.presence?'<span class="green">SI</span>':'<span class="red">NO</span>';
    document.getElementById('distance').textContent=d.radar.distance;
    document.getElementById('moving').innerHTML=d.radar.moving?'<span class="green">SI</span>':'NO';
    document.getElementById('stationary').innerHTML=d.radar.stationary?'<span class="green">SI</span>':'NO';
    document.getElementById('light').textContent=d.light.ambient;
    document.getElementById('uptime').textContent=Math.floor(d.system.uptime/60)+'m';
    document.getElementById('rssi').textContent=d.system.rssi;
    document.getElementById('heap').textContent=Math.floor(d.system.freeHeap/1024);
    document.getElementById('ip').textContent=d.system.ip;
    document.getElementById('autoPresence').checked=d.settings.autoPresence;
    document.getElementById('autoBrightness').checked=d.settings.autoBrightness;
  });
  api('/api/wifi/info').then(d=>{
    document.getElementById('wifiSSID').textContent=d.ssid||'--';
    document.getElementById('wifiMAC').textContent=d.mac?d.mac.substring(9):'--';
  });
  api('/api/calibration').then(d=>{
    document.getElementById('temp').textContent=d.calibratedTemp;
    document.getElementById('hum').textContent=d.calibratedHum;
    document.getElementById('press').textContent=d.pressure;
    document.getElementById('rawTemp').textContent=d.rawTemp;
    document.getElementById('calTemp').textContent=d.calibratedTemp;
    document.getElementById('rawHum').textContent=d.rawHum;
    document.getElementById('calHum').textContent=d.calibratedHum;
  });
}

// Carica i valori editabili solo una volta all'avvio
function loadEditableFields(){
  api('/api/status').then(d=>{
    document.getElementById('presenceTimeout').value=d.settings.presenceTimeout;
    document.getElementById('minBrightness').value=d.settings.minBrightness;
    document.getElementById('maxBrightness').value=d.settings.maxBrightness;
  });
  api('/api/calibration').then(d=>{
    document.getElementById('tempOffset').value=d.tempOffset;
    document.getElementById('humOffset').value=d.humOffset;
  });
  loadClients();
}

function toggleSetting(s){api('/api/toggle/'+s)}

function saveSettings(){
  let p=new URLSearchParams();
  p.append('presenceTimeout',document.getElementById('presenceTimeout').value);
  p.append('minBrightness',document.getElementById('minBrightness').value);
  p.append('maxBrightness',document.getElementById('maxBrightness').value);
  fetch('/api/settings',{method:'POST',body:p}).then(r=>r.json()).then(d=>{
    showMsg('msgSettings','ok','Salvato!');
  });
}

function saveCalibration(){
  let p=new URLSearchParams();
  p.append('tempOffset',document.getElementById('tempOffset').value);
  p.append('humOffset',document.getElementById('humOffset').value);
  fetch('/api/calibration',{method:'POST',body:p}).then(r=>r.json()).then(d=>{
    showMsg('msgCal','ok','Calibrazione salvata!');
  });
}

function resetCalibration(){
  api('/api/calibration/reset').then(d=>{
    showMsg('msgCal','ok','Reset completato!');
    loadEditableFields();
  });
}

function loadClients(){
  api('/api/clients').then(clients=>{
    let h='';
    let online=0,offline=0;
    if(clients.length==0)h='<p>Nessun client registrato</p>';
    else clients.forEach(c=>{
      let statusClass=c.online?'green':'red';
      let statusText=c.online?'ONLINE':'OFFLINE';
      if(c.online)online++;else offline++;
      h+='<div class="client">';
      h+='<span class="'+statusClass+'" style="margin-right:10px">●</span>';
      h+='<span style="flex:1"><b>'+c.name+'</b> - '+c.ip+':'+c.port;
      if(!c.online)h+=' <small>(fail:'+c.failedPings+')</small>';
      h+='</span>';
      h+='<button onclick="pingClient(\''+c.ip+'\')" style="background:#2980b9;padding:5px 10px">Ping</button>';
      h+='<button class="danger" onclick="removeClient(\''+c.ip+'\')" style="padding:5px 10px">X</button>';
      h+='</div>';
    });
    document.getElementById('clientList').innerHTML=h;
    document.getElementById('clientStats').innerHTML='<span class="green">'+online+' online</span> / <span class="red">'+offline+' offline</span>';
  });
}

function pingClient(ip){
  api('/api/client/ping?ip='+ip).then(d=>{
    loadClients();
  });
}

function pingAllClients(){
  document.getElementById('pingResult').textContent='Ping in corso...';
  api('/api/clients/ping').then(d=>{
    document.getElementById('pingResult').textContent='Online: '+d.online+' / Offline: '+d.offline;
    loadClients();
  });
}

function cleanupOffline(){
  if(!confirm('Rimuovere tutti i client offline?'))return;
  api('/api/clients/cleanup').then(d=>{
    alert('Rimossi '+d.removed+' client');
    loadClients();
  });
}

function addClient(){
  let ip=document.getElementById('newClientIp').value;
  let port=document.getElementById('newClientPort').value || 80;
  let name=document.getElementById('newClientName').value;
  if(!ip)return alert('Inserisci IP');
  api('/api/client/register?ip='+ip+'&port='+port+'&name='+encodeURIComponent(name)).then(d=>{
    document.getElementById('newClientIp').value='';
    document.getElementById('newClientPort').value='80';
    document.getElementById('newClientName').value='';
    loadClients();
  });
}

function removeClient(ip){
  if(confirm('Rimuovere '+ip+'?'))api('/api/client/remove?ip='+ip).then(d=>loadClients());
}

let scanRunning=false;
let progressInterval=null;

function startScan(method,duration){
  if(scanRunning)return false;
  scanRunning=true;
  document.getElementById('scanButtons').style.opacity='0.5';
  document.getElementById('scanButtons').style.pointerEvents='none';
  document.getElementById('scanProgress').style.display='block';
  document.getElementById('discoveryResult').className='msg';
  document.getElementById('scanLog').innerHTML='';
  document.getElementById('progressFill').style.width='0%';

  let progress=0;
  let step=100/(duration/100);
  addLog('Avvio '+method+'...');

  progressInterval=setInterval(()=>{
    progress+=step;
    if(progress>95)progress=95;
    document.getElementById('progressFill').style.width=progress+'%';
  },100);

  return true;
}

function stopScan(result){
  scanRunning=false;
  clearInterval(progressInterval);
  document.getElementById('progressFill').style.width='100%';
  document.getElementById('scanButtons').style.opacity='1';
  document.getElementById('scanButtons').style.pointerEvents='auto';
  setTimeout(()=>{
    document.getElementById('scanProgress').style.display='none';
  },1000);
  addLog('Completato: '+result);
}

function addLog(msg){
  let log=document.getElementById('scanLog');
  let time=new Date().toLocaleTimeString();
  log.innerHTML+='['+time+'] '+msg+'<br>';
  log.scrollTop=log.scrollHeight;
}

function discoverMDNS(){
  if(!startScan('mDNS Discovery',3000))return;
  document.getElementById('scanStatus').textContent='mDNS Discovery...';
  addLog('Cercando servizi _radardevice._tcp...');

  api('/api/discover').then(d=>{
    stopScan(d.newDevices+' nuovi device');
    let msg='mDNS: '+d.newDevices+' nuovi (totale: '+d.totalClients+')';
    showMsg('discoveryResult',d.newDevices>0?'success':'',msg);
    loadClients();
  }).catch(e=>{
    stopScan('Errore');
    showMsg('discoveryResult','error','Errore mDNS');
  });
}

function networkScan(){
  if(!startScan('Network Scan',45000))return;
  document.getElementById('scanStatus').textContent='Scansione rete (254 IP)...';
  addLog('Scansione sottorete locale...');
  addLog('Porte: 80, 8080');

  // Simula log progressivo
  let hosts=['50','100','150','200','254'];
  let i=0;
  let logInterval=setInterval(()=>{
    if(i<hosts.length&&scanRunning){
      addLog('Scansionati '+hosts[i]+'/254 host...');
      i++;
    }else{
      clearInterval(logInterval);
    }
  },8000);

  api('/api/scan').then(d=>{
    clearInterval(logInterval);
    stopScan(d.newDevices+' nuovi device');
    let msg='Scan: '+d.newDevices+' nuovi (totale: '+d.totalClients+')';
    showMsg('discoveryResult',d.newDevices>0?'success':'',msg);
    loadClients();
  }).catch(e=>{
    clearInterval(logInterval);
    stopScan('Errore');
    showMsg('discoveryResult','error','Errore scan');
  });
}

function fullDiscovery(){
  if(!startScan('Ricerca Completa',50000))return;
  document.getElementById('scanStatus').textContent='Ricerca completa (mDNS + Scan)...';
  addLog('Fase 1: mDNS Discovery...');

  setTimeout(()=>{if(scanRunning)addLog('Fase 2: Network Scan...');},3000);

  api('/api/discover/full').then(d=>{
    stopScan((d.mdnsFound+d.scanFound)+' nuovi device');
    let msg='mDNS: '+d.mdnsFound+', Scan: '+d.scanFound+' (totale: '+d.totalClients+')';
    showMsg('discoveryResult',d.totalNew>0?'success':'',msg);
    loadClients();
  }).catch(e=>{
    stopScan('Errore');
    showMsg('discoveryResult','error','Errore ricerca');
  });
}

function toggleAutoDiscovery(){
  api('/api/discovery/toggle').then(d=>{
    document.getElementById('autoDiscoveryStatus').textContent=d.autoDiscovery?'ON':'OFF';
  });
}

function showMsg(id,type,text){
  let el=document.getElementById(id);
  el.className='msg '+type;
  el.textContent=text;
  setTimeout(()=>el.className='msg',5000);
}

// Carica tutto all'avvio
loadEditableFields();
updateStatus();
// Aggiorna solo i dati in sola lettura ogni 3 secondi
setInterval(updateStatus,3000);
</script>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
  });

  // API JSON Status
  server.on("/api/status", HTTP_GET, []() {
    String json = "{";

    // Radar
    json += "\"radar\":{";
    json += "\"online\":" + String(radarCache.radarOnline ? "true" : "false") + ",";
    json += "\"presence\":" + String(presenceDetected ? "true" : "false") + ",";
    json += "\"moving\":" + String(radarCache.movingTargetDetected ? "true" : "false") + ",";
    json += "\"stationary\":" + String(radarCache.stationaryTargetDetected ? "true" : "false") + ",";
    json += "\"distance\":" + String(radarCache.detectedDistance) + ",";
    json += "\"movingDistance\":" + String(radarCache.movingTargetDistance) + ",";
    json += "\"movingSignal\":" + String(radarCache.movingTargetSignal) + ",";
    json += "\"stationaryDistance\":" + String(radarCache.stationaryTargetDistance) + ",";
    json += "\"stationarySignal\":" + String(radarCache.stationaryTargetSignal);
    json += "},";

    // Light
    json += "\"light\":{";
    json += "\"ambient\":" + String(ambientLight) + ",";
    json += "\"brightness\":" + String(calculatedBrightness) + ",";
    json += "\"autoEnabled\":" + String(autoBrightnessEnabled ? "true" : "false");
    json += "},";

    // Settings
    json += "\"settings\":{";
    json += "\"autoPresence\":" + String(autoPresenceEnabled ? "true" : "false") + ",";
    json += "\"autoBrightness\":" + String(autoBrightnessEnabled ? "true" : "false") + ",";
    json += "\"presenceTimeout\":" + String(presenceTimeout) + ",";
    json += "\"minBrightness\":" + String(minBrightness) + ",";
    json += "\"maxBrightness\":" + String(maxBrightness);
    json += "},";

    // System
    json += "\"system\":{";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"clients\":" + String(clientCount);
    json += "}";

    json += "}";

    server.send(200, "application/json", json);
  });

  // API Solo presenza
  server.on("/api/presence", HTTP_GET, []() {
    String json = "{";
    json += "\"presence\":" + String(presenceDetected ? "true" : "false") + ",";
    json += "\"moving\":" + String(radarCache.movingTargetDetected ? "true" : "false") + ",";
    json += "\"stationary\":" + String(radarCache.stationaryTargetDetected ? "true" : "false") + ",";
    json += "\"distance\":" + String(radarCache.detectedDistance);
    json += "}";
    server.send(200, "application/json", json);
  });

  // API /sensorData - Compatibile con AuroraDemo
  // Restituisce tutti i dati dei sensori
  server.on("/sensorData", HTTP_GET, []() {
    // Mappa ambientLight (0-255) a livello luce (0-4095) come AuroraDemo si aspetta
    int lightLevel = map(ambientLight, 0, 255, 0, 4095);

    String json = "{";
    json += "\"light\":{\"level\":" + String(lightLevel) + "},";
    json += "\"radar\":{\"presenceDetected\":" + String(presenceDetected ? "true" : "false") + ",";
    json += "\"distance\":" + String(radarCache.detectedDistance) + "},";
    // Temperatura e umidità dal BME280
    json += "\"temperature\":" + String(bmeTemperature, 1) + ",";
    json += "\"humidity\":" + String(bmeHumidity, 1) + ",";
    json += "\"pressure\":" + String(bmePressure, 1);
    json += "}";

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
  });

  // API Solo luminosita
  server.on("/api/brightness", HTTP_GET, []() {
    String json = "{";
    json += "\"ambient\":" + String(ambientLight) + ",";
    json += "\"brightness\":" + String(calculatedBrightness) + ",";
    json += "\"min\":" + String(minBrightness) + ",";
    json += "\"max\":" + String(maxBrightness);
    json += "}";
    server.send(200, "application/json", json);
  });

  // Toggle Auto Presenza
  server.on("/api/toggle/presence", HTTP_GET, []() {
    autoPresenceEnabled = !autoPresenceEnabled;
    saveSettings();
    server.send(200, "application/json",
      "{\"autoPresence\":" + String(autoPresenceEnabled ? "true" : "false") + "}");
  });

  // Toggle Auto Brightness
  server.on("/api/toggle/brightness", HTTP_GET, []() {
    autoBrightnessEnabled = !autoBrightnessEnabled;
    saveSettings();
    server.send(200, "application/json",
      "{\"autoBrightness\":" + String(autoBrightnessEnabled ? "true" : "false") + "}");
  });

  // Imposta settings
  server.on("/api/settings", HTTP_POST, []() {
    if (server.hasArg("presenceTimeout")) {
      presenceTimeout = server.arg("presenceTimeout").toInt();
    }
    if (server.hasArg("minBrightness")) {
      minBrightness = server.arg("minBrightness").toInt();
    }
    if (server.hasArg("maxBrightness")) {
      maxBrightness = server.arg("maxBrightness").toInt();
    }
    if (server.hasArg("autoPresence")) {
      autoPresenceEnabled = server.arg("autoPresence") == "true";
    }
    if (server.hasArg("autoBrightness")) {
      autoBrightnessEnabled = server.arg("autoBrightness") == "true";
    }
    saveSettings();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // API Calibrazione BME280 - GET per leggere, POST per impostare
  server.on("/api/calibration", HTTP_GET, []() {
    String json = "{";
    json += "\"tempOffset\":" + String(tempOffset, 1) + ",";
    json += "\"humOffset\":" + String(humOffset, 1) + ",";
    json += "\"rawTemp\":" + String(bme.readTemperature(), 1) + ",";
    json += "\"rawHum\":" + String(bme.readHumidity(), 1) + ",";
    json += "\"calibratedTemp\":" + String(bmeTemperature, 1) + ",";
    json += "\"calibratedHum\":" + String(bmeHumidity, 1) + ",";
    json += "\"pressure\":" + String(bmePressure, 1);
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/api/calibration", HTTP_POST, []() {
    bool changed = false;
    if (server.hasArg("tempOffset")) {
      tempOffset = server.arg("tempOffset").toFloat();
      changed = true;
      DEBUG_PRINTF("TempOffset impostato: %.1f\n", tempOffset);
    }
    if (server.hasArg("humOffset")) {
      humOffset = server.arg("humOffset").toFloat();
      changed = true;
      DEBUG_PRINTF("HumOffset impostato: %.1f\n", humOffset);
    }
    if (changed) {
      saveSettings();
    }
    String json = "{\"status\":\"ok\",\"tempOffset\":" + String(tempOffset, 1) +
                  ",\"humOffset\":" + String(humOffset, 1) + "}";
    server.send(200, "application/json", json);
  });

  // Reset calibrazione
  server.on("/api/calibration/reset", HTTP_GET, []() {
    tempOffset = 0.0;
    humOffset = 0.0;
    saveSettings();
    DEBUG_PRINTLN("Calibrazione resettata");
    server.send(200, "application/json", "{\"status\":\"reset\",\"tempOffset\":0,\"humOffset\":0}");
  });

  // Registra client
  server.on("/api/client/register", HTTP_GET, []() {
    String ip = server.arg("ip");
    String name = server.arg("name");
    String portStr = server.arg("port");
    uint16_t port = 80;  // Default porta 80

    if (ip.length() == 0) {
      ip = server.client().remoteIP().toString();
    }
    if (name.length() == 0) {
      name = "Device";
    }
    if (portStr.length() > 0) {
      port = portStr.toInt();
      if (port == 0) port = 80;  // Fallback se parsing fallisce
    }

    addClient(ip, name, port);

    // Invia notifica iniziale dopo breve delay per permettere la risposta HTTP
    // Il client riceverà lo stato corrente subito dopo la registrazione
    DEBUG_PRINTF("Invio stato iniziale a %s:%d...\n", ip.c_str(), port);

    // NOTA: inviamo ambientLight (luce RAW), il client mappa secondo i suoi parametri
    server.send(200, "application/json",
      "{\"status\":\"registered\",\"ip\":\"" + ip + "\",\"name\":\"" + name +
      "\",\"port\":" + String(port) +
      ",\"brightness\":" + String(ambientLight) +
      ",\"presence\":" + String(presenceDetected ? "true" : "false") + "}");

    // Forza notifica iniziale dopo 500ms (per dare tempo al client di elaborare la risposta)
    static unsigned long lastInitNotify = 0;
    lastInitNotify = millis();
  });

  // Rimuovi client
  server.on("/api/client/remove", HTTP_GET, []() {
    String ip = server.arg("ip");
    if (ip.length() == 0) {
      ip = server.client().remoteIP().toString();
    }
    removeClient(ip);
    server.send(200, "application/json", "{\"status\":\"removed\"}");
  });

  // Ping tutti i client
  server.on("/api/clients/ping", HTTP_GET, []() {
    int online = pingAllClients();
    int offline = clientCount - online;
    String json = "{\"online\":" + String(online) + ",\"offline\":" + String(offline) + ",\"total\":" + String(clientCount) + "}";
    server.send(200, "application/json", json);
  });

  // Ping singolo client
  server.on("/api/client/ping", HTTP_GET, []() {
    String ip = server.arg("ip");
    bool found = false;
    bool isOnline = false;

    for (int i = 0; i < clientCount; i++) {
      if (clients[i].ip == ip) {
        isOnline = pingClient(i);
        found = true;
        break;
      }
    }

    if (found) {
      String json = "{\"ip\":\"" + ip + "\",\"online\":" + String(isOnline ? "true" : "false") + "}";
      server.send(200, "application/json", json);
    } else {
      server.send(404, "application/json", "{\"error\":\"client not found\"}");
    }
  });

  // Rimuovi client offline
  server.on("/api/clients/cleanup", HTTP_GET, []() {
    int removed = cleanupOfflineClients();
    String json = "{\"removed\":" + String(removed) + ",\"remaining\":" + String(clientCount) + "}";
    server.send(200, "application/json", json);
  });

  // Toggle auto-ping
  server.on("/api/clients/autoping", HTTP_GET, []() {
    autoPingEnabled = !autoPingEnabled;
    String json = "{\"autoPing\":" + String(autoPingEnabled ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  // Lista client con dettagli completi
  server.on("/api/clients", HTTP_GET, []() {
    String json = "[";
    for (int i = 0; i < clientCount; i++) {
      if (i > 0) json += ",";
      json += "{";
      json += "\"ip\":\"" + clients[i].ip + "\",";
      json += "\"name\":\"" + clients[i].name + "\",";
      json += "\"port\":" + String(clients[i].port) + ",";
      json += "\"active\":" + String(clients[i].active ? "true" : "false") + ",";
      json += "\"online\":" + String(clients[i].online ? "true" : "false") + ",";
      json += "\"failedPings\":" + String(clients[i].failedPings) + ",";
      json += "\"notifyPresence\":" + String(clients[i].notifyPresence ? "true" : "false") + ",";
      json += "\"notifyBrightness\":" + String(clients[i].notifyBrightness ? "true" : "false");
      json += "}";
    }
    json += "]";
    server.send(200, "application/json", json);
  });

  // mDNS Auto-Discovery - cerca device sulla rete
  server.on("/api/discover", HTTP_GET, []() {
    DEBUG_PRINTLN("API: Avvio discovery manuale...");
    int newDevices = discoverClients();
    String json = "{";
    json += "\"status\":\"completed\",";
    json += "\"newDevices\":" + String(newDevices) + ",";
    json += "\"totalClients\":" + String(clientCount);
    json += "}";
    server.send(200, "application/json", json);
  });

  // Toggle auto-discovery
  server.on("/api/discovery/toggle", HTTP_GET, []() {
    autoDiscoveryEnabled = !autoDiscoveryEnabled;
    String json = "{\"autoDiscovery\":" + String(autoDiscoveryEnabled ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  // Imposta intervallo discovery (in minuti)
  server.on("/api/discovery/interval", HTTP_GET, []() {
    if (server.hasArg("minutes")) {
      int minutes = server.arg("minutes").toInt();
      if (minutes >= 1 && minutes <= 60) {
        discoveryInterval = (unsigned long)minutes * 60000;
      }
    }
    String json = "{\"intervalMinutes\":" + String(discoveryInterval / 60000) + "}";
    server.send(200, "application/json", json);
  });

  // Network Scan - scansione IP della sottorete
  server.on("/api/scan", HTTP_GET, []() {
    DEBUG_PRINTLN("API: Avvio network scan...");
    int newDevices = networkScan();
    String json = "{";
    json += "\"status\":\"completed\",";
    json += "\"method\":\"network_scan\",";
    json += "\"newDevices\":" + String(newDevices) + ",";
    json += "\"totalClients\":" + String(clientCount);
    json += "}";
    server.send(200, "application/json", json);
  });

  // Discovery completa (mDNS + Network Scan)
  server.on("/api/discover/full", HTTP_GET, []() {
    DEBUG_PRINTLN("API: Avvio discovery completa (mDNS + Network Scan)...");
    int mdnsFound = discoverClients();
    int scanFound = networkScan();
    String json = "{";
    json += "\"status\":\"completed\",";
    json += "\"mdnsFound\":" + String(mdnsFound) + ",";
    json += "\"scanFound\":" + String(scanFound) + ",";
    json += "\"totalNew\":" + String(mdnsFound + scanFound) + ",";
    json += "\"totalClients\":" + String(clientCount);
    json += "}";
    server.send(200, "application/json", json);
  });

  // Test connessione diretta a un client specifico
  // Uso: /api/test/client?ip=192.168.1.100&port=80
  server.on("/api/test/client", HTTP_GET, []() {
    String ip = server.arg("ip");
    String portStr = server.arg("port");
    int port = portStr.length() > 0 ? portStr.toInt() : 80;

    if (ip.length() == 0) {
      server.send(400, "application/json", "{\"error\":\"missing ip parameter\"}");
      return;
    }

    HTTPClient http;
    String url = "http://" + ip + ":" + String(port) + "/radar/test?value=ping";

    http.begin(url);
    http.setTimeout(5000);
    int httpCode = http.GET();
    String response = http.getString();
    http.end();

    String json = "{";
    json += "\"url\":\"" + url + "\",";
    json += "\"httpCode\":" + String(httpCode) + ",";
    json += "\"response\":\"" + response + "\",";
    json += "\"success\":" + String(httpCode == 200 ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  // Test notifica
  server.on("/api/notify/test", HTTP_GET, []() {
    notifyClients("test", "ping");
    server.send(200, "application/json", "{\"status\":\"sent\"}");
  });

  // Forza notifica presenza
  server.on("/api/notify/presence", HTTP_GET, []() {
    notifyClients("presence", presenceDetected ? "true" : "false");
    server.send(200, "application/json", "{\"status\":\"sent\"}");
  });

  // Forza notifica luminosita
  server.on("/api/notify/brightness", HTTP_GET, []() {
    notifyClients("brightness", String(ambientLight));
    server.send(200, "application/json", "{\"status\":\"sent\",\"value\":" + String(ambientLight) + "}");
  });

  // Forza notifica temperatura
  server.on("/api/notify/temperature", HTTP_GET, []() {
    if (bme280Initialized) {
      notifyClients("temperature", String(bmeTemperature, 1));
      server.send(200, "application/json", "{\"status\":\"sent\",\"value\":" + String(bmeTemperature, 1) + "}");
    } else {
      server.send(200, "application/json", "{\"status\":\"error\",\"message\":\"BME280 not initialized\"}");
    }
  });

  // Forza notifica umidita
  server.on("/api/notify/humidity", HTTP_GET, []() {
    if (bme280Initialized) {
      notifyClients("humidity", String(bmeHumidity, 1));
      server.send(200, "application/json", "{\"status\":\"sent\",\"value\":" + String(bmeHumidity, 1) + "}");
    } else {
      server.send(200, "application/json", "{\"status\":\"error\",\"message\":\"BME280 not initialized\"}");
    }
  });

  // Forza invio di TUTTI i dati sensore (per test)
  server.on("/api/notify/all", HTTP_GET, []() {
    notifyClients("presence", presenceDetected ? "true" : "false");
    notifyClients("brightness", String(ambientLight));
    if (bme280Initialized) {
      notifyClients("temperature", String(bmeTemperature, 1));
      notifyClients("humidity", String(bmeHumidity, 1));
    }
    String json = "{\"status\":\"sent\"";
    json += ",\"presence\":" + String(presenceDetected ? "true" : "false");
    json += ",\"brightness\":" + String(ambientLight);
    json += ",\"temperature\":" + String(bmeTemperature, 1);
    json += ",\"humidity\":" + String(bmeHumidity, 1);
    json += ",\"bme280\":" + String(bme280Initialized ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  // TEST: Forza presenza=false (LED ROSSO, client OFF)
  server.on("/api/test/off", HTTP_GET, []() {
    manualPresenceOverride = true;  // Blocca aggiornamento automatico
    presenceDetected = false;
    previousPresenceState = false;
    // Forza LED rosso direttamente
    statusLed.setPixelColor(0, statusLed.Color(255, 0, 0));
    statusLed.show();
    currentLedColor = statusLed.Color(255, 0, 0);
    notifyClients("presence", "false");
    server.send(200, "application/json", "{\"mode\":\"manual_off\",\"led\":\"RED\"}");
  });

  // TEST: Forza presenza=true (LED SPENTO, client ON)
  server.on("/api/test/on", HTTP_GET, []() {
    manualPresenceOverride = true;  // Blocca aggiornamento automatico
    presenceDetected = true;
    previousPresenceState = true;
    lastPresenceTime = millis();
    // Forza LED spento direttamente
    statusLed.setPixelColor(0, 0);
    statusLed.show();
    currentLedColor = 0;
    notifyClients("presence", "true");
    server.send(200, "application/json", "{\"mode\":\"manual_on\",\"led\":\"OFF\"}");
  });

  // TEST: Ritorna in modalità automatica
  server.on("/api/test/auto", HTTP_GET, []() {
    manualPresenceOverride = false;  // Riabilita radar
    server.send(200, "application/json", "{\"mode\":\"automatic\"}");
  });

  // TEST: Test LED hardware - cicla colori
  server.on("/api/test/led", HTTP_GET, []() {
    // Rosso
    statusLed.setPixelColor(0, statusLed.Color(255, 0, 0));
    statusLed.show();
    delay(500);
    // Verde
    statusLed.setPixelColor(0, statusLed.Color(0, 255, 0));
    statusLed.show();
    delay(500);
    // Blu
    statusLed.setPixelColor(0, statusLed.Color(0, 0, 255));
    statusLed.show();
    delay(500);
    // Spento
    statusLed.setPixelColor(0, 0);
    statusLed.show();
    server.send(200, "application/json", "{\"test\":\"led_cycle_complete\"}");
  });

  // Forza presenza ON (test)
  server.on("/api/presence/on", HTTP_GET, []() {
    presenceDetected = true;
    notifyClients("presence", "true");
    server.send(200, "application/json", "{\"presence\":true,\"forced\":true}");
    DEBUG_PRINTLN("[API] Forzato presenza ON");
  });

  // Forza presenza OFF (test)
  server.on("/api/presence/off", HTTP_GET, []() {
    presenceDetected = false;
    notifyClients("presence", "false");
    server.send(200, "application/json", "{\"presence\":false,\"forced\":true}");
    DEBUG_PRINTLN("[API] Forzato presenza OFF");
  });

  // Riavvia dispositivo
  server.on("/api/restart", HTTP_GET, []() {
    server.send(200, "text/html", "<html><body><h1>Riavvio in corso...</h1><script>setTimeout(()=>location='/',5000)</script></body></html>");
    delay(500);
    ESP.restart();
  });

  // Reset credenziali WiFi - riavvia in modalità configurazione
  server.on("/api/wifi/reset", HTTP_GET, []() {
    server.send(200, "text/html", R"rawliteral(
<html><body style='background:#1a1a2e;color:#fff;font-family:Arial;text-align:center;padding:50px'>
<h1 style='color:#ff4444'>Reset WiFi</h1>
<p>Le credenziali WiFi verranno cancellate.</p>
<p>Il dispositivo si riavviera in modalita configurazione.</p>
<p style='color:#00d4ff'>Connettiti alla rete: <b>RadarServer-Setup</b></p>
<p>(Rete aperta, senza password)</p>
<p>Poi vai su <b>http://192.168.4.1</b></p>
<script>setTimeout(()=>{},3000)</script>
</body></html>
)rawliteral");
    delay(1000);
    resetWiFiSettings();
  });

  // Info WiFi corrente
  server.on("/api/wifi/info", HTTP_GET, []() {
    String json = "{";
    json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
    json += "\"subnet\":\"" + WiFi.subnetMask().toString() + "\",";
    json += "\"dns\":\"" + WiFi.dnsIP().toString() + "\",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"hostname\":\"" + String(hostname) + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });

  // ========== API RELE ==========
  // Stato relè
  server.on("/api/relay", HTTP_GET, []() {
    String json = "{";
    json += "\"state\":" + String(relayState ? "true" : "false") + ",";
    json += "\"autoMode\":" + String(relayAutoMode ? "true" : "false") + ",";
    json += "\"inverted\":" + String(relayInverted ? "true" : "false") + ",";
    json += "\"pin\":" + String(RELAY_PIN);
    json += "}";
    server.send(200, "application/json", json);
  });

  // Accendi relè
  server.on("/api/relay/on", HTTP_GET, []() {
    relayAutoMode = false;  // Disabilita auto mode su comando manuale
    setRelay(true);
    saveSettings();
    server.send(200, "application/json", "{\"relay\":\"on\",\"autoMode\":false}");
  });

  // Spegni relè
  server.on("/api/relay/off", HTTP_GET, []() {
    relayAutoMode = false;  // Disabilita auto mode su comando manuale
    setRelay(false);
    saveSettings();
    server.send(200, "application/json", "{\"relay\":\"off\",\"autoMode\":false}");
  });

  // Toggle relè
  server.on("/api/relay/toggle", HTTP_GET, []() {
    relayAutoMode = false;  // Disabilita auto mode su comando manuale
    toggleRelay();
    saveSettings();
    String json = "{\"relay\":\"" + String(relayState ? "on" : "off") + "\",\"autoMode\":false}";
    server.send(200, "application/json", json);
  });

  // Abilita/disabilita auto mode (relè segue presenza)
  server.on("/api/relay/auto", HTTP_GET, []() {
    relayAutoMode = !relayAutoMode;
    saveSettings();
    String json = "{\"autoMode\":" + String(relayAutoMode ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  // Imposta auto mode ON
  server.on("/api/relay/auto/on", HTTP_GET, []() {
    relayAutoMode = true;
    saveSettings();
    updateRelayAuto();  // Applica subito
    server.send(200, "application/json", "{\"autoMode\":true}");
  });

  // Imposta auto mode OFF
  server.on("/api/relay/auto/off", HTTP_GET, []() {
    relayAutoMode = false;
    saveSettings();
    server.send(200, "application/json", "{\"autoMode\":false}");
  });

  // Inverti logica (relè ON quando assenza)
  server.on("/api/relay/invert", HTTP_GET, []() {
    relayInverted = !relayInverted;
    saveSettings();
    if (relayAutoMode) updateRelayAuto();  // Applica subito se in auto
    String json = "{\"inverted\":" + String(relayInverted ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  DEBUG_PRINTLN("WebServer avviato");
}

// ========== WIFI CON WIFIMANAGER ==========
void setupWiFi() {
  DEBUG_PRINTLN("Inizializzazione WiFi...");

  // LED BLU durante connessione WiFi
  setLedColor(statusLed.Color(0, 0, 255));

  // Display: inizializzazione WiFi
  if (displayInitialized) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("WiFi...");
    display.display();
  }

  // Leggi contatore tentativi WiFi falliti dalla NVS
  preferences.begin("radar", false);
  int wifiFailCount = preferences.getInt("wifiFailCnt", 0);
  DEBUG_PRINTF("Contatore tentativi WiFi falliti: %d\n", wifiFailCount);

  // IMPORTANTE per ESP32-C3: impostare WIFI_STA e persistent PRIMA di tutto
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(hostname);
  delay(500);

  // PRIMA prova a connettersi con credenziali salvate (senza WiFiManager)
  DEBUG_PRINTLN("Tentativo connessione con credenziali salvate...");
  WiFi.begin();  // Usa credenziali salvate nella NVS

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {  // 5 secondi
    delay(500);
    DEBUG_PRINT(".");
    attempts++;
    // Aggiorna display con tentativo corrente
    if (displayInitialized) {
      // Pulisce la riga prima di scrivere per evitare sovrapposizioni
      display.fillRect(0, 10, OLED_WIDTH, 10, SSD1306_BLACK);
      display.setCursor(0, 10);
      display.print("Tentativo ");
      display.print(attempts);
      display.print("/10");
      display.display();
    }
  }
  DEBUG_PRINTLN();

  // Se connesso, non serve WiFiManager
  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINTLN("Connesso con credenziali salvate!");
    DEBUG_PRINTF("SSID: %s\n", WiFi.SSID().c_str());
    DEBUG_PRINTF("IP: %s\n", WiFi.localIP().toString().c_str());
    DEBUG_PRINTF("RSSI: %d dBm\n", WiFi.RSSI());

    // Reset contatore tentativi falliti
    preferences.putInt("wifiFailCnt", 0);
    preferences.end();

    WiFi.setSleep(WIFI_PS_NONE);  // Disabilita power saving

    // mDNS
    if (MDNS.begin(hostname)) {
      DEBUG_PRINTF("mDNS: http://%s.local\n", hostname);
    }

    setLedColor(statusLed.Color(0, 255, 0));  // VERDE

    // Display: connesso con successo
    if (displayInitialized) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("WiFi OK");
      display.setCursor(0, 10);
      display.println(WiFi.SSID());
      display.setCursor(0, 20);
      display.println(WiFi.localIP().toString());
      display.display();
    }

    delay(1000);
    return;  // Esci, connessione riuscita
  }

  // Connessione fallita - strategia basata su contatore
  if (wifiFailCount == 0) {
    // Primo tentativo fallito -> incrementa contatore e riavvia
    DEBUG_PRINTLN("Primo tentativo fallito, riavvio ESP...");
    preferences.putInt("wifiFailCnt", 1);
    preferences.end();

    // LED ARANCIONE per indicare riavvio
    setLedColor(statusLed.Color(255, 128, 0));

    if (displayInitialized) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 10);
      display.println("WiFi fallito");
      display.setCursor(0, 20);
      display.println("Riavvio...");
      display.display();
    }

    delay(2000);
    ESP.restart();
  }

  // Secondo tentativo fallito -> avvia Access Point
  DEBUG_PRINTLN("Secondo tentativo fallito, avvio Access Point...");
  preferences.putInt("wifiFailCnt", 0);  // Reset contatore per prossimo ciclo
  preferences.end();

  // LED MAGENTA per indicare AP mode
  setLedColor(statusLed.Color(255, 0, 255));

  // Display: avvio Access Point
  if (displayInitialized) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi fallito");
    display.setCursor(0, 10);
    display.println("Avvio AP...");
    display.display();
  }
  delay(1000);

  WiFiManager wifiManager;

  // Timeout per il tentativo di connessione
  wifiManager.setConnectTimeout(5);

  // Timeout per il portale di configurazione
  wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);

  // Callback quando entra in modalità configurazione
  wifiManager.setAPCallback([](WiFiManager *myWiFiManager) {
    DEBUG_PRINTLN("===========================================");
    DEBUG_PRINTLN("  MODALITA' CONFIGURAZIONE WIFI ATTIVA");
    DEBUG_PRINTLN("===========================================");
    DEBUG_PRINTF("Connettiti alla rete: %s\n", WIFI_AP_NAME);
    DEBUG_PRINTLN("(Rete aperta, senza password)");
    DEBUG_PRINTLN("Poi vai su http://192.168.4.1");
    DEBUG_PRINTLN("===========================================");

    // LED MAGENTA lampeggiante durante AP mode
    setLedColor(statusLed.Color(255, 0, 255));

    // Mostra istruzioni sul display OLED
    if (displayInitialized) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("SETUP WIFI");
      display.setCursor(0, 10);
      display.println(WIFI_AP_NAME);
      display.setCursor(0, 20);
      display.println("192.168.4.1");
      display.display();
    }
  });

  // Callback quando salva nuove credenziali
  wifiManager.setSaveConfigCallback([]() {
    DEBUG_PRINTLN("Nuove credenziali WiFi salvate!");
    setLedColor(statusLed.Color(0, 255, 0));  // VERDE
  });

  // Tenta connessione automatica o avvia portale AP
  // autoConnect: prova a connettersi con credenziali salvate
  // Se fallisce, avvia AP aperto (senza password)
  bool connected = wifiManager.autoConnect(WIFI_AP_NAME);

  if (connected) {
    DEBUG_PRINTLN("\n===========================================");
    DEBUG_PRINTLN("         WIFI CONNESSO!");
    DEBUG_PRINTF("SSID: %s\n", WiFi.SSID().c_str());
    DEBUG_PRINTF("IP: %s\n", WiFi.localIP().toString().c_str());
    DEBUG_PRINTF("RSSI: %d dBm\n", WiFi.RSSI());
    DEBUG_PRINTLN("===========================================");

    // Disabilita completamente power saving WiFi
    WiFi.setSleep(WIFI_PS_NONE);
    WiFi.setAutoReconnect(true);

    // LED VERDE breve poi spento
    setLedColor(statusLed.Color(0, 255, 0));
    delay(500);

    // mDNS
    if (MDNS.begin(hostname)) {
      DEBUG_PRINTF("mDNS: http://%s.local\n", hostname);
    }
  } else {
    DEBUG_PRINTLN("\n===========================================");
    DEBUG_PRINTLN("  ERRORE: Timeout configurazione WiFi!");
    DEBUG_PRINTLN("  Riavvio in 5 secondi...");
    DEBUG_PRINTLN("===========================================");

    // LED ROSSO
    setLedColor(statusLed.Color(255, 0, 0));

    if (displayInitialized) {
      display.clearDisplay();
      display.setCursor(0, 10);
      display.println("WiFi TIMEOUT");
      display.setCursor(0, 20);
      display.println("Riavvio...");
      display.display();
    }

    delay(5000);
    ESP.restart();
  }
}

// Funzione per resettare le credenziali WiFi
void resetWiFiSettings() {
  DEBUG_PRINTLN("Reset credenziali WiFi...");
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  DEBUG_PRINTLN("Credenziali cancellate. Riavvio...");
  delay(1000);
  ESP.restart();
}

void setupOTA() {
  ArduinoOTA.setHostname(hostname);

  ArduinoOTA.onStart([]() {
    DEBUG_PRINTLN("OTA Start");
    otaInProgress = true;
    setLedColor(statusLed.Color(0, 255, 255));  // CIANO
  });

  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN("\nOTA End");
    otaInProgress = false;
    setLedColor(statusLed.Color(0, 255, 0));  // VERDE
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINTF("OTA Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINTF("OTA Error[%u]\n", error);
    otaInProgress = false;
    setLedColor(statusLed.Color(255, 0, 0));  // ROSSO
  });

  ArduinoOTA.begin();
}

// ========== SETUP ==========
void setup() {
  // PRIMISSIMA COSA: Relè OFF (prima di tutto, anche prima di Serial)
  // Per HIGH level trigger: LOW = OFF
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Forza LOW immediatamente (relè spento)

  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);

  DEBUG_PRINTLN("\n========================================");
  DEBUG_PRINTLN("    RADAR SERVER ESP32-C3");
  DEBUG_PRINTLN("========================================\n");

  // Pin setup
  pinMode(PULSE_PIN, OUTPUT);
  digitalWrite(PULSE_PIN, LOW);  // Assicura che parta spento

  // Inizializza LED WS2812 (prima di tutto per feedback visivo)
  DEBUG_PRINTLN("Inizializzazione LED WS2812...");
  initStatusLed();

  // Carica impostazioni
  loadSettings();
  loadClients();

  // Inizializza Relè (dopo loadSettings per applicare stato salvato)
  DEBUG_PRINTLN("Inizializzazione Relay...");
  initRelay();

  // Inizializza Display OLED
  DEBUG_PRINTLN("Inizializzazione Display...");
  initDisplay();

  // Inizializza BME280
  DEBUG_PRINTLN("Inizializzazione BME280...");
  initBME280();

  // Inizializza radar
  DEBUG_PRINTLN("Inizializzazione Radar...");
  initRadar();

  // WiFi
  DEBUG_PRINTLN("Connessione WiFi...");
  setupWiFi();

  // OTA
  setupOTA();

  // WebServer
  setupWebServer();

  DEBUG_PRINTLN("\n========================================");
  DEBUG_PRINTLN("         SISTEMA PRONTO");
  DEBUG_PRINTF("IP: %s\n", WiFi.localIP().toString().c_str());
  DEBUG_PRINTF("Hostname: %s.local\n", hostname);
  DEBUG_PRINTLN("========================================\n");

  // mDNS Auto-Discovery iniziale (dopo 3 secondi per dare tempo ai device di annunciarsi)
  DEBUG_PRINTLN("Avvio mDNS discovery iniziale tra 3 secondi...");
  delay(3000);
  int foundDevices = discoverClients();
  DEBUG_PRINTF("Discovery iniziale completata: %d device trovati\n", foundDevices);
  lastDiscoveryTime = millis();  // Reset timer per evitare doppia scansione

  // Mostra IP sul display
  if (displayInitialized) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("SISTEMA PRONTO");
    display.setCursor(0, 12);
    display.print("IP:");
    display.println(WiFi.localIP().toString());
    display.setCursor(0, 24);
    display.print("Radar: ");
    display.println(radarInitialized ? "OK" : "ERRORE");
    display.display();
    delay(2000);
  }
}

// ========== LOOP ==========
void loop() {
  // OTA
  ArduinoOTA.handle();

  // WebServer
  server.handleClient();

  // Aggiorna cache radar
  updateRadarCache();

  // Aggiorna sensore luce
  updateLightSensor();

  // Aggiorna BME280 (temperatura e umidità)
  updateBME280();

  // Gestione presenza
  updatePresence();

  // Gestione relè automatico (segue presenza se abilitato)
  updateRelayAuto();

  // Gestione impulso cambio presenza
  updatePulse();

  // Gestione impulso relè (pulsante 2 sec)
  updateRelayPulse();

  // Aggiorna LED WS2812 di stato
  updateStatusLed();

  // Aggiorna display OLED
  updateDisplay();

  // Notifica periodica ai client
  // IMPORTANTE: inviamo ambientLight (luce RAW 0-255), NON calculatedBrightness
  // Il client mappa questo valore secondo i suoi parametri radarBrightnessMin/Max
  static int lastSentLight = -1;
  static unsigned long lastFullSync = 0;

  // Notifica cambio luminosità SOLO se c'è presenza (ogni 1 secondo se cambio >= 2)
  if (presenceDetected && millis() - lastClientNotify > CLIENT_NOTIFY_MS) {
    if (abs(ambientLight - lastSentLight) >= 2) {
      notifyClients("brightness", String(ambientLight));
      lastSentLight = ambientLight;
    }
    lastClientNotify = millis();
  }

  // Sincronizzazione completa ogni 10 secondi
  // ORDINE IMPORTANTE: prima presence, poi brightness, poi sensori
  if (millis() - lastFullSync > 10000) {
    notifyClients("presence", presenceDetected ? "true" : "false");
    if (presenceDetected) {
      notifyClients("brightness", String(ambientLight));
      lastSentLight = ambientLight;
    }
    // Invia temperatura e umidità (se BME280 disponibile)
    if (bme280Initialized) {
      notifyClients("temperature", String(bmeTemperature, 1));
      notifyClients("humidity", String(bmeHumidity, 1));
    }
    lastFullSync = millis();
  }

  // Riconnessione WiFi se disconnesso - riavvia dopo 3 tentativi (ogni 10 sec)
  static unsigned long lastWifiCheck = 0;
  static int wifiFailCount = 0;

  if (millis() - lastWifiCheck > 10000) {  // Controlla ogni 10 secondi
    if (WiFi.status() != WL_CONNECTED) {
      wifiFailCount++;
      DEBUG_PRINTF("WiFi disconnesso, tentativo %d/3\n", wifiFailCount);
      WiFi.reconnect();

      if (wifiFailCount >= 3) {
        DEBUG_PRINTLN("WiFi non recuperabile, riavvio...");
        ESP.restart();
      }
    } else {
      wifiFailCount = 0;
    }
    lastWifiCheck = millis();
  }

  // mDNS Auto-Discovery periodica
  if (autoDiscoveryEnabled && WiFi.status() == WL_CONNECTED) {
    if (millis() - lastDiscoveryTime > discoveryInterval) {
      DEBUG_PRINTLN("[AUTO] Scansione mDNS periodica...");
      discoverClients();
      lastDiscoveryTime = millis();
    }
  }

  // Auto-ping client per verificare stato online/offline
  if (WiFi.status() == WL_CONNECTED) {
    autoPingClients();
  }
}
