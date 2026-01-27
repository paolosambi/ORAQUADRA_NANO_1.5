// ================== WEB SERVER PER CONFIGURAZIONE COMPLETA ORAQUADRA NANO ==================
// Interfaccia web unificata per modificare TUTTE le impostazioni del dispositivo
// Organizzate per tipologia: Display, Orari, Audio, Effetti, Sensori, Rete, Sistema
// Include attivazione/disattivazione delle singole modalità display
//
// Accesso: http://<IP_ESP32>:8080/settings
//

#include "settings_web_html.h"
#include "language_config.h"

// ================== SISTEMA MULTILINGUA ==================
#ifdef ENABLE_MULTILANGUAGE
// NOTA: La variabile currentLanguage è definita nel file principale oraQuadraNano_V1_4_I2C1.ino
// Qui implementiamo solo le funzioni di gestione

// Inizializza la lingua (carica da EEPROM o usa default)
void initLanguage() {
    uint8_t saved = EEPROM.read(EEPROM_LANGUAGE_ADDR);
    if (saved < LANG_COUNT) {
        currentLanguage = (Language)saved;
    } else {
        currentLanguage = LANG_IT;  // Default italiano
    }
    Serial.printf("[LANGUAGE] Lingua caricata: %s\n",
                  currentLanguage == LANG_IT ? "Italiano" : "English");
}

// Imposta la lingua corrente e salva in EEPROM
void setLanguage(Language lang) {
    if (lang < LANG_COUNT) {
        currentLanguage = lang;
        EEPROM.write(EEPROM_LANGUAGE_ADDR, (uint8_t)lang);
        EEPROM.commit();
        Serial.printf("[LANGUAGE] Lingua impostata: %s\n",
                      lang == LANG_IT ? "Italiano" : "English");
    }
}

// Ottiene la lingua corrente
Language getLanguage() {
    return currentLanguage;
}

// Cicla alla lingua successiva (per menu)
void nextLanguage() {
    uint8_t next = ((uint8_t)currentLanguage + 1) % LANG_COUNT;
    setLanguage((Language)next);
}

// Ottiene il nome della lingua corrente
const char* getLanguageName() {
    return LANGUAGE_NAMES[currentLanguage];
}

// Ottiene il codice breve della lingua corrente
const char* getLanguageCode() {
    return LANGUAGE_CODES[currentLanguage];
}
#endif // ENABLE_MULTILANGUAGE

// Dichiarazione extern per funzione audio I2C (definita in 7_AUDIO_WIFI.ino)
extern bool setVolumeViaI2C(uint8_t volume);

// Dichiarazione extern per ESP32-CAM stream URL (definita in 17_ESP32CAM.ino)
#ifdef EFFECT_ESP32CAM
extern String esp32camStreamUrl;
#endif

// Dichiarazioni extern per meteo esterno (definite in 19_WEATHER.ino)
extern float weatherTempOutdoor;
extern float weatherHumOutdoor;
extern bool weatherDataValid;

// Dichiarazioni extern per magnetometro QMC5883P (definite in 25_MAGNETOMETER.ino)
extern bool magnetometerConnected;
extern float magnetometerHeading;
extern bool magCalibrated;
extern const char* getCardinalDirection(float heading);

// Dichiarazioni extern per radar remoto (definite in 15_WEBSERVER_RADAR.ino)
extern bool radarServerEnabled;
extern bool radarServerConnected;
extern bool radarRemotePresence;
extern uint8_t radarRemoteBrightness;
extern float radarRemoteTemperature;
extern float radarRemoteHumidity;
extern bool useRemoteRadar();
extern String radarServerIPString();

// Dichiarazione extern per fonte dati sensore ambiente (0=nessuno, 1=BME280 interno, 2=radar remoto)
extern uint8_t indoorSensorSource;

// Dichiarazioni extern per flag inizializzazione modalità (definite nel file principale)
extern bool matrixInitialized;
extern bool slowInitialized;
extern bool fadeInitialized;
extern volatile bool webForceDisplayUpdate;

// Dichiarazione extern per flag scena Christmas (definita in 28_CHRISTMAS.ino)
#ifdef EFFECT_CHRISTMAS
extern bool xmasSceneDrawn;
#endif

// ================== VARIABILI PER GESTIONE MODALITÀ ABILITATE ==================
// Bitmask per memorizzare quali modalità sono abilitate (salvato in EEPROM)
// Ogni bit rappresenta una modalità (bit 0 = MODE_FADE, bit 1 = MODE_SLOW, ecc.)
uint32_t enabledModesMask = 0xFFFFFFFF;  // Default: tutte abilitate

// Indirizzo EEPROM per salvare le modalità abilitate (4 bytes)
// SPOSTATO a 700+ per evitare conflitti
#define EEPROM_ENABLED_MODES_ADDR 700

// ================== RAINBOW MODE ==================
#define EEPROM_RAINBOW_MODE_ADDR 704  // 1 byte per rainbow mode (0=off, 1=on)
bool rainbowModeEnabled = false;      // Flag per modalità rainbow

// ================== COLORI PER MODALITÀ ==================
// Ogni modalità ha il suo colore salvato in EEPROM
// Modalità supportate: 0-15, 17, 19-23 = 22 modalità x 3 bytes (RGB) = 66 bytes
// SPOSTATO a 710-775 per evitare conflitti
#define EEPROM_MODE_COLORS_BASE 710

// Mappa da ID modalità a indice array (0-21)
// Include tutte le modalità che supportano preset colore
// Esclude solo 16 (GEMINI) e 18 (MJPEG) che sono disabilitate
const uint8_t TEXT_MODE_IDS[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 19, 20, 21, 22, 23};
const uint8_t NUM_TEXT_MODES = 22;

// Colori default per ogni modalità (ordine corrisponde a TEXT_MODE_IDS)
const uint8_t DEFAULT_MODE_COLORS[][3] = {
  {255, 255, 255},  // 0: FADE - Bianco
  {255, 255, 255},  // 1: SLOW - Bianco
  {255, 255, 255},  // 2: FAST - Bianco
  {0, 255, 0},      // 3: MATRIX - Verde
  {0, 255, 0},      // 4: MATRIX2 - Verde
  {255, 255, 0},    // 5: SNAKE - Giallo
  {0, 191, 255},    // 6: WATER - Azzurro
  {255, 0, 0},      // 7: MARIO - Rosso
  {0, 255, 255},    // 8: TRON - Ciano
  {0, 255, 0},      // 9: GALAGA - Verde
  {255, 128, 0},    // 10: FLUX - Arancione
  {255, 255, 255},  // 11: FLIPCLOCK - Bianco
  {255, 128, 0},    // 12: BTTF - Arancione
  {255, 255, 255},  // 13: LED_RING - Bianco
  {255, 255, 255},  // 14: WEATHER - Bianco
  {255, 255, 255},  // 15: RADAR - Bianco
  {0, 255, 0},      // 17: GALAGA2 - Verde
  {255, 255, 255},  // 19: ESP32CAM - Bianco
  {255, 128, 0},    // 20: FLUX_CAP - Arancione
  {255, 0, 0},      // 21: CHRISTMAS - Rosso
  {255, 100, 0},    // 22: FIRE - Arancione fuoco
  {255, 100, 0}     // 23: FIRE_TEXT - Arancione fuoco
};

// Converte ID modalità in indice array (0-10), ritorna -1 se non è modalità testuale
int8_t getModeColorIndex(uint8_t modeId) {
  for (uint8_t i = 0; i < NUM_TEXT_MODES; i++) {
    if (TEXT_MODE_IDS[i] == modeId) return i;
  }
  return -1;
}

// Carica colore per una modalità specifica da EEPROM
void loadModeColor(uint8_t modeId, uint8_t &r, uint8_t &g, uint8_t &b) {
  int8_t idx = getModeColorIndex(modeId);
  if (idx < 0) {
    r = g = b = 255;  // Default bianco per modalità non testuali
    return;
  }

  uint16_t addr = EEPROM_MODE_COLORS_BASE + (idx * 3);
  r = EEPROM.read(addr);
  g = EEPROM.read(addr + 1);
  b = EEPROM.read(addr + 2);

  // Se EEPROM non inizializzata (0xFF), usa default
  if (r == 255 && g == 255 && b == 255) {
    // Controlla se è veramente bianco o non inizializzato
    // Per modalità che hanno default != bianco, usa il default
    r = DEFAULT_MODE_COLORS[idx][0];
    g = DEFAULT_MODE_COLORS[idx][1];
    b = DEFAULT_MODE_COLORS[idx][2];
  }
}

// Salva colore per una modalità specifica in EEPROM
void saveModeColor(uint8_t modeId, uint8_t r, uint8_t g, uint8_t b) {
  int8_t idx = getModeColorIndex(modeId);
  if (idx < 0) return;  // Non è una modalità testuale

  uint16_t addr = EEPROM_MODE_COLORS_BASE + (idx * 3);
  EEPROM.write(addr, r);
  EEPROM.write(addr + 1, g);
  EEPROM.write(addr + 2, b);
  EEPROM.commit();
  Serial.printf("[MODE COLOR] Salvato colore mode %d: R%d G%d B%d\n", modeId, r, g, b);
}

