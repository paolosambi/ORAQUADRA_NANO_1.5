// ================== WEB SERVER PER RICEZIONE NOTIFICHE RADAR REMOTO ==================
// Questo modulo gestisce le notifiche inviate dal RADAR SERVER (ESP32-C3 con LD2412)
// Il radar server invia notifiche HTTP quando cambia lo stato di presenza o luminosità
//
// FUNZIONALITA':
// 1. Registrazione automatica sul radar server all'avvio
// 2. Ricezione notifiche presenza e luminosità
// 3. Fallback su radar locale se radar server non disponibile
//
// Endpoint ricevuti dal radar server:
// - GET /radar/presence?value=true/false  -> Cambia stato presenza
// - GET /radar/brightness?value=XXX       -> Imposta luminosità display (0-255)
// - GET /radar/test?value=ping            -> Test connessione

// ================== VARIABILI RADAR REMOTO ==================
// IP del radar server (default: 0.0.0.0 = disabilitato)
uint8_t radarServerIP[4] = {0, 0, 0, 0};
bool radarServerEnabled = false;           // Flag per abilitare radar server remoto
bool radarServerConnected = false;         // Flag connessione radar server
unsigned long lastRadarServerCheck = 0;    // Timestamp ultimo tentativo connessione
unsigned long lastRadarRemoteUpdate = 0;   // Timestamp ultima ricezione dati
#define RADAR_SERVER_CHECK_INTERVAL 15000  // Intervallo check connessione (15 sec)
#define RADAR_SERVER_TIMEOUT 30000         // Timeout connessione (30 sec) - server sync ogni 10 sec

// Stato ricevuto dal radar remoto
bool radarRemotePresence = true;           // Stato presenza dal radar remoto
uint8_t radarRemoteBrightness = 128;       // Luminosità dal radar remoto (0-255)
float radarRemoteTemperature = 0.0;        // Temperatura dal radar server (BME280)
float radarRemoteHumidity = 0.0;           // Umidità dal radar server (BME280)

// ================== VARIABILI ALLARME GAS ==================
bool gasAlarmActive = false;               // Allarme gas attivo
String gasAlarmCode = "";                  // Codice gas (CO, CH4, etc.)
String gasAlarmName = "";                  // Nome gas (MONOSSIDO, METANO, etc.)
float gasAlarmValue = 0.0;                 // Valore PPM
unsigned long lastGasBeep = 0;             // Timestamp ultimo beep
#define GAS_BEEP_INTERVAL 1500             // Intervallo tra beep (1.5 sec)
volatile bool gasAlarmNeedsDraw = false;    // Flag: disegnare schermata allarme dal loop
volatile bool gasAlarmNeedsRedraw = false;  // Flag: ridisegnare orologio dopo fine allarme

// ================== FUNZIONI EEPROM RADAR SERVER ==================

// Carica IP radar server da EEPROM
void loadRadarServerConfig() {
  // Prima verifica il marker di validità
  uint8_t validMarker = EEPROM.read(EEPROM_RADAR_SERVER_VALID);

  Serial.printf("[RADAR REMOTE] EEPROM validMarker: 0x%02X (atteso 0x%02X)\n",
                validMarker, EEPROM_RADAR_SERVER_VALID_VALUE);

  if (validMarker != EEPROM_RADAR_SERVER_VALID_VALUE) {
    // Config non valida o mai scritta - usa default
    Serial.println("[RADAR REMOTE] Config EEPROM non valida, uso default");
    radarServerIP[0] = 0;
    radarServerIP[1] = 0;
    radarServerIP[2] = 0;
    radarServerIP[3] = 0;
    radarServerEnabled = false;
    return;
  }

  // Marker valido, leggi configurazione
  radarServerIP[0] = EEPROM.read(EEPROM_RADAR_SERVER_IP_ADDR);
  radarServerIP[1] = EEPROM.read(EEPROM_RADAR_SERVER_IP_ADDR + 1);
  radarServerIP[2] = EEPROM.read(EEPROM_RADAR_SERVER_IP_ADDR + 2);
  radarServerIP[3] = EEPROM.read(EEPROM_RADAR_SERVER_IP_ADDR + 3);
  uint8_t enabledVal = EEPROM.read(EEPROM_RADAR_SERVER_ENABLED);
  radarServerEnabled = (enabledVal == 1);

  Serial.printf("[RADAR REMOTE] Config caricata - IP: %d.%d.%d.%d, Enabled: %s\n",
                radarServerIP[0], radarServerIP[1], radarServerIP[2], radarServerIP[3],
                radarServerEnabled ? "SI" : "NO");
}

