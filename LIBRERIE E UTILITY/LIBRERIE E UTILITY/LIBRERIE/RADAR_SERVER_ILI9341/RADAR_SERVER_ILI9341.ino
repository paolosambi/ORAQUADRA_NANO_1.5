// ============================================================================
// RADAR SERVER - ESP32 con Display ILI9341 320x240 + Touch Screen
// SERVER autonomo con sensore LD2410 locale + OTA Updates
// Funzionalita complete importate da RADAR_SERVER_C3
// ============================================================================
// Autore: Sambinello Paolo
// Hardware: ESP32 + ILI9341 320x240 TFT + XPT2046 Touch + LD2410 + BME280
// Libreria: TFT_eSPI (touch integrato), MyLD2410
// ============================================================================

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <MyLD2410.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>

// ========== CONFIGURAZIONE WIFI ==========
const char* ssid = "";           // <-- INSERISCI IL TUO SSID
const char* password = "";   // <-- INSERISCI LA TUA PASSWORD

// ========== SI RACCOMANDA CONFIGURAZIONE IP STATICO ==========
IPAddress staticIP(192, 168, 1, 99);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);

// ========== CONFIGURAZIONE RADAR LD2410 ==========
#define LD2410_RX_PIN 16   // GPIO16 - RX2 (collegare a TX del LD2410)
#define LD2410_TX_PIN 17   // GPIO17 - TX2 (collegare a RX del LD2410)
#define LD2410_BAUD_RATE 256000
HardwareSerial radarSerial(2);  // Usa Serial2

// Sensibilita radar per gate (0=piu sensibile, 100=meno sensibile)
// Ogni gate copre circa 75cm: Gate0=0-75cm, Gate1=75-150cm, Gate2=150-225cm, ecc.
// VALORI PIU BASSI = RADAR PIU SENSIBILE
#define RADAR_MAX_MOVING_GATE 8       // Gate massimo movimento (8 = 600cm)
#define RADAR_MAX_STATIONARY_GATE 8   // Gate massimo stazionario
// Valori default sensibilita (possono essere modificati da web)
// Questi valori sono le SOGLIE di energia: se l'energia rilevata supera la soglia, rileva presenza
// VALORI ALTI = PIU SENSIBILE (soglia piu facile da superare)
#define RADAR_SENS_CLOSE_DEFAULT 40    // Vicino (0-225cm)
#define RADAR_SENS_MID_DEFAULT 60      // Medio (225-450cm)
#define RADAR_SENS_FAR_DEFAULT 80      // Lontano (450-675cm)

// Variabili sensibilita radar (modificabili da web)
int radarSensClose = RADAR_SENS_CLOSE_DEFAULT;   // Sensibilita vicino (gate 0-2)
int radarSensMid = RADAR_SENS_MID_DEFAULT;       // Sensibilita medio (gate 3-5)
int radarSensFar = RADAR_SENS_FAR_DEFAULT;       // Sensibilita lontano (gate 6-8)

// ========== CONFIGURAZIONE DISPOSITIVO ==========
const char* deviceName = "RadarDisplay1";
const char* hostname = "radar-display";
#define DEBUG_MODE

// ========== DEBUG MACRO ==========
#ifdef DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(format, ...) Serial.printf(format, ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(format, ...)
#endif

// ========== COLORI DISPLAY ==========
#define COLOR_BG        0x0000  // Nero
#define COLOR_BG_CARD   0x18E3  // Grigio scuro elegante
#define COLOR_BG_CARD2  0x2124  // Grigio medio per contrasto
#define COLOR_TEXT      0xFFFF  // Bianco
#define COLOR_TEXT_DIM  0xC618  // Grigio chiaro per testo secondario
#define COLOR_TITLE     0x07FF  // Ciano
#define COLOR_GREEN     0x07E0  // Verde
#define COLOR_GREEN_DIM 0x0400  // Verde scuro
#define COLOR_RED       0xF800  // Rosso
#define COLOR_RED_DIM   0x8000  // Rosso scuro
#define COLOR_YELLOW    0xFFE0  // Giallo
#define COLOR_BLUE      0x001F  // Blu
#define COLOR_BLUE_ACC  0x04FF  // Blu accent
#define COLOR_ORANGE    0xFD20  // Arancione
#define COLOR_GRAY      0x8410  // Grigio
#define COLOR_GRAY_DARK 0x4208  // Grigio scuro bordi
#define COLOR_CYAN      0x07FF  // Ciano
#define COLOR_MAGENTA   0xF81F  // Magenta
#define COLOR_ACCENT    0x04B3  // Colore accent (azzurro)

// ========== CONFIGURAZIONE HARDWARE AGGIUNTIVO ==========
#define LED_PIN 25          // GPIO per LED PWM (opzionale)
#define LED_CHANNEL 0
#define LED_FREQ 5000
#define LED_RESOLUTION 8

// ========== CONTROLLO DISPLAY PWM ==========
#define DISPLAY_PWM_PIN 5       // GPIO5 per luminosita display (PWM)
#define DISPLAY_PWM_CHANNEL 1   // Canale PWM (diverso da LED_CHANNEL)
#define DISPLAY_PWM_FREQ 5000
#define DISPLAY_PWM_RESOLUTION 8
int displayBrightness = 255;    // Luminosita corrente display (0-255)
int displayMinBright = 30;      // Luminosita minima (mai completamente buio se acceso)
int displayMaxBright = 255;     // Luminosita massima
bool displayEnabled = true;     // Display abilitato (presenza)

// ========== CONFIGURAZIONE RELE ==========
#define RELAY_PIN 26              // GPIO26 per rele
#define RELAY_PULSE_MS 2000       // Durata impulso rele (2 secondi)
#define RELAY_BOOT_DELAY_MS 5000  // Ritardo prima di abilitare rele dopo boot
bool relayActive = false;
unsigned long relayStartTime = 0;
bool relayEnabled = false;        // Disabilitato al boot
unsigned long bootTime = 0;       // Timestamp avvio
bool pendingRelayPulse = false;   // Attivazione rele pendente (durante boot delay)

// ========== SENSORE MONITOR TEMT6000 ==========
#define MONITOR_SENSOR_PIN 34         // GPIO34 (ADC1) per TEMT6000 puntato al monitor
#define MONITOR_SENSOR_UPDATE_MS 500  // Lettura ogni 500ms
#define MONITOR_THRESHOLD_ON 150      // Soglia per considerare monitor ACCESO (0-4095)
#define MONITOR_THRESHOLD_OFF 50      // Soglia per considerare monitor SPENTO (isteresi)
#define MONITOR_READINGS_AVG 5        // Numero letture per media (anti-rumore)
int monitorLightLevel = 0;            // Livello luce rilevato dal sensore (0-4095)
bool monitorDetectedOn = false;       // Stato rilevato: true = monitor acceso
bool monitorStateDesired = false;     // Stato desiderato: true = vogliamo monitor acceso
unsigned long lastMonitorSensorRead = 0;
bool monitorSyncEnabled = true;       // Sync relè abilitato

// ========== CONFIGURAZIONE WS2812 ==========
#define WS2812_PIN 27             // GPIO27 per WS2812
#define WS2812_NUM_LEDS 1         // Numero di LED (1 per status)
Adafruit_NeoPixel statusLed(WS2812_NUM_LEDS, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// Colori LED stato (formato RGB)
#define LED_COLOR_OFF       0x000000  // Spento
#define LED_COLOR_BOOT      0x800080  // Viola - Avvio
#define LED_COLOR_WIFI      0x0000FF  // Blu - Connessione WiFi
#define LED_COLOR_READY     0x00FF00  // Verde - Pronto/Idle
#define LED_COLOR_PRESENCE  0xFFFF00  // Giallo - Presenza rilevata
#define LED_COLOR_RADAR     0x00FFFF  // Ciano - Lettura radar
#define LED_COLOR_OTA       0xFF00FF  // Magenta - Aggiornamento OTA
#define LED_COLOR_ERROR     0xFF0000  // Rosso - Errore
#define LED_COLOR_RELAY     0xFFA500  // Arancione - Rele attivo
#define LED_COLOR_WEB       0xFFFFFF  // Bianco - Richiesta web

uint32_t currentLedColor = LED_COLOR_OFF;
uint32_t previousLedColor = LED_COLOR_OFF;

// ========== CONFIGURAZIONE BME280 ==========
#define BME280_SDA 21       // GPIO21 - SDA (default ESP32)
#define BME280_SCL 22       // GPIO22 - SCL (default ESP32)
#define BME280_ADDR 0x76    // Indirizzo I2C BME280 (può essere 0x77)
Adafruit_BME280 bme;
bool bme280Initialized = false;
float bmeTemperature = 0.0;
float bmeHumidity = 0.0;
float bmePressure = 0.0;
float tempOffset = 0.0;
float humOffset = 0.0;
#define BME280_UPDATE_MS 2000
unsigned long lastBME280Update = 0;

// ========== CONFIGURAZIONE NTP ==========
const char* ntpServer = "pool.ntp.org";
const char* ntpServer2 = "time.google.com";
const char* timezone = "CET-1CEST,M3.5.0,M10.5.0/3";  // Fuso orario Roma
bool ntpSynced = false;
unsigned long lastTimeUpdate = 0;
#define TIME_UPDATE_MS 1000
String currentDay = "";
String currentDate = "";
String currentTime = "";

// ========== COSTANTI TIMING ==========
#define POLLING_MODE true
#define POLLING_INTERVAL_MS 1000
#define DISPLAY_UPDATE_MS 100
#define RADAR_READ_INTERVAL_MS 50   // 50ms - lettura frequente per non perdere dati
#define PRESENCE_CHECK_MS 500
#define CLIENT_NOTIFY_MS 300
#define LIGHT_SENSOR_UPDATE_MS 500

// ========== OGGETTI GLOBALI ==========
TFT_eSPI tft = TFT_eSPI();
WebServer server(80);
Preferences preferences;
MyLD2410 radar(radarSerial);

// ========== VARIABILI OTA ==========
bool otaInProgress = false;
int otaProgress = 0;

// ========== VARIABILI STATO RADAR ==========
bool presenceDetected = false;
bool previousPresenceState = false;
bool radarOnline = false;
bool radarInitialized = false;
unsigned long lastPresenceTime = 0;
int radarFailCount = 0;                 // Contatore errori consecutivi radar
#define RADAR_MAX_FAIL_COUNT 10         // Numero massimo errori prima di considerare disconnesso

// Dati dal sensore LD2410
int radarDistance = 0;
int radarMovingDistance = 0;
int radarStationaryDistance = 0;
int radarMovingEnergy = 0;
int radarStationaryEnergy = 0;
bool radarMoving = false;
bool radarStationary = false;
int ambientLight = 0;
String radarFirmware = "";

// ========== VARIABILI CONTROLLO AUTOMATICO ==========
bool autoPresenceEnabled = true;
bool autoBrightnessEnabled = true;
// manualPresenceOverride RIMOSSO - il radar gestisce SEMPRE automaticamente
unsigned long presenceTimeout = 10000;  // 10 secondi timeout assenza (come DISPLAY_8x48)
int minBrightness = 10;
int maxBrightness = 255;
int calculatedBrightness = 128;
int currentBrightness = 128;
int targetBrightness = 128;

// ========== TIMING ==========
unsigned long lastRadarRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastBrightnessUpdate = 0;
unsigned long lastPresenceCheck = 0;
unsigned long lastClientNotify = 0;
unsigned long lastLightSensorUpdate = 0;
unsigned long lastFullSync = 0;

// ========== TOUCH ==========
bool touchPressed = false;
uint16_t touchX = 0;
uint16_t touchY = 0;
uint16_t touchCalData[5] = {0, 0, 0, 0, 0};  // Dati calibrazione touch
bool touchCalibrated = false;                  // Flag calibrazione valida

// ========== UI STATE ==========
int currentScreen = 0;  // 0=Main, 1=Radar
bool needsFullRedraw = true;

// Valori precedenti per aggiornamento parziale (anti-flicker)
int prevDistance = -1;
bool prevPresence = false;
int prevBrightness = -1;
bool prevRadarOnline = false;
bool prevMoving = false;
bool prevStationary = false;
int prevAmbientLight = -1;
String prevTime = "";
String prevDate = "";
float prevTemp = -999;
float prevHum = -999;

// Stringhe precedenti per aggiornamento senza flickering
char prevStatusStr[12] = "";
char prevPresenceStr[10] = "";
char prevMovingStr[6] = "";
char prevStationaryStr[6] = "";
char prevDistanceStr[8] = "";
char prevRawStr[8] = "";
char prevPwmStr[8] = "";
char prevPctStr[8] = "";
// Per schermata radar
char prevRadarDistStr[12] = "";
char prevRadarStateStr[10] = "";
char prevMovEnergyStr[6] = "";
char prevStatEnergyStr[6] = "";
char prevLightStr[6] = "";
// Per header
char prevHeaderTime[12] = "";
char prevHeaderDate[20] = "";
char prevHeaderTemp[10] = "";
char prevHeaderHum[8] = "";
bool headerDrawn = false;
// Per indicatore monitor
bool prevMonitorDetected = false;
bool prevMonitorInSync = true;
char prevMonitorLevelStr[8] = "";

// ========== STRUTTURE UI ==========
struct Button {
  int x, y, w, h;
  const char* label;
  uint16_t color;
  bool pressed;
};

Button btnOnOff = {4, 190, 100, 40, "ACCENDI", COLOR_GREEN, false};
bool manualAllOn = true;  // Stato toggle: true=prossimo comando ACCENDI, false=prossimo comando SPEGNI
Button btnSettings = {110, 190, 100, 40, "RADAR", COLOR_ORANGE, false};
Button btnRelay = {216, 190, 100, 40, "RELE", COLOR_RED, false};
Button btnBack = {110, 190, 100, 40, "INDIETRO", COLOR_GRAY, false};

// Forward declaration
void drawButton(Button &btn, bool highlight = false);

// ========== GESTIONE DISPOSITIVI ESTERNI ==========
#define MAX_CLIENTS 10
#define CLIENT_HTTP_TIMEOUT 200       // Timeout HTTP ridotto (ms)
#define CLIENT_CONNECT_TIMEOUT 150    // Timeout connessione ridotto (ms)

struct ClientDevice {
  String ip;
  String name;
  uint16_t port;
  bool active;
  bool notifyPresence;
  bool notifyBrightness;
  bool controlOnOff;      // Abilita controllo ON/OFF tramite radar
  bool currentState;      // Stato attuale (true=ON, false=OFF)
  unsigned long lastSeen;
  uint8_t failCount;      // Contatore fallimenti (per statistiche)
};
ClientDevice clients[MAX_CLIENTS];
int clientCount = 0;
bool lastNotifiedPresence = false;
int lastSentLight = -1;

// ========== DISCOVERY DISPOSITIVI ==========
#define DISCOVERY_TIMEOUT_MS 50      // Timeout per ogni IP (molto breve)
#define DISCOVERY_INTERVAL_MS 10     // Intervallo tra scansioni IP
#define DISCOVERY_PORT 80            // Porta da scansionare
bool discoveryActive = false;        // Discovery in corso
bool discoveryEnabled = false;       // Auto-discovery DISABILITATO di default
int discoveryCurrentIP = 1;          // IP corrente nella scansione (1-254)
int discoveryEndIP = 254;            // Ultimo IP da scansionare
int discoveryFoundCount = 0;         // Dispositivi trovati nell'ultima scansione
unsigned long lastDiscoveryStep = 0; // Timing per step non bloccante
unsigned long lastDiscoveryRun = 0;  // Ultima scansione completa
#define DISCOVERY_AUTO_INTERVAL 0    // Auto-discovery disabilitato (0=off)

// ========== FUNZIONI LED ==========
void setupLED() {
  ledcSetup(LED_CHANNEL, LED_FREQ, LED_RESOLUTION);
  ledcAttachPin(LED_PIN, LED_CHANNEL);
  ledcWrite(LED_CHANNEL, 0);
}

void setLEDBrightness(int brightness) {
  brightness = constrain(brightness, 0, 255);
  ledcWrite(LED_CHANNEL, brightness);
  currentBrightness = brightness;
}

// ========== FUNZIONI CONTROLLO DISPLAY PWM ==========
void setupDisplayPWM() {
  ledcSetup(DISPLAY_PWM_CHANNEL, DISPLAY_PWM_FREQ, DISPLAY_PWM_RESOLUTION);
  ledcAttachPin(DISPLAY_PWM_PIN, DISPLAY_PWM_CHANNEL);
  ledcWrite(DISPLAY_PWM_CHANNEL, displayMaxBright);  // Display acceso all'avvio
  displayEnabled = true;
  displayBrightness = displayMaxBright;
  DEBUG_PRINTLN("Display PWM inizializzato su GPIO5");
}

void setDisplayBrightness(int brightness) {
  brightness = constrain(brightness, 0, 255);
  if (brightness != displayBrightness) {
    ledcWrite(DISPLAY_PWM_CHANNEL, brightness);
    displayBrightness = brightness;
  }
}

void setDisplayEnabled(bool enabled) {
  displayEnabled = enabled;
  if (!enabled) {
    // Spegni display completamente
    setDisplayBrightness(0);
    DEBUG_PRINTLN("Display: SPENTO");
  } else {
    DEBUG_PRINTLN("Display: ACCESO");
  }
}

// Aggiorna luminosita display in base alla luce ambiente
void updateDisplayBrightness() {
  if (!displayEnabled) return;

  // Mappa luce ambiente (0-255) su luminosita display
  // Luce bassa -> display piu luminoso, Luce alta -> display meno luminoso (risparmio)
  // Oppure: Luce bassa -> display basso, Luce alta -> display alto (segue ambiente)
  // Usiamo la seconda logica: segue la luce ambiente
  int targetBright = map(ambientLight, 0, 255, displayMinBright, displayMaxBright);
  targetBright = constrain(targetBright, displayMinBright, displayMaxBright);

  setDisplayBrightness(targetBright);
}

// ========== FUNZIONI RELE ==========
void setupRelay() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  relayActive = false;
  DEBUG_PRINTLN("Rele inizializzato su GPIO26");
}

