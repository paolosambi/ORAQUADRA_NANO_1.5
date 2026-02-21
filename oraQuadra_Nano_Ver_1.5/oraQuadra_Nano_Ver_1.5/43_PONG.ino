// ===== 43_PONG.ino =====
// Gioco PONG per Dual Display ESP-NOW
// Canvas virtuale 960x480 (2x1 pannelli da 480x480)
// Master: logica + racchetta sinistra
// Slave: racchetta destra (touch forwarded via ESP-NOW)
//
// RENDERING: Lo sfondo (bordi, linea centrale, punteggio, velocita') e' un
// canvas permanente. La pallina viene disegnata PER ULTIMA e non cancella
// mai lo sfondo al suo passaggio. Quando la pallina si sposta, la vecchia
// posizione viene ripristinata con tutti gli elementi di sfondo.

#ifdef EFFECT_PONG

// ==================== VARIABILI GLOBALI ====================
bool pongInitialized = false;
int16_t pongBallX, pongBallY;
int16_t pongBallVX, pongBallVY;
int16_t pongPaddleLeftY, pongPaddleRightY;
uint8_t pongScoreLeft, pongScoreRight;

int16_t pongRemotePaddleY = 240;  // Y ricevuta dallo slave
unsigned long pongLastUpdate = 0;
bool pongGameOver = false;

// Posizioni precedenti per dirty-rect redraw
int16_t pongPrevBallX, pongPrevBallY;
int16_t pongPrevPaddleLeftY, pongPrevPaddleRightY;
uint8_t pongPrevScoreLeft, pongPrevScoreRight;
bool pongCenterlineDrawn = false;

// Sistema velocita' (5 livelli)
uint8_t pongSpeedLevel = 2;  // 0-4, default 2 (medio)
#define PONG_NUM_SPEEDS  5
const int8_t pongSpeedH[PONG_NUM_SPEEDS] = {4, 6, 8, 10, 13};
const int8_t pongSpeedV[PONG_NUM_SPEEDS] = {3, 4, 5,  7,  9};
bool pongSpeedDrawn = false;

// ==================== COSTANTI ====================
#define PONG_PADDLE_W    14
#define PONG_PADDLE_H    100
#define PONG_BALL_SIZE   14
#define PONG_PADDLE_L_X  20
#define PONG_PADDLE_R_X  926     // 960 - 20 - 14
#define PONG_MAX_SCORE   5
#define PONG_UPDATE_MS   25      // 40fps
#define PONG_CENTER_X    478     // X linea centrale (4px wide: 478-481)

// Colore campo
#define PONG_LINE_COLOR  0x4208

// HUD layout (coordinate locali per pannello, ripetuto su entrambi)
// Ogni pannello mostra: [P1 score] [speed bars] [P2 score]
#define HUD_Y           5
#define HUD_SL_LX       70       // Score P1 - offset locale da inizio pannello
#define HUD_SPEED_LX    180      // Speed bars - offset locale
#define HUD_SR_LX       350      // Score P2 - offset locale
#define HUD_SCORE_W     50
#define HUD_SCORE_H     38
#define HUD_SPEED_W     84
#define HUD_SPEED_H     38

// ==================== BYPASS DUALGFX ====================
#ifdef EFFECT_DUAL_DISPLAY
extern Arduino_RGB_Display *realGfx;
extern uint8_t panelX;
static Arduino_GFX *pongSavedGfx = nullptr;

// Verifica se un'area virtuale e' visibile sul pannello corrente (asse X)
static inline bool pongIsVisibleX(int16_t vx, int16_t w) {
  int16_t px = panelX * 480;
  return (vx + w > px && vx < px + 480);
}

static inline void pongBypassProxy() {
  extern Arduino_GFX *gfx;
  pongSavedGfx = gfx;
  if (isDualDisplayActive() && realGfx) {
    gfx = (Arduino_GFX*)realGfx;
  }
}

static inline void pongRestoreProxy() {
  extern Arduino_GFX *gfx;
  if (pongSavedGfx) {
    gfx = pongSavedGfx;
    pongSavedGfx = nullptr;
  }
}
#endif