// Salva IP radar server su EEPROM
void saveRadarServerConfig() {
  Serial.printf("[RADAR REMOTE] Salvataggio EEPROM - IP: %d.%d.%d.%d, Enabled: %d\n",
                radarServerIP[0], radarServerIP[1], radarServerIP[2], radarServerIP[3],
                radarServerEnabled ? 1 : 0);

  // Scrivi IP
  EEPROM.write(EEPROM_RADAR_SERVER_IP_ADDR, radarServerIP[0]);
  EEPROM.write(EEPROM_RADAR_SERVER_IP_ADDR + 1, radarServerIP[1]);
  EEPROM.write(EEPROM_RADAR_SERVER_IP_ADDR + 2, radarServerIP[2]);
  EEPROM.write(EEPROM_RADAR_SERVER_IP_ADDR + 3, radarServerIP[3]);
  // Scrivi enabled
  EEPROM.write(EEPROM_RADAR_SERVER_ENABLED, radarServerEnabled ? 1 : 0);
  // Scrivi marker di validità
  EEPROM.write(EEPROM_RADAR_SERVER_VALID, EEPROM_RADAR_SERVER_VALID_VALUE);

  bool ok = EEPROM.commit();
  Serial.printf("[RADAR REMOTE] EEPROM.commit() = %s\n", ok ? "OK" : "ERRORE");

  // Verifica lettura
  uint8_t v0 = EEPROM.read(EEPROM_RADAR_SERVER_IP_ADDR);
  uint8_t v1 = EEPROM.read(EEPROM_RADAR_SERVER_IP_ADDR + 1);
  uint8_t v2 = EEPROM.read(EEPROM_RADAR_SERVER_IP_ADDR + 2);
  uint8_t v3 = EEPROM.read(EEPROM_RADAR_SERVER_IP_ADDR + 3);
  uint8_t marker = EEPROM.read(EEPROM_RADAR_SERVER_VALID);
  Serial.printf("[RADAR REMOTE] Verifica EEPROM: %d.%d.%d.%d marker=0x%02X\n", v0, v1, v2, v3, marker);
}

// Converte IP array in stringa
String radarServerIPString() {
  return String(radarServerIP[0]) + "." + String(radarServerIP[1]) + "." +
         String(radarServerIP[2]) + "." + String(radarServerIP[3]);
}

// ================== REGISTRAZIONE SUL RADAR SERVER ==================

