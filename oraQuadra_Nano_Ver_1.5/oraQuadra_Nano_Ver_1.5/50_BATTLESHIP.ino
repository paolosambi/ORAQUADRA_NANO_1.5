// ============================================================
// 50_BATTLESHIP.ino - Battaglia Navale per Dual Display
// ============================================================
// Gioco classico Battleship per 2 giocatori su pannelli ESP-NOW.
// Ogni pannello e' una POSTAZIONE A TUTTO SCHERMO separata.
// - Tuo turno:  griglia ATTACCO full-screen (scegli bersaglio)
// - Turno avversario: griglia DIFESA full-screen (vedi la tua flotta)
// Il Master gestisce tutta la logica di gioco.
// ============================================================

#ifdef EFFECT_BATTLESHIP

// ==================== COSTANTI ====================
#define BN_PHASE_P1_PLACE  0
#define BN_PHASE_P2_PLACE  1
#define BN_PHASE_P1_TURN   2
#define BN_PHASE_P2_TURN   3
#define BN_PHASE_GAMEOVER  4

#define BN_NUM_SHIPS       5
#define BN_GRID_SIZE       10

// Colori RGB565
#define BN_COL_BG          0x0013  // Navy scuro
#define BN_COL_WATER       0x0019  // Blu scuro
#define BN_COL_SHIP        0x8410  // Grigio
#define BN_COL_HIT         0xF800  // Rosso
#define BN_COL_MISS        0xBDF7  // Bianco pallido
#define BN_COL_SUNK        0x8000  // Rosso scuro
#define BN_COL_CURSOR      0xFFE0  // Giallo
#define BN_COL_HEADER      0x07FF  // Ciano
#define BN_COL_FIRE_BTN    0xF800  // Rosso bottone FUOCO
#define BN_COL_FIRE_OFF    0x4208  // Grigio bottone disabilitato
#define BN_COL_ROTATE_BTN  0x04BF  // Azzurro bottone RUOTA
#define BN_COL_RANDOM_BTN  0x07E0  // Verde bottone RANDOM

// Griglia full-screen (usata per piazzamento E battaglia)
// Celle 40px + 1px gap = 409px totali, centrate in 480px
#define BN_CELL            40
#define BN_GAP             1
#define BN_GRID_PX         ((BN_CELL + BN_GAP) * BN_GRID_SIZE - BN_GAP)  // 409
#define BN_OX              ((480 - BN_GRID_PX) / 2)  // ~35
#define BN_OY              44
#define BN_FOOTER_Y        456

// Dimensione navi
static const uint8_t bnShipSizes[BN_NUM_SHIPS] = {5, 4, 3, 3, 2};
static const char* bnShipNames[BN_NUM_SHIPS] = {
  "Portaerei", "Corazzata", "Incrociatore", "Sottomarino", "Cacciatorped."
};

// ==================== VARIABILI GLOBALI ====================
bool battleshipInitialized = false;

uint8_t bnPhase = BN_PHASE_P1_PLACE;
uint8_t bnP1Grid[BN_GRID_SIZE][BN_GRID_SIZE];      // Navi P1 (shipID 1-5, 0=acqua)
uint8_t bnP2Grid[BN_GRID_SIZE][BN_GRID_SIZE];      // Navi P2
uint8_t bnP1ShotsOnP2[BN_GRID_SIZE][BN_GRID_SIZE]; // Tiri P1 su griglia P2 (0-3)
uint8_t bnP2ShotsOnP1[BN_GRID_SIZE][BN_GRID_SIZE]; // Tiri P2 su griglia P1 (0-3)

uint8_t bnPlaceShipIdx = 0;   // Nave attuale nel piazzamento (0-4, 0xFF=done)
uint8_t bnPlaceRotation = 0;  // 0=orizzontale, 1=verticale
int8_t  bnCursorR = -1;       // Cursore attacco (-1 = nessuno)
int8_t  bnCursorC = -1;
uint8_t bnP1ShipsSunkMask = 0;
uint8_t bnP2ShipsSunkMask = 0;
uint8_t bnWinner = 0;         // 0=nessuno, 1=P1, 2=P2
uint8_t bnAnimFrame = 0;
bool    bnNeedsRedraw = true;
uint8_t bnLastShotR = 0xFF;
uint8_t bnLastShotC = 0xFF;
uint8_t bnLastShotResult = 0; // 0=none, 1=miss, 2=hit, 3=sunk
unsigned long bnLastAnimTime = 0;
bool    bnShotAnimActive = false;
unsigned long bnShotAnimStart = 0;
uint8_t bnPendingPhase = 0xFF;  // Fase pendente dopo animazione tiro (0xFF = nessuna)

// ==================== INIT ====================
void initBattleship() {
  memset(bnP1Grid, 0, sizeof(bnP1Grid));
  memset(bnP2Grid, 0, sizeof(bnP2Grid));
  memset(bnP1ShotsOnP2, 0, sizeof(bnP1ShotsOnP2));
  memset(bnP2ShotsOnP1, 0, sizeof(bnP2ShotsOnP1));
  bnPhase = BN_PHASE_P1_PLACE;
  bnPlaceShipIdx = 0;
  bnPlaceRotation = 0;
  bnCursorR = -1;
  bnCursorC = -1;
  bnP1ShipsSunkMask = 0;
  bnP2ShipsSunkMask = 0;
  bnWinner = 0;
  bnAnimFrame = 0;
  bnNeedsRedraw = true;
  bnLastShotR = 0xFF;
  bnLastShotC = 0xFF;
  bnLastShotResult = 0;
  bnShotAnimActive = false;
  bnPendingPhase = 0xFF;

  // BATTLESHIP usa display 480x480 indipendenti, NON canvas virtuale.
  // Disabilita DualGFX proxy per questo modo (bypass = gfx punta a realGfx)
#ifdef EFFECT_DUAL_DISPLAY
  extern uint64_t dualModesMask;
  dualModesMask &= ~(1ULL << MODE_BATTLESHIP);
#endif

  battleshipInitialized = true;
  Serial.println("[BATTLESHIP] Inizializzato");
}

