// ================== oraQuadra Joystick Controller - ESP32-C3 ==================
// Controller joystick esterno per oraQuadra Nano Arcade
// Comunicazione via ESP-NOW (stesso protocollo del Dual Display)
// Hardware: Joystick analogico 2 assi + 4 pulsanti + OLED 1.3" SH1106 I2C
// Auto-discovery: invia PKT_DISCOVERY_REQ con role=3, oraQuadra risponde
//
// Dipendenze librerie (installa da Arduino Library Manager):
//   - WiFiManager by tzapu (>=2.0.0)
//   - U8g2 by oliver (>=2.34)
//   - ArduinoOTA (inclusa nel core ESP32)
//
// Board: ESP32-C3 Dev Module
// Flash: 4MB, Partition: Default (oppure "Minimal SPIFFS" per piu' spazio OTA)
// ============================================================================

#include <WiFi.h>
#include <Wire.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <U8g2lib.h>

// ===== Pin Assignment ESP32-C3 =====
#define JOY_X_PIN       0   // ADC - Asse X joystick analogico
#define JOY_Y_PIN       1   // ADC - Asse Y joystick analogico
#define BTN_FIRE_PIN    3   // Pulsante FIRE (INPUT_PULLUP, attivo LOW)
#define BTN_START_PIN   4   // Pulsante START
#define BTN_COIN_PIN    5   // Pulsante COIN
#define BTN_EXTRA_PIN   10  // Pulsante EXTRA
#define LED_STATUS_PIN  8   // LED stato (cercando/connesso/attivo)
#define OLED_SDA_PIN    6   // I2C SDA per display OLED
#define OLED_SCL_PIN    7   // I2C SCL per display OLED

// ===== Button bitmask (identica a arcade_config.h) =====
#define BUTTON_LEFT   0x01
#define BUTTON_RIGHT  0x02
#define BUTTON_UP     0x04
#define BUTTON_DOWN   0x08
#define BUTTON_FIRE   0x10
#define BUTTON_START  0x20
#define BUTTON_COIN   0x40
#define BUTTON_EXTRA  0x80

// ===== Protocollo ESP-NOW (compatibile con Dual Display) =====
#define DUAL_MAGIC          0xDC
#define PKT_DISCOVERY_REQ   4
#define PKT_DISCOVERY_RESP  5
#define PKT_JOYSTICK        8
#define ROLE_JOYSTICK       3

// ===== Strutture pacchetti =====
struct JoystickPacket {
  uint8_t  magic;
  uint8_t  packetType;
  uint8_t  buttons;
  uint8_t  mac[6];
};

struct DiscoveryPacket {
  uint8_t  magic;
  uint8_t  packetType;
  uint8_t  role;
  uint8_t  gridW;
  uint8_t  gridH;
  uint8_t  mac[6];
  uint8_t  assignedX;
  uint8_t  assignedY;
  uint8_t  enabled;
};

// ===== Configurazione =====
#define JOY_DEADZONE       25      // Deadzone su valore normalizzato (-100..+100)
#define JOY_OVERSAMPLE     4
#define JOY_CAL_SAMPLES    64      // Campioni per auto-calibrazione centro
#define JOY_CAL_MOVE_TIME  2500    // Tempo (ms) per muovere il joystick durante calibrazione
#define DEBOUNCE_MS        20
#define SEND_INTERVAL_MS   33
#define DISCOVERY_INTERVAL 3000
#define RECONNECT_FAILS    10
#define LED_BLINK_FAST     150
#define LED_BLINK_SLOW     1000
#define DISPLAY_INTERVAL   80      // Aggiornamento OLED ogni 80ms (~12fps)

#define OTA_HOSTNAME       "oraquadra-joystick"
#define WIFIMGR_AP_NAME    "OraQuadra-Joystick"
#define WIFIMGR_TIMEOUT    180

// ===== Stato =====
enum JoyState {
  STATE_WIFI_SETUP,   // Configurazione WiFi in corso
  STATE_SEARCHING,    // Cercando oraQuadra
  STATE_CONNECTED,    // Connesso e attivo
  STATE_IDLE
};