// Reset colore di una modalità al default
void resetModeColorToDefault(uint8_t modeId) {
  int8_t idx = getModeColorIndex(modeId);
  if (idx < 0) return;

  uint8_t r = DEFAULT_MODE_COLORS[idx][0];
  uint8_t g = DEFAULT_MODE_COLORS[idx][1];
  uint8_t b = DEFAULT_MODE_COLORS[idx][2];
  saveModeColor(modeId, r, g, b);
}

// ================== PRESET PER MODALITÀ ==================
// Ogni modalità ha il suo preset salvato in EEPROM
// SPOSTATO a 780-801 per evitare conflitti
// Valore 255 = nessun preset (usa colore personalizzato), 0-14 = preset ID, 100 = rainbow
#define EEPROM_MODE_PRESETS_BASE 780
#define PRESET_NONE 255
#define PRESET_RAINBOW 100

// Preset default per ogni modalità (255 = nessun preset, usa colore custom)
// Ordine corrisponde a TEXT_MODE_IDS
const uint8_t DEFAULT_MODE_PRESETS[] = {
  PRESET_NONE,  // 0: FADE - nessun preset
  PRESET_NONE,  // 1: SLOW - nessun preset
  PRESET_NONE,  // 2: FAST - nessun preset
  PRESET_NONE,  // 3: MATRIX - nessun preset
  PRESET_NONE,  // 4: MATRIX2 - nessun preset
  PRESET_NONE,  // 5: SNAKE - nessun preset
  PRESET_NONE,  // 6: WATER - nessun preset
  PRESET_NONE,  // 7: MARIO - nessun preset
  PRESET_NONE,  // 8: TRON - nessun preset
  PRESET_NONE,  // 9: GALAGA - nessun preset
  PRESET_NONE,  // 10: FLUX - nessun preset
  PRESET_NONE,  // 11: FLIPCLOCK - nessun preset
  PRESET_NONE,  // 12: BTTF - nessun preset
  PRESET_NONE,  // 13: LED_RING - nessun preset
  PRESET_NONE,  // 14: WEATHER - nessun preset
  PRESET_NONE,  // 15: RADAR - nessun preset
  PRESET_NONE,  // 17: GALAGA2 - nessun preset
  PRESET_NONE,  // 19: ESP32CAM - nessun preset
  PRESET_NONE,  // 20: FLUX_CAP - nessun preset
  PRESET_NONE,  // 21: CHRISTMAS - nessun preset
  PRESET_NONE,  // 22: FIRE - nessun preset
  PRESET_NONE   // 23: FIRE_TEXT - nessun preset
};

// Carica preset per una modalità specifica da EEPROM
uint8_t loadModePreset(uint8_t modeId) {
  int8_t idx = getModeColorIndex(modeId);
  if (idx < 0) return PRESET_NONE;

  uint16_t addr = EEPROM_MODE_PRESETS_BASE + idx;
  uint8_t preset = EEPROM.read(addr);

  // Se EEPROM non inizializzata (0xFF), ritorna nessun preset
  if (preset == 0xFF) {
    return PRESET_NONE;
  }
  return preset;
}

// Salva preset per una modalità specifica in EEPROM
void saveModePreset(uint8_t modeId, uint8_t presetId) {
  int8_t idx = getModeColorIndex(modeId);
  if (idx < 0) return;

  uint16_t addr = EEPROM_MODE_PRESETS_BASE + idx;
  EEPROM.write(addr, presetId);
  EEPROM.commit();
  delay(10);  // Piccolo delay per assicurare scrittura flash
}

// Reset preset di una modalità al default (nessun preset)
void resetModePreset(uint8_t modeId) {
  saveModePreset(modeId, PRESET_NONE);
}

// ================== VARIABILI PER IMPOSTAZIONI METEO (modificabili da web) ==================
// Salvate su LittleFS nel file /apikeys.txt
// NOTA: Open-Meteo non richiede API key, serve solo il nome della città
String apiOpenWeatherCity = "";   // Nome città per meteo (es. "Roma", "Milano")

#define API_KEYS_FILE "/apikeys.txt"

// ================== FUNZIONI PER API KEYS ==================

// Salva le impostazioni meteo su LittleFS
void saveApiKeysToLittleFS() {
  Serial.println("[METEO] === SALVATAGGIO SU LittleFS ===");
  Serial.printf("[METEO] Città da salvare: '%s'\n", apiOpenWeatherCity.c_str());
  File file = LittleFS.open(API_KEYS_FILE, "w");
  if (file) {
    file.println(apiOpenWeatherCity);  // Solo città, Open-Meteo non richiede API key
    file.close();
    Serial.println("[METEO] Salvato su LittleFS con successo");
  } else {
    Serial.println("[METEO] ERRORE apertura file LittleFS per scrittura!");
  }
}

// Carica le impostazioni meteo da LittleFS
// Gestisce anche la migrazione dal vecchio formato (4 righe) al nuovo (1 riga)
void loadApiKeysFromLittleFS() {
  if (LittleFS.exists(API_KEYS_FILE)) {
    File file = LittleFS.open(API_KEYS_FILE, "r");
    if (file) {
      String line1 = "";
      String line2 = "";

      if (file.available()) {
        line1 = file.readStringUntil('\n');
        line1.trim();
      }
      if (file.available()) {
        line2 = file.readStringUntil('\n');
        line2.trim();
      }
      file.close();

      // Verifica se è il vecchio formato (line1 = API key, line2 = città)
      // Le API key OpenWeatherMap sono lunghe 32 caratteri esadecimali
      // I nomi città sono più corti e contengono lettere
      bool isOldFormat = (line1.length() >= 20 && line2.length() > 0 && line2.length() < 50);

      if (isOldFormat) {
        // Vecchio formato: line1=apiKey, line2=city
        apiOpenWeatherCity = line2;
        Serial.println("[METEO] Migrazione da vecchio formato file");
        Serial.printf("[METEO] Città recuperata: %s\n", apiOpenWeatherCity.c_str());
        // Salva nel nuovo formato
        saveApiKeysToLittleFS();
      } else if (line1.length() > 0 && line1.length() < 50) {
        // Nuovo formato: line1=city
        apiOpenWeatherCity = line1;
        Serial.println("[METEO] Caricato da LittleFS (nuovo formato)");
        Serial.printf("[METEO] Città meteo: %s\n", apiOpenWeatherCity.c_str());
      } else {
        Serial.println("[METEO] File corrotto o vuoto, configurare da /settings");
        apiOpenWeatherCity = "";
      }
    }
  } else {
    Serial.println("[METEO] File non trovato, configurare città da pagina /settings");
  }
}

// Handler GET /settings/getapikeys - Restituisce le impostazioni meteo
void handleGetApiKeys(AsyncWebServerRequest *request) {
  String json = "{\n";
  // NOTA: Open-Meteo non richiede API key, solo il nome della città
  json += "  \"openweatherCity\": \"" + apiOpenWeatherCity + "\"\n";
  json += "}";

  request->send(200, "application/json", json);
}

// Handler GET /settings/saveapikeys - Salva le impostazioni meteo
void handleSaveApiKeys(AsyncWebServerRequest *request) {
  Serial.println("[METEO] Salvataggio città...");
  bool changed = false;

  // Dichiarazione extern per funzione reset coordinate meteo (definita in 19_WEATHER.ino)
  extern void resetWeatherCoords();

  if (request->hasParam("openweatherCity")) {
    String val = request->getParam("openweatherCity")->value();
    val.trim();
    Serial.printf("[METEO] Città ricevuta: '%s', attuale: '%s'\n",
                  val.c_str(), apiOpenWeatherCity.c_str());
    if (val != apiOpenWeatherCity) {
      apiOpenWeatherCity = val;
      changed = true;
      Serial.printf("[METEO] Città meteo %s: %s\n", val.length() > 0 ? "CAMBIATA in" : "cancellata", val.c_str());
    }
  }

  if (changed) {
    saveApiKeysToLittleFS();
    Serial.println("[METEO] ✓ Salvato con successo");

    // Resetta coordinate e forza nuovo geocoding
    resetWeatherCoords();  // Forza nuovo geocoding con Open-Meteo
    extern uint32_t lastWeatherUpdate;
    lastWeatherUpdate = 0;
    Serial.println("[METEO] Forzato nuovo geocoding e aggiornamento meteo");
  }

  request->send(200, "text/plain", "OK");
}

// ================== FUNZIONI HELPER ==================

// Converte colore RGB in stringa hex
String colorToHex(uint8_t r, uint8_t g, uint8_t b) {
  char hex[8];
  sprintf(hex, "#%02x%02x%02x", r, g, b);
  return String(hex);
}

// Converte stringa hex in componenti RGB
void hexToColor(const String& hex, uint8_t& r, uint8_t& g, uint8_t& b) {
  String h = hex;
  if (h.startsWith("#")) h = h.substring(1);
  long color = strtol(h.c_str(), NULL, 16);
  r = (color >> 16) & 0xFF;
  g = (color >> 8) & 0xFF;
  b = color & 0xFF;
}

