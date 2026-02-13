// ================== WEB SERVER YOUTUBE STATS ==================
// Endpoint per pagina web YouTube e API status JSON
// GET /youtube       -> Pagina HTML
// GET /youtube/status -> JSON con statistiche canale

#include "youtube_web_html.h"

#ifdef EFFECT_YOUTUBE

extern String ytChannelName;
extern long   ytSubscribers;
extern long   ytViews;
extern long   ytVideos;
extern String ytLastUpdate;
extern String ytError;

void setup_youtube_webserver(AsyncWebServer* server) {
  // Endpoint status JSON (registrato PRIMA di /youtube per evitare conflitti)
  server->on("/youtube/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"channelName\":\"" + ytChannelName + "\",";
    json += "\"subscribers\":" + String(ytSubscribers) + ",";
    json += "\"views\":" + String(ytViews) + ",";
    json += "\"videos\":" + String(ytVideos) + ",";
    json += "\"lastUpdate\":\"" + ytLastUpdate + "\",";
    json += "\"error\":\"" + ytError + "\"";
    json += "}";
    request->send(200, "application/json", json);
  });

  // Pagina HTML (registrato DOPO /youtube/status)
  server->on("/youtube", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", YOUTUBE_HTML);
  });

  Serial.println("[YOUTUBE] Webserver endpoints registrati (/youtube, /youtube/status)");
}

#endif // EFFECT_YOUTUBE