// ==================== LOGICA POSIZIONAMENTO ====================
bool bnCanPlaceShip(uint8_t grid[][BN_GRID_SIZE], int r, int c, int size, int rot) {
  for (int i = 0; i < size; i++) {
    int rr = r + (rot == 1 ? i : 0);
    int cc = c + (rot == 0 ? i : 0);
    if (rr < 0 || rr >= BN_GRID_SIZE || cc < 0 || cc >= BN_GRID_SIZE) return false;
    if (grid[rr][cc] != 0) return false;
  }
  return true;
}

void bnPlaceShip(uint8_t grid[][BN_GRID_SIZE], int r, int c, int shipIdx, int rot) {
  uint8_t shipId = shipIdx + 1;
  int size = bnShipSizes[shipIdx];
  for (int i = 0; i < size; i++) {
    int rr = r + (rot == 1 ? i : 0);
    int cc = c + (rot == 0 ? i : 0);
    grid[rr][cc] = shipId;
  }
}

void bnRandomPlacement(uint8_t grid[][BN_GRID_SIZE]) {
  memset(grid, 0, BN_GRID_SIZE * BN_GRID_SIZE);
  for (int s = 0; s < BN_NUM_SHIPS; s++) {
    int size = bnShipSizes[s];
    int attempts = 0;
    while (attempts < 200) {
      int rot = random(0, 2);
      int r = random(0, BN_GRID_SIZE);
      int c = random(0, BN_GRID_SIZE);
      if (bnCanPlaceShip(grid, r, c, size, rot)) {
        bnPlaceShip(grid, r, c, s, rot);
        break;
      }
      attempts++;
    }
  }
}

// ==================== LOGICA TIRO ====================
bool bnIsShipSunk(uint8_t target[][BN_GRID_SIZE], uint8_t shots[][BN_GRID_SIZE], uint8_t shipId) {
  for (int r = 0; r < BN_GRID_SIZE; r++) {
    for (int c = 0; c < BN_GRID_SIZE; c++) {
      if (target[r][c] == shipId && shots[r][c] < 2) return false;
    }
  }
  return true;
}

void bnMarkSunk(uint8_t target[][BN_GRID_SIZE], uint8_t shots[][BN_GRID_SIZE], uint8_t shipId) {
  for (int r = 0; r < BN_GRID_SIZE; r++) {
    for (int c = 0; c < BN_GRID_SIZE; c++) {
      if (target[r][c] == shipId) {
        shots[r][c] = 3;
      }
    }
  }
}

// Restituisce 1=miss, 2=hit, 3=sunk
uint8_t bnFireShot(uint8_t shots[][BN_GRID_SIZE], uint8_t target[][BN_GRID_SIZE],
                   int r, int c, uint8_t* sunkMask) {
  if (r < 0 || r >= BN_GRID_SIZE || c < 0 || c >= BN_GRID_SIZE) return 0;
  if (shots[r][c] != 0) return 0;

  if (target[r][c] == 0) {
    shots[r][c] = 1;
    return 1;
  }

  uint8_t shipId = target[r][c];
  shots[r][c] = 2;

  if (bnIsShipSunk(target, shots, shipId)) {
    bnMarkSunk(target, shots, shipId);
    *sunkMask |= (1 << (shipId - 1));
    return 3;
  }
  return 2;
}

bool bnAllShipsSunk(uint8_t sunkMask) {
  return sunkMask == 0x1F;
}

// ==================== DISEGNO HELPER ====================
uint16_t bnShotColor(uint8_t shotVal) {
  switch (shotVal) {
    case 1: return BN_COL_MISS;
    case 2: return BN_COL_HIT;
    case 3: return BN_COL_SUNK;
    default: return BN_COL_WATER;
  }
}

void bnDrawGridLabels() {
  gfx->setTextColor(0x8410);
  gfx->setTextSize(1);
  for (int c = 0; c < BN_GRID_SIZE; c++) {
    int px = BN_OX + c * (BN_CELL + BN_GAP) + BN_CELL / 2 - 3;
    gfx->setCursor(px, BN_OY - 10);
    gfx->print(c);
  }
  for (int r = 0; r < BN_GRID_SIZE; r++) {
    int py = BN_OY + r * (BN_CELL + BN_GAP) + BN_CELL / 2 - 4;
    gfx->setCursor(BN_OX - 14, py);
    gfx->print((char)('A' + r));
  }
}

