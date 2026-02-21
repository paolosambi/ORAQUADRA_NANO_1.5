// ===== 46_BREAKOUT.ino =====
// Gioco BREAKOUT per Dual Display ESP-NOW
// Canvas virtuale 960x480 (2x1 pannelli da 480x480)
// Master: logica + paddle (touch X orizzontale)
// Slave: touch X forwarded via ESP-NOW
//
// RENDERING: Sfondo (bordi, mattoncini, HUD) e' canvas permanente.
// La pallina viene disegnata PER ULTIMA e non cancella mai lo sfondo.

#ifdef EFFECT_BREAKOUT

// ==================== VARIABILI GLOBALI ====================
bool breakoutInitialized = false;
int16_t brkBallX, brkBallY;
int16_t brkBallVX, brkBallVY;
int16_t brkPaddleX;            // Centro paddle (coord. virtuale X)
uint16_t brkScore;
uint8_t  brkLives;
bool     brkGameOver = false;
bool     brkWin = false;
unsigned long brkLastUpdate = 0;

// Posizioni precedenti per dirty-rect
int16_t brkPrevBallX, brkPrevBallY;
int16_t brkPrevPaddleX;
uint16_t brkPrevScore;
uint8_t  brkPrevLives;

// Velocita' (5 livelli)
uint8_t brkSpeedLevel = 2;
#define BRK_NUM_SPEEDS  5
const int8_t brkSpeedH[BRK_NUM_SPEEDS] = {4, 5, 7, 9, 12};
const int8_t brkSpeedV[BRK_NUM_SPEEDS] = {3, 4, 5, 7, 9};
bool brkHudDrawn = false;

// Touch remoto (X dal slave)
int16_t brkRemotePaddleX = 480;
bool brkBordersDrawn = false;

// ==================== COSTANTI ====================
#define BRK_PADDLE_W     120
#define BRK_PADDLE_H     14
#define BRK_PADDLE_Y     456    // Y fissa del paddle
#define BRK_BALL_SIZE    10
#define BRK_UPDATE_MS    25     // 40fps
#define BRK_MAX_LIVES    3

// Mattoncini: 16 colonne x 5 righe = 80 bricks
#define BRK_COLS         16
#define BRK_ROWS         5
#define BRK_BRICK_W      54     // larghezza mattoncino
#define BRK_BRICK_H      16     // altezza mattoncino
#define BRK_GAP          4      // gap tra mattoncini
#define BRK_AREA_X       8      // offset X iniziale area mattoncini
#define BRK_AREA_Y       55     // offset Y iniziale (sotto HUD)
// Larghezza totale: 16*(54+4) - 4 = 924 + 8*2 padding = ok in 960

// Colori per riga (dall'alto al basso)
static const uint16_t brkRowColors[BRK_ROWS] = {
  0xF800,  // Rosso
  0xFD20,  // Arancione
  0xFFE0,  // Giallo
  0x07E0,  // Verde
  0x001F   // Blu
};
// Punti per riga (righe alte valgono di piu')
static const uint8_t brkRowPoints[BRK_ROWS] = {5, 4, 3, 2, 1};

// Stato mattoncini (80 bit = 10 bytes)
uint8_t brkBricks[10];      // bit 1 = mattoncino presente
uint8_t brkPrevBricks[10];  // stato precedente per rilevare distruzioni
#define BRK_LINE_COLOR  0x4208

// HUD layout (coordinate locali per pannello, ripetuto su entrambi)
#define BRK_HUD_Y        5
#define BRK_HUD_SCORE_X  70
#define BRK_HUD_SPEED_X  200
#define BRK_HUD_LIVES_X  350
#define BRK_HUD_W        80
#define BRK_HUD_H        38

// ==================== BYPASS DUALGFX ====================
#ifdef EFFECT_DUAL_DISPLAY
extern Arduino_RGB_Display *realGfx;
extern uint8_t panelX;
static Arduino_GFX *brkSavedGfx = nullptr;

static inline bool brkIsVisibleX(int16_t vx, int16_t w) {
  int16_t px = panelX * 480;
  return (vx + w > px && vx < px + 480);
}

static inline void brkBypassProxy() {
  extern Arduino_GFX *gfx;
  brkSavedGfx = gfx;
  if (isDualDisplayActive() && realGfx) {
    gfx = (Arduino_GFX*)realGfx;
  }
}