// Tenta di registrare questo client sul radar server
bool registerOnRadarServer() {
  if (!radarServerEnabled || WiFi.status() != WL_CONNECTED) {
    return false;
  }

  // Verifica IP valido
  if (radarServerIP[0] == 0 && radarServerIP[1] == 0 &&
      radarServerIP[2] == 0 && radarServerIP[3] == 0) {
    Serial.println("[RADAR REMOTE] IP radar server non configurato");
    return false;
  }

  HTTPClient http;
  // Usa endpoint /api/devices/add del radar client
  String url = "http://" + radarServerIPString() + "/api/devices/add";
  url += "?ip=" + WiFi.localIP().toString();
  url += "&port=8080";  // oraQuadraNano usa porta 8080
  extern String deviceHostname;
  url += "&name=" + deviceHostname;  // Nome univoco per device (es. "oraquadra-abc123")
  url += "&host=" + deviceHostname;  // hostname mDNS con device ID per risolvere IP anche se cambia (DHCP)

  Serial.printf("[RADAR REMOTE] Registrazione su %s...\n", url.c_str());

  http.begin(url);
  http.setTimeout(5000);  // Timeout 5 secondi
  Serial.println("[RADAR REMOTE] Invio richiesta registrazione...");
  int httpCode = http.GET();
  http.end();

  if (httpCode != 200) {
    Serial.printf("[RADAR REMOTE] Registrazione FALLITA (HTTP %d)\n", httpCode);
    radarServerConnected = false;
    return false;
  }

  Serial.println("[RADAR REMOTE] Registrazione OK, leggo stato...");

  // Ora leggi lo stato corrente da /api/status
  HTTPClient http2;
  String statusUrl = "http://" + radarServerIPString() + "/api/status";
  http2.begin(statusUrl);
  http2.setTimeout(5000);  // Timeout 5 secondi
  int statusCode = http2.GET();

  if (statusCode == 200) {
    String response = http2.getString();
    Serial.printf("[RADAR REMOTE] Status ricevuto: %d bytes\n", response.length());

    radarServerConnected = true;
    lastRadarRemoteUpdate = millis();

    // IMPORTANTE: Leggere PRIMA la presenza, POI la luminosità
    // per evitare race condition (brightness applicata con presenza stale)

    // Estrai presence (PRIMA!)
    int presIdx = response.indexOf("\"presence\":");
    if (presIdx > 0) {
      int presValStart = presIdx + 11;
      String presVal = response.substring(presValStart, presValStart + 6);
      radarRemotePresence = (presVal.indexOf("true") >= 0);
      Serial.printf("[RADAR REMOTE] Presenza: %s\n", radarRemotePresence ? "SI" : "NO");
    }

    // Estrai ambientLight (luminosita) - DOPO la presenza
    int briIdx = response.indexOf("\"ambientLight\":");
    if (briIdx > 0) {
      int briStart = briIdx + 15;
      int briEnd = response.indexOf(",", briStart);
      if (briEnd > briStart) {
        String briStr = response.substring(briStart, briEnd);
        int briVal = briStr.toInt();
        radarRemoteBrightness = constrain(briVal, 0, 255);
        Serial.printf("[RADAR REMOTE] Luminosita: %d\n", radarRemoteBrightness);

        if (radarBrightnessControl && radarRemotePresence) {
          uint8_t mappedBrightness = map(radarRemoteBrightness, 0, 255, radarBrightnessMin, radarBrightnessMax);
          ledcWrite(PWM_CHANNEL, mappedBrightness);
          lastAppliedBrightness = mappedBrightness;
        } else if (!radarRemotePresence) {
          // Nessuna presenza - forza display spento
          ledcWrite(PWM_CHANNEL, 0);
          lastAppliedBrightness = 0;
        }
      }
    }

    // Estrai temperatura
    int tempIdx = response.indexOf("\"temperature\":");
    if (tempIdx > 0) {
      int tempStart = tempIdx + 14;
      int tempEnd = response.indexOf(",", tempStart);
      if (tempEnd > tempStart) {
        radarRemoteTemperature = response.substring(tempStart, tempEnd).toFloat();
        Serial.printf("[RADAR REMOTE] Temperatura: %.1f\n", radarRemoteTemperature);
      }
    }

    // Estrai umidita
    int humIdx = response.indexOf("\"humidity\":");
    if (humIdx > 0) {
      int humStart = humIdx + 11;
      int humEnd = response.indexOf(",", humStart);
      if (humEnd > humStart) {
        radarRemoteHumidity = response.substring(humStart, humEnd).toFloat();
        Serial.printf("[RADAR REMOTE] Umidita: %.1f\n", radarRemoteHumidity);
      }
    }

    http2.end();
    return true;
  } else {
    Serial.printf("[RADAR REMOTE] Status FALLITO (HTTP %d)\n", statusCode);
    http2.end();
    return false;
  }
}

// Verifica connessione con radar server (ping)
bool checkRadarServerConnection() {
  if (!radarServerEnabled || WiFi.status() != WL_CONNECTED) {
    return false;
  }

  HTTPClient http;
  String url = "http://" + radarServerIPString() + "/api/status";

  http.begin(url);
  http.setTimeout(2000);
  int httpCode = http.GET();
  http.end();  // IMPORTANTE: chiudere PRIMA del return per evitare memory leak

  if (httpCode == 200) {
    radarServerConnected = true;
    return true;
  } else {
    radarServerConnected = false;
    return false;
  }
}

// ================== GESTIONE FALLBACK RADAR LOCALE ==================

// Verifica se usare radar remoto o locale
bool useRemoteRadar() {
  // Usa radar remoto se abilitato, connesso e dati recenti
  if (radarServerEnabled && radarServerConnected) {
    if ((millis() - lastRadarRemoteUpdate) < RADAR_SERVER_TIMEOUT) {
      return true;
    }
  }
  return false;
}

