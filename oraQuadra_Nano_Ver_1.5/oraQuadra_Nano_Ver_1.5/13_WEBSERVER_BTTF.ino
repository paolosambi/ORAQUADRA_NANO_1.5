#ifdef EFFECT_BTTF
// ================== WEB SERVER PER CONFIGURAZIONE BTTF ==================
// Interfaccia web per modificare:
// - Date iconiche (DESTINATION TIME e LAST TIME DEPARTED)
// - Abilitazione sveglie per suonare il buzzer
// SALVATAGGIO: File JSON su SD card (/bttf_config.json)

// HTML in file separato per evitare bug preprocessore Arduino IDE 1.8.19
#include "bttf_web_html.h"

// Funzione esterna per aggiornamento immediato del display (definita in 12_BTTF.ino)
extern void forceBTTFRedraw();

// ================== FUNZIONI SALVATAGGIO/CARICAMENTO SU SD ==================

// Salva configurazione BTTF su SD card in formato JSON
bool saveBTTFConfigToSD() {
  File configFile = SD.open("/bttf_config.json", FILE_WRITE);
  if (!configFile) {
    Serial.println("[BTTF] Errore apertura file config per scrittura");
    return false;
  }

  // Crea JSON manualmente
  String json = "{\n";

  json += "  \"destination\": {\n";
  json += "    \"month\": " + String(destinationTime.month) + ",\n";
  json += "    \"day\": " + String(destinationTime.day) + ",\n";
  json += "    \"year\": " + String(destinationTime.year) + ",\n";
  json += "    \"hour\": " + String(destinationTime.hour) + ",\n";
  json += "    \"minute\": " + String(destinationTime.minute) + ",\n";
  json += "    \"ampm\": \"" + String(destinationTime.ampm) + "\",\n";
  json += "    \"alarmEnabled\": " + String(alarmDestinationEnabled ? "true" : "false") + ",\n";
  json += "    \"dailyRepeat\": " + String(alarmDestinationDailyRepeat ? "true" : "false") + "\n";
  json += "  },\n";

  json += "  \"lastDeparted\": {\n";
  json += "    \"month\": " + String(lastDeparted.month) + ",\n";
  json += "    \"day\": " + String(lastDeparted.day) + ",\n";
  json += "    \"year\": " + String(lastDeparted.year) + ",\n";
  json += "    \"hour\": " + String(lastDeparted.hour) + ",\n";
  json += "    \"minute\": " + String(lastDeparted.minute) + ",\n";
  json += "    \"ampm\": \"" + String(lastDeparted.ampm) + "\",\n";
  json += "    \"alarmEnabled\": " + String(alarmLastDepartedEnabled ? "true" : "false") + ",\n";
  json += "    \"dailyRepeat\": " + String(alarmLastDepartedDailyRepeat ? "true" : "false") + "\n";
  json += "  }\n";
  json += "}\n";

  configFile.print(json);
  configFile.close();

  Serial.println("[BTTF] Configurazione salvata su /bttf_config.json");
  return true;
}

