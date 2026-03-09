// ================== CHRISTMAS - TEMA NATALIZIO ==================
/*
 * Effetto natalizio con:
 * - Albero di Natale stilizzato con rami sfumati
 * - Neve che cade e si accumula sul terreno
 * - Stella luminosa sulla cima
 * - Palline colorate e luci lampeggianti
 * - Orario visualizzato
 * - Regali sotto l'albero
 */

#ifdef EFFECT_CHRISTMAS

// ================== CONFIGURAZIONE ==================
#define XMAS_MAX_SNOWFLAKES  150     // Più fiocchi per effetto più denso
#define XMAS_MAX_LIGHTS      30      // Luci sull'albero
#define XMAS_MAX_ORNAMENTS   12      // Palline decorative

// Colori
#define XMAS_SKY_TOP         0x0008  // Blu notte scuro top
#define XMAS_SKY_BOTTOM      0x0010  // Blu notte più chiaro bottom
#define XMAS_SNOW_WHITE      0xFFFF  // Bianco puro
#define XMAS_TREE_DARK       0x03E0  // Verde scuro
#define XMAS_TREE_MID        0x07E0  // Verde medio
#define XMAS_TREE_LIGHT      0x07E8  // Verde chiaro
#define XMAS_TRUNK           0x4A28  // Marrone
#define XMAS_STAR_YELLOW     0xFFE0  // Giallo oro
#define XMAS_GROUND_SNOW     0xDEFB  // Bianco leggermente grigio

// Struttura fiocco di neve
struct XmasSnowflake {
  int16_t x;
  int16_t y;
  int8_t speedY;      // Velocità verticale (1-4)
  int8_t driftX;      // Deriva orizzontale (-1, 0, 1)
  uint8_t size;       // Dimensione (1-3)
};

// Struttura luce albero
struct XmasLight {
  int16_t x;
  int16_t y;
  uint16_t color;
  bool on;
};

// Struttura pallina
struct XmasOrnament {
  int16_t x;
  int16_t y;
  uint16_t color;
  uint8_t radius;
};

// Variabili globali
XmasSnowflake xmasSnow[XMAS_MAX_SNOWFLAKES];
XmasLight xmasLights[XMAS_MAX_LIGHTS];
XmasOrnament xmasOrnaments[XMAS_MAX_ORNAMENTS];
uint8_t xmasGroundSnow[480];  // Altezza neve accumulata per ogni X
bool christmasInitialized = false;
uint32_t xmasLastUpdate = 0;
uint32_t xmasLightTimer = 0;
uint8_t xmasStarPhase = 0;

// Colori per luci e palline
const uint16_t xmasLightColors[] = {0xF800, 0xFFE0, 0x001F, 0x07E0, 0xF81F, 0x07FF, 0xFBE0};
const uint16_t xmasOrnamentColors[] = {0xF800, 0x001F, 0xFFE0, 0xF81F, 0x07FF, 0xFD20};