// Verifica se una modalità è abilitata
bool isModeEnabled(uint8_t mode) {
  // Mode 16 (GEMINI) e 18 (MJPEG) sono sempre disabilitati
  if (mode == 16 || mode == 18) return false;
  return (enabledModesMask & (1UL << mode)) != 0;
}

// Abilita/disabilita una modalità
void setModeEnabled(uint8_t mode, bool enabled) {
  if (enabled) {
    enabledModesMask |= (1UL << mode);
  } else {
    enabledModesMask &= ~(1UL << mode);
  }
}

// Carica rainbow mode da EEPROM
void loadRainbowMode() {
  uint8_t val = EEPROM.read(EEPROM_RAINBOW_MODE_ADDR);
  rainbowModeEnabled = (val == 1);
  Serial.printf("[SETTINGS] Rainbow mode: %s\n", rainbowModeEnabled ? "ON" : "OFF");
}

// Carica modalità abilitate da EEPROM
void loadEnabledModes() {
  uint32_t saved = 0;
  saved |= (uint32_t)EEPROM.read(EEPROM_ENABLED_MODES_ADDR) << 24;
  saved |= (uint32_t)EEPROM.read(EEPROM_ENABLED_MODES_ADDR + 1) << 16;
  saved |= (uint32_t)EEPROM.read(EEPROM_ENABLED_MODES_ADDR + 2) << 8;
  saved |= (uint32_t)EEPROM.read(EEPROM_ENABLED_MODES_ADDR + 3);

  // Se EEPROM non inizializzata (0xFFFFFFFF), mantieni default (tutte abilitate)
  if (saved != 0xFFFFFFFF) {
    enabledModesMask = saved;
    // Rispetta le impostazioni salvate dall'utente
  }
  Serial.printf("[SETTINGS] Modalità abilitate caricate: 0x%08X\n", enabledModesMask);
}

// Salva modalità abilitate in EEPROM
void saveEnabledModes() {
  EEPROM.write(EEPROM_ENABLED_MODES_ADDR, (enabledModesMask >> 24) & 0xFF);
  EEPROM.write(EEPROM_ENABLED_MODES_ADDR + 1, (enabledModesMask >> 16) & 0xFF);
  EEPROM.write(EEPROM_ENABLED_MODES_ADDR + 2, (enabledModesMask >> 8) & 0xFF);
  EEPROM.write(EEPROM_ENABLED_MODES_ADDR + 3, enabledModesMask & 0xFF);
  EEPROM.commit();
  Serial.printf("[SETTINGS] Modalità abilitate salvate: 0x%08X\n", enabledModesMask);
}

// Trova la prossima modalità abilitata (per cambio modo con touch)
DisplayMode getNextEnabledMode(DisplayMode current) {
  int start = (int)current;
  for (int i = 1; i <= NUM_MODES; i++) {
    int next = (start + i) % NUM_MODES;
    if (isModeEnabled(next)) {
      return (DisplayMode)next;
    }
  }
  return current;  // Se nessuna abilitata, resta sulla corrente
}

// Trova la modalità precedente abilitata
DisplayMode getPrevEnabledMode(DisplayMode current) {
  int start = (int)current;
  for (int i = 1; i <= NUM_MODES; i++) {
    int prev = (start - i + NUM_MODES) % NUM_MODES;
    if (isModeEnabled(prev)) {
      return (DisplayMode)prev;
    }
  }
  return current;
}

// ================== ENDPOINT WEB SERVER ==================

// GET /settings - Pagina HTML principale
void handleSettingsPage(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", SETTINGS_HTML);
}

// GET /settings/config - Restituisce configurazione completa in JSON
void handleSettingsConfig(AsyncWebServerRequest *request) {
  String json = "{\n";

  // Display e Luminosità
  json += "  \"brightnessDay\": " + String(brightnessDay) + ",\n";
  json += "  \"brightnessNight\": " + String(brightnessNight) + ",\n";
  json += "  \"autoNightMode\": " + String(setupOptions.autoNightModeEnabled ? "true" : "false") + ",\n";
  json += "  \"powerSave\": " + String(setupOptions.powerSaveEnabled ? "true" : "false") + ",\n";

  // Orari
  json += "  \"dayStartHour\": " + String(dayStartHour) + ",\n";
  json += "  \"dayStartMinute\": " + String(dayStartMinute) + ",\n";
  json += "  \"nightStartHour\": " + String(nightStartHour) + ",\n";
  json += "  \"nightStartMinute\": " + String(nightStartMinute) + ",\n";

  // Audio
  json += "  \"audioVolume\": " + String(audioVolume) + ",\n";
  json += "  \"hourlyAnnounce\": " + String(hourlyAnnounceEnabled ? "true" : "false") + ",\n";
  json += "  \"touchSounds\": " + String(setupOptions.touchSoundsEnabled ? "true" : "false") + ",\n";
  json += "  \"vuMeterShow\": " + String(setupOptions.vuMeterShowEnabled ? "true" : "false") + ",\n";
  // Audio disponibile: locale (AUDIO) o I2C (audioSlaveConnected)
  #ifdef AUDIO
  json += "  \"audioSlaveConnected\": true,\n";  // Audio locale sempre disponibile
  json += "  \"audioType\": \"local\",\n";
  #else
  json += "  \"audioSlaveConnected\": " + String(audioSlaveConnected ? "true" : "false") + ",\n";
  json += "  \"audioType\": \"i2c\",\n";
  #endif
  // Audio giorno/notte
  json += "  \"audioDayEnabled\": " + String(audioDayEnabled ? "true" : "false") + ",\n";
  json += "  \"audioNightEnabled\": " + String(audioNightEnabled ? "true" : "false") + ",\n";
  json += "  \"volumeDay\": " + String(volumeDay) + ",\n";
  json += "  \"volumeNight\": " + String(volumeNight) + ",\n";

  // Modalità ed Effetti
  json += "  \"displayMode\": " + String((int)currentMode) + ",\n";
  json += "  \"colorPreset\": " + String(currentPreset) + ",\n";
  // Preset salvato per la modalità corrente (255=nessuno, 100=rainbow, 0-14=preset)
  uint8_t modePreset = loadModePreset((uint8_t)currentMode);
  json += "  \"modePreset\": " + String(modePreset) + ",\n";
  // Colore personalizzato per la modalità corrente (salvato per ogni modalità)
  uint8_t modeR, modeG, modeB;
  loadModeColor((uint8_t)currentMode, modeR, modeG, modeB);
  char modeColorHex[8];
  sprintf(modeColorHex, "#%02X%02X%02X", modeR, modeG, modeB);
  json += "  \"customColor\": \"" + String(modeColorHex) + "\",\n";
  json += "  \"rainbowMode\": " + String(rainbowModeEnabled ? "true" : "false") + ",\n";
  json += "  \"letterE\": " + String(word_E_state) + ",\n";
  json += "  \"randomModeEnabled\": " + String(randomModeEnabled ? "true" : "false") + ",\n";
  json += "  \"randomModeInterval\": " + String(randomModeInterval) + ",\n";

  // Array delle modalità abilitate (solo quelle che esistono realmente)
  json += "  \"enabledModes\": [";
  bool first = true;
  // Lista delle modalità valide (esclude 16=GEMINI, 18=MJPEG e 24=MP3 che sono disabilitate)
  const int validModes[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 19, 20, 21, 22, 23};
  const int numValidModes = sizeof(validModes) / sizeof(validModes[0]);
  for (int i = 0; i < numValidModes; i++) {
    int modeId = validModes[i];
    if (isModeEnabled(modeId)) {
      if (!first) json += ",";
      json += String(modeId);
      first = false;
    }
  }
  json += "],\n";

  // Sensori - Radar (locale e remoto)
  bool usingRemote = useRemoteRadar();
  json += "  \"radarAvailable\": " + String((radarAvailable || radarServerConnected) ? "true" : "false") + ",\n";
  json += "  \"radarType\": \"" + String(usingRemote ? "esterno" : (radarAvailable ? "interno" : "N/D")) + "\",\n";
  json += "  \"radarServerEnabled\": " + String(radarServerEnabled ? "true" : "false") + ",\n";
  json += "  \"radarServerConnected\": " + String(radarServerConnected ? "true" : "false") + ",\n";
  json += "  \"radarServerIP\": \"" + radarServerIPString() + "\",\n";
  json += "  \"presenceDetected\": " + String((usingRemote ? radarRemotePresence : presenceDetected) ? "true" : "false") + ",\n";
  json += "  \"lightLevel\": " + String(usingRemote ? radarRemoteBrightness : lastRadarLightLevel) + ",\n";
  json += "  \"displayBrightness\": " + String(lastAppliedBrightness) + ",\n";
  json += "  \"radarBrightnessControl\": " + String(radarBrightnessControl ? "true" : "false") + ",\n";
  json += "  \"radarBrightnessMin\": " + String(radarBrightnessMin) + ",\n";
  json += "  \"radarBrightnessMax\": " + String(radarBrightnessMax) + ",\n";
  // Dati aggiuntivi dal radar server remoto (temperatura/umidità)
  json += "  \"radarRemoteTemp\": " + String(radarRemoteTemperature, 1) + ",\n";
  json += "  \"radarRemoteHum\": " + String(radarRemoteHumidity, 1) + ",\n";

  // Sensori - BME280
  json += "  \"bme280Available\": " + String(bme280Available ? "true" : "false") + ",\n";
  json += "  \"tempIndoor\": " + String(temperatureIndoor, 1) + ",\n";
  json += "  \"humIndoor\": " + String(humidityIndoor, 0) + ",\n";
  json += "  \"pressIndoor\": " + String(pressureIndoor, 0) + ",\n";
  // Offset calibrazione BME280
  json += "  \"bme280TempOffset\": " + String(bme280TempOffset, 1) + ",\n";
  json += "  \"bme280HumOffset\": " + String(bme280HumOffset, 1) + ",\n";
  // Fonte dati sensore: 0=nessuno, 1=BME280 interno, 2=radar remoto
  json += "  \"indoorSensorSource\": " + String(indoorSensorSource) + ",\n";

  // Sensori - Meteo esterno (da 19_WEATHER.ino - OpenWeatherMap)
  json += "  \"weatherAvailable\": " + String(weatherDataValid ? "true" : "false") + ",\n";
  json += "  \"tempOutdoor\": " + String(weatherTempOutdoor, 1) + ",\n";
  json += "  \"humOutdoor\": " + String(weatherHumOutdoor, 0) + ",\n";

  // Sensori - Magnetometro QMC5883P (da 25_MAGNETOMETER.ino)
  json += "  \"magnetometerConnected\": " + String(magnetometerConnected ? "true" : "false") + ",\n";
  json += "  \"magnetometerHeading\": " + String(magnetometerHeading, 1) + ",\n";
  json += "  \"magnetometerDirection\": \"" + String(getCardinalDirection(magnetometerHeading)) + "\",\n";
  json += "  \"magnetometerCalibrated\": " + String(magCalibrated ? "true" : "false") + ",\n";

  // Rete
  json += "  \"wifiSSID\": \"" + String(WiFi.SSID()) + "\",\n";
  json += "  \"ipAddress\": \"" + WiFi.localIP().toString() + "\",\n";
  json += "  \"wifiRSSI\": " + String(WiFi.RSSI()) + ",\n";

  // Sistema
  json += "  \"uptime\": " + String(millis() / 1000) + ",\n";
  json += "  \"freeHeap\": " + String(ESP.getFreeHeap()) + ",\n";
  json += "  \"sdAvailable\": " + String(SD.cardType() != CARD_NONE ? "true" : "false") + ",\n";

  // ESP32-CAM
  #ifdef EFFECT_ESP32CAM
  json += "  \"esp32camEnabled\": true,\n";
  json += "  \"esp32camStreamUrl\": \"" + esp32camStreamUrl + "\",\n";
  // Estrai URL base della cam (rimuovi /stream per ottenere pagina config)
  {
    String camWebUrl = esp32camStreamUrl;
    int streamIdx = camWebUrl.indexOf("/stream");
    if (streamIdx > 0) {
      camWebUrl = camWebUrl.substring(0, streamIdx);
    }
    // Rimuovi anche :81 se presente per usare porta 80 (pagina web principale)
    camWebUrl.replace(":81", "");
    json += "  \"esp32camWebUrl\": \"" + camWebUrl + "\",\n";
  }
  #else
  json += "  \"esp32camEnabled\": false,\n";
  json += "  \"esp32camStreamUrl\": \"\",\n";
  json += "  \"esp32camWebUrl\": \"\",\n";
  #endif

  // Ora attuale
  json += "  \"currentHour\": " + String(myTZ.hour()) + ",\n";
  json += "  \"currentMinute\": " + String(myTZ.minute()) + ",\n";
  json += "  \"currentSecond\": " + String(myTZ.second()) + ",\n";

  // Flag meteo configurato (Open-Meteo non richiede API key, serve solo la città)
  json += "  \"hasWeatherApiKey\": " + String(apiOpenWeatherCity.length() >= 2 ? "true" : "false") + ",\n";

  // Lingua
  #ifdef ENABLE_MULTILANGUAGE
  json += "  \"language\": " + String((int)currentLanguage) + "\n";
  #else
  json += "  \"language\": 0\n";
  #endif

  json += "}";

  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "0");
  request->send(response);
}