// Carica la configurazione BTTF da SD card
bool loadBTTFConfigFromSD() {
  if (!SD.exists("/bttf_config.json")) {
    Serial.println("[BTTF] File config non trovato, uso valori di default");
    // Valori già impostati nello sketch
    saveBTTFConfigToSD();
    return true;
  }

  File configFile = SD.open("/bttf_config.json", FILE_READ);
  if (!configFile) {
    Serial.println("[BTTF] Errore apertura file config");
    return false;
  }

  String jsonStr = "";
  while (configFile.available()) {
    jsonStr += (char)configFile.read();
  }
  configFile.close();

  // Parse JSON manualmente
  auto extractInt = [](const String& json, const String& key) -> int {
    int idx = json.indexOf("\"" + key + "\":");
    if (idx == -1) return 0;
    idx = json.indexOf(":", idx) + 1;
    while (json.charAt(idx) == ' ' || json.charAt(idx) == '\n') idx++;
    int endIdx = idx;
    while (json.charAt(endIdx) >= '0' && json.charAt(endIdx) <= '9') endIdx++;
    return json.substring(idx, endIdx).toInt();
  };

  auto extractString = [](const String& json, const String& key) -> String {
    int idx = json.indexOf("\"" + key + "\":");
    if (idx == -1) return "";
    idx = json.indexOf("\"", idx + key.length() + 3) + 1;
    int endIdx = json.indexOf("\"", idx);
    return json.substring(idx, endIdx);
  };

  auto extractBool = [](const String& json, const String& key) -> bool {
    int idx = json.indexOf("\"" + key + "\":");
    if (idx == -1) return false;
    return json.indexOf("true", idx) > 0 && json.indexOf("true", idx) < idx + 20;
  };

  // Carica DESTINATION TIME
  int destIdx = jsonStr.indexOf("\"destination\"");
  String destSection = jsonStr.substring(destIdx, jsonStr.indexOf("\"lastDeparted\""));
  destinationTime.month = extractInt(destSection, "month");
  destinationTime.day = extractInt(destSection, "day");
  destinationTime.year = extractInt(destSection, "year");
  destinationTime.hour = extractInt(destSection, "hour");
  destinationTime.minute = extractInt(destSection, "minute");
  String destAmpm = extractString(destSection, "ampm");
  destinationTime.ampm = (destAmpm == "PM") ? "PM" : "AM";
  alarmDestinationEnabled = extractBool(destSection, "alarmEnabled");
  alarmDestinationDailyRepeat = extractBool(destSection, "dailyRepeat");

  // Carica LAST TIME DEPARTED
  int lastIdx = jsonStr.indexOf("\"lastDeparted\"");
  String lastSection = jsonStr.substring(lastIdx);
  lastDeparted.month = extractInt(lastSection, "month");
  lastDeparted.day = extractInt(lastSection, "day");
  lastDeparted.year = extractInt(lastSection, "year");
  lastDeparted.hour = extractInt(lastSection, "hour");
  lastDeparted.minute = extractInt(lastSection, "minute");
  String lastAmpm = extractString(lastSection, "ampm");
  lastDeparted.ampm = (lastAmpm == "PM") ? "PM" : "AM";
  alarmLastDepartedEnabled = extractBool(lastSection, "alarmEnabled");
  alarmLastDepartedDailyRepeat = extractBool(lastSection, "dailyRepeat");

  Serial.println("[BTTF] Configurazione caricata da /bttf_config.json");
  Serial.printf("[BTTF] DEST: %d/%d/%d %d:%02d %s (alarm=%d)\n",
                destinationTime.month, destinationTime.day, destinationTime.year,
                destinationTime.hour, destinationTime.minute, destinationTime.ampm,
                alarmDestinationEnabled);
  Serial.printf("[BTTF] LAST: %d/%d/%d %d:%02d %s (alarm=%d)\n",
                lastDeparted.month, lastDeparted.day, lastDeparted.year,
                lastDeparted.hour, lastDeparted.minute, lastDeparted.ampm,
                alarmLastDepartedEnabled);
  return true;
}

// ================== ENDPOINT WEB SERVER ==================

// GET /bttf - Pagina HTML principale
void handleBTTFEditor(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", BTTF_HTML);
}

// GET /bttf/config - Restituisce configurazione corrente in JSON
void handleGetConfig(AsyncWebServerRequest *request) {
  String json = "{\n";

  json += "  \"destination\": {\n";
  json += "    \"month\": " + String(destinationTime.month) + ",\n";
  json += "    \"day\": " + String(destinationTime.day) + ",\n";
  json += "    \"year\": " + String(destinationTime.year) + ",\n";
  json += "    \"hour\": " + String(destinationTime.hour) + ",\n";
  json += "    \"minute\": " + String(destinationTime.minute) + ",\n";
  json += "    \"ampm\": \"" + String(destinationTime.ampm) + "\",\n";
  json += "    \"alarmEnabled\": " + String(alarmDestinationEnabled ? "true" : "false") + ",\n";
  json += "    \"dailyRepeat\": " + String(alarmDestinationDailyRepeat ? "true" : "false") + "\n";
  json += "  },\n";

  json += "  \"lastDeparted\": {\n";
  json += "    \"month\": " + String(lastDeparted.month) + ",\n";
  json += "    \"day\": " + String(lastDeparted.day) + ",\n";
  json += "    \"year\": " + String(lastDeparted.year) + ",\n";
  json += "    \"hour\": " + String(lastDeparted.hour) + ",\n";
  json += "    \"minute\": " + String(lastDeparted.minute) + ",\n";
  json += "    \"ampm\": \"" + String(lastDeparted.ampm) + "\",\n";
  json += "    \"alarmEnabled\": " + String(alarmLastDepartedEnabled ? "true" : "false") + ",\n";
  json += "    \"dailyRepeat\": " + String(alarmLastDepartedDailyRepeat ? "true" : "false") + "\n";
  json += "  }\n";
  json += "}\n";

  request->send(200, "application/json", json);
}

