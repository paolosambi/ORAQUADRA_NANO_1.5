// ===================================================================
//                        EFFETTO OROLOGIO ANALOGICO
// ===================================================================
// Orologio con lancette e display digitale centrato
// Richiamabile tramite touch come gli altri effetti
//
// NOTA: Due opzioni per usare un'immagine personalizzata come quadrante:
//
// OPZIONE 1 - Immagine embedded (da PROGMEM):
// 1. Converti l'immagine con: python convert_image_to_c.py ROLEX.png rolex_clock.h rolex_face_480x480 480
// 2. Decommenta: #define USE_CUSTOM_CLOCK_IMAGE
// 3. Decommenta: #include "rolex_clock.h"
// 4. Ricompila e carica il programma
//
// OPZIONE 2 - Carica JPEG da SD card (CONSIGLIATO):
// 1. Copia il file JPG sulla SD (es. /rolex.jpg, /clock1.jpg, etc.)
// 2. Decommenta: #define USE_SD_CLOCK_IMAGE
// 3. Modifica CLOCK_SD_FILENAME con il nome del tuo file
// 4. Ricompila e carica il programma
// VANTAGGI: Non occupa PROGMEM, puoi cambiare immagine senza ricompilare!

#ifdef EFFECT_CLOCK

// Extern per fonte dati sensore ambiente (0=nessuno, 1=BME280, 2=radar)
extern uint8_t indoorSensorSource;

// ===== VARIABILI GLOBALI EFFECT_CLOCK =====
bool clockInitNeeded = true;    // Flag per indicare se è necessaria l'inizializzazione

// ===== FUNZIONE: Verifica se EFFECT_CLOCK può essere usato =====
// Sensore ambiente OPZIONALE - funziona anche solo con OpenWeatherMap
// Se nessun sensore presente, mostra solo dati meteo esterni
bool canUseEffectClock() {
  // Sensore ambiente è opzionale - solo avviso se non presente
  if (indoorSensorSource == 0) {
    Serial.println("canUseEffectClock: Nessun sensore ambiente disponibile (opzionale - solo dati esterni)");
  } else {
    Serial.printf("canUseEffectClock: Sensore ambiente OK (fonte: %s)\n",
                  indoorSensorSource == 1 ? "BME280" : "Radar");
  }

  // Verifica città meteo (Open-Meteo non richiede API key)
  extern String apiOpenWeatherCity;
  if (apiOpenWeatherCity.length() < 2) {
    Serial.println("canUseEffectClock: Città meteo non configurata (inserirla da /settings)");
    // Continua comunque - l'orologio funziona anche senza meteo
  }

  // SEMPRE ritorna true - l'orologio funziona sempre
  Serial.println("canUseEffectClock: OK - Modalita' orologio disponibile");
  return true;
}

// ===== CONFIGURAZIONE IMMAGINE QUADRANTE PERSONALIZZATA =====

// OPZIONE 1: Immagine embedded in PROGMEM (file .h)
//#define USE_CUSTOM_CLOCK_IMAGE

#ifdef USE_CUSTOM_CLOCK_IMAGE
// Decommenta e modifica con il nome del tuo file header
#include "rolex_clock.h"

// Configurazione dimensioni immagine personalizzata
#define CLOCK_IMAGE_SIZE 480       // Dimensione dell'immagine quadrata (schermo intero 480x480)
#define CLOCK_IMAGE_ARRAY rolex_face_480x480  // Nome dell'array nel file .h
#endif

// OPZIONE 2: Carica JPEG da SD card (più flessibile!)
#define USE_SD_CLOCK_IMAGE

// ===== CONFIGURAZIONE MULTI-SKIN =====

// OPZIONE: Skin casuale all'avvio (decommenta per attivare)
// Se attiva, ignora la skin salvata in EEPROM e ne carica una casuale
//#define CLOCK_RANDOM_SKIN_ON_BOOT

// Struttura per configurare ogni skin
struct ClockSkin {
  const char* filename;     // Nome del file sulla SD (es. "/rolex.jpg")
  const char* name;         // Nome descrittivo della skin

  // Configurazione centro orologio (se diverso da 240, 240)
  int16_t centerX;
  int16_t centerY;

  // Configurazione lancetta delle ore
  struct {
    int16_t length;         // Lunghezza lancetta
    uint8_t thickness;      // Spessore
    Color color;            // Colore RGB
  } hourHand;

  // Configurazione lancetta dei minuti
  struct {
    int16_t length;
    uint8_t thickness;
    Color color;
  } minuteHand;

  // Configurazione lancetta dei secondi
  struct {
    int16_t length;
    uint8_t thickness;
    Color color;
    bool visible;           // Alcuni orologi non hanno la lancetta dei secondi
  } secondHand;

  // Configurazione lancetta GMT (opzionale - fa un giro completo ogni minuto)
  struct {
    int16_t centerX;        // Posizione X del centro del sotto-quadrante GMT
    int16_t centerY;        // Posizione Y del centro del sotto-quadrante GMT
    int16_t length;
    uint8_t thickness;
    Color color;
    bool visible;           // true se questa skin ha la lancetta GMT
  } gmtHand;

  // Configurazione sotto-quadranti aggiuntivi
  struct {
    bool showDate;          // Mostra sotto-quadrante data (sinistra-basso)
    int16_t dateCenterX;
    int16_t dateCenterY;

    bool showDigitalTime;   // Mostra sotto-quadrante orario digitale (destra-basso)
    int16_t timeCenterX;
    int16_t timeCenterY;

    bool showTemperature;   // Mostra sotto-quadrante temperatura (centro-basso)
    int16_t tempCenterX;
    int16_t tempCenterY;
  } subdials;
};

// Array di skin disponibili (PERSONALIZZA QUI!)
ClockSkin clockSkins[] = {
  // SKIN 0: Daytona
  {
    "/daytona.jpg",
    "Rolex Daytona",
    240, 240,
    { 120, 8, Color(255, 215, 0) },     // ore: oro - ALLUNGATA da 80 a 120
    { 180, 6, Color(255, 215, 0) },     // minuti: oro - ALLUNGATA da 120 a 180
    { 200, 2, Color(255, 0, 0), true }, // secondi: rosso - ALLUNGATA da 140 a 200
    { 0, 0, 0, 0, Color(0, 0, 0), false },    // GMT: non visibile
    { true, 375, 240, false, 0, 0, false, 0, 0 }  // Subdials: SOLO Date (centro-destra X=375 Y=240, NERO, senza cerchio)
  },

  // SKIN 1: GMT Master
  {
    "/gmt.jpg",
    "Rolex GMT Master II",
    240, 240,
    { 120, 9, Color(255, 255, 255) },   // ore: bianco - ALLUNGATA da 95 a 120
    { 180, 7, Color(255, 255, 255) },   // minuti: bianco - ALLUNGATA da 145 a 180
    { 200, 2, Color(0, 100, 255), true }, // secondi: blu - ALLUNGATA da 165 a 200 (sostituita da icona meteo)
    { 240, 120, 50, 3, Color(255, 0, 0), true },  // GMT: centerX, centerY (posizione sotto-quadrante)
    { true, 120, 240, true, 360, 240, true, 240, 360 }  // Subdials: Date(sx X=120 Y=240), Time(dx X=360 Y=240), Temp(centro Y=360)
  }

  // AGGIUNGI QUI ALTRE SKIN...
};

const uint8_t NUM_CLOCK_SKINS = sizeof(clockSkins) / sizeof(clockSkins[0]);
uint8_t currentClockSkin = 1;  // Skin attualmente selezionata (default: 1 = GMT Master)

// Variabili globali per orologio
uint8_t lastClockHour = 255;
uint8_t lastClockMinute = 255;
uint8_t lastClockSecond = 255;
uint32_t lastClockUpdate = 0;
bool showDigitalTime = true;

// Variabili per dirty tracking subdial (ottimizzazione rendering)
static float lastDisplayedTempIndoor = -999.0;
static float lastDisplayedTempOutdoor = -999.0;
static float lastDisplayedHumIndoor = -999.0;
static float lastDisplayedHumOutdoor = -999.0;
static uint8_t lastDisplayedDayClock = 255;  // Rinominato per evitare conflitto con 12_BTTF.ino
static uint8_t lastDisplayedHourDigital = 255;
static uint8_t lastDisplayedMinuteDigital = 255;

// Buffer PSRAM per quadrante (480x480 pixel RGB565 = 460KB)
uint16_t* clockFaceBuffer = nullptr;      // Buffer sfondo (non modificato)
uint16_t* clockSkinFrameBuffer = nullptr; // Frame buffer per double buffering
OffscreenGFX* clockSkinOffscreenGfx = nullptr;  // GFX offscreen per disegnare sul frame buffer
bool clockFaceBufferLoaded = false;
String loadedSkinFilename = "";  // Tiene traccia di quale skin è caricata nel buffer

// Variabili per le lancette (struttura ClockHand definita nel file principale)
ClockHand hourHand;
ClockHand minuteHand;
ClockHand secondHand;
ClockHand gmtHand;       // Lancetta GMT (opzionale)

// Flag per distinguere tra "rientro nella modalità" e "cambio skin"
bool justEnteredClockMode = true;

// Variabili per anti-flickering lancette (memorizzano angoli precedenti)
float prevHourAngle = -1;
float prevMinuteAngle = -1;
float prevSecondAngle = -1;
float prevGmtAngle = -1;

// Target GFX per double buffering (punta a offscreen durante rendering, poi a gfx)
Arduino_GFX* clockTargetGfx = nullptr;

// ===================================================================
// DICHIARAZIONI FORWARD (funzioni definite più avanti in questo file)
// ===================================================================
void drawClockFace();
void drawDateSubdial();
void drawDigitalTimeSubdial();
void drawTemperatureSubdial();
void drawGmtSubdial();
void fetchOutdoorTemperature();
void drawWeatherIconOnly(int16_t centerX, int16_t centerY, String iconCode);
void drawWindArrow(int16_t centerX, int16_t centerY, float direction);
void restoreHandArea(float angle, uint8_t length, uint8_t thickness);

// Funzioni di disegno lancette (definite in 3_EFFETTI.ino)
extern void drawClockHandOffscreen(Arduino_GFX* targetGfx, int centerX, int centerY, float angle, int length, uint16_t color, int thickness);