// ================== INIZIALIZZAZIONE ==================
void initChristmas() {
  Serial.println("[XMAS] Inizializzazione tema natalizio...");

  // Reset flag scena statica - IMPORTANTE per ridisegnare quando si cambia mode
  xmasSceneDrawn = false;

  // Reset neve al suolo
  for (int i = 0; i < 480; i++) {
    xmasGroundSnow[i] = random(3, 8);  // Neve iniziale irregolare
  }

  // Inizializza fiocchi di neve - distribuiti su tutto lo schermo
  for (int i = 0; i < XMAS_MAX_SNOWFLAKES; i++) {
    xmasSnow[i].x = random(0, 480);
    xmasSnow[i].y = random(0, 480);  // Distribuiti ovunque all'inizio
    xmasSnow[i].speedY = random(2, 6);
    xmasSnow[i].driftX = random(-1, 2);
    xmasSnow[i].size = random(1, 4);
  }

  // Inizializza luci sull'albero
  // L'albero va da Y=100 a Y=380, centrato a X=240
  int lightIdx = 0;
  for (int row = 0; row < 6 && lightIdx < XMAS_MAX_LIGHTS; row++) {
    int y = 130 + row * 45;
    int halfWidth = 20 + row * 25;
    int numInRow = 3 + row;
    for (int l = 0; l < numInRow && lightIdx < XMAS_MAX_LIGHTS; l++) {
      xmasLights[lightIdx].x = 240 - halfWidth + (halfWidth * 2 * l / max(1, numInRow - 1));
      xmasLights[lightIdx].y = y + random(-10, 10);
      xmasLights[lightIdx].color = xmasLightColors[random(7)];
      xmasLights[lightIdx].on = random(2);
      lightIdx++;
    }
  }

  // Inizializza palline decorative
  int ornIdx = 0;
  for (int row = 0; row < 4 && ornIdx < XMAS_MAX_ORNAMENTS; row++) {
    int y = 160 + row * 55;
    int halfWidth = 30 + row * 28;
    int numInRow = 2 + row;
    for (int o = 0; o < numInRow && ornIdx < XMAS_MAX_ORNAMENTS; o++) {
      xmasOrnaments[ornIdx].x = 240 - halfWidth + 20 + random(halfWidth * 2 - 40);
      xmasOrnaments[ornIdx].y = y + random(-15, 15);
      xmasOrnaments[ornIdx].color = xmasOrnamentColors[random(6)];
      xmasOrnaments[ornIdx].radius = random(6, 10);
      ornIdx++;
    }
  }

  christmasInitialized = true;
  xmasLastUpdate = millis();
  xmasLightTimer = millis();

  // Disegno iniziale
  gfx->fillScreen(XMAS_SKY_TOP);

  Serial.println("[XMAS] Inizializzato!");
}

// Variabili per evitare flickering
int16_t xmasOldSnowX[XMAS_MAX_SNOWFLAKES];
int16_t xmasOldSnowY[XMAS_MAX_SNOWFLAKES];
bool xmasSceneDrawn = false;

// Calcola colore sfondo per una Y
uint16_t getXmasSkyColor(int y) {
  uint8_t blue = 8 + (y * 16 / 480);
  return blue;
}

// Controlla se un punto è dentro l'albero
bool isInsideTree(int x, int y) {
  int treeX = 240;
  int treeTop = 95;

  // Controlla per ogni livello dell'albero
  for (int layer = 0; layer < 5; layer++) {
    int layerTop = treeTop + layer * 55;
    int layerBottom = layerTop + 100;
    int bottomWidth = 60 + layer * 40;

    if (y >= layerTop && y <= layerBottom) {
      // Calcola la larghezza dell'albero a questa Y
      float progress = (float)(y - layerTop) / (layerBottom - layerTop);
      int widthAtY = (int)(progress * bottomWidth);
      if (x >= treeX - widthAtY && x <= treeX + widthAtY) {
        return true;
      }
    }
  }

  // Controlla tronco
  if (y >= 385 && y <= 440 && x >= 220 && x <= 260) {
    return true;
  }

  return false;
}

// Controlla se un punto è sopra un regalo
bool isInsideGift(int x, int y) {
  // Regalo 1
  if (x >= 160 && x <= 215 && y >= 410 && y <= 455) return true;
  // Regalo 2
  if (x >= 270 && x <= 315 && y >= 420 && y <= 455) return true;
  // Regalo 3
  if (x >= 225 && x <= 260 && y >= 430 && y <= 458) return true;
  return false;
}

// Controlla se un punto è sopra il box orario
bool isInsideTimeBox(int x, int y) {
  return (x >= 150 && x <= 330 && y >= 8 && y <= 72);
}

// Controlla se un punto è in zona neve al suolo
bool isInSnowZone(int x, int y) {
  return y >= (480 - xmasGroundSnow[x]);
}

