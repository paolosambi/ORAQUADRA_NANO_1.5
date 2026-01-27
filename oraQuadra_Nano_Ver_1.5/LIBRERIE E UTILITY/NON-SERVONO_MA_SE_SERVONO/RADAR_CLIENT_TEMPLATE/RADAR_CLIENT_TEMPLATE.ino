/*
 * =====================================================
 *        RADAR CLIENT TEMPLATE
 * =====================================================
 *
 * Template per creare nuovi device compatibili con
 * il sistema RADAR_SERVER_C3 auto-discovery.
 *
 * FUNZIONALITA':
 * - WiFiManager: configurazione WiFi senza hardcoding
 * - mDNS: auto-discovery dal radar server
 * - Riceve: presenza, luminosita, temperatura, umidita
 * - OTA: aggiornamenti over-the-air
 * - Web UI: controllo locale via browser
 * - Persistenza: salva impostazioni in flash
 *
 * CONFIGURAZIONE:
 * Modifica le define qui sotto per personalizzare il device
 *
 * COMPATIBILE CON:
 * - ESP32
 * - ESP32-S3
 * - ESP32-C3
 *
 * =====================================================
 */

// ==================== CONFIGURAZIONE DEVICE ====================

// Nome del device (apparira' nel radar server)
#define DEVICE_NAME         "NuovoDevice"

// Tipo device (lamp, display, clock, sensor, switch, etc.)
#define DEVICE_TYPE         "lamp"

// Nome rete WiFi AP per configurazione
#define WIFI_AP_NAME        "RadarDevice-Setup"

// Porta HTTP del device
#define HTTP_PORT           80

// Pin LED di esempio (cambia secondo il tuo hardware)
#define LED_PIN             -1       // LED integrato su molti ESP32
#define LED_PWM_CHANNEL     0
#define LED_PWM_FREQ        5000
#define LED_PWM_RESOLUTION  8       // 8 bit = 0-255

// Timeout portale configurazione WiFi (secondi)
#define WIFI_PORTAL_TIMEOUT 180

// ==================== LIBRERIE ====================

#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

// ==================== VARIABILI GLOBALI ====================

WebServer server(HTTP_PORT);
Preferences preferences;

// Dati ricevuti dal radar server
bool radarPresence = false;
int radarBrightness = 128;
float radarTemperature = 0.0;
float radarHumidity = 0.0;
unsigned long lastRadarUpdate = 0;

// Stato del device
bool deviceEnabled = true;          // On/Off generale
bool autoPresenceEnabled = true;    // Segui presenza radar
bool autoBrightnessEnabled = true;  // Segui luminosita radar
int manualBrightness = 128;         // Luminosita manuale (0-255)
int currentBrightness = 128;        // Luminosita attuale applicata

// Connessione radar
bool radarConnected = false;
unsigned long radarTimeout = 30000; // 30 secondi senza dati = disconnesso

// ==================== FUNZIONI LED ====================

void initLED() {
  // Configura PWM per il LED
  ledcSetup(LED_PWM_CHANNEL, LED_PWM_FREQ, LED_PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, LED_PWM_CHANNEL);
  ledcWrite(LED_PWM_CHANNEL, 0);
  Serial.println("LED inizializzato");
}

void setLED(int brightness) {
  // Limita il valore 0-255
  brightness = constrain(brightness, 0, 255);
  currentBrightness = brightness;
  ledcWrite(LED_PWM_CHANNEL, brightness);
}

void updateLED() {
  if (!deviceEnabled) {
    setLED(0);
    return;
  }

  if (autoPresenceEnabled && !radarPresence) {
    // Nessuna presenza - spegni
    setLED(0);
    return;
  }

  if (autoBrightnessEnabled && radarConnected) {
    // Usa luminosita dal radar
    setLED(radarBrightness);
  } else {
    // Usa luminosita manuale
    setLED(manualBrightness);
  }
}

// ==================== PERSISTENZA ====================

