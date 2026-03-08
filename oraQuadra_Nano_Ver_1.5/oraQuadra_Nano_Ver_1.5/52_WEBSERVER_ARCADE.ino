// ============================================================================
// 52_WEBSERVER_ARCADE.ino - Web API for Arcade mode
// Endpoints: /arcade (HTML), /arcade/status, /arcade/start, /arcade/stop,
//            /arcade/togglegame, /arcade/roms/list, /arcade/roms/upload,
//            /arcade/roms/delete, /arcade/roms/refresh
// ============================================================================

#ifdef EFFECT_ARCADE

#include "arcade_web_html.h"

// Static upload state for ROM file upload (same pattern as MP3 upload)
static File arcadeUploadFile;
static String arcadeUploadPath;
static bool arcadeUploadValid = false;
static String arcadeLastUploaded;

// Estrae il nome file/cartella dall'ultimo '/' di un path
// ESP32 SD: File.name() puo' ritornare "/ARCADE/PACMAN" oppure "PACMAN"
static String arcadeBaseName(const char* path) {
  String s = path;
  int i = s.lastIndexOf('/');
  return (i >= 0 && i < (int)s.length() - 1) ? s.substring(i + 1) : s;
}

void setup_arcade_webserver(AsyncWebServer* server) {
  Serial.println("[ARCADE WEBSERVER] Registering endpoints...");

  // IMPORTANT: HTML page handler is registered LAST (at end of function)
  // because ESPAsyncWebServer treats "/arcade" as prefix match for "/arcade/*"

  // JSON status endpoint (returns 404 if SD card not available)
  server->on("/arcade/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    extern bool sdCardPresent;
    if (!sdCardPresent || !SD.exists("/ARCADE")) {
      request->send(404, "application/json", "{\"error\":\"SD card with /ARCADE folder required\"}");
      return;
    }
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
      if (!arcadeGames[gameIdx].enabled) {
        request->send(400, "application/json", "{\"error\":\"Game is disabled\"}");
        return;
      }
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

  // ===== Toggle game enabled/disabled =====
  server->on("/arcade/togglegame", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("idx")) {
      request->send(400, "application/json", "{\"error\":\"Missing idx\"}");
      return;
    }
    int idx = request->getParam("idx")->value().toInt();
    bool en = true;
    if (request->hasParam("enabled")) {
      en = request->getParam("enabled")->value() == "true" || request->getParam("enabled")->value() == "1";
    }

    if (idx < 0 || idx >= arcadeMachineCount) {
      request->send(400, "application/json", "{\"error\":\"Invalid index\"}");
      return;
    }

    setArcadeGameEnabled(idx, en);
    arcadeGames[idx].enabled = en;

    // Redraw menu if currently visible
    if (currentMode == MODE_ARCADE && arcadeInMenu) {
      arcadeDrawMenu();
    }

    String resp = "{\"ok\":true,\"idx\":" + String(idx) + ",\"enabled\":" + String(en ? "true" : "false") + "}";
    request->send(200, "application/json", resp);
  });

  // ===== ROM Manager: List folders/files on SD =====
  // Scansione dinamica: elenca TUTTE le sottocartelle in /ARCADE/ (non solo le note)
  server->on("/arcade/roms/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[ARCADE-ROMS] /arcade/roms/list called");

    // Pre-check: is SD available?
    if (SD.cardType() == CARD_NONE) {
      Serial.println("[ARCADE-ROMS] ERROR: No SD card!");
      request->send(200, "application/json", "{\"folders\":[],\"sdTotal\":0,\"sdUsed\":0,\"sdFree\":0,\"error\":\"No SD card\"}");
      return;
    }

    String json;
    json.reserve(2048);
    json = "{\"folders\":[";
    bool firstFolder = true;

    // Scan /ARCADE/ directory for ALL subdirectories
    File arcadeRoot = SD.open("/ARCADE");
    if (arcadeRoot && arcadeRoot.isDirectory()) {
      File sub = arcadeRoot.openNextFile();
      while (sub) {
        if (sub.isDirectory()) {
          String folderName = arcadeBaseName(sub.name());

          if (!firstFolder) json += ",";
          firstFolder = false;
          json += "{\"name\":\"";
          json += folderName;
          json += "\",\"files\":[";

          Serial.printf("[ARCADE-ROMS] Scanning folder: %s\n", folderName.c_str());

          bool firstFile = true;
          File f = sub.openNextFile();
          while (f) {
            if (!f.isDirectory()) {
              if (!firstFile) json += ",";
              firstFile = false;
              String fname = arcadeBaseName(f.name());
              size_t fsize = f.size();
              json += "{\"name\":\"";
              json += fname;
              json += "\",\"size\":";
              json += String(fsize);
              json += "}";
              Serial.printf("[ARCADE-ROMS]   File: %s (%u bytes)\n", fname.c_str(), fsize);
            }
            f.close();
            f = sub.openNextFile();
          }
          json += "]}";
        }
        sub.close();
        sub = arcadeRoot.openNextFile();
      }
      arcadeRoot.close();
    } else {
      Serial.println("[ARCADE-ROMS] /ARCADE directory not found");
      if (arcadeRoot) arcadeRoot.close();
    }

    // SD card space info
    uint64_t sdTotal = SD.totalBytes();
    uint64_t sdUsed = SD.usedBytes();
    uint64_t sdFree = sdTotal > sdUsed ? sdTotal - sdUsed : 0;

    json += "],\"sdTotal\":";
    json += String((uint32_t)(sdTotal / 1024));
    json += ",\"sdUsed\":";
    json += String((uint32_t)(sdUsed / 1024));
    json += ",\"sdFree\":";
    json += String((uint32_t)(sdFree / 1024));
    json += "}";

    Serial.printf("[ARCADE-ROMS] JSON length: %d bytes\n", json.length());
    request->send(200, "application/json", json);
  });

  // ===== Simple test endpoint - verifica che il webserver funziona =====
  server->on("/arcade/roms/test", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[ARCADE-ROMS] /arcade/roms/test called");
    request->send(200, "text/plain", "ARCADE ROM endpoints OK");
  });

  // ===== Debug: test rapido lettura SD =====
  server->on("/arcade/roms/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[ARCADE-ROMS] /arcade/roms/debug called");

    String out;
    out.reserve(1024);

    out += "=== ARCADE ROM DEBUG ===\n";
    out += "SD cardType=" + String(SD.cardType()) + "\n";

    if (SD.cardType() == CARD_NONE) {
      out += "ERRORE: Nessuna SD card rilevata!\n";
      request->send(200, "text/plain", out);
      return;
    }

    out += "totalBytes=" + String((uint32_t)(SD.totalBytes() / 1024)) + "KB\n";
    out += "usedBytes=" + String((uint32_t)(SD.usedBytes() / 1024)) + "KB\n";

    bool arcadeExists = SD.exists("/ARCADE");
    out += "exists /ARCADE = " + String(arcadeExists ? "YES" : "NO") + "\n";

    if (!arcadeExists) {
      out += "\nLa cartella /ARCADE non esiste sulla SD.\n";
      out += "Premi 'Dump ROM Firmware su SD' per crearla.\n";
      request->send(200, "text/plain", out);
      return;
    }

    File root = SD.open("/ARCADE");
    if (!root) {
      out += "ERRORE: impossibile aprire /ARCADE\n";
      request->send(200, "text/plain", out);
      return;
    }
    if (!root.isDirectory()) {
      out += "ERRORE: /ARCADE non e' una directory\n";
      root.close();
      request->send(200, "text/plain", out);
      return;
    }

    File entry = root.openNextFile();
    int count = 0;
    while (entry) {
      out += (entry.isDirectory() ? "[DIR] " : "[FILE] ");
      out += String(entry.name()) + " size=" + String(entry.size()) + "\n";
      count++;
      entry.close();
      entry = root.openNextFile();
    }
    root.close();
    out += "Totale entries in /ARCADE: " + String(count) + "\n";

    // Prova una cartella specifica
    out += "\n--- Test /ARCADE/PACMAN ---\n";
    File test = SD.open("/ARCADE/PACMAN");
    if (!test) {
      out += "/ARCADE/PACMAN: non esiste\n";
    } else {
      out += "isDir=" + String(test.isDirectory() ? "YES" : "NO") + "\n";
      if (test.isDirectory()) {
        File f = test.openNextFile();
        while (f) {
          out += "  " + String(f.name()) + " size=" + String(f.size()) + "\n";
          f.close();
          f = test.openNextFile();
        }
      }
      test.close();
    }

    Serial.printf("[ARCADE-ROMS] Debug output: %d bytes\n", out.length());
    request->send(200, "text/plain", out);
  });

  // ===== ROM Manager: Upload file to SD =====
  server->on("/arcade/roms/upload", HTTP_POST,
    // Completion handler
    [](AsyncWebServerRequest *request) {
      String resp;
      if (arcadeLastUploaded.length() > 0) {
        resp = "{\"success\":true,\"filename\":\"" + arcadeLastUploaded + "\"}";
      } else {
        resp = "{\"success\":false,\"error\":\"Upload fallito\"}";
      }
      request->send(200, "application/json", resp);
    },
    // Upload data handler (chunked)
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (index == 0) {
        // First chunk: validate and open file
        Serial.printf("[ARCADE-ROM] Upload start: %s\n", filename.c_str());

        // Only accept .bin files
        String lower = filename;
        lower.toLowerCase();
        if (!lower.endsWith(".bin")) {
          Serial.println("[ARCADE-ROM] Error: only .bin accepted");
          arcadeLastUploaded = "";
          arcadeUploadValid = false;
          return;
        }

        // Get folder from query param
        String folder = "MISC";
        if (request->hasParam("folder")) {
          folder = request->getParam("folder")->value();
        }

        // Path traversal protection
        if (folder.indexOf("..") >= 0 || filename.indexOf("..") >= 0) {
          Serial.println("[ARCADE-ROM] Error: path traversal rejected");
          arcadeLastUploaded = "";
          arcadeUploadValid = false;
          return;
        }

        // Create directories
        if (!SD.exists("/ARCADE")) SD.mkdir("/ARCADE");
        String dirPath = "/ARCADE/" + folder;
        if (!SD.exists(dirPath.c_str())) SD.mkdir(dirPath.c_str());

        arcadeUploadPath = dirPath + "/" + filename;
        arcadeUploadFile = SD.open(arcadeUploadPath.c_str(), FILE_WRITE);
        if (!arcadeUploadFile) {
          Serial.println("[ARCADE-ROM] Error: cannot open file for writing");
          arcadeLastUploaded = "";
          arcadeUploadValid = false;
          return;
        }
        arcadeUploadValid = true;
        arcadeLastUploaded = filename;
      }

      // Write chunk
      if (arcadeUploadValid && arcadeUploadFile && len) {
        arcadeUploadFile.write(data, len);
        yield();
      }

      if (final) {
        if (arcadeUploadValid && arcadeUploadFile) {
          arcadeUploadFile.flush();
          arcadeUploadFile.close();
          Serial.printf("[ARCADE-ROM] Upload complete: %s (%u bytes)\n", arcadeUploadPath.c_str(), index + len);
        } else {
          arcadeLastUploaded = "";
        }
        arcadeUploadValid = false;
      }
    }
  );

  // ===== ROM Manager: Delete file or folder =====
  server->on("/arcade/roms/delete", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("path")) {
      request->send(400, "application/json", "{\"error\":\"Missing path\"}");
      return;
    }

    String relPath = request->getParam("path")->value();

    // Path traversal protection
    if (relPath.indexOf("..") >= 0) {
      request->send(400, "application/json", "{\"error\":\"Invalid path\"}");
      return;
    }

    String fullPath = "/ARCADE/" + relPath;

    if (!SD.exists(fullPath.c_str())) {
      request->send(404, "application/json", "{\"error\":\"Not found\"}");
      return;
    }

    File target = SD.open(fullPath.c_str());
    if (target.isDirectory()) {
      // Delete all files in directory first
      File f = target.openNextFile();
      while (f) {
        String fPath = fullPath + "/" + String(f.name());
        f.close();
        SD.remove(fPath.c_str());
        f = target.openNextFile();
      }
      target.close();
      SD.rmdir(fullPath.c_str());
      Serial.printf("[ARCADE-ROM] Deleted folder: %s\n", fullPath.c_str());
    } else {
      target.close();
      SD.remove(fullPath.c_str());
      Serial.printf("[ARCADE-ROM] Deleted file: %s\n", fullPath.c_str());
    }

    request->send(200, "application/json", "{\"ok\":true}");
  });

  // ===== Refresh game list after ROM upload/delete =====
  server->on("/arcade/roms/refresh", HTTP_POST, [](AsyncWebServerRequest *request) {
    extern int8_t arcadeMachineCount;
    extern void arcadeBuildGameList();
    extern void loadArcadeEnabledMask();
    extern ArcadeGameInfo arcadeGames[];
    extern uint16_t arcadeEnabledMask;
    extern bool arcadeInMenu;

    // Rebuild game list from SD card
    arcadeBuildGameList();
    loadArcadeEnabledMask();
    for (int i = 0; i < arcadeMachineCount; i++) {
      arcadeGames[i].enabled = (arcadeEnabledMask >> i) & 1;
    }

    // Redraw menu on display if currently showing menu
    if (arcadeInMenu) {
      extern void arcadeDrawMenu();
      arcadeDrawMenu();
    }

    String resp = "{\"ok\":true,\"gameCount\":" + String(arcadeMachineCount) + "}";
    request->send(200, "application/json", resp);
    Serial.printf("[ARCADE] Game list refreshed: %d games found\n", arcadeMachineCount);
  });

  // HTML page - MUST be registered LAST because "/arcade" prefix-matches "/arcade/*"
  // Guard: SD card required for arcade (ROMs are loaded from SD only)
  server->on("/arcade", HTTP_GET, [](AsyncWebServerRequest *request) {
    extern bool sdCardPresent;
    if (!sdCardPresent || !SD.exists("/ARCADE")) {
      request->send(404, "text/plain", "SD card with /ARCADE folder required");
      return;
    }
    request->send_P(200, "text/html", ARCADE_HTML);
  });

  Serial.println("[ARCADE WEBSERVER] Endpoints registered");
}

#endif // EFFECT_ARCADE