// ==================== HELPER ====================

// Test overlap tra due rettangoli
static inline bool pongRectsOverlap(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
                                    int16_t bx, int16_t by, int16_t bw, int16_t bh) {
  return (ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by);
}

// ==================== FUNZIONI ====================

void resetPongBall() {
  pongBallX = 480 - PONG_BALL_SIZE / 2;
  pongBallY = 240 - PONG_BALL_SIZE / 2;
  int8_t hs = pongSpeedH[pongSpeedLevel];
  int8_t vs = pongSpeedV[pongSpeedLevel];
  pongBallVX = (random(2) == 0) ? hs : -hs;
  pongBallVY = random(-vs, vs + 1);
  if (pongBallVY == 0) pongBallVY = 2;
}

void initPong() {
  pongPaddleLeftY = 190;
  pongPaddleRightY = 190;
  pongRemotePaddleY = 240;
  pongScoreLeft = 0;
  pongScoreRight = 0;
  pongGameOver = false;
  pongCenterlineDrawn = false;
  pongSpeedDrawn = false;
  resetPongBall();

  pongPrevBallX = pongBallX;
  pongPrevBallY = pongBallY;
  pongPrevPaddleLeftY = pongPaddleLeftY;
  pongPrevPaddleRightY = pongPaddleRightY;
  pongPrevScoreLeft = 255;
  pongPrevScoreRight = 255;

  pongInitialized = true;
  pongLastUpdate = millis();

  #ifdef EFFECT_DUAL_DISPLAY
  if (isDualDisplayActive()) {
    vFillScreen(BLACK);
  } else {
    gfx->fillScreen(BLACK);
  }
  #else
  gfx->fillScreen(BLACK);
  #endif

  Serial.println("[PONG] Inizializzato");
}

// ==================== RIPRISTINO SFONDO (CANVAS) ====================
// Riempie un'area con BLACK e ridisegna tutti gli elementi di sfondo
// che cadono all'interno (bordi, linea centrale tratteggiata).
// Questo garantisce che lo sfondo sia sempre integro dopo la cancellazione
// di un oggetto in movimento (pallina, racchetta).
void restoreBg(int16_t rx, int16_t ry, int16_t rw, int16_t rh) {
  #ifdef EFFECT_DUAL_DISPLAY
  // 1. Riempie con nero
  vFillRect(rx, ry, rw, rh, BLACK);

  // 2. Ripristina bordo superiore (linea a Y=0)
  if (ry == 0) {
    vDrawHLine(rx, 0, rw, PONG_LINE_COLOR);
  }

  // 3. Ripristina bordo inferiore (linea a Y=479)
  if (ry + rh >= 479) {
    vDrawHLine(rx, 479, rw, PONG_LINE_COLOR);
  }

  // 4. Ripristina segmenti linea centrale tratteggiata
  if (rx + rw > PONG_CENTER_X && rx < PONG_CENTER_X + 4) {
    for (int y = 0; y < 480; y += 25) {
      if (ry + rh > y && ry < y + 12) {
        vFillRect(PONG_CENTER_X, y, 4, 12, PONG_LINE_COLOR);
      }
    }
  }
  #endif
}

void drawPongCenterline() {
  #ifdef EFFECT_DUAL_DISPLAY
  for (int y = 0; y < 480; y += 25) {
    vFillRect(PONG_CENTER_X, y, 4, 12, PONG_LINE_COLOR);
  }
  vDrawHLine(0, 0, 960, PONG_LINE_COLOR);
  vDrawHLine(0, 479, 960, PONG_LINE_COLOR);
  pongCenterlineDrawn = true;
  #endif
}

// ==================== HUD (punteggi + velocita' su entrambi i pannelli) ====================
// Disegna un punteggio in una posizione virtuale X
// Skip se l'area non e' visibile sul pannello corrente (evita wraparound framebuffer)
static void drawScoreAt(int16_t vx, uint8_t score) {
  #ifdef EFFECT_DUAL_DISPLAY
  if (!pongIsVisibleX(vx, HUD_SCORE_W)) return;
  vFillRect(vx, HUD_Y, HUD_SCORE_W, HUD_SCORE_H, BLACK);
  char s[4];
  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(WHITE);
  snprintf(s, sizeof(s), "%d", score);
  vSetCursor(vx + 15, HUD_Y + 32);
  vPrint(s);
  #endif
}