bool startRelayPulse() {
  // RELE DISABILITATO - logica non ancora funzionante
  DEBUG_PRINTLN(">>> RELE: DISABILITATO <<<");
  return false;

  /* CODICE ORIGINALE COMMENTATO
  if (!relayEnabled) {
    DEBUG_PRINTLN(">>> RELE: Ignorato (boot in corso) <<<");
    return false;
  }
  if (!relayActive) {
    digitalWrite(RELAY_PIN, HIGH);
    relayActive = true;
    relayStartTime = millis();
    DEBUG_PRINTLN(">>> RELE: Impulso AVVIATO (2 sec) <<<");
    return true;
  }
  return false;
  */
}

void updateRelay() {
  if (relayActive && (millis() - relayStartTime >= RELAY_PULSE_MS)) {
    digitalWrite(RELAY_PIN, LOW);
    relayActive = false;
    DEBUG_PRINTLN(">>> RELE: Impulso TERMINATO <<<");
    // Ripristina colore pulsante e LED
    if (currentScreen == 0) {
      btnRelay.color = COLOR_RED;
      drawButton(btnRelay);
    }
    restorePreviousLedColor();
  }
}

// ========== FUNZIONI SENSORE MONITOR TEMT6000 ==========
void setupMonitorSensor() {
  // Configura ADC per GPIO34
  analogSetPinAttenuation(MONITOR_SENSOR_PIN, ADC_0db);  // Range 0-1.1V (massima sensibilità)
  analogReadResolution(12);  // 12 bit = 0-4095

  // Alcune letture di warmup (l'ADC ESP32 può essere instabile all'inizio)
  for (int i = 0; i < 10; i++) {
    analogRead(MONITOR_SENSOR_PIN);
    delay(10);
  }

  // Lettura iniziale
  monitorLightLevel = readMonitorSensor();
  monitorDetectedOn = (monitorLightLevel > MONITOR_THRESHOLD_ON);
  monitorStateDesired = monitorDetectedOn;  // Assume stato iniziale = stato rilevato
  DEBUG_PRINTF("Sensore monitor TEMT6000 inizializzato su GPIO%d, livello: %d, stato: %s\n",
               MONITOR_SENSOR_PIN, monitorLightLevel, monitorDetectedOn ? "ACCESO" : "SPENTO");
}

// Legge il sensore con media di più letture per ridurre rumore
int readMonitorSensor() {
  long sum = 0;
  for (int i = 0; i < MONITOR_READINGS_AVG; i++) {
    sum += analogRead(MONITOR_SENSOR_PIN);
    delayMicroseconds(100);  // Breve pausa tra letture
  }
  return sum / MONITOR_READINGS_AVG;
}

// Aggiorna lo stato rilevato del monitor con isteresi
void updateMonitorSensor() {
  unsigned long now = millis();
  if (now - lastMonitorSensorRead < MONITOR_SENSOR_UPDATE_MS) return;
  lastMonitorSensorRead = now;

  monitorLightLevel = readMonitorSensor();

  // Isteresi per evitare oscillazioni
  bool previousState = monitorDetectedOn;
  if (monitorDetectedOn) {
    // Se era acceso, deve scendere sotto THRESHOLD_OFF per considerarlo spento
    if (monitorLightLevel < MONITOR_THRESHOLD_OFF) {
      monitorDetectedOn = false;
    }
  } else {
    // Se era spento, deve superare THRESHOLD_ON per considerarlo acceso
    if (monitorLightLevel > MONITOR_THRESHOLD_ON) {
      monitorDetectedOn = true;
    }
  }

  if (previousState != monitorDetectedOn) {
    DEBUG_PRINTF(">>> MONITOR: Stato cambiato a %s (luce: %d) <<<\n",
                 monitorDetectedOn ? "ACCESO" : "SPENTO", monitorLightLevel);
  }
}

// Sincronizza lo stato del monitor con lo stato desiderato
// Attiva il relè se c'è discrepanza tra stato rilevato e stato desiderato
void syncMonitorState() {
  if (!monitorSyncEnabled) return;
  if (!relayEnabled) return;  // Non sincronizzare durante boot
  if (relayActive) return;    // Non interferire se relè già attivo

  // Se lo stato rilevato non corrisponde a quello desiderato, attiva il relè
  if (monitorDetectedOn != monitorStateDesired) {
    DEBUG_PRINTF(">>> SYNC MONITOR: Stato rilevato=%s, desiderato=%s -> Attivo RELE <<<\n",
                 monitorDetectedOn ? "ON" : "OFF",
                 monitorStateDesired ? "ON" : "OFF");
    startRelayPulse();
  }
}

// Imposta lo stato desiderato del monitor (chiamata quando si vuole accendere/spegnere)
void setMonitorDesiredState(bool wantOn) {
  DEBUG_PRINTF(">>> MONITOR: Stato desiderato impostato a %s <<<\n", wantOn ? "ACCESO" : "SPENTO");
  monitorStateDesired = wantOn;
  // La sincronizzazione verrà fatta da syncMonitorState() nel loop
}

// Funzione per toggle manuale - inverte lo stato desiderato
void toggleMonitorState() {
  setMonitorDesiredState(!monitorStateDesired);
}

// ========== FUNZIONI WS2812 STATUS LED ==========
void setupStatusLed() {
  statusLed.begin();
  statusLed.setBrightness(50);  // Luminosita moderata (0-255)
  statusLed.show();
  DEBUG_PRINTLN("WS2812 Status LED inizializzato su GPIO27");
}

void setStatusLed(uint32_t color) {
  if (color != currentLedColor) {
    previousLedColor = currentLedColor;
    currentLedColor = color;
    statusLed.setPixelColor(0, color);
    statusLed.show();
  }
}

void restorePreviousLedColor() {
  setStatusLed(previousLedColor);
}

void blinkStatusLed(uint32_t color, int times, int delayMs) {
  uint32_t savedColor = currentLedColor;
  for (int i = 0; i < times; i++) {
    setStatusLed(color);
    delay(delayMs);
    setStatusLed(LED_COLOR_OFF);
    delay(delayMs);
  }
  setStatusLed(savedColor);
}

// Funzioni di stato specifiche
void ledBoot() { setStatusLed(LED_COLOR_BOOT); }
void ledWiFiConnecting() { setStatusLed(LED_COLOR_WIFI); }
void ledReady() { setStatusLed(LED_COLOR_OFF); }  // LED spento quando pronto
void ledPresenceDetected() { setStatusLed(LED_COLOR_OFF); }  // LED spento con presenza
void ledPresenceGone() { setStatusLed(LED_COLOR_OFF); }  // LED spento
void ledRadarReading() { setStatusLed(LED_COLOR_RADAR); }
void ledOtaUpdate() { setStatusLed(LED_COLOR_OTA); }
void ledError() { setStatusLed(LED_COLOR_ERROR); }
void ledRelayActive() { setStatusLed(LED_COLOR_RELAY); }
void ledWebRequest() { setStatusLed(LED_COLOR_WEB); }

// ========== FUNZIONI SALVATAGGIO/CARICAMENTO IMPOSTAZIONI ==========
void saveSettings() {
  preferences.begin("radar", false);
  preferences.putBool("autoPresence", autoPresenceEnabled);
  preferences.putBool("autoBright", autoBrightnessEnabled);
  preferences.putULong("presTimeout", presenceTimeout);
  preferences.putInt("minBright", minBrightness);
  preferences.putInt("maxBright", maxBrightness);
  preferences.putFloat("tempOffset", tempOffset);
  preferences.putFloat("humOffset", humOffset);
  preferences.putInt("sensClose", radarSensClose);
  preferences.putInt("sensMid", radarSensMid);
  preferences.putInt("sensFar", radarSensFar);
  preferences.end();
  DEBUG_PRINTLN("Impostazioni salvate");
}

void loadSettings() {
  preferences.begin("radar", true);
  autoPresenceEnabled = preferences.getBool("autoPresence", true);
  autoBrightnessEnabled = preferences.getBool("autoBright", true);
  presenceTimeout = preferences.getULong("presTimeout", 5000);
  minBrightness = preferences.getInt("minBright", 10);
  maxBrightness = preferences.getInt("maxBright", 255);
  tempOffset = preferences.getFloat("tempOffset", 0.0);
  humOffset = preferences.getFloat("humOffset", 0.0);
  radarSensClose = preferences.getInt("sensClose", RADAR_SENS_CLOSE_DEFAULT);
  radarSensMid = preferences.getInt("sensMid", RADAR_SENS_MID_DEFAULT);
  radarSensFar = preferences.getInt("sensFar", RADAR_SENS_FAR_DEFAULT);
  preferences.end();
  DEBUG_PRINTF("Impostazioni caricate - Sens: %d/%d/%d\n", radarSensClose, radarSensMid, radarSensFar);
}

void saveClients() {
  preferences.begin("clients", false);
  preferences.putInt("count", clientCount);
  for (int i = 0; i < clientCount; i++) {
    String prefix = "c" + String(i);
    preferences.putString((prefix + "ip").c_str(), clients[i].ip);
    preferences.putString((prefix + "name").c_str(), clients[i].name);
    preferences.putUShort((prefix + "port").c_str(), clients[i].port);
    preferences.putBool((prefix + "act").c_str(), clients[i].active);
    preferences.putBool((prefix + "pres").c_str(), clients[i].notifyPresence);
    preferences.putBool((prefix + "bri").c_str(), clients[i].notifyBrightness);
    preferences.putBool((prefix + "onoff").c_str(), clients[i].controlOnOff);
  }
  preferences.end();
  DEBUG_PRINTF("Salvati %d dispositivi\n", clientCount);
}

void loadClients() {
  preferences.begin("clients", true);
  clientCount = preferences.getInt("count", 0);
  for (int i = 0; i < clientCount && i < MAX_CLIENTS; i++) {
    String prefix = "c" + String(i);
    clients[i].ip = preferences.getString((prefix + "ip").c_str(), "");
    clients[i].name = preferences.getString((prefix + "name").c_str(), "Device");
    clients[i].port = preferences.getUShort((prefix + "port").c_str(), 80);
    clients[i].active = preferences.getBool((prefix + "act").c_str(), true);
    clients[i].notifyPresence = preferences.getBool((prefix + "pres").c_str(), true);
    clients[i].notifyBrightness = preferences.getBool((prefix + "bri").c_str(), false);
    clients[i].controlOnOff = preferences.getBool((prefix + "onoff").c_str(), true);
    clients[i].currentState = false;
    clients[i].lastSeen = 0;
    clients[i].failCount = 0;
  }
  preferences.end();
  DEBUG_PRINTF("Caricati %d dispositivi\n", clientCount);
}

// ========== FUNZIONI CALIBRAZIONE TOUCH ==========
void saveTouchCalibration() {
  preferences.begin("touch", false);
  for (int i = 0; i < 5; i++) {
    preferences.putUShort(("cal" + String(i)).c_str(), touchCalData[i]);
  }
  preferences.putBool("calibrated", true);
  preferences.end();
  DEBUG_PRINTLN("Calibrazione touch salvata");
  DEBUG_PRINTF("CalData: { %d, %d, %d, %d, %d }\n",
    touchCalData[0], touchCalData[1], touchCalData[2], touchCalData[3], touchCalData[4]);
}

void loadTouchCalibration() {
  preferences.begin("touch", true);
  touchCalibrated = preferences.getBool("calibrated", false);
  if (touchCalibrated) {
    for (int i = 0; i < 5; i++) {
      touchCalData[i] = preferences.getUShort(("cal" + String(i)).c_str(), 0);
    }
    DEBUG_PRINTLN("Calibrazione touch caricata");
    DEBUG_PRINTF("CalData: { %d, %d, %d, %d, %d }\n",
      touchCalData[0], touchCalData[1], touchCalData[2], touchCalData[3], touchCalData[4]);
  } else {
    DEBUG_PRINTLN("Nessuna calibrazione touch salvata");
  }
  preferences.end();
}

void resetTouchCalibration() {
  preferences.begin("touch", false);
  preferences.clear();
  preferences.end();
  touchCalibrated = false;
  for (int i = 0; i < 5; i++) {
    touchCalData[i] = 0;
  }
  DEBUG_PRINTLN("Calibrazione touch resettata");
}

void touch_calibrate() {
  // Procedura di calibrazione touch - segui i punti sullo schermo
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(1);

  tft.setCursor(20, 0);
  tft.println("Calibrazione Touch Screen");
  tft.println();
  tft.println("Tocca i punti agli angoli");
  tft.println("indicati da frecce magenta");

  // Chiama la funzione di calibrazione della libreria TFT_eSPI
  // I parametri sono: array dati, colore freccia, colore sfondo, dimensione freccia
  tft.calibrateTouch(touchCalData, TFT_MAGENTA, TFT_BLACK, 15);

  // Applica la calibrazione
  tft.setTouch(touchCalData);
  touchCalibrated = true;

  // Salva in memoria
  saveTouchCalibration();

  // Mostra conferma
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(20, 60);
  tft.println("Calibrazione completata!");
  tft.println();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("Valori salvati:");
  tft.printf("{ %d, %d, %d, %d, %d }",
    touchCalData[0], touchCalData[1], touchCalData[2], touchCalData[3], touchCalData[4]);

  // Invia dati anche a Serial per debug
  Serial.println();
  Serial.println("// Calibrazione Touch completata!");
  Serial.println("// Puoi usare questi valori nel codice:");
  Serial.print("  uint16_t calData[5] = { ");
  for (int i = 0; i < 5; i++) {
    Serial.print(touchCalData[i]);
    if (i < 4) Serial.print(", ");
  }
  Serial.println(" };");
  Serial.println("  tft.setTouch(calData);");
  Serial.println();

  delay(3000);

  // Reset font
  tft.setTextFont(1);
  needsFullRedraw = true;
}

// ========== FUNZIONI BME280 ==========
void initBME280() {
  Wire.begin(BME280_SDA, BME280_SCL);
  delay(100);  // Attesa stabilizzazione I2C

  // Prova indirizzo 0x76, se fallisce prova 0x77
  if (bme.begin(0x76, &Wire)) {
    bme280Initialized = true;
    DEBUG_PRINTLN("Sensore BME280 trovato su 0x76");
  } else if (bme.begin(0x77, &Wire)) {
    bme280Initialized = true;
    DEBUG_PRINTLN("Sensore BME280 trovato su 0x77");
  } else {
    bme280Initialized = false;
    DEBUG_PRINTLN("ATTENZIONE: Sensore BME280 non trovato su 0x76 o 0x77!");
    return;
  }

  // Prima lettura
  bmeTemperature = bme.readTemperature() + tempOffset;
  bmeHumidity = bme.readHumidity() + humOffset;
  bmePressure = bme.readPressure() / 100.0F;

  DEBUG_PRINTF("BME280 - Temp: %.1f C, Hum: %.1f%%, Press: %.0f hPa\n",
               bmeTemperature, bmeHumidity, bmePressure);
}

void updateBME280() {
  if (!bme280Initialized) return;

  unsigned long nowMillis = millis();
  if (nowMillis - lastBME280Update < BME280_UPDATE_MS) return;

  bmeTemperature = bme.readTemperature() + tempOffset;
  bmeHumidity = bme.readHumidity() + humOffset;
  bmePressure = bme.readPressure() / 100.0F;
  bmeHumidity = constrain(bmeHumidity, 0.0, 100.0);

  lastBME280Update = nowMillis;
}

// ========== FUNZIONI NTP ==========
void initNTP() {
  configTzTime(timezone, ntpServer, ntpServer2);
  DEBUG_PRINTLN("NTP configurato per fuso orario Roma");
}