static inline void brkRestoreProxy() {
  extern Arduino_GFX *gfx;
  if (brkSavedGfx) {
    gfx = brkSavedGfx;
    brkSavedGfx = nullptr;
  }
}
#endif

// ==================== HELPER MATTONCINI ====================
static inline bool brkGetBrick(int col, int row) {
  int idx = row * BRK_COLS + col;
  return (brkBricks[idx / 8] >> (idx % 8)) & 1;
}

static inline void brkSetBrick(int col, int row, bool val) {
  int idx = row * BRK_COLS + col;
  if (val) brkBricks[idx / 8] |= (1 << (idx % 8));
  else     brkBricks[idx / 8] &= ~(1 << (idx % 8));
}

static inline void brkGetBrickRect(int col, int row, int16_t &bx, int16_t &by, int16_t &bw, int16_t &bh) {
  bx = BRK_AREA_X + col * (BRK_BRICK_W + BRK_GAP);
  by = BRK_AREA_Y + row * (BRK_BRICK_H + BRK_GAP);
  bw = BRK_BRICK_W;
  bh = BRK_BRICK_H;
}

static int brkCountBricks() {
  int count = 0;
  for (int r = 0; r < BRK_ROWS; r++)
    for (int c = 0; c < BRK_COLS; c++)
      if (brkGetBrick(c, r)) count++;
  return count;
}

// ==================== FUNZIONI ====================

void resetBreakoutBall() {
  brkBallX = brkPaddleX;
  brkBallY = BRK_PADDLE_Y - BRK_BALL_SIZE - 2;
  int8_t hs = brkSpeedH[brkSpeedLevel];
  int8_t vs = brkSpeedV[brkSpeedLevel];
  brkBallVX = (random(2) == 0) ? hs : -hs;
  brkBallVY = -vs;  // Sempre verso l'alto all'inizio
}

void initBreakout() {
  brkPaddleX = 480;  // Centro canvas 960
  brkRemotePaddleX = 480;
  brkScore = 0;
  brkLives = BRK_MAX_LIVES;
  brkGameOver = false;
  brkWin = false;
  brkHudDrawn = false;
  brkBordersDrawn = false;

  // Tutti i mattoncini presenti
  memset(brkBricks, 0xFF, sizeof(brkBricks));
  memset(brkPrevBricks, 0xFF, sizeof(brkPrevBricks));

  resetBreakoutBall();

  brkPrevBallX = brkBallX;
  brkPrevBallY = brkBallY;
  brkPrevPaddleX = brkPaddleX;
  brkPrevScore = 0xFFFF;
  brkPrevLives = 0xFF;

  breakoutInitialized = true;
  brkLastUpdate = millis();

  #ifdef EFFECT_DUAL_DISPLAY
  if (isDualDisplayActive()) {
    vFillScreen(BLACK);
  } else {
    gfx->fillScreen(BLACK);
  }
  #else
  gfx->fillScreen(BLACK);
  #endif

  Serial.println("[BREAKOUT] Inizializzato");
}

// ==================== RIPRISTINO SFONDO (CANVAS) ====================
void brkRestoreBg(int16_t rx, int16_t ry, int16_t rw, int16_t rh) {
  #ifdef EFFECT_DUAL_DISPLAY
  vFillRect(rx, ry, rw, rh, BLACK);

  // Bordo superiore
  if (ry == 0) vDrawHLine(rx, 0, rw, BRK_LINE_COLOR);
  // Bordo sinistro
  if (rx <= 0 && rw > 0) vDrawVLine(0, ry, rh, BRK_LINE_COLOR);
  // Bordo destro
  if (rx + rw >= 959) vDrawVLine(959, ry, rh, BRK_LINE_COLOR);

  // Ripristina mattoncini che si sovrappongono all'area
  for (int r = 0; r < BRK_ROWS; r++) {
    for (int c = 0; c < BRK_COLS; c++) {
      if (!brkGetBrick(c, r)) continue;
      int16_t bx, by, bw, bh;
      brkGetBrickRect(c, r, bx, by, bw, bh);
      // Overlap check
      if (rx < bx + bw && rx + rw > bx && ry < by + bh && ry + rh > by) {
        vFillRect(bx, by, bw, bh, brkRowColors[r]);
      }
    }
  }
  #endif
}