// ===================================================================
// FUNZIONE: Ripristina area lancetta dal buffer (anti-flickering)
// ===================================================================
void restoreHandArea(float angle, uint8_t length, uint8_t thickness) {
  if (!clockFaceBufferLoaded || clockFaceBuffer == nullptr) return;
  if (angle < 0) return;  // Angolo non valido (prima iterazione)

  const int16_t centerX = 240;
  const int16_t centerY = 240;

  // Calcola punto finale della lancetta
  float radians = (angle - 90) * PI / 180.0f;
  int16_t endX = centerX + (int16_t)(cos(radians) * length);
  int16_t endY = centerY + (int16_t)(sin(radians) * length);

  // Calcola bounding box con margine per lo spessore
  int16_t margin = thickness + 4;
  int16_t x1 = min(centerX, endX) - margin;
  int16_t y1 = min(centerY, endY) - margin;
  int16_t x2 = max(centerX, endX) + margin;
  int16_t y2 = max(centerY, endY) + margin;

  // Clamp ai bordi dello schermo
  x1 = max((int16_t)0, x1);
  y1 = max((int16_t)0, y1);
  x2 = min((int16_t)479, x2);
  y2 = min((int16_t)479, y2);

  int16_t w = x2 - x1 + 1;
  int16_t h = y2 - y1 + 1;

  // Alloca buffer temporaneo e copia l'area dal buffer PSRAM
  uint16_t* tempBuf = (uint16_t*)malloc(w * h * 2);
  if (tempBuf) {
    // Copia riga per riga dal buffer PSRAM al buffer temporaneo
    for (int16_t row = 0; row < h; row++) {
      memcpy(&tempBuf[row * w], &clockFaceBuffer[(y1 + row) * 480 + x1], w * 2);
    }
    // Disegna tutto in una sola chiamata
    gfx->draw16bitRGBBitmap(x1, y1, tempBuf, w, h);
    free(tempBuf);
  }
}

// ===================================================================
// OVERLOAD: drawClockHand per struttura ClockHand
// ===================================================================
// Usa il centro dello schermo (240, 240) e i parametri dalla struttura ClockHand
void drawClockHand(ClockHand& hand) {
  // Centro dello schermo
  int centerX = 240;
  int centerY = 240;

  // Converti Color a uint16_t RGB565
  uint16_t color565 = convertColor(hand.color);

  // Chiama la funzione base definita in 3_EFFETTI.ino
  drawClockHand(centerX, centerY, hand.angle, hand.length, color565, hand.thickness);
}

// ===================================================================
// FUNZIONE: Carica skin salvata o random all'avvio
// ===================================================================
void loadSavedOrRandomClockSkin() {
#ifdef CLOCK_RANDOM_SKIN_ON_BOOT
  // Modalità random: carica una skin casuale
  currentClockSkin = random(NUM_CLOCK_SKINS);
  Serial.printf("Skin casuale all'avvio: %d (%s)\n", currentClockSkin, clockSkins[currentClockSkin].name);
#else
  // Forza sempre skin GMT Master (indice 1) come default
  currentClockSkin = 1;
  EEPROM.write(EEPROM_CLOCK_SKIN_ADDR, 1);
  EEPROM.commit();
  Serial.printf("Skin forzata a GMT Master: %d (%s)\n", currentClockSkin, clockSkins[currentClockSkin].name);
#endif
}

// ===================================================================
// FUNZIONE: Animazione fade out/in della backlight
// ===================================================================
void fadeBacklight(bool fadeOut, uint16_t duration) {
  uint8_t currentBrightnessRead = ledcRead(PWM_CHANNEL);
  // Usa luminosità basata su orari GIORNO/NOTTE e presenza radar
  extern uint8_t currentHour;
  extern bool radarAvailable;
  extern bool presenceDetected;

  uint8_t targetBrightness;
  if (fadeOut) {
    targetBrightness = 0;
  } else {
    // Fade in: rispetta la presenza se il radar è disponibile
    if (radarAvailable && !presenceDetected) {
      targetBrightness = 0;  // Nessuna presenza = display spento
    } else {
      // Con presenza (o senza radar): usa la luminosità appropriata in base all'orario
      if(checkIsNightTime(currentHour, currentMinute)) {
        targetBrightness = brightnessNight;  // Notte: luminosità configurabile
      } else {
        targetBrightness = brightnessDay;    // Giorno: luminosità configurabile
      }
    }
  }

  int steps = 20;  // Numero di step per il fade
  int delayPerStep = duration / steps;

  for (int i = 0; i <= steps; i++) {
    uint8_t brightness;
    if (fadeOut) {
      brightness = currentBrightnessRead - (currentBrightnessRead * i / steps);
    } else {
      brightness = (targetBrightness * i / steps);
    }
    ledcWrite(PWM_CHANNEL, brightness);
    delay(delayPerStep);
  }
}

// ===================================================================
// FUNZIONE: Cambia skin dell'orologio con animazione
// ===================================================================
void changeClockSkin(uint8_t skinIndex) {
  if (skinIndex >= NUM_CLOCK_SKINS) {
    Serial.printf("Errore: skin index %d non valido (max: %d)\n", skinIndex, NUM_CLOCK_SKINS - 1);
    return;
  }

  currentClockSkin = skinIndex;
  ClockSkin& skin = clockSkins[currentClockSkin];

  Serial.printf("Cambio skin: %s (%s)\n", skin.name, skin.filename);

  // ========== ANIMAZIONE: Fade out ==========
  fadeBacklight(true, 300);  // Fade out in 300ms

  // ========== FEEDBACK VISIVO: Mostra nome skin ==========
  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(CYAN);

  // Titolo
  const char* title = "SKIN OROLOGIO:";
  int titleWidth = strlen(title) * 18;
  int titleX = (480 - titleWidth) / 2;
  if (titleX < 0) titleX = 10;
  gfx->setCursor(titleX, 210);
  gfx->println(title);

  // Nome skin
  int nameWidth = strlen(skin.name) * 18;
  int nameX = (480 - nameWidth) / 2;
  if (nameX < 0) nameX = 10;
  gfx->setCursor(nameX, 250);
  gfx->setTextColor(WHITE);
  gfx->println(skin.name);

  // Indicatore numerazione (es: "2/4")
  char indicator[16];
  snprintf(indicator, sizeof(indicator), "%d/%d", skinIndex + 1, NUM_CLOCK_SKINS);
  int indicatorWidth = strlen(indicator) * 18;
  int indicatorX = (480 - indicatorWidth) / 2;
  if (indicatorX < 0) indicatorX = 10;
  gfx->setCursor(indicatorX, 290);
  gfx->setTextColor(YELLOW);
  gfx->println(indicator);

  // ========== SALVA SKIN IN EEPROM ==========
  EEPROM.write(EEPROM_CLOCK_SKIN_ADDR, skinIndex);
  EEPROM.commit();
  Serial.printf("Skin salvata in EEPROM: %d\n", skinIndex);

  // ========== ANIMAZIONE: Fade in ==========
  fadeBacklight(false, 500);  // Fade in in 500ms

  // Attendi 700ms prima di ridisegnare
  delay(700);

  // ========== ANIMAZIONE: Fade out prima del cambio ==========
  fadeBacklight(true, 300);  // Fade out veloce prima di caricare nuova skin

  // Se la skin è diversa da quella caricata, invalida il buffer
  Serial.printf("DEBUG: loadedSkinFilename='%s', skin.filename='%s'\n", loadedSkinFilename.c_str(), skin.filename);
  if (loadedSkinFilename != String(skin.filename)) {
    clockFaceBufferLoaded = false;
    loadedSkinFilename = "";  // Reset anche loadedSkinFilename
    Serial.println("Buffer invalidato, verrà ricaricata nuova skin");
  } else {
    Serial.println("ATTENZIONE: Skin già caricata nel buffer!");
  }

  // Applica configurazione lancette dalle variabili globali (configurabili da pagina web)
  hourHand.length = clockHourHandLength;
  hourHand.thickness = clockHourHandWidth;
  hourHand.color = Color((clockHourHandColor >> 11) << 3, ((clockHourHandColor >> 5) & 0x3F) << 2, (clockHourHandColor & 0x1F) << 3);

  minuteHand.length = clockMinuteHandLength;
  minuteHand.thickness = clockMinuteHandWidth;
  minuteHand.color = Color((clockMinuteHandColor >> 11) << 3, ((clockMinuteHandColor >> 5) & 0x3F) << 2, (clockMinuteHandColor & 0x1F) << 3);

  secondHand.length = clockSecondHandLength;
  secondHand.thickness = clockSecondHandWidth;
  secondHand.color = Color((clockSecondHandColor >> 11) << 3, ((clockSecondHandColor >> 5) & 0x3F) << 2, (clockSecondHandColor & 0x1F) << 3);

  gmtHand.length = skin.gmtHand.length;
  gmtHand.thickness = skin.gmtHand.thickness;
  gmtHand.color = skin.gmtHand.color;

  // Forza ridisegno completo
  clockInitNeeded = true;
  lastClockHour = 255;
  lastClockMinute = 255;
  lastClockSecond = 255;

  // ========== ANIMAZIONE FINALE: Fade in nuovo quadrante ==========
  // (verrà eseguito dopo che updateClockMode() ridisegna tutto)
  // Per ora restaura luminosità normale
  fadeBacklight(false, 400);  // Fade in finale in 400ms
}

// ===================================================================
// FUNZIONE: Passa alla prossima skin (ciclo)
// ===================================================================
void nextClockSkin() {
  uint8_t nextSkin = (currentClockSkin + 1) % NUM_CLOCK_SKINS;
  changeClockSkin(nextSkin);
}

// ===================================================================
// FUNZIONE: Passa alla skin precedente (ciclo)
// ===================================================================
void previousClockSkin() {
  uint8_t prevSkin = (currentClockSkin == 0) ? (NUM_CLOCK_SKINS - 1) : (currentClockSkin - 1);
  changeClockSkin(prevSkin);
}