void saveSettings() {
  preferences.begin("device", false);
  preferences.putBool("enabled", deviceEnabled);
  preferences.putBool("autoPresence", autoPresenceEnabled);
  preferences.putBool("autoBright", autoBrightnessEnabled);
  preferences.putInt("manualBright", manualBrightness);
  preferences.end();
  Serial.println("Impostazioni salvate");
}

void loadSettings() {
  preferences.begin("device", true);
  deviceEnabled = preferences.getBool("enabled", true);
  autoPresenceEnabled = preferences.getBool("autoPresence", true);
  autoBrightnessEnabled = preferences.getBool("autoBright", true);
  manualBrightness = preferences.getInt("manualBright", 128);
  preferences.end();
  Serial.println("Impostazioni caricate");
  Serial.printf("  Enabled: %s\n", deviceEnabled ? "SI" : "NO");
  Serial.printf("  Auto Presence: %s\n", autoPresenceEnabled ? "SI" : "NO");
  Serial.printf("  Auto Brightness: %s\n", autoBrightnessEnabled ? "SI" : "NO");
  Serial.printf("  Manual Brightness: %d\n", manualBrightness);
}

// ==================== ENDPOINT RADAR ====================
// Questi endpoint ricevono i dati dal radar server

void setupRadarEndpoints() {

  // PRESENZA - riceve true/false
  server.on("/radar/presence", HTTP_GET, []() {
    String value = server.arg("value");
    bool newPresence = (value == "true");

    if (newPresence != radarPresence) {
      radarPresence = newPresence;
      Serial.printf("[RADAR] Presenza: %s\n", radarPresence ? "SI" : "NO");
      updateLED();
    }

    lastRadarUpdate = millis();
    radarConnected = true;
    server.send(200, "text/plain", "OK");
  });

  // LUMINOSITA - riceve 0-255
  server.on("/radar/brightness", HTTP_GET, []() {
    int value = server.arg("value").toInt();

    if (value != radarBrightness) {
      radarBrightness = constrain(value, 0, 255);
      Serial.printf("[RADAR] Luminosita: %d\n", radarBrightness);
      updateLED();
    }

    lastRadarUpdate = millis();
    radarConnected = true;
    server.send(200, "text/plain", "OK");
  });

  // TEMPERATURA - riceve valore float
  server.on("/radar/temperature", HTTP_GET, []() {
    radarTemperature = server.arg("value").toFloat();
    Serial.printf("[RADAR] Temperatura: %.1f C\n", radarTemperature);

    lastRadarUpdate = millis();
    radarConnected = true;
    server.send(200, "text/plain", "OK");
  });

  // UMIDITA - riceve valore float
  server.on("/radar/humidity", HTTP_GET, []() {
    radarHumidity = server.arg("value").toFloat();
    Serial.printf("[RADAR] Umidita: %.1f %%\n", radarHumidity);

    lastRadarUpdate = millis();
    radarConnected = true;
    server.send(200, "text/plain", "OK");
  });

  // TEST - per verificare connessione
  server.on("/radar/test", HTTP_GET, []() {
    Serial.println("[RADAR] Test ping ricevuto");
    lastRadarUpdate = millis();
    radarConnected = true;
    server.send(200, "text/plain", "OK");
  });
}

// ==================== WEB SERVER LOCALE ====================