// ==================== DISEGNO MATTONCINI ====================
void drawAllBricks() {
  #ifdef EFFECT_DUAL_DISPLAY
  for (int r = 0; r < BRK_ROWS; r++) {
    for (int c = 0; c < BRK_COLS; c++) {
      if (!brkGetBrick(c, r)) continue;
      int16_t bx, by, bw, bh;
      brkGetBrickRect(c, r, bx, by, bw, bh);
      vFillRect(bx, by, bw, bh, brkRowColors[r]);
    }
  }
  #endif
}

// ==================== DISEGNO BORDI ====================
void drawBreakoutBorders() {
  #ifdef EFFECT_DUAL_DISPLAY
  vDrawHLine(0, 0, 960, BRK_LINE_COLOR);     // Bordo superiore
  vDrawVLine(0, 0, 480, BRK_LINE_COLOR);      // Bordo sinistro
  vDrawVLine(959, 0, 480, BRK_LINE_COLOR);    // Bordo destro
  #endif
}

// ==================== HUD ====================
static void brkDrawScoreAt(int16_t vx) {
  #ifdef EFFECT_DUAL_DISPLAY
  if (!brkIsVisibleX(vx, BRK_HUD_W)) return;
  vFillRect(vx, BRK_HUD_Y, BRK_HUD_W, BRK_HUD_H, BLACK);
  char s[8];
  gfx->setFont(u8g2_font_helvB14_te);
  gfx->setTextColor(WHITE);
  snprintf(s, sizeof(s), "%d", brkScore);
  vSetCursor(vx + 5, BRK_HUD_Y + 28);
  vPrint(s);
  #endif
}

static void brkDrawSpeedBarsAt(int16_t vx) {
  #ifdef EFFECT_DUAL_DISPLAY
  if (!brkIsVisibleX(vx, BRK_HUD_W)) return;
  vFillRect(vx, BRK_HUD_Y, BRK_HUD_W, BRK_HUD_H, BLACK);
  for (int i = 0; i < BRK_NUM_SPEEDS; i++) {
    int barH = 8 + i * 6;
    int barX = vx + 4 + i * 15;
    int barY = BRK_HUD_Y + BRK_HUD_H - barH;
    uint16_t color = (i <= brkSpeedLevel) ? GREEN : 0x2104;
    vFillRect(barX, barY, 11, barH, color);
  }
  #endif
}

static void brkDrawLivesAt(int16_t vx) {
  #ifdef EFFECT_DUAL_DISPLAY
  if (!brkIsVisibleX(vx, BRK_HUD_W)) return;
  vFillRect(vx, BRK_HUD_Y, BRK_HUD_W, BRK_HUD_H, BLACK);
  // Disegna cuori/pallini per le vite
  for (int i = 0; i < brkLives; i++) {
    int cx = vx + 15 + i * 25;
    int cy = BRK_HUD_Y + BRK_HUD_H / 2;
    vFillCircle(cx, cy, 8, RED);
  }
  #endif
}

void drawBreakoutHUD() {
  #ifdef EFFECT_DUAL_DISPLAY
  for (int p = 0; p < 2; p++) {
    int16_t px = p * 480;
    brkDrawScoreAt(px + BRK_HUD_SCORE_X);
    brkDrawSpeedBarsAt(px + BRK_HUD_SPEED_X);
    brkDrawLivesAt(px + BRK_HUD_LIVES_X);
  }
  brkHudDrawn = true;
  #endif
}