// Disegna header con titolo e stato navi
void bnDrawHeader(const char* title, uint8_t sunkMask, bool showSunk) {
  gfx->setTextColor(BN_COL_HEADER);
  gfx->setTextSize(2);
  gfx->setCursor(40, 4);
  gfx->print(title);

  if (showSunk) {
    // Icone navi: quadratini per navi ancora a galla, X per affondate
    gfx->setTextSize(1);
    gfx->setCursor(40, 26);
    gfx->setTextColor(0x8410);
    gfx->print("Flotta: ");
    for (int i = 0; i < BN_NUM_SHIPS; i++) {
      if (sunkMask & (1 << i)) {
        gfx->setTextColor(BN_COL_HIT);
        gfx->print("X ");
      } else {
        gfx->setTextColor(BN_COL_SHIP);
        gfx->print("# ");
      }
    }
  }
}

// ==================== SCHERMATE ====================

// Schermata standalone (senza dual display)
void drawBattleshipStandalone() {
  gfx->fillScreen(BN_COL_BG);
  gfx->setTextColor(BN_COL_HEADER);
  gfx->setTextSize(2);
  gfx->setCursor(60, 180);
  gfx->print("BATTAGLIA NAVALE");
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(50, 230);
  gfx->print("Attiva Multi-Display per giocare");
  gfx->setCursor(55, 250);
  gfx->print("a BATTAGLIA NAVALE!");
  gfx->setCursor(100, 290);
  gfx->print("Configura da /dualdisplay");
}

// Schermata di attesa
void drawBattleshipWaiting(const char* msg) {
  gfx->fillScreen(BN_COL_BG);
  gfx->setTextColor(BN_COL_HEADER);
  gfx->setTextSize(2);
  gfx->setCursor(80, 200);
  gfx->print("ATTENDI...");
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(100, 240);
  gfx->print(msg);
  int dots = (millis() / 500) % 4;
  gfx->setCursor(100, 270);
  for (int i = 0; i < dots; i++) gfx->print(". ");
}

// ==================== PIAZZAMENTO FULL-SCREEN ====================
void drawBattleshipPlacement(uint8_t grid[][BN_GRID_SIZE], bool isP2) {
  gfx->fillScreen(BN_COL_BG);

  // Header
  gfx->setTextColor(BN_COL_HEADER);
  gfx->setTextSize(2);
  gfx->setCursor(40, 4);
  gfx->print(isP2 ? "P2 - POSIZIONA NAVI" : "P1 - POSIZIONA NAVI");

  // Nome nave corrente
  if (bnPlaceShipIdx < BN_NUM_SHIPS) {
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(BN_OX, BN_OY - 10);
    gfx->print(bnShipNames[bnPlaceShipIdx]);
    gfx->print(" (");
    gfx->print(bnShipSizes[bnPlaceShipIdx]);
    gfx->print(") ");
    gfx->print(bnPlaceRotation == 0 ? "[HORIZ]" : "[VERT]");
  } else {
    gfx->setTextColor(0x07E0); // Verde
    gfx->setTextSize(1);
    gfx->setCursor(BN_OX, BN_OY - 10);
    gfx->print("Tutte le navi posizionate!");
  }

  // Griglia 10x10 full-screen
  for (int r = 0; r < BN_GRID_SIZE; r++) {
    for (int c = 0; c < BN_GRID_SIZE; c++) {
      int px = BN_OX + c * (BN_CELL + BN_GAP);
      int py = BN_OY + r * (BN_CELL + BN_GAP);
      uint16_t col = (grid[r][c] > 0) ? BN_COL_SHIP : BN_COL_WATER;
      gfx->fillRect(px, py, BN_CELL, BN_CELL, col);
    }
  }

  bnDrawGridLabels();

  // Footer
  if (bnPlaceShipIdx < BN_NUM_SHIPS) {
    // RUOTA (sinistra)
    gfx->fillRoundRect(20, BN_FOOTER_Y, 130, 22, 5, BN_COL_ROTATE_BTN);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(42, BN_FOOTER_Y + 3);
    gfx->print("RUOTA");

    // RANDOM (destra)
    gfx->fillRoundRect(330, BN_FOOTER_Y, 130, 22, 5, BN_COL_RANDOM_BTN);
    gfx->setTextColor(BLACK);
    gfx->setTextSize(2);
    gfx->setCursor(342, BN_FOOTER_Y + 3);
    gfx->print("RANDOM");
  } else {
    // PRONTO!
    gfx->fillRoundRect(130, BN_FOOTER_Y, 220, 22, 5, BN_COL_HEADER);
    gfx->setTextColor(BLACK);
    gfx->setTextSize(2);
    gfx->setCursor(178, BN_FOOTER_Y + 3);
    gfx->print("PRONTO!");
  }
}