JoyState currentState = STATE_WIFI_SETUP;
uint8_t targetMAC[6] = {0};
bool targetRegistered = false;
uint8_t myMAC[6] = {0};
int sendFailCount = 0;
unsigned long lastSendTime = 0;
unsigned long lastDiscoveryTime = 0;
unsigned long lastHeartbeatRecv = 0;
uint8_t lastButtonState = 0;

// LED
unsigned long ledLastToggle = 0;
bool ledOn = false;

// Debounce pulsanti
unsigned long btnFireLastChange = 0;
unsigned long btnStartLastChange = 0;
unsigned long btnCoinLastChange = 0;
unsigned long btnExtraLastChange = 0;
bool btnFireState = false;
bool btnStartState = false;
bool btnCoinState = false;
bool btnExtraState = false;

// Valori analogici raw (per display)
int joyRawX = 2048;
int joyRawY = 2048;

// Calibrazione joystick (centro + min/max per ogni asse)
int joyCenterX = 2048;
int joyCenterY = 2048;
int joyMinX = 0;
int joyMaxX = 4095;
int joyMinY = 0;
int joyMaxY = 4095;

// Valori normalizzati -100..+100 (dopo calibrazione e map)
int joyNormX = 0;
int joyNormY = 0;

// WiFi
bool wifiConnected = false;

// OTA
bool otaInProgress = false;
unsigned int otaProgress = 0;

// WiFiManager
WiFiManager wifiManager;

// ESP-NOW
bool _espNowInit = false;

// ===== Display OLED 1.3" SH1106 128x64 I2C =====
// Se il tuo display usa SSD1306 invece di SH1106, cambia la riga sotto:
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
unsigned long lastDisplayUpdate = 0;
bool oledReady = false;

// Animazione searching
uint8_t searchAnim = 0;

// ===== Forward declarations =====
void initEspNow();
void sendDiscovery();
void sendJoystickPacket(uint8_t buttons);
uint8_t readInputs();
void updateLED();
void initOTA();
void updateDisplay();
void drawSplash();
void drawSearching();
void drawConnected(uint8_t buttons);
void drawOtaProgress();
void drawJoystickCross(int cx, int cy, int radius, int dotX, int dotY);
void drawButtonBox(int x, int y, int w, int h, const char* label, bool pressed);
void calibrateJoystick();

// ===== Callback ricezione ESP-NOW =====
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
  if (len < 2 || data[0] != DUAL_MAGIC) return;

  if (data[1] == PKT_DISCOVERY_RESP) {
    if ((size_t)len < sizeof(DiscoveryPacket)) return;

    memcpy(targetMAC, mac_addr, 6);
    if (!targetRegistered) {
      esp_now_peer_info_t peerInfo;
      memset(&peerInfo, 0, sizeof(peerInfo));
      memcpy(peerInfo.peer_addr, mac_addr, 6);
      peerInfo.channel = 0;
      peerInfo.encrypt = false;
      esp_err_t res = esp_now_add_peer(&peerInfo);
      if (res == ESP_OK || res == ESP_ERR_ESPNOW_EXIST) {
        targetRegistered = true;
      }
    }

    currentState = STATE_CONNECTED;
    sendFailCount = 0;
    lastHeartbeatRecv = millis();
    Serial.println("[JOY] CONNESSO a oraQuadra!");
  }
}

