// ============================================================================
// RADAR CLIENT - ESP32 con Display ILI9341 320x240 + Touch Screen
// Client autonomo con sensore LD2410 locale + OTA Updates
// Funzionalita complete importate da RADAR_SERVER_C3
// ============================================================================
// Autore: Sambinello Paolo
// Hardware: ESP32 + ILI9341 320x240 TFT + XPT2046 Touch + LD2410 + BME280
// Libreria: TFT_eSPI (touch integrato), MyLD2410
// ============================================================================

#include <WiFi.h>
#include <esp_system.h>  // Per esp_reset_reason()
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
#include <WiFiManager.h>      // WiFiManager by tzapu - Captive portal per configurazione WiFi

// QR Code - Usa libreria ESP-IDF nativa (nessun conflitto)
#define USE_QRCODE
#ifdef USE_QRCODE
  #include "qrcode.h"         // ESP-IDF qrcode (già inclusa in ESP32)
#endif
// #include "DFRobot_MICS.h"  // Non compatibile con modulo Adafruit - uso lettura ADC diretta

// ========== CONFIGURAZIONE WIFI (WiFiManager) ==========
// Le credenziali WiFi vengono configurate tramite captive portal
// Non serve piu hardcodare SSID e password!
#define WIFI_AP_NAME "RadarDisplay-Setup"     // Nome Access Point per configurazione
#define WIFI_AP_PASSWORD "radar12345"         // Password AP (min 8 caratteri, vuoto = aperto)
#define WIFI_CONFIG_TIMEOUT 180               // Timeout portale configurazione (secondi)
WiFiManager wifiManager;
bool wifiConfigMode = false;                  // Flag modalita configurazione attiva

// Parametri IP Statico (salvati in NVS)
char static_ip[16] = "192.168.1.99";
char static_gateway[16] = "192.168.1.1";
char static_subnet[16] = "255.255.255.0";
char static_dns[16] = "192.168.1.1";
bool useStaticIP = true;                      // Usa IP statico (default: si)

// ========== CONFIGURAZIONE IP STATICO ==========
// NOTA: IP statico ora configurabile via WiFiManager captive portal
// I valori default sono definiti nella sezione WiFiManager sopra

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
// #define LED_PIN 25       // DISABILITATO - GPIO25 libero (EN sensore MICS saldato a GND)
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
bool relayDeviceOn = false;       // Stato stimato TV/monitor (false=spento al boot)

// ========== SENSORE MONITOR TEMT6000 (DISATTIVATO) ==========
// #define MONITOR_SENSOR_PIN 34         // GPIO34 (ADC1) per TEMT6000 puntato al monitor
// #define MONITOR_SENSOR_UPDATE_MS 500  // Lettura ogni 500ms
// #define MONITOR_THRESHOLD_ON 150      // Soglia per considerare monitor ACCESO (0-4095)
// #define MONITOR_THRESHOLD_OFF 50      // Soglia per considerare monitor SPENTO (isteresi)
// #define MONITOR_READINGS_AVG 5        // Numero letture per media (anti-rumore)
// int monitorLightLevel = 0;            // Livello luce rilevato dal sensore (0-4095)
// bool monitorDetectedOn = false;       // Stato rilevato: true = monitor acceso
// bool monitorStateDesired = false;     // Stato desiderato: true = vogliamo monitor acceso
// unsigned long lastMonitorSensorRead = 0;
// bool monitorSyncEnabled = true;       // Sync relè abilitato

// ========== SENSORE GAS DFRobot MICS ==========
#define MICS_CALIBRATION_TIME   2       // Tempo calibrazione in minuti
#define MICS_ADC_PIN            34      // GPIO34 (ADC1)
// #define MICS_POWER_PIN       25      // DISABILITATO - Pin EN saldato a GND sul modulo
#define GAS_SENSOR_UPDATE_MS    1000    // Lettura ogni 1 secondo

// Variabili letture gas (PPM)
float gasConcentrationCO = 0;         // Monossido di carbonio
float gasConcentrationCH4 = 0;        // Metano
float gasConcentrationC2H5OH = 0;     // Etanolo
float gasConcentrationH2 = 0;         // Idrogeno
float gasConcentrationNH3 = 0;        // Ammoniaca
float gasConcentrationNO2 = 0;        // Biossido di azoto
unsigned long lastGasSensorRead = 0;
bool gasSensorReady = false;          // Sensore pronto dopo warmup
bool gasSensorConnected = false;      // Sensore connesso
unsigned long gasSensorWarmupStart = 0;  // Timestamp inizio warmup

// ========== SOGLIE ALLARME GAS (PPM) ==========
#define GAS_THRESHOLD_CO      50.0    // Monossido di carbonio (pericoloso > 35ppm)
#define GAS_THRESHOLD_CH4     1000.0  // Metano (LEL ~5% = 50000ppm, allarme precoce)
#define GAS_THRESHOLD_C2H5OH  500.0   // Etanolo
#define GAS_THRESHOLD_H2      500.0   // Idrogeno
#define GAS_THRESHOLD_NH3     25.0    // Ammoniaca (irritante > 25ppm)
#define GAS_THRESHOLD_NO2     5.0     // Biossido azoto (pericoloso > 5ppm)

// Stato allarme gas
bool gasAlarmActive = false;          // Allarme attivo
String gasAlarmType = "";             // Tipo di gas che ha causato allarme
float gasAlarmValue = 0;              // Valore del gas in allarme
unsigned long lastGasAlarmSent = 0;   // Ultimo invio allarme
#define GAS_ALARM_INTERVAL_MS 5000    // Intervallo minimo tra allarmi (5 sec)
bool gasAlarmAcknowledged = false;    // Allarme confermato dall'utente

// Variabili per compatibilità con logica monitor (legacy)
bool monitorDetectedOn = false;       // Legacy: mantiene compatibilità
bool monitorStateDesired = false;     // Legacy: mantiene compatibilità
int monitorLightLevel = 0;            // Legacy: mantiene compatibilità
bool monitorSyncEnabled = false;      // Disabilitato: non più usato con sensore gas

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
Button btnRelay = {216, 190, 100, 40, "GAS", COLOR_MAGENTA, false};
Button btnBack = {110, 190, 100, 40, "INDIETRO", COLOR_GRAY, false};

// Forward declarations
void drawButton(Button &btn, bool highlight = false);
void resetDisplayAfterAlarm();

// ========== GESTIONE DISPOSITIVI ESTERNI ==========
#define MAX_CLIENTS 10
#define CLIENT_HTTP_TIMEOUT 800       // Timeout HTTP per comandi ON/OFF (ms)
#define CLIENT_CONNECT_TIMEOUT 500    // Timeout connessione per comandi ON/OFF (ms)

struct ClientDevice {
  String ip;
  String name;
  String mdnsHost;        // Hostname mDNS (es. "oraquadra" -> oraquadra.local) per risolvere IP dinamici
  uint16_t port;
  bool active;
  bool notifyPresence;
  bool notifyBrightness;
  bool controlOnOff;      // Abilita controllo ON/OFF tramite radar
  bool currentState;      // Stato attuale (true=ON, false=OFF)
  unsigned long lastSeen;
  uint8_t failCount;      // Contatore fallimenti (per statistiche)
  bool radarVerified;     // true = risponde a /radar/ping (servizio radar installato)
};
ClientDevice clients[MAX_CLIENTS];
int clientCount = 0;

// ========== CODA VERIFICA DISCOVERY ==========
#define VERIFY_QUEUE_SIZE 32
struct VerifyCandidate {
  String ip;
  uint16_t port;
  bool pending;
};
VerifyCandidate verifyQueue[VERIFY_QUEUE_SIZE];
int verifyCount = 0;
bool verifyInProgress = false;
unsigned long lastVerifyStep = 0;
bool lastNotifiedPresence = false;
int lastSentLight = -1;

// ========== DISCOVERY DISPOSITIVI ==========
#define DISCOVERY_TIMEOUT_MS 150     // Timeout per ogni IP+porta
#define DISCOVERY_INTERVAL_MS 10     // Intervallo tra scansioni IP
const uint16_t DISCOVERY_PORTS[] = {80, 8080, 8081};  // Porte da scansionare
#define DISCOVERY_PORT_COUNT 3       // Numero porte
bool discoveryActive = false;        // Discovery in corso
bool discoveryEnabled = true;        // Auto-discovery ABILITATO di default
int discoveryCurrentIP = 1;          // IP corrente nella scansione (1-254)
int discoveryEndIP = 254;            // Ultimo IP da scansionare
int discoveryFoundCount = 0;         // Dispositivi trovati nell'ultima scansione
int discoveryPortIndex = 0;          // Indice porta corrente nella scansione
unsigned long lastDiscoveryStep = 0; // Timing per step non bloccante
unsigned long lastDiscoveryRun = 0;  // Ultima scansione completa
#define DISCOVERY_AUTO_INTERVAL 300000 // Auto-discovery ogni 5 minuti (ms)

// ========== FUNZIONI LED (DISABILITATO) ==========
void setupLED() {
  // LED PWM disabilitato - GPIO25 libero (non usato)
  // ledcSetup(LED_CHANNEL, LED_FREQ, LED_RESOLUTION);
  // ledcAttachPin(LED_PIN, LED_CHANNEL);
  // ledcWrite(LED_CHANNEL, 0);
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
  relayDeviceOn = false;  // Al boot assumiamo TV spenta
  DEBUG_PRINTLN("Rele inizializzato su GPIO26 (TV=OFF stimato)");
}

// Impulso grezzo - usa setRelayDevice() per logica smart
bool startRelayPulse() {
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
}

// Logica smart: invia impulso SOLO se lo stato desiderato e' diverso da quello stimato
bool setRelayDevice(bool wantOn) {
  if (wantOn == relayDeviceOn) {
    DEBUG_PRINTF(">>> RELE: TV gia %s, nessun impulso <<<\n", wantOn ? "ACCESA" : "SPENTA");
    return false;
  }
  bool ok = startRelayPulse();
  if (ok) {
    relayDeviceOn = wantOn;
    DEBUG_PRINTF(">>> RELE: TV -> %s <<<\n", wantOn ? "ACCESA" : "SPENTA");
  }
  return ok;
}

void updateRelay() {
  if (relayActive && (millis() - relayStartTime >= RELAY_PULSE_MS)) {
    digitalWrite(RELAY_PIN, LOW);
    relayActive = false;
    DEBUG_PRINTLN(">>> RELE: Impulso TERMINATO <<<");
    // Ripristina colore pulsante e LED
    if (currentScreen == 0) {
      btnRelay.color = gasAlarmActive ? COLOR_RED : COLOR_MAGENTA;
      drawButton(btnRelay);
    }
    restorePreviousLedColor();
  }
}

// ========== FUNZIONI SENSORE MONITOR TEMT6000 (DISATTIVATO) ==========
/*
void setupMonitorSensor_OLD() {
  analogSetPinAttenuation(MONITOR_SENSOR_PIN, ADC_0db);
  analogReadResolution(12);
  for (int i = 0; i < 10; i++) {
    analogRead(MONITOR_SENSOR_PIN);
    delay(10);
  }
  monitorLightLevel = readMonitorSensor();
  monitorDetectedOn = (monitorLightLevel > MONITOR_THRESHOLD_ON);
  monitorStateDesired = monitorDetectedOn;
}
*/

// ========== FUNZIONI SENSORE GAS MICS-5524 (LETTURA DIRETTA ADC) ==========
void setupMonitorSensor() {
  DEBUG_PRINTLN("Inizializzazione sensore gas MICS-5524 (ADC diretto)...");

  // Pin EN saldato a GND sul modulo = sensore sempre attivo
  DEBUG_PRINTLN("Sensore MICS-5524 sempre attivo (EN saldato a GND)");

  // Configura pin ADC
  pinMode(MICS_ADC_PIN, INPUT);

  // Configura ADC ESP32
  analogReadResolution(12);  // 12 bit = 0-4095
  analogSetPinAttenuation(MICS_ADC_PIN, ADC_11db);  // Range 0-3.3V

  delay(500);

  // Test lettura ADC diretta - 10 letture per diagnostica
  DEBUG_PRINTLN("=== TEST ADC MICS-5524 ===");
  int minVal = 4095, maxVal = 0;
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    int raw = analogRead(MICS_ADC_PIN);
    sum += raw;
    if (raw < minVal) minVal = raw;
    if (raw > maxVal) maxVal = raw;
    DEBUG_PRINTF("  Lettura %d: %d\n", i+1, raw);
    delay(100);
  }
  int avgVal = sum / 10;
  float voltage = (avgVal * 3.3) / 4095.0;
  DEBUG_PRINTF("GPIO%d - Min:%d Max:%d Media:%d Volt:%.2fV\n", MICS_ADC_PIN, minVal, maxVal, avgVal, voltage);

  if (avgVal == 0) {
    DEBUG_PRINTLN("!!! ATTENZIONE: ADC=0 !!!");
    DEBUG_PRINTLN("Verificare:");
    DEBUG_PRINTLN("  1. AOUT del sensore collegato a GPIO34");
    DEBUG_PRINTLN("  2. VCC del sensore collegato a 3.3V");
    DEBUG_PRINTLN("  3. GND del sensore collegato a GND");
    DEBUG_PRINTLN("  4. Sensore non guasto");
  } else if (avgVal > 4000) {
    DEBUG_PRINTLN("!!! ATTENZIONE: ADC saturato (>4000) !!!");
  }
  DEBUG_PRINTLN("=========================");

  gasSensorConnected = true;

  // Controlla motivo del reset per decidere se serve warmup
  esp_reset_reason_t resetReason = esp_reset_reason();

  if (resetReason == ESP_RST_POWERON || resetReason == ESP_RST_BROWNOUT) {
    // Accensione da spento o brownout = sensore freddo, serve warmup
    gasSensorReady = false;
    gasSensorWarmupStart = millis();
    DEBUG_PRINTF("Reset da POWER-ON/BROWNOUT - Warmup necessario: %d minuti\n", MICS_CALIBRATION_TIME);
  } else {
    // Reset software (OTA, crash, watchdog, etc.) = sensore già caldo
    gasSensorReady = true;
    DEBUG_PRINT("Reset da ");
    switch(resetReason) {
      case ESP_RST_SW:       DEBUG_PRINT("SOFTWARE"); break;
      case ESP_RST_PANIC:    DEBUG_PRINT("PANIC/CRASH"); break;
      case ESP_RST_INT_WDT:  DEBUG_PRINT("INT_WATCHDOG"); break;
      case ESP_RST_TASK_WDT: DEBUG_PRINT("TASK_WATCHDOG"); break;
      case ESP_RST_WDT:      DEBUG_PRINT("WATCHDOG"); break;
      case ESP_RST_DEEPSLEEP: DEBUG_PRINT("DEEP_SLEEP"); break;
      case ESP_RST_SDIO:     DEBUG_PRINT("SDIO"); break;
      default:               DEBUG_PRINT("UNKNOWN"); break;
    }
    DEBUG_PRINTLN(" - Sensore già caldo, NO warmup");
  }
}