// ==================== ATTACCO FULL-SCREEN ====================
// Griglia attacco: celle 40px, colpi sovrapposti, cursore giallo
void drawBattleshipAttack(uint8_t shots[][BN_GRID_SIZE], int8_t curR, int8_t curC,
                           uint8_t enemySunkMask) {
  gfx->fillScreen(BN_COL_BG);

  // Header
  bnDrawHeader("TUO TURNO - ATTACCO", enemySunkMask, true);

  // Indicatore turno e coordinata cursore
  gfx->setTextSize(1);
  if (curR >= 0 && curC >= 0) {
    gfx->setTextColor(BN_COL_CURSOR);
    gfx->setCursor(370, 6);
    gfx->print("BERSAGLIO:");
    gfx->setCursor(370, 18);
    gfx->print((char)('A' + curR));
    gfx->print(curC);
    gfx->print(" -> FUOCO!");
  } else {
    gfx->setTextColor(BN_COL_CURSOR);
    gfx->setCursor(370, 6);
    gfx->print("SCEGLI CELLA");
    gfx->setCursor(370, 18);
    gfx->print("E PREMI FUOCO");
  }

  // Griglia 10x10 full-screen
  for (int r = 0; r < BN_GRID_SIZE; r++) {
    for (int c = 0; c < BN_GRID_SIZE; c++) {
      int px = BN_OX + c * (BN_CELL + BN_GAP);
      int py = BN_OY + r * (BN_CELL + BN_GAP);

      // Colore base
      uint16_t col = BN_COL_WATER;
      if (shots[r][c] > 0) col = bnShotColor(shots[r][c]);
      gfx->fillRect(px, py, BN_CELL, BN_CELL, col);

      // Cursore: bordo giallo spesso
      if (r == curR && c == curC) {
        gfx->drawRect(px, py, BN_CELL, BN_CELL, BN_COL_CURSOR);
        gfx->drawRect(px + 1, py + 1, BN_CELL - 2, BN_CELL - 2, BN_COL_CURSOR);
        gfx->drawRect(px + 2, py + 2, BN_CELL - 4, BN_CELL - 4, BN_COL_CURSOR);
      }

      // Evidenzia ultimo tiro durante animazione (bordo bianco lampeggiante)
      if (bnShotAnimActive && r == bnLastShotR && c == bnLastShotC) {
        uint16_t hlCol = ((millis() / 300) % 2) ? WHITE : BN_COL_CURSOR;
        gfx->drawRect(px, py, BN_CELL, BN_CELL, hlCol);
        gfx->drawRect(px + 1, py + 1, BN_CELL - 2, BN_CELL - 2, hlCol);
      }

      // Segni tiro
      if (shots[r][c] == 1) {
        // Miss: pallino bianco
        gfx->fillCircle(px + BN_CELL / 2, py + BN_CELL / 2, 6, BN_COL_MISS);
      } else if (shots[r][c] == 2) {
        // Hit: X rossa grossa
        for (int t = 0; t < 3; t++) {
          gfx->drawLine(px + 4 + t, py + 4, px + BN_CELL - 5 + t, py + BN_CELL - 5, BN_COL_HIT);
          gfx->drawLine(px + 4 + t, py + BN_CELL - 5, px + BN_CELL - 5 + t, py + 4, BN_COL_HIT);
        }
      } else if (shots[r][c] == 3) {
        // Sunk: X rossa + bordo
        for (int t = 0; t < 3; t++) {
          gfx->drawLine(px + 3 + t, py + 3, px + BN_CELL - 4 + t, py + BN_CELL - 4, 0xF800);
          gfx->drawLine(px + 3 + t, py + BN_CELL - 4, px + BN_CELL - 4 + t, py + 3, 0xF800);
        }
        gfx->drawRect(px + 1, py + 1, BN_CELL - 2, BN_CELL - 2, BN_COL_SUNK);
      }
    }
  }

  bnDrawGridLabels();

  // Bottone FUOCO in basso
  bool ready = (curR >= 0 && curC >= 0);
  uint16_t btnCol = ready ? BN_COL_FIRE_BTN : BN_COL_FIRE_OFF;
  gfx->fillRoundRect(130, BN_FOOTER_Y, 220, 22, 5, btnCol);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(190, BN_FOOTER_Y + 3);
  gfx->print("FUOCO!");

  // Animazione risultato tiro
  if (bnLastShotResult > 0 && bnShotAnimActive) {
    // Overlay semi-trasparente al centro
    int boxW = 280, boxH = 50;
    int boxX = (480 - boxW) / 2, boxY = 200;
    gfx->fillRoundRect(boxX, boxY, boxW, boxH, 8, BN_COL_BG);
    gfx->drawRoundRect(boxX, boxY, boxW, boxH, 8, BN_COL_HEADER);
    gfx->setTextSize(3);
    if (bnLastShotResult == 1) {
      gfx->setTextColor(BN_COL_MISS);
      gfx->setCursor(boxX + 50, boxY + 12);
      gfx->print("MANCATO!");
    } else if (bnLastShotResult == 2) {
      gfx->setTextColor(BN_COL_HIT);
      gfx->setCursor(boxX + 45, boxY + 12);
      gfx->print("COLPITO!");
    } else if (bnLastShotResult == 3) {
      gfx->setTextColor(0xF800);
      gfx->setCursor(boxX + 22, boxY + 12);
      gfx->print("AFFONDATO!");
    }
  }
}