// GET /bttf/presenttime - Restituisce l'orario corrente in tempo reale
void handleGetPresentTime(AsyncWebServerRequest *request) {
  // Ottieni l'ora corrente da ezTime
  int hour24 = myTZ.hour();
  int minute = myTZ.minute();
  int second = myTZ.second();
  int day = myTZ.day();
  int month = myTZ.month();
  int year = myTZ.year();

  // Converti da formato 24h a 12h con AM/PM
  String ampm = (hour24 >= 12) ? "PM" : "AM";
  int hour12 = hour24 % 12;
  if (hour12 == 0) hour12 = 12;  // 00:xx diventa 12:xx AM

  String json = "{\n";
  json += "  \"month\": " + String(month) + ",\n";
  json += "  \"day\": " + String(day) + ",\n";
  json += "  \"year\": " + String(year) + ",\n";
  json += "  \"hour\": " + String(hour12) + ",\n";
  json += "  \"minute\": " + String(minute) + ",\n";
  json += "  \"second\": " + String(second) + ",\n";
  json += "  \"ampm\": \"" + ampm + "\"\n";
  json += "}\n";

  request->send(200, "application/json", json);
}

// GET /bttf/save - Salva configurazione
void handleSaveConfig(AsyncWebServerRequest *request) {
  Serial.println("=====================================");
  Serial.println("[BTTF WEB] *** RICEVUTA RICHIESTA SALVATAGGIO ***");

  // DESTINATION TIME
  if (request->hasParam("dest_month")) {
    destinationTime.month = request->getParam("dest_month")->value().toInt();
  }
  if (request->hasParam("dest_day")) {
    destinationTime.day = request->getParam("dest_day")->value().toInt();
  }
  if (request->hasParam("dest_year")) {
    destinationTime.year = request->getParam("dest_year")->value().toInt();
  }
  if (request->hasParam("dest_hour")) {
    destinationTime.hour = request->getParam("dest_hour")->value().toInt();
  }
  if (request->hasParam("dest_minute")) {
    destinationTime.minute = request->getParam("dest_minute")->value().toInt();
  }
  if (request->hasParam("dest_ampm")) {
    String ampm = request->getParam("dest_ampm")->value();
    destinationTime.ampm = (ampm == "PM") ? "PM" : "AM";
  }
  if (request->hasParam("dest_alarm")) {
    alarmDestinationEnabled = (request->getParam("dest_alarm")->value().toInt() == 1);
  }
  if (request->hasParam("dest_daily")) {
    alarmDestinationDailyRepeat = (request->getParam("dest_daily")->value().toInt() == 1);
  }

  Serial.printf("[BTTF WEB] DEST ricevuto: %d/%d/%d %d:%02d %s (alarm=%d, daily=%d)\n",
                destinationTime.month, destinationTime.day, destinationTime.year,
                destinationTime.hour, destinationTime.minute, destinationTime.ampm,
                alarmDestinationEnabled, alarmDestinationDailyRepeat);

  // LAST TIME DEPARTED
  if (request->hasParam("last_month")) {
    lastDeparted.month = request->getParam("last_month")->value().toInt();
  }
  if (request->hasParam("last_day")) {
    lastDeparted.day = request->getParam("last_day")->value().toInt();
  }
  if (request->hasParam("last_year")) {
    lastDeparted.year = request->getParam("last_year")->value().toInt();
  }
  if (request->hasParam("last_hour")) {
    lastDeparted.hour = request->getParam("last_hour")->value().toInt();
  }
  if (request->hasParam("last_minute")) {
    lastDeparted.minute = request->getParam("last_minute")->value().toInt();
  }
  if (request->hasParam("last_ampm")) {
    String ampm = request->getParam("last_ampm")->value();
    lastDeparted.ampm = (ampm == "PM") ? "PM" : "AM";
  }
  if (request->hasParam("last_alarm")) {
    alarmLastDepartedEnabled = (request->getParam("last_alarm")->value().toInt() == 1);
  }
  if (request->hasParam("last_daily")) {
    alarmLastDepartedDailyRepeat = (request->getParam("last_daily")->value().toInt() == 1);
  }

  Serial.printf("[BTTF WEB] LAST ricevuto: %d/%d/%d %d:%02d %s (alarm=%d, daily=%d)\n",
                lastDeparted.month, lastDeparted.day, lastDeparted.year,
                lastDeparted.hour, lastDeparted.minute, lastDeparted.ampm,
                alarmLastDepartedEnabled, alarmLastDepartedDailyRepeat);

  // Salva su SD card
  Serial.println("[BTTF WEB] Salvataggio su SD card...");
  bool saved = saveBTTFConfigToSD();

  if (saved) {
    Serial.println("[BTTF WEB] ✓ Salvato con successo su SD");
    // Reset flag allarmi per permettere riattivazione
    alarmDestinationTriggered = false;
    alarmLastDepartedTriggered = false;
  } else {
    Serial.println("[BTTF WEB] ✗ ERRORE salvataggio su SD!");
  }

  // Aggiorna display
  Serial.println("[BTTF WEB] Chiamata forceBTTFRedraw()...");
  forceBTTFRedraw();
  Serial.println("[BTTF WEB] forceBTTFRedraw() eseguita");
  Serial.println("=====================================");

  request->send(200, "text/plain", saved ? "OK" : "ERROR");
}