// GET /settings/status - Restituisce info dinamiche (per aggiornamento periodico in tempo reale)
void handleSettingsStatus(AsyncWebServerRequest *request) {
  String json = "{\n";

  // Info che cambiano frequentemente
  json += "  \"displayMode\": " + String((int)currentMode) + ",\n";
  json += "  \"colorPreset\": " + String(currentPreset) + ",\n";
  // Preset salvato per la modalità corrente
  uint8_t modePresetStatus = loadModePreset((uint8_t)currentMode);
  json += "  \"modePreset\": " + String(modePresetStatus) + ",\n";
  json += "  \"presenceDetected\": " + String(presenceDetected ? "true" : "false") + ",\n";
  json += "  \"lightLevel\": " + String(lastRadarLightLevel) + ",\n";
  json += "  \"uptime\": " + String(millis() / 1000) + ",\n";
  json += "  \"currentHour\": " + String(myTZ.hour()) + ",\n";
  json += "  \"currentMinute\": " + String(myTZ.minute()) + ",\n";
  json += "  \"currentSecond\": " + String(myTZ.second()) + ",\n";

  // Parametri sincronizzabili dal device
  json += "  \"brightnessDay\": " + String(brightnessDay) + ",\n";
  json += "  \"brightnessNight\": " + String(brightnessNight) + ",\n";
  json += "  \"letterE\": " + String(word_E_state) + ",\n";

  // Sensori - Radar (locale e remoto)
  bool usingRemoteStatus = useRemoteRadar();
  json += "  \"radarAvailable\": " + String((radarAvailable || radarServerConnected) ? "true" : "false") + ",\n";
  json += "  \"radarType\": \"" + String(usingRemoteStatus ? "esterno" : (radarAvailable ? "interno" : "N/D")) + "\",\n";
  json += "  \"radarServerConnected\": " + String(radarServerConnected ? "true" : "false") + ",\n";
  json += "  \"presenceDetected\": " + String((usingRemoteStatus ? radarRemotePresence : presenceDetected) ? "true" : "false") + ",\n";
  json += "  \"lightLevel\": " + String(usingRemoteStatus ? radarRemoteBrightness : lastRadarLightLevel) + ",\n";
  json += "  \"displayBrightness\": " + String(lastAppliedBrightness) + ",\n";
  json += "  \"radarBrightnessControl\": " + String(radarBrightnessControl ? "true" : "false") + ",\n";
  json += "  \"radarBrightnessMin\": " + String(radarBrightnessMin) + ",\n";
  json += "  \"radarBrightnessMax\": " + String(radarBrightnessMax) + ",\n";
  // Dati aggiuntivi dal radar server remoto
  json += "  \"radarRemoteTemp\": " + String(radarRemoteTemperature, 1) + ",\n";
  json += "  \"radarRemoteHum\": " + String(radarRemoteHumidity, 1) + ",\n";

  // Sensori - BME280 (temperatura, umidità, pressione interna)
  json += "  \"bme280Available\": " + String(bme280Available ? "true" : "false") + ",\n";
  json += "  \"tempIndoor\": " + String(temperatureIndoor, 1) + ",\n";
  json += "  \"humIndoor\": " + String(humidityIndoor, 0) + ",\n";
  json += "  \"pressIndoor\": " + String(pressureIndoor, 0) + ",\n";
  // Offset calibrazione BME280
  json += "  \"bme280TempOffset\": " + String(bme280TempOffset, 1) + ",\n";
  json += "  \"bme280HumOffset\": " + String(bme280HumOffset, 1) + ",\n";
  // Fonte dati sensore: 0=nessuno, 1=BME280 interno, 2=radar remoto
  json += "  \"indoorSensorSource\": " + String(indoorSensorSource) + ",\n";

  // Sensori - Meteo esterno (OpenWeatherMap)
  json += "  \"weatherAvailable\": " + String(weatherDataValid ? "true" : "false") + ",\n";
  json += "  \"tempOutdoor\": " + String(weatherTempOutdoor, 1) + ",\n";
  json += "  \"humOutdoor\": " + String(weatherHumOutdoor, 0) + ",\n";

  // Sensori - Magnetometro QMC5883P (da 25_MAGNETOMETER.ino)
  json += "  \"magnetometerConnected\": " + String(magnetometerConnected ? "true" : "false") + ",\n";
  json += "  \"magnetometerHeading\": " + String(magnetometerHeading, 1) + ",\n";
  json += "  \"magnetometerDirection\": \"" + String(getCardinalDirection(magnetometerHeading)) + "\",\n";
  json += "  \"magnetometerCalibrated\": " + String(magCalibrated ? "true" : "false") + ",\n";

  // Audio disponibile: locale (AUDIO) o I2C (audioSlaveConnected)
  #ifdef AUDIO
  json += "  \"audioSlaveConnected\": true,\n";  // Audio locale sempre disponibile
  #else
  json += "  \"audioSlaveConnected\": " + String(audioSlaveConnected ? "true" : "false") + ",\n";
  #endif

  // Colore personalizzato per la modalità corrente
  uint8_t custR, custG, custB;
  loadModeColor((uint8_t)currentMode, custR, custG, custB);
  char hexColor[8];
  sprintf(hexColor, "#%02X%02X%02X", custR, custG, custB);
  json += "  \"customColor\": \"" + String(hexColor) + "\",\n";

  // Random Mode (per status summary)
  json += "  \"randomModeEnabled\": " + String(randomModeEnabled ? "true" : "false") + ",\n";
  json += "  \"randomModeInterval\": " + String(randomModeInterval) + "\n";

  json += "}";

  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "0");
  request->send(response);
}