// ================== HANDLER ENDPOINT RADAR ==================

// GET /radar/presence?value=true/false
void handleRadarPresence(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    String value = request->getParam("value")->value();
    bool newPresence = (value == "true" || value == "1");

    radarRemotePresence = newPresence;
    radarServerConnected = true;
    lastRadarRemoteUpdate = millis();

    Serial.printf("[RADAR REMOTE] Presenza: %s\n", newPresence ? "SI" : "NO");

    // Applica effetto sul display
    if (newPresence) {
      // Presenza rilevata - riaccendi display
      if (radarBrightnessControl) {
        uint8_t wakeupBrightness = map(radarRemoteBrightness, 0, 255, radarBrightnessMin, radarBrightnessMax);
        ledcWrite(PWM_CHANNEL, wakeupBrightness);
        lastAppliedBrightness = wakeupBrightness;
        Serial.printf("[RADAR REMOTE] Display ON - Lum: %d\n", wakeupBrightness);
      } else {
        // Usa luminosità manuale giorno/notte
        uint8_t wakeupBrightness = checkIsNightTime(currentHour, currentMinute) ? brightnessNight : brightnessDay;
        ledcWrite(PWM_CHANNEL, wakeupBrightness);
        lastAppliedBrightness = wakeupBrightness;
        Serial.printf("[RADAR REMOTE] Display ON - Lum manuale: %d\n", wakeupBrightness);
      }
    } else {
      // Nessuna presenza - spegni display (radar server ha priorità)
      ledcWrite(PWM_CHANNEL, 0);
      lastAppliedBrightness = 0;
      Serial.println("[RADAR REMOTE] Display OFF - Nessuna presenza");
    }

    request->send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    request->send(400, "application/json", "{\"error\":\"missing value\"}");
  }
}

// GET /radar/brightness?value=XXX
void handleRadarBrightness(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    int value = request->getParam("value")->value().toInt();
    value = constrain(value, 0, 255);

    radarRemoteBrightness = value;
    radarServerConnected = true;
    lastRadarRemoteUpdate = millis();

    // Applica luminosità SEMPRE se c'è presenza (più reattivo)
    if (radarRemotePresence) {
      // Mappa 0-255 -> radarBrightnessMin-radarBrightnessMax
      uint8_t mappedBrightness = map(value, 0, 255, radarBrightnessMin, radarBrightnessMax);
      ledcWrite(PWM_CHANNEL, mappedBrightness);
      lastAppliedBrightness = mappedBrightness;
      Serial.printf("[RADAR REMOTE] Luce RAW=%d -> PWM=%d\n", value, mappedBrightness);
    }

    request->send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    request->send(400, "application/json", "{\"error\":\"missing value\"}");
  }
}

// GET /radar/test?value=ping
void handleRadarTest(AsyncWebServerRequest *request) {
  radarServerConnected = true;
  lastRadarRemoteUpdate = millis();

  extern String deviceHostname;
  String json = "{\"status\":\"ok\",\"device\":\"oraQuadraNano\",\"host\":\"" + deviceHostname + "\",\"mac\":\"" + WiFi.macAddress() + "\"}";
  request->send(200, "application/json", json);
}

// GET /radar/temperature?value=XX.X
void handleRadarTemperature(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    String value = request->getParam("value")->value();
    radarRemoteTemperature = value.toFloat();
    radarServerConnected = true;
    lastRadarRemoteUpdate = millis();
    Serial.printf("[RADAR REMOTE] Temperatura: %.1f C\n", radarRemoteTemperature);
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    request->send(400, "application/json", "{\"error\":\"missing value\"}");
  }
}

// GET /radar/humidity?value=XX.X
void handleRadarHumidity(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    String value = request->getParam("value")->value();
    radarRemoteHumidity = value.toFloat();
    radarServerConnected = true;
    lastRadarRemoteUpdate = millis();
    Serial.printf("[RADAR REMOTE] Umidita: %.1f%%\n", radarRemoteHumidity);
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    request->send(400, "application/json", "{\"error\":\"missing value\"}");
  }
}