// ===== Callback invio ESP-NOW =====
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    sendFailCount++;
    if (sendFailCount >= RECONNECT_FAILS && currentState == STATE_CONNECTED) {
      currentState = STATE_SEARCHING;
      targetRegistered = false;
      sendFailCount = 0;
    }
  } else {
    sendFailCount = 0;
  }
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== oraQuadra Joystick Controller C3 ===");

  // Pin setup
  pinMode(BTN_FIRE_PIN, INPUT_PULLUP);
  pinMode(BTN_START_PIN, INPUT_PULLUP);
  pinMode(BTN_COIN_PIN, INPUT_PULLUP);
  pinMode(BTN_EXTRA_PIN, INPUT_PULLUP);
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(LED_STATUS_PIN, LOW);

  // ADC
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // --- OLED Display ---
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  oled.begin();
  oledReady = true;

  // --- Auto-calibrazione centro joystick ---
  // Legge la posizione a riposo e la usa come riferimento
  calibrateJoystick();

  drawSplash();
  delay(1500);

  // --- WiFiManager ---
  currentState = STATE_WIFI_SETUP;
  updateDisplay();

  wifiManager.setConfigPortalTimeout(WIFIMGR_TIMEOUT);
  wifiManager.setAPCallback([](WiFiManager* mgr) {
    Serial.println("[JOY] Portale captive aperto");
  });

  digitalWrite(LED_STATUS_PIN, HIGH);
  bool connected = wifiManager.autoConnect(WIFIMGR_AP_NAME);

  if (connected) {
    wifiConnected = true;
    Serial.printf("[JOY] WiFi connesso! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[JOY] WiFiManager timeout, riavvio...");
    if (oledReady) {
      oled.clearBuffer();
      oled.setFont(u8g2_font_6x10_tr);
      oled.drawStr(20, 35, "WiFi TIMEOUT");
      oled.drawStr(25, 50, "Riavvio...");
      oled.sendBuffer();
    }
    delay(2000);
    ESP.restart();
  }

  // --- OTA ---
  initOTA();

  // --- ESP-NOW ---
  initEspNow();

  currentState = STATE_SEARCHING;
  Serial.println("[JOY] Setup completato, ricerca oraQuadra...");
}

// ===== Loop principale =====
void loop() {
  ArduinoOTA.handle();

  if (otaInProgress) {
    updateDisplay();
    delay(10);
    return;
  }

  // Riconnessione WiFi
  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    WiFi.reconnect();
    wifiConnected = false;
    currentState = STATE_SEARCHING;
    delay(1000);
    return;
  }
  if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
    wifiConnected = true;
    if (!_espNowInit) initEspNow();
  }

  unsigned long now = millis();

  // Discovery
  if (currentState == STATE_SEARCHING) {
    if (now - lastDiscoveryTime >= DISCOVERY_INTERVAL) {
      sendDiscovery();
      lastDiscoveryTime = now;
    }
  }

  // Input & send
  if (currentState == STATE_CONNECTED) {
    if (now - lastHeartbeatRecv > 10000) {
      currentState = STATE_SEARCHING;
      targetRegistered = false;
    }
    if (now - lastSendTime >= SEND_INTERVAL_MS) {
      uint8_t buttons = readInputs();
      sendJoystickPacket(buttons);
      lastButtonState = buttons;
      lastSendTime = now;
    }
  } else {
    // Leggi input anche se non connesso (per mostrarlo sul display)
    if (now - lastSendTime >= SEND_INTERVAL_MS) {
      lastButtonState = readInputs();
      lastSendTime = now;
    }
  }

  // Display
  updateDisplay();

  // LED
  updateLED();

  delay(1);
}

// ===== Mappa proporzionale per ogni semiasse =====
// raw < center → map(raw, min, center) → -100..0
// raw > center → map(raw, center, max) →  0..+100
int joyMapAxis(int raw, int minVal, int center, int maxVal) {
  if (raw <= center) {
    if (center == minVal) return 0; // Evita divisione per zero
    return (int)map(raw, minVal, center, -100, 0);
  } else {
    if (maxVal == center) return 0;
    return (int)map(raw, center, maxVal, 0, 100);
  }
}

