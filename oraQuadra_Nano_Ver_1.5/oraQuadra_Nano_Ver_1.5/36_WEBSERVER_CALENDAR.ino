// ================== WEB SERVER CALENDARIO + STORAGE LOCALE ==================
// Backend per gestione eventi calendario con LittleFS
// CRUD locale + merge con eventi Google + push bidirezionale
// Endpoint: /calendar, /calendar/list, /calendar/add, /calendar/edit, /calendar/delete, /calendar/sync, /calendar/push

#include "calendar_web_html.h"

#ifdef EFFECT_CALENDAR

// ============================================================================
// STORAGE LOCALE LITTLEFS
// ============================================================================
#define CALENDAR_EVENTS_FILE "/calendar_events.json"
#define MAX_LOCAL_EVENTS 50

struct LocalCalendarEvent {
  uint16_t id;
  String title;
  String date;      // DD/MM/YYYY
  String start;     // HH:mm
  String end;       // HH:mm
  String googleId;  // vuoto = non sincronizzato
};

LocalCalendarEvent localEvents[MAX_LOCAL_EVENTS];
int localEventCount = 0;
uint16_t nextLocalEventId = 1;
String lastSyncTime = "--";

// Flag per sincronizzazione asincrona (il main loop esegue il fetch)
volatile bool forceGoogleSync = false;
volatile int16_t pushToGoogleId = -1; // ID evento da pushare, -1 = nessuno

// ============================================================================
// FUNZIONI CRUD LITTLEFS
// ============================================================================

void loadLocalEventsFromLittleFS() {
  if (!LittleFS.exists(CALENDAR_EVENTS_FILE)) {
    Serial.println("[CAL-STORE] Nessun file eventi trovato");
    localEventCount = 0;
    return;
  }

  File f = LittleFS.open(CALENDAR_EVENTS_FILE, "r");
  if (!f) {
    Serial.println("[CAL-STORE] Errore apertura file eventi");
    localEventCount = 0;
    return;
  }

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    Serial.printf("[CAL-STORE] Errore parsing JSON: %s\n", err.c_str());
    localEventCount = 0;
    return;
  }

  localEventCount = 0;
  nextLocalEventId = doc["nextId"] | 1;

  JsonArray arr = doc["events"].as<JsonArray>();
  for (JsonObject obj : arr) {
    if (localEventCount >= MAX_LOCAL_EVENTS) break;
    localEvents[localEventCount].id = obj["id"] | 0;
    localEvents[localEventCount].title = obj["title"].as<String>();
    localEvents[localEventCount].date = obj["date"].as<String>();
    localEvents[localEventCount].start = obj["start"].as<String>();
    localEvents[localEventCount].end = obj["end"].as<String>();
    localEvents[localEventCount].googleId = obj["gid"].as<String>();
    localEventCount++;
  }

  Serial.printf("[CAL-STORE] Caricati %d eventi locali (nextId=%d)\n", localEventCount, nextLocalEventId);
}

void saveLocalEventsToLittleFS() {
  DynamicJsonDocument doc(8192);
  doc["nextId"] = nextLocalEventId;
  JsonArray arr = doc.createNestedArray("events");

  for (int i = 0; i < localEventCount; i++) {
    JsonObject obj = arr.createNestedObject();
    obj["id"] = localEvents[i].id;
    obj["title"] = localEvents[i].title;
    obj["date"] = localEvents[i].date;
    obj["start"] = localEvents[i].start;
    obj["end"] = localEvents[i].end;
    if (localEvents[i].googleId.length() > 0) {
      obj["gid"] = localEvents[i].googleId;
    }
  }

  File f = LittleFS.open(CALENDAR_EVENTS_FILE, "w");
  if (!f) {
    Serial.println("[CAL-STORE] Errore scrittura file eventi");
    return;
  }
  serializeJson(doc, f);
  f.close();
  Serial.printf("[CAL-STORE] Salvati %d eventi su LittleFS\n", localEventCount);
}

