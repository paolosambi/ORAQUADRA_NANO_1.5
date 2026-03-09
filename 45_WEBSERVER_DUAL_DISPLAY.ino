// ============================================================================
// 45_WEBSERVER_DUAL_DISPLAY.ino
// Endpoint web per la configurazione dual display (griglia multi-pannello)
// Gestisce: stato, configurazione, peer, scansione e auto-discovery
// ============================================================================

#include "dualdisplay_web_html.h"

// macToString() gia' definita in 44_DUAL_DISPLAY.ino

// ---------------------------------------------------------------------------
// Helper: converte stringa MAC "AA:BB:CC:DD:EE:FF" in array di 6 byte
// Ritorna true se il parsing ha successo
// ---------------------------------------------------------------------------
bool parseMacString(const String& str, uint8_t* mac) {
  return sscanf(str.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6;
}

// ---------------------------------------------------------------------------
// Helper: ritorna il nome del ruolo in base al valore numerico
// ---------------------------------------------------------------------------
static String roleName(uint8_t role) {
  switch (role) {
    case 0:  return "Standalone";
    case 1:  return "Master";
    case 2:  return "Slave";
    default: return "Sconosciuto";
  }
}

// ---------------------------------------------------------------------------
// Setup di tutti gli endpoint web per il dual display
// Chiamata prima di clockWebServer->begin()
// ---------------------------------------------------------------------------
void setup_dualdisplay_webserver(AsyncWebServer* server) {

  // GET /dualdisplay - Pagina HTML con valori correnti iniettati server-side
  server->on("/dualdisplay", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[DUAL-WEB] Richiesta pagina HTML /dualdisplay");
    uint8_t mac[6];
    WiFi.macAddress(mac);
    String macStr = macToString(mac);

    // Inietta TUTTI i valori correnti nel template HTML
    String html = String(DUALDISPLAY_HTML);
    html.replace("%%MAC%%", macStr);
    html.replace("%%ENABLED%%", dualDisplayEnabled ? "true" : "false");
    html.replace("%%ROLE%%", String(panelRole));
    html.replace("%%PANELX%%", String(panelX));
    html.replace("%%PANELY%%", String(panelY));
    html.replace("%%GRIDW%%", String(gridW));
    html.replace("%%GRIDH%%", String(gridH));
    html.replace("%%VIRTUALW%%", String((uint16_t)gridW * 480));
    html.replace("%%VIRTUALH%%", String((uint16_t)gridH * 480));
    html.replace("%%PEERCOUNT%%", String(peerCount));

    Serial.printf("[DUAL-WEB] Pagina servita con: enabled=%d grid=%dx%d pos=(%d,%d) role=%d peers=%d\n",
                  dualDisplayEnabled, gridW, gridH, panelX, panelY, panelRole, peerCount);

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    request->send(response);
  });

  // GET /dualdisplay/status - Stato completo in JSON (no-cache per refresh corretto)
  server->on("/dualdisplay/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    uint8_t myMAC[6];
    WiFi.macAddress(myMAC);
    String myMACStr = macToString(myMAC);

    uint16_t vw = (uint16_t)gridW * 480;
    uint16_t vh = (uint16_t)gridH * 480;

    String json = "{";
    json += "\"enabled\":" + String(dualDisplayEnabled ? "true" : "false") + ",";
    json += "\"panelX\":" + String(panelX) + ",";
    json += "\"panelY\":" + String(panelY) + ",";
    json += "\"gridW\":" + String(gridW) + ",";
    json += "\"gridH\":" + String(gridH) + ",";
    json += "\"role\":" + String(panelRole) + ",";
    json += "\"roleName\":\"" + roleName(panelRole) + "\",";
    json += "\"peerCount\":" + String(peerCount) + ",";

    json += "\"peers\":[";
    for (uint8_t i = 0; i < peerCount; i++) {
      if (i > 0) json += ",";
      json += "{\"mac\":\"" + macToString(peerMACs[i]) + "\",\"index\":" + String(i);
      json += ",\"active\":" + String(peerStates[i].active ? "true" : "false");
      json += ",\"peerPanelX\":" + String(peerStates[i].peerPanelX);
      json += ",\"peerPanelY\":" + String(peerStates[i].peerPanelY);
      json += "}";
    }
    json += "],";

    json += "\"frameCounter\":" + String(dualFrameCounter) + ",";
    json += "\"myMAC\":\"" + myMACStr + "\",";
    json += "\"virtualW\":" + String(vw) + ",";
    json += "\"virtualH\":" + String(vh) + ",";
    json += "\"discovery\":" + String(discoveryInProgress ? "true" : "false");
    json += "}";

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    request->send(response);
  });

  // POST /dualdisplay/config - Configura il pannello
  // Parametri form-encoded: enabled, panelX, panelY, gridW, gridH, role
  server->on("/dualdisplay/config", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("[DUAL-WEB] Richiesta configurazione /dualdisplay/config");

    if (request->hasParam("enabled", true)) {
      String val = request->getParam("enabled", true)->value();
      dualDisplayEnabled = (val == "1" || val == "true");
      Serial.printf("[DUAL-WEB] enabled = %s\n", dualDisplayEnabled ? "true" : "false");
    }

    if (request->hasParam("panelX", true)) {
      panelX = (uint8_t)request->getParam("panelX", true)->value().toInt();
    }
    if (request->hasParam("panelY", true)) {
      panelY = (uint8_t)request->getParam("panelY", true)->value().toInt();
    }
    if (request->hasParam("gridW", true)) {
      uint8_t val = (uint8_t)request->getParam("gridW", true)->value().toInt();
      if (val >= 1 && val <= 2) gridW = val;
    }
    if (request->hasParam("gridH", true)) {
      uint8_t val = (uint8_t)request->getParam("gridH", true)->value().toInt();
      if (val >= 1 && val <= 2) gridH = val;
    }
    if (request->hasParam("role", true)) {
      uint8_t val = (uint8_t)request->getParam("role", true)->value().toInt();
      if (val <= 2) panelRole = val;
    }

    // Validazione: panelX < gridW, panelY < gridH
    if (panelX >= gridW) panelX = gridW - 1;
    if (panelY >= gridH) panelY = gridH - 1;

    if (!saveDualDisplaySettings()) {
      Serial.println("[DUAL-WEB] ERRORE: EEPROM.commit() fallito!");
    }

    // Se siamo il master, propaga enable/disable a tutti gli slave via ESP-NOW
    // PRIMA di reinit (se disabilitiamo, ESP-NOW verra' deinizializzato dopo)
    if (panelRole == 1 && espNowInitialized) {
      sendConfigPushToAllPeers();
    }

    // Reinizializza ESP-NOW senza ri-leggere da EEPROM (i valori sono gia' in RAM)
    reinitDualDisplayEspNow();

    // Se abbiamo appena abilitato e ESP-NOW e' ora attivo, invia push ai peer
    if (panelRole == 1 && dualDisplayEnabled && espNowInitialized) {
      sendConfigPushToAllPeers();
    }

    // Forza ridisegno completo del display (il gfx pointer potrebbe essere cambiato)
    extern Arduino_GFX *gfx;
    gfx->fillScreen(BLACK);
    resetModeInitFlags();

    Serial.printf("[DUAL-WEB] Config salvata: enabled=%d pos=(%d,%d) grid=%dx%d role=%d\n",
                  dualDisplayEnabled, panelX, panelY, gridW, gridH, panelRole);

    String json = "{\"success\":true,";
    json += "\"enabled\":" + String(dualDisplayEnabled ? "true" : "false") + ",";
    json += "\"panelX\":" + String(panelX) + ",";
    json += "\"panelY\":" + String(panelY) + ",";
    json += "\"gridW\":" + String(gridW) + ",";
    json += "\"gridH\":" + String(gridH) + ",";
    json += "\"role\":" + String(panelRole) + ",";
    json += "\"roleName\":\"" + roleName(panelRole) + "\"";
    json += "}";

    request->send(200, "application/json", json);
  });

  // POST /dualdisplay/addpeer - Aggiunge un peer tramite MAC address
  // Parametro form-encoded: mac (formato "AA:BB:CC:DD:EE:FF")
  server->on("/dualdisplay/addpeer", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("[DUAL-WEB] Richiesta aggiunta peer");

    if (!request->hasParam("mac", true)) {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Parametro 'mac' mancante\"}");
      return;
    }

    String macStr = request->getParam("mac", true)->value();
    macStr.trim();

    if (peerCount >= 3) {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Max 3 peer raggiunto\"}");
      return;
    }

    uint8_t mac[6];
    if (!parseMacString(macStr, mac)) {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Formato MAC non valido\"}");
      return;
    }

    // Verifica duplicati
    for (uint8_t i = 0; i < peerCount; i++) {
      if (memcmp(peerMACs[i], mac, 6) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Peer gia' presente\"}");
        return;
      }
    }

    addPeerPanel(mac);
    saveDualDisplaySettings();

    Serial.printf("[DUAL-WEB] Peer aggiunto: %s (totale: %d)\n", macStr.c_str(), peerCount);

    String json = "{\"success\":true,\"mac\":\"" + macToString(mac) + "\",\"peerCount\":" + String(peerCount) + "}";
    request->send(200, "application/json", json);
  });

  // POST /dualdisplay/removepeer - Rimuove un peer tramite indice
  // Parametro form-encoded: index (0-2)
  server->on("/dualdisplay/removepeer", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("index", true)) {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Parametro 'index' mancante\"}");
      return;
    }

    int index = request->getParam("index", true)->value().toInt();
    if (index < 0 || index >= (int)peerCount) {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Indice fuori range\"}");
      return;
    }

    String removedMAC = macToString(peerMACs[index]);
    removePeerPanel(index);
    saveDualDisplaySettings();

    Serial.printf("[DUAL-WEB] Peer rimosso: %s (rimanenti: %d)\n", removedMAC.c_str(), peerCount);

    String json = "{\"success\":true,\"removedMAC\":\"" + removedMAC + "\",\"peerCount\":" + String(peerCount) + "}";
    request->send(200, "application/json", json);
  });

  // POST /dualdisplay/scan - Scansiona pannelli vicini via ESP-NOW
  server->on("/dualdisplay/scan", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("[DUAL-WEB] Avvio scansione pannelli");

    uint8_t before = peerCount;
    scanForPanels();
    uint8_t newPeers = peerCount - before;

    if (newPeers > 0) saveDualDisplaySettings();

    String json = "{\"success\":true,\"newPeersFound\":" + String(newPeers);
    json += ",\"peerCount\":" + String(peerCount);
    json += ",\"peers\":[";
    for (uint8_t i = 0; i < peerCount; i++) {
      if (i > 0) json += ",";
      json += "{\"mac\":\"" + macToString(peerMACs[i]) + "\",\"index\":" + String(i) + "}";
    }
    json += "]}";

    request->send(200, "application/json", json);
  });

  // GET /dualdisplay/discover - Endpoint di auto-discovery
  server->on("/dualdisplay/discover", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[DUAL-WEB] Discovery da %s\n", request->client()->remoteIP().toString().c_str());

    uint8_t myMAC[6];
    WiFi.macAddress(myMAC);

    String json = "{";
    json += "\"mac\":\"" + macToString(myMAC) + "\",";
    json += "\"role\":" + String(panelRole) + ",";
    json += "\"roleName\":\"" + roleName(panelRole) + "\",";
    json += "\"panelX\":" + String(panelX) + ",";
    json += "\"panelY\":" + String(panelY) + ",";
    json += "\"gridW\":" + String(gridW) + ",";
    json += "\"gridH\":" + String(gridH) + ",";
    json += "\"enabled\":" + String(dualDisplayEnabled ? "true" : "false");
    json += "}";

    request->send(200, "application/json", json);
  });

  // POST /dualdisplay/test - Invia pacchetto test sync a tutti i peer
  // Usa flag volatile per esecuzione thread-safe nel loop principale
  server->on("/dualdisplay/test", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("[DUAL-WEB] Test sync richiesto");

    if (!dualDisplayEnabled || !dualDisplayInitialized) {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Dual display non attivo\"}");
      return;
    }

    if (peerCount == 0) {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Nessun peer registrato\"}");
      return;
    }

    // Imposta flag - il loop principale inviera' il sync (thread-safe)
    dualTestSyncRequested = true;

    Serial.printf("[DUAL-WEB] Test sync schedulato per %d peer\n", peerCount);
    String json = "{\"success\":true,\"sentTo\":" + String(peerCount) + "}";
    request->send(200, "application/json", json);
  });

  // POST /dualdisplay/flash - Lampeggia il display locale per identificazione
  // Usa flag volatile per esecuzione thread-safe nel loop principale (SPI non e' thread-safe)
  server->on("/dualdisplay/flash", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("[DUAL-WEB] Flash pannello richiesto");

    // Imposta flag - il loop principale fara' il flash (thread-safe con SPI)
    dualFlashRequested = true;

    request->send(200, "application/json", "{\"success\":true}");
  });

  // POST /dualdisplay/reset - Reset completo configurazione dual display
  server->on("/dualdisplay/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("[DUAL-WEB] Reset completo dual display");
    resetDualDisplay();
    request->send(200, "application/json", "{\"success\":true}");
  });

  Serial.println("[DUAL-WEB] Webserver dual display inizializzato (10 endpoint)");
}
