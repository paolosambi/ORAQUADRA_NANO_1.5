// ================== WEB SERVER OTA UPDATE ==================
// Endpoint per aggiornamento firmware e LittleFS via browser
// GET  /update            -> Pagina HTML
// POST /update/firmware   -> Upload firmware (.bin)
// POST /update/littlefs   -> Upload LittleFS (.bin)

#include "ota_web_html.h"

void setup_ota_webserver(AsyncWebServer* server) {
  // Pagina HTML
  server->on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", OTA_HTML);
  });

  // Upload Firmware
  server->on("/update/firmware", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      bool success = !Update.hasError();
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain",
        success ? "OK" : "FAIL");
      response->addHeader("Connection", "close");
      request->send(response);
      if (success) {
        delay(500);
        ESP.restart();
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index,
       uint8_t *data, size_t len, bool final) {
      if (!index) {
        Serial.printf("[OTA] Firmware upload start: %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
          Update.printError(Serial);
        }
      }
      if (Update.isRunning()) {
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
        }
      }
      if (final) {
        if (Update.end(true)) {
          Serial.printf("[OTA] Firmware upload OK: %u bytes\n", index + len);
        } else {
          Update.printError(Serial);
        }
      }
    }
  );

  // Upload LittleFS
  server->on("/update/littlefs", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      bool success = !Update.hasError();
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain",
        success ? "OK" : "FAIL");
      response->addHeader("Connection", "close");
      request->send(response);
      if (success) {
        delay(500);
        ESP.restart();
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index,
       uint8_t *data, size_t len, bool final) {
      if (!index) {
        Serial.printf("[OTA] LittleFS upload start: %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
          Update.printError(Serial);
        }
      }
      if (Update.isRunning()) {
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
        }
      }
      if (final) {
        if (Update.end(true)) {
          Serial.printf("[OTA] LittleFS upload OK: %u bytes\n", index + len);
        } else {
          Update.printError(Serial);
        }
      }
    }
  );

  Serial.println("[WEBSERVER] OTA Update disponibile su /update");
}