// ==================== DISEGNO CAMPO ====================
void drawBreakoutField() {
  #ifdef EFFECT_DUAL_DISPLAY
  if (!isDualDisplayActive()) return;

  if (!brkBordersDrawn) {
    drawBreakoutBorders();
    drawAllBricks();
    memcpy(brkPrevBricks, brkBricks, 10);
    brkBordersDrawn = true;
  }

  // Cancella mattoncini distrutti (confronto stato precedente vs attuale)
  if (memcmp(brkPrevBricks, brkBricks, 10) != 0) {
    for (int r = 0; r < BRK_ROWS; r++) {
      for (int c = 0; c < BRK_COLS; c++) {
        int idx = r * BRK_COLS + c;
        bool wasThere = (brkPrevBricks[idx / 8] >> (idx % 8)) & 1;
        bool isNow    = (brkBricks[idx / 8] >> (idx % 8)) & 1;
        if (wasThere && !isNow) {
          int16_t bx, by, bw, bh;
          brkGetBrickRect(c, r, bx, by, bw, bh);
          vFillRect(bx, by, bw, bh, BLACK);
        }
      }
    }
    memcpy(brkPrevBricks, brkBricks, 10);
  }

  // Controlla se pallina copriva HUD
  bool ballHitHUD = false;
  if (brkPrevBallY < BRK_HUD_Y + BRK_HUD_H && brkPrevBallY + BRK_BALL_SIZE > BRK_HUD_Y) {
    ballHitHUD = true;
  }

  // 1. Cancella vecchia pallina e ripristina sfondo
  brkRestoreBg(brkPrevBallX, brkPrevBallY, BRK_BALL_SIZE, BRK_BALL_SIZE);

  // 2. Cancella vecchio paddle (delta)
  int16_t prevPL = brkPrevPaddleX - BRK_PADDLE_W / 2;
  int16_t currPL = brkPaddleX - BRK_PADDLE_W / 2;
  int16_t deltaP = currPL - prevPL;
  if (deltaP != 0) {
    if (abs(deltaP) >= BRK_PADDLE_W) {
      brkRestoreBg(prevPL, BRK_PADDLE_Y, BRK_PADDLE_W, BRK_PADDLE_H);
    } else if (deltaP > 0) {
      brkRestoreBg(prevPL, BRK_PADDLE_Y, deltaP, BRK_PADDLE_H);
    } else {
      brkRestoreBg(currPL + BRK_PADDLE_W, BRK_PADDLE_Y, -deltaP, BRK_PADDLE_H);
    }
  }

  // 3. Disegna paddle
  vFillRect(brkPaddleX - BRK_PADDLE_W / 2, BRK_PADDLE_Y, BRK_PADDLE_W, BRK_PADDLE_H, CYAN);

  // 4. HUD se necessario
  bool scoreChanged = (brkScore != brkPrevScore || brkLives != brkPrevLives);
  if (scoreChanged || !brkHudDrawn || ballHitHUD) {
    drawBreakoutHUD();
  }

  // 5. Pallina PER ULTIMA
  vFillRect(brkBallX, brkBallY, BRK_BALL_SIZE, BRK_BALL_SIZE, WHITE);

  // Salva posizioni
  brkPrevBallX = brkBallX;
  brkPrevBallY = brkBallY;
  brkPrevPaddleX = brkPaddleX;
  brkPrevScore = brkScore;
  brkPrevLives = brkLives;
  #endif
}

// ==================== GAME OVER / WIN ====================
void drawBreakoutGameOver() {
  #ifdef EFFECT_DUAL_DISPLAY
  if (!isDualDisplayActive()) return;
  // Coordinate locali (centrato su display 480x480)
  gfx->fillRect(80, 180, 320, 120, BLACK);
  gfx->drawRect(80, 180, 320, 120, WHITE);
  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(brkWin ? GREEN : RED);
  gfx->setCursor(110, 228);
  gfx->print(brkWin ? "HAI VINTO!" : "GAME OVER");
  gfx->setFont(u8g2_font_helvB14_te);
  gfx->setTextColor(YELLOW);
  char buf[20];
  snprintf(buf, sizeof(buf), "Punti: %d", brkScore);
  gfx->setCursor(160, 260);
  gfx->print(buf);
  gfx->setTextColor(WHITE);
  gfx->setCursor(100, 285);
  gfx->print("Tocca per ricominciare");
  #endif
}

// ==================== STANDALONE ====================
void drawBreakoutStandalone() {
  gfx->fillScreen(BLACK);
  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(CYAN);
  gfx->setCursor(100, 180);
  gfx->print("BREAKOUT");
  gfx->setFont(u8g2_font_helvB14_te);
  gfx->setTextColor(0x7BEF);
  gfx->setCursor(50, 250);
  gfx->print("Attiva Multi-Display");
  gfx->setCursor(50, 280);
  gfx->print("per giocare a BREAKOUT!");
  gfx->setCursor(40, 330);
  gfx->setTextColor(0x4208);
  gfx->print("Configura 2 pannelli in");
  gfx->setCursor(55, 355);
  gfx->print("griglia 2x1 (Master/Slave)");
  gfx->setCursor(80, 420);
  gfx->setTextColor(YELLOW);
  gfx->print("Tocca per cambiare modo");
}