// ================== DISEGNA SCENA STATICA (solo una volta) ==================
void drawXmasStaticScene() {
  // Sfondo gradiente
  for (int y = 0; y < 480; y += 2) {
    uint16_t skyColor = getXmasSkyColor(y);
    gfx->fillRect(0, y, 480, 2, skyColor);
  }

  // Albero
  int treeX = 240;
  int treeTop = 95;
  int treeBottom = 385;

  // Tronco
  gfx->fillRect(treeX - 20, treeBottom, 40, 55, XMAS_TRUNK);
  gfx->fillRect(treeX - 18, treeBottom, 36, 55, 0x5ACB);

  // Rami - 5 livelli
  for (int layer = 0; layer < 5; layer++) {
    int layerTop = treeTop + layer * 55;
    int layerBottom = layerTop + 100;
    int bottomWidth = 60 + layer * 40;

    gfx->fillTriangle(treeX, layerTop, treeX - bottomWidth, layerBottom, treeX + bottomWidth, layerBottom, XMAS_TREE_DARK);
    gfx->fillTriangle(treeX, layerTop + 5, treeX - bottomWidth + 15, layerBottom - 5, treeX + bottomWidth - 30, layerBottom - 5, XMAS_TREE_MID);

    for (int i = -bottomWidth; i < bottomWidth; i += 12) {
      int px = treeX + i;
      gfx->fillTriangle(px, layerBottom, px + 6, layerBottom + 8, px + 12, layerBottom, XMAS_TREE_DARK);
    }
  }

  // Palline (statiche)
  for (int i = 0; i < XMAS_MAX_ORNAMENTS; i++) {
    int ox = xmasOrnaments[i].x;
    int oy = xmasOrnaments[i].y;
    uint8_t r = xmasOrnaments[i].radius;
    gfx->fillCircle(ox, oy, r, xmasOrnaments[i].color);
    gfx->fillCircle(ox - r/3, oy - r/3, r/3, XMAS_SNOW_WHITE);
    gfx->drawCircle(ox, oy, r, 0x0000);
  }

  // Regali
  gfx->fillRect(160, 410, 55, 45, 0xF800);
  gfx->fillRect(183, 410, 8, 45, XMAS_STAR_YELLOW);
  gfx->fillRect(160, 428, 55, 8, XMAS_STAR_YELLOW);
  gfx->drawRect(160, 410, 55, 45, 0x8000);
  gfx->fillCircle(187, 410, 8, XMAS_STAR_YELLOW);

  gfx->fillRect(270, 420, 45, 35, 0x001F);
  gfx->fillRect(289, 420, 6, 35, XMAS_SNOW_WHITE);
  gfx->fillRect(270, 434, 45, 6, XMAS_SNOW_WHITE);
  gfx->drawRect(270, 420, 45, 35, 0x0010);
  gfx->fillCircle(292, 420, 6, XMAS_SNOW_WHITE);

  gfx->fillRect(225, 430, 35, 28, 0x07E0);
  gfx->fillRect(239, 430, 6, 28, 0xF800);
  gfx->fillRect(225, 441, 35, 6, 0xF800);
  gfx->drawRect(225, 430, 35, 28, 0x0400);
  gfx->fillCircle(242, 430, 5, 0xF800);

  xmasSceneDrawn = true;
}