// ==================== DIFESA FULL-SCREEN ====================
// Griglia difesa: le tue navi + tiri nemici, visibile durante turno avversario
void drawBattleshipDefense(uint8_t myGrid[][BN_GRID_SIZE], uint8_t enemyShots[][BN_GRID_SIZE],
                            uint8_t mySunkMask) {
  gfx->fillScreen(BN_COL_BG);

  // Header
  bnDrawHeader("TURNO AVVERSARIO", mySunkMask, true);

  gfx->setTextColor(0x8410);
  gfx->setTextSize(1);
  gfx->setCursor(370, 6);
  gfx->print("ATTENDI IL");
  gfx->setCursor(370, 18);
  gfx->print("TUO TURNO...");

  // Label
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(BN_OX, BN_OY - 10);
  gfx->print("LA TUA FLOTTA");

  // Griglia 10x10 full-screen
  for (int r = 0; r < BN_GRID_SIZE; r++) {
    for (int c = 0; c < BN_GRID_SIZE; c++) {
      int px = BN_OX + c * (BN_CELL + BN_GAP);
      int py = BN_OY + r * (BN_CELL + BN_GAP);

      uint16_t col = BN_COL_WATER;
      if (enemyShots[r][c] == 3) {
        col = BN_COL_SUNK;
      } else if (enemyShots[r][c] == 2) {
        col = BN_COL_HIT;
      } else if (enemyShots[r][c] == 1) {
        col = BN_COL_MISS;
      } else if (myGrid[r][c] > 0) {
        col = BN_COL_SHIP;
      }
      gfx->fillRect(px, py, BN_CELL, BN_CELL, col);

      // Evidenzia ultimo tiro nemico durante animazione
      if (bnShotAnimActive && r == bnLastShotR && c == bnLastShotC) {
        uint16_t hlCol = ((millis() / 300) % 2) ? WHITE : BN_COL_CURSOR;
        gfx->drawRect(px, py, BN_CELL, BN_CELL, hlCol);
        gfx->drawRect(px + 1, py + 1, BN_CELL - 2, BN_CELL - 2, hlCol);
      }

      // Segno tiro nemico sulle tue navi
      if (enemyShots[r][c] == 1) {
        gfx->fillCircle(px + BN_CELL / 2, py + BN_CELL / 2, 6, BN_COL_MISS);
      } else if (enemyShots[r][c] == 2) {
        for (int t = 0; t < 3; t++) {
          gfx->drawLine(px + 4 + t, py + 4, px + BN_CELL - 5 + t, py + BN_CELL - 5, 0xF800);
          gfx->drawLine(px + 4 + t, py + BN_CELL - 5, px + BN_CELL - 5 + t, py + 4, 0xF800);
        }
      } else if (enemyShots[r][c] == 3) {
        for (int t = 0; t < 3; t++) {
          gfx->drawLine(px + 3 + t, py + 3, px + BN_CELL - 4 + t, py + BN_CELL - 4, 0xF800);
          gfx->drawLine(px + 3 + t, py + BN_CELL - 4, px + BN_CELL - 4 + t, py + 3, 0xF800);
        }
        gfx->drawRect(px + 1, py + 1, BN_CELL - 2, BN_CELL - 2, BN_COL_SUNK);
      }
    }
  }

  bnDrawGridLabels();

  // Animazione risultato tiro nemico (il nemico ha sparato su di te)
  if (bnLastShotResult > 0 && bnShotAnimActive) {
    int boxW = 280, boxH = 50;
    int boxX = (480 - boxW) / 2, boxY = 200;
    gfx->fillRoundRect(boxX, boxY, boxW, boxH, 8, BN_COL_BG);
    gfx->drawRoundRect(boxX, boxY, boxW, boxH, 8, BN_COL_HIT);
    gfx->setTextSize(2);
    gfx->setTextColor(BN_COL_HIT);
    gfx->setCursor(boxX + 30, boxY + 15);
    if (bnLastShotResult == 1) {
      gfx->print("NEMICO HA MANCATO");
    } else if (bnLastShotResult == 2) {
      gfx->print("TI HANNO COLPITO!");
    } else if (bnLastShotResult == 3) {
      gfx->setTextColor(0xF800);
      gfx->print("NAVE AFFONDATA!");
    }
  }
}

// ==================== GAME OVER ====================
void drawBattleshipGameOver() {
  gfx->fillScreen(BN_COL_BG);

  gfx->setTextColor(BN_COL_HEADER);
  gfx->setTextSize(3);
  gfx->setCursor(50, 120);
  gfx->print("GAME OVER!");

  bool iAmP2 = false;
#ifdef EFFECT_DUAL_DISPLAY
  iAmP2 = isDualSlave();
#endif

  gfx->setTextSize(3);
  gfx->setCursor(40, 200);
  if ((bnWinner == 1 && !iAmP2) || (bnWinner == 2 && iAmP2)) {
    gfx->setTextColor(0x07E0); // Verde
    gfx->print("HAI VINTO!");
  } else {
    gfx->setTextColor(BN_COL_HIT); // Rosso
    gfx->print("HAI PERSO!");
  }

  gfx->setTextSize(2);
  gfx->setTextColor(WHITE);
  gfx->setCursor(80, 280);
  if (bnWinner == 1) {
    gfx->print("Vittoria Player 1");
  } else {
    gfx->print("Vittoria Player 2");
  }

  gfx->setCursor(90, 370);
  gfx->setTextColor(0x8410);
  gfx->setTextSize(2);
  gfx->print("Tocca per giocare");
}