// ===== Lettura input =====
uint8_t readInputs() {
  uint8_t buttons = 0;

  long sumX = 0, sumY = 0;
  for (int i = 0; i < JOY_OVERSAMPLE; i++) {
    sumX += analogRead(JOY_X_PIN);
    sumY += analogRead(JOY_Y_PIN);
  }
  joyRawX = sumX / JOY_OVERSAMPLE;
  joyRawY = sumY / JOY_OVERSAMPLE;

  // Normalizza -100..+100 con mappa proporzionale per ogni semiasse
  joyNormX = constrain(joyMapAxis(joyRawX, joyMinX, joyCenterX, joyMaxX), -100, 100);
  joyNormY = constrain(joyMapAxis(joyRawY, joyMinY, joyCenterY, joyMaxY), -100, 100);

  // Deadzone su valore normalizzato
  if (joyNormX < -JOY_DEADZONE) buttons |= BUTTON_LEFT;
  if (joyNormX >  JOY_DEADZONE) buttons |= BUTTON_RIGHT;
  if (joyNormY < -JOY_DEADZONE) buttons |= BUTTON_UP;
  if (joyNormY >  JOY_DEADZONE) buttons |= BUTTON_DOWN;

  unsigned long now = millis();

  bool fireRaw = !digitalRead(BTN_FIRE_PIN);
  if (fireRaw != btnFireState && (now - btnFireLastChange) >= DEBOUNCE_MS) {
    btnFireState = fireRaw; btnFireLastChange = now;
  }
  if (btnFireState) buttons |= BUTTON_FIRE;

  bool startRaw = !digitalRead(BTN_START_PIN);
  if (startRaw != btnStartState && (now - btnStartLastChange) >= DEBOUNCE_MS) {
    btnStartState = startRaw; btnStartLastChange = now;
  }
  if (btnStartState) buttons |= BUTTON_START;

  bool coinRaw = !digitalRead(BTN_COIN_PIN);
  if (coinRaw != btnCoinState && (now - btnCoinLastChange) >= DEBOUNCE_MS) {
    btnCoinState = coinRaw; btnCoinLastChange = now;
  }
  if (btnCoinState) buttons |= BUTTON_COIN;

  bool extraRaw = !digitalRead(BTN_EXTRA_PIN);
  if (extraRaw != btnExtraState && (now - btnExtraLastChange) >= DEBOUNCE_MS) {
    btnExtraState = extraRaw; btnExtraLastChange = now;
  }
  if (btnExtraState) buttons |= BUTTON_EXTRA;

  return buttons;
}

// ========================================================================
// =================== DISPLAY OLED =======================================
// ========================================================================