// Disegna le 5 barre velocita' in una posizione virtuale X
// Skip se l'area non e' visibile sul pannello corrente
static void drawSpeedBarsAt(int16_t vx) {
  #ifdef EFFECT_DUAL_DISPLAY
  if (!pongIsVisibleX(vx, HUD_SPEED_W)) return;
  vFillRect(vx, HUD_Y, HUD_SPEED_W, HUD_SPEED_H, BLACK);
  for (int i = 0; i < PONG_NUM_SPEEDS; i++) {
    int barH = 8 + i * 6;       // 8, 14, 20, 26, 32
    int barX = vx + 4 + i * 16;
    int barY = HUD_Y + HUD_SPEED_H - barH;
    uint16_t color = (i <= pongSpeedLevel) ? GREEN : 0x2104;
    vFillRect(barX, barY, 12, barH, color);
  }
  #endif
}

// Disegna l'intero HUD su entrambi i pannelli
// Ogni pannello mostra: [P1 score] [speed bars] [P2 score]
void drawPongHUD() {
  #ifdef EFFECT_DUAL_DISPLAY
  for (int p = 0; p < 2; p++) {
    int16_t px = p * 480;  // 0 per pannello sinistro, 480 per destro
    drawScoreAt(px + HUD_SL_LX, pongScoreLeft);
    drawSpeedBarsAt(px + HUD_SPEED_LX);
    drawScoreAt(px + HUD_SR_LX, pongScoreRight);
  }
  pongSpeedDrawn = true;
  #endif
}

void handlePongSpeedChange() {
  pongSpeedLevel = (pongSpeedLevel + 1) % PONG_NUM_SPEEDS;
  // Aggiorna velocita' orizzontale pallina (mantieni direzione)
  int8_t hs = pongSpeedH[pongSpeedLevel];
  pongBallVX = (pongBallVX > 0) ? hs : -hs;
  // Scala verticale proporzionalmente
  int8_t vs = pongSpeedV[pongSpeedLevel];
  if (pongBallVY > vs) pongBallVY = vs;
  else if (pongBallVY < -vs) pongBallVY = -vs;
  // Forza ridisegno indicatore
  pongSpeedDrawn = false;
  Serial.printf("[PONG] Velocita' livello %d (H:%d V:%d)\n", pongSpeedLevel + 1, hs, vs);
}

