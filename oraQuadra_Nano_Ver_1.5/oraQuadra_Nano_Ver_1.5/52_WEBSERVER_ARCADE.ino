// ============================================================================
// 52_WEBSERVER_ARCADE.ino - Web API for Arcade mode
// Endpoints: /arcade (HTML), /arcade/status (JSON), /arcade/start, /arcade/stop
// ============================================================================

#ifdef EFFECT_ARCADE

#include "arcade_web_html.h"

void setup_arcade_webserver(AsyncWebServer* server) {
  Serial.println("[ARCADE WEBSERVER] Registering endpoints...");

  // HTML page
  server->on("/arcade", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", ARCADE_HTML);
  });

  // JSON status endpoint
  server->on("/arcade/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"inMenu\":" + String(arcadeInMenu ? "true" : "false") + ",";
    json += "\"selectedGame\":" + String(arcadeSelectedGame) + ",";
    json += "\"gameName\":\"" + String(arcadeSelectedGame >= 0 ? arcadeGames[arcadeSelectedGame].name : "None") + "\",";
    json += "\"frames\":" + String(arcadeFrameCount) + ",";
    json += "\"gameCount\":" + String(arcadeMachineCount) + ",";

    // Games array
    json += "\"games\":[";
    for (int i = 0; i < arcadeMachineCount; i++) {
      if (i > 0) json += ",";
      json += "{\"name\":\"" + String(arcadeGames[i].name) + "\",";
      json += "\"type\":" + String(arcadeGames[i].machineType) + ",";
      json += "\"enabled\":" + String(arcadeGames[i].enabled ? "true" : "false") + "}";
    }
    json += "]";

    json += "}";
    request->send(200, "application/json", json);
  });

  // Start game
  server->on("/arcade/start", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (currentMode != MODE_ARCADE) {
      request->send(400, "application/json", "{\"error\":\"Not in ARCADE mode\"}");
      return;
    }

    int gameIdx = 0;
    if (request->hasParam("game")) {
      gameIdx = request->getParam("game")->value().toInt();
    }

    if (gameIdx >= 0 && gameIdx < arcadeMachineCount) {
      if (!arcadeInMenu) {
        arcadeStopGame();
        delay(100);
      }
      arcadeStartGame(gameIdx);
      request->send(200, "application/json", "{\"ok\":true,\"game\":\"" + String(arcadeGames[gameIdx].name) + "\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"Invalid game index\"}");
    }
  });

  // Stop game
  server->on("/arcade/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (currentMode == MODE_ARCADE && !arcadeInMenu) {
      arcadeStopGame();
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(200, "application/json", "{\"ok\":true,\"note\":\"Already in menu\"}");
    }
  });

  Serial.println("[ARCADE WEBSERVER] Endpoints registered");
}

#endif // EFFECT_ARCADE