void updateDisplay() {
  if (!oledReady) return;
  unsigned long now = millis();
  if (now - lastDisplayUpdate < DISPLAY_INTERVAL) return;
  lastDisplayUpdate = now;

  if (otaInProgress) {
    drawOtaProgress();
    return;
  }

  oled.clearBuffer();

  // --- Barra stato in alto (y=0..12) ---
  oled.setFont(u8g2_font_6x10_tr);

  // Titolo
  oled.drawStr(0, 9, "JOY");

  // Icona WiFi (3 archetti)
  if (wifiConnected) {
    int wx = 26;
    oled.drawCircle(wx, 9, 2, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    oled.drawCircle(wx, 9, 5, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    oled.drawCircle(wx, 9, 8, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    oled.drawDisc(wx, 9, 1);
  } else {
    oled.drawStr(22, 9, "---");
  }

  // Stato connessione oraQuadra
  if (currentState == STATE_CONNECTED) {
    oled.drawStr(40, 9, "oQ:OK");
    // Pallino pieno = connesso
    oled.drawDisc(76, 5, 3);
  } else if (currentState == STATE_SEARCHING) {
    // Animazione ricerca: puntini rotanti
    searchAnim = (searchAnim + 1) % 4;
    char dots[5] = "oQ: ";
    for (int i = 0; i < (int)searchAnim + 1; i++) dots[3 + i] = '.';
    dots[4 + searchAnim] = '\0';
    // Scrivi "oQ:" e poi puntini animati
    oled.drawStr(40, 9, "oQ:");
    for (int i = 0; i <= (int)searchAnim; i++) {
      oled.drawDisc(60 + i * 5, 6, 1);
    }
  } else if (currentState == STATE_WIFI_SETUP) {
    oled.drawStr(40, 9, "WiFi...");
  }

  // Pacchetti/s indicatore
  if (currentState == STATE_CONNECTED) {
    oled.setFont(u8g2_font_5x7_tr);
    oled.drawStr(100, 8, "30fps");
  }

  // --- Linea separatore ---
  oled.drawHLine(0, 12, 128);

  // --- Area centrale (y=14..52) ---
  // Sinistra: Joystick cross grafico (cerchio con punto mobile)
  int jCx = 28;   // Centro cerchio joystick
  int jCy = 34;
  int jR  = 17;   // Raggio cerchio

  // Mappa valori normalizzati (-100..+100) -> offset pixel (-jR..+jR)
  int dotX = map(joyNormX, -100, 100, -jR + 2, jR - 2);
  int dotY = map(joyNormY, -100, 100, -jR + 2, jR - 2);

  drawJoystickCross(jCx, jCy, jR, dotX, dotY);

  // Valori normalizzati sotto il cerchio
  oled.setFont(u8g2_font_5x7_tr);
  char buf[16];
  snprintf(buf, sizeof(buf), "X:%+4d", joyNormX);
  oled.drawStr(2, 58, buf);
  snprintf(buf, sizeof(buf), "Y:%+4d", joyNormY);
  oled.drawStr(2, 64, buf);

  // Frecce direzione attive (intorno al cerchio)
  uint8_t b = lastButtonState;
  if (b & BUTTON_UP)    oled.drawTriangle(jCx, jCy - jR - 5, jCx - 3, jCy - jR - 1, jCx + 3, jCy - jR - 1);
  if (b & BUTTON_DOWN)  oled.drawTriangle(jCx, jCy + jR + 5, jCx - 3, jCy + jR + 1, jCx + 3, jCy + jR + 1);
  if (b & BUTTON_LEFT)  oled.drawTriangle(jCx - jR - 5, jCy, jCx - jR - 1, jCy - 3, jCx - jR - 1, jCy + 3);
  if (b & BUTTON_RIGHT) oled.drawTriangle(jCx + jR + 5, jCy, jCx + jR + 1, jCy - 3, jCx + jR + 1, jCy + 3);

  // Destra: 4 pulsanti come box grafici (2x2 griglia)
  int bx0 = 60;  // Inizio area pulsanti
  int by0 = 16;
  int bw  = 32;  // Larghezza box
  int bh  = 15;  // Altezza box
  int gap = 3;

  drawButtonBox(bx0,           by0,             bw, bh, "FIRE",  b & BUTTON_FIRE);
  drawButtonBox(bx0 + bw + gap, by0,            bw, bh, "START", b & BUTTON_START);
  drawButtonBox(bx0,           by0 + bh + gap,  bw, bh, "COIN",  b & BUTTON_COIN);
  drawButtonBox(bx0 + bw + gap, by0 + bh + gap, bw, bh, "EXTRA", b & BUTTON_EXTRA);

  // --- Barra info in basso (y=54..64) ---
  oled.drawHLine(0, 53, 128);

  // IP address (se connesso)
  if (wifiConnected) {
    oled.setFont(u8g2_font_5x7_tr);
    oled.drawStr(50, 64, WiFi.localIP().toString().c_str());
  }

  oled.sendBuffer();
}

// ===== Splash screen iniziale =====
void drawSplash() {
  oled.clearBuffer();

  // Bordo decorativo
  oled.drawRFrame(0, 0, 128, 64, 4);
  oled.drawRFrame(2, 2, 124, 60, 3);

  // Titolo grande
  oled.setFont(u8g2_font_helvB12_tr);
  // Centra "oraQuadra"
  int tw = oled.getStrWidth("oraQuadra");
  oled.drawStr((128 - tw) / 2, 26, "oraQuadra");

  // Sottotitolo
  oled.setFont(u8g2_font_helvB08_tr);
  tw = oled.getStrWidth("JOYSTICK");
  oled.drawStr((128 - tw) / 2, 40, "JOYSTICK");

  // Versione
  oled.setFont(u8g2_font_5x7_tr);
  oled.drawStr(44, 54, "ESP32-C3");

  // Icona joystick stilizzata (in basso a sinistra)
  oled.drawCircle(18, 50, 6);
  oled.drawLine(18, 44, 18, 40);   // Leva su
  oled.drawDisc(18, 50, 2);        // Pomello

  // Icona controller (in basso a destra)
  oled.drawRFrame(100, 44, 16, 12, 2);
  oled.drawDisc(105, 50, 1);  // D-pad
  oled.drawDisc(111, 48, 1);  // Bottone

  oled.sendBuffer();
}

// ===== Schermata aggiornamento OTA =====
void drawOtaProgress() {
  oled.clearBuffer();

  oled.setFont(u8g2_font_helvB10_tr);
  int tw = oled.getStrWidth("OTA UPDATE");
  oled.drawStr((128 - tw) / 2, 18, "OTA UPDATE");

  // Progress bar
  int barX = 10, barY = 28, barW = 108, barH = 14;
  oled.drawRFrame(barX, barY, barW, barH, 3);
  int fillW = (int)((long)(barW - 4) * otaProgress / 100);
  if (fillW > 0) {
    oled.drawRBox(barX + 2, barY + 2, fillW, barH - 4, 2);
  }

  // Percentuale
  char buf[8];
  snprintf(buf, sizeof(buf), "%u%%", otaProgress);
  oled.setFont(u8g2_font_helvB12_tr);
  tw = oled.getStrWidth(buf);
  oled.drawStr((128 - tw) / 2, 58, buf);

  oled.sendBuffer();
}

// ===== Disegna joystick: cerchio con croce e punto mobile =====
void drawJoystickCross(int cx, int cy, int r, int dotX, int dotY) {
  // Cerchio esterno
  oled.drawCircle(cx, cy, r);

  // Croce centrale (assi)
  oled.drawHLine(cx - r + 2, cy, (r - 2) * 2);
  oled.drawVLine(cx, cy - r + 2, (r - 2) * 2);

  // Tacche intermedie sugli assi
  int half = r / 2;
  oled.drawVLine(cx - half, cy - 1, 3);
  oled.drawVLine(cx + half, cy - 1, 3);
  oled.drawHLine(cx - 1, cy - half, 3);
  oled.drawHLine(cx - 1, cy + half, 3);

  // Punto posizione joystick (disco pieno 3px)
  int px = cx + dotX;
  int py = cy + dotY;
  // Clamp dentro il cerchio
  int dx = px - cx;
  int dy = py - cy;
  if (dx * dx + dy * dy > (r - 3) * (r - 3)) {
    float angle = atan2(dy, dx);
    px = cx + (int)((r - 3) * cos(angle));
    py = cy + (int)((r - 3) * sin(angle));
  }
  oled.drawDisc(px, py, 3);
}

// ===== Disegna box pulsante con label (pieno se premuto) =====
void drawButtonBox(int x, int y, int w, int h, const char* label, bool pressed) {
  if (pressed) {
    // Box pieno con testo invertito
    oled.drawRBox(x, y, w, h, 3);
    oled.setFont(u8g2_font_5x8_tr);
    oled.setDrawColor(0);  // Testo nero su sfondo bianco
    int tw = oled.getStrWidth(label);
    oled.drawStr(x + (w - tw) / 2, y + h - 4, label);
    oled.setDrawColor(1);  // Ripristina bianco
  } else {
    // Box vuoto con bordo
    oled.drawRFrame(x, y, w, h, 3);
    oled.setFont(u8g2_font_5x8_tr);
    int tw = oled.getStrWidth(label);
    oled.drawStr(x + (w - tw) / 2, y + h - 4, label);
  }
}

// ========================================================================
// =================== CALIBRAZIONE JOYSTICK ==============================
// ========================================================================

// Fase 1: centro (riposo) + Fase 2: min/max (muovi in tutte le direzioni)
void calibrateJoystick() {

  // ===== FASE 1: Lettura centro (joystick a riposo) =====
  if (oledReady) {
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x10_tr);
    oled.drawStr(14, 15, "CALIBRAZIONE");
    oled.setFont(u8g2_font_5x7_tr);
    oled.drawStr(6, 30, "1/2 Non toccare il joy");
    oled.drawRFrame(14, 38, 100, 10, 3);
    oled.sendBuffer();
  }

  long sumX = 0, sumY = 0;
  for (int i = 0; i < JOY_CAL_SAMPLES; i++) {
    sumX += analogRead(JOY_X_PIN);
    sumY += analogRead(JOY_Y_PIN);
    delay(5);
    if (oledReady && (i % 8 == 0)) {
      int fillW = (int)((long)96 * i / JOY_CAL_SAMPLES);
      oled.drawRBox(16, 40, fillW, 6, 2);
      oled.sendBuffer();
    }
  }

  joyCenterX = sumX / JOY_CAL_SAMPLES;
  joyCenterY = sumY / JOY_CAL_SAMPLES;

  // Inizializza min/max con il centro
  joyMinX = joyCenterX;
  joyMaxX = joyCenterX;
  joyMinY = joyCenterY;
  joyMaxY = joyCenterY;

  Serial.printf("[JOY] Centro: X=%d Y=%d\n", joyCenterX, joyCenterY);

  // Mostra risultato fase 1
  if (oledReady) {
    oled.drawRBox(16, 40, 96, 6, 2);
    char buf[24];
    oled.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "C: %d,%d", joyCenterX, joyCenterY);
    oled.drawStr(28, 58, buf);
    oled.sendBuffer();
    delay(500);
  }

  // ===== FASE 2: Lettura min/max (muovi il joystick ovunque) =====
  if (oledReady) {
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x10_tr);
    oled.drawStr(14, 15, "CALIBRAZIONE");
    oled.setFont(u8g2_font_5x7_tr);
    oled.drawStr(2, 30, "2/2 Ruota il joy ovunque");
    // Cerchio vuoto al centro come anteprima
    oled.drawCircle(64, 50, 10);
    oled.drawRFrame(14, 38, 100, 4, 1);
    oled.sendBuffer();
  }

  unsigned long calStart = millis();
  unsigned long calEnd = calStart + JOY_CAL_MOVE_TIME;

  while (millis() < calEnd) {
    // Leggi con oversampling
    long sx = 0, sy = 0;
    for (int j = 0; j < JOY_OVERSAMPLE; j++) {
      sx += analogRead(JOY_X_PIN);
      sy += analogRead(JOY_Y_PIN);
    }
    int rx = sx / JOY_OVERSAMPLE;
    int ry = sy / JOY_OVERSAMPLE;

    // Aggiorna min/max
    if (rx < joyMinX) joyMinX = rx;
    if (rx > joyMaxX) joyMaxX = rx;
    if (ry < joyMinY) joyMinY = ry;
    if (ry > joyMaxY) joyMaxY = ry;

    // Aggiorna display: barra progresso + pallino in tempo reale
    if (oledReady) {
      unsigned long elapsed = millis() - calStart;
      int fillW = (int)((long)96 * elapsed / JOY_CAL_MOVE_TIME);
      if (fillW > 96) fillW = 96;

      // Barra progresso
      oled.drawRBox(16, 38, fillW, 4, 1);

      // Pallino posizione attuale nel cerchietto
      int dx = map(rx, joyMinX, joyMaxX, -8, 8);
      int dy = map(ry, joyMinY, joyMaxY, -8, 8);
      // Cancella cerchio (ridisegna sfondo)
      oled.setDrawColor(0);
      oled.drawDisc(64, 50, 9);
      oled.setDrawColor(1);
      oled.drawCircle(64, 50, 10);
      oled.drawDisc(64 + dx, 50 + dy, 2);

      oled.sendBuffer();
    }

    delay(10);
  }

  // Margine di sicurezza: aggiungi 5% oltre il range misurato
  int rangeX = joyMaxX - joyMinX;
  int rangeY = joyMaxY - joyMinY;
  joyMinX = max(0, joyMinX - rangeX / 20);
  joyMaxX = min(4095, joyMaxX + rangeX / 20);
  joyMinY = max(0, joyMinY - rangeY / 20);
  joyMaxY = min(4095, joyMaxY + rangeY / 20);

  Serial.printf("[JOY] Range X: %d..%d..%d  Y: %d..%d..%d\n",
                joyMinX, joyCenterX, joyMaxX, joyMinY, joyCenterY, joyMaxY);

  // Mostra risultato finale
  if (oledReady) {
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x10_tr);
    oled.drawStr(30, 15, "CAL OK!");
    oled.setFont(u8g2_font_5x7_tr);

    char buf[28];
    snprintf(buf, sizeof(buf), "X: %d..%d..%d", joyMinX, joyCenterX, joyMaxX);
    oled.drawStr(8, 32, buf);
    snprintf(buf, sizeof(buf), "Y: %d..%d..%d", joyMinY, joyCenterY, joyMaxY);
    oled.drawStr(8, 44, buf);

    // Range info
    snprintf(buf, sizeof(buf), "Lx%d Rx%d", joyCenterX - joyMinX, joyMaxX - joyCenterX);
    oled.drawStr(8, 58, buf);
    snprintf(buf, sizeof(buf), "Uy%d Dy%d", joyCenterY - joyMinY, joyMaxY - joyCenterY);
    oled.drawStr(72, 58, buf);

    oled.sendBuffer();
    delay(1200);
  }
}