// ==================== DISEGNO CAMPO ====================
// Ordine di rendering (back-to-front):
//   1. Cancella vecchia pallina → restoreBg (ricostruisce sfondo)
//   2. Cancella vecchie racchette (delta) → restoreBg
//   3. Ridisegna racchette
//   4. Ridisegna punteggio (se cambiato O se pallina l'ha coperto)
//   5. Ridisegna indicatore velocita' (se cambiato O se pallina l'ha coperto)
//   6. Disegna pallina PER ULTIMA → sempre in primo piano
void drawPongField() {
  #ifdef EFFECT_DUAL_DISPLAY
  if (!isDualDisplayActive()) return;

  if (!pongCenterlineDrawn) {
    drawPongCenterline();
  }

  // --- Controlla se la vecchia pallina sovrapponeva un elemento HUD ---
  // L'HUD occupa la fascia Y=5..43. Se la pallina non era in quella fascia, skip.
  bool ballHitHUD = false;
  if (pongPrevBallY < HUD_Y + HUD_SCORE_H && pongPrevBallY + PONG_BALL_SIZE > HUD_Y) {
    for (int p = 0; p < 2 && !ballHitHUD; p++) {
      int16_t px = p * 480;
      if (pongRectsOverlap(pongPrevBallX, pongPrevBallY, PONG_BALL_SIZE, PONG_BALL_SIZE,
                            px + HUD_SL_LX, HUD_Y, HUD_SCORE_W, HUD_SCORE_H)) ballHitHUD = true;
      if (pongRectsOverlap(pongPrevBallX, pongPrevBallY, PONG_BALL_SIZE, PONG_BALL_SIZE,
                            px + HUD_SPEED_LX, HUD_Y, HUD_SPEED_W, HUD_SPEED_H)) ballHitHUD = true;
      if (pongRectsOverlap(pongPrevBallX, pongPrevBallY, PONG_BALL_SIZE, PONG_BALL_SIZE,
                            px + HUD_SR_LX, HUD_Y, HUD_SCORE_W, HUD_SCORE_H)) ballHitHUD = true;
    }
  }

  // --- 1. CANCELLA vecchia pallina e RIPRISTINA sfondo sotto di essa ---
  restoreBg(pongPrevBallX, pongPrevBallY, PONG_BALL_SIZE, PONG_BALL_SIZE);

  // --- 2. CANCELLA vecchie racchette (delta ottimizzato) e ripristina sfondo ---
  // Racchetta sinistra
  int16_t deltaL = pongPaddleLeftY - pongPrevPaddleLeftY;
  if (deltaL != 0) {
    if (abs(deltaL) >= PONG_PADDLE_H) {
      restoreBg(PONG_PADDLE_L_X, pongPrevPaddleLeftY, PONG_PADDLE_W, PONG_PADDLE_H);
    } else if (deltaL > 0) {
      restoreBg(PONG_PADDLE_L_X, pongPrevPaddleLeftY, PONG_PADDLE_W, deltaL);
    } else {
      restoreBg(PONG_PADDLE_L_X, pongPaddleLeftY + PONG_PADDLE_H, PONG_PADDLE_W, -deltaL);
    }
  }
  // Racchetta destra
  int16_t deltaR = pongPaddleRightY - pongPrevPaddleRightY;
  if (deltaR != 0) {
    if (abs(deltaR) >= PONG_PADDLE_H) {
      restoreBg(PONG_PADDLE_R_X, pongPrevPaddleRightY, PONG_PADDLE_W, PONG_PADDLE_H);
    } else if (deltaR > 0) {
      restoreBg(PONG_PADDLE_R_X, pongPrevPaddleRightY, PONG_PADDLE_W, deltaR);
    } else {
      restoreBg(PONG_PADDLE_R_X, pongPaddleRightY + PONG_PADDLE_H, PONG_PADDLE_W, -deltaR);
    }
  }

  // --- 3. DISEGNA racchette ---
  vFillRect(PONG_PADDLE_L_X, pongPaddleLeftY, PONG_PADDLE_W, PONG_PADDLE_H, WHITE);
  vFillRect(PONG_PADDLE_R_X, pongPaddleRightY, PONG_PADDLE_W, PONG_PADDLE_H, WHITE);

  // --- 4. RIDISEGNA HUD se punteggio cambiato, velocita' cambiata, o pallina lo aveva coperto ---
  bool scoreChanged = (pongScoreLeft != pongPrevScoreLeft || pongScoreRight != pongPrevScoreRight);
  if (scoreChanged || !pongSpeedDrawn || ballHitHUD) {
    drawPongHUD();
  }

  // --- 6. DISEGNA pallina PER ULTIMA (sempre in primo piano) ---
  vFillRect(pongBallX, pongBallY, PONG_BALL_SIZE, PONG_BALL_SIZE, WHITE);

  // --- 7. Salva posizioni per prossimo frame ---
  pongPrevBallX = pongBallX;
  pongPrevBallY = pongBallY;
  pongPrevPaddleLeftY = pongPaddleLeftY;
  pongPrevPaddleRightY = pongPaddleRightY;
  pongPrevScoreLeft = pongScoreLeft;
  pongPrevScoreRight = pongScoreRight;
  #endif
}