void updateNTPTime() {
  unsigned long nowMillis = millis();
  if (nowMillis - lastTimeUpdate < TIME_UPDATE_MS) return;
  lastTimeUpdate = nowMillis;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    if (!ntpSynced) {
      currentDay = "---";
      currentDate = "--/--/----";
      currentTime = "--:--:--";
    }
    return;
  }

  ntpSynced = true;

  const char* giorni[] = {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"};
  currentDay = giorni[timeinfo.tm_wday];

  char dateStr[12];
  sprintf(dateStr, "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  currentDate = dateStr;

  char timeStr[10];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  currentTime = timeStr;
}

// ========== FUNZIONI TOUCH ==========
bool readTouch() {
  if (tft.getTouch(&touchX, &touchY)) {
    touchPressed = true;
    return true;
  }
  touchPressed = false;
  return false;
}

bool isTouchInButton(Button &btn) {
  return (touchX >= btn.x && touchX <= btn.x + btn.w &&
          touchY >= btn.y && touchY <= btn.y + btn.h);
}

// ========== FUNZIONI GESTIONE DISPOSITIVI ==========
int addClient(String ip, String name, uint16_t port, bool controlOnOff = true) {
  if (clientCount >= MAX_CLIENTS) return -1;

  for (int i = 0; i < clientCount; i++) {
    if (clients[i].ip == ip) {
      clients[i].name = name;
      clients[i].port = port;
      clients[i].active = true;
      saveClients();
      return i;
    }
  }

  clients[clientCount].ip = ip;
  clients[clientCount].name = name;
  clients[clientCount].port = port;
  clients[clientCount].active = true;
  clients[clientCount].notifyPresence = true;
  clients[clientCount].notifyBrightness = true;
  clients[clientCount].controlOnOff = controlOnOff;
  clients[clientCount].currentState = false;
  clients[clientCount].lastSeen = millis();
  clients[clientCount].failCount = 0;
  clientCount++;
  saveClients();
  DEBUG_PRINTF("Aggiunto device: %s (%s:%d)\n", name.c_str(), ip.c_str(), port);
  return clientCount - 1;
}

bool removeClient(int index) {
  if (index < 0 || index >= clientCount) return false;

  for (int i = index; i < clientCount - 1; i++) {
    clients[i] = clients[i + 1];
  }
  clientCount--;
  saveClients();
  return true;
}

void notifyClients(String event, String value) {
  if (clientCount == 0) return;

  for (int i = 0; i < clientCount; i++) {
    if (!clients[i].active) continue;
    if (event == "presence" && !clients[i].notifyPresence) continue;
    if (event == "brightness" && !clients[i].notifyBrightness) continue;

    HTTPClient http;
    String url = "http://" + clients[i].ip + ":" + String(clients[i].port) + "/radar/" + event + "?value=" + value;

    DEBUG_PRINTF("NOTIFY -> %s:%d /%s=%s ", clients[i].ip.c_str(), clients[i].port, event.c_str(), value.c_str());

    http.begin(url);
    http.setTimeout(500);  // Timeout breve
    http.setConnectTimeout(300);
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

// ========== COMANDI ON/OFF DISPOSITIVI ==========
// Invia comando a un singolo dispositivo - ritorna true se risponde
bool sendClientCommand(int index, bool turnOn) {
  if (index < 0 || index >= clientCount) return false;
  if (!clients[index].active || !clients[index].controlOnOff) return false;

  HTTPClient http;
  String cmd = turnOn ? "on" : "off";
  String url = "http://" + clients[index].ip + ":" + String(clients[index].port) + "/radar/" + cmd;

  DEBUG_PRINTF("CMD -> %s:%d /%s ", clients[index].ip.c_str(), clients[index].port, cmd.c_str());

  http.begin(url);
  http.setTimeout(CLIENT_HTTP_TIMEOUT);
  http.setConnectTimeout(CLIENT_CONNECT_TIMEOUT);
  int httpCode = http.GET();
  http.end();

  if (httpCode > 0 && httpCode < 400) {
    clients[index].lastSeen = millis();
    clients[index].currentState = turnOn;
    clients[index].failCount = 0;
    DEBUG_PRINTF("OK(%d)\n", httpCode);
    return true;
  } else {
    DEBUG_PRINTF("FAIL(%d)\n", httpCode);
    return false;
  }
}

// Forward declaration per leggere radar durante invio comandi
void readRadarData();

// Invia comando a tutti con retry: 2 passaggi
// 1° giro: prova tutti, segna chi fallisce
// 2° giro: riprova solo chi ha fallito
// Chi fallisce anche al 2° giro viene ignorato fino al prossimo comando
void sendAllCommand(bool turnOn) {
  const char* cmdName = turnOn ? "ON" : "OFF";
  DEBUG_PRINTF(">>> INVIO %s A TUTTI I DISPOSITIVI <<<\n", cmdName);

  if (clientCount == 0) {
    DEBUG_PRINTLN("Nessun dispositivo registrato");
    return;
  }

  // Array per segnare chi ha fallito (max 10 dispositivi)
  bool failed[MAX_CLIENTS] = {false};
  int failedCount = 0;

  // === PRIMO GIRO ===
  DEBUG_PRINTLN("--- Primo tentativo ---");
  for (int i = 0; i < clientCount; i++) {
    if (!clients[i].active || !clients[i].controlOnOff) continue;

    // Leggi radar tra un comando e l'altro per non perdere dati
    readRadarData();

    if (!sendClientCommand(i, turnOn)) {
      failed[i] = true;
      failedCount++;
    }
  }

  // Se tutti hanno risposto, finito
  if (failedCount == 0) {
    DEBUG_PRINTLN("Tutti i dispositivi hanno risposto");
    return;
  }

  // === SECONDO GIRO (solo chi ha fallito) ===
  DEBUG_PRINTF("--- Secondo tentativo (%d falliti) ---\n", failedCount);
  for (int i = 0; i < clientCount; i++) {
    if (!failed[i]) continue;  // Salta chi ha già risposto

    // Leggi radar tra un comando e l'altro
    readRadarData();

    if (sendClientCommand(i, turnOn)) {
      failed[i] = false;  // Ora ha risposto
      failedCount--;
    } else {
      // Fallito anche al secondo tentativo - segna come offline
      clients[i].failCount++;
      DEBUG_PRINTF("  %s:%d -> OFFLINE (ignorato)\n",
        clients[i].ip.c_str(), clients[i].port);
    }
  }

  if (failedCount > 0) {
    DEBUG_PRINTF(">>> %d dispositivi non raggiungibili <<<\n", failedCount);
  } else {
    DEBUG_PRINTLN("Tutti i dispositivi hanno risposto al retry");
  }
}

void sendAllOn() {
  sendAllCommand(true);
}

void sendAllOff() {
  sendAllCommand(false);
}

// ========== DISCOVERY DISPOSITIVI (NON BLOCCANTE) ==========
WiFiClient discoveryClient;
bool discoveryConnecting = false;
unsigned long discoveryConnectStart = 0;
String discoveryTestIP = "";

void startDiscovery() {
  if (discoveryActive) {
    DEBUG_PRINTLN("Discovery gia in corso!");
    return;
  }

  discoveryActive = true;
  discoveryCurrentIP = 1;
  discoveryFoundCount = 0;
  discoveryConnecting = false;
  lastDiscoveryStep = millis();

  DEBUG_PRINTLN("=== AVVIO DISCOVERY DISPOSITIVI ===");
  DEBUG_PRINTF("Scansione rete: %d.%d.%d.1 - %d.%d.%d.%d\n",
    WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2],
    WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], discoveryEndIP);
}

void stopDiscovery() {
  discoveryActive = false;
  discoveryConnecting = false;
  if (discoveryClient.connected()) {
    discoveryClient.stop();
  }
  DEBUG_PRINTF("Discovery terminato - Trovati %d dispositivi\n", discoveryFoundCount);
  lastDiscoveryRun = millis();
}

void addDiscoveredDevice(String ip) {
  // Aggiungi se non esiste gia
  bool exists = false;
  for (int i = 0; i < clientCount; i++) {
    if (clients[i].ip == ip) {
      exists = true;
      clients[i].active = true;
      clients[i].lastSeen = millis();
      break;
    }
  }

  if (!exists && clientCount < MAX_CLIENTS) {
    int lastOctet = ip.substring(ip.lastIndexOf('.') + 1).toInt();
    addClient(ip, "Device_" + String(lastOctet), DISCOVERY_PORT, true);
    discoveryFoundCount++;
    DEBUG_PRINTF("DISCOVERY: Aggiunto dispositivo %s\n", ip.c_str());
  }
}

// Chiamare nel loop - NON BLOCCANTE
void discoveryStep() {
  if (!discoveryActive || otaInProgress) return;

  unsigned long now = millis();

  // Se stiamo aspettando una connessione
  if (discoveryConnecting) {
    // Controlla timeout
    if (now - discoveryConnectStart > DISCOVERY_TIMEOUT_MS) {
      discoveryClient.stop();
      discoveryConnecting = false;
      discoveryCurrentIP++;
      return;
    }

    // Controlla se connesso
    if (discoveryClient.connected()) {
      // Dispositivo trovato!
      discoveryClient.stop();
      discoveryConnecting = false;
      addDiscoveredDevice(discoveryTestIP);
      discoveryCurrentIP++;
      return;
    }

    // Ancora in attesa, esci senza bloccare
    return;
  }

  // Intervallo tra tentativi
  if (now - lastDiscoveryStep < DISCOVERY_INTERVAL_MS) return;
  lastDiscoveryStep = now;

  // Salta il nostro IP
  while (discoveryCurrentIP == WiFi.localIP()[3]) {
    discoveryCurrentIP++;
  }

  if (discoveryCurrentIP > discoveryEndIP) {
    stopDiscovery();
    return;
  }

  // Costruisci IP da testare
  discoveryTestIP = String(WiFi.localIP()[0]) + "." +
                    String(WiFi.localIP()[1]) + "." +
                    String(WiFi.localIP()[2]) + "." +
                    String(discoveryCurrentIP);

  // Tenta connessione NON bloccante
  discoveryClient.stop();

  // connect() con timeout 1 = non bloccante
  if (discoveryClient.connect(discoveryTestIP.c_str(), DISCOVERY_PORT, 1)) {
    // Connesso subito (raro)
    discoveryClient.stop();
    addDiscoveredDevice(discoveryTestIP);
    discoveryCurrentIP++;
  } else {
    // In attesa di connessione
    discoveryConnecting = true;
    discoveryConnectStart = now;
  }
}

// ========== GESTIONE PRESENZA ==========
void updatePresence() {
  // NOTA: manualPresenceOverride rimosso - il radar gestisce SEMPRE automaticamente

  if (!autoPresenceEnabled) {
    // Solo se l'utente ha disabilitato manualmente "Auto Presenza"
    return;
  }
  if (!radarInitialized) {
    // Radar non ancora pronto - aspetta
    return;
  }

  unsigned long currentTime = millis();
  if (currentTime - lastPresenceCheck < PRESENCE_CHECK_MS) return;

  // Usa i dati GIA' LETTI da readRadarData() - non chiamare radar.presenceDetected() direttamente
  bool currentPresence = radarMoving || radarStationary;

  // DEBUG: Stampa stato ogni 2 secondi
  static unsigned long lastDebugPrint = 0;
  if (currentTime - lastDebugPrint > 2000) {
    DEBUG_PRINTF("[PRESENCE] radarPres=%d mov=%d stat=%d prev=%d elapsed=%lu timeout=%lu\n",
      currentPresence, radarMoving, radarStationary, previousPresenceState,
      currentTime - lastPresenceTime, presenceTimeout);
    lastDebugPrint = currentTime;
  }

  if (currentPresence) {
    // Presenza rilevata
    presenceDetected = true;
    lastPresenceTime = currentTime;

    if (!previousPresenceState) {
      DEBUG_PRINTLN(">>> PRESENZA RILEVATA <<<");
      // Accendi display
      setDisplayEnabled(true);
      updateDisplayBrightness();  // Imposta luminosita in base a luce ambiente
      notifyClients("presence", "true");
      // INVIA COMANDO ON A TUTTI I DISPOSITIVI
      sendAllOn();
      // IMPOSTA STATO DESIDERATO MONITOR = ACCESO
      // Il sensore TEMT6000 verificherà se il monitor è realmente acceso
      // e attiverà il relè solo se necessario (in syncMonitorState)
      setMonitorDesiredState(true);
      previousPresenceState = true;
    }
  } else {
    // Nessuna presenza - controlla timeout
    unsigned long elapsed = currentTime - lastPresenceTime;

    DEBUG_PRINTF("[NO PRESENCE] elapsed=%lu/%lu prevState=%d\n",
      elapsed, presenceTimeout, previousPresenceState);

    // Dopo il timeout, se eravamo in stato presenza, invia OFF
    if (elapsed > presenceTimeout && previousPresenceState) {
      DEBUG_PRINTLN(">>> PRESENZA ASSENTE - INVIO COMANDI OFF <<<");
      // Spegni display
      setDisplayEnabled(false);
      notifyClients("presence", "false");
      // INVIA COMANDO OFF A TUTTI I DISPOSITIVI
      sendAllOff();
      // IMPOSTA STATO DESIDERATO MONITOR = SPENTO
      // Il sensore TEMT6000 verificherà se il monitor è realmente spento
      // e attiverà il relè solo se necessario (in syncMonitorState)
      setMonitorDesiredState(false);
      previousPresenceState = false;
      presenceDetected = false;
    }
  }

  lastPresenceCheck = currentTime;
}

// ========== GESTIONE LUMINOSITA ==========
void updateLightSensor() {
  unsigned long currentTime = millis();
  if (currentTime - lastLightSensorUpdate < LIGHT_SENSOR_UPDATE_MS) return;

  if (autoBrightnessEnabled) {
    calculatedBrightness = map(ambientLight, 0, 255, minBrightness, maxBrightness);
    calculatedBrightness = constrain(calculatedBrightness, minBrightness, maxBrightness);
  }

  // Aggiorna luminosita display in base a luce ambiente
  updateDisplayBrightness();

  lastLightSensorUpdate = currentTime;
}

// ========== FUNZIONI DISPLAY ==========

// Funzione per aggiornare testo senza flickering
// Sovrascrive il vecchio testo con il colore di sfondo, poi scrive il nuovo
void updateText(int x, int y, const char* newStr, char* prevBuffer, uint16_t textColor, uint16_t bgColor, const GFXfont* font = nullptr) {
  if (font) tft.setFreeFont(font);

  // Se c'è un testo precedente diverso, cancellalo
  if (strlen(prevBuffer) > 0 && strcmp(prevBuffer, newStr) != 0) {
    tft.setTextColor(bgColor);
    tft.setCursor(x, y);
    tft.print(prevBuffer);
  }

  // Scrivi il nuovo testo
  tft.setTextColor(textColor);
  tft.setCursor(x, y);
  tft.print(newStr);

  // Salva il nuovo testo
  strcpy(prevBuffer, newStr);
}

void setupDisplay() {
  tft.init();
  tft.setRotation(1);  // Landscape 320x240
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(1);

  // Carica calibrazione touch da memoria
  loadTouchCalibration();

  // Applica calibrazione se presente
  if (touchCalibrated) {
    tft.setTouch(touchCalData);
    DEBUG_PRINTLN("Calibrazione touch applicata");
  } else {
    DEBUG_PRINTLN("Touch non calibrato - usa /api/touch/calibrate");
  }

  DEBUG_PRINTLN("Display ILI9341 inizializzato (320x240)");
}

void drawButton(Button &btn, bool highlight) {
  uint16_t bgColor = highlight ? COLOR_TEXT : btn.color;
  uint16_t txtColor = highlight ? COLOR_BG : COLOR_TEXT;

  tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 5, bgColor);
  tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 5, COLOR_TEXT);

  tft.setFreeFont(&arial8pt7b);
  int textWidth = tft.textWidth(btn.label);
  int textX = btn.x + (btn.w - textWidth) / 2;
  int textY = btn.y + (btn.h / 2) + 4;  // +4 per centrare verticalmente con baseline

  tft.setTextColor(txtColor);
  tft.setCursor(textX, textY);
  tft.print(btn.label);
  tft.setFreeFont(NULL);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
}

void drawHeader(const char* title) {
  // Disegna struttura statica solo una volta
  if (!headerDrawn) {
    tft.fillRect(0, 0, 320, 40, COLOR_BG_CARD);
    tft.drawFastHLine(0, 40, 320, COLOR_ACCENT);

    // Titolo a sinistra
    tft.setFreeFont(&arial12pt7b);
    tft.setTextColor(COLOR_ACCENT);
    tft.setCursor(6, 20);
    tft.print(title);

    // Sottotitolo "LD2410"
    tft.setFreeFont(&arial6pt7b);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(6, 34);
    tft.print("LD2410 Sensor");

    // Icona termometro
    tft.fillRect(232, 6, 3, 10, COLOR_RED);
    tft.fillCircle(233, 17, 4, COLOR_RED);

    // Icona goccia
    tft.fillTriangle(234, 24, 230, 30, 238, 30, COLOR_CYAN);
    tft.fillCircle(234, 30, 4, COLOR_CYAN);

    // Indicatore WiFi
    uint16_t wifiColor = (WiFi.status() == WL_CONNECTED) ? COLOR_GREEN : COLOR_RED;
    tft.drawCircle(300, 20, 8, wifiColor);
    tft.fillCircle(300, 20, 5, wifiColor);

    headerDrawn = true;
    // Reset stringhe per forzare primo disegno
    prevHeaderTime[0] = '\0';
    prevHeaderDate[0] = '\0';
    prevHeaderTemp[0] = '\0';
    prevHeaderHum[0] = '\0';
  }

  // === AGGIORNA SOLO PARTI DINAMICHE ===

  // Data (cambia raramente)
  String dateStr = currentDay + " " + currentDate;
  if (strcmp(dateStr.c_str(), prevHeaderDate) != 0) {
    updateText(110, 14, dateStr.c_str(), prevHeaderDate, COLOR_TEXT, COLOR_BG_CARD, &arial7pt7b);
  }

  // Ora (cambia ogni secondo)
  if (strcmp(currentTime.c_str(), prevHeaderTime) != 0) {
    updateText(115, 36, currentTime.c_str(), prevHeaderTime, COLOR_GREEN, COLOR_BG_CARD, &digital_display16pt7b);
  }

  // Temperatura
  char tempStr[10];
  if (bme280Initialized) {
    sprintf(tempStr, "%.1fC", bmeTemperature);
  } else {
    strcpy(tempStr, "--.-C");
  }
  if (strcmp(tempStr, prevHeaderTemp) != 0) {
    updateText(242, 16, tempStr, prevHeaderTemp, COLOR_TEXT, COLOR_BG_CARD, &arial7pt7b);
  }

  // Umidita
  char humStr[8];
  if (bme280Initialized) {
    sprintf(humStr, "%.0f%%", bmeHumidity);
  } else {
    strcpy(humStr, "--%");
  }
  if (strcmp(humStr, prevHeaderHum) != 0) {
    updateText(242, 34, humStr, prevHeaderHum, COLOR_TEXT, COLOR_BG_CARD, &arial7pt7b);
  }

  // Reset font
  tft.setFreeFont(NULL);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
}

// Funzione helper per disegnare card con bordo e accent
void drawCard(int x, int y, int w, int h, const char* title, uint16_t accentColor) {
  tft.fillRoundRect(x, y, w, h, 4, COLOR_BG_CARD);
  tft.drawRoundRect(x, y, w, h, 4, COLOR_GRAY_DARK);
  tft.fillRect(x + 1, y + 1, w - 2, 3, accentColor);
  tft.setTextColor(accentColor);
  tft.setFreeFont(&arial8pt7b);
  tft.setCursor(x + 6, y + 16);
  tft.print(title);
  tft.setFreeFont(NULL);
}