// ================== AGGIORNAMENTO PRINCIPALE ==================
void updateChristmas() {
  if (!christmasInitialized) {
    initChristmas();
    // Inizializza posizioni vecchie
    for (int i = 0; i < XMAS_MAX_SNOWFLAKES; i++) {
      xmasOldSnowX[i] = xmasSnow[i].x;
      xmasOldSnowY[i] = xmasSnow[i].y;
    }
    return;
  }

  uint32_t now = millis();

  // Aggiorna ogni 40ms (~25 FPS) - più lento = meno flickering
  if (now - xmasLastUpdate < 40) return;
  xmasLastUpdate = now;

  // Disegna scena statica solo la prima volta
  if (!xmasSceneDrawn) {
    drawXmasStaticScene();
  }

  // === CANCELLA VECCHI FIOCCHI (solo se erano su sfondo cielo) ===
  for (int i = 0; i < XMAS_MAX_SNOWFLAKES; i++) {
    int ox = xmasOldSnowX[i];
    int oy = xmasOldSnowY[i];
    // Cancella solo se il fiocco era nel cielo (non sopra albero, regali, orario o neve)
    if (oy >= 0 && oy < 480 && !isInsideTree(ox, oy) && !isInsideGift(ox, oy) && !isInsideTimeBox(ox, oy) && !isInSnowZone(ox, oy)) {
      uint16_t bgColor = getXmasSkyColor(oy);
      if (xmasSnow[i].size == 1) {
        gfx->drawPixel(ox, oy, bgColor);
      } else if (xmasSnow[i].size == 2) {
        gfx->drawPixel(ox, oy, bgColor);
        gfx->drawPixel(ox+1, oy, bgColor);
        gfx->drawPixel(ox, oy+1, bgColor);
      } else {
        gfx->fillCircle(ox, oy, 2, bgColor);
      }
    }
  }

  // === AGGIORNA POSIZIONE FIOCCHI ===
  for (int i = 0; i < XMAS_MAX_SNOWFLAKES; i++) {
    // Salva posizione vecchia
    xmasOldSnowX[i] = xmasSnow[i].x;
    xmasOldSnowY[i] = xmasSnow[i].y;

    // Muovi
    xmasSnow[i].y += xmasSnow[i].speedY;
    if ((now / 150 + i) % 3 == 0) {
      xmasSnow[i].x += xmasSnow[i].driftX;
    }

    // Wrap X
    if (xmasSnow[i].x < 0) xmasSnow[i].x = 479;
    if (xmasSnow[i].x >= 480) xmasSnow[i].x = 0;

    // Tocca suolo?
    int groundLevel = 480 - xmasGroundSnow[xmasSnow[i].x];
    if (xmasSnow[i].y >= groundLevel) {
      if (xmasGroundSnow[xmasSnow[i].x] < 80) {
        xmasGroundSnow[xmasSnow[i].x]++;
        int xi = xmasSnow[i].x;
        if (xi > 0 && xmasGroundSnow[xi] > xmasGroundSnow[xi-1] + 3) xmasGroundSnow[xi-1]++;
        if (xi < 479 && xmasGroundSnow[xi] > xmasGroundSnow[xi+1] + 3) xmasGroundSnow[xi+1]++;
      }
      // Respawn
      xmasSnow[i].x = random(0, 480);
      xmasSnow[i].y = random(-30, -5);
      xmasSnow[i].speedY = random(3, 7);
      xmasSnow[i].driftX = random(-1, 2);
      xmasSnow[i].size = random(1, 4);
      xmasOldSnowX[i] = xmasSnow[i].x;
      xmasOldSnowY[i] = -50;
    }
  }

  // === DISEGNA NUOVI FIOCCHI (solo se visibili nel cielo) ===
  for (int i = 0; i < XMAS_MAX_SNOWFLAKES; i++) {
    int x = xmasSnow[i].x;
    int y = xmasSnow[i].y;
    // Disegna solo se nel cielo (non sopra albero, regali, orario)
    if (y >= 0 && y < 480 && !isInsideTree(x, y) && !isInsideGift(x, y) && !isInsideTimeBox(x, y)) {
      if (xmasSnow[i].size == 1) {
        gfx->drawPixel(x, y, XMAS_SNOW_WHITE);
      } else if (xmasSnow[i].size == 2) {
        gfx->drawPixel(x, y, XMAS_SNOW_WHITE);
        gfx->drawPixel(x+1, y, XMAS_SNOW_WHITE);
        gfx->drawPixel(x, y+1, XMAS_SNOW_WHITE);
      } else {
        gfx->fillCircle(x, y, 2, XMAS_SNOW_WHITE);
      }
    }
  }

  // === AGGIORNA NEVE AL SUOLO (solo top della neve) ===
  for (int x = 0; x < 480; x++) {
    int h = xmasGroundSnow[x];
    if (h > 0) {
      gfx->drawPixel(x, 480 - h, XMAS_SNOW_WHITE);
      if (h > 1) gfx->drawPixel(x, 480 - h + 1, XMAS_GROUND_SNOW);
    }
  }

  // === AGGIORNA LUCI (solo quando cambiano) ===
  if (now - xmasLightTimer > 300) {
    xmasLightTimer = now;
    for (int i = 0; i < XMAS_MAX_LIGHTS; i++) {
      if (random(100) < 25) {
        int lx = xmasLights[i].x;
        int ly = xmasLights[i].y;
        if (xmasLights[i].on) {
          // Spegni - ridisegna con verde albero
          gfx->fillCircle(lx, ly, 7, XMAS_TREE_DARK);
        }
        xmasLights[i].on = !xmasLights[i].on;
        if (xmasLights[i].on) {
          // Accendi
          gfx->fillCircle(lx, ly, 4, xmasLights[i].color);
          gfx->drawCircle(lx, ly, 6, xmasLights[i].color);
        }
      }
    }
    xmasStarPhase = (xmasStarPhase + 1) % 10;
  }

  // === STELLA (aggiorna solo quando cambia fase) ===
  static uint8_t lastStarPhase = 255;
  if (xmasStarPhase != lastStarPhase) {
    lastStarPhase = xmasStarPhase;
    int starX = 240;
    int starY = 80;

    // Cancella stella vecchia
    gfx->fillCircle(starX, starY, 30, getXmasSkyColor(starY));

    // Disegna stella nuova
    int starSize = 18 + (xmasStarPhase < 5 ? xmasStarPhase : 10 - xmasStarPhase) * 2;
    uint16_t starColor = (xmasStarPhase < 5) ? XMAS_STAR_YELLOW : 0xFFC0;

    for (int i = 0; i < 5; i++) {
      float angle = (i * 72 - 90) * PI / 180.0;
      int px = starX + cos(angle) * starSize;
      int py = starY + sin(angle) * starSize;
      gfx->drawLine(starX, starY, px, py, starColor);
      gfx->drawLine(starX+1, starY, px+1, py, starColor);
    }
    gfx->fillCircle(starX, starY, 8, XMAS_STAR_YELLOW);
    gfx->fillCircle(starX, starY, 5, XMAS_SNOW_WHITE);
  }

  // === ORARIO (aggiorna solo ogni secondo) ===
  static uint8_t lastSecond = 255;
  if (currentSecond != lastSecond) {
    lastSecond = currentSecond;

    // Box orario
    gfx->fillRoundRect(155, 12, 170, 55, 12, 0x0000);
    gfx->drawRoundRect(155, 12, 170, 55, 12, XMAS_SNOW_WHITE);

    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", currentHour, currentMinute);
    gfx->setFont(u8g2_font_logisoso32_tn);
    gfx->setTextColor(XMAS_SNOW_WHITE);
    int16_t tx, ty;
    uint16_t tw, th;
    gfx->getTextBounds(timeStr, 0, 0, &tx, &ty, &tw, &th);
    gfx->setCursor(240 - tw/2, 52);
    gfx->print(timeStr);

    // Buon Natale
    if (currentSecond % 10 < 5) {
      gfx->setFont(u8g2_font_helvB14_tr);
      const char* msg = "Buon Natale!";
      gfx->getTextBounds(msg, 0, 0, &tx, &ty, &tw, &th);
      gfx->fillRoundRect(240 - tw/2 - 10, 458, tw + 20, 20, 5, 0xF800);
      gfx->setTextColor(XMAS_SNOW_WHITE);
      gfx->setCursor(240 - tw/2, 474);
      gfx->print(msg);
    } else {
      // Cancella scritta
      gfx->fillRect(130, 455, 220, 25, getXmasSkyColor(460));
    }
  }
}

#endif // EFFECT_CHRISTMAS