void drawPongStandalone() {
  gfx->fillScreen(BLACK);

  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(WHITE);
  gfx->setCursor(140, 180);
  gfx->print("PONG");

  gfx->setFont(u8g2_font_helvB14_te);
  gfx->setTextColor(0x7BEF);
  gfx->setCursor(50, 250);
  gfx->print("Attiva Multi-Display");
  gfx->setCursor(70, 280);
  gfx->print("per giocare a PONG!");

  gfx->setCursor(40, 330);
  gfx->setTextColor(0x4208);
  gfx->print("Configura 2 pannelli in");
  gfx->setCursor(55, 355);
  gfx->print("griglia 2x1 (Master/Slave)");

  gfx->setCursor(80, 420);
  gfx->setTextColor(YELLOW);
  gfx->print("Tocca per cambiare modo");
}

// Disegna box game over centrato sul display LOCALE (non usa coord. virtuali)
// Cosi' ogni pannello mostra il box identico senza bleeding tra pannelli.
void drawPongGameOver() {
  #ifdef EFFECT_DUAL_DISPLAY
  if (!isDualDisplayActive()) return;

  // Coordinate locali - centrato su display 480x480
  gfx->fillRect(100, 180, 280, 120, BLACK);
  gfx->drawRect(100, 180, 280, 120, WHITE);

  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(YELLOW);
  gfx->setCursor(145, 228);
  gfx->print(pongScoreLeft >= PONG_MAX_SCORE ? "P1 VINCE!" : "P2 VINCE!");

  gfx->setFont(u8g2_font_helvB14_te);
  gfx->setTextColor(WHITE);
  gfx->setCursor(105, 275);
  gfx->print("Tocca per ricominciare");
  #endif
}