// ==================== TOUCH PIAZZAMENTO ====================
void handleBattleshipPlacementTouch(int x, int y, bool isP2) {
  uint8_t (*grid)[BN_GRID_SIZE] = isP2 ? bnP2Grid : bnP1Grid;

  // Bottone RUOTA (20, FOOTER_Y, 130, 22)
  if (y >= BN_FOOTER_Y && y <= BN_FOOTER_Y + 22 && x >= 20 && x <= 150) {
    if (bnPlaceShipIdx < BN_NUM_SHIPS) {
      bnPlaceRotation = 1 - bnPlaceRotation;
      bnNeedsRedraw = true;
      Serial.println("[BATTLESHIP] Rotazione: " + String(bnPlaceRotation));
    }
    return;
  }

  // Bottone RANDOM (330, FOOTER_Y, 130, 22)
  if (y >= BN_FOOTER_Y && y <= BN_FOOTER_Y + 22 && x >= 330 && x <= 460) {
    if (bnPlaceShipIdx < BN_NUM_SHIPS) {
      bnRandomPlacement(grid);
      bnPlaceShipIdx = BN_NUM_SHIPS;
      bnNeedsRedraw = true;
      Serial.println("[BATTLESHIP] Piazzamento random");
    }
    return;
  }

  // Bottone PRONTO (130, FOOTER_Y, 220, 22)
  if (bnPlaceShipIdx >= BN_NUM_SHIPS) {
    if (y >= BN_FOOTER_Y && y <= BN_FOOTER_Y + 22 && x >= 130 && x <= 350) {
      if (bnPhase == BN_PHASE_P1_PLACE) {
        bnPhase = BN_PHASE_P2_PLACE;
        bnPlaceShipIdx = 0;
        bnPlaceRotation = 0;
        Serial.println("[BATTLESHIP] P1 pronto -> P2 piazza");
      } else if (bnPhase == BN_PHASE_P2_PLACE) {
        bnPhase = BN_PHASE_P1_TURN;
        bnCursorR = -1;
        bnCursorC = -1;
        Serial.println("[BATTLESHIP] P2 pronto -> Inizia battaglia!");
      }
      bnNeedsRedraw = true;
    }
    return;
  }

  // Tap su griglia -> piazza nave
  int gridEndX = BN_OX + BN_GRID_PX;
  int gridEndY = BN_OY + BN_GRID_PX;
  if (x >= BN_OX && x < gridEndX && y >= BN_OY && y < gridEndY) {
    int c = (x - BN_OX) / (BN_CELL + BN_GAP);
    int r = (y - BN_OY) / (BN_CELL + BN_GAP);
    if (r >= 0 && r < BN_GRID_SIZE && c >= 0 && c < BN_GRID_SIZE) {
      int size = bnShipSizes[bnPlaceShipIdx];
      if (bnCanPlaceShip(grid, r, c, size, bnPlaceRotation)) {
        bnPlaceShip(grid, r, c, bnPlaceShipIdx, bnPlaceRotation);
        Serial.printf("[BATTLESHIP] Nave %d piazzata a (%d,%d) rot=%d\n",
                      bnPlaceShipIdx + 1, r, c, bnPlaceRotation);
        bnPlaceShipIdx++;
        bnNeedsRedraw = true;
      }
    }
  }
}

// ==================== TOUCH BATTAGLIA ====================
void handleBattleshipBattleTouch(int x, int y, bool isP2) {
  bool myTurn = isP2 ? (bnPhase == BN_PHASE_P2_TURN) : (bnPhase == BN_PHASE_P1_TURN);
  if (!myTurn) return;
  if (bnShotAnimActive) return;  // Blocca touch durante animazione risultato

  // Tap su griglia attacco full-screen
  int gridEndX = BN_OX + BN_GRID_PX;
  int gridEndY = BN_OY + BN_GRID_PX;
  if (x >= BN_OX && x < gridEndX && y >= BN_OY && y < gridEndY) {
    int c = (x - BN_OX) / (BN_CELL + BN_GAP);
    int r = (y - BN_OY) / (BN_CELL + BN_GAP);
    if (r >= 0 && r < BN_GRID_SIZE && c >= 0 && c < BN_GRID_SIZE) {
      uint8_t (*shots)[BN_GRID_SIZE] = isP2 ? bnP2ShotsOnP1 : bnP1ShotsOnP2;
      if (shots[r][c] == 0) {
        bnCursorR = r;
        bnCursorC = c;
        bnNeedsRedraw = true;
        Serial.printf("[BATTLESHIP] %s cursore -> cella (%c%d) da pixel (%d,%d)\n",
                      isP2 ? "P2" : "P1", 'A' + r, c, x, y);
      }
    }
    return;
  }

  // Tap su bottone FUOCO (130, FOOTER_Y, 220, 22)
  if (y >= BN_FOOTER_Y && y <= BN_FOOTER_Y + 22 && x >= 130 && x <= 350) {
    if (bnCursorR >= 0 && bnCursorC >= 0) {
      uint8_t result;
      if (isP2) {
        result = bnFireShot(bnP2ShotsOnP1, bnP1Grid, bnCursorR, bnCursorC, &bnP1ShipsSunkMask);
      } else {
        result = bnFireShot(bnP1ShotsOnP2, bnP2Grid, bnCursorR, bnCursorC, &bnP2ShipsSunkMask);
      }

      if (result > 0) {
        bnLastShotR = bnCursorR;
        bnLastShotC = bnCursorC;
        bnLastShotResult = result;
        bnShotAnimActive = true;
        bnShotAnimStart = millis();

        Serial.printf("[BATTLESHIP] %s spara (%d,%d) -> %s\n",
                      isP2 ? "P2" : "P1", bnCursorR, bnCursorC,
                      result == 1 ? "MISS" : (result == 2 ? "HIT" : "SUNK"));

        // NON cambiare fase subito! Salva la fase pendente.
        // La fase cambiera' dopo l'animazione (2 sec) cosi' il giocatore
        // vede dove ha sparato sulla griglia ATTACCO prima del cambio schermo.
        if (isP2 && bnAllShipsSunk(bnP1ShipsSunkMask)) {
          bnWinner = 2;
          bnPendingPhase = BN_PHASE_GAMEOVER;
          Serial.println("[BATTLESHIP] P2 VINCE!");
        } else if (!isP2 && bnAllShipsSunk(bnP2ShipsSunkMask)) {
          bnWinner = 1;
          bnPendingPhase = BN_PHASE_GAMEOVER;
          Serial.println("[BATTLESHIP] P1 VINCE!");
        } else {
          bnPendingPhase = isP2 ? BN_PHASE_P1_TURN : BN_PHASE_P2_TURN;
        }

        bnCursorR = -1;
        bnCursorC = -1;
        bnNeedsRedraw = true;
      }
    }
    return;
  }
}

