// ===== 43_PONG.ino =====
// Gioco PONG per Dual Display ESP-NOW
// Canvas virtuale 960x480 (2x1 pannelli da 480x480)
// Master: logica + racchetta sinistra
// Slave: racchetta destra (touch forwarded via ESP-NOW)
//
// NOTA: Quando DualGFX proxy e' attivo, gfx-> aggiunge un offset di centratura
// che rompe le coordinate v*(). Prima di disegnare, PONG bypassa il proxy
// impostando gfx = realGfx, poi lo ripristina alla fine.

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

// ==================== COSTANTI ====================
#define PONG_PADDLE_W    14
#define PONG_PADDLE_H    100
#define PONG_BALL_SIZE   14
#define PONG_PADDLE_L_X  20
#define PONG_PADDLE_R_X  926     // 960 - 20 - 14
#define PONG_MAX_SCORE   5
#define PONG_UPDATE_MS   25      // 40fps
#define PONG_BALL_SPEED  7
#define PONG_BALL_VSPEED 5
#define PONG_CENTER_X    478     // X linea centrale (4px wide: 478-481)

// ==================== BYPASS DUALGFX ====================
// Il proxy DualGFX aggiunge un offset di centratura a tutte le chiamate gfx->
// Le funzioni v*() fanno gia' la conversione coordinate, quindi serve il display reale.
// Queste macro salvano/ripristinano gfx attorno al codice di rendering.

#ifdef EFFECT_DUAL_DISPLAY
extern Arduino_RGB_Display *realGfx;
static Arduino_GFX *pongSavedGfx = nullptr;

static inline void pongBypassProxy() {
  extern Arduino_GFX *gfx;
  pongSavedGfx = gfx;  // Salva il puntatore attuale (DualGFX o realGfx)
  if (isDualDisplayActive() && realGfx) {
    gfx = (Arduino_GFX*)realGfx;  // Bypassa DualGFX
  }
}

static inline void pongRestoreProxy() {
  extern Arduino_GFX *gfx;
  if (pongSavedGfx) {
    gfx = pongSavedGfx;  // Ripristina il puntatore originale
    pongSavedGfx = nullptr;
  }
}
#endif

// ==================== FUNZIONI ====================