// GET /settings/save - Salva tutte le impostazioni
void handleSettingsSave(AsyncWebServerRequest *request) {
  Serial.println("=====================================");
  Serial.println("[SETTINGS WEB] *** SALVATAGGIO IMPOSTAZIONI ***");
  Serial.printf("[SETTINGS WEB] Parametri ricevuti: %d\n", request->params());

  bool changed = false;

  // ===== DISPLAY =====
  if (request->hasParam("brightnessDay")) {
    String valStr = request->getParam("brightnessDay")->value();
    uint8_t val = valStr.toInt();
    Serial.printf("[SETTINGS] brightnessDay ricevuto: '%s' -> %d (attuale: %d)\n", valStr.c_str(), val, brightnessDay);
    if (val >= 10 && val <= 255 && val != brightnessDay) {
      brightnessDay = val;
      EEPROM.write(EEPROM_BRIGHTNESS_DAY_ADDR, brightnessDay);
      changed = true;
      Serial.printf("[SETTINGS] brightnessDay = %d\n", brightnessDay);
      // APPLICA IMMEDIATAMENTE se è giorno
      if (!checkIsNightTime(currentHour, currentMinute)) {
        ledcWrite(PWM_CHANNEL, brightnessDay);
        lastAppliedBrightness = brightnessDay;
        Serial.printf("[SETTINGS] Luminosità GIORNO applicata subito: %d\n", brightnessDay);
      }
    }
  }

  if (request->hasParam("brightnessNight")) {
    String valStr = request->getParam("brightnessNight")->value();
    uint8_t val = valStr.toInt();
    Serial.printf("[SETTINGS] brightnessNight ricevuto: '%s' -> %d (attuale: %d)\n", valStr.c_str(), val, brightnessNight);
    if (val >= 10 && val <= 255 && val != brightnessNight) {
      brightnessNight = val;
      EEPROM.write(EEPROM_BRIGHTNESS_NIGHT_ADDR, brightnessNight);
      changed = true;
      Serial.printf("[SETTINGS] brightnessNight = %d\n", brightnessNight);
      // APPLICA IMMEDIATAMENTE se è notte
      if (checkIsNightTime(currentHour, currentMinute)) {
        ledcWrite(PWM_CHANNEL, brightnessNight);
        lastAppliedBrightness = brightnessNight;
        Serial.printf("[SETTINGS] Luminosità NOTTE applicata subito: %d\n", brightnessNight);
      }
    }
  }

  if (request->hasParam("autoNightMode")) {
    bool val = (request->getParam("autoNightMode")->value().toInt() == 1);
    if (val != setupOptions.autoNightModeEnabled) {
      setupOptions.autoNightModeEnabled = val;
      changed = true;
      Serial.printf("[SETTINGS] autoNightMode = %s\n", val ? "ON" : "OFF");
    }
  }

  if (request->hasParam("powerSave")) {
    bool val = (request->getParam("powerSave")->value().toInt() == 1);
    if (val != setupOptions.powerSaveEnabled) {
      setupOptions.powerSaveEnabled = val;
      changed = true;
      Serial.printf("[SETTINGS] powerSave = %s\n", val ? "ON" : "OFF");
    }
  }

  if (request->hasParam("radarBrightnessControl")) {
    bool val = (request->getParam("radarBrightnessControl")->value().toInt() == 1);
    if (val != setupOptions.radarBrightnessEnabled) {
      setupOptions.radarBrightnessEnabled = val;
      radarBrightnessControl = val;  // Aggiorna anche la variabile globale
      EEPROM.write(EEPROM_RADAR_BRIGHTNESS_ADDR, val ? 1 : 0);
      changed = true;
      Serial.printf("[SETTINGS] radarBrightnessControl = %s\n", val ? "ON" : "OFF");
    }
  }

  if (request->hasParam("radarBrightnessMin")) {
    uint8_t val = request->getParam("radarBrightnessMin")->value().toInt();
    if (val >= 10 && val <= 255 && val != radarBrightnessMin) {
      radarBrightnessMin = val;
      EEPROM.write(EEPROM_RADAR_BRIGHT_MIN_ADDR, radarBrightnessMin);
      changed = true;
      Serial.printf("[SETTINGS] radarBrightnessMin = %d\n", radarBrightnessMin);
    }
  }

  if (request->hasParam("radarBrightnessMax")) {
    uint8_t val = request->getParam("radarBrightnessMax")->value().toInt();
    if (val >= 10 && val <= 255 && val != radarBrightnessMax) {
      radarBrightnessMax = val;
      EEPROM.write(EEPROM_RADAR_BRIGHT_MAX_ADDR, radarBrightnessMax);
      changed = true;
      Serial.printf("[SETTINGS] radarBrightnessMax = %d\n", radarBrightnessMax);
    }
  }

  // ===== ORARI =====
  if (request->hasParam("dayStartHour")) {
    uint8_t val = request->getParam("dayStartHour")->value().toInt();
    if (val < 24 && val != dayStartHour) {
      dayStartHour = val;
      EEPROM.write(EEPROM_DAY_START_HOUR_ADDR, dayStartHour);
      changed = true;
      Serial.printf("[SETTINGS] dayStartHour = %d\n", dayStartHour);
    }
  }

  if (request->hasParam("dayStartMinute")) {
    uint8_t val = request->getParam("dayStartMinute")->value().toInt();
    if (val < 60 && val != dayStartMinute) {
      dayStartMinute = val;
      EEPROM.write(EEPROM_DAY_START_MIN_ADDR, dayStartMinute);
      changed = true;
      Serial.printf("[SETTINGS] dayStartMinute = %d\n", dayStartMinute);
    }
  }

  if (request->hasParam("nightStartHour")) {
    uint8_t val = request->getParam("nightStartHour")->value().toInt();
    if (val < 24 && val != nightStartHour) {
      nightStartHour = val;
      EEPROM.write(EEPROM_NIGHT_START_HOUR_ADDR, nightStartHour);
      changed = true;
      Serial.printf("[SETTINGS] nightStartHour = %d\n", nightStartHour);
    }
  }

  if (request->hasParam("nightStartMinute")) {
    uint8_t val = request->getParam("nightStartMinute")->value().toInt();
    if (val < 60 && val != nightStartMinute) {
      nightStartMinute = val;
      EEPROM.write(EEPROM_NIGHT_START_MIN_ADDR, nightStartMinute);
      changed = true;
      Serial.printf("[SETTINGS] nightStartMinute = %d\n", nightStartMinute);
    }
  }

  // ===== AUDIO =====
  if (request->hasParam("audioVolume")) {
    uint8_t val = request->getParam("audioVolume")->value().toInt();
    if (val <= 100 && val != audioVolume) {
      audioVolume = val;
      EEPROM.write(EEPROM_AUDIO_VOLUME_ADDR, audioVolume);
      changed = true;
      Serial.printf("[SETTINGS] audioVolume = %d%%\n", audioVolume);

      // Invia nuovo volume all'ESP32C3
      setVolumeViaI2C(audioVolume);
    }
  }

  if (request->hasParam("hourlyAnnounce")) {
    bool val = (request->getParam("hourlyAnnounce")->value().toInt() == 1);
    if (val != hourlyAnnounceEnabled) {
      hourlyAnnounceEnabled = val;
      EEPROM.write(EEPROM_HOURLY_ANNOUNCE_ADDR, val ? 1 : 0);
      changed = true;
      Serial.printf("[SETTINGS] hourlyAnnounce = %s\n", val ? "ON" : "OFF");
    }
  }

  if (request->hasParam("touchSounds")) {
    bool val = (request->getParam("touchSounds")->value().toInt() == 1);
    if (val != setupOptions.touchSoundsEnabled) {
      setupOptions.touchSoundsEnabled = val;
      changed = true;
      Serial.printf("[SETTINGS] touchSounds = %s\n", val ? "ON" : "OFF");
    }
  }

  if (request->hasParam("vuMeterShow")) {
    bool val = (request->getParam("vuMeterShow")->value().toInt() == 1);
    if (val != setupOptions.vuMeterShowEnabled) {
      setupOptions.vuMeterShowEnabled = val;
      EEPROM.write(EEPROM_VUMETER_ENABLED_ADDR, val ? 1 : 0);
      EEPROM.commit();
      changed = true;
      Serial.printf("[SETTINGS] vuMeterShow = %s\n", val ? "ON" : "OFF");
    }
  }

  // ===== AUDIO GIORNO/NOTTE =====
  if (request->hasParam("audioDayEnabled")) {
    bool val = (request->getParam("audioDayEnabled")->value().toInt() == 1);
    if (val != audioDayEnabled) {
      audioDayEnabled = val;
      EEPROM.write(EEPROM_AUDIO_DAY_ENABLED_ADDR, val ? 1 : 0);
      changed = true;
      Serial.printf("[SETTINGS] audioDayEnabled = %s\n", val ? "ON" : "OFF");
    }
  }

  if (request->hasParam("audioNightEnabled")) {
    bool val = (request->getParam("audioNightEnabled")->value().toInt() == 1);
    if (val != audioNightEnabled) {
      audioNightEnabled = val;
      EEPROM.write(EEPROM_AUDIO_NIGHT_ENABLED_ADDR, val ? 1 : 0);
      changed = true;
      Serial.printf("[SETTINGS] audioNightEnabled = %s\n", val ? "ON" : "OFF");
    }
  }

  if (request->hasParam("volumeDay")) {
    uint8_t val = request->getParam("volumeDay")->value().toInt();
    if (val <= 100 && val != volumeDay) {
      volumeDay = val;
      EEPROM.write(EEPROM_VOLUME_DAY_ADDR, volumeDay);
      changed = true;
      Serial.printf("[SETTINGS] volumeDay = %d%%\n", volumeDay);
      // Applica immediatamente se siamo di giorno
      extern bool lastWasNightTime;
      extern uint8_t lastAppliedVolume;
      if (!lastWasNightTime && audioSlaveConnected) {
        setVolumeViaI2C(volumeDay);
        lastAppliedVolume = volumeDay;
        Serial.printf("[SETTINGS] Volume applicato (giorno): %d%%\n", volumeDay);
      }
    }
  }

  if (request->hasParam("volumeNight")) {
    uint8_t val = request->getParam("volumeNight")->value().toInt();
    if (val <= 100 && val != volumeNight) {
      volumeNight = val;
      EEPROM.write(EEPROM_VOLUME_NIGHT_ADDR, volumeNight);
      changed = true;
      Serial.printf("[SETTINGS] volumeNight = %d%%\n", volumeNight);
      // Applica immediatamente se siamo di notte
      extern bool lastWasNightTime;
      extern uint8_t lastAppliedVolume;
      if (lastWasNightTime && audioSlaveConnected) {
        setVolumeViaI2C(volumeNight);
        lastAppliedVolume = volumeNight;
        Serial.printf("[SETTINGS] Volume applicato (notte): %d%%\n", volumeNight);
      }
    }
  }

  // ===== EFFETTI =====
  if (request->hasParam("displayMode")) {
    uint8_t val = request->getParam("displayMode")->value().toInt();
    if (val < NUM_MODES && val != (int)currentMode) {
      currentMode = (DisplayMode)val;
      userMode = currentMode;
      EEPROM.write(EEPROM_MODE_ADDR, val);

      // Carica il preset e colore salvati per la nuova modalità (sincronizzato con touch)
      uint8_t modePreset = loadModePreset(val);
      uint8_t modeR, modeG, modeB;
      loadModeColor(val, modeR, modeG, modeB);

      // Se c'è un preset salvato per questa modalità, usa quello
      if (modePreset != 255) {  // 255 = PRESET_NONE
        currentPreset = modePreset;
        // Per preset 13 (personalizzato) usa il colore salvato
        if (modePreset == 13) {
          currentColor = Color(modeR, modeG, modeB);
          userColor = currentColor;
        }
      } else {
        // Nessun preset salvato: usa il colore salvato per questa modalità
        currentPreset = 13;  // Personalizzato
        currentColor = Color(modeR, modeG, modeB);
        userColor = currentColor;
      }

      // Aggiorna EEPROM con preset e colore caricati
      EEPROM.write(EEPROM_PRESET_ADDR, currentPreset);
      EEPROM.write(EEPROM_COLOR_R_ADDR, currentColor.r);
      EEPROM.write(EEPROM_COLOR_G_ADDR, currentColor.g);
      EEPROM.write(EEPROM_COLOR_B_ADDR, currentColor.b);

      changed = true;
      Serial.printf("[SETTINGS] displayMode = %d, preset = %d, color = R%d G%d B%d\n",
                    val, currentPreset, currentColor.r, currentColor.g, currentColor.b);

      // Forza ridisegno
      gfx->fillScreen(BLACK);
      lastHour = 255;
      lastMinute = 255;
    }
  }

  if (request->hasParam("colorPreset")) {
    uint8_t val = request->getParam("colorPreset")->value().toInt();
    // Salva preset per la modalità corrente (non globale!)
    currentPreset = val;
    EEPROM.write(EEPROM_PRESET_ADDR, currentPreset);

    // Salva il preset per questa specifica modalità
    saveModePreset((uint8_t)currentMode, val);

    // Applica il colore del preset alla modalità corrente (NON cambia mode!)
    switch (val) {
      case 0:  // Random
        currentColor = Color(random(256), random(256), random(256));
        break;
      case 1:  // Acqua
        currentColor = Color(0, 255, 255);
        break;
      case 2:  // Viola
        currentColor = Color(255, 0, 255);
        break;
      case 3:  // Arancione
        currentColor = Color(255, 165, 0);
        break;
      case 4:  // Rosso
        currentColor = Color(255, 0, 0);
        break;
      case 5:  // Verde
        currentColor = Color(0, 255, 0);
        break;
      case 6:  // Blu
        currentColor = Color(0, 0, 255);
        break;
      case 7:  // Giallo
        currentColor = Color(255, 255, 0);
        break;
      case 8:  // Ciano
        currentColor = Color(0, 255, 255);
        break;
      case 9:  // Verde Classico
        currentColor = Color(0, 255, 0);
        break;
      case 10: // Bianco
        currentColor = Color(255, 255, 255);
        break;
      case 11: // Giallo Snake
        currentColor = Color(255, 255, 0);
        break;
      case 12: // Azzurro Acqua
        currentColor = Color(0, 150, 255);
        break;
      case 13: // Personalizzato - usa colore salvato per questa modalità
        {
          uint8_t r, g, b;
          loadModeColor((uint8_t)currentMode, r, g, b);
          currentColor = Color(r, g, b);
        }
        break;
      default:
        currentColor = Color(255, 255, 255);
        break;
    }

    // Aggiorna anche userColor per coerenza
    userColor = currentColor;

    // NON salvare il colore personalizzato quando si seleziona un preset predefinito (0-12)
    // Il colore personalizzato deve cambiare SOLO da color picker o color cycle display
    // Salviamo solo il preset selezionato, non il colore

    // Richiedi ridisegno display dal loop principale
    webForceDisplayUpdate = true;
    changed = true;
    Serial.printf("[SETTINGS] Preset %d applicato per mode %d, Color = R%d G%d B%d\n",
                  val, (int)currentMode, currentColor.r, currentColor.g, currentColor.b);
  }

  if (request->hasParam("customColor")) {
    String hexColor = request->getParam("customColor")->value();
    uint8_t r, g, b;
    hexToColor(hexColor, r, g, b);

    // Salva colore per la modalità corrente
    saveModeColor((uint8_t)currentMode, r, g, b);

    // Se non è già stato impostato un preset in questa richiesta,
    // e il colore viene cambiato direttamente dal color picker,
    // imposta preset a 13 (Personalizzato) solo se non c'è colorPreset nella richiesta
    if (!request->hasParam("colorPreset")) {
      saveModePreset((uint8_t)currentMode, 13);  // 13 = Personalizzato
      Serial.printf("[SETTINGS] Preset 13 (Personalizzato) salvato per mode %d\n", (int)currentMode);
    }

    // Aggiorna anche userColor e currentColor per applicazione immediata
    userColor.r = r;
    userColor.g = g;
    userColor.b = b;
    currentColor = userColor;

    // Salva anche nel vecchio indirizzo per compatibilità
    EEPROM.write(EEPROM_COLOR_R_ADDR, r);
    EEPROM.write(EEPROM_COLOR_G_ADDR, g);
    EEPROM.write(EEPROM_COLOR_B_ADDR, b);

    // Richiedi ridisegno display dal loop principale
    webForceDisplayUpdate = true;
    changed = true;
    Serial.printf("[SETTINGS] customColor mode %d = %s (R=%d G=%d B=%d)\n", (int)currentMode, hexColor.c_str(), r, g, b);
  }

  if (request->hasParam("letterE")) {
    uint8_t val = request->getParam("letterE")->value().toInt();
    if (val <= 2 && val != word_E_state) {
      word_E_state = val;
      EEPROM.write(EEPROM_WORD_E_STATE_ADDR, word_E_state);
      changed = true;
      Serial.printf("[SETTINGS] letterE = %d\n", val);
    }
  }

  // ===== RAINBOW MODE =====
  if (request->hasParam("rainbowMode")) {
    bool val = (request->getParam("rainbowMode")->value().toInt() == 1);
    rainbowModeEnabled = val;
    EEPROM.write(EEPROM_RAINBOW_MODE_ADDR, val ? 1 : 0);

    // Salva rainbow come preset per questa modalità SOLO se attivato
    // NON resettare se è stato già impostato un colorPreset nella stessa richiesta
    if (val) {
      saveModePreset((uint8_t)currentMode, PRESET_RAINBOW);
      Serial.printf("[SETTINGS] Rainbow salvato per mode %d\n", (int)currentMode);
    }
    // Se rainbow viene disattivato E non c'è un colorPreset nella richiesta,
    // significa che l'utente sta solo disattivando rainbow senza scegliere un preset
    // In questo caso NON toccare il preset salvato

    webForceDisplayUpdate = true;
    changed = true;
    Serial.printf("[SETTINGS] rainbowMode = %s\n", val ? "ON" : "OFF");
  }

  // ===== RANDOM MODE =====
  if (request->hasParam("randomModeEnabled")) {
    bool val = (request->getParam("randomModeEnabled")->value().toInt() == 1);
    if (val != randomModeEnabled) {
      randomModeEnabled = val;
      EEPROM.write(EEPROM_RANDOM_MODE_ADDR, val ? 1 : 0);
      // Reset timer quando abilitato per iniziare il conteggio da ora
      if (val) {
        lastRandomModeChange = millis();
      }
      changed = true;
      Serial.printf("[SETTINGS] randomModeEnabled = %s\n", val ? "ON" : "OFF");
    }
  }

  if (request->hasParam("randomModeInterval")) {
    uint8_t val = request->getParam("randomModeInterval")->value().toInt();
    if (val >= 1 && val <= 60 && val != randomModeInterval) {
      randomModeInterval = val;
      EEPROM.write(EEPROM_RANDOM_INTERVAL_ADDR, randomModeInterval);
      changed = true;
      Serial.printf("[SETTINGS] randomModeInterval = %d minuti\n", randomModeInterval);
    }
  }

  // Salva tutte le modifiche
  if (changed) {
    EEPROM.commit();
    saveSetupOptions();
    Serial.println("[SETTINGS] ✓ Impostazioni salvate con successo");
  } else {
    Serial.println("[SETTINGS] Nessuna modifica rilevata");
  }

  Serial.println("=====================================");
  request->send(200, "text/plain", "OK");
}