// GET /settime - Imposta manualmente data e ora per test
// Esempio: http://192.168.1.X:8080/settime?year=2025&month=1&day=1&hour=0&minute=0&second=0
void handleSetTime(AsyncWebServerRequest *request) {
  if (!request->hasParam("year") || !request->hasParam("month") || !request->hasParam("day") ||
      !request->hasParam("hour") || !request->hasParam("minute") || !request->hasParam("second")) {
    request->send(400, "text/plain", "Parametri mancanti. Usa: /settime?year=2025&month=1&day=1&hour=0&minute=0&second=0");
    return;
  }

  int year = request->getParam("year")->value().toInt();
  int month = request->getParam("month")->value().toInt();
  int day = request->getParam("day")->value().toInt();
  int hour = request->getParam("hour")->value().toInt();
  int minute = request->getParam("minute")->value().toInt();
  int second = request->getParam("second")->value().toInt();

  // Costruisci la stringa di data/ora per logging
  char dateTimeStr[30];
  sprintf(dateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

  // Converti in timestamp Unix (secondi dal 1970-01-01)
  struct tm timeinfo;
  timeinfo.tm_year = year - 1900;  // tm_year è anni dal 1900
  timeinfo.tm_mon = month - 1;     // tm_mon è 0-11
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = minute;
  timeinfo.tm_sec = second;
  timeinfo.tm_isdst = -1;          // Auto-determina DST

  time_t timestamp = mktime(&timeinfo);

  // Imposta l'ora manualmente usando ezTime
  myTZ.setTime(timestamp);

  Serial.printf("[TEST] Ora impostata manualmente: %s\n", dateTimeStr);
  Serial.printf("[TEST] Verifica: %02d/%02d/%d %02d:%02d:%02d\n",
                myTZ.day(), myTZ.month(), myTZ.year(),
                myTZ.hour(), myTZ.minute(), myTZ.second());

  String response = "OK - Ora impostata: " + String(dateTimeStr);
  response += "\nVerifica: " + myTZ.dateTime("d/m/Y H:i:s");
  request->send(200, "text/plain", response);
}

// Registra gli endpoint sul server (chiamato da setup)
void setup_bttf_webserver(AsyncWebServer* server) {
  Serial.println("=====================================");
  Serial.println("[BTTF WEB] Registrazione endpoints...");
  Serial.println("[BTTF WEB] IMPORTANTE: endpoints specifici prima di quelli generici!");

  // IMPORTANTE: Registrare gli endpoint più specifici PRIMA di quelli generici
  // Altrimenti /bttf cattura tutto (incluso /bttf/save, /bttf/config, ecc.)

  server->on("/bttf/presenttime", HTTP_GET, handleGetPresentTime);
  Serial.println("[BTTF WEB] ✓ Registrato: GET /bttf/presenttime");

  server->on("/bttf/save", HTTP_GET, handleSaveConfig);
  Serial.println("[BTTF WEB] ✓ Registrato: GET /bttf/save");

  server->on("/bttf/config", HTTP_GET, handleGetConfig);
  Serial.println("[BTTF WEB] ✓ Registrato: GET /bttf/config");

  server->on("/settime", HTTP_GET, handleSetTime);
  Serial.println("[BTTF WEB] ✓ Registrato: GET /settime");

  // /bttf DEVE essere registrato per ULTIMO per non catturare gli altri endpoint
  server->on("/bttf", HTTP_GET, handleBTTFEditor);
  Serial.println("[BTTF WEB] ✓ Registrato: GET /bttf (pagina principale)");

  Serial.println("[BTTF WEB] Tutti gli endpoints registrati con successo!");
  Serial.println("=====================================");
}

#endif