void drawMainScreen() {
  int y = 46;  // Inizio dopo header (40px + 6px margine)

  // ===== STRUTTURA STATICA (solo al primo disegno) =====
  if (needsFullRedraw) {
    headerDrawn = false;  // Reset header per forzare ridisegno completo
    tft.fillScreen(COLOR_BG);
    drawHeader("RADAR");

    // === RIGA 1: Sistema e Presenza ===
    drawCard(4, y, 154, 62, "SISTEMA", COLOR_ACCENT);
    tft.setFreeFont(&arial6pt7b);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(10, y + 28);
    tft.print("Status:");
    tft.setCursor(10, y + 45);
    tft.print("IP:");
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(28, y + 45);
    tft.print(WiFi.localIP().toString());
    // Etichette Monitor (sotto IP, affiancate)
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(10, y + 57);
    tft.print("Mon:");
    tft.setCursor(55, y + 57);
    tft.print("Lux:");
    tft.setCursor(105, y + 57);
    tft.print("Sync:");

    drawCard(162, y, 154, 62, "PRESENZA", COLOR_GREEN);
    tft.setFreeFont(&arial6pt7b);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(170, y + 44);
    tft.print("MOV");
    tft.setCursor(170, y + 60);
    tft.print("STAT");

    // === RIGA 2: Distanza e Luminosita ===
    y = 112;
    drawCard(4, y, 154, 68, "DISTANZA", COLOR_CYAN);
    tft.setFreeFont(&arial7pt7b);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(125, y + 58);
    tft.print("cm");

    drawCard(162, y, 154, 68, "LUMINOSITA", COLOR_YELLOW);
    tft.drawRoundRect(170, y + 24, 138, 18, 3, COLOR_GRAY_DARK);
    tft.setFreeFont(&arial6pt7b);
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(170, y + 52);
    tft.print("Raw:");
    tft.setCursor(170, y + 64);
    tft.print("PWM:");
    tft.setFreeFont(NULL);

    // === Bottoni ===
    btnOnOff.y = 186;
    btnSettings.y = 186;
    btnRelay.y = 186;
    // Aggiorna label e colore in base allo stato
    btnOnOff.label = manualAllOn ? "ACCENDI" : "SPEGNI";
    btnOnOff.color = manualAllOn ? COLOR_GREEN : COLOR_RED;
    btnSettings.color = COLOR_ORANGE;
    btnRelay.color = relayActive ? COLOR_GREEN : COLOR_RED;
    drawButton(btnOnOff);
    drawButton(btnSettings);
    drawButton(btnRelay);

    prevRadarOnline = !radarOnline;
    prevPresence = !presenceDetected;
    prevMoving = !radarMoving;
    prevStationary = !radarStationary;
    prevDistance = -1;
    prevAmbientLight = -1;
    prevTime = "";
    prevDate = "";
    prevTemp = -999;
    prevHum = -999;
    // Reset stringhe per forzare ridisegno
    prevStatusStr[0] = '\0';
    prevPresenceStr[0] = '\0';
    prevMovingStr[0] = '\0';
    prevStationaryStr[0] = '\0';
    prevDistanceStr[0] = '\0';
    prevRawStr[0] = '\0';
    prevPwmStr[0] = '\0';
    prevPctStr[0] = '\0';
    // Reset indicatore monitor
    prevMonitorDetected = !monitorDetectedOn;
    prevMonitorInSync = !(monitorDetectedOn == monitorStateDesired);
    prevMonitorLevelStr[0] = '\0';
    needsFullRedraw = false;
  }

  // ===== AGGIORNA HEADER SE CAMBIATO =====
  bool headerChanged = (currentTime != prevTime) || (currentDate != prevDate) ||
                       (abs(bmeTemperature - prevTemp) > 0.1) || (abs(bmeHumidity - prevHum) > 0.1);
  if (headerChanged) {
    drawHeader("RADAR");
    prevTime = currentTime;
    prevDate = currentDate;
    prevTemp = bmeTemperature;
    prevHum = bmeHumidity;
  }

  y = 46;  // Stesso valore di inizio

  // ===== VALORI DINAMICI =====

  // Stato radar con LED - allineato con label Status (y + 28)
  if (radarOnline != prevRadarOnline) {
    tft.fillCircle(58, y + 26, 4, radarOnline ? COLOR_GREEN : COLOR_RED);
    const char* newStatus = radarOnline ? "ONLINE" : "OFFLINE";
    uint16_t statusColor = radarOnline ? COLOR_GREEN : COLOR_RED;
    updateText(72, y + 28, newStatus, prevStatusStr, statusColor, COLOR_BG_CARD, &arial6pt7b);
    tft.setFreeFont(NULL);
    prevRadarOnline = radarOnline;
  }

  // Indicatore presenza con icona
  if (presenceDetected != prevPresence) {
    // Icona presenza (questa deve essere ridisegnata completamente)
    tft.fillRect(255, y + 15, 55, 48, COLOR_BG_CARD);

    uint16_t ringColor = presenceDetected ? COLOR_GREEN : COLOR_GRAY_DARK;
    tft.drawCircle(282, y + 38, 18, ringColor);
    tft.drawCircle(282, y + 38, 17, ringColor);
    tft.fillCircle(282, y + 38, 14, presenceDetected ? COLOR_GREEN_DIM : COLOR_BG_CARD2);

    if (presenceDetected) {
      tft.fillCircle(282, y + 33, 4, COLOR_GREEN);
      tft.fillRect(280, y + 38, 5, 7, COLOR_GREEN);
    } else {
      tft.drawLine(277, y + 33, 287, y + 43, COLOR_GRAY);
      tft.drawLine(287, y + 33, 277, y + 43, COLOR_GRAY);
    }

    // Testo presenza (senza flickering)
    const char* newPresenceStr = presenceDetected ? "ATTIVA" : "VUOTO";
    uint16_t presenceColor = presenceDetected ? COLOR_GREEN : COLOR_TEXT_DIM;
    updateText(197, y + 34, newPresenceStr, prevPresenceStr, presenceColor, COLOR_BG_CARD, &arial7pt7b);
    tft.setFreeFont(NULL);
    prevPresence = presenceDetected;
  }

  // Movimento - allineato con label MOV (y + 44)
  if (radarMoving != prevMoving) {
    tft.fillCircle(204, y + 42, 3, radarMoving ? COLOR_YELLOW : COLOR_GRAY_DARK);
    const char* newMovStr = radarMoving ? "SI" : "--";
    uint16_t movColor = radarMoving ? COLOR_YELLOW : COLOR_TEXT_DIM;
    updateText(214, y + 44, newMovStr, prevMovingStr, movColor, COLOR_BG_CARD, &arial6pt7b);
    tft.setFreeFont(NULL);
    prevMoving = radarMoving;
  }

  // Stazionario - allineato con label STAT (y + 60)
  if (radarStationary != prevStationary) {
    tft.fillCircle(204, y + 58, 3, radarStationary ? COLOR_ORANGE : COLOR_GRAY_DARK);
    const char* newStatStr = radarStationary ? "SI" : "--";
    uint16_t statColor = radarStationary ? COLOR_ORANGE : COLOR_TEXT_DIM;
    updateText(214, y + 60, newStatStr, prevStationaryStr, statColor, COLOR_BG_CARD, &arial6pt7b);
    tft.setFreeFont(NULL);
    prevStationary = radarStationary;
  }

  y = 112;

  // Distanza grande - Font digitale custom CENTRATO
  if (radarDistance != prevDistance) {
    // Box distanza: x=4, w=154, area utile circa 10-148
    int boxX = 4;
    int boxW = 154;
    int boxCenterX = boxX + boxW / 2;  // Centro del box = 81

    tft.setFreeFont(&digital_display24pt7b);

    // Calcola larghezza testo per centrare
    char distStr[8];
    sprintf(distStr, "%d", radarDistance);
    int textW = tft.textWidth(distStr);
    int textX = boxCenterX - (textW / 2) - 10;  // -10 per compensare "cm"

    // Cancella testo precedente con sfondo
    if (strlen(prevDistanceStr) > 0 && strcmp(prevDistanceStr, distStr) != 0) {
      int prevTextW = tft.textWidth(prevDistanceStr);
      int prevTextX = boxCenterX - (prevTextW / 2) - 10;
      tft.setTextColor(COLOR_BG_CARD);
      tft.setCursor(prevTextX, y + 58);
      tft.print(prevDistanceStr);
    }

    // Scrivi nuovo testo
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(textX, y + 58);
    tft.print(distStr);
    strcpy(prevDistanceStr, distStr);

    tft.setFreeFont(NULL);
    prevDistance = radarDistance;
  }

  // Luce ambiente con barra colorata
  if (ambientLight != prevAmbientLight) {
    int barW = 134;
    int fillW = map(constrain(ambientLight, 0, 255), 0, 255, 0, barW);
    int pct = map(constrain(ambientLight, 0, 255), 0, 255, 0, 100);

    // Barra (ridisegno necessario per cambio colore/dimensione)
    tft.fillRoundRect(171, y + 25, 136, 16, 2, COLOR_BG_CARD2);
    uint16_t barCol = (pct < 30) ? COLOR_BLUE_ACC : (pct < 70) ? COLOR_GREEN : COLOR_YELLOW;
    if (fillW > 0) tft.fillRoundRect(172, y + 26, fillW, 14, 2, barCol);

    // Percentuale dentro la barra (senza flickering)
    char newPctStr[8];
    sprintf(newPctStr, "%d%%", pct);
    updateText(220, y + 38, newPctStr, prevPctStr, COLOR_TEXT, COLOR_BG_CARD2, &arial7pt7b);

    // Valori Raw e PWM (senza flickering)
    char newRawStr[8], newPwmStr[8];
    sprintf(newRawStr, "%d", ambientLight);
    sprintf(newPwmStr, "%d", calculatedBrightness);
    updateText(200, y + 52, newRawStr, prevRawStr, COLOR_TEXT, COLOR_BG_CARD, &arial6pt7b);
    updateText(200, y + 64, newPwmStr, prevPwmStr, COLOR_TEXT, COLOR_BG_CARD, &arial6pt7b);

    tft.setFreeFont(NULL);
    prevAmbientLight = ambientLight;
  }

  // === INDICATORE STATO MONITOR (nella card SISTEMA) ===
  bool monitorInSync = (monitorDetectedOn == monitorStateDesired);
  y = 46;  // Torna alla posizione della card SISTEMA

  // Stato monitor (ON/OFF) - cambia solo se diverso (affiancato sotto IP)
  if (monitorDetectedOn != prevMonitorDetected) {
    const char* monStateStr = monitorDetectedOn ? "ON" : "OFF";
    uint16_t monColor = monitorDetectedOn ? COLOR_GREEN : COLOR_RED;
    // Cancella area precedente
    tft.fillRect(32, y + 49, 22, 12, COLOR_BG_CARD);
    tft.setFreeFont(&arial6pt7b);
    tft.setTextColor(monColor);
    tft.setCursor(32, y + 57);
    tft.print(monStateStr);
    tft.setFreeFont(NULL);
    prevMonitorDetected = monitorDetectedOn;
  }

  // Livello luce monitor (affiancato)
  char monLevelStr[8];
  sprintf(monLevelStr, "%d", monitorLightLevel);
  if (strcmp(monLevelStr, prevMonitorLevelStr) != 0) {
    updateText(78, y + 57, monLevelStr, prevMonitorLevelStr, COLOR_TEXT, COLOR_BG_CARD, &arial6pt7b);
    tft.setFreeFont(NULL);
  }

  // Indicatore sync (pallino verde/rosso, affiancato)
  if (monitorInSync != prevMonitorInSync) {
    uint16_t syncColor = monitorInSync ? COLOR_GREEN : COLOR_RED;
    tft.fillCircle(135, y + 55, 4, syncColor);
    prevMonitorInSync = monitorInSync;
  }
}