// GET /api/status - Per discovery dal radar server
void handleApiStatus(AsyncWebServerRequest *request) {
  String json = "{";
  json += "\"name\":\"OraQuadraNano\",";
  json += "\"type\":\"clock\",";
  json += "\"port\":8080,";
  json += "\"radarConnected\":" + String(radarServerConnected ? "true" : "false") + ",";
  json += "\"radarPresence\":" + String(radarRemotePresence ? "true" : "false") + ",";
  json += "\"radarBrightness\":" + String(radarRemoteBrightness) + ",";
  json += "\"temperature\":" + String(radarRemoteTemperature, 1) + ",";
  json += "\"humidity\":" + String(radarRemoteHumidity, 1);
  json += "}";
  request->send(200, "application/json", json);
}

// GET /radar/status - Stato completo radar (locale + remoto)
void handleRadarStatus(AsyncWebServerRequest *request) {
  bool usingRemote = useRemoteRadar();
  unsigned long timeSinceUpdate = millis() - lastRadarRemoteUpdate;

  String json = "{";
  // Stato radar remoto
  json += "\"remote\":{";
  json += "\"enabled\":" + String(radarServerEnabled ? "true" : "false") + ",";
  json += "\"connected\":" + String(radarServerConnected ? "true" : "false") + ",";
  json += "\"serverIP\":\"" + radarServerIPString() + "\",";
  json += "\"presence\":" + String(radarRemotePresence ? "true" : "false") + ",";
  json += "\"brightness\":" + String(radarRemoteBrightness) + ",";
  json += "\"temperature\":" + String(radarRemoteTemperature, 1) + ",";
  json += "\"humidity\":" + String(radarRemoteHumidity, 1) + ",";
  json += "\"lastUpdate\":" + String(lastRadarRemoteUpdate) + ",";
  json += "\"timeSinceUpdate\":" + String(timeSinceUpdate);
  json += "},";
  // Stato radar locale
  json += "\"local\":{";
  json += "\"available\":" + String(radarAvailable ? "true" : "false") + ",";
  json += "\"presence\":" + String(presenceDetected ? "true" : "false") + ",";
  json += "\"lightLevel\":" + String(lastRadarLightLevel);
  json += "},";
  // Stato attivo
  json += "\"usingRemote\":" + String(usingRemote ? "true" : "false") + ",";
  json += "\"radarBrightnessControl\":" + String(radarBrightnessControl ? "true" : "false") + ",";
  json += "\"radarBrightnessMin\":" + String(radarBrightnessMin) + ",";
  json += "\"radarBrightnessMax\":" + String(radarBrightnessMax) + ",";
  json += "\"currentBrightness\":" + String(lastAppliedBrightness);
  json += "}";

  request->send(200, "application/json", json);
}

// GET /radar/config - Ottiene configurazione radar server
void handleRadarConfigGet(AsyncWebServerRequest *request) {
  // Se IP è 0.0.0.0, restituisci stringa vuota
  bool hasIP = (radarServerIP[0] != 0 || radarServerIP[1] != 0 ||
                radarServerIP[2] != 0 || radarServerIP[3] != 0);
  String ipStr = hasIP ? radarServerIPString() : "";

  String json = "{";
  json += "\"enabled\":" + String(radarServerEnabled ? "true" : "false") + ",";
  json += "\"serverIP\":\"" + ipStr + "\",";
  json += "\"connected\":" + String(radarServerConnected ? "true" : "false");
  json += "}";

  Serial.printf("[RADAR REMOTE] Config richiesta - IP: %s, Enabled: %s\n",
                ipStr.c_str(), radarServerEnabled ? "SI" : "NO");

  request->send(200, "application/json", json);
}