// ===================================================================
// FUNZIONE PRINCIPALE: Aggiorna orologio analogico
// ===================================================================
void updateClockMode() {

  // INIZIALIZZAZIONE (solo una volta o dopo cambio skin)
  if (clockInitNeeded) {
    clockInitNeeded = false;

    // Pulisci schermo SOLO all'inizializzazione
    gfx->fillScreen(BLACK);

    // Reset buffer
    memset(activePixels, 0, sizeof(activePixels));
    memset(targetPixels, 0, sizeof(targetPixels));
    memset(pixelChanged, 0, sizeof(pixelChanged));

    // Se è il primo ingresso nella modalità CLOCK (da un'altra modalità), resetta a skin 0
    if (justEnteredClockMode) {
      // Carica skin salvata solo al primo avvio assoluto
      static bool firstInit = true;
      if (firstInit) {
        loadSavedOrRandomClockSkin();
        firstInit = false;
      } else {
        // Quando rientri in MODE_CLOCK, parte sempre dalla skin GMT (skin 1)
        currentClockSkin = 1;
        // Invalida buffer per forzare ricaricamento della skin corretta
        clockFaceBufferLoaded = false;
        loadedSkinFilename = "";
        Serial.println("Rientro in MODE_CLOCK: reset a skin 1 (GMT Master)");
      }
      // Reset angoli precedenti per anti-flickering (evita artefatti al primo frame)
      prevHourAngle = -1;
      prevMinuteAngle = -1;
      prevSecondAngle = -1;
      prevGmtAngle = -1;
      justEnteredClockMode = false;  // Ora siamo dentro, non è più "appena entrato"
    }

    // Applica configurazione lancette dalle variabili globali (configurabili da pagina web)
    ClockSkin& skin = clockSkins[currentClockSkin];

    hourHand.length = clockHourHandLength;
    hourHand.thickness = clockHourHandWidth;
    hourHand.color = Color((clockHourHandColor >> 11) << 3, ((clockHourHandColor >> 5) & 0x3F) << 2, (clockHourHandColor & 0x1F) << 3);

    minuteHand.length = clockMinuteHandLength;
    minuteHand.thickness = clockMinuteHandWidth;
    minuteHand.color = Color((clockMinuteHandColor >> 11) << 3, ((clockMinuteHandColor >> 5) & 0x3F) << 2, (clockMinuteHandColor & 0x1F) << 3);

    secondHand.length = clockSecondHandLength;
    secondHand.thickness = clockSecondHandWidth;
    secondHand.color = Color((clockSecondHandColor >> 11) << 3, ((clockSecondHandColor >> 5) & 0x3F) << 2, (clockSecondHandColor & 0x1F) << 3);

    gmtHand.length = skin.gmtHand.length;
    gmtHand.thickness = skin.gmtHand.thickness;
    gmtHand.color = skin.gmtHand.color;

    Serial.printf("Inizializzazione skin: %s\n", skin.name);

    lastClockHour = 255;
    lastClockMinute = 255;
    lastClockSecond = 255;

    // Reset dirty flags per forzare ridisegno subdial
    lastDisplayedTempIndoor = -999.0;
    lastDisplayedTempOutdoor = -999.0;
    lastDisplayedHumIndoor = -999.0;
    lastDisplayedHumOutdoor = -999.0;
    lastDisplayedDayClock = 255;
    lastDisplayedHourDigital = 255;
    lastDisplayedMinuteDigital = 255;

    // Disegna quadrante SOLO all'inizializzazione
    drawClockFace();
  }

  // Aggiorna ogni secondo
  uint32_t currentMillis = millis();
  if (currentMillis - lastClockUpdate < 1000) {
    return;
  }
  lastClockUpdate = currentMillis;

  // Leggi temperatura dal sensore BME280 ogni 30 secondi (non ad ogni frame)
  static uint32_t lastBME280Read = 0;
  if (currentMillis - lastBME280Read >= 30000) {  // Ogni 30 secondi
    readBME280Temperature();
    lastBME280Read = currentMillis;
  }

  // Aggiorna temperatura esterna da OpenWeatherMap (ogni 10 minuti)
  fetchOutdoorTemperature();

  // Controlla se orario è cambiato
  bool hourMinuteChanged = (currentHour != lastClockHour || currentMinute != lastClockMinute);
  bool secondChanged = (currentSecond != lastClockSecond);

  if (!hourMinuteChanged && !secondChanged) {
    return;  // Nessun aggiornamento necessario
  }

  // Cancella area centrale (solo per lancette, non tutto lo schermo)
  int16_t centerX = 240;
  int16_t centerY = 240;

#ifdef USE_SD_CLOCK_IMAGE
  // ===== DOUBLE BUFFERING COMPLETO: ZERO FLICKERING =====
  if (clockFaceBufferLoaded && clockFaceBuffer != nullptr &&
      clockSkinFrameBuffer != nullptr && clockSkinOffscreenGfx != nullptr) {

    // FASE 1: Copia lo sfondo nel frame buffer
    memcpy(clockSkinFrameBuffer, clockFaceBuffer, 480 * 480 * sizeof(uint16_t));

    // FASE 2: Calcola angoli delle lancette
    hourHand.angle = ((currentHour % 12) * 30.0f) + (currentMinute * 0.5f);
    minuteHand.angle = (currentMinute * 6.0f) + (currentSecond * 0.1f);
    secondHand.angle = currentSecond * 6.0f;
    gmtHand.angle = currentSecond * 6.0f;

    // Ottieni configurazione skin
    ClockSkin& skin = clockSkins[currentClockSkin];

    // FASE 3: Imposta target GFX per disegnare sul buffer offscreen
    clockTargetGfx = clockSkinOffscreenGfx;

    // FASE 4: Disegna sotto-quadranti SUL BUFFER (prima delle lancette)
    if (skin.subdials.showDate) {
      drawDateSubdial();
    }
    if (skin.subdials.showDigitalTime) {
      drawDigitalTimeSubdial();
    }
    if (skin.subdials.showTemperature) {
      drawTemperatureSubdial();
    }
    if (skin.gmtHand.visible) {
      drawGmtSubdial();
    }

    // FASE 5: Disegna lancette SUL BUFFER (per ultime = sempre in primo piano)
    // Usa le variabili globali configurabili dalla pagina web (colore, lunghezza, larghezza)
    // Lancetta ore
    drawClockHandOffscreen(clockSkinOffscreenGfx, centerX, centerY,
                           hourHand.angle, clockHourHandLength, clockHourHandColor, clockHourHandWidth);

    // Lancetta minuti
    drawClockHandOffscreen(clockSkinOffscreenGfx, centerX, centerY,
                           minuteHand.angle, clockMinuteHandLength, clockMinuteHandColor, clockMinuteHandWidth);

    // Lancetta secondi (sempre in primo piano assoluto)
    drawClockHandOffscreen(clockSkinOffscreenGfx, centerX, centerY,
                           secondHand.angle, clockSecondHandLength, clockSecondHandColor, clockSecondHandWidth);

    // FASE 6: Resetta target GFX
    clockTargetGfx = nullptr;

    // FASE 7: Trasferisci TUTTO al display in un colpo solo (zero flickering)
    gfx->draw16bitRGBBitmap(0, 0, clockSkinFrameBuffer, 480, 480);

    // Aggiorna variabili
    lastClockHour = currentHour;
    lastClockMinute = currentMinute;
    lastClockSecond = currentSecond;

    return;  // Tutto fatto con double buffering completo
  } else {
    // Fallback: ricarica JPEG (non dovrebbe mai succedere)
    drawJpegFromSD(clockSkins[currentClockSkin].filename, 0, 0, 480, 480);
  }

#elif defined(USE_CUSTOM_CLOCK_IMAGE)
  // Con immagine personalizzata da PROGMEM: ridisegna solo l'area delle lancette
  int16_t imgX = centerX - (CLOCK_IMAGE_SIZE / 2);
  int16_t imgY = centerY - (CLOCK_IMAGE_SIZE / 2);

  // Alloca buffer temporaneo per ridisegnare l'immagine
  uint16_t* buffer = (uint16_t*)malloc(CLOCK_IMAGE_SIZE * CLOCK_IMAGE_SIZE * 2);
  if (buffer) {
    memcpy_P(buffer, CLOCK_IMAGE_ARRAY, CLOCK_IMAGE_SIZE * CLOCK_IMAGE_SIZE * 2);
    gfx->draw16bitRGBBitmap(imgX, imgY, buffer, CLOCK_IMAGE_SIZE, CLOCK_IMAGE_SIZE);
    free(buffer);
  } else {
    // Fallback: cancella solo l'area centrale
    gfx->fillCircle(centerX, centerY, CLOCK_IMAGE_SIZE / 2, BLACK);
  }
#else
  // Con quadrante tradizionale: cancella e ridisegna
  gfx->fillCircle(centerX, centerY, 165, BLACK);  // Cancella solo area lancette

  // ========== RIDISEGNA NUMERI ROMANI (erano stati cancellati) ==========
  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(convertColor(Color(255, 255, 255)));

  int16_t romanRadius = 140;

  // XII (12 ore) - in alto
  gfx->setCursor(centerX - 12, centerY - romanRadius + 10);
  gfx->print("XII");

  // III (3 ore) - destra
  gfx->setCursor(centerX + romanRadius - 15, centerY + 5);
  gfx->print("III");

  // VI (6 ore) - basso
  gfx->setCursor(centerX - 8, centerY + romanRadius + 5);
  gfx->print("VI");

  // IX (9 ore) - sinistra
  gfx->setCursor(centerX - romanRadius + 5, centerY + 5);
  gfx->print("IX");

  // Ridisegna centro (perno lancette)
  gfx->fillCircle(centerX, centerY, 8, convertColor(Color(255, 255, 255)));
  gfx->drawCircle(centerX, centerY, 8, convertColor(Color(0, 0, 0)));
#endif

  // ========== AREE DATA/ORARIO NON NECESSARIE (DISATTIVATE) ==========
  // (Se riattivi data/orario, decommenta queste righe per pulire le aree)
  // gfx->fillRect(50, 5, 380, 30, BLACK);    // Area data in alto
  // gfx->fillRect(100, 440, 280, 40, BLACK); // Area orario in basso

  // Aggiorna variabili
  lastClockHour = currentHour;
  lastClockMinute = currentMinute;
  lastClockSecond = currentSecond;

  // Calcola angoli lancette (0° = ore 12, senso orario)
  hourHand.angle = ((currentHour % 12) * 30.0f) + (currentMinute * 0.5f);
  minuteHand.angle = (currentMinute * 6.0f) + (currentSecond * 0.1f);
  secondHand.angle = currentSecond * 6.0f;
  gmtHand.angle = currentSecond * 6.0f;  // Fa un giro completo ogni minuto (60 secondi)

  // Disegna sotto-quadranti aggiuntivi se la skin li ha
  // NOTA: Devono essere ridisegnati ogni volta perché il quadrante li cancella
  ClockSkin& skin = clockSkins[currentClockSkin];

  // Date subdial (sempre visibile)
  if (skin.subdials.showDate) {
    drawDateSubdial();
  }

  // Digital time subdial (sempre visibile)
  if (skin.subdials.showDigitalTime) {
    drawDigitalTimeSubdial();
  }

  // Temperature subdial (sempre visibile)
  if (skin.subdials.showTemperature) {
    drawTemperatureSubdial();
  }

  // GMT subdial (si aggiorna ogni secondo con la lancetta)
  if (skin.gmtHand.visible) {
    drawGmtSubdial();
  }

  // Disegna lancette principali (ordine: ore, minuti, secondi)
  drawClockHand(hourHand);
  drawClockHand(minuteHand);
  drawClockHand(secondHand);  // Lancetta secondi centrale sempre visibile

  // Salva angoli correnti per anti-flickering (prossima iterazione)
  prevHourAngle = hourHand.angle;
  prevMinuteAngle = minuteHand.angle;
  prevSecondAngle = secondHand.angle;
  prevGmtAngle = gmtHand.angle;

  // ========== DATA E ORARIO DIGITALE DISATTIVATI ==========
  // Per riattivare: rimuovi "//" dalle righe seguenti
  // if (showDigitalTime) {
  //   drawDigitalTime();
  // }
}

