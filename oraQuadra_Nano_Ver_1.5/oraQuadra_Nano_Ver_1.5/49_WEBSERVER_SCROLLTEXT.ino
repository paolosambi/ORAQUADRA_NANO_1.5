// ================== WEB SERVER SCROLL TEXT ==================
// Endpoint per pagina web ScrollText e API configurazione
// GET /scrolltext          -> Pagina HTML
// GET /scrolltext/status   -> JSON con tutti i messaggi e stato
// GET /scrolltext/set      -> Salva singolo messaggio
// GET /scrolltext/global   -> Impostazioni globali
// GET /scrolltext/delete   -> Elimina messaggio

#include "scrolltext_web_html.h"

#ifdef EFFECT_SCROLLTEXT

extern bool scrollTextInitialized;
extern ScrollMessage scrollMessages[];
extern uint8_t scrollMessageCount;
extern uint8_t scrollCurrentMsg;
extern bool scrollPaused;
extern uint32_t scrollRotationInterval;
extern uint8_t scrollBgR, scrollBgG, scrollBgB;
extern bool scrollShowTime;
extern bool scrollShowDate;
extern void saveScrollTextToFS();
extern void loadScrollTextFromFS();
extern void scrollResetPosition();
#ifdef EFFECT_DUAL_DISPLAY
extern void scrollSendConfigToSlaves();
#endif

void setup_scrolltext_webserver(AsyncWebServer* server) {

  // Endpoint status JSON (registrato PRIMA di /scrolltext per evitare conflitti)
  server->on("/scrolltext/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"count\":" + String(scrollMessageCount) + ",";
    json += "\"current\":" + String(scrollCurrentMsg) + ",";
    json += "\"paused\":" + String(scrollPaused ? "true" : "false") + ",";
    json += "\"rotation\":" + String(scrollRotationInterval) + ",";
    json += "\"bgR\":" + String(scrollBgR) + ",";
    json += "\"bgG\":" + String(scrollBgG) + ",";
    json += "\"bgB\":" + String(scrollBgB) + ",";
    json += "\"showTime\":" + String(scrollShowTime ? 1 : 0) + ",";
    json += "\"showDate\":" + String(scrollShowDate ? 1 : 0) + ",";

    json += "\"msgs\":[";
    for (int i = 0; i < scrollMessageCount; i++) {
      if (i > 0) json += ",";
      json += "{\"text\":\"";
      // Escape JSON nel testo
      for (int c = 0; scrollMessages[i].text[c]; c++) {
        char ch = scrollMessages[i].text[c];
        if (ch == '"') json += "\\\"";
        else if (ch == '\\') json += "\\\\";
        else if (ch == '\n') json += "\\n";
        else json += ch;
      }
      json += "\",\"font\":" + String(scrollMessages[i].fontIndex);
      json += ",\"sc\":" + String(scrollMessages[i].scale);
      json += ",\"r\":" + String(scrollMessages[i].colorR);
      json += ",\"g\":" + String(scrollMessages[i].colorG);
      json += ",\"b\":" + String(scrollMessages[i].colorB);
      json += ",\"speed\":" + String(scrollMessages[i].speed);
      json += ",\"dir\":" + String(scrollMessages[i].direction);
      json += ",\"fx\":" + String(scrollMessages[i].effects);
      json += ",\"on\":" + String(scrollMessages[i].enabled ? 1 : 0);
      json += "}";
    }
    json += "]}";

    request->send(200, "application/json", json);
  });

  // Endpoint set singolo messaggio
  server->on("/scrolltext/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("idx")) {
      request->send(400, "text/plain", "Missing idx");
      return;
    }

    int idx = request->getParam("idx")->value().toInt();
    if (idx < 0 || idx >= MAX_SCROLL_MESSAGES) {
      request->send(400, "text/plain", "Invalid idx");
      return;
    }

    // Espandi array se necessario
    if (idx >= scrollMessageCount) {
      scrollMessageCount = idx + 1;
    }

    ScrollMessage& msg = scrollMessages[idx];

    if (request->hasParam("text")) {
      String text = request->getParam("text")->value();
      strncpy(msg.text, text.c_str(), MAX_SCROLL_TEXT_LEN);
      msg.text[MAX_SCROLL_TEXT_LEN] = '\0';
    }
    if (request->hasParam("font")) {
      msg.fontIndex = request->getParam("font")->value().toInt();
      if (msg.fontIndex >= SCROLL_FONT_COUNT) msg.fontIndex = 0;
    }
    if (request->hasParam("sc")) {
      msg.scale = request->getParam("sc")->value().toInt();
      if (msg.scale < 1) msg.scale = 1;
      if (msg.scale > 20) msg.scale = 20;
    }
    if (request->hasParam("r")) msg.colorR = request->getParam("r")->value().toInt();
    if (request->hasParam("g")) msg.colorG = request->getParam("g")->value().toInt();
    if (request->hasParam("b")) msg.colorB = request->getParam("b")->value().toInt();
    if (request->hasParam("speed")) {
      msg.speed = request->getParam("speed")->value().toInt();
      if (msg.speed < 1) msg.speed = 1;
      if (msg.speed > 10) msg.speed = 10;
    }
    if (request->hasParam("dir")) {
      msg.direction = request->getParam("dir")->value().toInt();
      if (msg.direction > 1) msg.direction = 0;
    }
    if (request->hasParam("fx")) msg.effects = request->getParam("fx")->value().toInt();
    if (request->hasParam("on")) msg.enabled = (request->getParam("on")->value().toInt() != 0);

    saveScrollTextToFS();
