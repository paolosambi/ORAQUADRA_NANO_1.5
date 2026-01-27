// ================== FLUX CAPACITOR - Animazione stile Ritorno al Futuro ==================
// Flusso canalizzatore animato con immagine di sfondo e luci che scorrono lungo i 3 bracci
// Utilizza double buffering per animazione fluida senza flickering
// L'animazione replica l'effetto del film: luci che partono dal centro e viaggiano
// verso le estremità dei 3 bracci formando una Y

#ifdef EFFECT_FLUX_CAPACITOR

// ================== CONFIGURAZIONE DISPLAY ==================
#define FLUX_SCREEN_WIDTH  480
#define FLUX_SCREEN_HEIGHT 480

// ================== GEOMETRIA DEL FLUX CAPACITOR ==================
// Coordinate calibrate sull'immagine flux.jpg 480x480
// Il centro Y è il punto dove convergono i 3 bracci metallici
// NOTA: L'immagine ha il pannello grigio con la Y dorata al centro

#define FLUX_CENTER_X 240       // Centro orizzontale esatto (metà schermo)
#define FLUX_CENTER_Y 252       // Punto di convergenza dei bracci (misurato)

// Lunghezze dei bracci (pixel dal centro alle estremità/manopole rosse)
// Misurate dall'immagine originale
#define FLUX_ARM_TOP_LENGTH    130   // Bracci superiori (fino alle manopole rosse)
#define FLUX_ARM_BOTTOM_LENGTH 125   // Braccio inferiore (fino alla manopola)

// Angoli dei 3 bracci (gradi, 0=destra, senso antiorario)
// La Y del flux capacitor ha i bracci superiori a circa 55° dalla verticale
#define FLUX_ARM_TOP_LEFT_ANGLE   136.0f   // Braccio superiore sinistro
#define FLUX_ARM_TOP_RIGHT_ANGLE   44.0f   // Braccio superiore destro
#define FLUX_ARM_BOTTOM_ANGLE     270.0f   // Braccio inferiore (dritto verso basso)

// ================== PARAMETRI ANIMAZIONE (configurabili da web) ==================
#define FLUX_MAX_PARTICLES 100        // Max particelle per braccio (per array)
uint8_t fluxAnimationSpeed = 10;      // ms tra frame (più basso = più veloce)
uint16_t fluxNumParticles = 8;        // Particelle principali per braccio
float fluxParticleSpeed = 0.15f;      // Velocità movimento (0.05-1.0 per ciclo)

// Parametri scia luminosa
#define FLUX_TRAIL_LENGTH       8     // Numero di segmenti nella scia
#define FLUX_TRAIL_SPACING      0.04f // Distanza tra segmenti scia

// Colore configurabile (RGB separati)
uint8_t fluxColorR = 255;             // Componente Rosso (default bianco)
uint8_t fluxColorG = 255;             // Componente Verde
uint8_t fluxColorB = 255;             // Componente Blu

// Modalità effetto: 0 = Impulsi (onde), 1 = Particelle
uint8_t fluxEffectMode = 0;

// Parametri impulsi
uint8_t fluxImpulseWidth = 20;        // Larghezza impulso (% del braccio)
uint8_t fluxImpulseThickness = 8;     // Spessore impulso (pixel)
uint8_t fluxImpulseFade = 20;         // Sfumatura colore (0=nessuna, 50=massima)

// Colori RGB565 calcolati dal colore configurato
uint16_t fluxColorBright = 0xF800;    // Colore brillante
uint16_t fluxColorMedium = 0xC000;    // Colore medio
uint16_t fluxColorDark = 0x8000;      // Colore scuro

// Funzione per aggiornare i colori RGB565 dal colore RGB configurato
void updateFluxColors() {
  // Colore brillante (100%)
  fluxColorBright = ((fluxColorR >> 3) << 11) | ((fluxColorG >> 2) << 5) | (fluxColorB >> 3);
  // Colore medio (60%)
  fluxColorMedium = (((fluxColorR * 6 / 10) >> 3) << 11) | (((fluxColorG * 6 / 10) >> 2) << 5) | ((fluxColorB * 6 / 10) >> 3);
  // Colore scuro (30%)
  fluxColorDark = (((fluxColorR * 3 / 10) >> 3) << 11) | (((fluxColorG * 3 / 10) >> 2) << 5) | ((fluxColorB * 3 / 10) >> 3);

  Serial.printf("[FLUX] Colori aggiornati: Bright=0x%04X Medium=0x%04X Dark=0x%04X\n",
                fluxColorBright, fluxColorMedium, fluxColorDark);
}