// ===================================================================
// DISEGNA IL QUADRANTE CIRCOLARE ACCATTIVANTE
// ===================================================================
void drawClockFace() {
  // Centro dello schermo (240, 240)
  int16_t centerX = 240;
  int16_t centerY = 240;

#ifdef USE_SD_CLOCK_IMAGE
  // ========== OPZIONE 2: CARICA JPEG DA SD CARD (con buffer PSRAM) ==========

  // Ottieni configurazione della skin corrente
  ClockSkin& skin = clockSkins[currentClockSkin];

  // Carica il JPEG una sola volta nel buffer PSRAM (o ricarica se skin cambiata)
  if (!clockFaceBufferLoaded || loadedSkinFilename != String(skin.filename)) {
    Serial.printf("Caricamento skin '%s' in PSRAM...\n", skin.name);

    // Alloca buffer PSRAM per sfondo (480x480 pixel RGB565 = 460 KB)
    if (clockFaceBuffer == nullptr) {
      clockFaceBuffer = (uint16_t*)ps_malloc(480 * 480 * 2);
      if (clockFaceBuffer == nullptr) {
        Serial.println("ERRORE: Impossibile allocare buffer PSRAM per quadrante!");
        // Fallback: disegna quadrante tradizionale
        gfx->fillScreen(BLACK);
        gfx->drawCircle(centerX, centerY, 200, convertColor(currentColor));
        return;
      }
      Serial.println("Buffer sfondo PSRAM allocato (460 KB)");
    }

    // Alloca frame buffer per double buffering
    if (clockSkinFrameBuffer == nullptr) {
      clockSkinFrameBuffer = (uint16_t*)ps_malloc(480 * 480 * 2);
      if (clockSkinFrameBuffer == nullptr) {
        Serial.println("ERRORE: Impossibile allocare frame buffer PSRAM!");
      } else {
        Serial.println("Frame buffer PSRAM allocato (460 KB)");
      }
    }

    // Crea OffscreenGFX per disegnare sul frame buffer
    if (clockSkinOffscreenGfx == nullptr && clockSkinFrameBuffer != nullptr) {
      clockSkinOffscreenGfx = new OffscreenGFX(clockSkinFrameBuffer, 480, 480);
      Serial.println("OffscreenGFX creato per MODE_CLOCK");
    }

    // Carica JPEG direttamente nel buffer PSRAM
    if (loadJpegToBuffer(skin.filename, clockFaceBuffer, 480, 480)) {
      // JPEG caricato con successo nel buffer!
      clockFaceBufferLoaded = true;
      loadedSkinFilename = String(skin.filename);
      Serial.printf("Skin '%s' caricata in PSRAM!\n", skin.name);

      // Copia il buffer sul display per la prima visualizzazione
      gfx->draw16bitRGBBitmap(0, 0, clockFaceBuffer, 480, 480);
    } else {
      // Fallback: JPEG non trovato
      Serial.printf("JPEG '%s' non trovato, uso quadrante tradizionale\n", skin.filename);
      free(clockFaceBuffer);
      clockFaceBuffer = nullptr;
      clockFaceBufferLoaded = false;
      loadedSkinFilename = "";
      gfx->fillScreen(BLACK);
      gfx->drawCircle(centerX, centerY, 200, convertColor(currentColor));
    }
  } else {
    // Buffer già caricato, copialo velocemente sul display (molto più veloce di ricaricare JPEG!)
    gfx->draw16bitRGBBitmap(0, 0, clockFaceBuffer, 480, 480);
  }

#elif defined(USE_CUSTOM_CLOCK_IMAGE)
  // ========== OPZIONE 1: DISEGNA IMMAGINE DA PROGMEM ==========
  // Calcola posizione per centrare l'immagine
  int16_t imgX = centerX - (CLOCK_IMAGE_SIZE / 2);
  int16_t imgY = centerY - (CLOCK_IMAGE_SIZE / 2);

  // Alloca buffer temporaneo in RAM per l'immagine
  uint16_t* buffer = (uint16_t*)malloc(CLOCK_IMAGE_SIZE * CLOCK_IMAGE_SIZE * 2);
  if (buffer) {
    // Copia l'immagine dalla PROGMEM alla RAM
    memcpy_P(buffer, CLOCK_IMAGE_ARRAY, CLOCK_IMAGE_SIZE * CLOCK_IMAGE_SIZE * 2);

    // Disegna l'immagine centrata
    gfx->draw16bitRGBBitmap(imgX, imgY, buffer, CLOCK_IMAGE_SIZE, CLOCK_IMAGE_SIZE);

    // Libera la memoria
    free(buffer);
  } else {
    // Fallback: se non c'è memoria, disegna un cerchio semplice
    gfx->drawCircle(centerX, centerY, CLOCK_IMAGE_SIZE / 2, convertColor(currentColor));
  }

#else
  // ========== DISEGNA QUADRANTE TRADIZIONALE ==========
  int16_t radius = 200;  // Raggio del quadrante

  // ========== CERCHI DECORATIVI CONCENTRICI ==========
  // Bordo esterno doppio (effetto cornice)
  gfx->drawCircle(centerX, centerY, radius, convertColor(currentColor));
  gfx->drawCircle(centerX, centerY, radius - 1, convertColor(currentColor));
  gfx->drawCircle(centerX, centerY, radius - 2, convertColor(currentColor));

  // Cerchio interno decorativo
  gfx->drawCircle(centerX, centerY, radius - 15, convertColor(Color(100, 100, 100)));

  // Cerchio centrale sottile
  gfx->drawCircle(centerX, centerY, 50, convertColor(Color(80, 80, 80)));

  // ========== NUMERI ROMANI ALLE ORE PRINCIPALI ==========
  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(convertColor(Color(255, 255, 255)));

  // Posiziona i numeri romani a metà raggio (140px dal centro)
  int16_t romanRadius = 140;

  // XII (12 ore) - in alto (angolo -90°)
  gfx->setCursor(centerX - 12, centerY - romanRadius + 10);
  gfx->print("XII");

  // III (3 ore) - destra (angolo 0°)
  gfx->setCursor(centerX + romanRadius - 15, centerY + 5);
  gfx->print("III");

  // VI (6 ore) - basso (angolo 90°)
  gfx->setCursor(centerX - 8, centerY + romanRadius + 5);
  gfx->print("VI");

  // IX (9 ore) - sinistra (angolo 180°)
  gfx->setCursor(centerX - romanRadius + 5, centerY + 5);
  gfx->print("IX");

  // ========== TACCHE DELLE ORE (60 tacche totali) ==========
  for (int i = 0; i < 60; i++) {
    float angle = i * 6.0f;  // 6° per ogni tacca (60 tacche)
    float rad = radians(angle - 90);

    bool isHour = (i % 5 == 0);  // Tacca dell'ora ogni 5 posizioni

    int16_t innerRadius, outerRadius;
    Color tickColor;

    if (isHour) {
      // Tacche ore (12 tacche principali)
      innerRadius = radius - 25;
      outerRadius = radius - 5;
      tickColor = Color(255, 200, 0);  // Oro
    } else {
      // Tacche minuti (48 tacche piccole)
      innerRadius = radius - 12;
      outerRadius = radius - 5;
      tickColor = Color(150, 150, 150);  // Grigio
    }

    // Calcola coordinate
    int16_t x1 = centerX + cos(rad) * innerRadius;
    int16_t y1 = centerY + sin(rad) * innerRadius;
    int16_t x2 = centerX + cos(rad) * outerRadius;
    int16_t y2 = centerY + sin(rad) * outerRadius;

    // Disegna tacca
    gfx->drawLine(x1, y1, x2, y2, convertColor(tickColor));

    // Tacche ore più spesse
    if (isHour) {
      gfx->drawLine(x1 + 1, y1, x2 + 1, y2, convertColor(tickColor));
      gfx->drawLine(x1, y1 + 1, x2, y2 + 1, convertColor(tickColor));
    }
  }

  // ========== PUNTI LUMINOSI ALLE ORE ==========
  for (int i = 0; i < 12; i++) {
    float angle = i * 30.0f;
    float rad = radians(angle - 90);

    int16_t dotX = centerX + cos(rad) * (radius - 30);
    int16_t dotY = centerY + sin(rad) * (radius - 30);

    // Punto luminoso (piccolo cerchio)
    gfx->fillCircle(dotX, dotY, 3, convertColor(Color(255, 200, 0)));
    gfx->drawCircle(dotX, dotY, 3, convertColor(Color(255, 255, 255)));
  }

  // ========== CENTRO DECORATIVO CON EFFETTO 3D ==========
  // Ombra esterna (effetto profondità)
  gfx->fillCircle(centerX + 2, centerY + 2, 12, convertColor(Color(50, 50, 50)));

  // Cerchio esterno scuro
  gfx->fillCircle(centerX, centerY, 12, convertColor(Color(80, 80, 80)));

  // Cerchio medio (metallico)
  gfx->fillCircle(centerX, centerY, 10, convertColor(Color(180, 180, 180)));

  // Cerchio interno chiaro (riflessione luce)
  gfx->fillCircle(centerX - 1, centerY - 1, 8, convertColor(Color(220, 220, 220)));

  // Centro finale
  gfx->fillCircle(centerX, centerY, 6, convertColor(Color(255, 255, 255)));

  // Bordo nero centrale
  gfx->drawCircle(centerX, centerY, 6, convertColor(Color(0, 0, 0)));
#endif
}