bool addLocalEvent(const String& title, const String& date, const String& start, const String& end) {
  if (localEventCount >= MAX_LOCAL_EVENTS) return false;
  localEvents[localEventCount].id = nextLocalEventId++;
  localEvents[localEventCount].title = title;
  localEvents[localEventCount].date = date;
  localEvents[localEventCount].start = start;
  localEvents[localEventCount].end = end;
  localEvents[localEventCount].googleId = "";
  localEventCount++;
  saveLocalEventsToLittleFS();
  return true;
}

bool editLocalEvent(uint16_t id, const String& title, const String& date, const String& start, const String& end) {
  for (int i = 0; i < localEventCount; i++) {
    if (localEvents[i].id == id) {
      localEvents[i].title = title;
      localEvents[i].date = date;
      localEvents[i].start = start;
      localEvents[i].end = end;
      saveLocalEventsToLittleFS();
      return true;
    }
  }
  return false;
}

bool deleteLocalEvent(uint16_t id) {
  for (int i = 0; i < localEventCount; i++) {
    if (localEvents[i].id == id) {
      // Shift array
      for (int j = i; j < localEventCount - 1; j++) {
        localEvents[j] = localEvents[j + 1];
      }
      localEventCount--;
      saveLocalEventsToLittleFS();
      return true;
    }
  }
  return false;
}

// ============================================================================
// MERGE EVENTI LOCALI + GOOGLE
// ============================================================================

// mergedEventsBuffer[] e mergedEventCount sono definiti in 35_CALENDAR.ino
// googleEventsBuffer[] e googleEventCount sono definiti in 35_CALENDAR.ino
extern CalendarEvent mergedEventsBuffer[];
extern int mergedEventCount;
extern CalendarEvent googleEventsBuffer[];
extern int googleEventCount;
extern bool isCalendarUpdating;

void mergeLocalAndGoogleEvents() {
  mergedEventCount = 0;

  // 1. Aggiungi eventi locali
  for (int i = 0; i < localEventCount && mergedEventCount < 30; i++) {
    CalendarEvent& ev = mergedEventsBuffer[mergedEventCount];
    ev.id = localEvents[i].id;
    ev.title = localEvents[i].title;
    ev.date = localEvents[i].date;
    // dateShort = DD/MM (primi 5 caratteri)
    ev.dateShort = localEvents[i].date.substring(0, 5);
    ev.start = localEvents[i].start;
    ev.end = localEvents[i].end;
    // Calcola startMinutes
    if (ev.start.length() >= 5) {
      ev.startMinutes = ev.start.substring(0, 2).toInt() * 60 + ev.start.substring(3, 5).toInt();
    } else {
      ev.startMinutes = 0;
    }
    // Calcola endMinutes
    if (ev.end.length() >= 5) {
      ev.endMinutes = ev.end.substring(0, 2).toInt() * 60 + ev.end.substring(3, 5).toInt();
    } else {
      ev.endMinutes = ev.startMinutes + 60;
    }
    ev.source = (localEvents[i].googleId.length() > 0) ? 2 : 0; // 0=locale, 2=sincronizzato
    mergedEventCount++;
  }

  // 2. Aggiungi eventi Google (solo quelli non già presenti come locali)
  for (int i = 0; i < googleEventCount && mergedEventCount < 30; i++) {
    bool isDuplicate = false;

    // Check 1: dedup per googleId (se disponibile)
    if (!isDuplicate && googleEventsBuffer[i].googleId.length() > 0) {
      for (int j = 0; j < localEventCount; j++) {
        if (localEvents[j].googleId == googleEventsBuffer[i].googleId) {
          isDuplicate = true;
          break;
        }
      }
    }

    // Check 2: dedup per titolo + data + ora inizio (fallback robusto)
    if (!isDuplicate) {
      for (int j = 0; j < localEventCount; j++) {
        if (localEvents[j].title == googleEventsBuffer[i].title &&
            localEvents[j].date == googleEventsBuffer[i].date &&
            localEvents[j].start == googleEventsBuffer[i].start) {
          isDuplicate = true;
          // Bonus: se il locale non ha googleId ma il Google si, salvalo
          if (localEvents[j].googleId.length() == 0 && googleEventsBuffer[i].googleId.length() > 0) {
            localEvents[j].googleId = googleEventsBuffer[i].googleId;
            saveLocalEventsToLittleFS();
            Serial.printf("[CAL-MERGE] Auto-link locale id=%d -> googleId=%s\n",
                          localEvents[j].id, googleEventsBuffer[i].googleId.c_str());
          }
          break;
        }
      }
    }

    if (isDuplicate) continue;

    CalendarEvent& ev = mergedEventsBuffer[mergedEventCount];
    ev.id = 0; // Google events non hanno id locale
    ev.title = googleEventsBuffer[i].title;
    ev.date = googleEventsBuffer[i].date;
    ev.dateShort = googleEventsBuffer[i].dateShort;
    ev.start = googleEventsBuffer[i].start;
    ev.end = googleEventsBuffer[i].end;
    ev.startMinutes = googleEventsBuffer[i].startMinutes;
    ev.endMinutes = googleEventsBuffer[i].endMinutes;
    ev.source = 1; // Google
    ev.googleId = googleEventsBuffer[i].googleId;
    mergedEventCount++;
  }

  // 3. Ordina per data e ora
  for (int i = 0; i < mergedEventCount - 1; i++) {
    for (int j = i + 1; j < mergedEventCount; j++) {
      bool swap = false;
      // Confronta date (DD/MM/YYYY -> YYYY/MM/DD per ordinamento)
      if (mergedEventsBuffer[i].date.length() >= 10 && mergedEventsBuffer[j].date.length() >= 10) {
        String di = mergedEventsBuffer[i].date.substring(6, 10) + mergedEventsBuffer[i].date.substring(3, 5) + mergedEventsBuffer[i].date.substring(0, 2);
        String dj = mergedEventsBuffer[j].date.substring(6, 10) + mergedEventsBuffer[j].date.substring(3, 5) + mergedEventsBuffer[j].date.substring(0, 2);
        if (di > dj) swap = true;
        else if (di == dj && mergedEventsBuffer[i].startMinutes > mergedEventsBuffer[j].startMinutes) swap = true;
      } else if (mergedEventsBuffer[i].startMinutes > mergedEventsBuffer[j].startMinutes) {
        swap = true;
      }
      if (swap) {
        CalendarEvent tmp = mergedEventsBuffer[i];
        mergedEventsBuffer[i] = mergedEventsBuffer[j];
        mergedEventsBuffer[j] = tmp;
      }
    }
  }

  Serial.printf("[CAL-MERGE] Totale eventi mergiati: %d (locali:%d, google:%d)\n",
                mergedEventCount, localEventCount, googleEventCount);
}