// GET /settings/reset - Ripristina impostazioni di fabbrica
void handleSettingsReset(AsyncWebServerRequest *request) {
  Serial.println("[SETTINGS] *** RESET IMPOSTAZIONI ***");

  // Cancella marker EEPROM per forzare reinizializzazione
  EEPROM.write(0, 0xFF);
  EEPROM.commit();

  request->send(200, "text/plain", "OK");

  // Riavvia dopo un breve delay
  delay(1000);
  ESP.restart();
}

// GET /settings/reboot - Riavvia il dispositivo
void handleSettingsReboot(AsyncWebServerRequest *request) {
  Serial.println("[SETTINGS] *** RIAVVIO DISPOSITIVO ***");

  request->send(200, "text/plain", "OK");

  // Riavvia dopo un breve delay
  delay(1000);
  ESP.restart();
}

// GET /settings/setmode - Imposta la modalità corrente
void handleSettingsSetMode(AsyncWebServerRequest *request) {
  if (request->hasParam("mode")) {
    uint8_t mode = request->getParam("mode")->value().toInt();
    if (mode < NUM_MODES && isModeEnabled(mode)) {
      currentMode = (DisplayMode)mode;
      userMode = currentMode;
      EEPROM.write(EEPROM_MODE_ADDR, mode);
      EEPROM.commit();

      // Carica il preset salvato per questa modalità
      uint8_t savedPreset = loadModePreset(mode);
      Serial.printf("[SETTINGS] Preset salvato per mode %d: %d\n", mode, savedPreset);

      // Gestisci il preset salvato
      if (savedPreset == PRESET_RAINBOW) {
        // Attiva rainbow per questa modalità
        rainbowModeEnabled = true;
        EEPROM.write(EEPROM_RAINBOW_MODE_ADDR, 1);
        Serial.printf("[SETTINGS] Rainbow attivato per mode %d\n", mode);
      } else {
        // Disattiva rainbow
        rainbowModeEnabled = false;
        EEPROM.write(EEPROM_RAINBOW_MODE_ADDR, 0);

        if (savedPreset != PRESET_NONE && savedPreset < 14) {
          // Applica il colore del preset salvato
          currentPreset = savedPreset;
          switch (savedPreset) {
            case 0: currentColor = Color(random(256), random(256), random(256)); break;
            case 1: currentColor = Color(0, 255, 255); break;    // Acqua
            case 2: currentColor = Color(255, 0, 255); break;    // Viola
            case 3: currentColor = Color(255, 165, 0); break;    // Arancione
            case 4: currentColor = Color(255, 0, 0); break;      // Rosso
            case 5: currentColor = Color(0, 255, 0); break;      // Verde
            case 6: currentColor = Color(0, 0, 255); break;      // Blu
            case 7: currentColor = Color(255, 255, 0); break;    // Giallo
            case 8: currentColor = Color(0, 255, 255); break;    // Ciano
            case 9: currentColor = Color(0, 255, 0); break;      // Verde Classico
            case 10: currentColor = Color(255, 255, 255); break; // Bianco
            case 11: currentColor = Color(255, 255, 0); break;   // Giallo Snake
            case 12: currentColor = Color(0, 150, 255); break;   // Azzurro Acqua
            case 13: // Personalizzato
            default:
              {
                uint8_t r, g, b;
                loadModeColor(mode, r, g, b);
                currentColor = Color(r, g, b);
              }
              break;
          }
          userColor = currentColor;
          Serial.printf("[SETTINGS] Colore preset %d applicato: R%d G%d B%d\n",
                        savedPreset, currentColor.r, currentColor.g, currentColor.b);
        } else {
          // Nessun preset, carica colore personalizzato
          uint8_t r, g, b;
          loadModeColor(mode, r, g, b);
          userColor.r = r;
          userColor.g = g;
          userColor.b = b;
          currentColor = userColor;
          Serial.printf("[SETTINGS] Colore custom mode %d caricato: R%d G%d B%d\n", mode, r, g, b);
        }
      }
      EEPROM.commit();

      // Forza ridisegno
      gfx->fillScreen(BLACK);
      lastHour = 255;
      lastMinute = 255;
      webForceDisplayUpdate = true;

      // Reset flag inizializzazione per modalità speciali
      #ifdef EFFECT_CHRISTMAS
      christmasInitialized = false;
      xmasSceneDrawn = false;  // Reset anche la scena statica
      #endif
      #ifdef EFFECT_FIRE
      fireInitialized = false;
      #endif
      #ifdef EFFECT_FIRE_TEXT
      fireTextInitialized = false;
      #endif
      fluxCapacitorInitialized = false;
      matrixInitialized = false;
      tronInitialized = false;
      flipClockInitialized = false;
      bttfInitialized = false;
      ledRingInitialized = false;
      weatherStationInitialized = false;

      Serial.printf("[SETTINGS] Modalità impostata: %d (flags reset)\n", mode);
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Modalità non valida o disabilitata");
    }
  } else {
    request->send(400, "text/plain", "Parametro mode mancante");
  }
}