// ==================== TOUCH HANDLERS ====================
void handleBattleshipTouch(int x, int y) {
#ifdef EFFECT_DUAL_DISPLAY
  if (isDualSlave()) return;
#endif

  // Exit: angolo alto-sx
  if (x < 80 && y < 80) {
    extern void handleModeChange();
    handleModeChange();
    return;
  }

  switch (bnPhase) {
    case BN_PHASE_P1_PLACE:
      handleBattleshipPlacementTouch(x, y, false);
      break;
    case BN_PHASE_P1_TURN:
      handleBattleshipBattleTouch(x, y, false);
      break;
    case BN_PHASE_GAMEOVER:
      initBattleship();
      break;
    default:
      break;
  }
}

void handleBattleshipRemoteTouch(int x, int y) {
  Serial.printf("[BATTLESHIP] Remote touch ricevuto: pixel (%d,%d) fase=%d\n", x, y, bnPhase);
  switch (bnPhase) {
    case BN_PHASE_P2_PLACE:
      handleBattleshipPlacementTouch(x, y, true);
      break;
    case BN_PHASE_P2_TURN:
      handleBattleshipBattleTouch(x, y, true);
      break;
    case BN_PHASE_GAMEOVER:
      initBattleship();
      break;
    default:
      break;
  }
}

// ==================== SYNC PACK/UNPACK ====================
int bnPackSyncData(uint8_t* data, int maxLen) {
  if (maxLen < 163) return 0;
  int off = 0;

  data[off++] = bnPhase;
  data[off++] = bnPlaceShipIdx;
  data[off++] = bnPlaceRotation;
  data[off++] = (uint8_t)bnCursorR;
  data[off++] = (uint8_t)bnCursorC;
  data[off++] = bnP1ShipsSunkMask;
  data[off++] = bnP2ShipsSunkMask;
  data[off++] = bnWinner;
  data[off++] = bnLastShotR;
  data[off++] = bnLastShotC;
  data[off++] = bnLastShotResult;
  data[off++] = bnAnimFrame;
  data[off++] = bnPendingPhase;  // 0xFF = nessun cambio pendente

  // P2 Grid (100 bytes)
  for (int r = 0; r < BN_GRID_SIZE; r++)
    for (int c = 0; c < BN_GRID_SIZE; c++)
      data[off++] = bnP2Grid[r][c];

  // P1 shots on P2 packed (25 bytes, 2 bit/cella)
  for (int i = 0; i < 25; i++) {
    uint8_t packed = 0;
    for (int j = 0; j < 4; j++) {
      int idx = i * 4 + j;
      if (idx < 100) {
        packed |= (bnP1ShotsOnP2[idx / BN_GRID_SIZE][idx % BN_GRID_SIZE] & 0x03) << (j * 2);
      }
    }
    data[off++] = packed;
  }

  // P2 shots on P1 packed (25 bytes, 2 bit/cella)
  for (int i = 0; i < 25; i++) {
    uint8_t packed = 0;
    for (int j = 0; j < 4; j++) {
      int idx = i * 4 + j;
      if (idx < 100) {
        packed |= (bnP2ShotsOnP1[idx / BN_GRID_SIZE][idx % BN_GRID_SIZE] & 0x03) << (j * 2);
      }
    }
    data[off++] = packed;
  }

  return off; // 163
}

void bnUnpackSyncData(const uint8_t* data, int dataLen) {
  if (dataLen < 163) return;

  // Checksum veloce per evitare ridisegno se dati identici
  static uint32_t lastSyncHash = 0;
  uint32_t hash = 0;
  for (int i = 0; i < dataLen; i++) hash = hash * 31 + data[i];
  if (hash == lastSyncHash) return;  // Nessun cambiamento
  lastSyncHash = hash;

  int off = 0;

  bnPhase = data[off++];
  bnPlaceShipIdx = data[off++];
  bnPlaceRotation = data[off++];
  bnCursorR = (int8_t)data[off++];
  bnCursorC = (int8_t)data[off++];
  bnP1ShipsSunkMask = data[off++];
  bnP2ShipsSunkMask = data[off++];
  bnWinner = data[off++];
  bnLastShotR = data[off++];
  bnLastShotC = data[off++];
  uint8_t newResult = data[off++];
  if (newResult != bnLastShotResult && newResult > 0) {
    bnShotAnimActive = true;
    bnShotAnimStart = millis();
  }
  bnLastShotResult = newResult;
  bnAnimFrame = data[off++];
  bnPendingPhase = data[off++];

  // P2 Grid (100 bytes)
  for (int r = 0; r < BN_GRID_SIZE; r++)
    for (int c = 0; c < BN_GRID_SIZE; c++)
      bnP2Grid[r][c] = data[off++];

  // P1 shots on P2 packed (25 bytes)
  for (int i = 0; i < 25; i++) {
    uint8_t packed = data[off++];
    for (int j = 0; j < 4; j++) {
      int idx = i * 4 + j;
      if (idx < 100) {
        bnP1ShotsOnP2[idx / BN_GRID_SIZE][idx % BN_GRID_SIZE] = (packed >> (j * 2)) & 0x03;
      }
    }
  }

  // P2 shots on P1 packed (25 bytes)
  for (int i = 0; i < 25; i++) {
    uint8_t packed = data[off++];
    for (int j = 0; j < 4; j++) {
      int idx = i * 4 + j;
      if (idx < 100) {
        bnP2ShotsOnP1[idx / BN_GRID_SIZE][idx % BN_GRID_SIZE] = (packed >> (j * 2)) & 0x03;
      }
    }
  }

  bnNeedsRedraw = true;  // Hash diverso = dati cambiati = ridisegna
}