// Legge il sensore gas MICS-5524 direttamente da ADC
void readGasSensor() {
  if (!gasSensorConnected || !gasSensorReady) return;

  // Leggi ADC raw (media di 10 letture)
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(MICS_ADC_PIN);
    delayMicroseconds(100);
  }
  int rawAdc = sum / 10;
  float voltage = (rawAdc * 3.3) / 4095.0;

  // MICS-5524: tensione bassa = aria pulita, tensione alta = gas presente
  // Calcolo PPM basato sulla tensione di uscita
  // Il sensore risponde a: CO, CH4, C2H5OH, H2, NH3

  // Sempre calcola i valori (anche con ADC basso = aria pulita = ~0 PPM)
  float ratio = voltage / 3.3;  // 0-1

  // Formule approssimative per MICS-5524
  // Tensione bassa (~0V) = aria pulita = 0 PPM
  // Tensione alta (~3V) = alta concentrazione gas
  gasConcentrationCO = ratio * 1000.0;      // 0-1000 ppm (range sensore)
  gasConcentrationCH4 = ratio * 10000.0;    // 0-10000 ppm
  gasConcentrationC2H5OH = ratio * 500.0;   // 0-500 ppm
  gasConcentrationH2 = ratio * 1000.0;      // 0-1000 ppm
  gasConcentrationNH3 = ratio * 500.0;      // 0-500 ppm
  gasConcentrationNO2 = 0;  // Non supportato da MICS-5524

  // Controlla allarme gas
  checkGasAlarm();
}

// Controlla se un gas supera la soglia di allarme
void checkGasAlarm() {
  // NON controllare allarmi se sensore non pronto o non connesso
  if (!gasSensorConnected || !gasSensorReady) {
    return;
  }

  bool alarmDetected = false;
  String detectedGas = "";
  float detectedValue = 0;

  // Funzione helper per verificare valore valido (no NaN, no negativi, no zero)
  auto isValidReading = [](float val) {
    return !isnan(val) && val > 0.1;  // Deve essere > 0.1 per essere considerato valido
  };

  // Controlla ogni gas contro la sua soglia (solo se valore valido)
  if (isValidReading(gasConcentrationCO) && gasConcentrationCO > GAS_THRESHOLD_CO) {
    alarmDetected = true;
    detectedGas = "CO";
    detectedValue = gasConcentrationCO;
  } else if (isValidReading(gasConcentrationCH4) && gasConcentrationCH4 > GAS_THRESHOLD_CH4) {
    alarmDetected = true;
    detectedGas = "CH4";
    detectedValue = gasConcentrationCH4;
  } else if (isValidReading(gasConcentrationC2H5OH) && gasConcentrationC2H5OH > GAS_THRESHOLD_C2H5OH) {
    alarmDetected = true;
    detectedGas = "C2H5OH";
    detectedValue = gasConcentrationC2H5OH;
  } else if (isValidReading(gasConcentrationH2) && gasConcentrationH2 > GAS_THRESHOLD_H2) {
    alarmDetected = true;
    detectedGas = "H2";
    detectedValue = gasConcentrationH2;
  } else if (isValidReading(gasConcentrationNH3) && gasConcentrationNH3 > GAS_THRESHOLD_NH3) {
    alarmDetected = true;
    detectedGas = "NH3";
    detectedValue = gasConcentrationNH3;
  }
  // NO2 disabilitato - MICS-5524 non supporta questo gas

  // Aggiorna stato allarme
  if (alarmDetected) {
    gasAlarmActive = true;
    gasAlarmType = detectedGas;
    gasAlarmValue = detectedValue;
    gasAlarmAcknowledged = false;

    // Invia allarme ai dispositivi (con rate limiting)
    unsigned long now = millis();
    if (now - lastGasAlarmSent >= GAS_ALARM_INTERVAL_MS) {
      sendGasAlarm();
      lastGasAlarmSent = now;
    }
  } else {
    // Nessun gas in allarme
    if (gasAlarmActive) {
      DEBUG_PRINTLN(">>> ALLARME GAS TERMINATO (livelli normali) <<<");
      // Notifica fine allarme
      notifyGasAlarmEnd();
      // Reset allarme
      gasAlarmActive = false;
      gasAlarmType = "";
      gasAlarmValue = 0;
      ledReady();
      // Reset display
      resetDisplayAfterAlarm();
    }
  }
}