// ===================================================================
// DISEGNA UNA LANCETTA A FORMA DI FRECCIA
// ===================================================================
void drawClockHand(const ClockHand& hand) {
  // Centro dello schermo
  int16_t centerX = 240;
  int16_t centerY = 240;

  // Converti angolo a radianti (-90 perché 0° è alle ore 3)
  float rad = radians(hand.angle - 90);
  float radPerp = rad + PI / 2;  // Angolo perpendicolare per larghezza

  // Usa direttamente i parametri della lancetta
  float lengthPx = hand.length;  // Lunghezza in pixel dalla configurazione

  // Usa lo spessore dalla configurazione
  float width = hand.thickness / 2.0f;  // Dividi per 2 perché verrà usato come raggio

  // Calcola coordinate punto finale (punta)
  int16_t tipX = centerX + cos(rad) * lengthPx;
  int16_t tipY = centerY + sin(rad) * lengthPx;

  // Calcola coordinate base (coda)
  int16_t baseX = centerX - cos(rad) * 10;  // Leggermente dietro il centro
  int16_t baseY = centerY - sin(rad) * 10;

  // Calcola i 4 punti della freccia
  int16_t left1X = baseX + cos(radPerp) * width;
  int16_t left1Y = baseY + sin(radPerp) * width;
  int16_t right1X = baseX - cos(radPerp) * width;
  int16_t right1Y = baseY - sin(radPerp) * width;

  int16_t left2X = tipX - cos(rad) * 15 + cos(radPerp) * (width / 2);
  int16_t left2Y = tipY - sin(rad) * 15 + sin(radPerp) * (width / 2);
  int16_t right2X = tipX - cos(rad) * 15 - cos(radPerp) * (width / 2);
  int16_t right2Y = tipY - sin(rad) * 15 - sin(radPerp) * (width / 2);

  uint16_t color565 = convertColor(hand.color);
  uint16_t shadowColor = convertColor(Color(50, 50, 50));

  // Disegna ombra (effetto 3D)
  gfx->drawLine(left1X + 2, left1Y + 2, left2X + 2, left2Y + 2, shadowColor);
  gfx->drawLine(right1X + 2, right1Y + 2, right2X + 2, right2Y + 2, shadowColor);
  gfx->drawLine(left2X + 2, left2Y + 2, tipX + 2, tipY + 2, shadowColor);
  gfx->drawLine(right2X + 2, right2Y + 2, tipX + 2, tipY + 2, shadowColor);

  // Disegna corpo principale lancetta
  gfx->drawLine(left1X, left1Y, left2X, left2Y, color565);
  gfx->drawLine(right1X, right1Y, right2X, right2Y, color565);
  gfx->drawLine(left2X, left2Y, tipX, tipY, color565);
  gfx->drawLine(right2X, right2Y, tipX, tipY, color565);

  // Riempimento (disegna linee parallele per riempire)
  for (int i = -width; i <= width; i += 2) {
    int16_t fillLeft1X = baseX + cos(radPerp) * i;
    int16_t fillLeft1Y = baseY + sin(radPerp) * i;
    int16_t fillLeft2X = tipX - cos(rad) * 15 + cos(radPerp) * (i / 2);
    int16_t fillLeft2Y = tipY - sin(rad) * 15 + sin(radPerp) * (i / 2);

    gfx->drawLine(fillLeft1X, fillLeft1Y, fillLeft2X, fillLeft2Y, color565);
  }

  // Bordo chiaro superiore (effetto riflessione)
  uint16_t highlightColor = convertColor(Color(255, 255, 255));
  gfx->drawLine(left1X - 1, left1Y - 1, left2X - 1, left2Y - 1, highlightColor);
  gfx->drawLine(left2X - 1, left2Y - 1, tipX - 1, tipY - 1, highlightColor);
}

// ===================================================================
// DISEGNA SOTTO-QUADRANTE GMT (separato dal quadrante principale)
// ===================================================================
void drawGmtSubdial() {
  ClockSkin& skin = clockSkins[currentClockSkin];
  Arduino_GFX* tgt = (clockTargetGfx != nullptr) ? clockTargetGfx : gfx;

  // Ottieni configurazione GMT
  int16_t gmtCenterX = skin.gmtHand.centerX;
  int16_t gmtCenterY = skin.gmtHand.centerY;
  int16_t gmtRadius = 60;  // Raggio del sotto-quadrante GMT

  // Disegna punti cardinali solo nella metà inferiore (intorno alla freccia vento)
  if (windDirectionAvailable) {
    tgt->setTextSize(1);
    tgt->setTextColor(convertColor(Color(0, 255, 0)));  // Verde brillante

    // Calcola il centro della metà inferiore (dove c'è la freccia)
    int16_t windCenterY = gmtCenterY + 25;
    int16_t windRadius = 23;

    // N (Nord - in alto della metà inferiore)
    tgt->setCursor(gmtCenterX - 3, windCenterY - windRadius + 5);
    tgt->print("N");

    // E (Est - destra)
    tgt->setCursor(gmtCenterX + windRadius - 4, windCenterY + 4);
    tgt->print("E");

    // S (Sud - in basso)
    tgt->setCursor(gmtCenterX - 3, windCenterY + windRadius + 10);
    tgt->print("S");

    // W (Ovest - sinistra)
    tgt->setCursor(gmtCenterX - windRadius - 6, windCenterY + 4);
    tgt->print("W");
  }

  // MODIFICA: Quadrante diviso in due parti
  // Parte superiore: icona meteo
  // Parte inferiore: freccia direzione vento

  if (weatherIconAvailable || windDirectionAvailable) {
    // Linea divisoria orizzontale al centro
    tgt->drawLine(gmtCenterX - gmtRadius + 5, gmtCenterY,
                  gmtCenterX + gmtRadius - 5, gmtCenterY,
                  convertColor(Color(150, 150, 150)));

    // PARTE SUPERIORE: Icona meteo (senza freccia vento)
    if (weatherIconAvailable) {
      int16_t iconCenterY = gmtCenterY - 25;
      drawWeatherIconOnly(gmtCenterX, iconCenterY, weatherIconCode);
    }

    // PARTE INFERIORE: Freccia direzione vento
    if (windDirectionAvailable) {
      int16_t windCenterY = gmtCenterY + 25;
      drawWindArrow(gmtCenterX, windCenterY, windDirection);
    }
  } else {
    // Altrimenti disegna la lancetta GMT normale
    float rad = radians(gmtHand.angle - 90);
    int16_t tipX = gmtCenterX + cos(rad) * skin.gmtHand.length;
    int16_t tipY = gmtCenterY + sin(rad) * skin.gmtHand.length;

    uint16_t color565 = convertColor(skin.gmtHand.color);

    // Disegna lancetta semplice (linea spessa)
    for (int i = -skin.gmtHand.thickness / 2; i <= skin.gmtHand.thickness / 2; i++) {
      float perpRad = rad + PI / 2;
      int16_t offsetX = cos(perpRad) * i;
      int16_t offsetY = sin(perpRad) * i;
      tgt->drawLine(gmtCenterX + offsetX, gmtCenterY + offsetY, tipX + offsetX, tipY + offsetY, color565);
    }

    // Disegna centro del sotto-quadrante
    tgt->fillCircle(gmtCenterX, gmtCenterY, 4, color565);
    tgt->drawCircle(gmtCenterX, gmtCenterY, 5, convertColor(Color(255, 255, 255)));
  }
}

// ===================================================================
// DISEGNA SOTTO-QUADRANTE DATA (sinistra-basso)
// ===================================================================
void drawDateSubdial() {
  ClockSkin& skin = clockSkins[currentClockSkin];
  Arduino_GFX* tgt = (clockTargetGfx != nullptr) ? clockTargetGfx : gfx;

  int16_t centerX = skin.subdials.dateCenterX;
  int16_t centerY = skin.subdials.dateCenterY;
  int16_t radius = 60;

  // Skin 0 (Daytona): NESSUN CERCHIO, testo NERO
  // Skin 1 (GMT): CON CERCHIO, testo BIANCO/GRIGIO
  bool isDaytona = (currentClockSkin == 0);

  // Ottieni giorno settimana e data
  const char* giorniAbbr[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};
  const char* mesiAbbr[] = {"", "GEN", "FEB", "MAR", "APR", "MAG", "GIU",
                            "LUG", "AGO", "SET", "OTT", "NOV", "DIC"};

  uint8_t dayOfWeek = myTZ.weekday();  // 1=Domenica, 2=Lunedì, ..., 7=Sabato
  uint8_t day = myTZ.day();
  uint8_t month = myTZ.month();

  // Converti dayOfWeek da 1-7 a indice array 0-6
  const char* nomeGiorno = giorniAbbr[(dayOfWeek == 0) ? 0 : (dayOfWeek - 1)];
  const char* nomeMese = mesiAbbr[month];

  // Colori dipendenti dalla skin
  Color textColorSmall = isDaytona ? Color(0, 0, 0) : Color(200, 200, 200);
  Color textColorBig = isDaytona ? Color(0, 0, 0) : Color(255, 255, 255);

  // Disegna giorno della settimana (sopra)
  tgt->setFont(u8g2_font_6x10_tr);
  tgt->setTextColor(convertColor(textColorSmall));
  int16_t textWidth = strlen(nomeGiorno) * 6;
  tgt->setCursor(centerX - textWidth / 2, centerY - 15);
  tgt->print(nomeGiorno);

  // Disegna giorno del mese (centro - grande)
  tgt->setFont(u8g2_font_fub20_tn);  // Font grande per il numero
  char dayStr[3];
  snprintf(dayStr, sizeof(dayStr), "%d", day);
  textWidth = strlen(dayStr) * 14;
  tgt->setTextColor(convertColor(textColorBig));
  tgt->setCursor(centerX - textWidth / 2, centerY + 10);
  tgt->print(dayStr);

  // Disegna mese (sotto)
  tgt->setFont(u8g2_font_6x10_tr);
  tgt->setTextColor(convertColor(textColorSmall));
  textWidth = strlen(nomeMese) * 6;
  tgt->setCursor(centerX - textWidth / 2, centerY + 30);
  tgt->print(nomeMese);
}