void drawRadarScreen() {
  static int prevTargetY = -1;
  static int prevRadarDist = -1;
  static bool prevPresenceRadar = false;
  static int pulseRadius = 0;
  static unsigned long lastPulse = 0;

  // Area sensore - vista laterale/frontale
  int sensorX = 100;      // Posizione sensore (in basso)
  int sensorY = 200;
  int fieldWidth = 180;   // Larghezza campo visivo
  int fieldHeight = 160;  // Altezza/profondità campo

  // Colori tema radar
  uint16_t radarGreen = tft.color565(0, 60, 0);
  uint16_t radarLine = tft.color565(0, 150, 0);
  uint16_t radarBright = tft.color565(0, 255, 0);
  uint16_t zoneSafe = tft.color565(0, 40, 0);

  if (needsFullRedraw) {
    tft.fillScreen(COLOR_BG);

    // Header
    tft.fillRect(0, 0, 320, 28, tft.color565(0, 30, 0));
    tft.setTextColor(radarBright);
    tft.setTextSize(2);
    tft.setCursor(8, 6);
    tft.print("SENSORE LD2410");
    tft.setTextSize(1);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(200, 10);
    tft.print(radarFirmware);

    // === CAMPO VISIVO A CONO/TRAPEZIO ===
    // Disegna zona di rilevamento (trapezio che si allarga)
    int topY = 35;
    int topWidth = 40;
    int bottomWidth = fieldWidth;

    // Sfondo zona rilevamento con gradiente
    for (int y = topY; y <= sensorY; y++) {
      int progress = y - topY;
      int totalH = sensorY - topY;
      int w = map(progress, 0, totalH, topWidth, bottomWidth);
      int x1 = sensorX - w/2;
      uint8_t g = map(progress, 0, totalH, 30, 60);
      tft.drawFastHLine(x1, y, w, tft.color565(0, g, 0));
    }

    // Linee zona (ogni metro)
    for (int m = 1; m <= 4; m++) {
      int y = sensorY - (m * fieldHeight / 4);
      int progress = sensorY - y;
      int totalH = sensorY - topY;
      int w = map(progress, 0, totalH, topWidth, bottomWidth);
      int x1 = sensorX - w/2;
      int x2 = sensorX + w/2;

      // Linea orizzontale tratteggiata
      for (int x = x1; x < x2; x += 8) {
        tft.drawFastHLine(x, y, 4, radarLine);
      }

      // Etichetta distanza
      tft.setTextColor(radarLine);
      tft.setCursor(x2 + 5, y - 4);
      tft.print(m);
      tft.print("m");
    }

    // Bordi laterali del cono
    tft.drawLine(sensorX - topWidth/2, topY, sensorX - bottomWidth/2, sensorY, radarLine);
    tft.drawLine(sensorX + topWidth/2, topY, sensorX + bottomWidth/2, sensorY, radarLine);

    // Linea superiore
    tft.drawFastHLine(sensorX - topWidth/2, topY, topWidth, radarLine);

    // === ICONA SENSORE in basso ===
    tft.fillRoundRect(sensorX - 20, sensorY + 2, 40, 12, 3, tft.color565(50, 50, 50));
    tft.drawRoundRect(sensorX - 20, sensorY + 2, 40, 12, 3, radarBright);
    tft.setTextColor(radarBright);
    tft.setCursor(sensorX - 15, sensorY + 5);
    tft.print("LD2410");

    // === PANNELLO INFO A DESTRA ===
    int panelX = 210;
    int panelY = 32;

    tft.fillRoundRect(panelX, panelY, 108, 180, 5, tft.color565(10, 30, 10));
    tft.drawRoundRect(panelX, panelY, 108, 180, 5, radarLine);

    tft.setTextColor(radarBright);
    tft.setCursor(panelX + 5, panelY + 5);
    tft.print("DISTANZA");

    tft.setCursor(panelX + 5, panelY + 48);
    tft.print("STATO");

    tft.setCursor(panelX + 5, panelY + 91);
    tft.print("ENERGIA MOV");

    tft.setCursor(panelX + 5, panelY + 120);
    tft.print("ENERGIA STAT");

    tft.setCursor(panelX + 5, panelY + 149);
    tft.print("LUCE");

    // Pulsante indietro
    tft.fillRoundRect(panelX, 218, 108, 20, 3, tft.color565(80, 0, 0));
    tft.drawRoundRect(panelX, 218, 108, 20, 3, COLOR_RED);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(panelX + 28, 222);
    tft.print("INDIETRO");

    btnBack.x = panelX;
    btnBack.y = 218;
    btnBack.w = 108;
    btnBack.h = 20;

    prevTargetY = -1;
    prevRadarDist = -1;
    prevPresenceRadar = !presenceDetected;
    pulseRadius = 0;
    // Reset stringhe per forzare ridisegno
    prevRadarDistStr[0] = '\0';
    prevRadarStateStr[0] = '\0';
    prevMovEnergyStr[0] = '\0';
    prevStatEnergyStr[0] = '\0';
    prevLightStr[0] = '\0';

    needsFullRedraw = false;
  }

  // Colori (ridefiniti per uso nel loop)
  radarGreen = tft.color565(0, 60, 0);
  radarLine = tft.color565(0, 150, 0);
  radarBright = tft.color565(0, 255, 0);

  // === EFFETTO PULSE (onde dal sensore) ===
  unsigned long now = millis();
  if (now - lastPulse > 80) {
    int topY = 35;

    // Cancella vecchio pulse
    if (pulseRadius > 0) {
      int oldY = sensorY - pulseRadius;
      if (oldY >= topY) {
        int progress = sensorY - oldY;
        int totalH = sensorY - topY;
        int w = map(progress, 0, totalH, 40, 180);
        // Ridisegna sfondo
        uint8_t g = map(progress, 0, totalH, 30, 60);
        tft.drawFastHLine(sensorX - w/2 + 1, oldY, w - 2, tft.color565(0, g, 0));
      }
    }

    // Nuovo pulse
    pulseRadius += 8;
    if (pulseRadius > fieldHeight) pulseRadius = 0;

    if (pulseRadius > 0 && presenceDetected) {
      int newY = sensorY - pulseRadius;
      if (newY >= topY) {
        int progress = sensorY - newY;
        int totalH = sensorY - topY;
        int w = map(progress, 0, totalH, 40, 180);
        // Disegna pulse verde brillante
        tft.drawFastHLine(sensorX - w/2 + 2, newY, w - 4, radarBright);
      }
    }

    lastPulse = now;
  }

  // === AGGIORNA TARGET E PANNELLO ===
  if (radarDistance != prevRadarDist || presenceDetected != prevPresenceRadar) {
    int panelX = 210;
    int panelY = 32;
    int topY = 35;
    int totalH = sensorY - topY;

    // Ridisegna TUTTO il cono per evitare artefatti
    for (int y = topY; y <= sensorY; y++) {
      int prog = sensorY - y;
      int w = map(prog, 0, totalH, 40, 180);
      uint8_t g = map(prog, 0, totalH, 30, 60);
      tft.drawFastHLine(sensorX - w/2 + 1, y, w - 2, tft.color565(0, g, 0));
    }

    // Ridisegna le linee di distanza
    for (int m = 1; m <= 4; m++) {
      int y = sensorY - (m * fieldHeight / 4);
      int progress = sensorY - y;
      int w = map(progress, 0, totalH, 40, 180);
      int x1 = sensorX - w/2;
      int x2 = sensorX + w/2;
      for (int x = x1; x < x2; x += 8) {
        tft.drawFastHLine(x, y, 4, radarLine);
      }
      tft.setTextColor(radarLine);
      tft.setTextSize(1);
      tft.setCursor(x2 + 5, y - 4);
      tft.print(m);
      tft.print("m");
    }

    // === AGGIORNA PANNELLO ===
    uint16_t panelBgColor = tft.color565(10, 30, 10);

    // Distanza - sovrascrivi testo precedente
    char newRadarDistStr[12];
    if (presenceDetected && radarDistance > 0) {
      sprintf(newRadarDistStr, "%d cm", radarDistance);
    } else {
      strcpy(newRadarDistStr, "---");
    }
    // Cancella vecchio testo
    if (strlen(prevRadarDistStr) > 0 && strcmp(prevRadarDistStr, newRadarDistStr) != 0) {
      tft.setTextSize(2);
      tft.setTextColor(panelBgColor);
      tft.setCursor(panelX + 10, panelY + 22);
      tft.print(prevRadarDistStr);
    }
    // Scrivi nuovo testo
    tft.setTextSize(2);
    tft.setTextColor((presenceDetected && radarDistance > 0) ? COLOR_CYAN : COLOR_GRAY);
    tft.setCursor(panelX + 10, panelY + 22);
    tft.print(newRadarDistStr);
    strcpy(prevRadarDistStr, newRadarDistStr);
    tft.setTextSize(1);

    // Stato - sovrascrivi testo precedente
    char newRadarStateStr[10];
    uint16_t stateColor;
    if (radarMoving) {
      strcpy(newRadarStateStr, "MOVE");
      stateColor = COLOR_RED;
    } else if (radarStationary) {
      strcpy(newRadarStateStr, "FERMO");
      stateColor = COLOR_ORANGE;
    } else if (presenceDetected) {
      strcpy(newRadarStateStr, "OK");
      stateColor = COLOR_GREEN;
    } else {
      strcpy(newRadarStateStr, "---");
      stateColor = COLOR_GRAY;
    }
    // Cancella vecchio testo
    if (strlen(prevRadarStateStr) > 0 && strcmp(prevRadarStateStr, newRadarStateStr) != 0) {
      tft.setTextSize(2);
      tft.setTextColor(panelBgColor);
      tft.setCursor(panelX + 10, panelY + 68);
      tft.print(prevRadarStateStr);
    }
    // Scrivi nuovo testo
    tft.setTextSize(2);
    tft.setTextColor(stateColor);
    tft.setCursor(panelX + 10, panelY + 68);
    tft.print(newRadarStateStr);
    strcpy(prevRadarStateStr, newRadarStateStr);
    tft.setTextSize(1);

    // Energia movimento (barra) - ridisegno necessario per barra
    int barM = map(constrain(radarMovingEnergy, 0, 100), 0, 100, 0, 90);
    tft.fillRect(panelX + 8, panelY + 104, 90, 10, tft.color565(30, 30, 30));
    tft.fillRect(panelX + 8, panelY + 104, barM, 10, COLOR_RED);
    // Valore energia - sovrascrivi
    char newMovEnergyStr[6];
    sprintf(newMovEnergyStr, "%d", radarMovingEnergy);
    if (strlen(prevMovEnergyStr) > 0 && strcmp(prevMovEnergyStr, newMovEnergyStr) != 0) {
      tft.setTextColor(panelBgColor);
      tft.setCursor(panelX + 80, panelY + 105);
      tft.print(prevMovEnergyStr);
    }
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(panelX + 80, panelY + 105);
    tft.print(newMovEnergyStr);
    strcpy(prevMovEnergyStr, newMovEnergyStr);

    // Energia stazionario (barra)
    int barS = map(constrain(radarStationaryEnergy, 0, 100), 0, 100, 0, 90);
    tft.fillRect(panelX + 8, panelY + 133, 90, 10, tft.color565(30, 30, 30));
    tft.fillRect(panelX + 8, panelY + 133, barS, 10, COLOR_ORANGE);
    // Valore energia - sovrascrivi
    char newStatEnergyStr[6];
    sprintf(newStatEnergyStr, "%d", radarStationaryEnergy);
    if (strlen(prevStatEnergyStr) > 0 && strcmp(prevStatEnergyStr, newStatEnergyStr) != 0) {
      tft.setTextColor(panelBgColor);
      tft.setCursor(panelX + 80, panelY + 134);
      tft.print(prevStatEnergyStr);
    }
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(panelX + 80, panelY + 134);
    tft.print(newStatEnergyStr);
    strcpy(prevStatEnergyStr, newStatEnergyStr);

    // Luce ambiente (barra)
    int barL = map(ambientLight, 0, 255, 0, 90);
    tft.fillRect(panelX + 8, panelY + 162, 90, 10, tft.color565(30, 30, 30));
    tft.fillRect(panelX + 8, panelY + 162, barL, 10, COLOR_YELLOW);
    // Valore luce - sovrascrivi
    char newLightStr[6];
    sprintf(newLightStr, "%d", ambientLight);
    if (strlen(prevLightStr) > 0 && strcmp(prevLightStr, newLightStr) != 0) {
      tft.setTextColor(panelBgColor);
      tft.setCursor(panelX + 75, panelY + 163);
      tft.print(prevLightStr);
    }
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(panelX + 75, panelY + 163);
    tft.print(newLightStr);
    strcpy(prevLightStr, newLightStr);

    // === DISEGNA NUOVO TARGET ===
    if (presenceDetected && radarDistance > 0) {
      // Calcola posizione Y nel cono (distanza -> posizione)
      int targetY = sensorY - map(constrain(radarDistance, 0, 400), 0, 400, 10, fieldHeight - 5);

      // Colore basato su movimento
      uint16_t targetColor;
      uint16_t glowColor;
      if (radarMoving) {
        targetColor = COLOR_RED;
        glowColor = tft.color565(80, 0, 0);
      } else if (radarStationary) {
        targetColor = COLOR_ORANGE;
        glowColor = tft.color565(80, 40, 0);
      } else {
        targetColor = radarBright;
        glowColor = tft.color565(0, 80, 0);
      }

      // Icona persona stilizzata (testa + corpo)
      tft.fillCircle(sensorX, targetY - 8, 6, glowColor);
      tft.fillCircle(sensorX, targetY - 8, 4, targetColor);
      tft.fillRoundRect(sensorX - 5, targetY - 2, 10, 14, 2, glowColor);
      tft.fillRoundRect(sensorX - 4, targetY - 1, 8, 12, 2, targetColor);

      // Cerchio di rilevamento attorno
      tft.drawCircle(sensorX, targetY, 16, targetColor);

      prevTargetY = targetY;
    } else {
      prevTargetY = -1;

      // Mostra "VUOTO" al centro del cono
      tft.setTextSize(1);
      tft.setTextColor(tft.color565(0, 120, 0));
      tft.setCursor(sensorX - 18, sensorY - 90);
      tft.print("VUOTO");
    }

    prevRadarDist = radarDistance;
    prevPresenceRadar = presenceDetected;
  }
}

void updateDisplay() {
  switch (currentScreen) {
    case 0:
      drawMainScreen();
      break;
    case 1:
      drawRadarScreen();
      break;
  }
}

// ========== GESTIONE TOUCH UI ==========
void handleTouch() {
  static unsigned long lastTouchTime = 0;

  if (readTouch()) {
    if (millis() - lastTouchTime < 300) return;
    lastTouchTime = millis();

    DEBUG_PRINTF("Touch: X=%d, Y=%d\n", touchX, touchY);

    switch (currentScreen) {
      case 0:
        if (isTouchInButton(btnOnOff)) {
          if (manualAllOn) {
            DEBUG_PRINTLN("Pulsante ACCENDI -> ACCENDI TUTTI");
            sendAllOn();
            startRelayPulse();
            manualAllOn = false;  // Prossimo tocco = SPEGNI
            btnOnOff.label = "SPEGNI";
            btnOnOff.color = COLOR_RED;
          } else {
            DEBUG_PRINTLN("Pulsante SPEGNI -> SPEGNI TUTTI");
            sendAllOff();
            startRelayPulse();
            manualAllOn = true;  // Prossimo tocco = ACCENDI
            btnOnOff.label = "ACCENDI";
            btnOnOff.color = COLOR_GREEN;
          }
          drawButton(btnOnOff);
        }
        else if (isTouchInButton(btnSettings)) {
          DEBUG_PRINTLN("Pulsante RADAR premuto");
          currentScreen = 1;
          needsFullRedraw = true;
        }
        else if (isTouchInButton(btnRelay)) {
          DEBUG_PRINTLN("Pulsante RELE premuto - TEST");
          if (startRelayPulse()) {
            ledRelayActive();
            btnRelay.color = COLOR_GREEN;
            drawButton(btnRelay);
          }
        }
        break;

      case 1:
        if (isTouchInButton(btnBack)) {
          DEBUG_PRINTLN("Pulsante INDIETRO premuto");
          currentScreen = 0;
          needsFullRedraw = true;
          prevDistance = -1;
          prevBrightness = -1;
          prevRadarOnline = false;
        }
        break;
    }
  }
}

// ========== GESTIONE RADAR LD2410 ==========
bool applyRadarSensitivity() {
  if (!radarInitialized) return false;

  DEBUG_PRINTLN("Applicazione sensibilita radar...");
  DEBUG_PRINTF("Valori: close=%d, mid=%d, far=%d\n", radarSensClose, radarSensMid, radarSensFar);

  if (radar.configMode(true)) {
    DEBUG_PRINTLN("Config mode attivato");

    // Imposta gate massimi
    radar.setMaxGate(RADAR_MAX_MOVING_GATE, RADAR_MAX_STATIONARY_GATE, 5);

    // Gate 0-2: distanza ravvicinata (0-225cm)
    for (int gate = 0; gate <= 2; gate++) {
      radar.setGateParameters(gate, radarSensClose, radarSensClose);
    }

    // Gate 3-5: distanza media (225-450cm)
    for (int gate = 3; gate <= 5; gate++) {
      radar.setGateParameters(gate, radarSensMid, radarSensMid);
    }

    // Gate 6-8: distanza lontana (450-675cm)
    for (int gate = 6; gate <= 8; gate++) {
      radar.setGateParameters(gate, radarSensFar, radarSensFar);
    }

    radar.configMode(false);
    DEBUG_PRINTLN("Sensibilita radar applicata!");
    return true;
  }

  DEBUG_PRINTLN("ERRORE: impossibile entrare in config mode");
  return false;
}

void setupRadar() {
  DEBUG_PRINTLN("Inizializzazione radar LD2410...");
  ledRadarReading();  // LED ciano durante init radar

  radarSerial.begin(LD2410_BAUD_RATE, SERIAL_8N1, LD2410_RX_PIN, LD2410_TX_PIN);
  delay(500);

  if (radar.begin()) {
    radarOnline = true;
    radarInitialized = true;
    DEBUG_PRINTLN("Radar LD2410 inizializzato!");

    radarFirmware = radar.getFirmware();
    DEBUG_PRINT("Firmware: ");
    DEBUG_PRINTLN(radarFirmware);

    if (radar.enhancedMode(true)) {
      DEBUG_PRINTLN("Modalita Enhanced attivata");
    }

    // Applica sensibilita custom per migliorare rilevamento stazionario
    applyRadarSensitivity();
    DEBUG_PRINTLN("Sensibilita radar custom applicata");

  } else {
    radarOnline = false;
    radarInitialized = false;
    ledError();  // LED rosso se radar non risponde
    DEBUG_PRINTLN("ERRORE: Radar LD2410 non risponde!");
  }
}

void readRadarData() {
  MyLD2410::Response response = radar.check();

  if (response == MyLD2410::DATA) {
    // Dati ricevuti - reset contatore errori
    radarFailCount = 0;

    // Se riceviamo dati, il radar è online e inizializzato
    if (!radarOnline || !radarInitialized) {
      radarOnline = true;
      radarInitialized = true;
      DEBUG_PRINTLN("RADAR: Connesso e funzionante!");
    }

    // Leggi dati dal radar
    radarMoving = radar.movingTargetDetected();
    radarStationary = radar.stationaryTargetDetected();
    radarMovingDistance = radar.movingTargetDistance();
    radarStationaryDistance = radar.stationaryTargetDistance();
    radarMovingEnergy = radar.movingTargetSignal();
    radarStationaryEnergy = radar.stationaryTargetSignal();

    // Calcola distanza combinata: priorità a movimento, altrimenti stazionario
    if (radarMoving && radarMovingDistance > 0) {
      radarDistance = radarMovingDistance;
    } else if (radarStationary && radarStationaryDistance > 0) {
      radarDistance = radarStationaryDistance;
    } else {
      radarDistance = 0;
    }

    if (radar.inEnhancedMode()) {
      ambientLight = radar.getLightLevel();
    }

    // Debug presenza
    bool currentPresence = radarMoving || radarStationary;
    static bool lastLoggedPresence = false;
    if (currentPresence != lastLoggedPresence) {
      DEBUG_PRINTF("RADAR: Presenza %s (mov=%d stat=%d dist=%d)\n",
                   currentPresence ? "RILEVATA" : "ASSENTE",
                   radarMoving, radarStationary, radarDistance);
      lastLoggedPresence = currentPresence;
    }
  } else if (response == MyLD2410::FAIL) {
    // Errore comunicazione - incrementa contatore
    radarFailCount++;

    // Solo dopo molti errori consecutivi considera disconnesso
    if (radarFailCount >= RADAR_MAX_FAIL_COUNT && radarOnline) {
      radarOnline = false;
      DEBUG_PRINTF("RADAR: Disconnesso dopo %d errori consecutivi!\n", radarFailCount);
    }
  }
  // NOTA: Altri tipi di response (TIMEOUT, etc) vengono ignorati - non sono errori fatali
}

// ========== GESTIONE OTA ==========
void setupOTA() {
  ArduinoOTA.setHostname(hostname);

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    ledOtaUpdate();  // LED magenta durante OTA
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    DEBUG_PRINTLN("OTA Start: " + type);

    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_TITLE);
    tft.setTextSize(2);
    tft.setCursor(60, 60);
    tft.print("OTA UPDATE");
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(40, 100);
    tft.print("Aggiornamento in corso...");
    tft.drawRect(40, 140, 240, 30, COLOR_TEXT);
  });

  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    blinkStatusLed(LED_COLOR_READY, 3, 200);  // Lampeggia verde 3 volte
    DEBUG_PRINTLN("OTA End");
    tft.fillRect(41, 141, 238, 28, COLOR_GREEN);
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(80, 180);
    tft.print("COMPLETATO!");
    delay(1000);
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    otaProgress = (progress / (total / 100));
    int barWidth = map(otaProgress, 0, 100, 0, 236);
    tft.fillRect(42, 142, barWidth, 26, COLOR_BLUE);
    tft.fillRect(130, 175, 60, 15, COLOR_BG);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(130, 175);
    tft.print(otaProgress);
    tft.print("%");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    ledError();  // LED rosso su errore
    DEBUG_PRINTF("OTA Error[%u]: ", error);
    String errMsg = "Errore: ";
    if (error == OTA_AUTH_ERROR) errMsg += "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) errMsg += "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) errMsg += "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) errMsg += "Receive Failed";
    else if (error == OTA_END_ERROR) errMsg += "End Failed";
    DEBUG_PRINTLN(errMsg);
    tft.fillRect(41, 141, 238, 28, COLOR_RED);
    tft.setTextColor(COLOR_RED);
    tft.setCursor(60, 180);
    tft.print(errMsg);
    delay(3000);
    ledReady();  // Torna a verde
    needsFullRedraw = true;
  });

  ArduinoOTA.begin();
  DEBUG_PRINTLN("OTA pronto");
}