// GET /settings/savemodes - Salva quali modalità sono abilitate
void handleSettingsSaveModes(AsyncWebServerRequest *request) {
  if (request->hasParam("enabled")) {
    String enabledList = request->getParam("enabled")->value();

    // Reset mask
    enabledModesMask = 0;

    // Parse lista separata da virgole
    int start = 0;
    int end;
    while ((end = enabledList.indexOf(',', start)) != -1) {
      int modeId = enabledList.substring(start, end).toInt();
      if (modeId >= 0 && modeId < 32) {
        setModeEnabled(modeId, true);
      }
      start = end + 1;
    }
    // Ultimo elemento (o unico se non ci sono virgole)
    if (start < enabledList.length()) {
      int modeId = enabledList.substring(start).toInt();
      if (modeId >= 0 && modeId < 32) {
        setModeEnabled(modeId, true);
      }
    }

    // Salva in EEPROM
    saveEnabledModes();

    Serial.printf("[SETTINGS] Modalità abilitate aggiornate: %s\n", enabledList.c_str());
    request->send(200, "text/plain", "OK");
  } else {
    request->send(400, "text/plain", "Parametro enabled mancante");
  }
}

// Handler per cambiare la lingua
void handleSetLanguage(AsyncWebServerRequest *request) {
  #ifdef ENABLE_MULTILANGUAGE
  if (request->hasParam("lang")) {
    uint8_t lang = request->getParam("lang")->value().toInt();
    if (lang < LANG_COUNT) {
      setLanguage((Language)lang);

      // Forza ridisegno display per applicare la nuova lingua
      gfx->fillScreen(BLACK);
      lastHour = 255;
      lastMinute = 255;
      // Forza reinizializzazione dei mode Word Clock per cambio lingua
      fadeInitialized = false;
      slowInitialized = false;
      matrixInitialized = false;
      webForceDisplayUpdate = true;

      Serial.printf("[SETTINGS] Lingua cambiata: %s\n", lang == LANG_EN ? "English" : "Italiano");
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Lingua non valida");
    }
  } else {
    request->send(400, "text/plain", "Parametro lang mancante");
  }
  #else
  request->send(400, "text/plain", "Multilingua non abilitato");
  #endif
}