// ===================================================================
// DISEGNA SOTTO-QUADRANTE ORARIO DIGITALE (destra-basso)
// ===================================================================
void drawDigitalTimeSubdial() {
  ClockSkin& skin = clockSkins[currentClockSkin];
  Arduino_GFX* tgt = (clockTargetGfx != nullptr) ? clockTargetGfx : gfx;

  int16_t centerX = skin.subdials.timeCenterX;
  int16_t centerY = skin.subdials.timeCenterY;
  int16_t radius = 60;  // Raggio del sotto-quadrante

  // Disegna label "TIME" sopra
  tgt->setFont(u8g2_font_6x10_tr);
  tgt->setTextColor(convertColor(Color(255, 255, 255)));  // BIANCO per visibilità
  int16_t textWidth = 4 * 6;  // "TIME" = 4 caratteri
  tgt->setCursor(centerX - textWidth / 2, centerY - 15);
  tgt->print("TIME");

  // Disegna orario digitale HH:MM (centro - grande)
  tgt->setFont(u8g2_font_fub14_tn);  // Font medio-grande per i numeri
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", currentHour, currentMinute);
  textWidth = strlen(timeStr) * 10;
  tgt->setTextColor(convertColor(Color(255, 255, 255)));  // BIANCO per visibilità
  tgt->setCursor(centerX - textWidth / 2, centerY + 8);
  tgt->print(timeStr);

  // Disegna secondi sotto (piccoli)
  tgt->setFont(u8g2_font_6x10_tr);
  char secStr[4];
  snprintf(secStr, sizeof(secStr), "%02ds", currentSecond);
  textWidth = strlen(secStr) * 6;
  tgt->setTextColor(convertColor(Color(255, 255, 255)));  // BIANCO per visibilità
  tgt->setCursor(centerX - textWidth / 2, centerY + 28);
  tgt->print(secStr);
}

// ===================================================================
// DISEGNA SOTTO-QUADRANTE TEMPERATURA (centro-basso)
// ===================================================================
void drawTemperatureSubdial() {
  ClockSkin& skin = clockSkins[currentClockSkin];
  Arduino_GFX* tgt = (clockTargetGfx != nullptr) ? clockTargetGfx : gfx;

  int16_t centerX = skin.subdials.tempCenterX;
  int16_t centerY = skin.subdials.tempCenterY;
  int16_t radius = 60;  // Raggio del sotto-quadrante

  // Disegna linea divisoria orizzontale al centro
  tgt->drawLine(centerX - 40, centerY, centerX + 40, centerY, convertColor(Color(150, 150, 150)));

  // ========== PARTE SUPERIORE: Temperatura e Umidità INTERNA (BME280 o Radar) ==========
  tgt->setFont(u8g2_font_6x10_tr);
  tgt->setTextColor(convertColor(Color(200, 200, 200)));

  // Label "INT" (interna)
  int16_t textWidth = 3 * 6;
  tgt->setCursor(centerX - textWidth / 2, centerY - 35);
  tgt->print("INT");

  // Temperatura interna (da BME280 o Radar)
  tgt->setFont(u8g2_font_fub11_tn);
  char tempStr[12];
  if (indoorSensorSource > 0) {
    snprintf(tempStr, sizeof(tempStr), "%.1f", temperatureIndoor);
  } else {
    snprintf(tempStr, sizeof(tempStr), "N.P.");
  }
  textWidth = strlen(tempStr) * 8;
  tgt->setTextColor(convertColor(Color(255, 200, 100)));  // Arancione chiaro
  tgt->setCursor(centerX - textWidth / 2, centerY - 18);
  tgt->print(tempStr);

  // Simbolo °C (spostato 5 pixel a destra)
  if (indoorSensorSource > 0) {
    tgt->setFont(u8g2_font_6x10_tr);
    tgt->setTextColor(convertColor(Color(200, 200, 200)));
    tgt->setCursor(centerX + textWidth / 2 + 7, centerY - 22);
    tgt->print("C");
    tgt->drawCircle(centerX + textWidth / 2 + 3, centerY - 25, 2, convertColor(Color(200, 200, 200)));

    // Umidità sotto la temperatura
    char humStr[8];
    snprintf(humStr, sizeof(humStr), "%.0f%%", humidityIndoor);
    int16_t humWidth = strlen(humStr) * 6;
    tgt->setFont(u8g2_font_6x10_tr);
    tgt->setTextColor(convertColor(Color(100, 200, 255)));  // Azzurro per umidità
    tgt->setCursor(centerX - humWidth / 2, centerY - 5);
    tgt->print(humStr);
  }

  // ========== PARTE INFERIORE: Temperatura e Umidità ESTERNA (OpenWeatherMap) ==========
  tgt->setFont(u8g2_font_6x10_tr);
  tgt->setTextColor(convertColor(Color(200, 200, 200)));

  // Label "EST" (esterna)
  textWidth = 3 * 6;
  tgt->setCursor(centerX - textWidth / 2, centerY + 12);
  tgt->print("EST");

  // Temperatura esterna
  tgt->setFont(u8g2_font_fub11_tn);
  if (outdoorTempAvailable) {
    snprintf(tempStr, sizeof(tempStr), "%.1f", temperatureOutdoor);
  } else {
    snprintf(tempStr, sizeof(tempStr), "N.P.");
  }
  textWidth = strlen(tempStr) * 8;
  tgt->setTextColor(convertColor(Color(100, 180, 255)));  // Azzurro
  tgt->setCursor(centerX - textWidth / 2, centerY + 28);
  tgt->print(tempStr);

  // Simbolo °C (spostato 5 pixel a destra)
  if (outdoorTempAvailable) {
    tgt->setFont(u8g2_font_6x10_tr);
    tgt->setTextColor(convertColor(Color(200, 200, 200)));
    tgt->setCursor(centerX + textWidth / 2 + 7, centerY + 24);
    tgt->print("C");
    tgt->drawCircle(centerX + textWidth / 2 + 3, centerY + 21, 2, convertColor(Color(200, 200, 200)));

    // Umidità esterna sotto la temperatura
    char humStr[8];
    snprintf(humStr, sizeof(humStr), "%.0f%%", humidityOutdoor);
    int16_t humWidth = strlen(humStr) * 6;
    tgt->setFont(u8g2_font_6x10_tr);
    tgt->setTextColor(convertColor(Color(100, 200, 255)));  // Azzurro per umidità
    tgt->setCursor(centerX - humWidth / 2, centerY + 45);
    tgt->print(humStr);
  }
}

// NOTA: readBME280Temperature() è già definita nel file principale (oraQuadraNano_V1_4_I2C.ino)

// ===================================================================
// TEST CONNETTIVITÀ INTERNET (diagnostica)
// ===================================================================
bool testInternetConnectivity() {
  HTTPClient http;
  http.setTimeout(5000);  // Timeout 5 secondi per test rapido

  // Prova a connettersi a un server HTTP semplice e affidabile
  Serial.println("Test connettività: tentativo connessione a example.com...");

  if (!http.begin("http://example.com")) {
    Serial.println("  FALLITO: impossibile inizializzare connessione");
    return false;
  }

  int httpCode = http.GET();
  http.end();

  if (httpCode > 0) {
    Serial.printf("  OK: connessione riuscita (HTTP %d)\n", httpCode);
    return true;
  } else {
    Serial.printf("  FALLITO: errore HTTP %d\n", httpCode);
    return false;
  }
}

// ===================================================================
// OTTIENI TEMPERATURA ESTERNA DA OPENWEATHERMAP
// ===================================================================
void fetchOutdoorTemperature() {
  // Questa funzione ora usa i dati già ottenuti da Open-Meteo (19_WEATHER.ino)
  // Non fa più chiamate HTTP dirette - i dati vengono aggiornati da updateWeatherData()

  // Dichiarazioni extern per dati meteo da 19_WEATHER.ino
  extern float weatherTempOutdoor;
  extern float weatherHumOutdoor;
  extern bool weatherDataValid;
  extern String weatherIcon;
  extern float weatherWindSpeed;
  extern int weatherWindDeg;

  // Copia i dati da 19_WEATHER.ino alle variabili locali usate da 8_CLOCK
  if (weatherDataValid) {
    temperatureOutdoor = weatherTempOutdoor;
    humidityOutdoor = weatherHumOutdoor;
    weatherIconCode = weatherIcon;
    windDirection = (float)weatherWindDeg;
    windSpeed = weatherWindSpeed;

    outdoorTempAvailable = true;
    weatherIconAvailable = (weatherIcon.length() > 0);
    windDirectionAvailable = true;

    // Log solo se ci sono nuovi dati (evita spam)
    static float lastLoggedTemp = -999.0;
    if (temperatureOutdoor != lastLoggedTemp) {
      Serial.println("[CLOCK] Dati meteo Open-Meteo sincronizzati:");
      Serial.printf("  Temperatura: %.1f°C, Umidità: %.0f%%\n", temperatureOutdoor, humidityOutdoor);
      Serial.printf("  Vento: %.1f m/s da %d°, Icona: %s\n", windSpeed, (int)windDirection, weatherIconCode.c_str());
      lastLoggedTemp = temperatureOutdoor;
    }
  } else {
    // Dati meteo non ancora disponibili
    outdoorTempAvailable = false;
    weatherIconAvailable = false;
    windDirectionAvailable = false;
  }
}