// ==================== UPDATE ====================
void updateBreakout() {
  #ifdef EFFECT_DUAL_DISPLAY

  // === STANDALONE ===
  if (!isDualDisplayActive()) {
    if (!breakoutInitialized) {
      drawBreakoutStandalone();
      breakoutInitialized = true;
    }
    return;
  }

  // === DUAL DISPLAY ATTIVO ===
  brkBypassProxy();

  // === SLAVE: solo disegno ===
  if (isDualSlave()) {
    static bool slaveWasGameOver = false;

    if (!breakoutInitialized) {
      vFillScreen(BLACK);
      breakoutInitialized = true;
      brkLastUpdate = millis();
      brkHudDrawn = false;
      brkBordersDrawn = false;
      brkPrevBallX = brkBallX;
      brkPrevBallY = brkBallY;
      brkPrevPaddleX = brkPaddleX;
      brkPrevScore = 0xFFFF;
      brkPrevLives = 0xFF;
      memcpy(brkPrevBricks, brkBricks, 10);
      slaveWasGameOver = false;
    }

    // Rileva restart
    if (slaveWasGameOver && !brkGameOver) {
      vFillScreen(BLACK);
      brkHudDrawn = false;
      brkBordersDrawn = false;
      brkPrevBallX = brkBallX;
      brkPrevBallY = brkBallY;
      brkPrevPaddleX = brkPaddleX;
      brkPrevScore = 0xFFFF;
      brkPrevLives = 0xFF;
      memcpy(brkPrevBricks, brkBricks, 10);
    }
    slaveWasGameOver = brkGameOver;

    unsigned long now = millis();
    if (now - brkLastUpdate < BRK_UPDATE_MS) {
      brkRestoreProxy();
      return;
    }
    brkLastUpdate = now;

    drawBreakoutField();
    if (brkGameOver || brkWin) drawBreakoutGameOver();
    brkRestoreProxy();
    return;
  }

  // === MASTER: logica + disegno ===
  if (!breakoutInitialized) {
    initBreakout();
  }

  unsigned long now = millis();
  if (now - brkLastUpdate < BRK_UPDATE_MS) {
    brkRestoreProxy();
    return;
  }
  brkLastUpdate = now;

  // Game over o win: aspetta touch
  if (brkGameOver || brkWin) {
    drawBreakoutField();
    drawBreakoutGameOver();
    brkRestoreProxy();
    return;
  }

  // Aggiorna paddle X con touch remoto (slave)
  // brkRemotePaddleX viene aggiornato dal touch
  brkPaddleX = brkRemotePaddleX;
  if (brkPaddleX < BRK_PADDLE_W / 2) brkPaddleX = BRK_PADDLE_W / 2;
  if (brkPaddleX > 960 - BRK_PADDLE_W / 2) brkPaddleX = 960 - BRK_PADDLE_W / 2;

  // Muovi pallina
  brkBallX += brkBallVX;
  brkBallY += brkBallVY;

  int8_t curHS = brkSpeedH[brkSpeedLevel];
  int8_t curVS = brkSpeedV[brkSpeedLevel];

  // Collisione bordo sinistro
  if (brkBallX <= 1) {
    brkBallX = 1;
    brkBallVX = abs(brkBallVX);
  }
  // Collisione bordo destro
  if (brkBallX >= 960 - BRK_BALL_SIZE - 1) {
    brkBallX = 960 - BRK_BALL_SIZE - 1;
    brkBallVX = -abs(brkBallVX);
  }
  // Collisione bordo superiore
  if (brkBallY <= 1) {
    brkBallY = 1;
    brkBallVY = abs(brkBallVY);
  }

  // Collisione paddle
  int16_t padL = brkPaddleX - BRK_PADDLE_W / 2;
  int16_t padR = brkPaddleX + BRK_PADDLE_W / 2;
  if (brkBallVY > 0 &&
      brkBallY + BRK_BALL_SIZE >= BRK_PADDLE_Y &&
      brkBallY < BRK_PADDLE_Y + BRK_PADDLE_H &&
      brkBallX + BRK_BALL_SIZE >= padL &&
      brkBallX <= padR) {
    brkBallY = BRK_PADDLE_Y - BRK_BALL_SIZE;
    brkBallVY = -curVS;
    // Angolo in base a dove colpisce il paddle
    int hitCenter = (brkBallX + BRK_BALL_SIZE / 2) - brkPaddleX;
    brkBallVX = (int16_t)hitCenter * curHS / (BRK_PADDLE_W / 2);
    if (brkBallVX == 0) brkBallVX = (random(2) == 0) ? 2 : -2;
    if (brkBallVX > curHS) brkBallVX = curHS;
    if (brkBallVX < -curHS) brkBallVX = -curHS;
  }

  // Pallina persa (sotto il paddle)
  if (brkBallY > 480) {
    brkLives--;
    if (brkLives == 0) {
      brkGameOver = true;
    } else {
      resetBreakoutBall();
    }
  }

  // Collisione mattoncini
  for (int r = 0; r < BRK_ROWS; r++) {
    for (int c = 0; c < BRK_COLS; c++) {
      if (!brkGetBrick(c, r)) continue;
      int16_t bx, by, bw, bh;
      brkGetBrickRect(c, r, bx, by, bw, bh);
      // Overlap ball-brick
      if (brkBallX + BRK_BALL_SIZE > bx && brkBallX < bx + bw &&
          brkBallY + BRK_BALL_SIZE > by && brkBallY < by + bh) {
        // Rimuovi mattoncino
        brkSetBrick(c, r, false);
        brkScore += brkRowPoints[r];
        // Cancella mattoncino dallo schermo
        vFillRect(bx, by, bw, bh, BLACK);
        // Rimbalzo: determina lato di collisione
        int16_t overlapL = (brkBallX + BRK_BALL_SIZE) - bx;
        int16_t overlapR = (bx + bw) - brkBallX;
        int16_t overlapT = (brkBallY + BRK_BALL_SIZE) - by;
        int16_t overlapB = (by + bh) - brkBallY;
        int16_t minOverX = (overlapL < overlapR) ? overlapL : overlapR;
        int16_t minOverY = (overlapT < overlapB) ? overlapT : overlapB;
        if (minOverX < minOverY) {
          brkBallVX = -brkBallVX;
        } else {
          brkBallVY = -brkBallVY;
        }
        // Un solo mattoncino per frame
        goto brickDone;
      }
    }
  }
  brickDone:

  // Vittoria
  if (brkCountBricks() == 0) {
    brkWin = true;
  }

  drawBreakoutField();
  brkRestoreProxy();

  #else
  if (!breakoutInitialized) {
    drawBreakoutStandalone();
    breakoutInitialized = true;
  }
  #endif
}