// ========================================================================
// =================== ESP-NOW & COMUNICAZIONE ============================
// ========================================================================

void initEspNow() {
  if (_espNowInit) return;

  esp_err_t result = esp_now_init();
  if (result != ESP_OK) {
    Serial.printf("[JOY] ERRORE init ESP-NOW: %d\n", result);
    return;
  }

  _espNowInit = true;
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  uint8_t broadcastMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memcpy(peerInfo.peer_addr, broadcastMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  WiFi.macAddress(myMAC);
  Serial.printf("[JOY] ESP-NOW OK, MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                myMAC[0], myMAC[1], myMAC[2], myMAC[3], myMAC[4], myMAC[5]);
}

void sendDiscovery() {
  DiscoveryPacket req;
  memset(&req, 0, sizeof(req));
  req.magic      = DUAL_MAGIC;
  req.packetType = PKT_DISCOVERY_REQ;
  req.role       = ROLE_JOYSTICK;
  WiFi.macAddress(req.mac);

  uint8_t broadcastMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_send(broadcastMAC, (uint8_t*)&req, sizeof(req));
}

void sendJoystickPacket(uint8_t buttons) {
  if (!targetRegistered) return;

  JoystickPacket pkt;
  pkt.magic      = DUAL_MAGIC;
  pkt.packetType = PKT_JOYSTICK;
  pkt.buttons    = buttons;
  memcpy(pkt.mac, myMAC, 6);

  esp_now_send(targetMAC, (uint8_t*)&pkt, sizeof(pkt));
}

// ========================================================================
// =================== LED & OTA ==========================================
// ========================================================================

void updateLED() {
  unsigned long now = millis();

  if (otaInProgress) {
    if (now - ledLastToggle >= 100) {
      ledOn = !ledOn;
      digitalWrite(LED_STATUS_PIN, ledOn ? HIGH : LOW);
      ledLastToggle = now;
    }
    return;
  }

  switch (currentState) {
    case STATE_SEARCHING:
    case STATE_WIFI_SETUP:
      if (now - ledLastToggle >= LED_BLINK_FAST) {
        ledOn = !ledOn;
        digitalWrite(LED_STATUS_PIN, ledOn ? HIGH : LOW);
        ledLastToggle = now;
      }
      break;
    case STATE_CONNECTED:
      if (!ledOn) { ledOn = true; digitalWrite(LED_STATUS_PIN, HIGH); }
      break;
    case STATE_IDLE:
      if (ledOn) { ledOn = false; digitalWrite(LED_STATUS_PIN, LOW); }
      break;
  }
}

void initOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);

  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    otaProgress = 0;
    if (_espNowInit) { esp_now_deinit(); _espNowInit = false; }
  });

  ArduinoOTA.onEnd([]() {
    otaProgress = 100;
    otaInProgress = false;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    otaProgress = progress / (total / 100);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    initEspNow();
  });

  ArduinoOTA.begin();
  Serial.printf("[OTA] Pronto! %s.local (%s)\n", OTA_HOSTNAME, WiFi.localIP().toString().c_str());
}