// GET /radar/config/set?ip=X.X.X.X&enabled=1
void handleRadarConfigSet(AsyncWebServerRequest *request) {
  Serial.println("[RADAR REMOTE] === handleRadarConfigSet chiamato ===");

  bool changed = false;

  if (request->hasParam("ip")) {
    String ipStr = request->getParam("ip")->value();
    Serial.printf("[RADAR REMOTE] IP ricevuto: '%s'\n", ipStr.c_str());

    // Parse IP string
    int parts[4] = {0, 0, 0, 0};
    int parsed = sscanf(ipStr.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]);
    Serial.printf("[RADAR REMOTE] sscanf parsed %d parts: %d.%d.%d.%d\n", parsed, parts[0], parts[1], parts[2], parts[3]);

    if (parsed == 4 && parts[0] > 0 && parts[0] < 255) {
      radarServerIP[0] = parts[0];
      radarServerIP[1] = parts[1];
      radarServerIP[2] = parts[2];
      radarServerIP[3] = parts[3];
      changed = true;
      Serial.printf("[RADAR REMOTE] IP impostato: %d.%d.%d.%d\n",
                    radarServerIP[0], radarServerIP[1], radarServerIP[2], radarServerIP[3]);
    } else {
      Serial.println("[RADAR REMOTE] IP non valido!");
    }
  }

  if (request->hasParam("enabled")) {
    int enabledVal = request->getParam("enabled")->value().toInt();
    radarServerEnabled = (enabledVal == 1);
    changed = true;
    Serial.printf("[RADAR REMOTE] Enabled ricevuto: %d -> %s\n", enabledVal, radarServerEnabled ? "SI" : "NO");
  }

  if (changed) {
    Serial.println("[RADAR REMOTE] Salvataggio configurazione...");
    saveRadarServerConfig();

    // Se abilitato e IP valido, tenta registrazione
    if (radarServerEnabled && radarServerIP[0] > 0) {
      Serial.println("[RADAR REMOTE] Tentativo registrazione...");
      bool regOk = registerOnRadarServer();
      Serial.printf("[RADAR REMOTE] Registrazione: %s, radarServerConnected=%s\n",
                    regOk ? "OK" : "FALLITA",
                    radarServerConnected ? "true" : "false");
    } else {
      // Se disabilitato, resetta stato connessione
      radarServerConnected = false;
      Serial.println("[RADAR REMOTE] Radar server disabilitato, connessione reset");
    }
  }

  Serial.printf("[RADAR REMOTE] Stato finale - enabled=%s, connected=%s\n",
                radarServerEnabled ? "true" : "false",
                radarServerConnected ? "true" : "false");

  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"enabled\":" + String(radarServerEnabled ? "true" : "false") + ",";
  json += "\"serverIP\":\"" + radarServerIPString() + "\",";
  json += "\"connected\":" + String(radarServerConnected ? "true" : "false");
  json += "}";

  Serial.printf("[RADAR REMOTE] Risposta JSON: %s\n", json.c_str());
  request->send(200, "application/json", json);
}

// ================== FUNZIONE UPDATE PERIODICA ==================

// Chiamare nel loop principale per gestire riconnessione
void updateRadarServer() {
  if (!radarServerEnabled || WiFi.status() != WL_CONNECTED) {
    return;
  }

  unsigned long currentMillis = millis();

  // Check periodico connessione
  if (currentMillis - lastRadarServerCheck > RADAR_SERVER_CHECK_INTERVAL) {
    lastRadarServerCheck = currentMillis;

    // Se non connesso o timeout, tenta riconnessione
    unsigned long timeSinceUpdate = currentMillis - lastRadarRemoteUpdate;
    if (!radarServerConnected || timeSinceUpdate > RADAR_SERVER_TIMEOUT) {
      Serial.println("[RADAR REMOTE] Tentativo riconnessione...");
      if (registerOnRadarServer()) {
        Serial.println("[RADAR REMOTE] Riconnesso con successo!");
      } else {
        Serial.println("[RADAR REMOTE] Riconnessione fallita, uso radar locale");
      }
    }
  }
}

// ================== FUNZIONI ALLARME GAS ==================

// Mostra schermata allarme gas (display rosso con nome gas)
void showGasAlarmScreen() {
  extern Arduino_GFX *gfx;
  if (!gfx) return;

  // Sfondo rosso
  gfx->fillScreen(0xF800);  // Rosso

  // Testo ALLARME GAS
  gfx->setTextColor(0xFFFF);  // Bianco
  gfx->setFont(u8g2_font_maniac_te);

  // "ALLARME GAS" centrato
  gfx->setCursor(80, 150);
  gfx->print("ALLARME GAS!");

  // Nome del gas (es. METANO)
  gfx->setCursor(140, 250);
  gfx->print(gasAlarmName);

  // Valore PPM
  gfx->setFont(u8g2_font_crox5hb_tr);
  gfx->setCursor(160, 320);
  char valueStr[32];
  sprintf(valueStr, "%.1f PPM", gasAlarmValue);
  gfx->print(valueStr);

  // Istruzioni
  gfx->setFont(u8g2_font_helvB08_tr);
  gfx->setCursor(100, 420);
  gfx->print("Aerare il locale! Verificare perdite!");
  gfx->setCursor(120, 450);
  gfx->print("Reset da pannello RADAR");

  Serial.printf("[GAS ALARM] Schermata allarme: %s (%s) = %.1f PPM\n",
                gasAlarmName.c_str(), gasAlarmCode.c_str(), gasAlarmValue);
}