// ===================================================================
// DISEGNA SOLO ICONA METEO (senza freccia vento)
// ===================================================================
void drawWeatherIconOnly(int16_t centerX, int16_t centerY, String iconCode) {
  if (!weatherIconAvailable || iconCode.length() == 0) {
    return;  // Nessuna icona disponibile
  }

  // Usa clockTargetGfx se disponibile (per double buffering)
  Arduino_GFX* tgt = (clockTargetGfx != nullptr) ? clockTargetGfx : gfx;

  // Dimensione icona ridotta per la metà superiore
  const int16_t iconSize = 25;

  // Colori
  uint16_t sunColor = convertColor(Color(255, 200, 0));        // Giallo sole
  uint16_t cloudColor = convertColor(Color(200, 200, 200));    // Grigio nuvola
  uint16_t darkCloudColor = convertColor(Color(120, 120, 120)); // Grigio scuro
  uint16_t rainColor = convertColor(Color(100, 150, 255));     // Blu pioggia
  uint16_t snowColor = convertColor(Color(255, 255, 255));     // Bianco neve
  uint16_t thunderColor = convertColor(Color(255, 255, 0));    // Giallo fulmine

  // Estrai codice base (primi 2 caratteri, es. "01" da "01d")
  String baseCode = iconCode.substring(0, 2);

  // Disegna icona in base al codice (versione compatta)
  if (baseCode == "01") {
    // Clear sky - SOLE
    tgt->fillCircle(centerX, centerY, 12, sunColor);
    // Raggi sole
    for (int i = 0; i < 8; i++) {
      float angle = i * 45.0f;
      float rad = radians(angle);
      int16_t x1 = centerX + cos(rad) * 15;
      int16_t y1 = centerY + sin(rad) * 15;
      int16_t x2 = centerX + cos(rad) * 22;
      int16_t y2 = centerY + sin(rad) * 22;
      tgt->drawLine(x1, y1, x2, y2, sunColor);
    }
  }
  else if (baseCode == "02") {
    // Few clouds - SOLE + PICCOLA NUVOLA
    tgt->fillCircle(centerX - 8, centerY - 8, 10, sunColor);
    tgt->fillCircle(centerX + 6, centerY + 6, 10, cloudColor);
    tgt->fillCircle(centerX + 14, centerY + 6, 8, cloudColor);
    tgt->fillCircle(centerX + 10, centerY + 1, 8, cloudColor);
  }
  else if (baseCode == "03") {
    // Scattered clouds - NUVOLA SINGOLA
    tgt->fillCircle(centerX - 8, centerY, 12, cloudColor);
    tgt->fillCircle(centerX + 4, centerY, 14, cloudColor);
    tgt->fillCircle(centerX + 8, centerY - 4, 10, cloudColor);
    tgt->fillCircle(centerX, centerY - 6, 10, cloudColor);
  }
  else if (baseCode == "04") {
    // Broken clouds - NUVOLE MULTIPLE SCURE
    tgt->fillCircle(centerX - 8, centerY - 4, 10, darkCloudColor);
    tgt->fillCircle(centerX + 4, centerY - 4, 12, darkCloudColor);
    tgt->fillCircle(centerX + 8, centerY + 4, 10, cloudColor);
    tgt->fillCircle(centerX - 4, centerY + 4, 8, cloudColor);
  }
  else if (baseCode == "09" || baseCode == "10") {
    // Rain - NUVOLA + GOCCE PIOGGIA
    tgt->fillCircle(centerX - 6, centerY - 6, 10, darkCloudColor);
    tgt->fillCircle(centerX + 6, centerY - 6, 12, darkCloudColor);
    tgt->fillCircle(centerX, centerY - 10, 8, darkCloudColor);
    // Gocce pioggia
    for (int i = 0; i < 4; i++) {
      int16_t x = centerX - 12 + i * 8;
      int16_t y = centerY + 4 + (i % 2) * 4;
      tgt->drawLine(x, y, x, y + 6, rainColor);
    }
  }
  else if (baseCode == "11") {
    // Thunderstorm - NUVOLA SCURA + FULMINE
    tgt->fillCircle(centerX - 6, centerY - 8, 12, darkCloudColor);
    tgt->fillCircle(centerX + 6, centerY - 8, 10, darkCloudColor);
    // Fulmine zigzag
    tgt->drawLine(centerX, centerY, centerX - 4, centerY + 6, thunderColor);
    tgt->drawLine(centerX - 4, centerY + 6, centerX + 2, centerY + 6, thunderColor);
    tgt->drawLine(centerX + 2, centerY + 6, centerX - 2, centerY + 14, thunderColor);
  }
  else if (baseCode == "13") {
    // Snow - NUVOLA + FIOCCHI NEVE
    tgt->fillCircle(centerX - 6, centerY - 6, 10, cloudColor);
    tgt->fillCircle(centerX + 6, centerY - 6, 12, cloudColor);
    // Fiocchi neve (asterischi)
    for (int i = 0; i < 3; i++) {
      int16_t x = centerX - 10 + i * 10;
      int16_t y = centerY + 6 + (i % 2) * 5;
      tgt->drawLine(x - 2, y, x + 2, y, snowColor);
      tgt->drawLine(x, y - 2, x, y + 2, snowColor);
      tgt->drawLine(x - 2, y - 2, x + 2, y + 2, snowColor);
      tgt->drawLine(x - 2, y + 2, x + 2, y - 2, snowColor);
    }
  }
  else if (baseCode == "50") {
    // Mist/Fog - LINEE ORIZZONTALI
    for (int i = 0; i < 4; i++) {
      int16_t y = centerY - 10 + i * 5;
      int16_t len = 20 - abs(i - 2) * 3;
      tgt->drawLine(centerX - len / 2, y, centerX + len / 2, y, cloudColor);
    }
  }
  else {
    // Icona sconosciuta - PUNTO INTERROGATIVO
    tgt->setTextSize(2);
    tgt->setTextColor(convertColor(Color(255, 255, 255)));
    tgt->setCursor(centerX - 6, centerY - 8);
    tgt->print("?");
  }
}

// ===================================================================
// DISEGNA FRECCIA DIREZIONE VENTO CON COORDINATE POLARI
// ===================================================================
void drawWindArrow(int16_t centerX, int16_t centerY, float direction) {
  if (!windDirectionAvailable) {
    return;  // Nessun dato disponibile
  }

  // Usa clockTargetGfx se disponibile (per double buffering)
  Arduino_GFX* tgt = (clockTargetGfx != nullptr) ? clockTargetGfx : gfx;

  // Colore freccia vento (rosso brillante per visibilità)
  uint16_t arrowColor = convertColor(Color(255, 80, 80));
  uint16_t textColor = convertColor(Color(255, 255, 255));

  // Lunghezza freccia ridotta per la metà inferiore
  int16_t arrowLength = 20;  // Accorciata da 25 a 20 pixel
  int16_t arrowHeadSize = 6;

  // Converti direzione vento da gradi meteorologici a radianti
  // In meteorologia: 0° = Nord, 90° = Est, 180° = Sud, 270° = Ovest
  // Sottrai 90° per convertire in coordinate grafiche (0° = Est)
  float windRad = radians(direction - 90);

  // Calcola punto finale freccia
  int16_t arrowTipX = centerX + cos(windRad) * arrowLength;
  int16_t arrowTipY = centerY + sin(windRad) * arrowLength;

  // Disegna linea principale freccia (più spessa)
  for (int i = -1; i <= 1; i++) {
    float perpRad = windRad + PI / 2;
    int16_t offsetX = cos(perpRad) * i;
    int16_t offsetY = sin(perpRad) * i;
    tgt->drawLine(centerX + offsetX, centerY + offsetY,
                  arrowTipX + offsetX, arrowTipY + offsetY, arrowColor);
  }

  // Disegna punta freccia (triangolo)
  float headAngle1 = windRad + radians(150);  // 150° indietro a sinistra
  float headAngle2 = windRad - radians(150);  // 150° indietro a destra

  int16_t head1X = arrowTipX + cos(headAngle1) * arrowHeadSize;
  int16_t head1Y = arrowTipY + sin(headAngle1) * arrowHeadSize;
  int16_t head2X = arrowTipX + cos(headAngle2) * arrowHeadSize;
  int16_t head2Y = arrowTipY + sin(headAngle2) * arrowHeadSize;

  // Disegna i lati della punta
  tgt->drawLine(arrowTipX, arrowTipY, head1X, head1Y, arrowColor);
  tgt->drawLine(arrowTipX, arrowTipY, head2X, head2Y, arrowColor);
  tgt->drawLine(head1X, head1Y, head2X, head2Y, arrowColor);

  // Riempimento punta freccia
  for (int i = 0; i < arrowHeadSize; i++) {
    float ratio = i / (float)arrowHeadSize;
    int16_t fillX1 = arrowTipX + (head1X - arrowTipX) * ratio;
    int16_t fillY1 = arrowTipY + (head1Y - arrowTipY) * ratio;
    int16_t fillX2 = arrowTipX + (head2X - arrowTipX) * ratio;
    int16_t fillY2 = arrowTipY + (head2Y - arrowTipY) * ratio;
    tgt->drawLine(fillX1, fillY1, fillX2, fillY2, arrowColor);
  }

  // Disegna coordinate polari (angolo in gradi)
  tgt->setFont(u8g2_font_6x10_tr);
  tgt->setTextColor(textColor);
  char angleStr[8];
  snprintf(angleStr, sizeof(angleStr), "%.0f", direction);
  int16_t textWidth = strlen(angleStr) * 6;
  tgt->setCursor(centerX - textWidth / 2, centerY + 12);
  tgt->print(angleStr);

  // Disegna simbolo di grado
  tgt->drawCircle(centerX + textWidth / 2 + 2, centerY + 5, 2, textColor);

  // Disegna velocità del vento (km/h) sotto l'angolo
  char speedStr[12];
  snprintf(speedStr, sizeof(speedStr), "%.1f km/h", windSpeed * 3.6);  // Converti m/s in km/h
  int16_t speedWidth = strlen(speedStr) * 6;
  tgt->setCursor(centerX - speedWidth / 2, centerY + 24);
  tgt->print(speedStr);
}

