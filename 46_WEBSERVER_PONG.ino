// ================== WEB SERVER PONG ==================
// Endpoint per pagina web PONG e API status JSON
// GET /pong         -> Pagina HTML con istruzioni e stato
// GET /pong/status  -> JSON con stato gioco (mode, scores, dual, role)

#include "pong_web_html.h"

#ifdef EFFECT_PONG

void setup_pong_webserver(AsyncWebServer* server) {

  // Endpoint JSON status (registrato PRIMA della pagina HTML)
  server->on("/pong/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";

    // Modo corrente
    json += "\"mode\":" + String((int)currentMode);
    json += ",\"modeName\":\"PONG\"";

    // Punteggio
    json += ",\"scoreL\":" + String(pongScoreLeft);
    json += ",\"scoreR\":" + String(pongScoreRight);
    json += ",\"gameOver\":" + String(pongGameOver ? "true" : "false");

    // Stato Dual Display
#ifdef EFFECT_DUAL_DISPLAY
    json += ",\"dualActive\":" + String(isDualDisplayActive() ? "true" : "false");
    json += ",\"role\":" + String(panelRole);
    json += ",\"gridW\":" + String(gridW);
    json += ",\"gridH\":" + String(gridH);
#else
    json += ",\"dualActive\":false";
    json += ",\"role\":0";
    json += ",\"gridW\":1";
    json += ",\"gridH\":1";
#endif

    json += "}";
    request->send(200, "application/json", json);
  });

  // Pagina HTML (registrato DOPO /pong/status)
  server->on("/pong", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", PONG_HTML);
  });

  Serial.println("[PONG] Webserver endpoints registrati (/pong, /pong/status)");
}

#endif // EFFECT_PONG