// Salva impostazioni Flux in EEPROM
void saveFluxSettings() {
  EEPROM.write(EEPROM_FLUX_COLOR_R_ADDR, fluxColorR);
  EEPROM.write(EEPROM_FLUX_COLOR_G_ADDR, fluxColorG);
  EEPROM.write(EEPROM_FLUX_COLOR_B_ADDR, fluxColorB);
  EEPROM.write(EEPROM_FLUX_PARTICLES_ADDR, fluxNumParticles & 0xFF);         // Low byte
  EEPROM.write(EEPROM_FLUX_PARTICLES_ADDR + 1, (fluxNumParticles >> 8) & 0xFF); // High byte
  EEPROM.write(EEPROM_FLUX_ANIM_SPEED_ADDR, fluxAnimationSpeed);
  EEPROM.write(EEPROM_FLUX_PART_SPEED_ADDR, (uint8_t)(fluxParticleSpeed * 100));
  EEPROM.write(EEPROM_FLUX_VALID_ADDR, EEPROM_FLUX_VALID_MARKER);
  EEPROM.commit();
  Serial.println("[FLUX] Impostazioni salvate in EEPROM");
}

// Carica impostazioni Flux da EEPROM
void loadFluxSettings() {
  if (EEPROM.read(EEPROM_FLUX_VALID_ADDR) == EEPROM_FLUX_VALID_MARKER) {
    fluxColorR = EEPROM.read(EEPROM_FLUX_COLOR_R_ADDR);
    fluxColorG = EEPROM.read(EEPROM_FLUX_COLOR_G_ADDR);
    fluxColorB = EEPROM.read(EEPROM_FLUX_COLOR_B_ADDR);
    uint16_t particles = EEPROM.read(EEPROM_FLUX_PARTICLES_ADDR) | (EEPROM.read(EEPROM_FLUX_PARTICLES_ADDR + 1) << 8);
    fluxNumParticles = constrain(particles, 3, FLUX_MAX_PARTICLES);
    fluxAnimationSpeed = constrain(EEPROM.read(EEPROM_FLUX_ANIM_SPEED_ADDR), 5, 50);
    uint8_t pspeedRaw = EEPROM.read(EEPROM_FLUX_PART_SPEED_ADDR);
    fluxParticleSpeed = constrain(pspeedRaw, 5, 100) / 100.0f;
    Serial.printf("[FLUX] Impostazioni caricate: R=%d G=%d B=%d Particles=%d Speed=%d PSpeed=%.2f\n",
                  fluxColorR, fluxColorG, fluxColorB, fluxNumParticles, fluxAnimationSpeed, fluxParticleSpeed);
  } else {
    Serial.println("[FLUX] Nessuna impostazione salvata, uso default");
  }
  // Aggiorna i colori RGB565
  updateFluxColors();
}

// Dimensioni luci
#define FLUX_LIGHT_RADIUS_MAX   12    // Raggio massimo testa particella
#define FLUX_LIGHT_RADIUS_MIN   3     // Raggio minimo coda scia
#define FLUX_GLOW_EXTRA_RADIUS  8     // Raggio extra per alone

// ================== COLORI PREDEFINITI (RGB565) ==================
#define FLUX_COLOR_WHITE        0xFFFF  // Bianco puro
#define FLUX_COLOR_YELLOW       0xFFE0  // Giallo brillante
#define FLUX_COLOR_ORANGE       0xFD00  // Arancione
#define FLUX_COLOR_AMBER        0xFC00  // Ambra
#define FLUX_COLOR_RED          0xF800  // Rosso (coda)
#define FLUX_COLOR_GLOW_YELLOW  0xCE40  // Giallo tenue per alone
#define FLUX_COLOR_GLOW_ORANGE  0x8B00  // Arancione tenue per alone

// ================== STRUTTURA PARTICELLA ==================
struct FluxParticle {
  float position;     // Posizione 0.0 (centro) - 1.0 (estremità)
  float speed;        // Velocità individuale (leggera variazione)
  uint8_t brightness; // Luminosità 0-255
  bool active;        // Particella attiva
};

// ================== VARIABILI GLOBALI ==================
uint16_t *fluxBackgroundBuffer = nullptr;

// Array di particelle per ogni braccio
static FluxParticle fluxParticles[3][FLUX_MAX_PARTICLES];
static unsigned long fluxLastUpdate = 0;
static float fluxGlobalPhase = 0.0f;  // Fase globale per effetti sincronizzati

// ================== FUNZIONI ==================

// Callback JPEG per caricare sfondo nel buffer
int fluxJpegDrawCallback(JPEGDRAW *pDraw) {
  if (fluxBackgroundBuffer != nullptr) {
    for (int y = 0; y < pDraw->iHeight; y++) {
      for (int x = 0; x < pDraw->iWidth; x++) {
        int bufferIndex = (pDraw->y + y) * FLUX_SCREEN_WIDTH + (pDraw->x + x);
        if (bufferIndex < FLUX_SCREEN_WIDTH * FLUX_SCREEN_HEIGHT) {
          fluxBackgroundBuffer[bufferIndex] = pDraw->pPixels[y * pDraw->iWidth + x];
        }
      }
    }
  }
  return 1;
}