// Invia allarme gas a tutti i dispositivi connessi
void sendGasAlarm() {
  DEBUG_PRINTF(">>> !!! ALLARME GAS: %s = %.1f PPM !!! <<<\n", gasAlarmType.c_str(), gasAlarmValue);

  // LED rosso lampeggiante
  setStatusLed(LED_COLOR_ERROR);

  // Invia a tutti i client
  if (clientCount == 0) {
    DEBUG_PRINTLN("Nessun dispositivo per allarme");
    return;
  }

  for (int i = 0; i < clientCount; i++) {
    if (!clients[i].active) continue;

    HTTPClient http;
    String url = "http://" + clients[i].ip + ":" + String(clients[i].port) +
                 "/radar/gas_alarm?gas=" + gasAlarmType + "&name=" + getGasFullName(gasAlarmType) + "&value=" + String(gasAlarmValue, 1);

    DEBUG_PRINTF("ALLARME -> %s:%d ", clients[i].ip.c_str(), clients[i].port);

    http.begin(url);
    http.setTimeout(500);
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

// Notifica fine allarme
void notifyGasAlarmEnd() {
  for (int i = 0; i < clientCount; i++) {
    if (!clients[i].active) continue;

    HTTPClient http;
    String url = "http://" + clients[i].ip + ":" + String(clients[i].port) + "/radar/gas_alarm_end";

    http.begin(url);
    http.setTimeout(500);
    http.setConnectTimeout(300);
    http.GET();
    http.end();
  }

  // Ripristina LED
  ledReady();
}

// Reset manuale allarme (acknowledge)
void acknowledgeGasAlarm() {
  gasAlarmAcknowledged = true;
  DEBUG_PRINTLN("Allarme gas confermato dall'utente");
}

// Aggiorna le letture del sensore gas periodicamente
void updateMonitorSensor() {
  // Se sensore non connesso, esci subito
  if (!gasSensorConnected) return;

  unsigned long now = millis();

  // Se non ancora pronto, controlla warmup (tempo manuale)
  if (!gasSensorReady) {
    unsigned long elapsed = (now - gasSensorWarmupStart) / 1000;
    unsigned long totalSec = MICS_CALIBRATION_TIME * 60;

    if (elapsed >= totalSec) {
      gasSensorReady = true;
      DEBUG_PRINTLN(">>> Sensore gas MICS-5524 PRONTO! <<<");
      readGasSensor();  // Prima lettura
    }
    return;
  }

  // Sensore pronto - leggi periodicamente
  if (now - lastGasSensorRead < GAS_SENSOR_UPDATE_MS) return;
  lastGasSensorRead = now;

  readGasSensor();

  DEBUG_PRINTF("GAS: CO=%.1f CH4=%.1f C2H5OH=%.1f H2=%.1f NH3=%.1f NO2=%.1f PPM\n",
               gasConcentrationCO, gasConcentrationCH4, gasConcentrationC2H5OH,
               gasConcentrationH2, gasConcentrationNH3, gasConcentrationNO2);
}

// Sincronizza lo stato del monitor - DISABILITATO per sensore gas
void syncMonitorState() {
  // Funzione legacy - non più usata con sensore gas
  // Il sensore gas non controlla lo stato del monitor
}

// Funzioni legacy per compatibilità
void setMonitorDesiredState(bool wantOn) {
  monitorStateDesired = wantOn;
}

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
  preferences.putBool("discAuto", discoveryEnabled);
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
  discoveryEnabled = preferences.getBool("discAuto", true);
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
    preferences.putString((prefix + "host").c_str(), clients[i].mdnsHost);
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
    clients[i].mdnsHost = preferences.getString((prefix + "host").c_str(), "");
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

// ========== RISOLUZIONE mDNS HOSTNAME -> IP ==========
// Risolve l'hostname mDNS di un client per ottenere il suo IP corrente.
// Se l'hostname e' vuoto o la risoluzione fallisce, usa l'IP salvato.
// Se l'IP e' cambiato (DHCP), aggiorna automaticamente il record.
String resolveClientIP(int index) {
  if (index < 0 || index >= clientCount) return "";

  // Se non ha hostname mDNS, usa IP statico salvato
  if (clients[index].mdnsHost.length() == 0) {
    return clients[index].ip;
  }

  // Risolvi hostname via mDNS
  IPAddress resolved = MDNS.queryHost(clients[index].mdnsHost, 1000);  // timeout 1 sec

  if (resolved == IPAddress(0, 0, 0, 0)) {
    // Risoluzione fallita - usa IP salvato come fallback
    DEBUG_PRINTF("mDNS: %s.local non risolto, uso IP salvato %s\n",
                 clients[index].mdnsHost.c_str(), clients[index].ip.c_str());
    return clients[index].ip;
  }

  String resolvedIP = resolved.toString();

  // Se IP e' cambiato, aggiorna il record!
  if (resolvedIP != clients[index].ip) {
    DEBUG_PRINTF("mDNS: %s.local IP cambiato %s -> %s (DHCP)\n",
                 clients[index].mdnsHost.c_str(), clients[index].ip.c_str(), resolvedIP.c_str());
    clients[index].ip = resolvedIP;
    clients[index].failCount = 0;  // Reset errori - nuovo IP
    saveClients();
  }

  return resolvedIP;
}

// Risolvi tutti i client con hostname mDNS (chiamare periodicamente)
void resolveAllClientsMdns() {
  for (int i = 0; i < clientCount; i++) {
    if (!clients[i].active || clients[i].mdnsHost.length() == 0) continue;
    resolveClientIP(i);  // Aggiorna IP se cambiato
  }
}

// Forward declaration (addClient definita piu avanti)
int addClient(String ip, String name, uint16_t port, bool controlOnOff, String mdnsHost);

// Scopri automaticamente dispositivi che annunciano il servizio _radardevice._tcp via mDNS
int discoverMdnsDevices() {
  DEBUG_PRINTLN("=== mDNS DISCOVERY: Cerco servizi _radardevice._tcp ===");

  int found = MDNS.queryService("radardevice", "tcp");

  if (found == 0) {
    DEBUG_PRINTLN("mDNS DISCOVERY: Nessun dispositivo trovato");
    return 0;
  }

  DEBUG_PRINTF("mDNS DISCOVERY: Trovati %d dispositivi\n", found);
  int added = 0;

  for (int i = 0; i < found; i++) {
    String ip = MDNS.IP(i).toString();
    String hostName = MDNS.hostname(i);
    uint16_t port = MDNS.port(i);

    // Leggi TXT records per nome e porta personalizzata
    String deviceName = hostName;
    for (int j = 0; j < MDNS.numTxt(i); j++) {
      if (MDNS.txtKey(i, j) == "name") deviceName = MDNS.txt(i, j);
      if (MDNS.txtKey(i, j) == "port") port = MDNS.txt(i, j).toInt();
    }

    // Non aggiungere se stesso
    if (ip == WiFi.localIP().toString()) continue;

    DEBUG_PRINTF("  [%d] %s (%s.local) -> %s:%d\n", i, deviceName.c_str(), hostName.c_str(), ip.c_str(), port);

    // Aggiungi/aggiorna il dispositivo con hostname mDNS (mDNS = radar verificato)
    int idx = addClient(ip, deviceName, port, true, hostName);
    if (idx >= 0) {
      clients[idx].radarVerified = true;
      added++;
    }
  }

  DEBUG_PRINTF("mDNS DISCOVERY: %d dispositivi aggiunti/aggiornati\n", added);
  return added;
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
int addClient(String ip, String name, uint16_t port, bool controlOnOff = true, String mdnsHost = "") {
  if (clientCount >= MAX_CLIENTS) return -1;

  // 1) Cerca per hostname mDNS (identificatore STABILE - non cambia con DHCP)
  if (mdnsHost.length() > 0) {
    for (int i = 0; i < clientCount; i++) {
      if (clients[i].mdnsHost == mdnsHost) {
        if (clients[i].ip != ip) {
          DEBUG_PRINTF("mDNS MATCH: %s.local IP cambiato %s -> %s\n", mdnsHost.c_str(), clients[i].ip.c_str(), ip.c_str());
        }
        clients[i].ip = ip;
        clients[i].name = name;
        clients[i].port = port;
        clients[i].active = true;
        clients[i].failCount = 0;
        clients[i].lastSeen = millis();
        saveClients();
        DEBUG_PRINTF("Device aggiornato (mDNS %s.local): %s (%s:%d)\n", mdnsHost.c_str(), name.c_str(), ip.c_str(), port);
        return i;
      }
    }
  }

  // 2) Cerca per IP esatto (stesso device, stesso IP)
  for (int i = 0; i < clientCount; i++) {
    if (clients[i].ip == ip) {
      clients[i].name = name;
      clients[i].port = port;
      if (mdnsHost.length() > 0) clients[i].mdnsHost = mdnsHost;  // Aggiorna hostname se fornito
      clients[i].active = true;
      clients[i].failCount = 0;
      clients[i].lastSeen = millis();
      saveClients();
      DEBUG_PRINTF("Device aggiornato (stesso IP): %s (%s:%d)\n", name.c_str(), ip.c_str(), port);
      return i;
    }
  }

  // 3) Cerca per nome+porta (stesso device, IP cambiato via DHCP)
  if (name.length() > 0 && name != "Device") {
    for (int i = 0; i < clientCount; i++) {
      if (clients[i].name == name && clients[i].port == port) {
        DEBUG_PRINTF("Device IP cambiato: %s %s -> %s\n", name.c_str(), clients[i].ip.c_str(), ip.c_str());
        clients[i].ip = ip;
        if (mdnsHost.length() > 0) clients[i].mdnsHost = mdnsHost;
        clients[i].active = true;
        clients[i].failCount = 0;
        clients[i].lastSeen = millis();
        saveClients();
        return i;
      }
    }
  }

  // 4) Nuovo device - aggiungi
  clients[clientCount].ip = ip;
  clients[clientCount].name = name;
  clients[clientCount].mdnsHost = mdnsHost;
  clients[clientCount].port = port;
  clients[clientCount].active = true;
  clients[clientCount].notifyPresence = true;
  clients[clientCount].notifyBrightness = true;
  clients[clientCount].controlOnOff = controlOnOff;
  clients[clientCount].currentState = false;
  clients[clientCount].lastSeen = millis();
  clients[clientCount].failCount = 0;
  clients[clientCount].radarVerified = false;  // Verra verificato con /radar/ping
  clientCount++;
  saveClients();
  DEBUG_PRINTF("Nuovo device: %s (%s:%d) host=%s\n", name.c_str(), ip.c_str(), port, mdnsHost.length() > 0 ? mdnsHost.c_str() : "none");
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

#define CLIENT_MAX_CONSECUTIVE_FAILS 10  // Dopo 10 fallimenti consecutivi, sospendi il device

void notifyClients(String event, String value) {
  if (clientCount == 0) return;

  for (int i = 0; i < clientCount; i++) {
    if (!clients[i].active) continue;
    if (event == "presence" && !clients[i].notifyPresence) continue;
    if (event == "brightness" && !clients[i].notifyBrightness) continue;

    // SKIP device irraggiungibili per evitare blocco (troppi fail consecutivi)
    if (clients[i].failCount >= CLIENT_MAX_CONSECUTIVE_FAILS) {
      // Ogni 30 tentativi riprova per vedere se è tornato online
      static uint8_t retryCounter[MAX_CLIENTS] = {0};
      retryCounter[i]++;
      if (retryCounter[i] < 30) continue;  // Salta - device probabilmente offline
      retryCounter[i] = 0;  // Reset - riprova questo giro
      // Se ha hostname mDNS, prova a risolvere nuovo IP (forse DHCP ha cambiato)
      if (clients[i].mdnsHost.length() > 0) {
        resolveClientIP(i);  // Aggiorna IP se cambiato
      }
      DEBUG_PRINTF("RETRY -> %s:%d (era offline, riprovo)\n", clients[i].ip.c_str(), clients[i].port);
    }

    HTTPClient http;
    // Usa sempre l'IP salvato (viene aggiornato periodicamente da resolveAllClientsMdns)
    String url = "http://" + clients[i].ip + ":" + String(clients[i].port) + "/radar/" + event + "?value=" + value;

    DEBUG_PRINTF("NOTIFY -> %s:%d /%s=%s ", clients[i].ip.c_str(), clients[i].port, event.c_str(), value.c_str());

    http.begin(url);
    http.setTimeout(500);  // Timeout breve
    http.setConnectTimeout(300);
    int httpCode = http.GET();

    if (httpCode >= 200 && httpCode < 400) {
      clients[i].lastSeen = millis();
      clients[i].failCount = 0;  // Reset fail count on success
      DEBUG_PRINTF("OK(%d)\n", httpCode);
    } else {
      clients[i].failCount++;
      DEBUG_PRINTF("FAIL(%d) [%d/%d]\n", httpCode, clients[i].failCount, CLIENT_MAX_CONSECUTIVE_FAILS);
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

  if (httpCode >= 200 && httpCode < 400) {
    clients[index].lastSeen = millis();
    clients[index].currentState = turnOn;
    clients[index].failCount = 0;
    clients[index].radarVerified = true;  // Risponde a /radar/* -> verificato
    DEBUG_PRINTF("OK(%d)\n", httpCode);
    return true;
  } else {
    // Fallito - se ha hostname mDNS, prova a risolvere nuovo IP e ritenta
    if (clients[index].mdnsHost.length() > 0) {
      String oldIP = clients[index].ip;
      resolveClientIP(index);
      if (clients[index].ip != oldIP) {
        // IP cambiato! Riprova con il nuovo IP
        DEBUG_PRINTF("mDNS RETRY -> %s:%d (nuovo IP da %s)\n", clients[index].ip.c_str(), clients[index].port, oldIP.c_str());
        String url2 = "http://" + clients[index].ip + ":" + String(clients[index].port) + "/radar/" + cmd;
        HTTPClient http2;
        http2.begin(url2);
        http2.setTimeout(CLIENT_HTTP_TIMEOUT);
        http2.setConnectTimeout(CLIENT_CONNECT_TIMEOUT);
        int httpCode2 = http2.GET();
        http2.end();
        if (httpCode2 >= 200 && httpCode2 < 400) {
          clients[index].lastSeen = millis();
          clients[index].currentState = turnOn;
          clients[index].failCount = 0;
          DEBUG_PRINTF("OK(%d) con nuovo IP!\n", httpCode2);
          return true;
        }
      }
    }
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

    // Device noti come offline: riprova ogni 5 cicli (non ogni volta per evitare blocco)
    if (clients[i].failCount >= CLIENT_MAX_CONSECUTIVE_FAILS) {
      static uint8_t cmdRetry[MAX_CLIENTS] = {0};
      cmdRetry[i]++;
      if (cmdRetry[i] < 5) continue;  // Salta - riprova tra 5 comandi
      cmdRetry[i] = 0;
      DEBUG_PRINTF("RETRY CMD -> %s:%d (era offline, riprovo)\n", clients[i].ip.c_str(), clients[i].port);
    }

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
// Discovery usa WiFiClient locale in discoveryStep()

void startDiscovery() {
  if (discoveryActive) {
    DEBUG_PRINTLN("Discovery gia in corso!");
    return;
  }

  discoveryActive = true;
  discoveryCurrentIP = 1;
  discoveryPortIndex = 0;
  discoveryFoundCount = 0;
  lastDiscoveryStep = millis();

  DEBUG_PRINTLN("=== AVVIO DISCOVERY DISPOSITIVI ===");
  DEBUG_PRINTF("Scansione rete: %d.%d.%d.1 - %d.%d.%d.%d (porte: 80,8080,8081)\n",
    WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2],
    WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], discoveryEndIP);
}

void stopDiscovery() {
  discoveryActive = false;
  DEBUG_PRINTF("Discovery terminato - Trovati %d dispositivi\n", discoveryFoundCount);
  lastDiscoveryRun = millis();
}

void addDiscoveredDevice(String ip, uint16_t port) {
  // Controlla se esiste gia come client
  for (int i = 0; i < clientCount; i++) {
    if (clients[i].ip == ip && clients[i].port == port) {
      clients[i].active = true;
      clients[i].lastSeen = millis();
      return;
    }
  }

  // Controlla se e' gia in coda di verifica
  for (int i = 0; i < verifyCount; i++) {
    if (verifyQueue[i].ip == ip && verifyQueue[i].port == port) return;
  }

  // Aggiungi alla coda di verifica (verra controllato /radar/ping nel loop)
  if (verifyCount < VERIFY_QUEUE_SIZE) {
    verifyQueue[verifyCount].ip = ip;
    verifyQueue[verifyCount].port = port;
    verifyQueue[verifyCount].pending = true;
    verifyCount++;
    discoveryFoundCount++;
    DEBUG_PRINTF("DISCOVERY: %s:%d -> coda verifica radar/ping\n", ip.c_str(), port);
  }
}

// Chiamare nel loop - un tentativo di connessione per ciclo
// Connect bloccante con timeout breve (150ms max per tentativo)
// Per IP senza server il connect fallisce in <10ms (no blocco reale)
void discoveryStep() {
  if (!discoveryActive || otaInProgress) return;

  unsigned long now = millis();

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
  String testIP = String(WiFi.localIP()[0]) + "." +
                  String(WiFi.localIP()[1]) + "." +
                  String(WiFi.localIP()[2]) + "." +
                  String(discoveryCurrentIP);

  // Porta corrente da testare
  uint16_t testPort = DISCOVERY_PORTS[discoveryPortIndex];

  // Tenta connessione bloccante con timeout breve
  WiFiClient testClient;
  if (testClient.connect(testIP.c_str(), testPort, DISCOVERY_TIMEOUT_MS)) {
    // Dispositivo trovato!
    testClient.stop();
    addDiscoveredDevice(testIP, testPort);
    DEBUG_PRINTF("DISCOVERY HIT: %s:%d\n", testIP.c_str(), testPort);
  }
  testClient.stop();

  // Prossima porta, poi prossimo IP
  discoveryPortIndex++;
  if (discoveryPortIndex >= DISCOVERY_PORT_COUNT) {
    discoveryPortIndex = 0;
    discoveryCurrentIP++;
  }
}

// ========== VERIFICA CANDIDATI DISCOVERY ==========
// Verifica un candidato alla volta con HTTP GET /radar/ping
void verifyStep() {
  if (verifyCount == 0 || otaInProgress) return;

  unsigned long now = millis();
  if (now - lastVerifyStep < 500) return;  // Max 2 verifiche al secondo
  lastVerifyStep = now;

  // Trova il prossimo candidato pendente
  int idx = -1;
  for (int i = 0; i < verifyCount; i++) {
    if (verifyQueue[i].pending) { idx = i; break; }
  }
  if (idx < 0) {
    // Tutti verificati, svuota la coda
    verifyCount = 0;
    return;
  }

  String ip = verifyQueue[idx].ip;
  uint16_t port = verifyQueue[idx].port;
  verifyQueue[idx].pending = false;

  DEBUG_PRINTF("VERIFY: Controllo %s:%d /radar/ping...\n", ip.c_str(), port);

  HTTPClient http;
  http.setTimeout(800);
  String url = "http://" + ip + ":" + String(port) + "/radar/ping";
  http.begin(url);
  int httpCode = http.GET();
  String payload = "";
  if (httpCode == 200) {
    payload = http.getString();
  }
  http.end();

  bool isRadar = false;
  String deviceName = "Device_" + ip.substring(ip.lastIndexOf('.') + 1);

  if (httpCode == 200 && payload.indexOf("ok") >= 0) {
    isRadar = true;
    // Estrai nome dal JSON se presente (campo "host" o "name")
    int hostIdx = payload.indexOf("\"host\":\"");
    if (hostIdx >= 0) {
      int start = hostIdx + 8;
      int end = payload.indexOf("\"", start);
      if (end > start) deviceName = payload.substring(start, end);
    }
    DEBUG_PRINTF("VERIFY: %s:%d -> RADAR OK (%s)\n", ip.c_str(), port, deviceName.c_str());
  } else {
    DEBUG_PRINTF("VERIFY: %s:%d -> NO radar (HTTP %d)\n", ip.c_str(), port, httpCode);
  }

  // Aggiungi SOLO dispositivi con radar verificato (non aggiungere router, stampanti, etc.)
  if (isRadar && clientCount < MAX_CLIENTS) {
    int addedIdx = addClient(ip, deviceName, port, true);
    if (addedIdx >= 0) {
      clients[addedIdx].radarVerified = true;
    }
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
      // Accendi TV/monitor via rele (solo se spenta)
      setRelayDevice(true);
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
      // Spegni TV/monitor via rele (solo se accesa)
      setRelayDevice(false);
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
    btnRelay.color = gasAlarmActive ? COLOR_RED : COLOR_MAGENTA;
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

  // Livello CO sensore gas (affiancato)
  char monLevelStr[8];
  sprintf(monLevelStr, "%.0f", gasConcentrationCO);
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

// ========== SCHERMATA MONITOR GAS ==========
void drawGasScreen() {
  static float prevCO = -1, prevCH4 = -1, prevC2H5OH = -1;
  static float prevH2 = -1, prevNH3 = -1, prevNO2 = -1;
  static bool prevReady = false;
  static bool prevConnected = false;

  // Coordinate layout
  const int COL1_LABEL = 12;      // Etichette colonna 1
  const int COL1_VALUE = 75;      // Valori colonna 1
  const int COL2_LABEL = 168;     // Etichette colonna 2
  const int COL2_VALUE = 230;     // Valori colonna 2
  const int ROW1 = 48;            // Prima riga
  const int ROW2 = 93;            // Seconda riga
  const int ROW3 = 138;           // Terza riga
  const int ROW_HEIGHT = 40;      // Altezza riga

  if (needsFullRedraw) {
    tft.fillScreen(COLOR_BG);

    // Header
    tft.fillRect(0, 0, 320, 32, tft.color565(40, 0, 40));
    tft.setTextColor(COLOR_MAGENTA);
    tft.setTextSize(2);
    tft.setCursor(50, 8);
    tft.print("SENSORE GAS MICS");

    // Card principale
    tft.fillRoundRect(4, 38, 312, 155, 8, COLOR_BG_CARD);
    tft.drawRoundRect(4, 38, 312, 155, 8, COLOR_MAGENTA);

    // Linee divisorie orizzontali
    tft.drawFastHLine(10, ROW2 - 5, 300, tft.color565(50, 50, 50));
    tft.drawFastHLine(10, ROW3 - 5, 300, tft.color565(50, 50, 50));
    // Linea divisoria verticale
    tft.drawFastVLine(160, ROW1, 135, tft.color565(50, 50, 50));

    tft.setTextSize(1);

    // === COLONNA SINISTRA ===
    // Riga 1: Monossido di Carbonio
    tft.setTextColor(COLOR_RED);
    tft.setCursor(COL1_LABEL, ROW1 + 5);
    tft.print("MONOSSIDO");
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(COL1_LABEL, ROW1 + 20);
    tft.print("(CO)");

    // Riga 2: CH4
    tft.setTextColor(COLOR_ORANGE);
    tft.setCursor(COL1_LABEL, ROW2 + 5);
    tft.print("CH4");
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(COL1_LABEL, ROW2 + 20);
    tft.print("Metano");

    // Riga 3: C2H5OH
    tft.setTextColor(COLOR_YELLOW);
    tft.setCursor(COL1_LABEL, ROW3 + 5);
    tft.print("C2H5OH");
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(COL1_LABEL, ROW3 + 20);
    tft.print("Etanolo");

    // === COLONNA DESTRA ===
    // Riga 1: H2
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(COL2_LABEL, ROW1 + 5);
    tft.print("H2");
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(COL2_LABEL, ROW1 + 20);
    tft.print("Idrogeno");

    // Riga 2: NH3
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(COL2_LABEL, ROW2 + 5);
    tft.print("NH3");
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(COL2_LABEL, ROW2 + 20);
    tft.print("Ammoniaca");

    // Riga 3: NO2 (disabilitato)
    tft.setTextColor(COLOR_GRAY);
    tft.setCursor(COL2_LABEL, ROW3 + 5);
    tft.print("NO2");
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(COL2_LABEL, ROW3 + 20);
    tft.print("N/D");

    // Barra stato in basso
    tft.fillRect(4, 195, 312, 18, tft.color565(30, 30, 30));
    tft.setTextColor(COLOR_TEXT_DIM);
    tft.setCursor(10, 200);
    tft.print("Stato:");

    // Pulsante INDIETRO
    btnBack.x = 4;
    btnBack.y = 215;
    btnBack.w = 100;
    btnBack.h = 28;
    btnBack.label = "INDIETRO";
    btnBack.color = COLOR_GRAY;
    drawButton(btnBack);

    // Pulsante RESET ALARM
    tft.fillRoundRect(110, 215, 96, 28, 6, COLOR_RED);
    tft.drawRoundRect(110, 215, 96, 28, 6, COLOR_TEXT);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(118, 224);
    tft.print("RESET ALARM");

    // Pulsante TEST ALARM
    tft.fillRoundRect(212, 215, 104, 28, 6, COLOR_ORANGE);
    tft.drawRoundRect(212, 215, 104, 28, 6, COLOR_TEXT);
    tft.setTextColor(COLOR_BG);
    tft.setCursor(225, 224);
    tft.print("TEST ALARM");

    needsFullRedraw = false;
    prevCO = prevCH4 = prevC2H5OH = prevH2 = prevNH3 = prevNO2 = -999;
    prevReady = !gasSensorReady;
    prevConnected = !gasSensorConnected;
  }

  // Aggiorna valori (solo se cambiati)
  tft.setTextSize(2);
  char valBuf[16];

  // CO (colonna 1, riga 1)
  if (gasConcentrationCO != prevCO) {
    tft.fillRect(COL1_VALUE, ROW1, 80, 30, COLOR_BG_CARD);
    tft.setTextColor(gasConcentrationCO > GAS_THRESHOLD_CO ? COLOR_RED : COLOR_TEXT);
    tft.setCursor(COL1_VALUE, ROW1 + 5);
    sprintf(valBuf, "%.1f", gasConcentrationCO);
    tft.print(valBuf);
    tft.setTextSize(1);
    tft.print(" ppm");
    tft.setTextSize(2);
    prevCO = gasConcentrationCO;
  }

  // CH4 (colonna 1, riga 2)
  if (gasConcentrationCH4 != prevCH4) {
    tft.fillRect(COL1_VALUE, ROW2, 80, 30, COLOR_BG_CARD);
    tft.setTextColor(gasConcentrationCH4 > GAS_THRESHOLD_CH4 ? COLOR_RED : COLOR_TEXT);
    tft.setCursor(COL1_VALUE, ROW2 + 5);
    sprintf(valBuf, "%.0f", gasConcentrationCH4);
    tft.print(valBuf);
    tft.setTextSize(1);
    tft.print(" ppm");
    tft.setTextSize(2);
    prevCH4 = gasConcentrationCH4;
  }

  // C2H5OH (colonna 1, riga 3)
  if (gasConcentrationC2H5OH != prevC2H5OH) {
    tft.fillRect(COL1_VALUE, ROW3, 80, 30, COLOR_BG_CARD);
    tft.setTextColor(gasConcentrationC2H5OH > GAS_THRESHOLD_C2H5OH ? COLOR_RED : COLOR_TEXT);
    tft.setCursor(COL1_VALUE, ROW3 + 5);
    sprintf(valBuf, "%.0f", gasConcentrationC2H5OH);
    tft.print(valBuf);
    tft.setTextSize(1);
    tft.print(" ppm");
    tft.setTextSize(2);
    prevC2H5OH = gasConcentrationC2H5OH;
  }

  // H2 (colonna 2, riga 1)
  if (gasConcentrationH2 != prevH2) {
    tft.fillRect(COL2_VALUE, ROW1, 80, 30, COLOR_BG_CARD);
    tft.setTextColor(gasConcentrationH2 > GAS_THRESHOLD_H2 ? COLOR_RED : COLOR_TEXT);
    tft.setCursor(COL2_VALUE, ROW1 + 5);
    sprintf(valBuf, "%.0f", gasConcentrationH2);
    tft.print(valBuf);
    tft.setTextSize(1);
    tft.print(" ppm");
    tft.setTextSize(2);
    prevH2 = gasConcentrationH2;
  }

  // NH3 (colonna 2, riga 2)
  if (gasConcentrationNH3 != prevNH3) {
    tft.fillRect(COL2_VALUE, ROW2, 80, 30, COLOR_BG_CARD);
    tft.setTextColor(gasConcentrationNH3 > GAS_THRESHOLD_NH3 ? COLOR_RED : COLOR_TEXT);
    tft.setCursor(COL2_VALUE, ROW2 + 5);
    sprintf(valBuf, "%.1f", gasConcentrationNH3);
    tft.print(valBuf);
    tft.setTextSize(1);
    tft.print(" ppm");
    tft.setTextSize(2);
    prevNH3 = gasConcentrationNH3;
  }

  // NO2 - Disabilitato (non supportato da MICS-5524)
  if (gasConcentrationNO2 != prevNO2) {
    tft.fillRect(COL2_VALUE, ROW3, 80, 30, COLOR_BG_CARD);
    tft.setTextColor(COLOR_GRAY);
    tft.setCursor(COL2_VALUE, ROW3 + 5);
    tft.print("--");
    tft.setTextSize(1);
    tft.print(" n/d");
    tft.setTextSize(2);
    prevNO2 = gasConcentrationNO2;
  }

  // Stato sensore + ADC raw
  static int prevRawAdc = -1;
  int currentRawAdc = analogRead(MICS_ADC_PIN);

  if (gasSensorConnected != prevConnected || gasSensorReady != prevReady || abs(currentRawAdc - prevRawAdc) > 50) {
    tft.fillRect(50, 195, 260, 18, tft.color565(30, 30, 30));
    tft.setTextSize(1);
    if (!gasSensorConnected) {
      tft.setTextColor(COLOR_RED);
      tft.setCursor(50, 200);
      tft.print("NON CONNESSO");
    } else if (!gasSensorReady) {
      tft.setTextColor(COLOR_YELLOW);
      tft.setCursor(50, 200);
      unsigned long elapsed = (millis() - gasSensorWarmupStart) / 1000;
      tft.printf("WARMUP %lu/%d sec", elapsed, MICS_CALIBRATION_TIME * 60);
    } else {
      tft.setTextColor(COLOR_GREEN);
      tft.setCursor(50, 200);
      tft.print("PRONTO");
      if (gasAlarmActive) {
        tft.setTextColor(COLOR_RED);
        tft.print(" - ALLARME!");
      }
    }
    // Mostra ADC raw e tensione per debug
    float adcVoltage = (currentRawAdc * 3.3) / 4095.0;
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(200, 200);
    tft.printf("ADC:%d (%.2fV)", currentRawAdc, adcVoltage);

    prevConnected = gasSensorConnected;
    prevReady = gasSensorReady;
    prevRawAdc = currentRawAdc;
  }
}

// ========== SCHERMATA ALLARME GAS ==========
void drawGasAlarmScreen() {
  static unsigned long lastBlink = 0;
  static bool blinkState = false;
  static String prevAlarmType = "";
  static float prevAlarmValue = -1;

  // Effetto lampeggio sfondo rosso/nero
  unsigned long now = millis();
  if (now - lastBlink >= 500) {
    blinkState = !blinkState;
    lastBlink = now;

    // Sfondo lampeggiante
    uint16_t bgColor = blinkState ? COLOR_RED : tft.color565(60, 0, 0);
    tft.fillScreen(bgColor);

    // Icona pericolo (triangolo con !)
    int cx = 160, cy = 60;
    tft.fillTriangle(cx, cy - 35, cx - 40, cy + 25, cx + 40, cy + 25, COLOR_YELLOW);
    tft.fillTriangle(cx, cy - 25, cx - 30, cy + 18, cx + 30, cy + 18, bgColor);
    tft.setTextColor(COLOR_YELLOW);
    tft.setTextSize(3);
    tft.setCursor(cx - 8, cy - 5);
    tft.print("!");

    // Titolo ALLARME
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(3);
    tft.setCursor(60, 100);
    tft.print("! ALLARME !");

    // Sottotitolo GAS
    tft.setTextSize(2);
    tft.setCursor(100, 130);
    tft.print("GAS RILEVATO");

    // Nome del gas rilevato
    tft.setTextColor(COLOR_YELLOW);
    tft.setTextSize(4);
    String gasName = getGasFullName(gasAlarmType);
    int textW = gasName.length() * 24;
    tft.setCursor((320 - textW) / 2, 160);
    tft.print(gasName);

    // Valore PPM
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(2);
    char valueStr[32];
    sprintf(valueStr, "%.1f PPM", gasAlarmValue);
    int valW = strlen(valueStr) * 12;
    tft.setCursor((320 - valW) / 2, 200);
    tft.print(valueStr);

    // Istruzioni
    tft.setTextSize(1);
    tft.setTextColor(COLOR_YELLOW);
    tft.setCursor(60, 225);
    tft.print("ARIEGGIARE - EVACUARE SE NECESSARIO");
  }

  // LED rosso lampeggiante
  setStatusLed(blinkState ? LED_COLOR_ERROR : LED_COLOR_OFF);
}

// Restituisce il nome completo del gas
String getGasFullName(String gasCode) {
  if (gasCode == "CO") return "MONOSSIDO";
  if (gasCode == "CH4") return "METANO";
  if (gasCode == "C2H5OH") return "ETANOLO";
  if (gasCode == "H2") return "IDROGENO";
  if (gasCode == "NH3") return "AMMONIACA";
  if (gasCode == "NO2") return "BIOSS.AZOTO";
  if (gasCode == "TEST") return "TEST";
  return gasCode;
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
  // Se allarme gas attivo, mostra schermata allarme
  if (gasAlarmActive && !gasAlarmAcknowledged) {
    drawGasAlarmScreen();
    return;
  }

  switch (currentScreen) {
    case 0:
      drawMainScreen();
      break;
    case 1:
      drawRadarScreen();
      break;
    case 2:
      drawGasScreen();
      break;
  }
}

// ========== RESET DISPLAY DOPO ALLARME ==========
void resetDisplayAfterAlarm() {
  DEBUG_PRINTLN("Reset display dopo allarme...");

  // Pulisci completamente lo schermo
  tft.fillScreen(COLOR_BG);

  // Reset font
  tft.setFreeFont(NULL);
  tft.setTextSize(1);

  // Forza ridisegno completo
  needsFullRedraw = true;
  headerDrawn = false;

  // Ridisegna SUBITO la schermata corrente
  switch (currentScreen) {
    case 0:
      drawMainScreen();
      break;
    case 1:
      drawRadarScreen();
      break;
    case 2:
      drawGasScreen();
      break;
  }

  DEBUG_PRINTLN("Display resettato.");
}

// ========== GESTIONE TOUCH UI ==========
void handleTouch() {
  static unsigned long lastTouchTime = 0;

  if (readTouch()) {
    if (millis() - lastTouchTime < 300) return;
    lastTouchTime = millis();

    DEBUG_PRINTF("Touch: X=%d, Y=%d\n", touchX, touchY);

    // Se allarme gas attivo, qualsiasi tocco RESETTA l'allarme
    if (gasAlarmActive) {
      DEBUG_PRINTLN(">>> ALLARME GAS RESETTATO DA TOUCH <<<");
      // Notifica fine allarme a tutti i device
      notifyGasAlarmEnd();
      // Reset completo allarme
      gasAlarmActive = false;
      gasAlarmType = "";
      gasAlarmValue = 0;
      gasAlarmAcknowledged = false;
      ledReady();
      // Reset completo display e ridisegna subito
      resetDisplayAfterAlarm();
      return;
    }

    switch (currentScreen) {
      case 0:
        if (isTouchInButton(btnOnOff)) {
          if (manualAllOn) {
            DEBUG_PRINTLN("Pulsante ACCENDI -> ACCENDI TUTTI");
            sendAllOn();
            setRelayDevice(true);
            manualAllOn = false;  // Prossimo tocco = SPEGNI
            btnOnOff.label = "SPEGNI";
            btnOnOff.color = COLOR_RED;
          } else {
            DEBUG_PRINTLN("Pulsante SPEGNI -> SPEGNI TUTTI");
            sendAllOff();
            setRelayDevice(false);
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
          DEBUG_PRINTLN("Pulsante GAS premuto");
          currentScreen = 2;
          needsFullRedraw = true;
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
        // Tocco in basso a destra -> vai a schermata GAS
        else if (touchX > 220 && touchY > 200) {
          DEBUG_PRINTLN("Vai a schermata GAS");
          currentScreen = 2;
          needsFullRedraw = true;
        }
        break;

      case 2:  // Schermata GAS
        // Pulsante INDIETRO (4, 205, 100, 32)
        if (touchX >= 4 && touchX <= 104 && touchY >= 205 && touchY <= 237) {
          DEBUG_PRINTLN("GAS: Pulsante INDIETRO");
          // Reset font prima di tornare alla home
          tft.setFreeFont(NULL);
          tft.setTextSize(1);
          headerDrawn = false;
          currentScreen = 0;
          needsFullRedraw = true;
        }
        // Pulsante RESET ALARM (112, 205, 100, 32)
        else if (touchX >= 112 && touchX <= 212 && touchY >= 205 && touchY <= 237) {
          DEBUG_PRINTLN("GAS: Reset Allarme");
          // Notifica fine allarme a tutti i device
          notifyGasAlarmEnd();
          // Reset allarme
          gasAlarmActive = false;
          gasAlarmType = "";
          gasAlarmValue = 0;
          gasAlarmAcknowledged = false;
          ledReady();
          // Reset display
          resetDisplayAfterAlarm();
        }
        // Pulsante TEST ALARM (216, 205, 100, 32)
        else if (touchX >= 216 && touchX <= 316 && touchY >= 205 && touchY <= 237) {
          DEBUG_PRINTLN("GAS: Test Allarme");
          gasAlarmActive = true;
          gasAlarmType = "TEST";
          gasAlarmValue = 999.9;
          gasAlarmAcknowledged = false;
          sendGasAlarm();
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
  // NOTA: HTML in flash (PROGMEM) per evitare allocazione heap ~40KB
  static const char pageHTML[] PROGMEM = R"rawliteral(
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
    <div class="tab" onclick="showTab(4)">WiFi</div>
    <div class="tab" onclick="showTab(5)">OTA</div>
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
      <button class="success" onclick="startDiscovery()">Scan IP</button>
      <button class="success" onclick="mdnsDiscover()" style="background:#4CAF50">mDNS</button>
      <button class="danger" onclick="stopDiscovery()">Stop</button>
      <button onclick="toggleDiscovery()">Auto:<span id="discAuto">--</span></button>
    </div>
    <div class="card" style="border-left:3px solid #4CAF50">
      <h2>Auto-Install Radar</h2>
      <div style="display:flex;gap:4px;margin:6px 0">
        <button class="success" onclick="runAutoInstall()" style="flex:1;font-size:1.1em;padding:10px">AUTO INSTALLA TUTTI</button>
      </div>
      <div id="installStatus" style="display:none;background:#111;padding:8px;border-radius:4px;margin:6px 0;font-size:0.85em"></div>
      <details style="margin-top:8px">
        <summary style="color:#aaa;font-size:0.8em;cursor:pointer">Metodo manuale (curl)</summary>
        <div style="margin-top:4px">
          <div style="background:#111;padding:8px;border-radius:4px;font-family:monospace;font-size:0.8em;word-break:break-all;border:1px solid #333" onclick="copyInstall()">curl http://<span id="myIP">...</span>/api/install-script | sudo bash</div>
          <div style="display:flex;gap:4px;margin-top:4px">
            <button onclick="copyInstall()" style="flex:1">Copia Comando</button>
            <button onclick="window.open('/api/install-script')" style="flex:1">Vedi Script</button>
          </div>
        </div>
      </details>
    </div>
    <div class="card">
      <h2>Aggiungi</h2>
      <div class="row"><label>IP</label><input type="text" id="newIp" placeholder="192.168.1.x"></div>
      <div class="row"><label>Porta</label><input type="number" id="newPort" value="80"></div>
      <div class="row"><label>Nome</label><input type="text" id="newName" placeholder="Nome"></div>
      <div class="row"><label>mDNS host</label><input type="text" id="newHost" placeholder="hostname (senza .local)"></div>
      <button onclick="addDevice()">Aggiungi</button>
      <button onclick="testNotify()">Test</button>
    </div>
    <div class="card">
      <h2>Dispositivi (<span id="devCount">0</span>)</h2>
      <div id="deviceList">--</div>
    </div>
  </div>

  <!-- TAB WIFI -->
  <div class="tab-content" id="tab4">
    <div class="card">
      <h2>Connessione Attuale</h2>
      <div class="grid">
        <div class="stat"><div class="stat-value" id="wifiSSID">--</div><div class="stat-label">SSID</div></div>
        <div class="stat"><div class="stat-value" id="wifiIP">--</div><div class="stat-label">IP</div></div>
        <div class="stat"><div class="stat-value" id="wifiRSSI">--</div><div class="stat-label">RSSI</div></div>
        <div class="stat"><div class="stat-value" id="wifiMode">--</div><div class="stat-label">Modo</div></div>
      </div>
    </div>
    <div class="card">
      <h2>Configurazione IP</h2>
      <div class="row"><label>IP Address</label><input type="text" id="cfgIP" placeholder="192.168.1.99"></div>
      <div class="row"><label>Gateway</label><input type="text" id="cfgGW" placeholder="192.168.1.1"></div>
      <div class="row"><label>Subnet</label><input type="text" id="cfgSN" placeholder="255.255.255.0"></div>
      <div class="row"><label>DNS</label><input type="text" id="cfgDNS" placeholder="192.168.1.1"></div>
      <p>Compila per IP statico, lascia vuoto per DHCP</p>
      <button onclick="saveWifiConfig()">Salva IP</button>
      <div class="msg" id="msgWifi"></div>
    </div>
    <div class="card">
      <h2>Reset WiFi</h2>
      <p>Cancella credenziali e riavvia in modalita configurazione (Access Point)</p>
      <button class="danger" onclick="if(confirm('Reset WiFi e riavvio?'))location='/api/wifi/reset'">Reset WiFi</button>
    </div>
  </div>

  <!-- TAB OTA -->
  <div class="tab-content" id="tab5">
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
      let offline=c.failCount>=10;
      let noRadar=!c.radarVerified;
      let bg=noRadar?'#332200':(offline?'#411':'#222');
      let border=noRadar?'#f80':(offline?'#f44':'#0f8');
      h+='<div style="margin:8px 0;padding:8px;background:'+bg+';border-radius:6px;border-left:3px solid '+border+'">';
      h+='<div style="margin-bottom:6px"><b>'+c.name+'</b> - '+c.ip+':'+c.port;
      if(c.mdnsHost)h+=' <span style="color:#4CAF50;font-size:0.8em">('+c.mdnsHost+'.local)</span>';
      if(noRadar)h+=' <span style="color:#f80">[NO RADAR]</span>';
      else if(offline)h+=' <span class="r">[OFFLINE x'+c.failCount+']</span>';
      else if(c.failCount>0)h+=' <span class="y">[fail:'+c.failCount+']</span>';
      else h+=' <span class="g">[OK]</span>';
      h+='</div>';
      if(noRadar){
        h+='<div style="background:#111;padding:6px;border-radius:4px;margin:4px 0;font-size:0.75em">';
        h+='<div style="color:#f80;margin-bottom:4px">Radar non installato. Esegui sul device:</div>';
        h+='<code style="color:#0f0;word-break:break-all">curl http://'+location.hostname+'/api/install-script | sudo bash</code>';
        h+='</div>';
        h+='<div style="display:flex;gap:4px;flex-wrap:wrap">';
        h+='<button style="background:#f80;color:#000" onclick="copyInstall()">Copia Install</button>';
        h+='<button onclick="verifyDevice('+c.index+')">Verifica</button>';
        h+='<button onclick="removeDevice('+c.index+')">X</button>';
        h+='</div>';
      } else {
        h+='<div style="display:flex;gap:4px;align-items:center;flex-wrap:wrap">';
        h+='<button class="'+(c.controlOnOff?'toggle-on':'toggle-off')+'" onclick="toggleOnOff('+c.index+','+!c.controlOnOff+')">';
        h+='CTRL: '+(c.controlOnOff?'ON':'OFF')+'</button>';
        h+='<button class="success" onclick="sendCmd('+c.index+',\'on\')">ACCENDI</button>';
        h+='<button class="danger" onclick="sendCmd('+c.index+',\'off\')">SPEGNI</button>';
        if(offline)h+='<button style="background:#fa0;color:#000" onclick="resetDevice('+c.index+')">RESET</button>';
        h+='<button onclick="removeDevice('+c.index+')">X</button>';
        h+='</div>';
      }
      h+='</div>';
    });
    document.getElementById('deviceList').innerHTML=h;
  });
}

function resetDevice(idx){
  api('/api/devices/reset?index='+idx).then(d=>loadDevices());
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

function mdnsDiscover(){
  api('/api/devices/mdns-discover').then(d=>{
    alert('mDNS: trovati '+d.found+' dispositivi (totale: '+d.total+')');
    loadDevices();
  });
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
  let host=document.getElementById('newHost').value||'';
  if(!ip)return alert('Inserisci IP');
  let url='/api/devices/add?ip='+ip+'&name='+encodeURIComponent(name)+'&port='+port;
  if(host)url+='&host='+encodeURIComponent(host);
  api(url).then(d=>{
    document.getElementById('newIp').value='';
    document.getElementById('newName').value='';
    document.getElementById('newHost').value='';
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

// WiFi functions
function loadWifiConfig(){
  api('/api/wifi/config').then(d=>{
    document.getElementById('wifiSSID').textContent=d.ssid||'--';
    document.getElementById('wifiIP').textContent=d.ip||'--';
    document.getElementById('wifiRSSI').textContent=d.rssi+'dBm';
    document.getElementById('wifiMode').textContent=d.use_static?'Statico':'DHCP';
    document.getElementById('cfgIP').value=d.static_ip||'';
    document.getElementById('cfgGW').value=d.static_gateway||'';
    document.getElementById('cfgSN').value=d.static_subnet||'';
    document.getElementById('cfgDNS').value=d.static_dns||'';
  }).catch(e=>{console.log('WiFi config error:',e)});
}

function saveWifiConfig(){
  let ip=document.getElementById('cfgIP').value;
  let gw=document.getElementById('cfgGW').value;
  let sn=document.getElementById('cfgSN').value;
  let dns=document.getElementById('cfgDNS').value;
  let url='/api/wifi/setip?ip='+ip+'&gateway='+gw+'&subnet='+sn+'&dns='+dns;
  api(url).then(d=>{
    if(d.success){
      showMsg('msgWifi','ok','Salvato! Riavvia per applicare.');
    }else{
      showMsg('msgWifi','err','Errore salvataggio');
    }
  }).catch(e=>{showMsg('msgWifi','err','Errore: '+e)});
}

function copyInstall(){
  let cmd='curl http://'+location.hostname+'/api/install-script | sudo bash';
  navigator.clipboard.writeText(cmd).then(()=>alert('Comando copiato!')).catch(()=>{
    prompt('Copia il comando:',cmd);
  });
}
function verifyDevice(idx){
  api('/api/devices/verify?index='+idx).then(d=>{
    if(d.radarVerified)alert(d.name+' - Radar OK!');
    else alert(d.name+' - Radar non trovato. Installa il servizio.');
    loadDevices();
  });
}
function runAutoInstall(){
  let st=document.getElementById('installStatus');
  st.style.display='block';
  // Conta device senza radar
  api('/api/devices').then(d=>{
    let noRadar=d.devices.filter(c=>!c.radarVerified);
    if(noRadar.length==0){
      st.innerHTML='<span class="g">Tutti i device hanno gia il radar!</span>';
      return;
    }
    st.innerHTML='<span style="color:#f80">'+noRadar.length+' device senza radar trovati.</span><br>'+
      '<div style="margin:8px 0;padding:8px;background:#000;border-radius:4px;font-family:monospace;font-size:0.85em">'+
      '<div style="color:#aaa;margin-bottom:4px">Esegui dal PC (nella cartella del progetto):</div>'+
      '<div style="color:#0f0">python auto_install_radar.py</div>'+
      '<div style="color:#aaa;margin-top:6px">Oppure modalita watch (controlla ogni 60s):</div>'+
      '<div style="color:#0f0">python auto_install_radar.py --watch</div>'+
      '</div>'+
      '<div style="display:flex;gap:4px;margin-top:4px">'+
      '<button class="success" onclick="copyAutoCmd()">Copia Comando</button>'+
      '<button onclick="installAllVerify()">Verifica Tutti</button>'+
      '</div>';
  });
}
function copyAutoCmd(){
  let cmd='python auto_install_radar.py';
  navigator.clipboard.writeText(cmd).then(()=>alert('Copiato!')).catch(()=>prompt('Copia:',cmd));
}
function installAllVerify(){
  let st=document.getElementById('installStatus');
  st.innerHTML='<span class="y">Verifico tutti i device...</span>';
  api('/api/devices').then(d=>{
    let promises=d.devices.filter(c=>!c.radarVerified).map(c=>
      api('/api/devices/verify?index='+c.index)
    );
    Promise.all(promises).then(results=>{
      let ok=results.filter(r=>r.radarVerified).length;
      let fail=results.length-ok;
      st.innerHTML='<span class="g">Verificati: '+ok+' OK</span>'+
        (fail>0?' <span class="r">'+fail+' senza radar</span>':'');
      loadDevices();
    });
  });
}

loadEditableFields();
loadTouchStatus();
loadDiscoveryStatus();
loadWifiConfig();
updateStatus();
document.getElementById('myIP').textContent=location.hostname;
setInterval(updateStatus,3000);
setInterval(loadDiscoveryStatus,2000);
</script>
</body>
</html>
)rawliteral";

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", pageHTML);
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
    json += "\"relayDeviceOn\":" + String(relayDeviceOn ? "true" : "false") + ",";
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
    // Sensore gas DFRobot MICS
    json += "\"gasSensorConnected\":" + String(gasSensorConnected ? "true" : "false") + ",";
    json += "\"gasSensorReady\":" + String(gasSensorReady ? "true" : "false") + ",";
    json += "\"gasCO\":" + String(gasConcentrationCO, 1) + ",";
    json += "\"gasCH4\":" + String(gasConcentrationCH4, 1) + ",";
    json += "\"gasC2H5OH\":" + String(gasConcentrationC2H5OH, 1) + ",";
    json += "\"gasH2\":" + String(gasConcentrationH2, 1) + ",";
    json += "\"gasNH3\":" + String(gasConcentrationNH3, 1) + ",";
    json += "\"gasNO2\":" + String(gasConcentrationNO2, 1) + ",";
    json += "\"gasAlarm\":" + String(gasAlarmActive ? "true" : "false") + ",";
    json += "\"gasAlarmType\":\"" + gasAlarmType + "\",";
    json += "\"gasAlarmValue\":" + String(gasAlarmValue, 1);
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
  static const char otaHTML[] PROGMEM = R"rawliteral(
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

  server.on("/update", HTTP_GET, []() {
    server.send_P(200, "text/html", otaHTML);
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
        otaInProgress = true;  // Blocca discovery/verify durante web upload
        DEBUG_PRINTF("Web Update: %s\n", upload.filename.c_str());
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
      json += ",\"mdnsHost\":\"" + clients[i].mdnsHost + "\"";
      json += ",\"port\":" + String(clients[i].port);
      json += ",\"active\":" + String(clients[i].active ? "true" : "false");
      json += ",\"notifyPresence\":" + String(clients[i].notifyPresence ? "true" : "false");
      json += ",\"notifyBrightness\":" + String(clients[i].notifyBrightness ? "true" : "false");
      json += ",\"controlOnOff\":" + String(clients[i].controlOnOff ? "true" : "false");
      json += ",\"currentState\":" + String(clients[i].currentState ? "true" : "false");
      json += ",\"lastSeen\":" + String(clients[i].lastSeen);
      json += ",\"failCount\":" + String(clients[i].failCount);
      json += ",\"radarVerified\":" + String(clients[i].radarVerified ? "true" : "false");
      json += "}";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  server.on("/api/devices/add", HTTP_GET, []() {
    String ip = server.arg("ip");
    String name = server.arg("name");
    String host = server.arg("host");  // hostname mDNS (es. "oraquadra")
    int port = server.arg("port").toInt();
    if (port == 0) port = 80;

    if (ip.length() == 0) {
      server.send(400, "application/json", "{\"error\":\"IP richiesto\"}");
      return;
    }

    int idx = addClient(ip, name.length() > 0 ? name : "Device", port, true, host);
    if (idx >= 0) {
      String json = "{\"status\":\"ok\",\"index\":" + String(idx);
      if (host.length() > 0) json += ",\"mdnsHost\":\"" + host + "\"";
      json += "}";
      server.send(200, "application/json", json);
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

  // Verifica manuale /radar/ping su un dispositivo
  server.on("/api/devices/verify", HTTP_GET, []() {
    int idx = server.arg("index").toInt();
    if (idx < 0 || idx >= clientCount) {
      server.send(400, "application/json", "{\"error\":\"Indice non valido\"}");
      return;
    }
    HTTPClient http;
    http.setTimeout(1000);
    String url = "http://" + clients[idx].ip + ":" + String(clients[idx].port) + "/radar/ping";
    http.begin(url);
    int httpCode = http.GET();
    String payload = "";
    if (httpCode == 200) payload = http.getString();
    http.end();

    bool verified = (httpCode == 200 && payload.indexOf("ok") >= 0);
    clients[idx].radarVerified = verified;
    if (verified) {
      clients[idx].controlOnOff = true;
      clients[idx].failCount = 0;
      // Estrai nome se presente
      int hostIdx = payload.indexOf("\"host\":\"");
      if (hostIdx >= 0) {
        int start = hostIdx + 8;
        int end = payload.indexOf("\"", start);
        if (end > start) clients[idx].name = payload.substring(start, end);
      }
      saveClients();
    }
    String json = "{\"status\":\"ok\",\"index\":" + String(idx);
    json += ",\"radarVerified\":" + String(verified ? "true" : "false");
    json += ",\"name\":\"" + clients[idx].name + "\"}";
    server.send(200, "application/json", json);
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

  // Purge automatico: rimuovi device irraggiungibili (lastSeen==0 o failCount alto)
  server.on("/api/devices/purge", HTTP_GET, []() {
    int removed = 0;
    // Rimuovi dal fondo per non spostare gli indici
    for (int i = clientCount - 1; i >= 0; i--) {
      if (clients[i].failCount >= CLIENT_MAX_CONSECUTIVE_FAILS && clients[i].lastSeen == 0) {
        DEBUG_PRINTF("PURGE: Rimosso %s (%s:%d) - mai contattato, %d fail\n",
                     clients[i].name.c_str(), clients[i].ip.c_str(), clients[i].port, clients[i].failCount);
        removeClient(i);
        removed++;
      }
    }
    String json = "{\"status\":\"ok\",\"removed\":" + String(removed) + ",\"remaining\":" + String(clientCount) + "}";
    server.send(200, "application/json", json);
  });

  // mDNS discovery - trova dispositivi _radardevice._tcp sulla rete
  server.on("/api/devices/mdns-discover", HTTP_GET, []() {
    int found = discoverMdnsDevices();
    String json = "{\"status\":\"ok\",\"found\":" + String(found) + ",\"total\":" + String(clientCount) + "}";
    server.send(200, "application/json", json);
  });

  // mDNS resolve - aggiorna IP di tutti i client con hostname mDNS
  server.on("/api/devices/mdns-resolve", HTTP_GET, []() {
    resolveAllClientsMdns();
    String json = "{\"status\":\"ok\",\"devices\":[";
    bool first = true;
    for (int i = 0; i < clientCount; i++) {
      if (clients[i].mdnsHost.length() == 0) continue;
      if (!first) json += ",";
      json += "{\"name\":\"" + clients[i].name + "\",\"host\":\"" + clients[i].mdnsHost + "\",\"ip\":\"" + clients[i].ip + "\"}";
      first = false;
    }
    json += "]}";
    server.send(200, "application/json", json);
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
    saveSettings();
    server.send(200, "application/json", "{\"enabled\":" + String(discoveryEnabled ? "true" : "false") + "}");
  });

  // ========== API INSTALL SCRIPT ==========

  // Script auto-installazione radar per dispositivi Linux (Raspberry Pi, etc.)
  static const char installScript[] PROGMEM = R"INST(#!/bin/bash
# ============================================
# RADAR SERVICE - AUTO INSTALLER
# Esegui su un Raspberry Pi o Linux:
#   curl http://RADAR_IP/api/install-script | sudo bash
# ============================================
set -e
RADAR_PORT=${RADAR_PORT:-8081}
USER=$(logname 2>/dev/null || echo "pi")
HOME_DIR=$(eval echo ~$USER)
INSTALL_DIR="/opt/radar-service"

echo ""
echo "====================================="
echo " RADAR SERVICE - AUTO INSTALLER"
echo "====================================="
echo " Porta:  $RADAR_PORT"
echo " Utente: $USER"
echo " Dir:    $INSTALL_DIR"
echo "====================================="
echo ""

# Controlla se radar endpoint gia esiste (verifica contenuto risposta, non solo connessione)
PING_RESPONSE=$(curl -s --connect-timeout 2 http://127.0.0.1:$RADAR_PORT/radar/ping 2>/dev/null)
if echo "$PING_RESPONSE" | grep -q '"ok"'; then
  echo "[OK] Radar service gia attivo su porta $RADAR_PORT"
  echo "[INFO] Verifico solo avahi..."
else
  echo "[1/4] Creo directory..."
  mkdir -p "$INSTALL_DIR"

  echo "[2/4] Installo radar_service.py..."
  cat > "$INSTALL_DIR/radar_service.py" << 'PYEOF'
#!/usr/bin/env python3
"""Radar presence service - auto-installed by RADAR_CLIENT
Endpoints:
  /radar/on              - Accende display
  /radar/off             - Spegne display
  /radar/ping            - Health check
  /radar/presence?value= - Presenza (true=accendi, false=spegni)
  /radar/brightness?value= - Luminosita ambientale (0-4095)
  /radar/temperature?value= - Temperatura (C)
  /radar/humidity?value=    - Umidita (%)
  /radar/status          - Stato completo JSON
"""
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import subprocess, json, os, socket, time

PORT = int(os.environ.get('RADAR_PORT', '8081'))
DISPLAY = os.environ.get('DISPLAY', ':0')
XAUTH = os.environ.get('XAUTHORITY', os.path.expanduser('~/.Xauthority'))
HOST = socket.gethostname()

# Stato corrente
state = {
    'display': True,
    'presence': False,
    'brightness': 0,
    'temperature': 0.0,
    'humidity': 0.0,
    'last_update': 0,
}

def x_env():
    return {**os.environ, 'DISPLAY': DISPLAY, 'XAUTHORITY': XAUTH}

def display_on():
    try:
        subprocess.run(['xrandr', '--output', 'HDMI-1', '--auto'], env=x_env(), timeout=5)
        state['display'] = True
        return True
    except Exception as e:
        print(f"Display ON error: {e}")
        return False

def display_off():
    try:
        subprocess.run(['xrandr', '--output', 'HDMI-1', '--off'], env=x_env(), timeout=5)
        state['display'] = False
        return True
    except Exception as e:
        print(f"Display OFF error: {e}")
        return False

def set_brightness(val):
    """Regola luminosita display (0-100). Prova backlight sysfs, poi xrandr."""
    try:
        pct = max(10, min(100, int(val * 100 / 4095))) if val > 0 else 10
        bl = '/sys/class/backlight'
        if os.path.isdir(bl):
            devs = os.listdir(bl)
            if devs:
                mx = int(open(f'{bl}/{devs[0]}/max_brightness').read().strip())
                bv = max(1, int(mx * pct / 100))
                open(f'{bl}/{devs[0]}/brightness', 'w').write(str(bv))
                return True
        # Fallback: xrandr brightness (0.1-1.0)
        xb = max(0.1, pct / 100.0)
        subprocess.run(['xrandr', '--output', 'HDMI-1', '--brightness', f'{xb:.2f}'],
                       env=x_env(), timeout=5)
        return True
    except Exception as e:
        print(f"Brightness error: {e}")
        return False

class H(BaseHTTPRequestHandler):
    def do_GET(self):
        u = urlparse(self.path)
        p = u.path
        q = parse_qs(u.query)
        v = q.get('value', [''])[0]

        if p == '/radar/on':
            ok = display_on()
            self.j(200, {'status': 'on' if ok else 'error'})
        elif p == '/radar/off':
            ok = display_off()
            self.j(200, {'status': 'off' if ok else 'error'})
        elif p == '/radar/ping':
            self.j(200, {'status':'ok','type':'radar-service','host':HOST,
                         'temperature':state['temperature'],'humidity':state['humidity']})
        elif p == '/radar/presence':
            if v.lower() == 'true':
                state['presence'] = True
                display_on()
            elif v.lower() == 'false':
                state['presence'] = False
                display_off()
            state['last_update'] = time.time()
            self.j(200, {'status':'ok','presence':state['presence']})
        elif p == '/radar/brightness':
            try:
                bv = int(float(v))
                state['brightness'] = bv
                set_brightness(bv)
                state['last_update'] = time.time()
                self.j(200, {'status':'ok','brightness':bv})
            except:
                self.j(400, {'error':'valore non valido'})
        elif p == '/radar/temperature':
            try:
                state['temperature'] = round(float(v), 1)
                state['last_update'] = time.time()
                self.j(200, {'status':'ok','temperature':state['temperature']})
            except:
                self.j(400, {'error':'valore non valido'})
        elif p == '/radar/humidity':
            try:
                state['humidity'] = round(float(v), 1)
                state['last_update'] = time.time()
                self.j(200, {'status':'ok','humidity':state['humidity']})
            except:
                self.j(400, {'error':'valore non valido'})
        elif p == '/radar/status':
            self.j(200, {**state, 'host': HOST, 'uptime': time.time()})
        else:
            self.j(404, {'error': 'not found'})

    def j(self, code, data):
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def log_message(self, fmt, *a): pass

if __name__ == '__main__':
    print(f'Radar service [{HOST}] porta {PORT}')
    print(f'Endpoints: /radar/on|off|ping|presence|brightness|temperature|humidity|status')
    HTTPServer(('', PORT), H).serve_forever()
PYEOF
  chmod +x "$INSTALL_DIR/radar_service.py"

  echo "[3/4] Creo systemd service..."
  cat > /etc/systemd/system/radar-service.service << SVCEOF
[Unit]
Description=Radar Presence Service
After=network.target

[Service]
Type=simple
User=$USER
Environment=DISPLAY=:0
Environment=XAUTHORITY=$HOME_DIR/.Xauthority
Environment=RADAR_PORT=$RADAR_PORT
WorkingDirectory=$INSTALL_DIR
ExecStart=/usr/bin/python3 $INSTALL_DIR/radar_service.py
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
SVCEOF

  systemctl daemon-reload
  systemctl enable radar-service
  systemctl restart radar-service
  echo "[OK] Servizio radar avviato"
fi

echo "[4/4] Configuro mDNS (avahi)..."
mkdir -p /etc/avahi/services
HNAME=$(hostname)
cat > /etc/avahi/services/radardevice.service << AVEOF
<?xml version="1.0" standalone='no'?>
<!DOCTYPE service-group SYSTEM "avahi-service.dtd">
<service-group>
  <name>$HNAME Radar Service</name>
  <service>
    <type>_radardevice._tcp</type>
    <port>$RADAR_PORT</port>
    <txt-record>name=$HNAME</txt-record>
  </service>
</service-group>
AVEOF
systemctl restart avahi-daemon 2>/dev/null || true

sleep 2
echo ""
echo "====================================="
echo " INSTALLAZIONE COMPLETATA!"
echo "====================================="
echo " Verifica: curl http://localhost:$RADAR_PORT/radar/ping"
curl -s http://127.0.0.1:$RADAR_PORT/radar/ping 2>/dev/null && echo ""
echo " mDNS: $HNAME.local:$RADAR_PORT"
echo " Il dispositivo verra rilevato automaticamente."
echo "====================================="
)INST";

  server.on("/api/install-script", HTTP_GET, []() {
    server.send_P(200, "text/x-shellscript", installScript);
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
    setRelayDevice(false);  // Spegni TV (solo se accesa)
    needsFullRedraw = true;
    server.send(200, "application/json", "{\"status\":\"ok\",\"action\":\"test_off\"}");
  });

  server.on("/api/test/on", HTTP_GET, []() {
    // NON imposta manualPresenceOverride - il radar continua a funzionare
    notifyClients("presence", "true");
    sendAllOn();  // Accendi tutti i dispositivi LAN
    setRelayDevice(true);  // Accendi TV (solo se spenta)
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

  // Reset credenziali WiFi e avvia portale configurazione
  server.on("/api/wifi/reset", HTTP_GET, []() {
    server.send(200, "text/html", "<html><body style='background:#1a1a2e;color:#eee;text-align:center;padding:50px;font-family:Arial;'><h1 style='color:#ff6b6b;'>Reset WiFi</h1><p>Le credenziali WiFi sono state cancellate.</p><p>Il dispositivo si riavviera in modalita configurazione.</p><p>Connettiti a: <b>" WIFI_AP_NAME "</b></p></body></html>");
    delay(1000);
    wifiManager.resetSettings();  // Cancella credenziali salvate
    ESP.restart();
  });

  // Mostra configurazione IP attuale
  server.on("/api/wifi/config", HTTP_GET, []() {
    String json = "{";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
    json += "\"subnet\":\"" + WiFi.subnetMask().toString() + "\",";
    json += "\"dns\":\"" + WiFi.dnsIP().toString() + "\",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"hostname\":\"" + String(hostname) + "\",";
    json += "\"static_ip\":\"" + String(static_ip) + "\",";
    json += "\"static_gateway\":\"" + String(static_gateway) + "\",";
    json += "\"static_subnet\":\"" + String(static_subnet) + "\",";
    json += "\"static_dns\":\"" + String(static_dns) + "\",";
    json += "\"use_static\":" + String(useStaticIP ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  // Modifica configurazione IP statico
  server.on("/api/wifi/setip", HTTP_GET, []() {
    if (server.hasArg("ip")) strncpy(static_ip, server.arg("ip").c_str(), sizeof(static_ip) - 1);
    if (server.hasArg("gateway")) strncpy(static_gateway, server.arg("gateway").c_str(), sizeof(static_gateway) - 1);
    if (server.hasArg("subnet")) strncpy(static_subnet, server.arg("subnet").c_str(), sizeof(static_subnet) - 1);
    if (server.hasArg("dns")) strncpy(static_dns, server.arg("dns").c_str(), sizeof(static_dns) - 1);
    if (server.hasArg("use_static")) useStaticIP = (server.arg("use_static") == "1" || server.arg("use_static") == "true");

    saveIPConfig();

    String json = "{\"success\":true,\"message\":\"Configurazione IP salvata. Riavvia per applicare.\",";
    json += "\"ip\":\"" + String(static_ip) + "\",";
    json += "\"gateway\":\"" + String(static_gateway) + "\",";
    json += "\"subnet\":\"" + String(static_subnet) + "\",";
    json += "\"dns\":\"" + String(static_dns) + "\",";
    json += "\"use_static\":" + String(useStaticIP ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  // ========== API SENSORE GAS ==========

  // Stato sensore gas DFRobot MICS
  server.on("/api/monitor/status", HTTP_GET, []() {
    String json = "{";
    json += "\"connected\":" + String(gasSensorConnected ? "true" : "false") + ",";
    json += "\"ready\":" + String(gasSensorReady ? "true" : "false") + ",";
    json += "\"co\":" + String(gasConcentrationCO, 1) + ",";
    json += "\"ch4\":" + String(gasConcentrationCH4, 1) + ",";
    json += "\"c2h5oh\":" + String(gasConcentrationC2H5OH, 1) + ",";
    json += "\"h2\":" + String(gasConcentrationH2, 1) + ",";
    json += "\"nh3\":" + String(gasConcentrationNH3, 1) + ",";
    json += "\"no2\":" + String(gasConcentrationNO2, 1);
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

  // ========== API ALLARME GAS ==========

  // Stato allarme gas
  server.on("/api/gas/alarm", HTTP_GET, []() {
    String json = "{";
    json += "\"active\":" + String(gasAlarmActive ? "true" : "false") + ",";
    json += "\"type\":\"" + gasAlarmType + "\",";
    json += "\"value\":" + String(gasAlarmValue, 1) + ",";
    json += "\"acknowledged\":" + String(gasAlarmAcknowledged ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  // Reset/Acknowledge allarme gas
  server.on("/api/gas/alarm/ack", HTTP_GET, []() {
    acknowledgeGasAlarm();
    needsFullRedraw = true;
    server.send(200, "application/json", "{\"acknowledged\":true}");
  });

  // Reset COMPLETO allarme gas (forza stop)
  server.on("/api/gas/alarm/reset", HTTP_GET, []() {
    // Notifica fine allarme a tutti i device
    notifyGasAlarmEnd();
    // Reset allarme
    gasAlarmActive = false;
    gasAlarmType = "";
    gasAlarmValue = 0;
    gasAlarmAcknowledged = false;
    ledReady();
    // Reset display
    resetDisplayAfterAlarm();
    server.send(200, "application/json", "{\"reset\":true,\"alarm\":false}");
  });

  // Test allarme gas (per debug)
  server.on("/api/gas/alarm/test", HTTP_GET, []() {
    gasAlarmActive = true;
    gasAlarmType = "TEST";
    gasAlarmValue = 999.9;
    gasAlarmAcknowledged = false;
    sendGasAlarm();
    server.send(200, "application/json", "{\"test\":\"alarm triggered\"}");
  });

  // Test sensore gas - lettura ADC diretta (senza libreria)
  server.on("/api/gas/test", HTTP_GET, []() {
    // EN saldato a GND - sensore sempre ON

    // Leggi ADC 10 volte
    long sum = 0;
    for (int i = 0; i < 10; i++) {
      sum += analogRead(MICS_ADC_PIN);
      delay(10);
    }
    int rawAdc = sum / 10;
    float voltage = (rawAdc * 3.3) / 4095.0;

    String json = "{";
    json += "\"adc_pin\":" + String(MICS_ADC_PIN) + ",";
    json += "\"en_pin\":\"disabled\",";  // EN saldato a GND sul modulo
    json += "\"raw_adc\":" + String(rawAdc) + ",";
    json += "\"voltage\":" + String(voltage, 3) + ",";
    json += "\"connected\":" + String(gasSensorConnected ? "true" : "false") + ",";
    json += "\"ready\":" + String(gasSensorReady ? "true" : "false") + ",";

    if (rawAdc < 50) {
      json += "\"status\":\"NO_SIGNAL\",";
      json += "\"message\":\"ADC=0, verificare AOUT collegato a GPIO" + String(MICS_ADC_PIN) + "\"";
    } else if (rawAdc > 4000) {
      json += "\"status\":\"SATURATED\",";
      json += "\"message\":\"ADC saturo, verificare GND\"";
    } else {
      json += "\"status\":\"OK\",";
      json += "\"message\":\"Sensore funzionante\"";
    }
    json += "}";

    server.send(200, "application/json", json);
  });

  // Forza lettura e imposta pronto
  server.on("/api/gas/wake", HTTP_GET, []() {
    // EN saldato a GND - sensore sempre ON

    // Leggi ADC
    int rawAdc = analogRead(MICS_ADC_PIN);

    // Forza stato
    gasSensorConnected = true;
    gasSensorReady = true;

    String json = "{";
    json += "\"raw_adc\":" + String(rawAdc) + ",";
    json += "\"forced_ready\":true,";
    json += "\"status\":\"OK\"";
    json += "}";

    server.send(200, "application/json", json);
  });

  // Forza sincronizzazione immediata - toggle manuale TV
  server.on("/api/monitor/sync", HTTP_GET, []() {
    // Toggle forzato: inverte lo stato stimato della TV
    relayDeviceOn = !relayDeviceOn;
    startRelayPulse();
    server.send(200, "application/json",
      "{\"action\":\"relay_pulse\",\"relayDeviceOn\":" + String(relayDeviceOn ? "true" : "false") + "}");
  });

  server.begin();
  DEBUG_PRINTLN("WebServer avviato con tutte le API");
}

// ========== WIFI ==========
// Carica configurazione IP da NVS
void loadIPConfig() {
  preferences.begin("wifi_config", true);  // read-only

  String ip = preferences.getString("static_ip", "192.168.1.99");
  String gw = preferences.getString("gateway", "192.168.1.1");
  String sn = preferences.getString("subnet", "255.255.255.0");
  String dns_str = preferences.getString("dns", "192.168.1.1");
  useStaticIP = preferences.getBool("use_static", true);

  strncpy(static_ip, ip.c_str(), sizeof(static_ip) - 1);
  strncpy(static_gateway, gw.c_str(), sizeof(static_gateway) - 1);
  strncpy(static_subnet, sn.c_str(), sizeof(static_subnet) - 1);
  strncpy(static_dns, dns_str.c_str(), sizeof(static_dns) - 1);

  preferences.end();

  DEBUG_PRINTLN("Configurazione IP caricata:");
  DEBUG_PRINTF("  IP: %s\n", static_ip);
  DEBUG_PRINTF("  Gateway: %s\n", static_gateway);
  DEBUG_PRINTF("  Subnet: %s\n", static_subnet);
  DEBUG_PRINTF("  DNS: %s\n", static_dns);
  DEBUG_PRINTF("  Usa IP statico: %s\n", useStaticIP ? "Si" : "No");
}

// Salva configurazione IP in NVS
void saveIPConfig() {
  preferences.begin("wifi_config", false);  // read-write
  preferences.putString("static_ip", static_ip);
  preferences.putString("gateway", static_gateway);
  preferences.putString("subnet", static_subnet);
  preferences.putString("dns", static_dns);
  preferences.putBool("use_static", useStaticIP);
  preferences.end();
  DEBUG_PRINTLN("Configurazione IP salvata in NVS");
}

// Callback salvataggio parametri WiFiManager
// NOTA: Non salvare qui! I valori vengono letti DOPO la connessione
void saveParamsCallback() {
  DEBUG_PRINTLN("Parametri WiFiManager: callback salvataggio ricevuto");
  // Il salvataggio effettivo avviene in setupWiFi() dopo getValue()
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);

  // Carica configurazione IP salvata
  loadIPConfig();

  // Mostra schermata iniziale
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(2);
  tft.setCursor(60, 80);
  tft.print("Connessione");
  tft.setCursor(90, 110);
  tft.print("WiFi...");

  ledWiFiConnecting();  // LED blu

  // Configura parametri personalizzati per IP statico
  WiFiManagerParameter custom_text("<p><b>Configurazione IP Statico:</b></p>");
  WiFiManagerParameter param_ip("static_ip", "IP Address", static_ip, 16);
  WiFiManagerParameter param_gateway("gateway", "Gateway", static_gateway, 16);
  WiFiManagerParameter param_subnet("subnet", "Subnet Mask", static_subnet, 16);
  WiFiManagerParameter param_dns("dns", "DNS Server", static_dns, 16);

  // Nota: Se compili i campi IP, verra usato IP statico automaticamente
  WiFiManagerParameter custom_note("<p><small>Lascia vuoto per usare DHCP</small></p>");

  // Aggiungi parametri a WiFiManager
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&param_ip);
  wifiManager.addParameter(&param_gateway);
  wifiManager.addParameter(&param_subnet);
  wifiManager.addParameter(&param_dns);
  wifiManager.addParameter(&custom_note);

  // Configura callback
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveParamsCallback(saveParamsCallback);

  // Timeout portale configurazione
  wifiManager.setConfigPortalTimeout(WIFI_CONFIG_TIMEOUT);

  // Non bloccare se gia connesso
  wifiManager.setConnectRetries(3);

  // Configura IP statico SE abilitato e valido PRIMA di autoConnect
  if (useStaticIP) {
    IPAddress ip, gw, sn, dns_addr;
    if (ip.fromString(static_ip) && gw.fromString(static_gateway) &&
        sn.fromString(static_subnet) && dns_addr.fromString(static_dns)) {
      wifiManager.setSTAStaticIPConfig(ip, gw, sn, dns_addr);
      DEBUG_PRINTLN("IP statico configurato per WiFiManager");
    }
  }

  // Tenta connessione automatica, altrimenti avvia portale
  bool connected = wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD);

  if (connected) {
    // Leggi parametri inseriti dall'utente (solo se in config mode)
    if (wifiConfigMode) {
      const char* newIP = param_ip.getValue();
      const char* newGW = param_gateway.getValue();
      const char* newSN = param_subnet.getValue();
      const char* newDNS = param_dns.getValue();

      // Aggiorna solo se i valori sono stati modificati (non vuoti)
      if (strlen(newIP) > 0) strncpy(static_ip, newIP, sizeof(static_ip) - 1);
      if (strlen(newGW) > 0) strncpy(static_gateway, newGW, sizeof(static_gateway) - 1);
      if (strlen(newSN) > 0) strncpy(static_subnet, newSN, sizeof(static_subnet) - 1);
      if (strlen(newDNS) > 0) strncpy(static_dns, newDNS, sizeof(static_dns) - 1);

      // Se IP e' stato inserito, abilita IP statico
      useStaticIP = (strlen(newIP) > 0);

      // Salva nuovi parametri
      saveIPConfig();
      DEBUG_PRINTLN("Nuova configurazione IP salvata:");
      DEBUG_PRINTF("  IP: %s\n", static_ip);
      DEBUG_PRINTF("  Gateway: %s\n", static_gateway);
      DEBUG_PRINTF("  Usa statico: %s\n", useStaticIP ? "Si" : "No");
    }

    DEBUG_PRINTLN("\nWiFi connesso!");
    DEBUG_PRINT("IP: ");
    DEBUG_PRINTLN(WiFi.localIP());
    DEBUG_PRINT("SSID: ");
    DEBUG_PRINTLN(WiFi.SSID());

    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_GREEN);
    tft.setTextSize(2);
    tft.setCursor(60, 80);
    tft.print("CONNESSO!");

    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(40, 115);
    tft.print("SSID: ");
    tft.print(WiFi.SSID());

    tft.setCursor(40, 135);
    tft.print("IP: ");
    tft.print(WiFi.localIP().toString());

    tft.setTextColor(COLOR_GRAY);
    tft.setCursor(30, 160);
    tft.print("http://");
    tft.print(hostname);
    tft.print(".local");

    delay(2000);

    if (MDNS.begin(hostname)) {
      DEBUG_PRINTF("mDNS: http://%s.local\n", hostname);
    }

    // Discovery automatica al boot (partira dopo 5 sec nel loop, senza bloccare)
    if (discoveryEnabled) {
      lastDiscoveryRun = millis() - DISCOVERY_AUTO_INTERVAL + 5000; // Forza prima discovery dopo 5 sec
    }

    wifiConfigMode = false;
    ledReady();  // LED verde

  } else {
    DEBUG_PRINTLN("\nErrore connessione WiFi!");
    DEBUG_PRINTLN("Timeout portale configurazione.");

    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_RED);
    tft.setTextSize(2);
    tft.setCursor(30, 70);
    tft.print("WiFi ERRORE!");

    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(20, 110);
    tft.print("Nessuna rete configurata");
    tft.setCursor(20, 130);
    tft.print("o timeout raggiunto.");

    tft.setTextColor(COLOR_YELLOW);
    tft.setCursor(20, 160);
    tft.print("Riavvio in 5 secondi...");
    tft.setCursor(20, 180);
    tft.print("Tieni premuto BOOT per");
    tft.setCursor(20, 195);
    tft.print("forzare config mode.");

    ledError();
    delay(5000);
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

// ========== QR CODE SUL DISPLAY (ESP-IDF nativo) ==========
#ifdef USE_QRCODE
// Variabili globali per passare parametri alla callback
static int qr_draw_x = 0;
static int qr_draw_y = 0;
static int qr_module_size = 2;

// Callback per disegnare QR code sul display TFT
void qrcode_tft_display(esp_qrcode_handle_t qrcode) {
  int size = esp_qrcode_get_size(qrcode);
  int qrSize = size * qr_module_size;

  // Disegna sfondo bianco con bordo
  tft.fillRect(qr_draw_x - 4, qr_draw_y - 4, qrSize + 8, qrSize + 8, TFT_WHITE);

  // Disegna moduli QR
  for (int qy = 0; qy < size; qy++) {
    for (int qx = 0; qx < size; qx++) {
      if (esp_qrcode_get_module(qrcode, qx, qy)) {
        tft.fillRect(qr_draw_x + qx * qr_module_size,
                     qr_draw_y + qy * qr_module_size,
                     qr_module_size, qr_module_size, TFT_BLACK);
      }
    }
  }
}

// Funzione wrapper per disegnare QR code
void drawQRCode(const char* text, int x, int y, int moduleSize) {
  // Imposta parametri globali per la callback
  qr_draw_x = x;
  qr_draw_y = y;
  qr_module_size = moduleSize;

  // Configura QR code con callback personalizzata
  esp_qrcode_config_t cfg = {
    .display_func = qrcode_tft_display,
    .max_qrcode_version = 10,
    .qrcode_ecc_level = ESP_QRCODE_ECC_LOW
  };

  // Genera e disegna QR code
  esp_err_t err = esp_qrcode_generate(&cfg, text);
  if (err != ESP_OK) {
    DEBUG_PRINTLN("Errore generazione QR code!");
  }
}
#endif

// Mostra schermata configurazione WiFi
void drawWiFiConfigScreen() {
  tft.fillScreen(COLOR_BG);

  // Titolo
  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(2);
  tft.setCursor(30, 10);
  tft.print("CONFIG. WiFi");

  // Istruzioni
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 45);
  tft.print("1. Connettiti alla rete:");

  tft.setTextColor(COLOR_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.print(WIFI_AP_NAME);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 90);
  tft.print("2. Password:");
  tft.setTextColor(COLOR_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 105);
  tft.print(WIFI_AP_PASSWORD);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 135);
  tft.print("3. Apri browser:");
  tft.setTextColor(COLOR_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 150);
  tft.print("192.168.4.1");

#ifdef USE_QRCODE
  // QR Code per connessione WiFi all'AP
  // Formato standard WiFi: WIFI:T:WPA;S:ssid;P:password;;
  String wifiQR = "WIFI:T:WPA;S:";
  wifiQR += WIFI_AP_NAME;
  wifiQR += ";P:";
  wifiQR += WIFI_AP_PASSWORD;
  wifiQR += ";;";

  // Disegna QR code
  drawQRCode(wifiQR.c_str(), 200, 45, 2);

  tft.setTextColor(COLOR_GRAY);
  tft.setTextSize(1);
  tft.setCursor(195, 120);
  tft.print("Scansiona QR");
#endif

  // Box informativo
  tft.drawRect(5, 180, 310, 55, COLOR_GRAY);
  tft.setTextColor(COLOR_ORANGE);
  tft.setTextSize(1);
  tft.setCursor(15, 190);
  tft.print("Configura SSID, password e IP statico");
  tft.setCursor(15, 205);
  tft.print("nella pagina web che si aprira'.");

  tft.setTextColor(COLOR_RED);
  tft.setCursor(15, 220);
  tft.print("Timeout: ");
  tft.print(WIFI_CONFIG_TIMEOUT);
  tft.print(" secondi");
}

// Callback quando entra in modalita configurazione
void configModeCallback(WiFiManager *myWiFiManager) {
  DEBUG_PRINTLN("Modalita configurazione WiFi attiva!");
  DEBUG_PRINT("AP SSID: ");
  DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());
  DEBUG_PRINT("AP IP: ");
  DEBUG_PRINTLN(WiFi.softAPIP());

  wifiConfigMode = true;
  ledWiFiConnecting();  // LED blu
  drawWiFiConfigScreen();
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
  tft.setCursor(70, 155);
  tft.print("WiFiManager + OTA");

  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(75, 185);
  tft.print("by Sambinello Paolo");

  tft.setCursor(85, 210);
  tft.print("v3.1 - 2026");

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

  // Inizializza sensore gas DFRobot MICS (sostituisce TEMT6000)
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
  DEBUG_PRINTF("Sensore Gas MICS: ADC=GPIO%d (EN saldato a GND)\n", MICS_ADC_PIN);
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

  // Aggiorna sensore gas DFRobot MICS
  updateMonitorSensor();
  syncMonitorState();  // Legacy - non più utilizzato

  // Aggiorna sensore luce
  updateLightSensor();

  // Discovery dispositivi (non bloccante)
  discoveryStep();
  verifyStep();  // Verifica candidati con /radar/ping

  // Auto-discovery periodico: IP scan ogni 5 minuti (non bloccante)
  if (discoveryEnabled && DISCOVERY_AUTO_INTERVAL > 0 && !discoveryActive) {
    if (now - lastDiscoveryRun > DISCOVERY_AUTO_INTERVAL) {
      startDiscovery();  // IP scan su porte 80, 8080, 8081 (non bloccante)
    }
  }

  // mDNS discovery periodica: ogni 30 minuti (bloccante ~1-2 sec, quindi meno frequente)
  static unsigned long lastMdnsDiscovery = 0;
  if (discoveryEnabled && !discoveryActive) {
    if (lastMdnsDiscovery == 0 || (now - lastMdnsDiscovery > 1800000)) {  // 30 min
      discoverMdnsDevices();
      lastMdnsDiscovery = now;
    }
  }

  // Risoluzione periodica mDNS - aggiorna IP dei client ogni 2 minuti
  static unsigned long lastMdnsResolve = 0;
  if (now - lastMdnsResolve > 120000) {  // Ogni 2 minuti
    resolveAllClientsMdns();
    lastMdnsResolve = now;
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

  // Auto-pulizia device morti ogni 5 minuti
  static unsigned long lastPurgeCheck = 0;
  if (now - lastPurgeCheck > 300000) {  // 5 minuti
    lastPurgeCheck = now;
    for (int i = clientCount - 1; i >= 0; i--) {
      if (clients[i].failCount >= CLIENT_MAX_CONSECUTIVE_FAILS && clients[i].lastSeen == 0) {
        DEBUG_PRINTF("[PURGE] Rimosso %s (%s:%d) - irraggiungibile\n",
                     clients[i].name.c_str(), clients[i].ip.c_str(), clients[i].port);
        removeClient(i);
      }
    }
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