void resetPongBall() {
  pongBallX = 480 - PONG_BALL_SIZE / 2;
  pongBallY = 240 - PONG_BALL_SIZE / 2;
  pongBallVX = (random(2) == 0) ? PONG_BALL_SPEED : -PONG_BALL_SPEED;
  pongBallVY = random(-PONG_BALL_VSPEED, PONG_BALL_VSPEED + 1);
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
  resetPongBall();

  pongPrevBallX = pongBallX;
  pongPrevBallY = pongBallY;
  pongPrevPaddleLeftY = pongPaddleLeftY;
  pongPrevPaddleRightY = pongPaddleRightY;
  pongPrevScoreLeft = 255;
  pongPrevScoreRight = 255;

  pongInitialized = true;
  pongLastUpdate = millis();

  // Pulisci schermo (vFillScreen usa gfx->fillScreen che funziona sempre)
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

void drawPongCenterline() {
  #ifdef EFFECT_DUAL_DISPLAY
  for (int y = 0; y < 480; y += 25) {
    vFillRect(PONG_CENTER_X, y, 4, 12, 0x4208);
  }
  vDrawHLine(0, 0, 960, 0x4208);
  vDrawHLine(0, 479, 960, 0x4208);
  pongCenterlineDrawn = true;
  #endif
}

void repairCenterline(int16_t prevBX, int16_t prevBY) {
  #ifdef EFFECT_DUAL_DISPLAY
  if (prevBX + PONG_BALL_SIZE < PONG_CENTER_X || prevBX > PONG_CENTER_X + 4) return;
  for (int y = 0; y < 480; y += 25) {
    if (prevBY + PONG_BALL_SIZE >= y && prevBY <= y + 12) {
      vFillRect(PONG_CENTER_X, y, 4, 12, 0x4208);
    }
  }
  if (prevBY <= 1) vDrawHLine(prevBX, 0, PONG_BALL_SIZE, 0x4208);
  if (prevBY + PONG_BALL_SIZE >= 478) vDrawHLine(prevBX, 479, PONG_BALL_SIZE, 0x4208);
  #endif
}

void drawPongField() {
  #ifdef EFFECT_DUAL_DISPLAY
  if (!isDualDisplayActive()) return;

  if (!pongCenterlineDrawn) {
    drawPongCenterline();
  }

  // Cancella posizioni precedenti (dirty rect)
  vFillRect(pongPrevBallX, pongPrevBallY, PONG_BALL_SIZE, PONG_BALL_SIZE, BLACK);
  vFillRect(PONG_PADDLE_L_X, pongPrevPaddleLeftY, PONG_PADDLE_W, PONG_PADDLE_H, BLACK);
  vFillRect(PONG_PADDLE_R_X, pongPrevPaddleRightY, PONG_PADDLE_W, PONG_PADDLE_H, BLACK);

  repairCenterline(pongPrevBallX, pongPrevBallY);

  // Racchette
  vFillRect(PONG_PADDLE_L_X, pongPaddleLeftY, PONG_PADDLE_W, PONG_PADDLE_H, WHITE);
  vFillRect(PONG_PADDLE_R_X, pongPaddleRightY, PONG_PADDLE_W, PONG_PADDLE_H, WHITE);

  // Pallina
  vFillRect(pongBallX, pongBallY, PONG_BALL_SIZE, PONG_BALL_SIZE, WHITE);

  // Punteggio (solo se cambiato)
  if (pongScoreLeft != pongPrevScoreLeft || pongScoreRight != pongPrevScoreRight) {
    vFillRect(200, 8, 60, 40, BLACK);
    vFillRect(700, 8, 60, 40, BLACK);

    char scoreStr[4];
    gfx->setFont(u8g2_font_maniac_te);
    gfx->setTextColor(WHITE);

    snprintf(scoreStr, sizeof(scoreStr), "%d", pongScoreLeft);
    vSetCursor(220, 40);
    vPrint(scoreStr);

    snprintf(scoreStr, sizeof(scoreStr), "%d", pongScoreRight);
    vSetCursor(720, 40);
    vPrint(scoreStr);
  }

  // Salva posizioni per prossimo frame
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

void drawPongGameOver() {
  #ifdef EFFECT_DUAL_DISPLAY
  if (!isDualDisplayActive()) return;

  vFillRect(330, 180, 300, 120, BLACK);
  vDrawRect(330, 180, 300, 120, WHITE);

  gfx->setFont(u8g2_font_maniac_te);
  gfx->setTextColor(YELLOW);

  if (pongScoreLeft >= PONG_MAX_SCORE) {
    vSetCursor(365, 228);
    vPrint("P1 VINCE!");
  } else {
    vSetCursor(365, 228);
    vPrint("P2 VINCE!");
  }

  gfx->setFont(u8g2_font_helvB14_te);
  gfx->setTextColor(WHITE);
  vSetCursor(350, 275);
  vPrint("Tocca per ricominciare");
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
  // Bypassa DualGFX proxy per evitare doppio offset coordinate
  pongBypassProxy();

  // === SLAVE: solo disegno con throttle ===
  if (isDualSlave()) {
    if (!pongInitialized) {
      vFillScreen(BLACK);
      pongInitialized = true;
      pongLastUpdate = millis();
      pongCenterlineDrawn = false;
      pongPrevBallX = pongBallX;
      pongPrevBallY = pongBallY;
      pongPrevPaddleLeftY = pongPaddleLeftY;
      pongPrevPaddleRightY = pongPaddleRightY;
      pongPrevScoreLeft = 255;
      pongPrevScoreRight = 255;
    }
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

  // Collisione racchetta sinistra
  if (pongBallVX < 0 &&
      pongBallX <= PONG_PADDLE_L_X + PONG_PADDLE_W &&
      pongBallX + PONG_BALL_SIZE >= PONG_PADDLE_L_X &&
      pongBallY + PONG_BALL_SIZE >= pongPaddleLeftY &&
      pongBallY <= pongPaddleLeftY + PONG_PADDLE_H) {
    pongBallX = PONG_PADDLE_L_X + PONG_PADDLE_W;
    pongBallVX = -pongBallVX;
    int hitPos = (pongBallY + PONG_BALL_SIZE / 2) - (pongPaddleLeftY + PONG_PADDLE_H / 2);
    pongBallVY = hitPos / 8;
    if (pongBallVY == 0) pongBallVY = (random(2) == 0) ? 2 : -2;
  }

  // Collisione racchetta destra
  if (pongBallVX > 0 &&
      pongBallX + PONG_BALL_SIZE >= PONG_PADDLE_R_X &&
      pongBallX <= PONG_PADDLE_R_X + PONG_PADDLE_W &&
      pongBallY + PONG_BALL_SIZE >= pongPaddleRightY &&
      pongBallY <= pongPaddleRightY + PONG_PADDLE_H) {
    pongBallX = PONG_PADDLE_R_X - PONG_BALL_SIZE;
    pongBallVX = -pongBallVX;
    int hitPos = (pongBallY + PONG_BALL_SIZE / 2) - (pongPaddleRightY + PONG_PADDLE_H / 2);
    pongBallVY = hitPos / 8;
    if (pongBallVY == 0) pongBallVY = (random(2) == 0) ? 2 : -2;
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