// ==================== UPDATE LOOP ====================
void updateBattleship() {
  if (!battleshipInitialized) {
    initBattleship();
  }

  // Forza rendering diretto su display fisico 480x480 (bypass DualGFX proxy)
#ifdef EFFECT_DUAL_DISPLAY
  extern Arduino_RGB_Display* realGfx;
  gfx = realGfx;
#endif

  // Reset font a built-in (il bypass transition imposta u8g2_font_inb21_mr che e' enorme)
  gfx->setFont((const GFXfont*)NULL);
  gfx->setTextSize(1);
  gfx->setTextWrap(false);

  // Animazione risultato tiro (2 sec) + cambio fase ritardato
  if (bnShotAnimActive && millis() - bnShotAnimStart > 2000) {
    bnShotAnimActive = false;
    bnLastShotResult = 0;
    // Applica il cambio fase pendente ORA (dopo che il giocatore ha visto il risultato)
    if (bnPendingPhase != 0xFF) {
      bnPhase = bnPendingPhase;
      bnPendingPhase = 0xFF;
      Serial.printf("[BATTLESHIP] Fase cambiata a %d dopo animazione\n", bnPhase);
    }
    bnNeedsRedraw = true;
  }

  bool dualActive = false;
  bool isSlave = false;
#ifdef EFFECT_DUAL_DISPLAY
  dualActive = isDualDisplayActive();
  isSlave = isDualSlave();
#endif

  if (!dualActive) {
    if (bnNeedsRedraw) {
      drawBattleshipStandalone();
      bnNeedsRedraw = false;
    }
    return;
  }

  // ---- SLAVE (P2) ----
  if (isSlave) {
    if (bnNeedsRedraw) {
      switch (bnPhase) {
        case BN_PHASE_P1_PLACE:
          drawBattleshipWaiting("Player 1 sta posizionando...");
          break;
        case BN_PHASE_P2_PLACE:
          drawBattleshipPlacement(bnP2Grid, true);
          break;
        case BN_PHASE_P2_TURN:
          // MIO turno (sono P2): griglia ATTACCO full-screen
          drawBattleshipAttack(bnP2ShotsOnP1, bnCursorR, bnCursorC, bnP1ShipsSunkMask);
          break;
        case BN_PHASE_P1_TURN:
          // Turno avversario: griglia DIFESA full-screen (le mie navi)
          drawBattleshipDefense(bnP2Grid, bnP1ShotsOnP2, bnP2ShipsSunkMask);
          break;
        case BN_PHASE_GAMEOVER:
          drawBattleshipGameOver();
          break;
      }
      bnNeedsRedraw = false;
    }
    return;
  }

  // ---- MASTER (P1) ----
  if (bnNeedsRedraw) {
    switch (bnPhase) {
      case BN_PHASE_P1_PLACE:
        drawBattleshipPlacement(bnP1Grid, false);
        break;
      case BN_PHASE_P2_PLACE:
        drawBattleshipWaiting("Player 2 sta posizionando...");
        break;
      case BN_PHASE_P1_TURN:
        // MIO turno (sono P1): griglia ATTACCO full-screen
        drawBattleshipAttack(bnP1ShotsOnP2, bnCursorR, bnCursorC, bnP2ShipsSunkMask);
        break;
      case BN_PHASE_P2_TURN:
        // Turno avversario: griglia DIFESA full-screen (le mie navi)
        drawBattleshipDefense(bnP1Grid, bnP2ShotsOnP1, bnP1ShipsSunkMask);
        break;
      case BN_PHASE_GAMEOVER:
        drawBattleshipGameOver();
        break;
    }
    bnNeedsRedraw = false;
  }

  // Ridisegna durante animazione tiro (per lampeggiamento evidenziazione)
  if (bnShotAnimActive) {
    static unsigned long lastAnimRedraw = 0;
    if (millis() - lastAnimRedraw > 300) {
      lastAnimRedraw = millis();
      bnNeedsRedraw = true;
    }
  }

  // Animazione puntini attesa piazzamento P2
  if (bnPhase == BN_PHASE_P2_PLACE) {
    static unsigned long lastDotUpdate = 0;
    if (millis() - lastDotUpdate > 500) {
      lastDotUpdate = millis();
      drawBattleshipWaiting("Player 2 sta posizionando...");
    }
  }
}

#endif // EFFECT_BATTLESHIP