// Handler per resettare il colore della modalità corrente al default
void handleResetModeColor(AsyncWebServerRequest *request) {
  resetModeColorToDefault((uint8_t)currentMode);

  // Carica il nuovo colore e applicalo
  uint8_t r, g, b;
  loadModeColor((uint8_t)currentMode, r, g, b);
  userColor.r = r;
  userColor.g = g;
  userColor.b = b;
  currentColor = userColor;

  // Aggiorna anche EEPROM globale per compatibilità
  EEPROM.write(EEPROM_COLOR_R_ADDR, r);
  EEPROM.write(EEPROM_COLOR_G_ADDR, g);
  EEPROM.write(EEPROM_COLOR_B_ADDR, b);
  EEPROM.commit();

  webForceDisplayUpdate = true;

  // Restituisci il nuovo colore in formato hex
  char colorHex[8];
  sprintf(colorHex, "#%02X%02X%02X", r, g, b);
  Serial.printf("[SETTINGS] Reset colore mode %d a default: %s\n", (int)currentMode, colorHex);
  request->send(200, "text/plain", colorHex);
}

// Handler per salvare calibrazione BME280
void handleSaveBME280Calibration(AsyncWebServerRequest *request) {
  Serial.println("[BME280 CALIB] Salvataggio calibrazione...");

  bool changed = false;

  // Leggi offset temperatura (range: -10.0 a +10.0)
  if (request->hasParam("tempOffset")) {
    float val = request->getParam("tempOffset")->value().toFloat();
    // Limita il valore al range ammesso
    if (val < -10.0f) val = -10.0f;
    if (val > 10.0f) val = 10.0f;

    if (val != bme280TempOffset) {
      bme280TempOffset = val;
      // Salva come int16 * 10
      int16_t tempOffsetRaw = (int16_t)(val * 10.0f);
      EEPROM.write(EEPROM_BME280_TEMP_OFFSET_ADDR, (tempOffsetRaw >> 8) & 0xFF);
      EEPROM.write(EEPROM_BME280_TEMP_OFFSET_ADDR + 1, tempOffsetRaw & 0xFF);
      changed = true;
      Serial.printf("[BME280 CALIB] Offset temperatura: %.1f°C\n", bme280TempOffset);
    }
  }

  // Leggi offset umidità (range: -20.0 a +20.0)
  if (request->hasParam("humOffset")) {
    float val = request->getParam("humOffset")->value().toFloat();
    // Limita il valore al range ammesso
    if (val < -20.0f) val = -20.0f;
    if (val > 20.0f) val = 20.0f;

    if (val != bme280HumOffset) {
      bme280HumOffset = val;
      // Salva come int16 * 10
      int16_t humOffsetRaw = (int16_t)(val * 10.0f);
      EEPROM.write(EEPROM_BME280_HUM_OFFSET_ADDR, (humOffsetRaw >> 8) & 0xFF);
      EEPROM.write(EEPROM_BME280_HUM_OFFSET_ADDR + 1, humOffsetRaw & 0xFF);
      changed = true;
      Serial.printf("[BME280 CALIB] Offset umidità: %.1f%%\n", bme280HumOffset);
    }
  }

  if (changed) {
    // Scrivi marker validità calibrazione
    EEPROM.write(EEPROM_BME280_CALIB_VALID_ADDR, EEPROM_BME280_CALIB_VALID_VALUE);
    EEPROM.commit();
    Serial.println("[BME280 CALIB] ✓ Calibrazione salvata con successo");

    // Forza rilettura immediata del sensore per applicare i nuovi offset
    if (bme280Available) {
      readBME280Temperature();
    }
  } else {
    Serial.println("[BME280 CALIB] Nessuna modifica");
  }

  // Restituisci JSON con i nuovi valori
  String json = "{";
  json += "\"success\": true,";
  json += "\"tempOffset\": " + String(bme280TempOffset, 1) + ",";
  json += "\"humOffset\": " + String(bme280HumOffset, 1) + ",";
  json += "\"tempIndoor\": " + String(temperatureIndoor, 1) + ",";
  json += "\"humIndoor\": " + String(humidityIndoor, 0);
  json += "}";
  request->send(200, "application/json", json);
}

// Handler per resettare calibrazione BME280 a zero
void handleResetBME280Calibration(AsyncWebServerRequest *request) {
  Serial.println("[BME280 CALIB] Reset calibrazione a valori di default...");

  bme280TempOffset = 0.0f;
  bme280HumOffset = 0.0f;

  // Salva offset a zero
  EEPROM.write(EEPROM_BME280_TEMP_OFFSET_ADDR, 0);
  EEPROM.write(EEPROM_BME280_TEMP_OFFSET_ADDR + 1, 0);
  EEPROM.write(EEPROM_BME280_HUM_OFFSET_ADDR, 0);
  EEPROM.write(EEPROM_BME280_HUM_OFFSET_ADDR + 1, 0);
  EEPROM.write(EEPROM_BME280_CALIB_VALID_ADDR, EEPROM_BME280_CALIB_VALID_VALUE);
  EEPROM.commit();

  Serial.println("[BME280 CALIB] ✓ Reset completato");

  // Forza rilettura immediata del sensore
  if (bme280Available) {
    readBME280Temperature();
  }

  // Restituisci JSON con i nuovi valori
  String json = "{";
  json += "\"success\": true,";
  json += "\"tempOffset\": 0.0,";
  json += "\"humOffset\": 0.0,";
  json += "\"tempIndoor\": " + String(temperatureIndoor, 1) + ",";
  json += "\"humIndoor\": " + String(humidityIndoor, 0);
  json += "}";
  request->send(200, "application/json", json);
}

// ================== REGISTRAZIONE ENDPOINTS ==================

void setup_settings_webserver(AsyncWebServer* server) {
  Serial.println("=====================================");
  Serial.println("[SETTINGS WEB] Registrazione endpoints...");

  // Carica modalità abilitate da EEPROM
  loadEnabledModes();

  // Carica rainbow mode da EEPROM
  loadRainbowMode();

  // Carica lingua da EEPROM
  #ifdef ENABLE_MULTILANGUAGE
  initLanguage();
  #endif

  // Carica colore specifico per la modalità corrente
  uint8_t modeR, modeG, modeB;
  loadModeColor((uint8_t)currentMode, modeR, modeG, modeB);
  userColor.r = modeR;
  userColor.g = modeG;
  userColor.b = modeB;
  currentColor = userColor;
  Serial.printf("[SETTINGS WEB] Colore mode %d caricato: R%d G%d B%d\n", (int)currentMode, modeR, modeG, modeB);

  // Endpoint specifici prima di quello generico
  server->on("/settings/status", HTTP_GET, handleSettingsStatus);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/status");

  server->on("/settings/save", HTTP_GET, handleSettingsSave);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/save");

  server->on("/settings/config", HTTP_GET, handleSettingsConfig);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/config");

  server->on("/settings/reset", HTTP_GET, handleSettingsReset);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/reset");

  server->on("/settings/reboot", HTTP_GET, handleSettingsReboot);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/reboot");

  server->on("/settings/setmode", HTTP_GET, handleSettingsSetMode);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/setmode");

  server->on("/settings/savemodes", HTTP_GET, handleSettingsSaveModes);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/savemodes");

  server->on("/settings/getapikeys", HTTP_GET, handleGetApiKeys);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/getapikeys");

  server->on("/settings/saveapikeys", HTTP_GET, handleSaveApiKeys);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/saveapikeys");

  server->on("/settings/resetmodecolor", HTTP_GET, handleResetModeColor);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/resetmodecolor");

  server->on("/settings/setlanguage", HTTP_GET, handleSetLanguage);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/setlanguage");

  // Calibrazione BME280
  server->on("/settings/savebmecalib", HTTP_GET, handleSaveBME280Calibration);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/savebmecalib");

  server->on("/settings/resetbmecalib", HTTP_GET, handleResetBME280Calibration);
  Serial.println("[SETTINGS WEB] ✓ GET /settings/resetbmecalib");

  // Carica API keys da LittleFS all'avvio
  loadApiKeysFromLittleFS();

  // Pagina principale (deve essere l'ultima)
  server->on("/settings", HTTP_GET, handleSettingsPage);
  Serial.println("[SETTINGS WEB] ✓ GET /settings (pagina principale)");

  Serial.println("[SETTINGS WEB] Tutti gli endpoints registrati!");
  Serial.println("=====================================");
}