// Carica immagine di sfondo flux.jpg dalla SD
bool loadFluxBackgroundToBuffer() {
  String filepath = "/Flux Capacitor.jpg";

  Serial.printf("[FLUX] Caricamento sfondo: '%s'\n", filepath.c_str());

  // Alloca buffer sfondo se non esiste
  if (fluxBackgroundBuffer == nullptr) {
    fluxBackgroundBuffer = (uint16_t*)heap_caps_malloc(
      FLUX_SCREEN_WIDTH * FLUX_SCREEN_HEIGHT * sizeof(uint16_t),
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    if (fluxBackgroundBuffer == nullptr) {
      fluxBackgroundBuffer = (uint16_t*)ps_malloc(FLUX_SCREEN_WIDTH * FLUX_SCREEN_HEIGHT * sizeof(uint16_t));
      if (fluxBackgroundBuffer == nullptr) {
        Serial.println("[FLUX] ERRORE: Memoria insufficiente per sfondo!");
        return false;
      }
    }
  }

  // Se file non esiste, riempi con grigio scuro
  if (!SD.exists(filepath.c_str())) {
    Serial.println("[FLUX] File flux.jpg non trovato, uso sfondo grigio");
    for (int i = 0; i < FLUX_SCREEN_WIDTH * FLUX_SCREEN_HEIGHT; i++) {
      fluxBackgroundBuffer[i] = 0x2104;  // Grigio scuro
    }
    return false;
  }

  // Carica JPEG
  File jpegFile = SD.open(filepath.c_str(), FILE_READ);
  if (!jpegFile) {
    Serial.println("[FLUX] Errore apertura file");
    return false;
  }

  size_t fileSize = jpegFile.size();
  uint8_t *jpegBuffer = (uint8_t *)malloc(fileSize);
  if (!jpegBuffer) {
    jpegFile.close();
    return false;
  }

  jpegFile.read(jpegBuffer, fileSize);
  jpegFile.close();

  int result = jpeg.openRAM(jpegBuffer, fileSize, fluxJpegDrawCallback);
  if (result == 1) {
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
    jpeg.decode(0, 0, 0);
    jpeg.close();
    Serial.println("[FLUX] Sfondo caricato con successo!");
  }

  free(jpegBuffer);
  return (result == 1);
}

// Inizializza le particelle con posizioni e velocità casuali
void initFluxParticles() {
  // Aggiorna i colori RGB565
  updateFluxColors();

  for (int arm = 0; arm < 3; arm++) {
    for (uint16_t i = 0; i < fluxNumParticles; i++) {
      // Distribuisci le particelle lungo il braccio
      fluxParticles[arm][i].position = (float)i / fluxNumParticles;
      fluxParticles[arm][i].speed = fluxParticleSpeed * (0.8f + (random(40) / 100.0f));
      fluxParticles[arm][i].brightness = 200 + random(55);
      fluxParticles[arm][i].active = true;
    }
    // Disattiva particelle oltre il limite
    for (uint16_t i = fluxNumParticles; i < FLUX_MAX_PARTICLES; i++) {
      fluxParticles[arm][i].active = false;
    }
  }
}

// Inizializza il modo Flux Capacitor
void initFluxCapacitor() {
  if (fluxCapacitorInitialized) return;

  Serial.println("=== INIT FLUX CAPACITOR MODE ===");

  // Carica impostazioni salvate da EEPROM
  loadFluxSettings();

  // Usa il frameBuffer condiviso
  if (frameBuffer == nullptr) {
    frameBuffer = (uint16_t*)heap_caps_malloc(
      FLUX_SCREEN_WIDTH * FLUX_SCREEN_HEIGHT * sizeof(uint16_t),
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    if (frameBuffer == nullptr) {
      frameBuffer = (uint16_t*)ps_malloc(FLUX_SCREEN_WIDTH * FLUX_SCREEN_HEIGHT * sizeof(uint16_t));
      if (frameBuffer == nullptr) {
        Serial.println("[FLUX] ERRORE: Impossibile allocare frameBuffer!");
        return;
      }
    }
    Serial.println("[FLUX] FrameBuffer allocato");
  }

  // Crea GFX offscreen se non esiste
  if (offscreenGfx == nullptr) {
    offscreenGfx = new OffscreenGFX(frameBuffer, FLUX_SCREEN_WIDTH, FLUX_SCREEN_HEIGHT);
    Serial.println("[FLUX] OffscreenGFX creato");
  }

  // Carica sfondo
  loadFluxBackgroundToBuffer();

  // Inizializza particelle
  initFluxParticles();

  // Reset
  fluxLastUpdate = millis();
  fluxGlobalPhase = 0.0f;

  fluxCapacitorInitialized = true;
  Serial.println("[FLUX] Inizializzazione completata");
}

// Calcola posizione su un braccio
// armIndex: 0=top-left, 1=top-right, 2=bottom
// distance: 0.0 (centro) - 1.0 (estremità)
void getFluxArmPosition(int armIndex, float distance, int16_t &outX, int16_t &outY) {
  float angle, length;

  switch (armIndex) {
    case 0: // Top-left
      angle = FLUX_ARM_TOP_LEFT_ANGLE;
      length = FLUX_ARM_TOP_LENGTH;
      break;
    case 1: // Top-right
      angle = FLUX_ARM_TOP_RIGHT_ANGLE;
      length = FLUX_ARM_TOP_LENGTH;
      break;
    case 2: // Bottom
    default:
      angle = FLUX_ARM_BOTTOM_ANGLE;
      length = FLUX_ARM_BOTTOM_LENGTH;
      break;
  }

  float radians = angle * PI / 180.0f;
  outX = FLUX_CENTER_X + (int16_t)(cos(radians) * distance * length);
  outY = FLUX_CENTER_Y - (int16_t)(sin(radians) * distance * length);
}

// Interpola colore in base alla posizione (0=centro giallo, 1=estremità rosso)
uint16_t getFluxParticleColor(float position, float brightness) {
  // Gradiente: bianco/giallo al centro -> arancione -> rosso alle estremità
  if (position < 0.3f) {
    return (brightness > 200) ? FLUX_COLOR_WHITE : FLUX_COLOR_YELLOW;
  } else if (position < 0.6f) {
    return FLUX_COLOR_ORANGE;
  } else if (position < 0.85f) {
    return FLUX_COLOR_AMBER;
  } else {
    return FLUX_COLOR_RED;
  }
}

// Disegna una particella con scia luminosa (scia verso l'esterno, testa verso il centro)
void drawFluxParticleWithTrail(int armIndex, float headPosition, uint8_t brightness) {
  // Disegna la scia (dalla coda alla testa)
  for (int t = FLUX_TRAIL_LENGTH - 1; t >= 0; t--) {
    // Scia verso l'esterno (+ invece di -)
    float segmentPos = headPosition + (t * FLUX_TRAIL_SPACING);

    if (segmentPos < 0.0f || segmentPos > 1.0f) continue;

    int16_t x, y;
    getFluxArmPosition(armIndex, segmentPos, x, y);

    // Fattore di dissolvenza
    float fadeFactor = 1.0f - ((float)t / FLUX_TRAIL_LENGTH);
    fadeFactor = fadeFactor * fadeFactor;

    // Colore sfumato
    uint16_t segmentColor;
    if (fadeFactor > 0.6f) {
      segmentColor = fluxColorBright;
    } else if (fadeFactor > 0.3f) {
      segmentColor = fluxColorMedium;
    } else {
      segmentColor = fluxColorDark;
    }

    // Cluster di punti per effetto fiume
    int numPoints = 2 + (int)(fadeFactor * 10);
    int spread = 3 + (int)(fadeFactor * 5);

    for (int p = 0; p < numPoints; p++) {
      int16_t px = x + random(-spread, spread + 1);
      int16_t py = y + random(-spread, spread + 1);
      offscreenGfx->drawPixel(px, py, segmentColor);
    }
  }
}

// Variabili per gli impulsi
static float impulsePositions[3] = {1.0f, 1.0f, 1.0f};  // Posizione di ogni impulso (1=esterno, 0=centro)

// Disegna impulsi solidi che vanno dall'esterno al centro
void drawFluxImpulses(float phase) {
  // Larghezza dell'impulso (segmento solido) - da parametro configurabile
  float impulseWidth = fluxImpulseWidth / 100.0f;
  // Spessore linea - da parametro configurabile
  int lineThickness = fluxImpulseThickness;

  for (int arm = 0; arm < 3; arm++) {
    // Muovi l'impulso verso il centro
    impulsePositions[arm] -= fluxParticleSpeed;

    // Quando arriva al centro, riparte dall'esterno
    if (impulsePositions[arm] <= 0.0f) {
      impulsePositions[arm] = 1.0f;
    }

    // Calcola angolo e lunghezza per questo braccio
    float angle, length;
    switch (arm) {
      case 0: angle = FLUX_ARM_TOP_LEFT_ANGLE; length = FLUX_ARM_TOP_LENGTH; break;
      case 1: angle = FLUX_ARM_TOP_RIGHT_ANGLE; length = FLUX_ARM_TOP_LENGTH; break;
      default: angle = FLUX_ARM_BOTTOM_ANGLE; length = FLUX_ARM_BOTTOM_LENGTH; break;
    }
    float radians = angle * PI / 180.0f;
    float cosA = cos(radians);
    float sinA = sin(radians);

    // Posizione corrente dell'impulso
    float headPos = impulsePositions[arm];
    float tailPos = headPos + impulseWidth;
    if (tailPos > 1.0f) tailPos = 1.0f;

    // Disegna il segmento solido dall'esterno (tailPos) alla testa (headPos)
    // Usa piccoli step per riempire la linea
    for (float pos = headPos; pos <= tailPos; pos += 0.005f) {
      if (pos < 0.0f) continue;

      int16_t x = FLUX_CENTER_X + (int16_t)(cosA * pos * length);
      int16_t y = FLUX_CENTER_Y - (int16_t)(sinA * pos * length);

      // Intensità: più brillante alla testa, sfuma verso la coda
      float intensity = 1.0f - (pos - headPos) / impulseWidth;

      // Calcola colore con sfumatura configurabile
      // fade=0: tutto bright, fade=50: sfumatura massima
      float fadeAmount = fluxImpulseFade / 100.0f;  // 0.0 - 0.5
      float colorBlend = intensity * (1.0f - fadeAmount) + fadeAmount;  // Limita la sfumatura

      // Interpola colore RGB
      uint8_t r = fluxColorR * colorBlend;
      uint8_t g = fluxColorG * colorBlend;
      uint8_t b = fluxColorB * colorBlend;

      // Converti a RGB565
      uint16_t color = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

      // Disegna linea spessa (cerchio piccolo per ogni punto)
      for (int dx = -lineThickness/2; dx <= lineThickness/2; dx++) {
        for (int dy = -lineThickness/2; dy <= lineThickness/2; dy++) {
          if (dx*dx + dy*dy <= (lineThickness/2)*(lineThickness/2)) {
            offscreenGfx->drawPixel(x + dx, y + dy, color);
          }
        }
      }
    }
  }
}

// Disegna il centro pulsante del Flux Capacitor
void drawFluxCenter(float phase) {
  float pulse = 0.5f + 0.5f * sin(phase * PI * 2);
  int baseSpread = 10;
  int pulseSpread = baseSpread + (int)(pulse * 8);
  int numCenterPoints = 30 + (int)(pulse * 20);

  for (int p = 0; p < numCenterPoints; p++) {
    int16_t px = FLUX_CENTER_X + random(-pulseSpread, pulseSpread + 1);
    int16_t py = FLUX_CENTER_Y + random(-pulseSpread, pulseSpread + 1);
    int dist = abs(px - FLUX_CENTER_X) + abs(py - FLUX_CENTER_Y);
    uint16_t color;
    if (dist < pulseSpread / 3) {
      color = fluxColorBright;
    } else if (dist < pulseSpread * 2 / 3) {
      color = fluxColorMedium;
    } else {
      color = fluxColorDark;
    }
    offscreenGfx->drawPixel(px, py, color);
  }
}

// Aggiorna posizione di tutte le particelle (dall'esterno verso il centro)
void updateFluxParticles() {
  for (int arm = 0; arm < 3; arm++) {
    for (uint16_t i = 0; i < fluxNumParticles; i++) {
      FluxParticle &p = fluxParticles[arm][i];

      // Muovi verso il centro (position decresce)
      p.position -= p.speed;

      // Quando arriva al centro, ricomincia dall'esterno
      if (p.position <= 0.0f) {
        p.position = 1.0f;
        p.speed = fluxParticleSpeed * (0.8f + (random(40) / 100.0f));
        p.brightness = 200 + random(55);
      }

      // Flicker leggero
      p.brightness = constrain(p.brightness + random(-5, 6), 180, 255);
    }
  }
}

// Disegna tutte le particelle con scie
void drawFluxAllParticles() {
  for (int arm = 0; arm < 3; arm++) {
    for (uint16_t i = 0; i < fluxNumParticles; i++) {
      FluxParticle &p = fluxParticles[arm][i];
      if (p.active && p.position > 0.05f) {
        drawFluxParticleWithTrail(arm, p.position, p.brightness);
      }
    }
  }
}

// Funzione principale di aggiornamento
void updateFluxCapacitor() {
  if (!fluxCapacitorInitialized) {
    initFluxCapacitor();
  }

  unsigned long currentTime = millis();

  // Aggiorna solo se è passato abbastanza tempo
  if (currentTime - fluxLastUpdate >= fluxAnimationSpeed) {
    fluxLastUpdate = currentTime;

    // Aggiorna fase globale
    fluxGlobalPhase += 0.03f;
    if (fluxGlobalPhase > 1.0f) fluxGlobalPhase -= 1.0f;

    // ===== RENDERING OFFSCREEN =====

    // Copia sfondo nel frameBuffer
    if (fluxBackgroundBuffer != nullptr) {
      memcpy(frameBuffer, fluxBackgroundBuffer, FLUX_SCREEN_WIDTH * FLUX_SCREEN_HEIGHT * sizeof(uint16_t));
    } else {
      memset(frameBuffer, 0, FLUX_SCREEN_WIDTH * FLUX_SCREEN_HEIGHT * sizeof(uint16_t));
    }

    // Disegna centro pulsante
    drawFluxCenter(fluxGlobalPhase);

    // Disegna effetto in base alla modalità
    if (fluxEffectMode == 0) {
      // Modalità Impulsi (onde continue)
      drawFluxImpulses(fluxGlobalPhase);
    } else {
      // Modalità Particelle
      updateFluxParticles();
      drawFluxAllParticles();
    }

    // ===== TRASFERISCI AL DISPLAY =====
    gfx->draw16bitRGBBitmap(0, 0, frameBuffer, FLUX_SCREEN_WIDTH, FLUX_SCREEN_HEIGHT);
  }
}

// Forza ridisegno completo
void forceFluxCapacitorRedraw() {
  initFluxParticles();
  fluxGlobalPhase = 0.0f;
  fluxLastUpdate = 0;
}

// ================== WEBSERVER FLUX CAPACITOR ==================

// Pagina HTML per configurazione Flux Capacitor
const char FLUX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Flux Capacitor Config</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Segoe UI', Arial, sans-serif;
      background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
      color: #eee; min-height: 100vh; padding: 20px;
    }
    .container { max-width: 500px; margin: 0 auto; }
    h1 {
      text-align: center; color: #ff4444; margin-bottom: 30px;
      text-shadow: 0 0 20px #ff4444;
    }
    .card {
      background: rgba(255,255,255,0.05); border-radius: 15px;
      padding: 20px; margin-bottom: 20px;
      border: 1px solid rgba(255,68,68,0.3);
    }
    .card h2 { color: #ff6666; margin-bottom: 15px; font-size: 1.2em; }
    .row { display: flex; align-items: center; margin-bottom: 15px; }
    .row label { flex: 1; }
    .row input[type="range"] { flex: 2; }
    .row input[type="color"] { width: 60px; height: 40px; border: none; cursor: pointer; }
    .row .value { width: 50px; text-align: right; color: #ff8888; font-weight: bold; }
    input[type="range"] {
      -webkit-appearance: none; background: #333; height: 8px;
      border-radius: 4px; outline: none;
    }
    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none; width: 20px; height: 20px;
      background: #ff4444; border-radius: 50%; cursor: pointer;
    }
    .btn {
      display: block; width: 100%; padding: 15px; margin-top: 10px;
      background: linear-gradient(135deg, #ff4444, #cc0000);
      border: none; border-radius: 10px; color: white;
      font-size: 1.1em; font-weight: bold; cursor: pointer;
      transition: transform 0.2s, box-shadow 0.2s;
    }
    .btn:hover { transform: scale(1.02); box-shadow: 0 5px 20px rgba(255,68,68,0.4); }
    .btn-secondary { background: linear-gradient(135deg, #444, #222); }
    .status { text-align: center; margin-top: 15px; color: #88ff88; }
    .back-link { display: block; text-align: center; margin-top: 20px; color: #888; }
  </style>
</head>
<body>
  <div class="container">
    <h1>⚡ FLUX CAPACITOR ⚡</h1>

    <div class="card">
      <h2>🌊 Modalità Effetto</h2>
      <div class="row" style="justify-content: center; gap: 10px;">
        <button class="btn" id="btnImpulsi" style="width: auto; padding: 15px 30px;" onclick="setMode(0)">⚡ IMPULSI</button>
        <button class="btn btn-secondary" id="btnParticelle" style="width: auto; padding: 15px 30px;" onclick="setMode(1)">✨ PARTICELLE</button>
      </div>
    </div>

    <div class="card" id="impulseSettings">
      <h2>⚡ Impostazioni Impulsi</h2>
      <div class="row">
        <label>Lunghezza:</label>
        <input type="range" id="impWidth" min="5" max="80" value="20">
        <span class="value" id="impWidthVal">20%</span>
      </div>
      <div class="row">
        <label>Spessore:</label>
        <input type="range" id="impThick" min="2" max="20" value="8">
        <span class="value" id="impThickVal">8px</span>
      </div>
      <div class="row">
        <label>Sfumatura:</label>
        <input type="range" id="impFade" min="0" max="50" value="20">
        <span class="value" id="impFadeVal">20</span>
      </div>
    </div>

    <div class="card">
      <h2>🎨 Colore</h2>
      <div class="row">
        <label>Colore:</label>
        <input type="color" id="color" value="#ff0000">
      </div>
      <div class="row" style="justify-content: center; gap: 10px; flex-wrap: wrap;">
        <button class="btn btn-secondary" style="width: auto; padding: 10px 20px;" onclick="setColor('#ff0000')">Rosso</button>
        <button class="btn btn-secondary" style="width: auto; padding: 10px 20px;" onclick="setColor('#00ff00')">Verde</button>
        <button class="btn btn-secondary" style="width: auto; padding: 10px 20px;" onclick="setColor('#0088ff')">Blu</button>
        <button class="btn btn-secondary" style="width: auto; padding: 10px 20px;" onclick="setColor('#ffaa00')">Arancio</button>
        <button class="btn btn-secondary" style="width: auto; padding: 10px 20px;" onclick="setColor('#ff00ff')">Magenta</button>
        <button class="btn btn-secondary" style="width: auto; padding: 10px 20px;" onclick="setColor('#ffffff')">Bianco</button>
      </div>
    </div>

    <div class="card">
      <h2>✨ Particelle</h2>
      <div class="row">
        <label>Quantità:</label>
        <input type="range" id="particles" min="3" max="100" value="8">
        <span class="value" id="particlesVal">8</span>
      </div>
    </div>

    <div class="card">
      <h2>🚀 Velocità</h2>
      <div class="row">
        <label>Frame delay (ms):</label>
        <input type="range" id="speed" min="5" max="50" value="20">
        <span class="value" id="speedVal">20</span>
      </div>
      <div class="row">
        <label>Velocità impulsi:</label>
        <input type="range" id="pspeed" min="5" max="100" value="10">
        <span class="value" id="pspeedVal">0.10</span>
      </div>
    </div>

    <div class="status" id="status"></div>

    <a href="/" class="back-link">&larr; Home</a>
  </div>

  <script>
    let currentMode = 0;

    // Auto-apply quando cambiano i valori
    function apply() {
      const color = document.getElementById('color').value;
      const particles = document.getElementById('particles').value;
      const speed = document.getElementById('speed').value;
      const pspeed = document.getElementById('pspeed').value;

      const r = parseInt(color.substr(1,2), 16);
      const g = parseInt(color.substr(3,2), 16);
      const b = parseInt(color.substr(5,2), 16);

      const impWidth = document.getElementById('impWidth').value;
      const impThick = document.getElementById('impThick').value;
      const impFade = document.getElementById('impFade').value;

      const url = `/fluxcap/save?r=${r}&g=${g}&b=${b}&particles=${particles}&speed=${speed}&pspeed=${pspeed}&mode=${currentMode}&impwidth=${impWidth}&impthick=${impThick}&impfade=${impFade}`;

      fetch(url)
        .then(r => r.text())
        .then(d => {
          document.getElementById('status').textContent = '✓';
          setTimeout(() => document.getElementById('status').textContent = '', 500);
        })
        .catch(e => {
          document.getElementById('status').textContent = '✗';
        });
    }

    // Aggiorna valori e applica subito
    document.getElementById('particles').oninput = function() {
      document.getElementById('particlesVal').textContent = this.value;
      apply();
    };
    document.getElementById('speed').oninput = function() {
      document.getElementById('speedVal').textContent = this.value;
      apply();
    };
    document.getElementById('pspeed').oninput = function() {
      document.getElementById('pspeedVal').textContent = (this.value / 100).toFixed(2);
      apply();
    };
    document.getElementById('color').oninput = function() {
      apply();
    };
    document.getElementById('impWidth').oninput = function() {
      document.getElementById('impWidthVal').textContent = this.value + '%';
      apply();
    };
    document.getElementById('impThick').oninput = function() {
      document.getElementById('impThickVal').textContent = this.value + 'px';
      apply();
    };
    document.getElementById('impFade').oninput = function() {
      document.getElementById('impFadeVal').textContent = this.value;
      apply();
    };

    function setColor(c) {
      document.getElementById('color').value = c;
      apply();
    }

    function setMode(m) {
      currentMode = m;
      // Aggiorna stile pulsanti
      if (m == 0) {
        document.getElementById('btnImpulsi').className = 'btn';
        document.getElementById('btnParticelle').className = 'btn btn-secondary';
      } else {
        document.getElementById('btnImpulsi').className = 'btn btn-secondary';
        document.getElementById('btnParticelle').className = 'btn';
      }
      apply();
    }

    function updateModeButtons(m) {
      currentMode = m;
      if (m == 0) {
        document.getElementById('btnImpulsi').className = 'btn';
        document.getElementById('btnParticelle').className = 'btn btn-secondary';
      } else {
        document.getElementById('btnImpulsi').className = 'btn btn-secondary';
        document.getElementById('btnParticelle').className = 'btn';
      }
    }

    // Carica configurazione attuale
    console.log('Loading config from /fluxcap/config');
    fetch('/fluxcap/config')
      .then(r => {
        console.log('Config response status: ' + r.status);
        if (!r.ok) throw new Error('HTTP ' + r.status);
        return r.json();
      })
      .then(d => {
        console.log('Config loaded:', d);
        const hex = '#' +
          ('0' + d.r.toString(16)).slice(-2) +
          ('0' + d.g.toString(16)).slice(-2) +
          ('0' + d.b.toString(16)).slice(-2);
        document.getElementById('color').value = hex;
        document.getElementById('particles').value = d.particles;
        document.getElementById('particlesVal').textContent = d.particles;
        document.getElementById('speed').value = d.speed;
        document.getElementById('speedVal').textContent = d.speed;
        document.getElementById('pspeed').value = d.pspeed;
        document.getElementById('pspeedVal').textContent = (d.pspeed / 100).toFixed(2);
        if (d.mode !== undefined) updateModeButtons(d.mode);
        if (d.impwidth !== undefined) {
          document.getElementById('impWidth').value = d.impwidth;
          document.getElementById('impWidthVal').textContent = d.impwidth + '%';
        }
        if (d.impthick !== undefined) {
          document.getElementById('impThick').value = d.impthick;
          document.getElementById('impThickVal').textContent = d.impthick + 'px';
        }
        if (d.impfade !== undefined) {
          document.getElementById('impFade').value = d.impfade;
          document.getElementById('impFadeVal').textContent = d.impfade;
        }
      })
      .catch(e => {
        console.error('Errore caricamento config:', e);
      });
  </script>
</body>
</html>
)rawliteral";

// Handler GET /flux - Pagina configurazione
void handleFluxPage(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", FLUX_HTML);
}

// Handler GET /flux/config - Restituisce configurazione attuale
void handleFluxConfig(AsyncWebServerRequest *request) {
  String json = "{";
  json += "\"r\":" + String(fluxColorR) + ",";
  json += "\"g\":" + String(fluxColorG) + ",";
  json += "\"b\":" + String(fluxColorB) + ",";
  json += "\"particles\":" + String(fluxNumParticles) + ",";
  json += "\"speed\":" + String(fluxAnimationSpeed) + ",";
  json += "\"pspeed\":" + String((int)(fluxParticleSpeed * 100)) + ",";
  json += "\"mode\":" + String(fluxEffectMode) + ",";
  json += "\"impwidth\":" + String(fluxImpulseWidth) + ",";
  json += "\"impthick\":" + String(fluxImpulseThickness) + ",";
  json += "\"impfade\":" + String(fluxImpulseFade);
  json += "}";
  request->send(200, "application/json", json);
}

// Handler GET /flux/save - Salva configurazione
void handleFluxSave(AsyncWebServerRequest *request) {
  Serial.println("[FLUX] Save request received");

  if (request->hasParam("r")) {
    fluxColorR = (uint8_t)constrain(request->getParam("r")->value().toInt(), 0, 255);
  }
  if (request->hasParam("g")) {
    fluxColorG = (uint8_t)constrain(request->getParam("g")->value().toInt(), 0, 255);
  }
  if (request->hasParam("b")) {
    fluxColorB = (uint8_t)constrain(request->getParam("b")->value().toInt(), 0, 255);
  }
  if (request->hasParam("particles")) {
    fluxNumParticles = constrain(request->getParam("particles")->value().toInt(), 3, 100);
  }
  if (request->hasParam("speed")) {
    fluxAnimationSpeed = constrain(request->getParam("speed")->value().toInt(), 5, 50);
  }
  if (request->hasParam("pspeed")) {
    int pspeedVal = constrain(request->getParam("pspeed")->value().toInt(), 5, 100);
    fluxParticleSpeed = pspeedVal / 100.0f;
  }
  if (request->hasParam("mode")) {
    fluxEffectMode = constrain(request->getParam("mode")->value().toInt(), 0, 1);
  }
  if (request->hasParam("impwidth")) {
    fluxImpulseWidth = constrain(request->getParam("impwidth")->value().toInt(), 5, 80);
  }
  if (request->hasParam("impthick")) {
    fluxImpulseThickness = constrain(request->getParam("impthick")->value().toInt(), 2, 20);
  }
  if (request->hasParam("impfade")) {
    fluxImpulseFade = constrain(request->getParam("impfade")->value().toInt(), 0, 50);
  }

  // Aggiorna colori RGB565
  updateFluxColors();

  // Reinizializza particelle con nuovi parametri
  initFluxParticles();

  // Salva in EEPROM
  saveFluxSettings();

  Serial.printf("[FLUX] Config SALVATA: R=%d G=%d B=%d, Particles=%d, Speed=%d, PSpeed=%.2f\n",
                fluxColorR, fluxColorG, fluxColorB, fluxNumParticles, fluxAnimationSpeed, fluxParticleSpeed);

  request->send(200, "text/plain", "OK");
}

// Registra endpoints webserver
void setup_flux_webserver(AsyncWebServer* server) {
  // IMPORTANTE: registrare gli endpoint specifici PRIMA di quello generico!
  server->on("/fluxcap/config", HTTP_GET, handleFluxConfig);
  server->on("/fluxcap/save", HTTP_GET, handleFluxSave);
  server->on("/fluxcap", HTTP_GET, handleFluxPage);
  Serial.println("[FLUX] Webserver endpoints registrati: /fluxcap/config, /fluxcap/save, /fluxcap");
}

#endif // EFFECT_FLUX_CAPACITOR

// ================== GUIDA CALIBRAZIONE ==================
//
// Se le luci non sono allineate ai bracci metallici nell'immagine,
// modifica questi valori:
//
// FLUX_CENTER_X, FLUX_CENTER_Y = punto centrale dove convergono i 3 bracci
// FLUX_ARM_TOP_LEFT_ANGLE = angolo braccio superiore sinistro (gradi, 0=destra, antiorario)
// FLUX_ARM_TOP_RIGHT_ANGLE = angolo braccio superiore destro
// FLUX_ARM_BOTTOM_ANGLE = angolo braccio inferiore (270 = verso il basso)
// FLUX_ARM_TOP_LENGTH = lunghezza in pixel dei bracci superiori
// FLUX_ARM_BOTTOM_LENGTH = lunghezza in pixel del braccio inferiore
//
// Per trovare le coordinate esatte:
// 1. Apri flux.jpg in un editor di immagini
// 2. Trova il pixel centrale (dove i 3 bracci si incontrano)
// 3. Misura la distanza dal centro alle estremità di ogni braccio
// 4. Calcola gli angoli usando arctan o misurando con un goniometro digitale
//