// Suona beep allarme (chiamare nel loop)
void updateGasAlarmBeep() {
  if (!gasAlarmActive) return;

  unsigned long now = millis();
  if (now - lastGasBeep >= GAS_BEEP_INTERVAL) {
    lastGasBeep = now;

    #ifdef AUDIO
    // Riproduci beep.mp3 da LittleFS
    extern bool playLocalMP3(const char* filename);
    playLocalMP3("beep.mp3");
    #endif

    Serial.println("[GAS ALARM] BEEP!");
  }
}

// Handler: GET /radar/gas_alarm?gas=CO&name=MONOSSIDO&value=150.5
void handleRadarGasAlarm(AsyncWebServerRequest *request) {
  // Estrai parametri
  if (request->hasParam("gas")) {
    gasAlarmCode = request->getParam("gas")->value();
  }
  if (request->hasParam("name")) {
    gasAlarmName = request->getParam("name")->value();
  }
  if (request->hasParam("value")) {
    gasAlarmValue = request->getParam("value")->value().toFloat();
  }

  // Attiva allarme (solo flag - il display viene aggiornato dal loop principale)
  gasAlarmActive = true;
  gasAlarmNeedsDraw = true;  // Il loop principale disegnera la schermata
  lastGasBeep = 0;  // Forza beep immediato

  Serial.printf("[GAS ALARM] >>> ALLARME RICEVUTO: %s (%s) = %.1f PPM <<<\n",
                gasAlarmName.c_str(), gasAlarmCode.c_str(), gasAlarmValue);

  // NON chiamare showGasAlarmScreen() qui! AsyncWebServer gira in un task separato
  // e le operazioni SPI sul display causano crash. Il loop principale gestira il draw.

  request->send(200, "application/json", "{\"status\":\"ok\",\"alarm\":\"active\"}");
}

// Handler: GET /radar/gas_alarm_end
void handleRadarGasAlarmEnd(AsyncWebServerRequest *request) {
  if (gasAlarmActive) {
    Serial.println("[GAS ALARM] >>> ALLARME TERMINATO <<<");
    gasAlarmActive = false;
    gasAlarmCode = "";
    gasAlarmName = "";
    gasAlarmValue = 0;

    // NON chiamare forceDisplayUpdate() qui! Gira in task async.
    // Setta flag per il loop principale
    gasAlarmNeedsRedraw = true;
  }

  request->send(200, "application/json", "{\"status\":\"ok\",\"alarm\":\"cleared\"}");
}

// Handler: GET /radar/gas_alarm_status
void handleRadarGasAlarmStatus(AsyncWebServerRequest *request) {
  String json = "{";
  json += "\"active\":" + String(gasAlarmActive ? "true" : "false") + ",";
  json += "\"code\":\"" + gasAlarmCode + "\",";
  json += "\"name\":\"" + gasAlarmName + "\",";
  json += "\"value\":" + String(gasAlarmValue, 1);
  json += "}";
  request->send(200, "application/json", json);
}

// ================== HANDLER ON/OFF (chiamati da sendAllOn/sendAllOff del radar server) ==================

// GET /radar/off - Comando spegnimento display dal radar server
void handleRadarOff(AsyncWebServerRequest *request) {
  radarRemotePresence = false;
  radarServerConnected = true;
  lastRadarRemoteUpdate = millis();

  // Spegni display
  ledcWrite(PWM_CHANNEL, 0);
  lastAppliedBrightness = 0;

  Serial.println("[RADAR REMOTE] CMD OFF ricevuto - Display OFF");
  request->send(200, "application/json", "{\"status\":\"ok\",\"display\":\"off\"}");
}