// ========== WEB SERVER ==========
void setupWebServer() {
  server.enableCORS(true);

  // Pagina principale con tabs completi
  server.on("/", HTTP_GET, []() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Radar Display Client</title>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:Arial,sans-serif;padding:8px;background:#1a1a2e;color:#ddd;font-size:11px}
    .container{max-width:400px;margin:0 auto}
    h1{color:#00d4ff;text-align:center;font-size:14px;margin-bottom:2px}
    .time{text-align:center;color:#ffd700;margin-bottom:8px;font-size:10px}
    h2{color:#0af;font-size:11px;margin-bottom:5px;border-bottom:1px solid #0af;padding-bottom:2px}
    .card{background:#16213e;padding:8px;border-radius:4px;margin-bottom:6px}
    .grid{display:grid;grid-template-columns:repeat(4,1fr);gap:4px}
    .stat{background:#0f3460;padding:4px;border-radius:3px;text-align:center}
    .stat-value{font-size:12px;font-weight:bold;color:#0df}
    .stat-label{font-size:8px;color:#888}
    .g{color:#0f8}.r{color:#f44}.y{color:#fa0}
    .toggle-on{background:#0a0!important;color:#fff;font-weight:bold;padding:2px 8px;border-radius:4px}
    .toggle-off{background:#a00!important;color:#fff;font-weight:bold;padding:2px 8px;border-radius:4px}
    .row{display:flex;align-items:center;margin:3px 0}
    .row label:first-child{width:90px;font-size:10px}
    .row input[type=number]{width:45px}
    .row input[type=text]{width:90px}
    .row input,.row select{padding:2px 4px;border:1px solid #07a;border-radius:2px;background:#0f3460;color:#eee;font-size:10px}
    button{background:#07a;color:#fff;border:none;padding:4px 8px;border-radius:3px;cursor:pointer;margin:2px;font-size:10px}
    button:hover{background:#09c}
    button.danger{background:#a33}
    button.success{background:#3a3}
    .sw{position:relative;width:24px;height:14px;display:inline-block;vertical-align:middle}
    .sw input{opacity:0;width:0;height:0}
    .sl{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#444;border-radius:14px;transition:.2s}
    .sl:before{position:absolute;content:"";height:10px;width:10px;left:2px;bottom:2px;background:#fff;border-radius:50%;transition:.2s}
    input:checked+.sl{background:#0f8}
    input:checked+.sl:before{transform:translateX(10px)}
    .client{background:#0f3460;padding:4px;margin:3px 0;border-radius:3px;font-size:10px}
    .msg{padding:4px;border-radius:3px;margin:4px 0;display:none;font-size:10px}
    .msg.ok{background:#141;display:block}
    .msg.err{background:#411;display:block}
    .tabs{display:flex;border-bottom:1px solid #0af;margin-bottom:6px}
    .tab{padding:4px 8px;cursor:pointer;background:#0f3460;margin-right:2px;font-size:10px;border-radius:3px 3px 0 0}
    .tab.active{background:#07a}
    .tab-content{display:none}
    .tab-content.active{display:block}
    table{font-size:9px;border-collapse:collapse;margin-bottom:4px}
    th,td{padding:2px 4px;border:1px solid #444}
    th{background:#333}
    .btnbig{font-size:12px;padding:8px 16px}
    p{font-size:9px;color:#777;margin:4px 0}
  </style>
</head>
<body>
<div class="container">
  <h1>Radar Display Client</h1>
  <div class="time" id="datetime">--</div>

  <div class="tabs">
    <div class="tab active" onclick="showTab(0)">Stato</div>
    <div class="tab" onclick="showTab(1)">Impostazioni</div>
    <div class="tab" onclick="showTab(2)">Calibrazione</div>
    <div class="tab" onclick="showTab(3)">Dispositivi</div>
    <div class="tab" onclick="showTab(4)">OTA</div>
  </div>

  <!-- TAB STATO -->
  <div class="tab-content active" id="tab0">
    <div class="card">
      <h2>Radar</h2>
      <div class="grid">
        <div class="stat"><div class="stat-value" id="presence">--</div><div class="stat-label">Presenza</div></div>
        <div class="stat"><div class="stat-value" id="distance">--</div><div class="stat-label">Dist cm</div></div>
        <div class="stat"><div class="stat-value" id="moving">--</div><div class="stat-label">Mov</div></div>
        <div class="stat"><div class="stat-value" id="stationary">--</div><div class="stat-label">Stat</div></div>
      </div>
    </div>
    <div class="card">
      <h2>Ambiente</h2>
      <div class="grid">
        <div class="stat"><div class="stat-value" id="temp">--</div><div class="stat-label">Temp C</div></div>
        <div class="stat"><div class="stat-value" id="hum">--</div><div class="stat-label">Umid %</div></div>
        <div class="stat"><div class="stat-value" id="press">--</div><div class="stat-label">hPa</div></div>
        <div class="stat"><div class="stat-value" id="light">--</div><div class="stat-label">Luce</div></div>
      </div>
    </div>
    <div class="card">
      <h2>Sistema</h2>
      <div class="grid">
        <div class="stat"><div class="stat-value" id="ip">--</div><div class="stat-label">IP</div></div>
        <div class="stat"><div class="stat-value" id="radar">--</div><div class="stat-label">Radar</div></div>
        <div class="stat"><div class="stat-value" id="uptime">--</div><div class="stat-label">Up</div></div>
        <div class="stat"><div class="stat-value" id="heap">--</div><div class="stat-label">Heap</div></div>
      </div>
    </div>
  </div>

  <!-- TAB IMPOSTAZIONI -->
  <div class="tab-content" id="tab1">
    <div class="card">
      <h2>Automatico</h2>
      <div class="row"><label>Auto Presenza</label><label class="sw"><input type="checkbox" id="autoPresence" onchange="toggleSetting('presence')"><span class="sl"></span></label></div>
      <div class="row"><label>Auto Brightness</label><label class="sw"><input type="checkbox" id="autoBrightness" onchange="toggleSetting('brightness')"><span class="sl"></span></label></div>
      <div class="row"><label>Timeout (ms)</label><input type="number" id="presenceTimeout" min="1000" max="300000" step="1000"></div>
      <div class="row"><label>Bright Min</label><input type="number" id="minBrightness" min="0" max="255"></div>
      <div class="row"><label>Bright Max</label><input type="number" id="maxBrightness" min="0" max="255"></div>
      <button onclick="saveSettings()">Salva</button>
      <div class="msg" id="msgSettings"></div>
    </div>
    <div class="card">
      <h2>Sensibilita Radar</h2>
      <table><tr><th>Val</th><th>Sens</th></tr>
        <tr><td>0-20</td><td class="r">Alta</td></tr>
        <tr><td>30-50</td><td class="y">Media</td></tr>
        <tr><td>60-80</td><td>Bassa</td></tr>
        <tr><td>80-100</td><td class="g">Min</td></tr>
      </table>
      <div class="row"><label>Vicino 0-2m</label><input type="number" id="sensClose" min="0" max="100"></div>
      <div class="row"><label>Medio 2-4m</label><input type="number" id="sensMid" min="0" max="100"></div>
      <div class="row"><label>Lontano 4-6m</label><input type="number" id="sensFar" min="0" max="100"></div>
      <button onclick="saveRadarSens()">Applica</button>
      <div class="msg" id="msgRadar"></div>
    </div>
    <div class="card">
      <h2>Test Dispositivi</h2>
      <button class="success" onclick="api('/api/test/on')">ACCENDI TUTTI + RELE</button>
      <button class="danger" onclick="api('/api/test/off')">SPEGNI TUTTI + RELE</button>
      <p style="font-size:10px;color:#888;margin-top:8px">Il radar continua a gestire automaticamente</p>
    </div>
  </div>

  <!-- TAB CALIBRAZIONE -->
  <div class="tab-content" id="tab2">
    <div class="card">
      <h2>BME280</h2>
      <div class="grid">
        <div class="stat"><div class="stat-value" id="rawTemp">--</div><div class="stat-label">Raw T</div></div>
        <div class="stat"><div class="stat-value" id="calTemp">--</div><div class="stat-label">Cal T</div></div>
        <div class="stat"><div class="stat-value" id="rawHum">--</div><div class="stat-label">Raw H</div></div>
        <div class="stat"><div class="stat-value" id="calHum">--</div><div class="stat-label">Cal H</div></div>
      </div>
      <div class="row"><label>Offset Temp</label><input type="number" id="tempOffset" step="0.1" min="-10" max="10"></div>
      <div class="row"><label>Offset Umid</label><input type="number" id="humOffset" step="0.1" min="-20" max="20"></div>
      <button onclick="saveCalibration()">Salva</button>
      <button class="danger" onclick="resetCalibration()">Reset</button>
      <div class="msg" id="msgCal"></div>
    </div>
    <div class="card">
      <h2>Touch</h2>
      <div class="row"><label>Stato</label><span id="touchStatus">--</span></div>
      <div class="row"><label>Valori</label><span id="touchCalData">--</span></div>
      <button class="success" onclick="calibrateTouch()">Calibra</button>
      <button class="danger" onclick="resetTouch()">Reset</button>
      <div class="msg" id="msgTouch"></div>
      <p>Tocca i 4 angoli indicati sullo schermo.</p>
    </div>
  </div>

  <!-- TAB DISPOSITIVI -->
  <div class="tab-content" id="tab3">
    <div class="card">
      <h2>Discovery</h2>
      <div class="row"><label>Stato</label><span id="discStatus">--</span></div>
      <div class="row"><label>Progresso</label><span id="discProgress">--</span></div>
      <button class="success" onclick="startDiscovery()">Scan</button>
      <button class="danger" onclick="stopDiscovery()">Stop</button>
      <button onclick="toggleDiscovery()">Auto:<span id="discAuto">--</span></button>
    </div>
    <div class="card">
      <h2>Aggiungi</h2>
      <div class="row"><label>IP</label><input type="text" id="newIp" placeholder="192.168.1.x"></div>
      <div class="row"><label>Porta</label><input type="number" id="newPort" value="80"></div>
      <div class="row"><label>Nome</label><input type="text" id="newName" placeholder="Nome"></div>
      <button onclick="addDevice()">Aggiungi</button>
      <button onclick="testNotify()">Test</button>
    </div>
    <div class="card">
      <h2>Dispositivi (<span id="devCount">0</span>)</h2>
      <div id="deviceList">--</div>
    </div>
  </div>

  <!-- TAB OTA -->
  <div class="tab-content" id="tab4">
    <div class="card">
      <h2>Firmware OTA</h2>
      <form method='POST' action='/doUpdate' enctype='multipart/form-data'>
        <p>Seleziona file .bin:</p>
        <input type='file' name='update' accept='.bin' required>
        <button type='submit'>Carica</button>
      </form>
    </div>
    <div class="card">
      <h2>Sistema</h2>
      <button class="danger" onclick="if(confirm('Riavviare?'))location='/api/restart'">Riavvia</button>
    </div>
  </div>
</div>

<script>
let tabs=document.querySelectorAll('.tab');
let contents=document.querySelectorAll('.tab-content');
function showTab(n){tabs.forEach((t,i)=>{t.classList.toggle('active',i==n);contents[i].classList.toggle('active',i==n)})}

function api(url){return fetch(url).then(r=>r.json())}

function updateMode(){
  api('/api/status').then(d=>{
    let mode=document.getElementById('modeStatus');
    if(d.manualOverride){
      mode.innerHTML=d.presence?'<span style="color:orange">MANUALE ON</span>':'<span style="color:red">MANUALE OFF</span>';
    }else{
      mode.innerHTML='<span style="color:#0f0">AUTOMATICO</span>';
    }
  });
}

function updateStatus(){
  api('/api/status').then(d=>{
    document.getElementById('datetime').textContent=d.day+' '+d.date+' '+d.time;
    document.getElementById('presence').innerHTML=d.presence?'<span class="g">SI</span>':'<span class="r">NO</span>';
    document.getElementById('distance').textContent=d.distance;
    document.getElementById('moving').innerHTML=d.moving?'<span class="y">SI</span>':'NO';
    document.getElementById('stationary').innerHTML=d.stationary?'<span class="y">SI</span>':'NO';
    document.getElementById('temp').textContent=d.temperature.toFixed(1);
    document.getElementById('hum').textContent=d.humidity.toFixed(1);
    document.getElementById('press').textContent=d.pressure.toFixed(0);
    document.getElementById('light').textContent=d.ambientLight;
    document.getElementById('ip').textContent=d.ip||'--';
    document.getElementById('radar').innerHTML=d.radarOnline?'<span class="g">ON</span>':'<span class="r">OFF</span>';
    document.getElementById('uptime').textContent=Math.floor(d.uptime/60)+'m';
    document.getElementById('heap').textContent=Math.floor(d.freeHeap/1024);
    document.getElementById('autoPresence').checked=d.autoPresence;
    document.getElementById('autoBrightness').checked=d.autoBrightness;
    document.getElementById('rawTemp').textContent=(d.temperature-d.tempOffset).toFixed(1);
    document.getElementById('calTemp').textContent=d.temperature.toFixed(1);
    document.getElementById('rawHum').textContent=(d.humidity-d.humOffset).toFixed(1);
    document.getElementById('calHum').textContent=d.humidity.toFixed(1);
    document.getElementById('devCount').textContent=d.clientCount;
    // Aggiorna stato modalita
    let mode=document.getElementById('modeStatus');
    if(d.manualOverride){
      mode.innerHTML=d.presence?'<span style="color:orange">MANUALE ON</span>':'<span style="color:red">MANUALE OFF</span>';
    }else{
      mode.innerHTML='<span style="color:#0f0">AUTOMATICO</span>';
    }
  });
}

function loadEditableFields(){
  api('/api/status').then(d=>{
    document.getElementById('presenceTimeout').value=d.presenceTimeout;
    document.getElementById('minBrightness').value=d.minBrightness;
    document.getElementById('maxBrightness').value=d.maxBrightness;
    document.getElementById('tempOffset').value=d.tempOffset;
    document.getElementById('humOffset').value=d.humOffset;
    document.getElementById('sensClose').value=d.sensClose;
    document.getElementById('sensMid').value=d.sensMid;
    document.getElementById('sensFar').value=d.sensFar;
  });
  loadDevices();
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

function saveRadarSens(){
  let c=document.getElementById('sensClose').value;
  let m=document.getElementById('sensMid').value;
  let f=document.getElementById('sensFar').value;
  api('/api/radar/sensitivity?close='+c+'&mid='+m+'&far='+f).then(d=>{
    if(d.status=='ok') showMsg('msgRadar','ok','Sensibilita applicata!');
    else showMsg('msgRadar','err','Errore: '+d.error);
  });
}

function saveCalibration(){
  let t=document.getElementById('tempOffset').value;
  let h=document.getElementById('humOffset').value;
  api('/api/calibration/set?tempOffset='+t+'&humOffset='+h).then(d=>{
    showMsg('msgCal','ok','Calibrazione salvata!');
  });
}

function resetCalibration(){
  api('/api/calibration/reset').then(d=>{
    showMsg('msgCal','ok','Reset completato!');
    loadEditableFields();
  });
}

function loadDevices(){
  api('/api/devices').then(d=>{
    let h='';
    if(d.count==0)h='<p>Nessun dispositivo registrato</p>';
    else d.devices.forEach(c=>{
      h+='<div class="client" style="margin:8px 0;padding:8px;background:#222;border-radius:6px">';
      h+='<div style="margin-bottom:6px"><b>'+c.name+'</b> - '+c.ip+':'+c.port+'</div>';
      h+='<div style="display:flex;gap:4px;align-items:center;flex-wrap:wrap">';
      h+='<button class="'+(c.controlOnOff?'toggle-on':'toggle-off')+'" onclick="toggleOnOff('+c.index+','+!c.controlOnOff+')">';
      h+='CTRL: '+(c.controlOnOff?'ON':'OFF')+'</button>';
      h+='<button class="success" onclick="sendCmd('+c.index+',\'on\')">ACCENDI</button>';
      h+='<button class="danger" onclick="sendCmd('+c.index+',\'off\')">SPEGNI</button>';
      h+='<button onclick="removeDevice('+c.index+')">X</button>';
      h+='</div></div>';
    });
    document.getElementById('deviceList').innerHTML=h;
  });
}

function toggleOnOff(idx,val){
  api('/api/devices/update?index='+idx+'&controlOnOff='+(val?'true':'false')).then(d=>loadDevices());
}

function loadDiscoveryStatus(){
  api('/api/discovery/status').then(d=>{
    document.getElementById('discStatus').innerHTML=d.active?'<span class="y">Scansione...</span>':'<span class="g">Pronto</span>';
    document.getElementById('discProgress').textContent=d.active?(d.progress+'% (IP '+d.currentIP+'/'+d.endIP+') - Trovati: '+d.foundCount):'--';
    document.getElementById('discAuto').innerHTML=d.enabled?'<span class="g">ON</span>':'<span class="r">OFF</span>';
  });
}

function startDiscovery(){
  api('/api/discovery/start').then(d=>{loadDiscoveryStatus();});
}

function stopDiscovery(){
  api('/api/discovery/stop').then(d=>{loadDiscoveryStatus();loadDevices();});
}

function toggleDiscovery(){
  api('/api/discovery/toggle').then(d=>{loadDiscoveryStatus();});
}

function sendCmd(idx,cmd){
  api('/api/devices/command?index='+idx+'&cmd='+cmd).then(d=>{
    console.log('Comando '+cmd+' a device '+idx+': '+d.status);
  });
}

function addDevice(){
  let ip=document.getElementById('newIp').value;
  let name=document.getElementById('newName').value||'Device';
  let port=document.getElementById('newPort').value||80;
  if(!ip)return alert('Inserisci IP');
  api('/api/devices/add?ip='+ip+'&name='+encodeURIComponent(name)+'&port='+port).then(d=>{
    document.getElementById('newIp').value='';
    document.getElementById('newName').value='';
    loadDevices();
  });
}

function removeDevice(idx){
  if(confirm('Rimuovere questo dispositivo?'))api('/api/devices/remove?index='+idx).then(d=>loadDevices());
}

function testNotify(){
  api('/api/devices/test').then(d=>alert('Notifica inviata a '+d.notified+' dispositivi'));
}

function showMsg(id,type,text){
  let el=document.getElementById(id);
  el.className='msg '+type;
  el.textContent=text;
  setTimeout(()=>el.className='msg',3000);
}

function loadTouchStatus(){
  api('/api/touch/status').then(d=>{
    document.getElementById('touchStatus').innerHTML=d.calibrated?'<span class="g">Calibrato</span>':'<span class="r">Non calibrato</span>';
    document.getElementById('touchCalData').textContent='{ '+d.calData.join(', ')+' }';
  });
}

function calibrateTouch(){
  if(confirm('Avviare la calibrazione touch? Dovrai toccare i 4 angoli del display.')){
    showMsg('msgTouch','ok','Calibrazione avviata - guarda lo schermo!');
    api('/api/touch/calibrate').then(d=>{
      setTimeout(()=>{loadTouchStatus();showMsg('msgTouch','ok','Calibrazione completata!')},5000);
    });
  }
}

function resetTouch(){
  if(confirm('Resettare la calibrazione touch?')){
    api('/api/touch/reset').then(d=>{
      loadTouchStatus();
      showMsg('msgTouch','ok','Calibrazione resettata');
    });
  }
}

loadEditableFields();
loadTouchStatus();
loadDiscoveryStatus();
updateStatus();
setInterval(updateStatus,3000);
setInterval(loadDiscoveryStatus,2000);
</script>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
  });

  // API Status JSON completo
  server.on("/api/status", HTTP_GET, []() {
    String json = "{";
    json += "\"name\":\"" + String(deviceName) + "\",";
    json += "\"radarOnline\":" + String(radarOnline ? "true" : "false") + ",";
    json += "\"firmware\":\"" + radarFirmware + "\",";
    json += "\"presence\":" + String(presenceDetected ? "true" : "false") + ",";
    json += "\"distance\":" + String(radarDistance) + ",";
    json += "\"movingDistance\":" + String(radarMovingDistance) + ",";
    json += "\"stationaryDistance\":" + String(radarStationaryDistance) + ",";
    json += "\"moving\":" + String(radarMoving ? "true" : "false") + ",";
    json += "\"stationary\":" + String(radarStationary ? "true" : "false") + ",";
    json += "\"movingEnergy\":" + String(radarMovingEnergy) + ",";
    json += "\"stationaryEnergy\":" + String(radarStationaryEnergy) + ",";
    json += "\"ambientLight\":" + String(ambientLight) + ",";
    json += "\"brightness\":" + String(calculatedBrightness) + ",";
    json += "\"temperature\":" + String(bmeTemperature, 1) + ",";
    json += "\"humidity\":" + String(bmeHumidity, 1) + ",";
    json += "\"pressure\":" + String(bmePressure, 0) + ",";
    json += "\"bme280\":" + String(bme280Initialized ? "true" : "false") + ",";
    json += "\"time\":\"" + currentTime + "\",";
    json += "\"date\":\"" + currentDate + "\",";
    json += "\"day\":\"" + currentDay + "\",";
    json += "\"ntpSynced\":" + String(ntpSynced ? "true" : "false") + ",";
    json += "\"clientCount\":" + String(clientCount) + ",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"autoPresence\":" + String(autoPresenceEnabled ? "true" : "false") + ",";
    json += "\"autoBrightness\":" + String(autoBrightnessEnabled ? "true" : "false") + ",";
    json += "\"radarInitialized\":" + String(radarInitialized ? "true" : "false") + ",";
    json += "\"relayEnabled\":" + String(relayEnabled ? "true" : "false") + ",";
    json += "\"previousPresenceState\":" + String(previousPresenceState ? "true" : "false") + ",";
    json += "\"lastPresenceTime\":" + String(lastPresenceTime) + ",";
    json += "\"timeSincePresence\":" + String(millis() - lastPresenceTime) + ",";
    json += "\"presenceTimeout\":" + String(presenceTimeout) + ",";
    json += "\"minBrightness\":" + String(minBrightness) + ",";
    json += "\"maxBrightness\":" + String(maxBrightness) + ",";
    json += "\"tempOffset\":" + String(tempOffset, 1) + ",";
    json += "\"humOffset\":" + String(humOffset, 1) + ",";
    json += "\"sensClose\":" + String(radarSensClose) + ",";
    json += "\"sensMid\":" + String(radarSensMid) + ",";
    json += "\"sensFar\":" + String(radarSensFar) + ",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    // Sensore monitor TEMT6000
    json += "\"monitorLight\":" + String(monitorLightLevel) + ",";
    json += "\"monitorDetected\":" + String(monitorDetectedOn ? "true" : "false") + ",";
    json += "\"monitorDesired\":" + String(monitorStateDesired ? "true" : "false") + ",";
    json += "\"monitorSync\":" + String(monitorSyncEnabled ? "true" : "false") + ",";
    json += "\"monitorInSync\":" + String(monitorDetectedOn == monitorStateDesired ? "true" : "false");
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

  // API ricezione luminosita dal server
  server.on("/radar/brightness", HTTP_GET, []() {
    String value = server.arg("value");
    targetBrightness = value.toInt();
    server.send(200, "application/json", "{\"status\":\"ok\",\"brightness\":" + String(targetBrightness) + "}");
  });

  // API ricezione presenza dal server
  server.on("/radar/presence", HTTP_GET, []() {
    String value = server.arg("value");
    // Non sovrascrive la presenza locale, solo log
    DEBUG_PRINTF("Ricevuta presenza dal server: %s\n", value.c_str());
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // API ricezione test dal server
  server.on("/radar/test", HTTP_GET, []() {
    String value = server.arg("value");
    DEBUG_PRINTF("Ricevuto test dal server: %s\n", value.c_str());
    server.send(200, "application/json", "{\"status\":\"ok\",\"received\":\"" + value + "\"}");
  });

  // API ricezione temperatura dal server
  server.on("/radar/temperature", HTTP_GET, []() {
    String value = server.arg("value");
    DEBUG_PRINTF("Ricevuta temperatura dal server: %s\n", value.c_str());
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // API ricezione umidita dal server
  server.on("/radar/humidity", HTTP_GET, []() {
    String value = server.arg("value");
    DEBUG_PRINTF("Ricevuta umidita dal server: %s\n", value.c_str());
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // Endpoint sensorData per client esterni (OROLOGIO_GRANDE, ecc.)
  server.on("/sensorData", HTTP_GET, []() {
    int lightPercent = map(ambientLight, 0, 4095, 0, 100);
    String json = "{";
    json += "\"light\":{\"level\":" + String(ambientLight) + ",\"percentage\":" + String(lightPercent) + "},";
    json += "\"bme280\":{\"temperature\":" + String(bmeTemperature, 1) + ",\"humidity\":" + String(bmeHumidity, 1) + "},";
    json += "\"radar\":{\"presenceDetected\":" + String(presenceDetected ? "true" : "false") + "}";
    json += "}";
    server.send(200, "application/json", json);
  });

  // Pagina OTA Web Update
  server.on("/update", HTTP_GET, []() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>OTA Update</title>
  <meta charset='UTF-8'>
  <style>
    body { font-family: Arial; background: #1a1a2e; color: #eee; padding: 20px; text-align: center; }
    h1 { color: #e94560; }
    .upload-form { background: #16213e; padding: 30px; border-radius: 10px; max-width: 400px; margin: 20px auto; }
    input[type=file] { margin: 20px 0; }
    .btn { background: #e94560; color: #fff; padding: 15px 30px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
    .btn:hover { background: #ff6b6b; }
  </style>
</head>
<body>
  <h1>Aggiornamento Firmware OTA</h1>
  <div class='upload-form'>
    <form method='POST' action='/doUpdate' enctype='multipart/form-data'>
      <p>Seleziona il file .bin del firmware:</p>
      <input type='file' name='update' accept='.bin' required>
      <br><br>
      <button type='submit' class='btn'>Carica Firmware</button>
    </form>
    <br>
    <a href='/'><button class='btn' style='background:#0f3460'>Torna alla Home</button></a>
  </div>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
  });

  // Gestione upload OTA
  server.on("/doUpdate", HTTP_POST,
    []() {
      server.sendHeader("Connection", "close");
      String result = Update.hasError() ? "FAIL" : "OK";
      String html = "<html><body style='background:#1a1a2e;color:#eee;text-align:center;padding:50px;font-family:Arial;'>";
      html += "<h1 style='color:" + String(Update.hasError() ? "#ff4444" : "#00ff88") + ";'>";
      html += "Aggiornamento: " + result + "</h1>";
      html += "<p>Il dispositivo si riavviera...</p></body></html>";
      server.send(200, "text/html", html);
      delay(1000);
      ESP.restart();
    },
    []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        DEBUG_PRINTF("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
          DEBUG_PRINTF("Update Success: %u bytes\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
    }
  );

  // ========== API GESTIONE DISPOSITIVI ==========

  server.on("/api/devices", HTTP_GET, []() {
    String json = "{\"count\":" + String(clientCount) + ",\"devices\":[";
    for (int i = 0; i < clientCount; i++) {
      if (i > 0) json += ",";
      json += "{\"index\":" + String(i);
      json += ",\"ip\":\"" + clients[i].ip + "\"";
      json += ",\"name\":\"" + clients[i].name + "\"";
      json += ",\"port\":" + String(clients[i].port);
      json += ",\"active\":" + String(clients[i].active ? "true" : "false");
      json += ",\"notifyPresence\":" + String(clients[i].notifyPresence ? "true" : "false");
      json += ",\"notifyBrightness\":" + String(clients[i].notifyBrightness ? "true" : "false");
      json += ",\"controlOnOff\":" + String(clients[i].controlOnOff ? "true" : "false");
      json += ",\"currentState\":" + String(clients[i].currentState ? "true" : "false");
      json += ",\"lastSeen\":" + String(clients[i].lastSeen);
      json += ",\"failCount\":" + String(clients[i].failCount);
      json += "}";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  server.on("/api/devices/add", HTTP_GET, []() {
    String ip = server.arg("ip");
    String name = server.arg("name");
    int port = server.arg("port").toInt();
    if (port == 0) port = 80;

    if (ip.length() == 0) {
      server.send(400, "application/json", "{\"error\":\"IP richiesto\"}");
      return;
    }

    int idx = addClient(ip, name.length() > 0 ? name : "Device", port);
    if (idx >= 0) {
      server.send(200, "application/json", "{\"status\":\"ok\",\"index\":" + String(idx) + "}");
    } else {
      server.send(400, "application/json", "{\"error\":\"Limite dispositivi raggiunto\"}");
    }
  });

  server.on("/api/devices/remove", HTTP_GET, []() {
    int idx = server.arg("index").toInt();
    if (removeClient(idx)) {
      server.send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      server.send(400, "application/json", "{\"error\":\"Indice non valido\"}");
    }
  });

  server.on("/api/devices/update", HTTP_GET, []() {
    int idx = server.arg("index").toInt();
    if (idx < 0 || idx >= clientCount) {
      server.send(400, "application/json", "{\"error\":\"Indice non valido\"}");
      return;
    }

    if (server.hasArg("name")) clients[idx].name = server.arg("name");
    if (server.hasArg("active")) clients[idx].active = (server.arg("active") == "true");
    if (server.hasArg("notifyPresence")) clients[idx].notifyPresence = (server.arg("notifyPresence") == "true");
    if (server.hasArg("notifyBrightness")) clients[idx].notifyBrightness = (server.arg("notifyBrightness") == "true");
    if (server.hasArg("controlOnOff")) clients[idx].controlOnOff = (server.arg("controlOnOff") == "true");

    saveClients();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/devices/test", HTTP_GET, []() {
    notifyClients("presence", presenceDetected ? "true" : "false");
    server.send(200, "application/json", "{\"status\":\"ok\",\"notified\":" + String(clientCount) + "}");
  });

  // Reset contatore fallimenti di un dispositivo
  server.on("/api/devices/reset", HTTP_GET, []() {
    int idx = server.arg("index").toInt();
    if (idx < 0 || idx >= clientCount) {
      server.send(400, "application/json", "{\"error\":\"Indice non valido\"}");
      return;
    }
    clients[idx].failCount = 0;
    DEBUG_PRINTF("Reset fallimenti per %s:%d\n", clients[idx].ip.c_str(), clients[idx].port);
    server.send(200, "application/json", "{\"status\":\"ok\",\"index\":" + String(idx) + "}");
  });

  // Reset contatore fallimenti di tutti i dispositivi
  server.on("/api/devices/resetAll", HTTP_GET, []() {
    int resetCount = 0;
    for (int i = 0; i < clientCount; i++) {
      if (clients[i].failCount > 0) {
        clients[i].failCount = 0;
        resetCount++;
      }
    }
    DEBUG_PRINTF("Reset fallimenti per %d dispositivi\n", resetCount);
    server.send(200, "application/json", "{\"status\":\"ok\",\"resetCount\":" + String(resetCount) + "}");
  });

  // ========== API DISCOVERY ==========

  server.on("/api/discovery/start", HTTP_GET, []() {
    startDiscovery();
    server.send(200, "application/json", "{\"status\":\"started\"}");
  });

  server.on("/api/discovery/stop", HTTP_GET, []() {
    stopDiscovery();
    server.send(200, "application/json", "{\"status\":\"stopped\"}");
  });

  server.on("/api/discovery/status", HTTP_GET, []() {
    String json = "{";
    json += "\"active\":" + String(discoveryActive ? "true" : "false") + ",";
    json += "\"enabled\":" + String(discoveryEnabled ? "true" : "false") + ",";
    json += "\"currentIP\":" + String(discoveryCurrentIP) + ",";
    json += "\"endIP\":" + String(discoveryEndIP) + ",";
    json += "\"foundCount\":" + String(discoveryFoundCount) + ",";
    json += "\"progress\":" + String((discoveryCurrentIP * 100) / discoveryEndIP);
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/api/discovery/toggle", HTTP_GET, []() {
    discoveryEnabled = !discoveryEnabled;
    server.send(200, "application/json", "{\"enabled\":" + String(discoveryEnabled ? "true" : "false") + "}");
  });

  // ========== API COMANDI ON/OFF ==========

  server.on("/api/devices/on", HTTP_GET, []() {
    sendAllOn();
    server.send(200, "application/json", "{\"status\":\"ok\",\"command\":\"on\"}");
  });

  server.on("/api/devices/off", HTTP_GET, []() {
    sendAllOff();
    server.send(200, "application/json", "{\"status\":\"ok\",\"command\":\"off\"}");
  });

  server.on("/api/devices/command", HTTP_GET, []() {
    if (!server.hasArg("index") || !server.hasArg("cmd")) {
      server.send(400, "application/json", "{\"error\":\"Parametri index e cmd richiesti\"}");
      return;
    }
    int idx = server.arg("index").toInt();
    String cmd = server.arg("cmd");
    bool turnOn = (cmd == "on" || cmd == "1" || cmd == "true");
    bool success = sendClientCommand(idx, turnOn);
    server.send(200, "application/json", "{\"status\":\"" + String(success ? "ok" : "fail") + "\",\"index\":" + String(idx) + ",\"command\":\"" + cmd + "\"}");
  });

  // Endpoint per rispondere al discovery da altri dispositivi
  server.on("/radar/ping", HTTP_GET, []() {
    String json = "{";
    json += "\"name\":\"" + String(deviceName) + "\",";
    json += "\"type\":\"radar-display\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"radarOnline\":" + String(radarOnline ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  // Endpoint ON - altri dispositivi possono accenderci
  server.on("/radar/on", HTTP_GET, []() {
    DEBUG_PRINTLN("Ricevuto comando ON remoto");
    setLEDBrightness(maxBrightness);
    server.send(200, "application/json", "{\"status\":\"ok\",\"state\":\"on\"}");
  });

  // Endpoint OFF - altri dispositivi possono spegnerci
  server.on("/radar/off", HTTP_GET, []() {
    DEBUG_PRINTLN("Ricevuto comando OFF remoto");
    setLEDBrightness(0);
    server.send(200, "application/json", "{\"status\":\"ok\",\"state\":\"off\"}");
  });

  // ========== API CALIBRAZIONE BME280 ==========

  server.on("/api/calibration", HTTP_GET, []() {
    String json = "{";
    json += "\"tempOffset\":" + String(tempOffset, 1) + ",";
    json += "\"humOffset\":" + String(humOffset, 1) + ",";
    json += "\"rawTemperature\":" + String(bmeTemperature - tempOffset, 1) + ",";
    json += "\"rawHumidity\":" + String(bmeHumidity - humOffset, 1) + ",";
    json += "\"temperature\":" + String(bmeTemperature, 1) + ",";
    json += "\"humidity\":" + String(bmeHumidity, 1) + ",";
    json += "\"pressure\":" + String(bmePressure, 1);
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/api/calibration/set", HTTP_GET, []() {
    if (server.hasArg("tempOffset")) {
      tempOffset = server.arg("tempOffset").toFloat();
    }
    if (server.hasArg("humOffset")) {
      humOffset = server.arg("humOffset").toFloat();
    }
    saveSettings();
    DEBUG_PRINTF("Calibrazione salvata: tempOffset=%.1f, humOffset=%.1f\n", tempOffset, humOffset);
    server.send(200, "application/json", "{\"status\":\"ok\",\"tempOffset\":" + String(tempOffset, 1) + ",\"humOffset\":" + String(humOffset, 1) + "}");
  });

  server.on("/api/calibration/reset", HTTP_GET, []() {
    tempOffset = 0.0;
    humOffset = 0.0;
    saveSettings();
    DEBUG_PRINTLN("Calibrazione resettata a 0");
    server.send(200, "application/json", "{\"status\":\"ok\",\"tempOffset\":0,\"humOffset\":0}");
  });

  // ========== API SENSIBILITA RADAR ==========

  server.on("/api/radar/sensitivity", HTTP_GET, []() {
    if (!server.hasArg("close") || !server.hasArg("mid") || !server.hasArg("far")) {
      server.send(400, "application/json", "{\"status\":\"error\",\"error\":\"Parametri mancanti\"}");
      return;
    }

    int newClose = server.arg("close").toInt();
    int newMid = server.arg("mid").toInt();
    int newFar = server.arg("far").toInt();

    // Valida range 0-100
    newClose = constrain(newClose, 0, 100);
    newMid = constrain(newMid, 0, 100);
    newFar = constrain(newFar, 0, 100);

    radarSensClose = newClose;
    radarSensMid = newMid;
    radarSensFar = newFar;

    // Salva nelle preferences
    saveSettings();

    // Applica al radar
    bool applied = applyRadarSensitivity();

    String json = "{\"status\":\"ok\",";
    json += "\"applied\":" + String(applied ? "true" : "false") + ",";
    json += "\"sensClose\":" + String(radarSensClose) + ",";
    json += "\"sensMid\":" + String(radarSensMid) + ",";
    json += "\"sensFar\":" + String(radarSensFar) + "}";
    server.send(200, "application/json", json);

    DEBUG_PRINTF("Sensibilita radar aggiornata: %d/%d/%d\n", radarSensClose, radarSensMid, radarSensFar);
  });

  // Reset sensibilita radar ai valori di default
  server.on("/api/radar/reset", HTTP_GET, []() {
    radarSensClose = RADAR_SENS_CLOSE_DEFAULT;
    radarSensMid = RADAR_SENS_MID_DEFAULT;
    radarSensFar = RADAR_SENS_FAR_DEFAULT;

    // Salva nelle preferences
    saveSettings();

    // Applica al radar
    bool applied = applyRadarSensitivity();

    String json = "{\"status\":\"ok\",";
    json += "\"applied\":" + String(applied ? "true" : "false") + ",";
    json += "\"sensClose\":" + String(radarSensClose) + ",";
    json += "\"sensMid\":" + String(radarSensMid) + ",";
    json += "\"sensFar\":" + String(radarSensFar) + "}";
    server.send(200, "application/json", json);

    DEBUG_PRINTF("Sensibilita radar resettata ai default: %d/%d/%d\n", radarSensClose, radarSensMid, radarSensFar);
  });

  // ========== API CALIBRAZIONE TOUCH ==========

  server.on("/api/touch/status", HTTP_GET, []() {
    String json = "{";
    json += "\"calibrated\":" + String(touchCalibrated ? "true" : "false") + ",";
    json += "\"calData\":[";
    for (int i = 0; i < 5; i++) {
      json += String(touchCalData[i]);
      if (i < 4) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  server.on("/api/touch/calibrate", HTTP_GET, []() {
    server.send(200, "application/json", "{\"status\":\"starting\",\"message\":\"Calibrazione avviata - segui le istruzioni sullo schermo\"}");
    delay(100);
    touch_calibrate();
  });

  server.on("/api/touch/set", HTTP_GET, []() {
    // Imposta calibrazione manuale: /api/touch/set?c0=xxx&c1=xxx&c2=xxx&c3=xxx&c4=xxx
    if (server.hasArg("c0") && server.hasArg("c1") && server.hasArg("c2") &&
        server.hasArg("c3") && server.hasArg("c4")) {
      touchCalData[0] = server.arg("c0").toInt();
      touchCalData[1] = server.arg("c1").toInt();
      touchCalData[2] = server.arg("c2").toInt();
      touchCalData[3] = server.arg("c3").toInt();
      touchCalData[4] = server.arg("c4").toInt();
      tft.setTouch(touchCalData);
      touchCalibrated = true;
      saveTouchCalibration();
      String json = "{\"status\":\"ok\",\"calData\":[";
      for (int i = 0; i < 5; i++) {
        json += String(touchCalData[i]);
        if (i < 4) json += ",";
      }
      json += "]}";
      server.send(200, "application/json", json);
    } else {
      server.send(400, "application/json", "{\"error\":\"Richiesti parametri c0,c1,c2,c3,c4\"}");
    }
  });

  server.on("/api/touch/reset", HTTP_GET, []() {
    resetTouchCalibration();
    server.send(200, "application/json", "{\"status\":\"ok\",\"calibrated\":false}");
  });

  // ========== API NOTIFICHE ==========

  server.on("/api/notify/test", HTTP_GET, []() {
    notifyClients("test", "ping");
    server.send(200, "application/json", "{\"status\":\"sent\"}");
  });

  server.on("/api/notify/presence", HTTP_GET, []() {
    notifyClients("presence", presenceDetected ? "true" : "false");
    server.send(200, "application/json", "{\"status\":\"sent\"}");
  });

  server.on("/api/notify/brightness", HTTP_GET, []() {
    notifyClients("brightness", String(ambientLight));
    server.send(200, "application/json", "{\"status\":\"sent\",\"value\":" + String(ambientLight) + "}");
  });

  server.on("/api/notify/temperature", HTTP_GET, []() {
    if (bme280Initialized) {
      notifyClients("temperature", String(bmeTemperature, 1));
      delay(500);  // Delay per permettere al client di processare
      notifyClients("humidity", String(bmeHumidity, 1));
      server.send(200, "application/json", "{\"status\":\"sent\",\"temperature\":" + String(bmeTemperature, 1) + ",\"humidity\":" + String(bmeHumidity, 1) + "}");
    } else {
      server.send(200, "application/json", "{\"status\":\"error\",\"message\":\"BME280 not initialized\"}");
    }
  });

  server.on("/api/notify/humidity", HTTP_GET, []() {
    if (bme280Initialized) {
      notifyClients("humidity", String(bmeHumidity, 1));
      server.send(200, "application/json", "{\"status\":\"sent\",\"value\":" + String(bmeHumidity, 1) + "}");
    } else {
      server.send(200, "application/json", "{\"status\":\"error\",\"message\":\"BME280 not initialized\"}");
    }
  });

  server.on("/api/notify/all", HTTP_GET, []() {
    notifyClients("presence", presenceDetected ? "true" : "false");
    notifyClients("brightness", String(ambientLight));
    if (bme280Initialized) {
      notifyClients("temperature", String(bmeTemperature, 1));
      notifyClients("humidity", String(bmeHumidity, 1));
    }
    server.send(200, "application/json", "{\"status\":\"sent\"}");
  });

  // ========== API TEST ==========
  // NOTA: Questi endpoint sono solo per TEST manuale
  // NON bloccano il controllo automatico del radar

  server.on("/api/test/off", HTTP_GET, []() {
    // NON imposta manualPresenceOverride - il radar continua a funzionare
    notifyClients("presence", "false");
    sendAllOff();  // Spegni tutti i dispositivi LAN
    startRelayPulse();  // Attiva impulso rele
    needsFullRedraw = true;
    server.send(200, "application/json", "{\"status\":\"ok\",\"action\":\"test_off\"}");
  });

  server.on("/api/test/on", HTTP_GET, []() {
    // NON imposta manualPresenceOverride - il radar continua a funzionare
    notifyClients("presence", "true");
    sendAllOn();  // Accendi tutti i dispositivi LAN
    startRelayPulse();  // Attiva impulso rele
    needsFullRedraw = true;
    server.send(200, "application/json", "{\"status\":\"ok\",\"action\":\"test_on\"}");
  });

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

  // ========== API SISTEMA ==========

  server.on("/api/restart", HTTP_GET, []() {
    server.send(200, "text/html", "<html><body style='background:#1a1a2e;color:#eee;text-align:center;padding:50px;font-family:Arial;'><h1 style='color:#ffd700;'>Riavvio in corso...</h1><p>Attendere alcuni secondi...</p></body></html>");
    delay(1000);
    ESP.restart();
  });

  // ========== API SENSORE MONITOR ==========

  // Stato sensore monitor
  server.on("/api/monitor/status", HTTP_GET, []() {
    String json = "{";
    json += "\"lightLevel\":" + String(monitorLightLevel) + ",";
    json += "\"detectedOn\":" + String(monitorDetectedOn ? "true" : "false") + ",";
    json += "\"desiredOn\":" + String(monitorStateDesired ? "true" : "false") + ",";
    json += "\"syncEnabled\":" + String(monitorSyncEnabled ? "true" : "false") + ",";
    json += "\"thresholdOn\":" + String(MONITOR_THRESHOLD_ON) + ",";
    json += "\"thresholdOff\":" + String(MONITOR_THRESHOLD_OFF) + ",";
    json += "\"inSync\":" + String(monitorDetectedOn == monitorStateDesired ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  // Abilita/disabilita sincronizzazione
  server.on("/api/monitor/sync/toggle", HTTP_GET, []() {
    monitorSyncEnabled = !monitorSyncEnabled;
    String json = "{\"syncEnabled\":" + String(monitorSyncEnabled ? "true" : "false") + "}";
    server.send(200, "application/json", json);
    DEBUG_PRINTF("Monitor sync %s\n", monitorSyncEnabled ? "ABILITATO" : "DISABILITATO");
  });

  // Imposta stato desiderato manualmente
  server.on("/api/monitor/set", HTTP_GET, []() {
    if (server.hasArg("on")) {
      bool wantOn = (server.arg("on") == "true" || server.arg("on") == "1");
      setMonitorDesiredState(wantOn);
      String json = "{\"desiredOn\":" + String(monitorStateDesired ? "true" : "false") + "}";
      server.send(200, "application/json", json);
    } else {
      server.send(400, "application/json", "{\"error\":\"missing 'on' parameter\"}");
    }
  });

  // Toggle stato desiderato
  server.on("/api/monitor/toggle", HTTP_GET, []() {
    toggleMonitorState();
    String json = "{\"desiredOn\":" + String(monitorStateDesired ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  // Forza sincronizzazione immediata
  server.on("/api/monitor/sync", HTTP_GET, []() {
    if (monitorDetectedOn != monitorStateDesired) {
      startRelayPulse();
      server.send(200, "application/json", "{\"action\":\"relay_pulse\",\"reason\":\"force_sync\"}");
    } else {
      server.send(200, "application/json", "{\"action\":\"none\",\"reason\":\"already_in_sync\"}");
    }
  });

  server.begin();
  DEBUG_PRINTLN("WebServer avviato con tutte le API");
}

// ========== WIFI ==========
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);

  if (!WiFi.config(staticIP, gateway, subnet, dns)) {
    DEBUG_PRINTLN("Errore configurazione IP statico!");
  }

  WiFi.begin(ssid, password);

  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(2);
  tft.setCursor(60, 80);
  tft.print("Connessione");
  tft.setCursor(90, 110);
  tft.print("WiFi...");

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(80, 150);
  tft.print("SSID: ");
  tft.print(ssid);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    DEBUG_PRINT(".");
    tft.setCursor(100 + (attempts % 6) * 15, 180);
    tft.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINTLN("\nWiFi connesso!");
    DEBUG_PRINT("IP: ");
    DEBUG_PRINTLN(WiFi.localIP());

    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_GREEN);
    tft.setTextSize(2);
    tft.setCursor(60, 90);
    tft.print("CONNESSO!");

    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(60, 130);
    tft.print("IP: ");
    tft.print(WiFi.localIP().toString());

    delay(1500);

    if (MDNS.begin(hostname)) {
      DEBUG_PRINTF("mDNS: http://%s.local\n", hostname);
    }
  } else {
    DEBUG_PRINTLN("\nErrore connessione WiFi! Riavvio in corso...");

    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_RED);
    tft.setTextSize(2);
    tft.setCursor(40, 80);
    tft.print("WiFi ERRORE!");
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(50, 120);
    tft.print("Riavvio in 3 secondi...");
    delay(3000);
    ESP.restart();
  }
}

void updateBrightnessSmooth() {
  if (currentBrightness != targetBrightness) {
    unsigned long now = millis();
    if (now - lastBrightnessUpdate > 20) {
      if (currentBrightness < targetBrightness) {
        currentBrightness++;
      } else {
        currentBrightness--;
      }
      ledcWrite(LED_CHANNEL, currentBrightness);
      lastBrightnessUpdate = now;
    }
  }
}

// ========== SCHERMATA AVVIO ==========
void drawSplashScreen() {
  tft.fillScreen(COLOR_BG);

  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(3);
  tft.setCursor(55, 50);
  tft.print("RADAR");

  tft.setTextSize(2);
  tft.setCursor(60, 90);
  tft.print("LD2410");

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(35, 130);
  tft.print("ESP32 + ILI9341 + LD2410");

  tft.setTextColor(COLOR_GREEN);
  tft.setCursor(80, 155);
  tft.print("OTA Enabled");

  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(75, 185);
  tft.print("by Sambinello Paolo");

  tft.setCursor(85, 210);
  tft.print("v3.0 - 2026");

  delay(2000);
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(500);

  DEBUG_PRINTLN("\n========================================");
  DEBUG_PRINTLN("  RADAR LD2410 - Display Client + OTA");
  DEBUG_PRINTLN("  Versione completa con tutte le API");
  DEBUG_PRINTLN("========================================\n");

  // Inizializza Display
  setupDisplay();

  // Carica impostazioni e dispositivi
  loadSettings();
  loadClients();

  // Splash screen
  drawSplashScreen();

  // Inizializza LED PWM backlight
  setupLED();

  // Inizializza WS2812 Status LED
  setupStatusLed();
  ledBoot();  // LED viola durante avvio

  // Inizializza Rele
  setupRelay();

  // Inizializza sensore monitor TEMT6000
  setupMonitorSensor();

  // Inizializza controllo display PWM
  setupDisplayPWM();

  // Inizializza Radar LD2410
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(40, 100);
  tft.print("Inizializzazione radar...");
  setupRadar();

  tft.setCursor(40, 120);
  if (radarOnline) {
    tft.setTextColor(COLOR_GREEN);
    tft.print("Radar OK - FW: ");
    tft.print(radarFirmware);
  } else {
    tft.setTextColor(COLOR_RED);
    tft.print("Radar NON RISPONDE!");
  }
  delay(1000);

  // WiFi
  ledWiFiConnecting();  // LED blu durante connessione WiFi
  setupWiFi();

  // OTA e NTP
  if (WiFi.status() == WL_CONNECTED) {
    setupOTA();
    initNTP();
  }

  // Inizializza BME280
  initBME280();

  // WebServer
  setupWebServer();

  needsFullRedraw = true;

  DEBUG_PRINTLN("\n========================================");
  DEBUG_PRINTLN("      RADAR DISPLAY PRONTO");
  DEBUG_PRINTF("IP: %s\n", WiFi.localIP().toString().c_str());
  DEBUG_PRINTF("Hostname: %s.local\n", hostname);
  DEBUG_PRINTF("Radar: %s\n", radarOnline ? "ONLINE" : "OFFLINE");
  DEBUG_PRINTF("BME280: %s\n", bme280Initialized ? "OK" : "NON TROVATO");
  DEBUG_PRINTF("Rele: GPIO%d\n", RELAY_PIN);
  DEBUG_PRINTF("Monitor Sensor: GPIO%d (soglia ON=%d, OFF=%d)\n", MONITOR_SENSOR_PIN, MONITOR_THRESHOLD_ON, MONITOR_THRESHOLD_OFF);
  DEBUG_PRINTF("WS2812: GPIO%d\n", WS2812_PIN);
  DEBUG_PRINTF("Display PWM: GPIO%d\n", DISPLAY_PWM_PIN);
  DEBUG_PRINTLN("OTA: ArduinoOTA + Web Update attivi");
  DEBUG_PRINTLN("========================================\n");

  // Sistema pronto - LED spento
  ledReady();

  // Salva timestamp boot per ritardo attivazione rele
  bootTime = millis();
  DEBUG_PRINTF("Rele sara abilitato tra %d ms\n", RELAY_BOOT_DELAY_MS);
}

// ========== LOOP ==========
void loop() {
  if (otaInProgress) {
    ArduinoOTA.handle();
    return;
  }

  ArduinoOTA.handle();
  server.handleClient();

  updateBME280();
  updateNTPTime();
  handleTouch();
  updateBrightnessSmooth();

  unsigned long now = millis();

  // Abilita rele dopo ritardo boot
  if (!relayEnabled && (now - bootTime >= RELAY_BOOT_DELAY_MS)) {
    relayEnabled = true;
    pendingRelayPulse = false;  // Ignora attivazioni pendenti dal boot
    DEBUG_PRINTLN(">>> RELE: Abilitato (no attivazione al boot) <<<");
  }

  // Lettura dati radar - SEMPRE ad ogni loop (non bloccante)
  // Il radar LD2410 invia dati ogni ~100ms, dobbiamo leggerli frequentemente
  readRadarData();

  // Aggiorna presenza
  updatePresence();

  // Gestione impulso rele (non bloccante)
  bool wasRelayActive = relayActive;
  updateRelay();
  // Quando il rele termina, ripristina il colore LED appropriato
  if (wasRelayActive && !relayActive) {
    if (presenceDetected) {
      ledPresenceDetected();  // Giallo se c'e presenza
    } else {
      ledReady();  // Verde se idle
    }
  }

  // Aggiorna sensore monitor TEMT6000 e sincronizza relè
  updateMonitorSensor();
  syncMonitorState();

  // Aggiorna sensore luce
  updateLightSensor();

  // Discovery dispositivi (non bloccante)
  discoveryStep();

  // Auto-discovery periodico
  if (discoveryEnabled && DISCOVERY_AUTO_INTERVAL > 0 && !discoveryActive) {
    if (now - lastDiscoveryRun > DISCOVERY_AUTO_INTERVAL) {
      startDiscovery();
    }
  }

  // Aggiorna display
  if (now - lastDisplayUpdate > DISPLAY_UPDATE_MS) {
    updateDisplay();
    lastDisplayUpdate = now;
  }

  // NOTA: La notifica presenza e' gestita SOLO in updatePresence()
  // per evitare desync tra presenceDetected, previousPresenceState e lastNotifiedPresence
  // Il blocco seguente e' stato RIMOSSO perche' causava notifiche duplicate senza chiamare sendAllOn/Off
  // if (presenceDetected != lastNotifiedPresence) { ... }

  // Sincronizza lastNotifiedPresence con presenceDetected (senza notificare)
  lastNotifiedPresence = presenceDetected;

  // Notifica periodica luminosita e temperatura/umidita
  static float lastSentTemp = -100;
  static float lastSentHum = -100;
  if (now - lastClientNotify > 5000) {  // Ogni 5 secondi
    if (abs(ambientLight - lastSentLight) >= 10) {
      notifyClients("brightness", String(ambientLight));
      lastSentLight = ambientLight;
    }
    // Invia temperatura se cambiata di 0.5+ gradi
    if (bme280Initialized && abs(bmeTemperature - lastSentTemp) >= 0.5) {
      notifyClients("temperature", String(bmeTemperature, 1));
      lastSentTemp = bmeTemperature;
      delay(100);
      notifyClients("humidity", String(bmeHumidity, 1));
      lastSentHum = bmeHumidity;
    }
    lastClientNotify = now;
  }

  // Sincronizzazione completa ogni 60 secondi (ridotta per non bloccare)
  if (now - lastFullSync > 60000) {
    if (clientCount > 0 && clientCount <= 3) {  // Solo se pochi dispositivi
      notifyClients("presence", presenceDetected ? "true" : "false");
      // Rimuovi sync periodica di brightness/temp/hum per evitare blocchi
    }
    lastFullSync = now;
  }

  // Riconnessione WiFi
  static unsigned long lastWifiCheck = 0;
  static int wifiFailCount = 0;

  if (now - lastWifiCheck > 10000) {
    if (WiFi.status() != WL_CONNECTED) {
      wifiFailCount++;
      DEBUG_PRINTF("WiFi disconnesso, tentativo %d/3\n", wifiFailCount);
      WiFi.reconnect();
      needsFullRedraw = true;

      if (wifiFailCount >= 3) {
        DEBUG_PRINTLN("WiFi non recuperabile, riavvio...");
        ESP.restart();
      }
    } else {
      wifiFailCount = 0;
    }
    lastWifiCheck = now;
  }
}