// ==================== TOUCH ====================
void handleBreakoutTouch(int x, int y) {
  #ifdef EFFECT_DUAL_DISPLAY
  if (brkGameOver || brkWin) {
    initBreakout();
    return;
  }
  // x e' la coordinata locale del pannello master (0-479)
  brkRemotePaddleX = x;
  #endif
}

void handleBreakoutRemoteTouch(int vx, int vy) {
  // vx e' la coordinata virtuale X (0-959)
  brkRemotePaddleX = vx;
  #ifdef EFFECT_DUAL_DISPLAY
  if (brkGameOver || brkWin) {
    initBreakout();
  }
  #endif
}

void handleBreakoutSpeedChange() {
  brkSpeedLevel = (brkSpeedLevel + 1) % BRK_NUM_SPEEDS;
  int8_t hs = brkSpeedH[brkSpeedLevel];
  int8_t vs = brkSpeedV[brkSpeedLevel];
  brkBallVX = (brkBallVX > 0) ? hs : -hs;
  if (brkBallVY > 0) brkBallVY = vs;
  else brkBallVY = -vs;
  brkHudDrawn = false;
  Serial.printf("[BREAKOUT] Velocita' livello %d\n", brkSpeedLevel + 1);
}

#endif // EFFECT_BREAKOUT