// GET /radar/on - Comando accensione display dal radar server
void handleRadarOn(AsyncWebServerRequest *request) {
  radarRemotePresence = true;
  radarServerConnected = true;
  lastRadarRemoteUpdate = millis();

  // Riaccendi display
  uint8_t wakeupBrightness;
  if (radarBrightnessControl) {
    wakeupBrightness = map(radarRemoteBrightness, 0, 255, radarBrightnessMin, radarBrightnessMax);
  } else {
    wakeupBrightness = checkIsNightTime(currentHour, currentMinute) ? brightnessNight : brightnessDay;
  }
  ledcWrite(PWM_CHANNEL, wakeupBrightness);
  lastAppliedBrightness = wakeupBrightness;

  Serial.printf("[RADAR REMOTE] CMD ON ricevuto - Display ON lum:%d\n", wakeupBrightness);
  request->send(200, "application/json", "{\"status\":\"ok\",\"display\":\"on\"}");
}

// ================== SETUP WEBSERVER RADAR ==================

void setup_radar_webserver(AsyncWebServer* server) {
  Serial.println("[RADAR REMOTE] Inizializzazione...");

  // Carica configurazione da EEPROM
  loadRadarServerConfig();

  // Registra endpoint (IMPORTANTE: endpoint più specifici PRIMA di quelli generici!)
  server->on("/radar/config/set", HTTP_GET, handleRadarConfigSet);  // Prima questo!
  server->on("/radar/config", HTTP_GET, handleRadarConfigGet);       // Poi questo
  server->on("/radar/presence", HTTP_GET, handleRadarPresence);
  server->on("/radar/brightness", HTTP_GET, handleRadarBrightness);
  server->on("/radar/temperature", HTTP_GET, handleRadarTemperature);
  server->on("/radar/humidity", HTTP_GET, handleRadarHumidity);
  server->on("/radar/test", HTTP_GET, handleRadarTest);
  server->on("/radar/status", HTTP_GET, handleRadarStatus);
  server->on("/radar/on", HTTP_GET, handleRadarOn);     // Comando ON dal radar server
  server->on("/radar/off", HTTP_GET, handleRadarOff);    // Comando OFF dal radar server
  server->on("/api/status", HTTP_GET, handleApiStatus);  // Per discovery

  // Endpoint allarme gas
  server->on("/radar/gas_alarm", HTTP_GET, handleRadarGasAlarm);
  server->on("/radar/gas_alarm_end", HTTP_GET, handleRadarGasAlarmEnd);
  server->on("/radar/gas_alarm_status", HTTP_GET, handleRadarGasAlarmStatus);

  Serial.println("[RADAR REMOTE] Endpoints registrati:");
  Serial.println("[RADAR REMOTE]   GET /radar/presence?value=true/false");
  Serial.println("[RADAR REMOTE]   GET /radar/brightness?value=XXX");
  Serial.println("[RADAR REMOTE]   GET /radar/temperature?value=XX.X");
  Serial.println("[RADAR REMOTE]   GET /radar/humidity?value=XX.X");
  Serial.println("[RADAR REMOTE]   GET /radar/on  (comando ON dal server)");
  Serial.println("[RADAR REMOTE]   GET /radar/off (comando OFF dal server)");
  Serial.println("[RADAR REMOTE]   GET /radar/test");
  Serial.println("[RADAR REMOTE]   GET /radar/status");
  Serial.println("[RADAR REMOTE]   GET /radar/config");
  Serial.println("[RADAR REMOTE]   GET /radar/config/set?ip=X.X.X.X&enabled=1");
  Serial.println("[RADAR REMOTE]   GET /radar/gas_alarm?gas=XX&name=XX&value=XX");
  Serial.println("[RADAR REMOTE]   GET /radar/gas_alarm_end");
  Serial.println("[RADAR REMOTE]   GET /radar/gas_alarm_status");

  // Tenta registrazione sul radar server (dopo un delay per stabilità WiFi)
  if (radarServerEnabled) {
    Serial.println("[RADAR REMOTE] Registrazione automatica programmata...");
  }
}

// Chiamare dopo setup WiFi completato
void initRadarServerConnection() {
  if (radarServerEnabled && WiFi.status() == WL_CONNECTED) {
    Serial.println("[RADAR REMOTE] Tentativo registrazione automatica...");
    if (registerOnRadarServer()) {
      Serial.println("[RADAR REMOTE] Registrato con successo sul radar server!");
    } else {
      Serial.println("[RADAR REMOTE] Radar server non raggiungibile, uso radar locale");
    }
  }
}