// ============================================================================
// PUSH EVENTO VERSO GOOGLE
// ============================================================================

bool pushEventToGoogle(uint16_t id) {
  // Trova l'evento locale
  int idx = -1;
  for (int i = 0; i < localEventCount; i++) {
    if (localEvents[i].id == id) { idx = i; break; }
  }
  if (idx < 0) return false;

  if (calendarGoogleUrl.length() == 0 || calendarApiKey.length() == 0) {
    Serial.println("[CAL-PUSH] URL o API key Google non configurati");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  // FORCE: segue i redirect 302 anche per POST (necessario per Google Apps Script)
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setTimeout(15000);

  String baseUrl = calendarGoogleUrl;
  int keyIdx = baseUrl.indexOf("?key=");
  if (keyIdx > 0) baseUrl = baseUrl.substring(0, keyIdx);

  String url = baseUrl + "?key=" + calendarApiKey;

  if (!http.begin(client, url)) {
    Serial.println("[CAL-PUSH] Errore connessione");
    return false;
  }

  http.addHeader("Content-Type", "application/json");

  // Crea JSON payload
  DynamicJsonDocument doc(512);
  doc["action"] = "create";
  doc["title"] = localEvents[idx].title;
  doc["date"] = localEvents[idx].date;
  doc["start"] = localEvents[idx].start;
  doc["end"] = localEvents[idx].end;

  String payload;
  serializeJson(doc, payload);

  Serial.printf("[CAL-PUSH] POST %s\n", url.c_str());
  Serial.printf("[CAL-PUSH] Payload: %s\n", payload.c_str());
  int httpCode = http.POST(payload);
  Serial.printf("[CAL-PUSH] HTTP code: %d\n", httpCode);

  bool success = false;
  if (httpCode == 200) {
    String response = http.getString();
    Serial.printf("[CAL-PUSH] Risposta: %s\n", response.c_str());
    DynamicJsonDocument resp(512);
    deserializeJson(resp, response);
    if (resp["success"] == true) {
      String gid = resp["googleId"].as<String>();
      if (gid.length() > 0) {
        localEvents[idx].googleId = gid;
        saveLocalEventsToLittleFS();
        Serial.printf("[CAL-PUSH] Evento %d sincronizzato, googleId: %s\n", id, gid.c_str());
      }
      success = true;
    } else {
      Serial.printf("[CAL-PUSH] Errore dal server: %s\n", response.c_str());
    }
  } else {
    Serial.printf("[CAL-PUSH] Errore HTTP: %d\n", httpCode);
  }

  http.end();
  return success;
}

// ============================================================================
// HELPER: converte trattini in separatori corretti
// Date: DD-MM-YYYY -> DD/MM/YYYY    Time: HH-MM -> HH:MM
// ============================================================================
String calFixDate(const String& s) {
  // DD-MM-YYYY -> DD/MM/YYYY
  String r = s;
  if (r.length() >= 10 && r.charAt(2) == '-' && r.charAt(5) == '-') {
    r.setCharAt(2, '/');
    r.setCharAt(5, '/');
  }
  return r;
}

String calFixTime(const String& s) {
  // HH-MM -> HH:MM
  String r = s;
  if (r.length() >= 5 && r.charAt(2) == '-') {
    r.setCharAt(2, ':');
  }
  return r;
}

// ============================================================================
// ENDPOINT WEB SERVER
// ============================================================================

void setup_calendar_webserver(AsyncWebServer* server) {
  Serial.println("[CALENDAR WEB] Registrazione endpoints...");

  // Pagina HTML calendario (usa /cal per evitare conflitto prefisso con /calendar/*)
  server->on("/cal", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", CALENDAR_HTML);
  });

  // Lista eventi per mese (locali + Google mergiati)
  server->on("/calendar/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    int month = 0, year = 0;
    if (request->hasParam("month")) month = request->getParam("month")->value().toInt();
    if (request->hasParam("year")) year = request->getParam("year")->value().toInt();

    // Forza merge prima di rispondere
    mergeLocalAndGoogleEvents();

    DynamicJsonDocument doc(8192);
    doc["localCount"] = localEventCount;
    doc["googleCount"] = googleEventCount;
    doc["lastSync"] = lastSyncTime;

    JsonArray arr = doc.createNestedArray("events");

    for (int i = 0; i < mergedEventCount; i++) {
      // Filtra per mese/anno se specificati
      if (month > 0 && year > 0 && mergedEventsBuffer[i].date.length() >= 10) {
        int evMonth = mergedEventsBuffer[i].date.substring(3, 5).toInt();
        int evYear = mergedEventsBuffer[i].date.substring(6, 10).toInt();
        if (evMonth != month || evYear != year) continue;
      }

      JsonObject obj = arr.createNestedObject();
      obj["id"] = mergedEventsBuffer[i].id;
      obj["title"] = mergedEventsBuffer[i].title;
      obj["date"] = mergedEventsBuffer[i].date;
      obj["start"] = mergedEventsBuffer[i].start;
      obj["end"] = mergedEventsBuffer[i].end;
      obj["source"] = mergedEventsBuffer[i].source;
    }

    String json;
    serializeJson(doc, json);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    request->send(response);
  });

  // Aggiungi evento locale
  server->on("/calendar/add", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[CAL-WEB] /calendar/add chiamato, params=%d\n", request->params());
    if (!request->hasParam("title") || !request->hasParam("date")) {
      Serial.println("[CAL-WEB] Parametri mancanti: title o date");
      request->send(200, "application/json", "{\"ok\":false,\"error\":\"Parametri mancanti\"}");
      return;
    }

    String title = request->getParam("title")->value();
    String date = calFixDate(request->getParam("date")->value());
    String start = request->hasParam("start") ? calFixTime(request->getParam("start")->value()) : "00:00";
    String end = request->hasParam("end") ? calFixTime(request->getParam("end")->value()) : "";

    Serial.printf("[CAL-WEB] ADD title='%s' date='%s' start='%s' end='%s'\n",
                  title.c_str(), date.c_str(), start.c_str(), end.c_str());

    if (title.length() == 0 || date.length() < 10) {
      Serial.printf("[CAL-WEB] Dati non validi: title.len=%d date.len=%d\n", title.length(), date.length());
      request->send(200, "application/json", "{\"ok\":false,\"error\":\"Dati non validi\"}");
      return;
    }

    if (addLocalEvent(title, date, start, end)) {
      uint16_t newId = nextLocalEventId - 1; // ID appena assegnato
      mergeLocalAndGoogleEvents();
      // Auto-push a Google Calendar
      pushToGoogleId = (int16_t)newId;
      forceGoogleSync = true; // Anche sync per aggiornare dati Google
      Serial.printf("[CAL-WEB] Evento aggiunto (id=%d) + auto-push a Google\n", newId);
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(200, "application/json", "{\"ok\":false,\"error\":\"Limite eventi raggiunto\"}");
    }
  });

  // Modifica evento locale
  server->on("/calendar/edit", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("id")) {
      request->send(200, "application/json", "{\"ok\":false,\"error\":\"ID mancante\"}");
      return;
    }

    uint16_t id = request->getParam("id")->value().toInt();
    String title = request->hasParam("title") ? request->getParam("title")->value() : "";
    String date = request->hasParam("date") ? calFixDate(request->getParam("date")->value()) : "";
    String start = request->hasParam("start") ? calFixTime(request->getParam("start")->value()) : "";
    String end = request->hasParam("end") ? calFixTime(request->getParam("end")->value()) : "";

    if (editLocalEvent(id, title, date, start, end)) {
      mergeLocalAndGoogleEvents();
      forceGoogleSync = true; // Refresh dati Google dopo modifica
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(200, "application/json", "{\"ok\":false,\"error\":\"Evento non trovato\"}");
    }
  });

  // Elimina evento locale
  server->on("/calendar/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("id")) {
      request->send(200, "application/json", "{\"ok\":false,\"error\":\"ID mancante\"}");
      return;
    }

    uint16_t id = request->getParam("id")->value().toInt();
    if (deleteLocalEvent(id)) {
      mergeLocalAndGoogleEvents();
      forceGoogleSync = true; // Refresh dati Google dopo eliminazione
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(200, "application/json", "{\"ok\":false,\"error\":\"Evento non trovato\"}");
    }
  });

  // Forza sincronizzazione con Google (non-bloccante: setta flag, il loop fa il fetch)
  server->on("/calendar/sync", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Setta il flag - il main loop eseguira' il fetch HTTPS bloccante
    forceGoogleSync = true;

    // Rispondi subito con i dati correnti
    DynamicJsonDocument doc(256);
    doc["ok"] = true;
    doc["fetched"] = googleEventCount;
    doc["total"] = mergedEventCount;
    doc["message"] = "Sync avviata, ricarica tra qualche secondo";

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });

  // Push singolo evento a Google (non-bloccante: setta flag, il loop fa il push)
  server->on("/calendar/push", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("id")) {
      request->send(200, "application/json", "{\"ok\":false,\"error\":\"ID mancante\"}");
      return;
    }

    uint16_t id = request->getParam("id")->value().toInt();
    pushToGoogleId = (int16_t)id;

    request->send(200, "application/json", "{\"ok\":true,\"message\":\"Push avviato\"}");
  });

  // Stato sync (per polling dalla pagina web)
  server->on("/calendar/syncstatus", HTTP_GET, [](AsyncWebServerRequest *request) {
    bool busy = isCalendarUpdating || forceGoogleSync || pushToGoogleId >= 0;
    DynamicJsonDocument doc(128);
    doc["syncing"] = busy;
    doc["lastSync"] = lastSyncTime;
    String json;
    serializeJson(doc, json);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    request->send(response);
  });

  Serial.println("[CALENDAR WEB] Endpoints registrati su /calendar/*");
}

#endif // EFFECT_CALENDAR