void setupWebServer() {

  // Endpoint radar (riceve dati dal server)
  setupRadarEndpoints();

  // HOME PAGE
  server.on("/", HTTP_GET, []() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>)rawliteral" + String(DEVICE_NAME) + R"rawliteral(</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;padding:15px}
    .card{background:#16213e;padding:15px;border-radius:10px;margin-bottom:15px}
    h1{color:#e94560;margin-bottom:20px;text-align:center}
    h2{color:#0f3460;background:#e94560;padding:8px;border-radius:5px;margin:-15px -15px 15px -15px}
    .row{display:flex;justify-content:space-between;align-items:center;padding:8px 0;border-bottom:1px solid #0f3460}
    .row:last-child{border:none}
    label{color:#aaa}
    input[type=range]{width:150px}
    button{background:#e94560;color:#fff;border:none;padding:10px 20px;border-radius:5px;cursor:pointer;margin:5px}
    button:hover{background:#ff6b6b}
    button.off{background:#666}
    .status{padding:5px 10px;border-radius:3px;font-weight:bold}
    .status.on{background:#27ae60;color:#fff}
    .status.off{background:#c0392b;color:#fff}
    .value{font-size:1.2em;font-weight:bold;color:#e94560}
    .small{font-size:0.8em;color:#888}
  </style>
</head>
<body>
  <h1>)rawliteral" + String(DEVICE_NAME) + R"rawliteral(</h1>

  <div class="card">
    <h2>Stato Radar Server</h2>
    <div class="row">
      <label>Connessione:</label>
      <span id="radarStatus" class="status off">-</span>
    </div>
    <div class="row">
      <label>Presenza:</label>
      <span id="presence" class="value">-</span>
    </div>
    <div class="row">
      <label>Luminosita ricevuta:</label>
      <span id="radarBright" class="value">-</span>
    </div>
    <div class="row">
      <label>Temperatura:</label>
      <span id="temperature" class="value">-</span>
    </div>
    <div class="row">
      <label>Umidita:</label>
      <span id="humidity" class="value">-</span>
    </div>
  </div>

  <div class="card">
    <h2>Controlli Device</h2>
    <div class="row">
      <label>Device:</label>
      <button id="btnEnabled" onclick="toggle('enabled')">-</button>
    </div>
    <div class="row">
      <label>Segui Presenza:</label>
      <button id="btnPresence" onclick="toggle('autoPresence')">-</button>
    </div>
    <div class="row">
      <label>Segui Luminosita:</label>
      <button id="btnBrightness" onclick="toggle('autoBrightness')">-</button>
    </div>
    <div class="row">
      <label>Luminosita Manuale:</label>
      <input type="range" min="0" max="255" id="manualBright" onchange="setBrightness(this.value)">
      <span id="manualBrightVal">128</span>
    </div>
    <div class="row">
      <label>Luminosita Attuale:</label>
      <span id="currentBright" class="value">-</span>
    </div>
  </div>

  <div class="card">
    <h2>Sistema</h2>
    <div class="row">
      <label>IP:</label>
      <span class="value">)rawliteral" + WiFi.localIP().toString() + R"rawliteral(</span>
    </div>
    <div class="row">
      <label>WiFi:</label>
      <span>)rawliteral" + WiFi.SSID() + R"rawliteral(</span>
    </div>
    <div class="row">
      <label>RSSI:</label>
      <span id="rssi">-</span> dBm
    </div>
    <div class="row">
      <button onclick="if(confirm('Reset WiFi?'))fetch('/resetWifi')">Reset WiFi</button>
      <button onclick="if(confirm('Riavvio?'))fetch('/reboot')">Riavvia</button>
    </div>
  </div>

  <p class="small" style="text-align:center;margin-top:10px">
    Tipo: )rawliteral" + String(DEVICE_TYPE) + R"rawliteral( | Porta: )rawliteral" + String(HTTP_PORT) + R"rawliteral(
  </p>

<script>
function api(url){return fetch(url).then(r=>r.json())}

function updateStatus(){
  api('/api/status').then(d=>{
    document.getElementById('radarStatus').textContent=d.radarConnected?'CONNESSO':'DISCONNESSO';
    document.getElementById('radarStatus').className='status '+(d.radarConnected?'on':'off');
    document.getElementById('presence').textContent=d.radarPresence?'SI':'NO';
    document.getElementById('radarBright').textContent=d.radarBrightness;
    document.getElementById('temperature').textContent=d.temperature.toFixed(1)+' C';
    document.getElementById('humidity').textContent=d.humidity.toFixed(1)+' %';
    document.getElementById('currentBright').textContent=d.currentBrightness;
    document.getElementById('rssi').textContent=d.rssi;

    document.getElementById('btnEnabled').textContent=d.enabled?'ON':'OFF';
    document.getElementById('btnEnabled').className=d.enabled?'':'off';
    document.getElementById('btnPresence').textContent=d.autoPresence?'ON':'OFF';
    document.getElementById('btnPresence').className=d.autoPresence?'':'off';
    document.getElementById('btnBrightness').textContent=d.autoBrightness?'ON':'OFF';
    document.getElementById('btnBrightness').className=d.autoBrightness?'':'off';
    document.getElementById('manualBright').value=d.manualBrightness;
    document.getElementById('manualBrightVal').textContent=d.manualBrightness;
  });
}

function toggle(what){
  api('/api/toggle?param='+what).then(d=>updateStatus());
}

function setBrightness(val){
  document.getElementById('manualBrightVal').textContent=val;
  api('/api/brightness?value='+val).then(d=>updateStatus());
}

updateStatus();
setInterval(updateStatus,2000);
</script>
</body>
</html>
)rawliteral";
    server.send(200, "text/html", html);
  });

  // API STATUS
  server.on("/api/status", HTTP_GET, []() {
    // Controlla timeout radar
    if (millis() - lastRadarUpdate > radarTimeout) {
      radarConnected = false;
    }

    String json = "{";
    json += "\"name\":\"" + String(DEVICE_NAME) + "\",";  // Nome per discovery
    json += "\"type\":\"" + String(DEVICE_TYPE) + "\",";  // Tipo device
    json += "\"port\":" + String(HTTP_PORT) + ",";
    json += "\"radarConnected\":" + String(radarConnected ? "true" : "false") + ",";
    json += "\"radarPresence\":" + String(radarPresence ? "true" : "false") + ",";
    json += "\"radarBrightness\":" + String(radarBrightness) + ",";
    json += "\"temperature\":" + String(radarTemperature, 1) + ",";
    json += "\"humidity\":" + String(radarHumidity, 1) + ",";
    json += "\"enabled\":" + String(deviceEnabled ? "true" : "false") + ",";
    json += "\"autoPresence\":" + String(autoPresenceEnabled ? "true" : "false") + ",";
    json += "\"autoBrightness\":" + String(autoBrightnessEnabled ? "true" : "false") + ",";
    json += "\"manualBrightness\":" + String(manualBrightness) + ",";
    json += "\"currentBrightness\":" + String(currentBrightness) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI());
    json += "}";
    server.send(200, "application/json", json);
  });

  // API TOGGLE
  server.on("/api/toggle", HTTP_GET, []() {
    String param = server.arg("param");

    if (param == "enabled") {
      deviceEnabled = !deviceEnabled;
    } else if (param == "autoPresence") {
      autoPresenceEnabled = !autoPresenceEnabled;
    } else if (param == "autoBrightness") {
      autoBrightnessEnabled = !autoBrightnessEnabled;
    }

    updateLED();
    saveSettings();
    server.send(200, "application/json", "{\"ok\":true}");
  });

  // API BRIGHTNESS MANUALE
  server.on("/api/brightness", HTTP_GET, []() {
    if (server.hasArg("value")) {
      manualBrightness = constrain(server.arg("value").toInt(), 0, 255);
      updateLED();
      saveSettings();
    }
    server.send(200, "application/json", "{\"brightness\":" + String(manualBrightness) + "}");
  });

  // RESET WIFI
  server.on("/resetWifi", HTTP_GET, []() {
    server.send(200, "text/plain", "Reset WiFi...");
    delay(1000);
    WiFiManager wm;
    wm.resetSettings();
    ESP.restart();
  });

  // REBOOT
  server.on("/reboot", HTTP_GET, []() {
    server.send(200, "text/plain", "Riavvio...");
    delay(1000);
    ESP.restart();
  });

  server.begin();
  Serial.printf("Web server avviato su porta %d\n", HTTP_PORT);
}

// ==================== WIFI ====================

void setupWiFi() {
  Serial.println("\n========================================");
  Serial.println("  CONFIGURAZIONE WIFI");
  Serial.println("========================================");

  WiFiManager wifiManager;

  // Timeout portale
  wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);

  // Callback AP mode
  wifiManager.setAPCallback([](WiFiManager *myWiFiManager) {
    Serial.println("\n!!! MODALITA' CONFIGURAZIONE !!!");
    Serial.printf("Connettiti a: %s\n", WIFI_AP_NAME);
    Serial.println("Poi vai su: http://192.168.4.1");
  });

  // Connetti o avvia portale
  if (wifiManager.autoConnect(WIFI_AP_NAME)) {
    Serial.println("\nWiFi CONNESSO!");
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());

    // Disabilita power saving
    WiFi.setSleep(WIFI_PS_NONE);
    WiFi.setAutoReconnect(true);

  } else {
    Serial.println("\nWiFi TIMEOUT! Riavvio...");
    delay(3000);
    ESP.restart();
  }
}

// ==================== mDNS ====================

void setupMDNS() {
  // Crea hostname dal nome device (minuscolo, senza spazi)
  String hostname = String(DEVICE_NAME);
  hostname.toLowerCase();
  hostname.replace(" ", "");
  hostname.replace("_", "");

  if (MDNS.begin(hostname.c_str())) {
    Serial.printf("mDNS: %s.local\n", hostname.c_str());

    // Annuncia servizio per auto-discovery
    MDNS.addService("radardevice", "tcp", HTTP_PORT);
    MDNS.addServiceTxt("radardevice", "tcp", "name", DEVICE_NAME);
    MDNS.addServiceTxt("radardevice", "tcp", "type", DEVICE_TYPE);
    MDNS.addServiceTxt("radardevice", "tcp", "port", String(HTTP_PORT).c_str());

    Serial.println("mDNS: Servizio _radardevice._tcp annunciato");
    Serial.println("Il radar server puo' ora trovare questo device!");
  } else {
    Serial.println("mDNS: ERRORE inizializzazione!");
  }
}

// ==================== OTA ====================

void setupOTA() {
  ArduinoOTA.setHostname(DEVICE_NAME);

  ArduinoOTA.onStart([]() {
    Serial.println("OTA: Avvio aggiornamento...");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA: Completato!");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Errore[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("OTA pronto");
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║       RADAR CLIENT TEMPLATE            ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.printf("║  Device: %-29s ║\n", DEVICE_NAME);
  Serial.printf("║  Tipo:   %-29s ║\n", DEVICE_TYPE);
  Serial.printf("║  Porta:  %-29d ║\n", HTTP_PORT);
  Serial.println("╚════════════════════════════════════════╝");

  // Inizializza LED
  initLED();

  // Carica impostazioni salvate
  loadSettings();

  // WiFi
  setupWiFi();

  // mDNS per auto-discovery
  setupMDNS();

  // OTA
  setupOTA();

  // Web Server
  setupWebServer();

  // Applica stato iniziale
  updateLED();

  Serial.println("\n========================================");
  Serial.println("         SISTEMA PRONTO");
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.println("In attesa di discovery dal radar server...");
  Serial.println("========================================\n");
}

// ==================== LOOP ====================

void loop() {
  // OTA
  ArduinoOTA.handle();

  // Web Server
  server.handleClient();

  // Controlla timeout connessione radar
  static bool wasConnected = false;
  if (millis() - lastRadarUpdate > radarTimeout) {
    if (radarConnected) {
      radarConnected = false;
      Serial.println("[RADAR] Connessione persa - timeout");
      updateLED();  // Passa a modalita' manuale
    }
  }

  // Log stato connessione cambio
  if (radarConnected != wasConnected) {
    if (radarConnected) {
      Serial.println("[RADAR] Connesso al server!");
    }
    wasConnected = radarConnected;
  }

  // Riconnessione WiFi
  static unsigned long lastWifiCheck = 0;
  if (millis() - lastWifiCheck > 10000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnesso, tentativo riconnessione...");
      WiFi.reconnect();
    }
    lastWifiCheck = millis();
  }
}