void updatePong() {
  #ifdef EFFECT_DUAL_DISPLAY

  // === STANDALONE: mostra messaggio (nessun bypass necessario) ===
  if (!isDualDisplayActive()) {
    if (!pongInitialized) {
      drawPongStandalone();
      pongInitialized = true;
    }
    return;
  }

  // === DUAL DISPLAY ATTIVO ===
  pongBypassProxy();

  // === SLAVE: solo disegno con throttle ===
  if (isDualSlave()) {
    static bool slaveWasGameOver = false;

    if (!pongInitialized) {
      vFillScreen(BLACK);
      pongInitialized = true;
      pongLastUpdate = millis();
      pongCenterlineDrawn = false;
      pongSpeedDrawn = false;
      pongPrevBallX = pongBallX;
      pongPrevBallY = pongBallY;
      pongPrevPaddleLeftY = pongPaddleLeftY;
      pongPrevPaddleRightY = pongPaddleRightY;
      pongPrevScoreLeft = 255;
      pongPrevScoreRight = 255;
      slaveWasGameOver = false;
    }

    // Rileva restart: game over → nuovo gioco → pulisci schermo
    if (slaveWasGameOver && !pongGameOver) {
      vFillScreen(BLACK);
      pongCenterlineDrawn = false;
      pongSpeedDrawn = false;
      pongPrevBallX = pongBallX;
      pongPrevBallY = pongBallY;
      pongPrevPaddleLeftY = pongPaddleLeftY;
      pongPrevPaddleRightY = pongPaddleRightY;
      pongPrevScoreLeft = 255;
      pongPrevScoreRight = 255;
    }
    slaveWasGameOver = pongGameOver;

    unsigned long now = millis();
    if (now - pongLastUpdate < PONG_UPDATE_MS) {
      pongRestoreProxy();
      return;
    }
    pongLastUpdate = now;

    drawPongField();
    if (pongGameOver) drawPongGameOver();
    pongRestoreProxy();
    return;
  }

  // === MASTER: logica + disegno ===
  if (!pongInitialized) {
    initPong();
  }

  unsigned long now = millis();
  if (now - pongLastUpdate < PONG_UPDATE_MS) {
    pongRestoreProxy();
    return;
  }
  pongLastUpdate = now;

  // Game over: aspetta touch per restart
  if (pongGameOver) {
    drawPongField();
    drawPongGameOver();
    pongRestoreProxy();
    return;
  }

  // Aggiorna racchetta destra con Y ricevuta dallo slave
  pongPaddleRightY = pongRemotePaddleY - PONG_PADDLE_H / 2;
  if (pongPaddleRightY < 0) pongPaddleRightY = 0;
  if (pongPaddleRightY > 480 - PONG_PADDLE_H) pongPaddleRightY = 480 - PONG_PADDLE_H;

  // Muovi pallina
  pongBallX += pongBallVX;
  pongBallY += pongBallVY;

  // Collisione bordi superiore/inferiore
  if (pongBallY <= 0) {
    pongBallY = 0;
    pongBallVY = -pongBallVY;
  }
  if (pongBallY >= 480 - PONG_BALL_SIZE) {
    pongBallY = 480 - PONG_BALL_SIZE;
    pongBallVY = -pongBallVY;
  }

  // Velocita' correnti dal livello
  int8_t curHS = pongSpeedH[pongSpeedLevel];
  int8_t curVS = pongSpeedV[pongSpeedLevel];

  // Collisione racchetta sinistra
  if (pongBallVX < 0 &&
      pongBallX <= PONG_PADDLE_L_X + PONG_PADDLE_W &&
      pongBallX + PONG_BALL_SIZE >= PONG_PADDLE_L_X &&
      pongBallY + PONG_BALL_SIZE >= pongPaddleLeftY &&
      pongBallY <= pongPaddleLeftY + PONG_PADDLE_H) {
    pongBallX = PONG_PADDLE_L_X + PONG_PADDLE_W;
    pongBallVX = curHS;
    int hitPos = (pongBallY + PONG_BALL_SIZE / 2) - (pongPaddleLeftY + PONG_PADDLE_H / 2);
    pongBallVY = (int16_t)hitPos * curVS / (PONG_PADDLE_H / 2);
    if (pongBallVY == 0) pongBallVY = (random(2) == 0) ? 2 : -2;
    if (pongBallVY > curVS) pongBallVY = curVS;
    if (pongBallVY < -curVS) pongBallVY = -curVS;
  }

  // Collisione racchetta destra
  if (pongBallVX > 0 &&
      pongBallX + PONG_BALL_SIZE >= PONG_PADDLE_R_X &&
      pongBallX <= PONG_PADDLE_R_X + PONG_PADDLE_W &&
      pongBallY + PONG_BALL_SIZE >= pongPaddleRightY &&
      pongBallY <= pongPaddleRightY + PONG_PADDLE_H) {
    pongBallX = PONG_PADDLE_R_X - PONG_BALL_SIZE;
    pongBallVX = -curHS;
    int hitPos = (pongBallY + PONG_BALL_SIZE / 2) - (pongPaddleRightY + PONG_PADDLE_H / 2);
    pongBallVY = (int16_t)hitPos * curVS / (PONG_PADDLE_H / 2);
    if (pongBallVY == 0) pongBallVY = (random(2) == 0) ? 2 : -2;
    if (pongBallVY > curVS) pongBallVY = curVS;
    if (pongBallVY < -curVS) pongBallVY = -curVS;
  }

  // Punto segnato
  if (pongBallX < 0) {
    pongScoreRight++;
    if (pongScoreRight >= PONG_MAX_SCORE) {
      pongGameOver = true;
    } else {
      resetPongBall();
    }
  }
  if (pongBallX > 960 - PONG_BALL_SIZE) {
    pongScoreLeft++;
    if (pongScoreLeft >= PONG_MAX_SCORE) {
      pongGameOver = true;
    } else {
      resetPongBall();
    }
  }

  drawPongField();
  pongRestoreProxy();

  #else
  if (!pongInitialized) {
    drawPongStandalone();
    pongInitialized = true;
  }
  #endif
}

void handlePongTouch(int x, int y) {
  #ifdef EFFECT_DUAL_DISPLAY
  if (pongGameOver) {
    initPong();
    return;
  }
  pongPaddleLeftY = y - PONG_PADDLE_H / 2;
  if (pongPaddleLeftY < 0) pongPaddleLeftY = 0;
  if (pongPaddleLeftY > 480 - PONG_PADDLE_H) pongPaddleLeftY = 480 - PONG_PADDLE_H;
  #endif
}

void handlePongRemoteTouch(int vy) {
  pongRemotePaddleY = vy;

  #ifdef EFFECT_DUAL_DISPLAY
  if (pongGameOver) {
    initPong();
  }
  #endif
}

#endif // EFFECT_PONG