// ===================================================================
// DISEGNA ICONA METEO PERSONALIZZATA (solo per skin GMT Master II)
// ===================================================================
void drawWeatherIcon(int16_t centerX, int16_t centerY, String iconCode) {
  if (!weatherIconAvailable || iconCode.length() == 0) {
    return;  // Nessuna icona disponibile
  }

  // Dimensione icona (raggio area disegno)
  const int16_t iconSize = 35;

  // Colori
  uint16_t sunColor = convertColor(Color(255, 200, 0));        // Giallo sole
  uint16_t cloudColor = convertColor(Color(200, 200, 200));    // Grigio nuvola
  uint16_t darkCloudColor = convertColor(Color(120, 120, 120)); // Grigio scuro
  uint16_t rainColor = convertColor(Color(100, 150, 255));     // Blu pioggia
  uint16_t snowColor = convertColor(Color(255, 255, 255));     // Bianco neve
  uint16_t thunderColor = convertColor(Color(255, 255, 0));    // Giallo fulmine

  // Estrai codice base (primi 2 caratteri, es. "01" da "01d")
  String baseCode = iconCode.substring(0, 2);

  // Disegna icona in base al codice
  if (baseCode == "01") {
    // Clear sky - SOLE
    gfx->fillCircle(centerX, centerY, 18, sunColor);
    // Raggi sole
    for (int i = 0; i < 8; i++) {
      float angle = i * 45.0f;
      float rad = radians(angle);
      int16_t x1 = centerX + cos(rad) * 22;
      int16_t y1 = centerY + sin(rad) * 22;
      int16_t x2 = centerX + cos(rad) * 32;
      int16_t y2 = centerY + sin(rad) * 32;
      gfx->drawLine(x1, y1, x2, y2, sunColor);
      gfx->drawLine(x1, y1 + 1, x2, y2 + 1, sunColor);
    }
  }
  else if (baseCode == "02") {
    // Few clouds - SOLE + PICCOLA NUVOLA
    // Sole spostato a sinistra-alto
    gfx->fillCircle(centerX - 10, centerY - 10, 12, sunColor);
    // Nuvola piccola destra-basso
    gfx->fillCircle(centerX + 8, centerY + 8, 12, cloudColor);
    gfx->fillCircle(centerX + 18, centerY + 8, 10, cloudColor);
    gfx->fillCircle(centerX + 13, centerY + 2, 10, cloudColor);
  }
  else if (baseCode == "03") {
    // Scattered clouds - NUVOLA SINGOLA
    gfx->fillCircle(centerX - 10, centerY, 14, cloudColor);
    gfx->fillCircle(centerX + 5, centerY, 16, cloudColor);
    gfx->fillCircle(centerX + 10, centerY - 5, 12, cloudColor);
    gfx->fillCircle(centerX, centerY - 8, 12, cloudColor);
  }
  else if (baseCode == "04") {
    // Broken clouds - NUVOLE MULTIPLE SCURE
    gfx->fillCircle(centerX - 10, centerY - 5, 12, darkCloudColor);
    gfx->fillCircle(centerX + 5, centerY - 5, 14, darkCloudColor);
    gfx->fillCircle(centerX + 10, centerY + 5, 12, cloudColor);
    gfx->fillCircle(centerX - 5, centerY + 5, 10, cloudColor);
  }
  else if (baseCode == "09" || baseCode == "10") {
    // Rain - NUVOLA + GOCCE PIOGGIA
    // Nuvola
    gfx->fillCircle(centerX - 8, centerY - 8, 12, darkCloudColor);
    gfx->fillCircle(centerX + 8, centerY - 8, 14, darkCloudColor);
    gfx->fillCircle(centerX, centerY - 12, 10, darkCloudColor);
    // Gocce pioggia
    for (int i = 0; i < 5; i++) {
      int16_t x = centerX - 15 + i * 8;
      int16_t y = centerY + 5 + (i % 2) * 5;
      gfx->drawLine(x, y, x, y + 8, rainColor);
      gfx->drawLine(x + 1, y, x + 1, y + 8, rainColor);
    }
  }
  else if (baseCode == "11") {
    // Thunderstorm - NUVOLA SCURA + FULMINE
    // Nuvola scura
    gfx->fillCircle(centerX - 8, centerY - 10, 14, darkCloudColor);
    gfx->fillCircle(centerX + 8, centerY - 10, 12, darkCloudColor);
    // Fulmine zigzag
    gfx->drawLine(centerX, centerY, centerX - 5, centerY + 8, thunderColor);
    gfx->drawLine(centerX - 5, centerY + 8, centerX + 3, centerY + 8, thunderColor);
    gfx->drawLine(centerX + 3, centerY + 8, centerX - 2, centerY + 18, thunderColor);
    gfx->drawLine(centerX + 1, centerY, centerX - 4, centerY + 8, thunderColor);
  }
  else if (baseCode == "13") {
    // Snow - NUVOLA + FIOCCHI NEVE
    // Nuvola
    gfx->fillCircle(centerX - 8, centerY - 8, 12, cloudColor);
    gfx->fillCircle(centerX + 8, centerY - 8, 14, cloudColor);
    // Fiocchi neve (asterischi)
    for (int i = 0; i < 3; i++) {
      int16_t x = centerX - 12 + i * 12;
      int16_t y = centerY + 8 + (i % 2) * 6;
      // Croce
      gfx->drawLine(x - 3, y, x + 3, y, snowColor);
      gfx->drawLine(x, y - 3, x, y + 3, snowColor);
      gfx->drawLine(x - 2, y - 2, x + 2, y + 2, snowColor);
      gfx->drawLine(x - 2, y + 2, x + 2, y - 2, snowColor);
    }
  }
  else if (baseCode == "50") {
    // Mist/Fog - LINEE ORIZZONTALI
    for (int i = 0; i < 5; i++) {
      int16_t y = centerY - 12 + i * 6;
      int16_t len = 25 - abs(i - 2) * 3;
      gfx->drawLine(centerX - len / 2, y, centerX + len / 2, y, cloudColor);
      gfx->drawLine(centerX - len / 2, y + 1, centerX + len / 2, y + 1, cloudColor);
    }
  }
  else {
    // Icona sconosciuta - PUNTO INTERROGATIVO
    gfx->setTextSize(3);
    gfx->setTextColor(convertColor(Color(255, 255, 255)));
    gfx->setCursor(centerX - 10, centerY - 12);
    gfx->print("?");
  }

  // ===== FRECCIA DIREZIONE VENTO =====
  // Disegna freccia che indica la direzione del vento se disponibile
  if (windDirectionAvailable) {
    // Colore freccia vento (rosso brillante per visibilità)
    uint16_t arrowColor = convertColor(Color(255, 80, 80));

    // Lunghezza freccia
    int16_t arrowLength = 45;
    int16_t arrowHeadSize = 8;

    // Converti direzione vento da gradi meteorologici a radianti
    // In meteorologia: 0° = Nord, 90° = Est, 180° = Sud, 270° = Ovest
    // Sottrai 90° per convertire in coordinate grafiche (0° = Est)
    float windRad = radians(windDirection - 90);

    // Calcola punto finale freccia
    int16_t arrowTipX = centerX + cos(windRad) * arrowLength;
    int16_t arrowTipY = centerY + sin(windRad) * arrowLength;

    // Disegna linea principale freccia (più spessa)
    for (int i = -1; i <= 1; i++) {
      float perpRad = windRad + PI / 2;
      int16_t offsetX = cos(perpRad) * i;
      int16_t offsetY = sin(perpRad) * i;
      gfx->drawLine(centerX + offsetX, centerY + offsetY, arrowTipX + offsetX, arrowTipY + offsetY, arrowColor);
    }

    // Disegna punta freccia (triangolo)
    // Calcola i due punti della base del triangolo
    float headAngle1 = windRad + radians(150);  // 150° indietro a sinistra
    float headAngle2 = windRad - radians(150);  // 150° indietro a destra

    int16_t head1X = arrowTipX + cos(headAngle1) * arrowHeadSize;
    int16_t head1Y = arrowTipY + sin(headAngle1) * arrowHeadSize;
    int16_t head2X = arrowTipX + cos(headAngle2) * arrowHeadSize;
    int16_t head2Y = arrowTipY + sin(headAngle2) * arrowHeadSize;

    // Disegna i lati della punta
    gfx->drawLine(arrowTipX, arrowTipY, head1X, head1Y, arrowColor);
    gfx->drawLine(arrowTipX, arrowTipY, head2X, head2Y, arrowColor);
    gfx->drawLine(head1X, head1Y, head2X, head2Y, arrowColor);

    // Riempimento punta freccia (linee parallele)
    for (int i = 0; i < arrowHeadSize; i++) {
      float ratio = i / (float)arrowHeadSize;
      int16_t fillX1 = arrowTipX + (head1X - arrowTipX) * ratio;
      int16_t fillY1 = arrowTipY + (head1Y - arrowTipY) * ratio;
      int16_t fillX2 = arrowTipX + (head2X - arrowTipX) * ratio;
      int16_t fillY2 = arrowTipY + (head2Y - arrowTipY) * ratio;
      gfx->drawLine(fillX1, fillY1, fillX2, fillY2, arrowColor);
    }
  }
}

// ===================================================================
// DISEGNA ORA DIGITALE E DATA (FUNZIONE ORIGINALE - DEPRECATA)
// ===================================================================
void drawDigitalTime() {
  // ========== DATA IN ALTO - STILE ROLEX DAYTONA ==========

  // Array nomi giorni abbreviati stile Rolex (elegante e minimalista)
  const char* giorniAbbr[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};

  // Array nomi mesi abbreviati stile Rolex
  const char* mesiAbbr[] = {"", "GEN", "FEB", "MAR", "APR", "MAG", "GIU",
                            "LUG", "AGO", "SET", "OTT", "NOV", "DIC"};

  // Ottieni giorno settimana, giorno, mese
  uint8_t dayOfWeek = myTZ.weekday();  // ezTime weekday(): 1=Domenica, 2=Lunedì, ..., 7=Sabato

  // ezTime usa convenzione 1-7, ma l'array giorniAbbr usa indici 0-6
  // Converti: 1=Dom->0, 2=Lun->1, ..., 7=Sab->6
  if (dayOfWeek >= 1 && dayOfWeek <= 7) {
    dayOfWeek = dayOfWeek - 1;
  }

  uint8_t day = myTZ.day();            // 1-31
  uint8_t month = myTZ.month();        // 1-12

  // Formato data elegante Rolex: "VEN 30 OTT" (più compatto e lussuoso)
  char dateStr[32];
  sprintf(dateStr, "%s %d %s", giorniAbbr[dayOfWeek], day, mesiAbbr[month]);

  // Font elegante per la data
  gfx->setFont(u8g2_font_inb16_mr);

  // Calcola posizione centrata (data più corta = più centrata)
  int16_t textX = 180;  // Centrato per testo corto

  // ========== BOX ELEGANTE PER DATA (stile Rolex) ==========
  // Sfondo semi-trasparente scuro per leggibilità
  int16_t boxWidth = 120;
  int16_t boxHeight = 22;
  int16_t boxX = 180;
  int16_t boxY = 8;

  // Bordo elegante oro/acciaio
  gfx->drawRect(boxX, boxY, boxWidth, boxHeight, convertColor(Color(180, 180, 185)));
  gfx->drawRect(boxX+1, boxY+1, boxWidth-2, boxHeight-2, convertColor(Color(200, 200, 205)));

  // Testo data in oro elegante Rolex
  gfx->setCursor(textX + 5, 25);
  gfx->setTextColor(convertColor(Color(255, 215, 0)));  // Oro Rolex
  gfx->print(dateStr);

  // ========== ORARIO DIGITALE IN BASSO - STILE ROLEX DAYTONA ==========
  char timeStr[16];
  sprintf(timeStr, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);

  // Font grande e bold per l'orario (stile Rolex)
  gfx->setFont(u8g2_font_inb24_mr);

  // ========== BOX ELEGANTE PER ORARIO (stile Rolex) ==========
  int16_t timeBoxWidth = 140;
  int16_t timeBoxHeight = 30;
  int16_t timeBoxX = 170;
  int16_t timeBoxY = 445;

  // Bordo elegante oro/acciaio
  gfx->drawRect(timeBoxX, timeBoxY, timeBoxWidth, timeBoxHeight, convertColor(Color(180, 180, 185)));
  gfx->drawRect(timeBoxX+1, timeBoxY+1, timeBoxWidth-2, timeBoxHeight-2, convertColor(Color(200, 200, 205)));

  // Orario in bianco platino brillante
  gfx->setCursor(175, 470);
  gfx->setTextColor(convertColor(Color(240, 240, 245)));  // Bianco platino
  gfx->print(timeStr);
}

#endif // EFFECT_CLOCK