#ifdef EFFECT_DUAL_DISPLAY
    scrollSendConfigToSlaves();
#endif

    // Reset display se siamo in MODE_SCROLLTEXT
    if (currentMode == MODE_SCROLLTEXT) {
      scrollTextInitialized = false;
    }

    request->send(200, "application/json", "{\"ok\":true}");
    Serial.printf("[SCROLLTEXT WEB] Msg %d salvato: %s\n", idx, msg.text);
  });

  // Endpoint impostazioni globali
  server->on("/scrolltext/global", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("count")) {
      int cnt = request->getParam("count")->value().toInt();
      if (cnt >= 0 && cnt <= MAX_SCROLL_MESSAGES) {
        scrollMessageCount = cnt;
      }
    }
    if (request->hasParam("rotation")) {
      scrollRotationInterval = request->getParam("rotation")->value().toInt();
      if (scrollRotationInterval < 3000) scrollRotationInterval = 3000;
      if (scrollRotationInterval > 300000) scrollRotationInterval = 300000;
    }
    if (request->hasParam("bgR")) scrollBgR = request->getParam("bgR")->value().toInt();
    if (request->hasParam("bgG")) scrollBgG = request->getParam("bgG")->value().toInt();
    if (request->hasParam("bgB")) scrollBgB = request->getParam("bgB")->value().toInt();
    if (request->hasParam("showTime")) scrollShowTime = (request->getParam("showTime")->value().toInt() != 0);
    if (request->hasParam("showDate")) scrollShowDate = (request->getParam("showDate")->value().toInt() != 0);

    saveScrollTextToFS();
#ifdef EFFECT_DUAL_DISPLAY
    scrollSendConfigToSlaves();
#endif

    if (currentMode == MODE_SCROLLTEXT) {
      scrollTextInitialized = false;
    }

    request->send(200, "application/json", "{\"ok\":true}");
    Serial.printf("[SCROLLTEXT WEB] Globali: count=%d rot=%d bg=%d,%d,%d\n",
                  scrollMessageCount, scrollRotationInterval, scrollBgR, scrollBgG, scrollBgB);
  });

  // Endpoint elimina messaggio
  server->on("/scrolltext/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("idx")) {
      request->send(400, "text/plain", "Missing idx");
      return;
    }

    int idx = request->getParam("idx")->value().toInt();
    if (idx < 0 || idx >= scrollMessageCount) {
      request->send(400, "text/plain", "Invalid idx");
      return;
    }

    // Shift messaggi
    for (int i = idx; i < scrollMessageCount - 1; i++) {
      scrollMessages[i] = scrollMessages[i + 1];
    }
    scrollMessageCount--;
    memset(&scrollMessages[scrollMessageCount], 0, sizeof(ScrollMessage));

    if (scrollCurrentMsg >= scrollMessageCount && scrollMessageCount > 0) {
      scrollCurrentMsg = 0;
    }

    saveScrollTextToFS();
#ifdef EFFECT_DUAL_DISPLAY
    scrollSendConfigToSlaves();
#endif

    if (currentMode == MODE_SCROLLTEXT) {
      scrollTextInitialized = false;
    }

    request->send(200, "application/json", "{\"ok\":true}");
    Serial.printf("[SCROLLTEXT WEB] Msg %d eliminato, count=%d\n", idx, scrollMessageCount);
  });

  // Pagina HTML (registrata PER ULTIMA per evitare conflitti con sub-path)
  server->on("/scrolltext", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", SCROLLTEXT_HTML);
  });
}

#endif // EFFECT_SCROLLTEXT
