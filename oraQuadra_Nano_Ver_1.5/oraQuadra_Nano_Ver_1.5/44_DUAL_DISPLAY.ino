// ================== DUAL DISPLAY - SINCRONIZZAZIONE MULTI-PANNELLO ESP-NOW ==================
// Sistema multi-display che permette fino a 4 pannelli OraQuadra Nano in griglia 2x2
// Ogni pannello e' 480x480 pixel, il canvas virtuale e' gridW*480 x gridH*480
// Comunicazione tra pannelli tramite ESP-NOW (bassa latenza, senza WiFi router)
//
// Layout griglia:
// +------+------+
// | 0,0  | 1,0  |  RIGA SUPERIORE
// | TL   | TR   |
// +------+------+
// | 0,1  | 1,1  |  RIGA INFERIORE
// | BL   | BR   |
// +------+------+
//
// Ruoli:
// - STANDALONE (0): Pannello singolo, nessuna sincronizzazione
// - MASTER (1): Invia stato a tutti gli slave ogni ~33ms (30fps)
// - SLAVE (2): Riceve stato dal master e lo applica
//
// Protocollo discovery:
// - Lo slave invia un pacchetto broadcast con magic 0xDC e tipo DISCOVERY_REQUEST
// - Il master risponde con la sua configurazione (MAC, griglia, ecc.)
//
// EEPROM: indirizzi 908-932

#include <esp_now.h>

// ===== EEPROM addresses (908-932) =====
#define EEPROM_DUAL_ENABLED       908
#define EEPROM_DUAL_PANEL_X       909
#define EEPROM_DUAL_PANEL_Y       910
#define EEPROM_DUAL_GRID_W        911
#define EEPROM_DUAL_GRID_H        912
#define EEPROM_DUAL_ROLE          913
#define EEPROM_DUAL_PEER1_MAC     914  // 6 bytes (914-919)
#define EEPROM_DUAL_PEER2_MAC     920  // 6 bytes (920-925)
#define EEPROM_DUAL_PEER3_MAC     926  // 6 bytes (926-931)
#define EEPROM_DUAL_MARKER        932
#define EEPROM_DUAL_MARKER_VALUE  0xDD

// ===== EEPROM per-mode dual display mask (943-948) =====
#define EEPROM_DUAL_MODES_MASK_ADDR   943  // 5 bytes (943-947) = 40 bit per i modi 0-39
#define EEPROM_DUAL_MODES_MARKER_ADDR 948  // Marker 0xDE
#define EEPROM_DUAL_MODES_MARKER_VALUE 0xDE

// ===== Costanti protocollo =====
#define DUAL_MAGIC               0xDC  // "Dual Clock"
#define DUAL_SYNC_INTERVAL_MS    33    // ~30fps per sincronizzazione stato
#define DUAL_HEARTBEAT_INTERVAL  1000  // 1 secondo
#define DUAL_DISCOVERY_INTERVAL  5000  // Ogni 5 secondi durante la ricerca
#define DUAL_PEER_TIMEOUT        5000  // Timeout heartbeat peer (5 secondi)
#define DUAL_PANEL_SIZE          480   // Dimensione pannello in pixel

// ===== Tipi di pacchetto =====
#define PKT_STATE_SYNC       0  // Sincronizzazione stato completo (master->slave)
#define PKT_HEARTBEAT        1  // Segnale di vita (bidirezionale)
#define PKT_CONFIG_REQUEST   2  // Richiesta configurazione (slave->master broadcast)
#define PKT_TOUCH_EVENT      3  // Evento touch inoltrato (slave->master)
#define PKT_DISCOVERY_REQ    4  // Richiesta discovery broadcast
#define PKT_DISCOVERY_RESP   5  // Risposta discovery dal master
#define PKT_CONFIG_PUSH      6  // Il master invia configurazione allo slave
#define PKT_MODE_CONFIG      7  // Sync config mode-specific (chunked, master->slave)

// ===== Config ID per PKT_MODE_CONFIG =====
#define MODE_CFG_SCROLLTEXT  0x01

// ===== Indirizzo broadcast ESP-NOW =====
static const uint8_t broadcastMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ===== Struttura pacchetto di sincronizzazione =====
// Dimensione max ESP-NOW: 250 bytes
struct SyncPacket {
  uint8_t  magic;           // 0xDC per "Dual Clock"
  uint8_t  packetType;      // Tipo pacchetto (vedi costanti PKT_*)
  uint8_t  mode;            // currentMode attuale
  uint8_t  hours;           // Ora corrente
  uint8_t  minutes;         // Minuti correnti
  uint8_t  seconds;         // Secondi correnti
  uint16_t frameCounter;    // Contatore frame per sincronizzazione animazioni
  uint8_t  colorR;          // Colore corrente - componente rossa
  uint8_t  colorG;          // Colore corrente - componente verde
  uint8_t  colorB;          // Colore corrente - componente blu
  uint8_t  brightness;      // Luminosita' display
  uint8_t  preset;          // Preset corrente
  uint8_t  rainbowEnabled;  // Modalita' rainbow attiva
  // --- Impostazioni sincronizzate Master -> Slave (16 bytes) ---
  uint8_t  brightnessDay;   // Luminosita' giorno del master
  uint8_t  brightnessNight; // Luminosita' notte del master
  uint8_t  volumeDay;       // Volume giorno del master
  uint8_t  volumeNight;     // Volume notte del master
  uint8_t  enabledMask0;    // enabledModesMask byte 0 (LSB)
  uint8_t  enabledMask1;    // enabledModesMask byte 1
  uint8_t  enabledMask2;    // enabledModesMask byte 2
  uint8_t  enabledMask3;    // enabledModesMask byte 3
  uint8_t  enabledMaskExt;  // enabledModesMask byte 4 (bits 32-39)
  uint8_t  ledEnabled;      // LED RGB abilitato
  uint8_t  ledBrightness;   // LED RGB luminosita'
  uint8_t  ledOverride;     // LED RGB override colore
  uint8_t  ledOverR;        // LED RGB override rosso
  uint8_t  ledOverG;        // LED RGB override verde
  uint8_t  ledOverB;        // LED RGB override blu
  uint8_t  ledAudioReact;   // LED RGB audio reactive
  // Area dati aggiuntivi per sincronizzazione specifica del modo
  uint8_t  dataLen;         // Lunghezza dati aggiuntivi (0-200)
  uint8_t  data[200];       // Dati mode-specific
};

// ===== Struttura pacchetto touch =====
struct TouchPacket {
  uint8_t  magic;       // 0xDC
  uint8_t  packetType;  // PKT_TOUCH_EVENT
  int16_t  touchX;      // Coordinata X tocco sul pannello slave
  int16_t  touchY;      // Coordinata Y tocco sul pannello slave
  uint8_t  panelX;      // Posizione X del pannello slave nella griglia
  uint8_t  panelY;      // Posizione Y del pannello slave nella griglia
  uint8_t  touchType;   // 0=tap, 1=long press, 2=release
};

// ===== Struttura pacchetto discovery =====
struct DiscoveryPacket {
  uint8_t  magic;       // 0xDC
  uint8_t  packetType;  // PKT_DISCOVERY_REQ o PKT_DISCOVERY_RESP
  uint8_t  role;        // Ruolo del mittente (1=master, 2=slave)
  uint8_t  gridW;       // Larghezza griglia (solo nella risposta)
  uint8_t  gridH;       // Altezza griglia (solo nella risposta)
  uint8_t  mac[6];      // MAC del mittente
  uint8_t  assignedX;   // Posizione X assegnata (solo nella risposta)
  uint8_t  assignedY;   // Posizione Y assegnata (solo nella risposta)
  uint8_t  enabled;     // 1=dual display attivo, 0=disattivato (per CONFIG_PUSH)
};

// ===== Struttura stato peer =====
struct PeerState {
  uint8_t  mac[6];              // MAC address del peer
  bool     active;              // Il peer e' attualmente attivo
  unsigned long lastHeartbeat;  // Timestamp ultimo heartbeat ricevuto
  uint8_t  peerPanelX;         // Posizione X del peer nella griglia
  uint8_t  peerPanelY;         // Posizione Y del peer nella griglia
};

// ===== Variabili globali =====
bool    dualDisplayEnabled    = false;  // Funzionalita' dual display attiva
uint8_t panelX                = 0;      // Posizione X di questo pannello (0 o 1)
uint8_t panelY                = 0;      // Posizione Y di questo pannello (0 o 1)
uint8_t gridW                 = 1;      // Larghezza griglia (1 o 2)
uint8_t gridH                 = 1;      // Altezza griglia (1 o 2)
uint8_t panelRole             = 0;      // 0=standalone, 1=master, 2=slave
uint8_t peerMACs[3][6]        = {0};    // MAC address dei peer (fino a 3)
uint8_t peerCount             = 0;      // Numero peer registrati
bool    dualDisplayInitialized = false; // Flag inizializzazione completata
uint16_t dualFrameCounter     = 0;      // Contatore frame (incrementato dal master, sincronizzato agli slave)

// Stato dei peer per monitoraggio heartbeat
PeerState peerStates[3] = {0};

// Flag interni
bool espNowInitialized     = false;  // ESP-NOW inizializzato
bool broadcastPeerAdded    = false;  // Peer broadcast registrato
bool discoveryInProgress   = false;  // Ricerca peer in corso
volatile bool dualFlashRequested = false;  // Flag per lampeggio pannello (thread-safe)
volatile bool dualTestSyncRequested = false; // Flag per test sync (thread-safe)

// Bitmask per-mode: bit N=1 → mode N usa multi-display. Default: tutti abilitati (40 bit)
uint64_t dualModesMask = 0xFFFFFFFFFFULL;
unsigned long discoveryStart = 0;    // Timestamp inizio discovery
unsigned long lastDiscoveryBroadcast = 0; // Ultimo broadcast discovery inviato

// Ultimo pacchetto di sincronizzazione ricevuto (per lo slave)
SyncPacket lastReceivedSync;
volatile bool newSyncAvailable = false; // Flag: nuovo pacchetto sync da processare (volatile: scritto da callback WiFi)
portMUX_TYPE syncMux = portMUX_INITIALIZER_UNLOCKED; // Mutex per proteggere lastReceivedSync da race condition

// ===== Forward declarations (necessarie per Arduino IDE) =====
// Coordinate virtuali
void vDrawHLine(int vx, int vy, int w, uint16_t color);
void vDrawVLine(int vx, int vy, int h, uint16_t color);
void vDrawPixel(int vx, int vy, uint16_t color);
void vDrawRoundRect(int vx, int vy, int w, int h, int r, uint16_t color);
void vDrawTriangle(int vx0, int vy0, int vx1, int vy1, int vx2, int vy2, uint16_t color);
// ESP-NOW helpers
String macToString(const uint8_t* mac);
static bool macEqual(const uint8_t* a, const uint8_t* b);
static int findPeerIndex(const uint8_t* mac);
// Gestione peer
void addPeerPanel(const uint8_t* mac);
void removePeerPanel(int index);
void scanForPanels();
static void sendDiscoveryBroadcast();
// Sincronizzazione
void sendSyncPacket();
void processSyncPacket(SyncPacket &pkt);
void sendHeartbeat();
void packModeSpecificData(SyncPacket &pkt);
void unpackModeSpecificData(const SyncPacket &pkt);
void resetModeInitFlags();
void checkPeerHealth();
// EEPROM
void loadDualDisplaySettings();
bool saveDualDisplaySettings();
// Reinizializza ESP-NOW senza ri-leggere config da EEPROM
void reinitDualDisplayEspNow();
// DualGFX proxy
void initDualGfxProxy();
// Config push (master -> slave)
void sendConfigPushToAllPeers();

// ========================================================================
// =================== DUALGFX PROXY CLASS ================================
// ========================================================================
// Classe proxy trasparente: _width/_height restano 480x480 (i modi non sanno
// di essere su un canvas piu' grande). Internamente aggiunge un offset di
// centratura e clipping al viewport del pannello locale.
// Per griglia 2x1: _centerX = (960-480)/2 = 240
//   Panel 0 (panelX=0): vede la meta' sinistra del contenuto (x 0-239)
//   Panel 1 (panelX=1): vede la meta' destra del contenuto (x 240-479)

class DualGFX : public Arduino_GFX {
public:
  Arduino_RGB_Display *_real;
  int16_t _centerX;  // Offset X per centrare 480x480 nel canvas virtuale
  int16_t _centerY;  // Offset Y per centrare 480x480 nel canvas virtuale

  DualGFX(Arduino_RGB_Display *real)
    : Arduino_GFX(480, 480), _real(real) {
    _centerX = ((int)gridW * DUAL_PANEL_SIZE - 480) / 2;
    _centerY = ((int)gridH * DUAL_PANEL_SIZE - 480) / 2;
  }

  bool begin(int32_t speed = GFX_NOT_DEFINED) override {
    return true; // Il display reale e' gia' inizializzato
  }

  void startWrite() override { _real->startWrite(); }
  void endWrite()   override { _real->endWrite(); }
  void flush(bool force_flush = false) override { _real->flush(force_flush); }

  void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {
    int vx = (int)x + _centerX;
    int vy = (int)y + _centerY;
    int lx = vx - panelX * DUAL_PANEL_SIZE;
    int ly = vy - panelY * DUAL_PANEL_SIZE;
    if (lx >= 0 && lx < DUAL_PANEL_SIZE && ly >= 0 && ly < DUAL_PANEL_SIZE) {
      _real->writePixelPreclipped(lx, ly, color);
    }
  }

  void writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    // fillScreen(color) arriva come (0, 0, 480, 480) → riempi tutto il pannello fisico
    if (x == 0 && y == 0 && w >= 480 && h >= 480) {
      _real->writeFillRectPreclipped(0, 0, DUAL_PANEL_SIZE, DUAL_PANEL_SIZE, color);
      return;
    }
    int vx = (int)x + _centerX;
    int vy = (int)y + _centerY;
    int px = panelX * DUAL_PANEL_SIZE;
    int py = panelY * DUAL_PANEL_SIZE;
    int cx0 = max(vx, px);
    int cy0 = max(vy, py);
    int cx1 = min(vx + (int)w, px + DUAL_PANEL_SIZE);
    int cy1 = min(vy + (int)h, py + DUAL_PANEL_SIZE);
    if (cx0 >= cx1 || cy0 >= cy1) return;
    _real->writeFillRectPreclipped(cx0 - px, cy0 - py, cx1 - cx0, cy1 - cy0, color);
  }

  void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override {
    writeFillRectPreclipped(x, y, w, 1, color);
  }

  void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override {
    writeFillRectPreclipped(x, y, 1, h, color);
  }

  void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) override {
    Arduino_GFX::writeLine(x0, y0, x1, y1, color); // Bresenham via writePixelPreclipped
  }

  void draw16bitRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h) override {
    int vx = (int)x + _centerX;
    int vy = (int)y + _centerY;
    int px = panelX * DUAL_PANEL_SIZE;
    int py = panelY * DUAL_PANEL_SIZE;
    int srcX = max(0, px - vx);
    int srcY = max(0, py - vy);
    int dstX = max(0, vx - px);
    int dstY = max(0, vy - py);
    int cw = min(vx + (int)w, px + DUAL_PANEL_SIZE) - max(vx, px);
    int ch = min(vy + (int)h, py + DUAL_PANEL_SIZE) - max(vy, py);
    if (cw <= 0 || ch <= 0) return;
    for (int row = 0; row < ch; row++) {
      _real->draw16bitRGBBitmap(dstX, dstY + row, bitmap + (srcY + row) * w + srcX, cw, 1);
    }
  }

  void draw16bitRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h) override {
    draw16bitRGBBitmap(x, y, (uint16_t *)bitmap, w, h);
  }

  void updateCentering() {
    _centerX = ((int)gridW * DUAL_PANEL_SIZE - 480) / 2;
    _centerY = ((int)gridH * DUAL_PANEL_SIZE - 480) / 2;
  }
};

DualGFX *dualGfx = nullptr;

// Bypass proxy DualGFX per disegnare a schermo intero sul pannello locale (480x480)
// Usato dal mode selector overlay che deve riempire tutto lo schermo del master
void dualGfxBypass(bool bypass) {
  extern Arduino_RGB_Display *realGfx;
  extern Arduino_GFX *gfx;
  if (bypass) {
    gfx = realGfx;  // Disegna direttamente sul display fisico
  } else {
    if (dualGfx && dualDisplayEnabled && (gridW > 1 || gridH > 1)) {
      gfx = dualGfx;  // Ripristina il proxy DualGFX
    }
  }
}

// ===== Per-mode dual display mask =====
// Ritorna true se il mode specificato usa il multi-display
bool isModeDualEnabled(uint8_t mode) {
  return (dualModesMask & (1ULL << mode)) != 0;
}

// Salva dualModesMask in EEPROM (5 bytes big-endian + marker)
void saveDualModesMask() {
  for (int i = 0; i < 5; i++) {
    EEPROM.write(EEPROM_DUAL_MODES_MASK_ADDR + i, (uint8_t)(dualModesMask >> (i * 8)));
  }
  EEPROM.write(EEPROM_DUAL_MODES_MARKER_ADDR, EEPROM_DUAL_MODES_MARKER_VALUE);
  EEPROM.commit();
  Serial.printf("[DUAL] Salvata dualModesMask: 0x%010llX\n", dualModesMask);
}

// Carica dualModesMask da EEPROM (5 bytes + marker check)
void loadDualModesMask() {
  if (EEPROM.read(EEPROM_DUAL_MODES_MARKER_ADDR) != EEPROM_DUAL_MODES_MARKER_VALUE) {
    // Prima volta: scrivi il default in EEPROM per evitare dati sporchi
    dualModesMask = 0xFFFFFFFFFFULL;
    saveDualModesMask();
    Serial.println("[DUAL] dualModesMask: primo avvio, scritto default tutti abilitati");
    return;
  }
  dualModesMask = 0;
  for (int i = 0; i < 5; i++) {
    dualModesMask |= ((uint64_t)EEPROM.read(EEPROM_DUAL_MODES_MASK_ADDR + i)) << (i * 8);
  }
  // Protezione: se la mask e' 0 (nessun modo usa dual = inutile), resetta al default
  if (dualModesMask == 0) {
    dualModesMask = 0xFFFFFFFFFFULL;
    saveDualModesMask();
    Serial.println("[DUAL] dualModesMask era 0, resettata a default tutti abilitati");
    return;
  }
  Serial.printf("[DUAL] Caricata dualModesMask: 0x%010llX\n", dualModesMask);
}

// Attiva/disattiva il proxy DualGFX in base alla configurazione griglia
void initDualGfxProxy() {
  extern Arduino_RGB_Display *realGfx;
  extern Arduino_GFX *gfx;

  if (dualDisplayEnabled && (gridW > 1 || gridH > 1)) {
    if (dualGfx) {
      dualGfx->updateCentering();
    } else {
      dualGfx = new DualGFX(realGfx);
    }
    gfx = dualGfx;
    Serial.printf("[DUAL] DualGFX proxy attivo: center(%d,%d), pannello (%d,%d)\n",
                  dualGfx->_centerX, dualGfx->_centerY, panelX, panelY);
  } else {
    gfx = realGfx;
    if (dualGfx) {
      Serial.println("[DUAL] DualGFX proxy disattivato, display diretto");
    }
  }
}

// ========================================================================
// =================== FUNZIONI COORDINATE VIRTUALI =======================
// ========================================================================

// Ritorna la larghezza totale del canvas virtuale
int virtualWidth() {
  return gridW * DUAL_PANEL_SIZE;
}

// Ritorna l'altezza totale del canvas virtuale
int virtualHeight() {
  return gridH * DUAL_PANEL_SIZE;
}

// Converte una coordinata X virtuale in coordinata locale del pannello
int localX(int vx) {
  return vx - panelX * DUAL_PANEL_SIZE;
}

// Converte una coordinata Y virtuale in coordinata locale del pannello
int localY(int vy) {
  return vy - panelY * DUAL_PANEL_SIZE;
}

// Verifica se un rettangolo virtuale e' visibile su questo pannello
// vx, vy: angolo in alto a sinistra del rettangolo in coordinate virtuali
// w, h: dimensioni del rettangolo
bool isVisibleOnPanel(int vx, int vy, int w, int h) {
  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;
  // Il rettangolo e' fuori dal pannello se:
  // - finisce prima dell'inizio del pannello (asse X o Y)
  // - inizia dopo la fine del pannello (asse X o Y)
  return !(vx + w <= px || vx >= px + DUAL_PANEL_SIZE ||
           vy + h <= py || vy >= py + DUAL_PANEL_SIZE);
}

// ===== Disegna un rettangolo pieno usando coordinate virtuali =====
// Il rettangolo viene clippato ai bordi del pannello locale
void vFillRect(int vx, int vy, int w, int h, uint16_t color) {
  // In modalita' standalone, disegna direttamente senza conversione
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->fillRect(vx, vy, w, h, color);
    return;
  }

  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;

  // Calcola intersezione tra rettangolo virtuale e area del pannello
  int x1 = max(vx, px);
  int y1 = max(vy, py);
  int x2 = min(vx + w, px + DUAL_PANEL_SIZE);
  int y2 = min(vy + h, py + DUAL_PANEL_SIZE);

  // Disegna solo se l'intersezione e' valida (area positiva)
  if (x1 < x2 && y1 < y2) {
    gfx->fillRect(x1 - px, y1 - py, x2 - x1, y2 - y1, color);
  }
}

// ===== Disegna il contorno di un rettangolo usando coordinate virtuali =====
void vDrawRect(int vx, int vy, int w, int h, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->drawRect(vx, vy, w, h, color);
    return;
  }

  // Disegna i 4 lati come linee singole
  vDrawHLine(vx, vy, w, color);              // Lato superiore
  vDrawHLine(vx, vy + h - 1, w, color);      // Lato inferiore
  vDrawVLine(vx, vy, h, color);              // Lato sinistro
  vDrawVLine(vx + w - 1, vy, h, color);      // Lato destro
}

// ===== Disegna una linea orizzontale usando coordinate virtuali =====
void vDrawHLine(int vx, int vy, int w, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->drawFastHLine(vx, vy, w, color);
    return;
  }

  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;

  // La linea e' visibile solo se la Y cade nel pannello
  if (vy < py || vy >= py + DUAL_PANEL_SIZE) return;

  // Clippa la X
  int x1 = max(vx, px);
  int x2 = min(vx + w, px + DUAL_PANEL_SIZE);
  if (x1 < x2) {
    gfx->drawFastHLine(x1 - px, vy - py, x2 - x1, color);
  }
}

// ===== Disegna una linea verticale usando coordinate virtuali =====
void vDrawVLine(int vx, int vy, int h, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->drawFastVLine(vx, vy, h, color);
    return;
  }

  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;

  // La linea e' visibile solo se la X cade nel pannello
  if (vx < px || vx >= px + DUAL_PANEL_SIZE) return;

  // Clippa la Y
  int y1 = max(vy, py);
  int y2 = min(vy + h, py + DUAL_PANEL_SIZE);
  if (y1 < y2) {
    gfx->drawFastVLine(vx - px, y1 - py, y2 - y1, color);
  }
}

// ===== Disegna una linea generica usando coordinate virtuali =====
// Utilizza l'algoritmo di Bresenham solo per le parti visibili
void vDrawLine(int vx0, int vy0, int vx1, int vy1, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->drawLine(vx0, vy0, vx1, vy1, color);
    return;
  }

  // Per semplicita' e performance, convertiamo e deleghiamo a gfx
  // con clipping tramite Cohen-Sutherland semplificato
  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;
  int pxMax = px + DUAL_PANEL_SIZE - 1;
  int pyMax = py + DUAL_PANEL_SIZE - 1;

  // Clipping Cohen-Sutherland
  int x0 = vx0, y0 = vy0, x1 = vx1, y1 = vy1;

  // Codici regione per Cohen-Sutherland
  auto computeCode = [&](int x, int y) -> int {
    int code = 0;
    if (x < px)    code |= 1;  // Sinistra
    if (x > pxMax) code |= 2;  // Destra
    if (y < py)    code |= 4;  // Sopra
    if (y > pyMax) code |= 8;  // Sotto
    return code;
  };

  int code0 = computeCode(x0, y0);
  int code1 = computeCode(x1, y1);
  bool accept = false;

  while (true) {
    if (!(code0 | code1)) {
      // Entrambi i punti dentro il pannello
      accept = true;
      break;
    } else if (code0 & code1) {
      // Entrambi fuori dallo stesso lato, linea invisibile
      break;
    } else {
      // Clippa il punto che e' fuori
      int codeOut = code0 ? code0 : code1;
      int x, y;

      if (codeOut & 8) {        // Sotto
        x = x0 + (long)(x1 - x0) * (pyMax - y0) / (y1 - y0);
        y = pyMax;
      } else if (codeOut & 4) { // Sopra
        x = x0 + (long)(x1 - x0) * (py - y0) / (y1 - y0);
        y = py;
      } else if (codeOut & 2) { // Destra
        y = y0 + (long)(y1 - y0) * (pxMax - x0) / (x1 - x0);
        x = pxMax;
      } else {                  // Sinistra
        y = y0 + (long)(y1 - y0) * (px - x0) / (x1 - x0);
        x = px;
      }

      if (codeOut == code0) {
        x0 = x; y0 = y;
        code0 = computeCode(x0, y0);
      } else {
        x1 = x; y1 = y;
        code1 = computeCode(x1, y1);
      }
    }
  }

  if (accept) {
    gfx->drawLine(x0 - px, y0 - py, x1 - px, y1 - py, color);
  }
}

// ===== Riempi tutto lo schermo locale con un colore =====
void vFillScreen(uint16_t color) {
  gfx->fillScreen(color);
}

// ===== Disegna il contorno di un cerchio usando coordinate virtuali =====
void vDrawCircle(int vcx, int vcy, int r, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->drawCircle(vcx, vcy, r, color);
    return;
  }

  // Verifica se il bounding box del cerchio interseca il pannello
  if (!isVisibleOnPanel(vcx - r, vcy - r, r * 2 + 1, r * 2 + 1)) return;

  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;

  // Se il cerchio e' completamente contenuto nel pannello, disegna direttamente
  if (vcx - r >= px && vcx + r < px + DUAL_PANEL_SIZE &&
      vcy - r >= py && vcy + r < py + DUAL_PANEL_SIZE) {
    gfx->drawCircle(vcx - px, vcy - py, r, color);
    return;
  }

  // Altrimenti disegna pixel per pixel con l'algoritmo di Bresenham per cerchi
  // verificando la visibilita' di ciascun punto
  int x = 0, y = r;
  int d = 3 - 2 * r;

  auto plotPixel = [&](int sx, int sy) {
    if (sx >= px && sx < px + DUAL_PANEL_SIZE &&
        sy >= py && sy < py + DUAL_PANEL_SIZE) {
      gfx->drawPixel(sx - px, sy - py, color);
    }
  };

  auto plotCirclePoints = [&](int cx, int cy, int ox, int oy) {
    plotPixel(cx + ox, cy + oy);
    plotPixel(cx - ox, cy + oy);
    plotPixel(cx + ox, cy - oy);
    plotPixel(cx - ox, cy - oy);
    plotPixel(cx + oy, cy + ox);
    plotPixel(cx - oy, cy + ox);
    plotPixel(cx + oy, cy - ox);
    plotPixel(cx - oy, cy - ox);
  };

  while (y >= x) {
    plotCirclePoints(vcx, vcy, x, y);
    x++;
    if (d > 0) {
      y--;
      d = d + 4 * (x - y) + 10;
    } else {
      d = d + 4 * x + 6;
    }
  }
}

// ===== Disegna un cerchio pieno usando coordinate virtuali =====
void vFillCircle(int vcx, int vcy, int r, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->fillCircle(vcx, vcy, r, color);
    return;
  }

  // Verifica se il bounding box del cerchio interseca il pannello
  if (!isVisibleOnPanel(vcx - r, vcy - r, r * 2 + 1, r * 2 + 1)) return;

  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;

  // Se il cerchio e' completamente contenuto nel pannello, disegna direttamente
  if (vcx - r >= px && vcx + r < px + DUAL_PANEL_SIZE &&
      vcy - r >= py && vcy + r < py + DUAL_PANEL_SIZE) {
    gfx->fillCircle(vcx - px, vcy - py, r, color);
    return;
  }

  // Disegna riga per riga (scanline) con clipping
  for (int dy = -r; dy <= r; dy++) {
    int sy = vcy + dy;
    // Verifica che la riga sia visibile sul pannello
    if (sy < py || sy >= py + DUAL_PANEL_SIZE) continue;

    // Calcola la larghezza della riga a questa altezza (cerchio)
    // x^2 + dy^2 = r^2 => x = sqrt(r^2 - dy^2)
    float halfW = sqrt((float)(r * r - dy * dy));
    int sx1 = (int)(vcx - halfW);
    int sx2 = (int)(vcx + halfW);

    // Clippa ai bordi del pannello
    int cx1 = max(sx1, px);
    int cx2 = min(sx2, px + DUAL_PANEL_SIZE - 1);

    if (cx1 <= cx2) {
      gfx->drawFastHLine(cx1 - px, sy - py, cx2 - cx1 + 1, color);
    }
  }
}

// ===== Disegna testo a una posizione virtuale =====
// Il testo viene disegnato solo se la posizione di partenza cade nel pannello
// NOTA: per testo lungo che attraversa i bordi, ogni pannello mostra la sua porzione
void vDrawText(int vx, int vy, const char* text, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->setTextColor(color);
    gfx->setCursor(vx, vy);
    gfx->print(text);
    return;
  }

  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;

  // Stima approssimativa larghezza testo (dipende dal font attivo)
  // Usiamo 10px per carattere come stima conservativa
  int estWidth = strlen(text) * 10;
  int estHeight = 20; // Altezza stimata del font

  // Verifica se il bounding box stimato del testo interseca il pannello
  if (!isVisibleOnPanel(vx, vy - estHeight, estWidth, estHeight + 5)) return;

  // Converti in coordinate locali e disegna
  gfx->setTextColor(color);
  gfx->setCursor(vx - px, vy - py);
  gfx->print(text);
}

// ===== Posiziona il cursore usando coordinate virtuali =====
void vSetCursor(int vx, int vy) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->setCursor(vx, vy);
    return;
  }

  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;
  gfx->setCursor(vx - px, vy - py);
}

// ===== Stampa testo alla posizione corrente del cursore virtuale =====
void vPrint(const char* text) {
  gfx->print(text);
}

// ===== Disegna un rettangolo arrotondato pieno usando coordinate virtuali =====
void vFillRoundRect(int vx, int vy, int w, int h, int r, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->fillRoundRect(vx, vy, w, h, r, color);
    return;
  }

  if (!isVisibleOnPanel(vx, vy, w, h)) return;

  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;

  // Se completamente contenuto, disegna direttamente
  if (vx >= px && vx + w <= px + DUAL_PANEL_SIZE &&
      vy >= py && vy + h <= py + DUAL_PANEL_SIZE) {
    gfx->fillRoundRect(vx - px, vy - py, w, h, r, color);
    return;
  }

  // Fallback: disegna come rettangolo normale clippato (approssimazione)
  // Per bordi arrotondati perfetti servirebbe un rasterizzatore completo
  vFillRect(vx, vy, w, h, color);
}

// ===== Disegna un triangolo pieno usando coordinate virtuali =====
void vFillTriangle(int vx0, int vy0, int vx1, int vy1, int vx2, int vy2, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->fillTriangle(vx0, vy0, vx1, vy1, vx2, vy2, color);
    return;
  }

  // Calcola bounding box del triangolo
  int minX = min(vx0, min(vx1, vx2));
  int maxX = max(vx0, max(vx1, vx2));
  int minY = min(vy0, min(vy1, vy2));
  int maxY = max(vy0, max(vy1, vy2));

  if (!isVisibleOnPanel(minX, minY, maxX - minX + 1, maxY - minY + 1)) return;

  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;

  // Se completamente contenuto, disegna direttamente
  if (minX >= px && maxX < px + DUAL_PANEL_SIZE &&
      minY >= py && maxY < py + DUAL_PANEL_SIZE) {
    gfx->fillTriangle(vx0 - px, vy0 - py, vx1 - px, vy1 - py, vx2 - px, vy2 - py, color);
    return;
  }

  // Fallback: disegna con offset (i pixel fuori dal pannello verranno ignorati dalla libreria GFX)
  gfx->fillTriangle(vx0 - px, vy0 - py, vx1 - px, vy1 - py, vx2 - px, vy2 - py, color);
}

// ===== Disegna un singolo pixel usando coordinate virtuali =====
void vDrawPixel(int vx, int vy, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->drawPixel(vx, vy, color);
    return;
  }

  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;

  if (vx >= px && vx < px + DUAL_PANEL_SIZE &&
      vy >= py && vy < py + DUAL_PANEL_SIZE) {
    gfx->drawPixel(vx - px, vy - py, color);
  }
}

// ===== Disegna il contorno di un rettangolo arrotondato usando coordinate virtuali =====
void vDrawRoundRect(int vx, int vy, int w, int h, int r, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->drawRoundRect(vx, vy, w, h, r, color);
    return;
  }

  if (!isVisibleOnPanel(vx, vy, w, h)) return;

  int px = panelX * DUAL_PANEL_SIZE;
  int py = panelY * DUAL_PANEL_SIZE;

  // Se completamente contenuto, disegna direttamente
  if (vx >= px && vx + w <= px + DUAL_PANEL_SIZE &&
      vy >= py && vy + h <= py + DUAL_PANEL_SIZE) {
    gfx->drawRoundRect(vx - px, vy - py, w, h, r, color);
    return;
  }

  // Fallback: disegna come rettangolo clippato
  vDrawRect(vx, vy, w, h, color);
}

// ===== Disegna contorno triangolo usando coordinate virtuali =====
void vDrawTriangle(int vx0, int vy0, int vx1, int vy1, int vx2, int vy2, uint16_t color) {
  if (!dualDisplayEnabled || (gridW == 1 && gridH == 1)) {
    gfx->drawTriangle(vx0, vy0, vx1, vy1, vx2, vy2, color);
    return;
  }

  vDrawLine(vx0, vy0, vx1, vy1, color);
  vDrawLine(vx1, vy1, vx2, vy2, color);
  vDrawLine(vx2, vy2, vx0, vy0, color);
}

// ===== Converti coordinate touch locali in coordinate virtuali =====
// Utile quando lo slave deve comunicare al master la posizione esatta del tocco
// nel canvas virtuale
void localToVirtual(int lx, int ly, int &vx, int &vy) {
  vx = lx + panelX * DUAL_PANEL_SIZE;
  vy = ly + panelY * DUAL_PANEL_SIZE;
}

// ========================================================================
// ========================= FUNZIONI ESP-NOW =============================
// ========================================================================

// ===== Helper: confronta due MAC address =====
static bool macEqual(const uint8_t* a, const uint8_t* b) {
  return memcmp(a, b, 6) == 0;
}

// ===== Helper: formatta MAC come stringa =====
String macToString(const uint8_t* mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

// ===== Helper: verifica se un MAC e' gia' registrato tra i peer =====
static int findPeerIndex(const uint8_t* mac) {
  for (int i = 0; i < (int)peerCount; i++) {
    if (macEqual(peerMACs[i], mac)) return i;
  }
  return -1;
}

// ===== Callback invio dati ESP-NOW =====
// Chiamata dopo ogni tentativo di invio - logga errori per debug
void onDualDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.printf("[DUAL] Errore invio a %s\n", macToString(mac_addr).c_str());
  }
}

// ===== Callback ricezione dati ESP-NOW =====
// Gestisce tutti i tipi di pacchetto ricevuti
void onDualDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  // Verifica dimensione minima (almeno magic + packetType)
  if (len < 2) return;

  // Verifica magic byte
  if (incomingData[0] != DUAL_MAGIC) return;

  uint8_t pktType = incomingData[1];

  switch (pktType) {

    // --- Pacchetto sincronizzazione stato (slave riceve dal master) ---
    case PKT_STATE_SYNC: {
      if (panelRole != 2) return; // Solo lo slave processa i sync
      if ((size_t)len < sizeof(SyncPacket) - 200) return; // Dimensione minima senza dati

      portENTER_CRITICAL(&syncMux);
      memcpy(&lastReceivedSync, incomingData, min((size_t)len, sizeof(SyncPacket)));
      newSyncAvailable = true;
      portEXIT_CRITICAL(&syncMux);

      // Aggiorna heartbeat del master
      int idx = findPeerIndex(mac_addr);
      if (idx >= 0) {
        peerStates[idx].lastHeartbeat = millis();
        peerStates[idx].active = true;
      }
      break;
    }

    // --- Heartbeat (entrambe le direzioni) ---
    case PKT_HEARTBEAT: {
      int idx = findPeerIndex(mac_addr);
      if (idx >= 0) {
        peerStates[idx].lastHeartbeat = millis();
        peerStates[idx].active = true;
      } else if (peerCount < 3) {
        // Peer sconosciuto ma con magic valido - potrebbe essere un nuovo pannello
        Serial.printf("[DUAL] Heartbeat da peer sconosciuto: %s\n", macToString(mac_addr).c_str());
      }
      break;
    }

    // --- Richiesta configurazione (master riceve dallo slave) ---
    case PKT_CONFIG_REQUEST: {
      if (panelRole != 1) return; // Solo il master risponde
      Serial.printf("[DUAL] Richiesta config da: %s\n", macToString(mac_addr).c_str());

      // Aggiungi automaticamente come peer se non presente
      if (findPeerIndex(mac_addr) < 0 && peerCount < 3) {
        addPeerPanel(mac_addr);
        Serial.printf("[DUAL] Aggiunto automaticamente peer da config request\n");
      }

      // Invia risposta con configurazione completa
      DiscoveryPacket resp;
      memset(&resp, 0, sizeof(resp));
      resp.magic      = DUAL_MAGIC;
      resp.packetType = PKT_CONFIG_PUSH;
      resp.role       = 1; // master
      resp.gridW      = gridW;
      resp.gridH      = gridH;
      resp.enabled    = dualDisplayEnabled ? 1 : 0;
      // Fornisci il MAC del master
      WiFi.macAddress(resp.mac);
      // Assegna posizione: il primo slave libero viene assegnato in ordine
      // (1,0) -> (0,1) -> (1,1)
      resp.assignedX = 0;
      resp.assignedY = 0;
      // Trova la prima posizione libera nella griglia
      bool positionTaken[2][2] = {false};
      positionTaken[panelX][panelY] = true; // Posizione del master
      for (int i = 0; i < (int)peerCount; i++) {
        if (peerStates[i].active && !macEqual(peerMACs[i], mac_addr)) {
          positionTaken[peerStates[i].peerPanelX][peerStates[i].peerPanelY] = true;
        }
      }
      // Cerca prima posizione libera (ordine: TL, TR, BL, BR)
      bool assigned = false;
      for (int gy = 0; gy < (int)gridH && !assigned; gy++) {
        for (int gx = 0; gx < (int)gridW && !assigned; gx++) {
          if (!positionTaken[gx][gy]) {
            resp.assignedX = gx;
            resp.assignedY = gy;
            assigned = true;
          }
        }
      }

      // Salva la posizione assegnata nello stato del peer
      int idx = findPeerIndex(mac_addr);
      if (idx >= 0) {
        peerStates[idx].peerPanelX = resp.assignedX;
        peerStates[idx].peerPanelY = resp.assignedY;
      }

      Serial.printf("[DUAL] Assegnata posizione (%d,%d) a %s\n",
                    resp.assignedX, resp.assignedY, macToString(mac_addr).c_str());

      esp_now_send(mac_addr, (uint8_t*)&resp, sizeof(resp));
      break;
    }

    // --- Evento touch inoltrato dallo slave ---
    case PKT_TOUCH_EVENT: {
      if (panelRole != 1) return; // Solo il master processa i touch
      if ((size_t)len < sizeof(TouchPacket)) return;

      TouchPacket tp;
      memcpy(&tp, incomingData, sizeof(TouchPacket));

      // Converti coordinate touch dello slave in coordinate virtuali
      int virtualTouchX = tp.touchX + tp.panelX * DUAL_PANEL_SIZE;
      int virtualTouchY = tp.touchY + tp.panelY * DUAL_PANEL_SIZE;

      Serial.printf("[DUAL] Touch remoto da pannello (%d,%d): locale(%d,%d) -> virtuale(%d,%d) tipo=%d\n",
                    tp.panelX, tp.panelY, tp.touchX, tp.touchY,
                    virtualTouchX, virtualTouchY, tp.touchType);

      // Il master convertira' le coordinate virtuali in coordinate locali
      // per il proprio pannello e le passera' al sistema touch esistente.
      // Per ora il touch remoto viene gestito come un cambio di modalita'
      // se tocca nella zona apposita (alto-sinistra del canvas virtuale).
      // La propagazione avviene tramite il prossimo pacchetto sync.

      // Converti in coordinate locali del master (pannello 0,0 tipicamente)
      int masterLocalX = virtualTouchX - panelX * DUAL_PANEL_SIZE;
      int masterLocalY = virtualTouchY - panelY * DUAL_PANEL_SIZE;

      // Se il touch cade nel pannello master, simulalo come touch locale
      if (masterLocalX >= 0 && masterLocalX < DUAL_PANEL_SIZE &&
          masterLocalY >= 0 && masterLocalY < DUAL_PANEL_SIZE) {
        // Nota: checkButtons() nel loop principale legge dal touch fisico.
        // Per i touch remoti, impostiamo variabili che il loop puo' leggere.
        // Questo e' un approccio base - per un'integrazione completa
        // servirebbe un sistema di coda eventi touch.
        Serial.printf("[DUAL] Touch remoto mappato su master locale: (%d,%d)\n",
                      masterLocalX, masterLocalY);
      }

      // Gestione touch remoto per modalita' PONG
#ifdef EFFECT_PONG
      if (currentMode == MODE_PONG) {
        extern void handlePongRemoteTouch(int vy);
        handlePongRemoteTouch(virtualTouchY);
      }
#endif
      // Gestione touch remoto per modalita' SCROLLTEXT
#ifdef EFFECT_SCROLLTEXT
      if (currentMode == MODE_SCROLLTEXT) {
        extern void handleScrollTextTouch(int x, int y);
        handleScrollTextTouch(virtualTouchX, virtualTouchY);
      }
#endif
      break;
    }

    // --- Richiesta discovery (broadcast) ---
    case PKT_DISCOVERY_REQ: {
      if (panelRole != 1) return; // Solo il master risponde al discovery

      DiscoveryPacket req;
      if ((size_t)len >= sizeof(DiscoveryPacket)) {
        memcpy(&req, incomingData, sizeof(DiscoveryPacket));
      }

      Serial.printf("[DUAL] Discovery request ricevuta da: %s\n", macToString(mac_addr).c_str());

      // Aggiungi come peer se non presente
      if (findPeerIndex(mac_addr) < 0 && peerCount < 3) {
        addPeerPanel(mac_addr);
      }

      // Rispondi con info del master
      DiscoveryPacket resp;
      memset(&resp, 0, sizeof(resp));
      resp.magic      = DUAL_MAGIC;
      resp.packetType = PKT_DISCOVERY_RESP;
      resp.role       = 1;
      resp.gridW      = gridW;
      resp.gridH      = gridH;
      WiFi.macAddress(resp.mac);

      // Assegna posizione come per CONFIG_REQUEST
      bool positionTaken2[2][2] = {false};
      positionTaken2[panelX][panelY] = true;
      for (int i = 0; i < (int)peerCount; i++) {
        if (peerStates[i].active && !macEqual(peerMACs[i], mac_addr)) {
          positionTaken2[peerStates[i].peerPanelX][peerStates[i].peerPanelY] = true;
        }
      }
      bool found = false;
      for (int gy = 0; gy < (int)gridH && !found; gy++) {
        for (int gx = 0; gx < (int)gridW && !found; gx++) {
          if (!positionTaken2[gx][gy]) {
            resp.assignedX = gx;
            resp.assignedY = gy;
            found = true;
          }
        }
      }

      int idx = findPeerIndex(mac_addr);
      if (idx >= 0) {
        peerStates[idx].peerPanelX = resp.assignedX;
        peerStates[idx].peerPanelY = resp.assignedY;
      }

      Serial.printf("[DUAL] Discovery response: posizione (%d,%d) per %s\n",
                    resp.assignedX, resp.assignedY, macToString(mac_addr).c_str());

      esp_now_send(mac_addr, (uint8_t*)&resp, sizeof(resp));
      break;
    }

    // --- Risposta discovery (slave riceve dal master) ---
    case PKT_DISCOVERY_RESP:
    case PKT_CONFIG_PUSH: {
      if (panelRole != 2) return; // Solo lo slave processa queste risposte

      if ((size_t)len < sizeof(DiscoveryPacket)) return;

      DiscoveryPacket resp;
      memcpy(&resp, incomingData, sizeof(DiscoveryPacket));

      Serial.printf("[DUAL] %s ricevuto dal master %s\n",
                    pktType == PKT_CONFIG_PUSH ? "Config push" : "Discovery resp",
                    macToString(mac_addr).c_str());

      if (pktType == PKT_DISCOVERY_RESP) {
        // Discovery: applica tutta la configurazione (griglia + posizione)
        Serial.printf("[DUAL] Discovery: griglia=%dx%d pos=(%d,%d)\n",
                      resp.gridW, resp.gridH, resp.assignedX, resp.assignedY);
        gridW  = resp.gridW;
        gridH  = resp.gridH;
        panelX = resp.assignedX;
        panelY = resp.assignedY;
      }

      if (pktType == PKT_CONFIG_PUSH) {
        // Config push: cambia SOLO enabled (non sovrascrivere grid/posizione
        // perche' peerStates[].peerPanelX/Y non sono salvati in EEPROM e sarebbero 0)
        bool newEnabled = (resp.enabled == 1);
        if (dualDisplayEnabled != newEnabled) {
          dualDisplayEnabled = newEnabled;
          Serial.printf("[DUAL] Slave %s dal master\n", newEnabled ? "ATTIVATO" : "DISATTIVATO");
        }
        // Aggiorna griglia solo se il master la manda diversa da zero
        if (resp.gridW >= 1 && resp.gridH >= 1) {
          gridW = resp.gridW;
          gridH = resp.gridH;
        }
      }

      // Registra il master come peer
      if (findPeerIndex(mac_addr) < 0 && peerCount < 3) {
        addPeerPanel(mac_addr);
      }

      // Salva le impostazioni aggiornate
      saveDualDisplaySettings();

      // Aggiorna il proxy GFX (attiva/disattiva centratura)
      // NON deinizializzare ESP-NOW: resta in ascolto per ri-abilitazione dal master
      initDualGfxProxy();
      dualDisplayInitialized = dualDisplayEnabled;

      // Forza ridisegno completo del display (il gfx pointer e' cambiato)
      extern Arduino_GFX *gfx;
      gfx->fillScreen(BLACK);
      resetModeInitFlags();

      discoveryInProgress = false;
      Serial.printf("[DUAL] Configurazione applicata! enabled=%d pos=(%d,%d) grid=%dx%d\n",
                    dualDisplayEnabled, panelX, panelY, gridW, gridH);
      break;
    }

    // --- Config mode-specific (master invia config persistente allo slave) ---
    case PKT_MODE_CONFIG: {
      if (panelRole != 2) return; // Solo lo slave riceve
      if (len < 6) return;
      uint8_t configId = incomingData[2];
#ifdef EFFECT_SCROLLTEXT
      if (configId == MODE_CFG_SCROLLTEXT) {
        extern void scrollReceiveConfigChunk(const uint8_t* data, int len);
        scrollReceiveConfigChunk(incomingData, len);
      }
#endif
      break;
    }

    default:
      Serial.printf("[DUAL] Pacchetto sconosciuto tipo=%d da %s\n",
                    pktType, macToString(mac_addr).c_str());
      break;
  }
}

// ========================================================================
// =================== GESTIONE PEER ======================================
// ========================================================================

// ===== Aggiungi un pannello peer =====
// Registra il MAC address e aggiunge il peer in ESP-NOW
void addPeerPanel(const uint8_t* mac) {
  // Verifica se gia' presente
  if (findPeerIndex(mac) >= 0) {
    Serial.printf("[DUAL] Peer %s gia' registrato\n", macToString(mac).c_str());
    return;
  }

  // Verifica spazio disponibile
  if (peerCount >= 3) {
    Serial.println("[DUAL] ERRORE: Raggiunto il limite massimo di 3 peer!");
    return;
  }

  // Copia MAC nell'array
  memcpy(peerMACs[peerCount], mac, 6);

  // Inizializza stato peer
  peerStates[peerCount].active = true;
  peerStates[peerCount].lastHeartbeat = millis();
  memcpy(peerStates[peerCount].mac, mac, 6);
  peerStates[peerCount].peerPanelX = 0;
  peerStates[peerCount].peerPanelY = 0;

  // Registra il peer in ESP-NOW
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0; // Stesso canale WiFi corrente
  peerInfo.encrypt = false;

  esp_err_t result = esp_now_add_peer(&peerInfo);
  if (result == ESP_OK) {
    Serial.printf("[DUAL] Peer aggiunto [%d]: %s\n", peerCount, macToString(mac).c_str());
    peerCount++;
    saveDualDisplaySettings();
  } else if (result == ESP_ERR_ESPNOW_EXIST) {
    // Peer gia' registrato in ESP-NOW (ma non nel nostro array) - ok
    Serial.printf("[DUAL] Peer %s gia' in ESP-NOW, aggiunto al nostro array\n", macToString(mac).c_str());
    peerCount++;
    saveDualDisplaySettings();
  } else {
    Serial.printf("[DUAL] ERRORE aggiunta peer: %d\n", result);
  }
}

// ===== Rimuovi un pannello peer =====
// index: indice nel array peerMACs (0-2)
void removePeerPanel(int index) {
  if (index < 0 || index >= (int)peerCount) {
    Serial.printf("[DUAL] ERRORE: Indice peer %d non valido (count=%d)\n", index, peerCount);
    return;
  }

  Serial.printf("[DUAL] Rimozione peer [%d]: %s\n", index, macToString(peerMACs[index]).c_str());

  // Rimuovi da ESP-NOW
  esp_now_del_peer(peerMACs[index]);

  // Compatta l'array spostando gli elementi successivi
  for (int i = index; i < (int)peerCount - 1; i++) {
    memcpy(peerMACs[i], peerMACs[i + 1], 6);
    peerStates[i] = peerStates[i + 1];
  }

  // Azzera l'ultimo slot
  memset(peerMACs[peerCount - 1], 0, 6);
  memset(&peerStates[peerCount - 1], 0, sizeof(PeerState));

  peerCount--;
  saveDualDisplaySettings();
  Serial.printf("[DUAL] Peer rimosso, peer rimanenti: %d\n", peerCount);
}

// ===== Scansione broadcast per trovare pannelli vicini =====
// Invia un pacchetto discovery in broadcast. I master nelle vicinanze rispondono.
void scanForPanels() {
  if (!espNowInitialized) {
    Serial.println("[DUAL] ESP-NOW non inizializzato, impossibile fare scansione");
    return;
  }

  Serial.println("[DUAL] Avvio scansione pannelli via broadcast...");

  discoveryInProgress = true;
  discoveryStart = millis();
  lastDiscoveryBroadcast = 0; // Forza invio immediato

  // Assicurati che il peer broadcast sia registrato
  if (!broadcastPeerAdded) {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_err_t res = esp_now_add_peer(&peerInfo);
    if (res == ESP_OK || res == ESP_ERR_ESPNOW_EXIST) {
      broadcastPeerAdded = true;
    } else {
      Serial.printf("[DUAL] ERRORE registrazione peer broadcast: %d\n", res);
      discoveryInProgress = false;
      return;
    }
  }

  // Il primo broadcast verra' inviato nel prossimo updateDualDisplay()
}

// ===== Invia un pacchetto discovery broadcast =====
static void sendDiscoveryBroadcast() {
  DiscoveryPacket pkt;
  memset(&pkt, 0, sizeof(pkt));
  pkt.magic      = DUAL_MAGIC;
  pkt.packetType = PKT_DISCOVERY_REQ;
  pkt.role       = panelRole;
  pkt.gridW      = gridW;
  pkt.gridH      = gridH;
  WiFi.macAddress(pkt.mac);

  esp_err_t result = esp_now_send(broadcastMAC, (uint8_t*)&pkt, sizeof(pkt));
  if (result == ESP_OK) {
    Serial.println("[DUAL] Discovery broadcast inviato");
  } else {
    Serial.printf("[DUAL] ERRORE invio discovery broadcast: %d\n", result);
  }
}

// ========================================================================
// =================== SINCRONIZZAZIONE ===================================
// ========================================================================

// Variabili extern necessarie dal file principale
extern uint8_t currentPreset;
extern bool rainbowModeEnabled;
extern uint8_t brightnessDay;
extern uint8_t brightnessNight;
extern uint8_t volumeDay;
extern uint8_t volumeNight;
extern uint64_t enabledModesMask;
extern bool    ledRgbEnabled;
extern uint8_t ledRgbBrightness;
extern bool    ledRgbOverride;
extern uint8_t ledRgbOverrideR;
extern uint8_t ledRgbOverrideG;
extern uint8_t ledRgbOverrideB;
extern uint8_t ledAudioReactive;

// ===== Invia pacchetto di sincronizzazione stato =====
// Chiamato dal master ogni DUAL_SYNC_INTERVAL_MS per mantenere gli slave sincronizzati
void sendSyncPacket() {
  if (peerCount == 0) return; // Nessun peer a cui inviare

  SyncPacket pkt;
  memset(&pkt, 0, sizeof(pkt));

  pkt.magic          = DUAL_MAGIC;
  pkt.packetType     = PKT_STATE_SYNC;
  pkt.mode           = (uint8_t)currentMode;
  pkt.hours          = currentHour;
  pkt.minutes        = currentMinute;
  pkt.seconds        = currentSecond;
  pkt.frameCounter   = dualFrameCounter;
  pkt.colorR         = currentColor.r;
  pkt.colorG         = currentColor.g;
  pkt.colorB         = currentColor.b;
  pkt.brightness     = lastAppliedBrightness;
  pkt.preset         = currentPreset;
  pkt.rainbowEnabled = rainbowModeEnabled ? 1 : 0;
  // Impostazioni sincronizzate Master -> Slave
  pkt.brightnessDay  = brightnessDay;
  pkt.brightnessNight = brightnessNight;
  pkt.volumeDay      = volumeDay;
  pkt.volumeNight    = volumeNight;
  pkt.enabledMask0   = (uint8_t)(enabledModesMask & 0xFF);
  pkt.enabledMask1   = (uint8_t)((enabledModesMask >> 8) & 0xFF);
  pkt.enabledMask2   = (uint8_t)((enabledModesMask >> 16) & 0xFF);
  pkt.enabledMask3   = (uint8_t)((enabledModesMask >> 24) & 0xFF);
  pkt.enabledMaskExt = (uint8_t)((enabledModesMask >> 32) & 0xFF);
  pkt.ledEnabled     = ledRgbEnabled ? 1 : 0;
  pkt.ledBrightness  = ledRgbBrightness;
  pkt.ledOverride    = ledRgbOverride ? 1 : 0;
  pkt.ledOverR       = ledRgbOverrideR;
  pkt.ledOverG       = ledRgbOverrideG;
  pkt.ledOverB       = ledRgbOverrideB;
  pkt.ledAudioReact  = ledAudioReactive;
  pkt.dataLen        = 0;

  // Dati mode-specific: aggiungi informazioni extra per modalita' che ne hanno bisogno
  // I dati vengono scritti nell'area pkt.data[] e pkt.dataLen viene aggiornato
  // Questo permette agli slave di replicare fedelmente lo stato delle animazioni
  packModeSpecificData(pkt);

  // Codifica flag dual bypass nel bit 7 di pkt.mode (mode 0-29, bit 7 sempre libero)
  // Bit 7 = 1 significa "questo modo NON usa multi-display" (bypass attivo)
  if (!isModeDualEnabled((uint8_t)currentMode)) {
    pkt.mode |= 0x80;
  }

  // Calcola la dimensione effettiva del pacchetto (senza i byte data[] non usati)
  size_t pktSize = sizeof(SyncPacket) - 200 + pkt.dataLen;

  // Invia a tutti i peer registrati
  for (int i = 0; i < (int)peerCount; i++) {
    esp_now_send(peerMACs[i], (uint8_t*)&pkt, pktSize);
  }

  // Incrementa il frame counter
  dualFrameCounter++;
}

// ===== Prepara dati mode-specific per il pacchetto sync =====
// Ogni modalita' puo' inserire dati aggiuntivi per sincronizzare le animazioni
void packModeSpecificData(SyncPacket &pkt) {
  // Qui si possono aggiungere dati specifici per ogni modalita'
  // Esempio: posizione elementi animati, seed random, contatori, ecc.

  switch ((DisplayMode)pkt.mode) {

    // Per le modalita' con animazioni, includiamo il seed per il random
    // cosi' tutti i pannelli generano le stesse sequenze pseudo-casuali
    case MODE_MATRIX:
    case MODE_MATRIX2: {
      // Invia il frame counter come seed per generare la stessa pioggia
      if (pkt.dataLen + 4 <= 200) {
        uint32_t seed = dualFrameCounter;
        memcpy(pkt.data + pkt.dataLen, &seed, 4);
        pkt.dataLen += 4;
      }
      break;
    }

    case MODE_SNAKE: {
      // Sincronizza stato completo del serpente: pathChoice(1) + pathIndex(2) + completed(1) + 8 segments(16) = 20 bytes
      extern uint8_t snakePathChoice;
      extern uint16_t snakePathIndex;
      extern bool snakeIsCompleted;
      extern Snake snake;
      if (pkt.dataLen + 20 <= 200) {
        pkt.data[pkt.dataLen++] = snakePathChoice;
        memcpy(pkt.data + pkt.dataLen, &snakePathIndex, 2); pkt.dataLen += 2;
        pkt.data[pkt.dataLen++] = snakeIsCompleted ? 1 : 0;
        for (uint8_t i = 0; i < 8; i++) {
          uint16_t idx = snake.segments[i].ledIndex;
          memcpy(pkt.data + pkt.dataLen, &idx, 2); pkt.dataLen += 2;
        }
      }
      break;
    }

#ifdef EFFECT_PONG
    case MODE_PONG: {
      extern int16_t pongBallX, pongBallY, pongBallVX, pongBallVY;
      extern int16_t pongPaddleLeftY, pongPaddleRightY;
      extern uint8_t pongScoreLeft, pongScoreRight;
      extern bool pongGameOver;
      if (pkt.dataLen + 15 <= 200) {
        memcpy(pkt.data + pkt.dataLen, &pongBallX, 2);      pkt.dataLen += 2;
        memcpy(pkt.data + pkt.dataLen, &pongBallY, 2);      pkt.dataLen += 2;
        memcpy(pkt.data + pkt.dataLen, &pongBallVX, 2);     pkt.dataLen += 2;
        memcpy(pkt.data + pkt.dataLen, &pongBallVY, 2);     pkt.dataLen += 2;
        memcpy(pkt.data + pkt.dataLen, &pongPaddleLeftY, 2); pkt.dataLen += 2;
        memcpy(pkt.data + pkt.dataLen, &pongPaddleRightY, 2); pkt.dataLen += 2;
        pkt.data[pkt.dataLen++] = pongScoreLeft;
        pkt.data[pkt.dataLen++] = pongScoreRight;
        pkt.data[pkt.dataLen++] = pongGameOver ? 1 : 0;
      }
      break;
    }
#endif

#ifdef EFFECT_SCROLLTEXT
    case MODE_SCROLLTEXT: {
      extern int scrollPackSyncData(uint8_t* data, int maxLen);
      int written = scrollPackSyncData(pkt.data + pkt.dataLen, 200 - pkt.dataLen);
      pkt.dataLen += written;
      break;
    }
#endif

    default:
      // Nessun dato aggiuntivo necessario
      break;
  }
}

// ===== Processa un pacchetto di sincronizzazione ricevuto =====
// Chiamato dallo slave per applicare lo stato ricevuto dal master
void processSyncPacket(SyncPacket &pkt) {
  // Estrai flag dual bypass dal bit 7 di pkt.mode e pulisci il campo
  bool modeDualBypass = (pkt.mode & 0x80) != 0;
  pkt.mode &= 0x7F;  // Pulisci bit 7, ora pkt.mode contiene solo il modo (0-29)

  // Aggiorna lo stato globale con i dati ricevuti dal master
  bool modeChanged = (currentMode != (DisplayMode)pkt.mode);

  // Rileva se il tempo e' cambiato PRIMA di applicare i nuovi valori
  bool timeChanged = (currentHour != pkt.hours || currentMinute != pkt.minutes);

  // Se la modalita' e' cambiata:
  // 1. Cleanup vecchia modalita' (ferma audio, chiude decoder, resetta flag vecchio modo)
  // 2. Resetta TUTTI i flag di init (cosi' il nuovo modo si reinizializza)
  if (modeChanged) {
    Serial.printf("[DUAL] Cambio modalita' da master: %d -> %d\n", (int)currentMode, pkt.mode);
    extern void cleanupPreviousMode(DisplayMode previousMode);
    cleanupPreviousMode(currentMode);
    resetModeInitFlags();  // Resetta TUTTI i flag (vecchio + nuovo modo)
    // Forza ridisegno per modi time-based (FAST, SLOW, FADE etc.)
    // Senza questo, lastHour==currentHour e il modo non ridisegna (schermo nero)
    extern uint8_t lastHour, lastMinute;
    lastHour = 255;
    lastMinute = 255;
  }

  currentMode    = (DisplayMode)pkt.mode;
  currentHour    = pkt.hours;
  currentMinute  = pkt.minutes;
  currentSecond  = pkt.seconds;
  dualFrameCounter = pkt.frameCounter;
  currentColor.r = pkt.colorR;
  currentColor.g = pkt.colorG;
  currentColor.b = pkt.colorB;
  // Se il radar server ha comandato display OFF, non sovrascrivere la luminosita'
  // Salva comunque il valore del master per ripristinarlo quando torna la presenza
  extern bool radarServerEnabled;
  extern bool radarRemotePresence;
  if (radarServerEnabled && !radarRemotePresence) {
    // Radar dice OFF: mantieni lastAppliedBrightness a 0, salva valore master a parte
    // (il loop principale forzera' PWM a 0)
  } else {
    lastAppliedBrightness = pkt.brightness;
  }
  currentPreset  = pkt.preset;
  rainbowModeEnabled = (pkt.rainbowEnabled != 0);

  // Applica impostazioni sincronizzate dal master
  brightnessDay   = pkt.brightnessDay;
  brightnessNight = pkt.brightnessNight;
  volumeDay       = pkt.volumeDay;
  volumeNight     = pkt.volumeNight;
  enabledModesMask = (uint64_t)pkt.enabledMask0
                   | ((uint64_t)pkt.enabledMask1 << 8)
                   | ((uint64_t)pkt.enabledMask2 << 16)
                   | ((uint64_t)pkt.enabledMask3 << 24)
                   | ((uint64_t)pkt.enabledMaskExt << 32);
  ledRgbEnabled    = (pkt.ledEnabled != 0);
  ledRgbBrightness = pkt.ledBrightness;
  ledRgbOverride   = (pkt.ledOverride != 0);
  ledRgbOverrideR  = pkt.ledOverR;
  ledRgbOverrideG  = pkt.ledOverG;
  ledRgbOverrideB  = pkt.ledOverB;
  ledAudioReactive = pkt.ledAudioReact;

  // Aggiorna dualModesMask dello slave per il modo corrente in base al master
  // Rileva se lo stato bypass e' cambiato per questo modo (senza cambio modo)
  bool wasDualEnabled = isModeDualEnabled(pkt.mode);
  if (modeDualBypass) {
    dualModesMask &= ~(1ULL << pkt.mode);  // Bit off: modo non usa dual
  } else {
    dualModesMask |= (1ULL << pkt.mode);   // Bit on: modo usa dual
  }
  bool nowDualEnabled = isModeDualEnabled(pkt.mode);

  // FIX: Se bypass e' cambiato senza cambio modo, forza reset display
  if (!modeChanged && wasDualEnabled != nowDualEnabled) {
    Serial.printf("[DUAL] Slave: bypass cambiato per mode %d: dual=%d->%d\n",
                  pkt.mode, wasDualEnabled, nowDualEnabled);
    resetModeInitFlags();
    // Forza ridisegno per modi time-based
    extern uint8_t lastHour, lastMinute;
    lastHour = 255;
    lastMinute = 255;
  }

  // Luminosita' display applicata dal blocco brightness nel loop principale
  // (non chiamare ledcWrite qui per evitare glitch PWM a 30fps)

  // ANTI-FLICKER: Se il tempo e il modo NON sono cambiati, forza lastHour/lastMinute
  // in sync con currentHour/currentMinute. Questo previene ridisegni accidentali
  // causati da race condition, corruzioni di stato, o path di codice non previste.
  // Quando il tempo O il modo O il bypass cambiano, lascia che la funzione del modo faccia UN solo redraw.
  bool bypassChanged = (!modeChanged && wasDualEnabled != nowDualEnabled);
  if (!modeChanged && !timeChanged && !bypassChanged) {
    extern uint8_t lastHour, lastMinute;
    lastHour = currentHour;
    lastMinute = currentMinute;
  }

  // Processa dati mode-specific se presenti
  if (pkt.dataLen > 0) {
    unpackModeSpecificData(pkt);
  }
}

// ===== Reset dei flag di inizializzazione di TUTTE le modalita' =====
// Quando il master cambia modo, lo slave deve reinizializzare il display
// Resetta TUTTI i flag indipendentemente dalla modalita' corrente
void resetModeInitFlags() {

  // --- Pattern "Initialized = false" ---
#ifdef EFFECT_MP3_PLAYER
  extern bool mp3PlayerInitialized;
  mp3PlayerInitialized = false;
#endif
#ifdef EFFECT_WEB_RADIO
  extern bool webRadioInitialized;
  webRadioInitialized = false;
#endif
#ifdef EFFECT_RADIO_ALARM
  extern bool radioAlarmInitialized;
  radioAlarmInitialized = false;
#endif
#ifdef EFFECT_MJPEG_STREAM
  extern bool mjpegInitialized;
  mjpegInitialized = false;
#endif
#ifdef EFFECT_ESP32CAM
  extern bool esp32camInitialized;
  esp32camInitialized = false;
#endif
#ifdef EFFECT_BTTF
  extern bool bttfInitialized;
  bttfInitialized = false;
#endif
#ifdef EFFECT_FLIP_CLOCK
  extern bool flipClockInitialized;
  flipClockInitialized = false;
#endif
#ifdef EFFECT_LED_RING
  extern bool ledRingInitialized;
  ledRingInitialized = false;
#endif
#ifdef EFFECT_FLUX_CAPACITOR
  extern bool fluxCapacitorInitialized;
  fluxCapacitorInitialized = false;
#endif
  extern bool matrixInitialized;
  matrixInitialized = false;
  extern bool tronInitialized;
  tronInitialized = false;
#ifdef EFFECT_WEATHER_STATION
  extern bool weatherStationInitialized;
  weatherStationInitialized = false;
#endif
#ifdef EFFECT_CALENDAR
  extern bool calendarStationInitialized;
  calendarStationInitialized = false;
#endif
#ifdef EFFECT_YOUTUBE
  extern bool youtubeInitialized;
  youtubeInitialized = false;
#endif
#ifdef EFFECT_NEWS
  extern bool newsInitialized;
  newsInitialized = false;
#endif
#ifdef EFFECT_GEMINI_AI
  extern bool geminiInitialized;
  geminiInitialized = false;
#endif
  extern bool fadeInitialized;
  fadeInitialized = false;
  extern bool slowInitialized;
  slowInitialized = false;
#ifdef EFFECT_PONG
  extern bool pongInitialized;
  pongInitialized = false;
#endif
#ifdef EFFECT_SCROLLTEXT
  extern bool scrollTextInitialized;
  scrollTextInitialized = false;
#endif

  // --- Pattern "InitNeeded = true" ---
#ifdef EFFECT_CLOCK
  extern bool clockInitNeeded;
  clockInitNeeded = true;
#endif
  extern bool snakeInitNeeded;
  snakeInitNeeded = true;
  extern bool waterDropInitNeeded;
  waterDropInitNeeded = true;
  extern bool marioInitNeeded;
  marioInitNeeded = true;
#ifdef EFFECT_GALAGA
  extern bool galagaInitNeeded;
  galagaInitNeeded = true;
#endif
#ifdef EFFECT_GALAGA2
  extern bool galaga2InitNeeded;
  galaga2InitNeeded = true;
#endif
#ifdef EFFECT_ANALOG_CLOCK
  extern bool analogClockInitNeeded;
  analogClockInitNeeded = true;
#endif

  Serial.println("[DUAL] TUTTI i flag inizializzazione resettati");
}

// ===== Estrai dati mode-specific dal pacchetto sync =====
void unpackModeSpecificData(const SyncPacket &pkt) {
  switch ((DisplayMode)pkt.mode) {

    case MODE_MATRIX:
    case MODE_MATRIX2: {
      // Ripristina il seed random per generare la stessa pioggia
      if (pkt.dataLen >= 4) {
        uint32_t seed;
        memcpy(&seed, pkt.data, 4);
        // Il seed verra' usato dal codice della modalita' matrix
        // per generare posizioni coerenti tra tutti i pannelli
      }
      break;
    }

    case MODE_SNAKE: {
      // Ripristina stato completo del serpente dal master
      extern uint8_t snakePathChoice;
      extern uint16_t snakePathIndex;
      extern bool snakeIsCompleted;
      extern Snake snake;
      extern bool snakeInitNeeded;
      if (pkt.dataLen >= 20) {
        uint8_t offset = 0;
        uint8_t newPathChoice = pkt.data[offset++];
        // Se il percorso cambia, forza re-init per aggiornare currentPath
        if (newPathChoice != snakePathChoice) {
          snakePathChoice = newPathChoice;
          snakeInitNeeded = true;
        }
        memcpy(&snakePathIndex, pkt.data + offset, 2); offset += 2;
        snakeIsCompleted = (pkt.data[offset++] != 0);
        for (uint8_t i = 0; i < 8; i++) {
          memcpy(&snake.segments[i].ledIndex, pkt.data + offset, 2); offset += 2;
        }
      }
      break;
    }

#ifdef EFFECT_PONG
    case MODE_PONG: {
      extern int16_t pongBallX, pongBallY, pongBallVX, pongBallVY;
      extern int16_t pongPaddleLeftY, pongPaddleRightY;
      extern uint8_t pongScoreLeft, pongScoreRight;
      extern bool pongGameOver;
      if (pkt.dataLen >= 15) {
        int off = 0;
        memcpy(&pongBallX, pkt.data + off, 2);      off += 2;
        memcpy(&pongBallY, pkt.data + off, 2);      off += 2;
        memcpy(&pongBallVX, pkt.data + off, 2);     off += 2;
        memcpy(&pongBallVY, pkt.data + off, 2);     off += 2;
        memcpy(&pongPaddleLeftY, pkt.data + off, 2); off += 2;
        memcpy(&pongPaddleRightY, pkt.data + off, 2); off += 2;
        pongScoreLeft = pkt.data[off++];
        pongScoreRight = pkt.data[off++];
        pongGameOver = (pkt.data[off] != 0);
      }
      break;
    }
#endif

#ifdef EFFECT_SCROLLTEXT
    case MODE_SCROLLTEXT: {
      extern void scrollUnpackSyncData(const uint8_t* data, int dataLen);
      scrollUnpackSyncData(pkt.data, pkt.dataLen);
      break;
    }
#endif

    default:
      break;
  }
}

// ===== Invia heartbeat =====
// Segnale di vita inviato ogni secondo da tutti i pannelli (master e slave)
void sendHeartbeat() {
  if (peerCount == 0 && !broadcastPeerAdded) return;

  SyncPacket hb;
  memset(&hb, 0, sizeof(hb));
  hb.magic      = DUAL_MAGIC;
  hb.packetType = PKT_HEARTBEAT;
  hb.mode       = (uint8_t)currentMode;
  hb.hours      = currentHour;
  hb.minutes    = currentMinute;
  hb.seconds    = currentSecond;
  hb.dataLen    = 0;

  // Dimensione ridotta per heartbeat (solo l'header)
  size_t hbSize = sizeof(SyncPacket) - 200;

  // Invia a tutti i peer
  for (int i = 0; i < (int)peerCount; i++) {
    esp_now_send(peerMACs[i], (uint8_t*)&hb, hbSize);
  }
}

// ===== Inoltra evento touch al master =====
// Chiamato dallo slave quando rileva un tocco locale
void forwardTouchToMaster(int touchX, int touchY, uint8_t touchType) {
  if (panelRole != 2) return; // Solo lo slave inoltra
  if (peerCount == 0) return; // Nessun master registrato

  TouchPacket tp;
  memset(&tp, 0, sizeof(tp));
  tp.magic      = DUAL_MAGIC;
  tp.packetType = PKT_TOUCH_EVENT;
  tp.touchX     = (int16_t)touchX;
  tp.touchY     = (int16_t)touchY;
  tp.panelX     = panelX;
  tp.panelY     = panelY;
  tp.touchType  = touchType;

  // Invia al primo peer (che dovrebbe essere il master)
  esp_now_send(peerMACs[0], (uint8_t*)&tp, sizeof(tp));

  Serial.printf("[DUAL] Touch inoltrato al master: (%d,%d) tipo=%d\n", touchX, touchY, touchType);
}

// ========================================================================
// =================== EEPROM =============================================
// ========================================================================

// ===== Carica impostazioni dual display dalla EEPROM =====
void loadDualDisplaySettings() {
  // Verifica marker di validita'
  if (EEPROM.read(EEPROM_DUAL_MARKER) != EEPROM_DUAL_MARKER_VALUE) {
    Serial.println("[DUAL] Nessuna configurazione salvata in EEPROM, uso valori predefiniti");
    // Mantieni i valori di default (disabled, standalone)
    dualDisplayEnabled = false;
    panelX    = 0;
    panelY    = 0;
    gridW     = 1;
    gridH     = 1;
    panelRole = 0;
    peerCount = 0;
    memset(peerMACs, 0, sizeof(peerMACs));
    return;
  }

  dualDisplayEnabled = EEPROM.read(EEPROM_DUAL_ENABLED) != 0;
  panelX    = EEPROM.read(EEPROM_DUAL_PANEL_X);
  panelY    = EEPROM.read(EEPROM_DUAL_PANEL_Y);
  gridW     = EEPROM.read(EEPROM_DUAL_GRID_W);
  gridH     = EEPROM.read(EEPROM_DUAL_GRID_H);
  panelRole = EEPROM.read(EEPROM_DUAL_ROLE);

  // Sanitize dei valori
  if (panelX > 1) panelX = 0;
  if (panelY > 1) panelY = 0;
  if (gridW < 1 || gridW > 2) gridW = 1;
  if (gridH < 1 || gridH > 2) gridH = 1;
  if (panelRole > 2) panelRole = 0;

  // Carica MAC dei peer
  peerCount = 0;
  for (int i = 0; i < 3; i++) {
    int baseAddr = EEPROM_DUAL_PEER1_MAC + i * 6;
    uint8_t mac[6];
    bool allZero = true;
    for (int j = 0; j < 6; j++) {
      mac[j] = EEPROM.read(baseAddr + j);
      if (mac[j] != 0) allZero = false;
    }
    if (!allZero) {
      memcpy(peerMACs[peerCount], mac, 6);
      peerCount++;
    }
  }

  Serial.printf("[DUAL] Impostazioni caricate: enabled=%d, pos=(%d,%d), griglia=%dx%d, ruolo=%d, peer=%d\n",
                dualDisplayEnabled, panelX, panelY, gridW, gridH, panelRole, peerCount);

  for (int i = 0; i < (int)peerCount; i++) {
    Serial.printf("[DUAL]   Peer[%d]: %s\n", i, macToString(peerMACs[i]).c_str());
  }
}

// ===== Salva impostazioni dual display nella EEPROM =====
// Ritorna true se EEPROM.commit() ha successo, false altrimenti
bool saveDualDisplaySettings() {
  EEPROM.write(EEPROM_DUAL_ENABLED, dualDisplayEnabled ? 1 : 0);
  EEPROM.write(EEPROM_DUAL_PANEL_X, panelX);
  EEPROM.write(EEPROM_DUAL_PANEL_Y, panelY);
  EEPROM.write(EEPROM_DUAL_GRID_W, gridW);
  EEPROM.write(EEPROM_DUAL_GRID_H, gridH);
  EEPROM.write(EEPROM_DUAL_ROLE, panelRole);

  // Salva MAC dei peer (3 slot da 6 bytes ciascuno)
  for (int i = 0; i < 3; i++) {
    int baseAddr = EEPROM_DUAL_PEER1_MAC + i * 6;
    for (int j = 0; j < 6; j++) {
      if (i < (int)peerCount) {
        EEPROM.write(baseAddr + j, peerMACs[i][j]);
      } else {
        EEPROM.write(baseAddr + j, 0); // Azzera slot vuoti
      }
    }
  }

  // Scrivi il marker di validita'
  EEPROM.write(EEPROM_DUAL_MARKER, EEPROM_DUAL_MARKER_VALUE);
  bool ok = EEPROM.commit();

  if (ok) {
    Serial.println("[DUAL] Impostazioni salvate in EEPROM");
  } else {
    Serial.println("[DUAL] ERRORE: EEPROM.commit() fallito!");
  }

  // Verifica immediata: rileggi il marker per confermare il salvataggio
  uint8_t verify = EEPROM.read(EEPROM_DUAL_MARKER);
  if (verify != EEPROM_DUAL_MARKER_VALUE) {
    Serial.printf("[DUAL] ERRORE VERIFICA: marker letto = 0x%02X (atteso 0x%02X)\n", verify, EEPROM_DUAL_MARKER_VALUE);
    ok = false;
  }

  return ok;
}

// ========================================================================
// =================== CONFIG PUSH (MASTER → SLAVE) =======================
// ========================================================================

// Invia la configurazione corrente (incluso enabled) a tutti i peer registrati
void sendConfigPushToAllPeers() {
  if (!espNowInitialized || peerCount == 0) return;

  DiscoveryPacket pkt;
  memset(&pkt, 0, sizeof(pkt));
  pkt.magic      = DUAL_MAGIC;
  pkt.packetType = PKT_CONFIG_PUSH;
  pkt.role       = panelRole;
  pkt.gridW      = gridW;
  pkt.gridH      = gridH;
  pkt.enabled    = dualDisplayEnabled ? 1 : 0;
  WiFi.macAddress(pkt.mac);

  for (uint8_t i = 0; i < peerCount; i++) {
    pkt.assignedX = peerStates[i].peerPanelX;
    pkt.assignedY = peerStates[i].peerPanelY;
    esp_err_t result = esp_now_send(peerMACs[i], (uint8_t*)&pkt, sizeof(pkt));
    Serial.printf("[DUAL] Config push a %s: enabled=%d grid=%dx%d result=%d\n",
                  macToString(peerMACs[i]).c_str(), pkt.enabled, gridW, gridH, result);
  }
}

// ========================================================================
// =================== INIZIALIZZAZIONE ===================================
// ========================================================================

// ===== Inizializza il sistema dual display =====
// Deve essere chiamato dopo WiFi.mode() e prima di clockWebServer->begin()
void initDualDisplay() {
  Serial.println("[DUAL] === Inizializzazione Dual Display ===");

  // Carica configurazione dalla EEPROM
  loadDualDisplaySettings();

  // Carica la maschera per-mode dual display
  loadDualModesMask();

  // Se non abilitato
  if (!dualDisplayEnabled) {
    dualDisplayInitialized = false;
    initDualGfxProxy(); // Ripristina gfx = realGfx

    // SLAVE: anche se disabilitato, tiene ESP-NOW attivo per ricevere
    // config push dal master (che puo' ri-abilitarlo da remoto)
    if (panelRole == 2 && !espNowInitialized) {
      Serial.println("[DUAL] Slave disabilitato: avvio ESP-NOW in ascolto per comandi master");
      esp_err_t result = esp_now_init();
      if (result == ESP_OK) {
        espNowInitialized = true;
        esp_now_register_send_cb(onDualDataSent);
        esp_now_register_recv_cb(onDualDataRecv);
        // Registra broadcast peer per discovery
        if (!broadcastPeerAdded) {
          esp_now_peer_info_t peerInfo;
          memset(&peerInfo, 0, sizeof(peerInfo));
          memcpy(peerInfo.peer_addr, broadcastMAC, 6);
          peerInfo.channel = 0;
          peerInfo.encrypt = false;
          if (esp_now_add_peer(&peerInfo) == ESP_OK) broadcastPeerAdded = true;
        }
        // Registra peer salvati (master)
        for (int i = 0; i < (int)peerCount; i++) {
          esp_now_peer_info_t pi;
          memset(&pi, 0, sizeof(pi));
          memcpy(pi.peer_addr, peerMACs[i], 6);
          pi.channel = 0; pi.encrypt = false;
          esp_now_add_peer(&pi);
        }
      }
    } else {
      Serial.println("[DUAL] Dual Display disabilitato, skip inizializzazione ESP-NOW");
    }
    return;
  }

  // Stampa info MAC locale
  uint8_t myMAC[6];
  WiFi.macAddress(myMAC);
  Serial.printf("[DUAL] MAC locale: %s\n", macToString(myMAC).c_str());
  Serial.printf("[DUAL] Ruolo: %s\n",
                panelRole == 0 ? "STANDALONE" :
                panelRole == 1 ? "MASTER" : "SLAVE");
  Serial.printf("[DUAL] Posizione: (%d,%d) in griglia %dx%d\n", panelX, panelY, gridW, gridH);

  // Inizializza ESP-NOW (solo se non gia' inizializzato)
  // NOTA: WiFi deve gia' essere inizializzato (WiFi.mode(WIFI_STA) o WIFI_AP_STA)
  // ESP-NOW funziona in parallelo con WiFi
  if (!espNowInitialized) {
    esp_err_t result = esp_now_init();
    if (result != ESP_OK) {
      Serial.printf("[DUAL] ERRORE inizializzazione ESP-NOW: %d\n", result);
      dualDisplayInitialized = false;
      return;
    }

    espNowInitialized = true;
    Serial.println("[DUAL] ESP-NOW inizializzato con successo");

    // Registra callback per invio e ricezione
    esp_now_register_send_cb(onDualDataSent);
    esp_now_register_recv_cb(onDualDataRecv);
  } else {
    Serial.println("[DUAL] ESP-NOW gia' inizializzato, ri-configurazione");
  }

  // Registra il peer broadcast per discovery
  if (!broadcastPeerAdded) {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_err_t res = esp_now_add_peer(&peerInfo);
    if (res == ESP_OK || res == ESP_ERR_ESPNOW_EXIST) {
      broadcastPeerAdded = true;
      Serial.println("[DUAL] Peer broadcast registrato");
    }
  }

  // Registra i peer salvati in EEPROM
  for (int i = 0; i < (int)peerCount; i++) {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, peerMACs[i], 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_err_t res = esp_now_add_peer(&peerInfo);
    if (res == ESP_OK || res == ESP_ERR_ESPNOW_EXIST) {
      Serial.printf("[DUAL] Peer[%d] registrato in ESP-NOW: %s\n", i, macToString(peerMACs[i]).c_str());
      peerStates[i].active = false; // Verra' attivato al primo heartbeat
      peerStates[i].lastHeartbeat = 0;
      memcpy(peerStates[i].mac, peerMACs[i], 6);
    } else {
      Serial.printf("[DUAL] ERRORE registrazione peer[%d]: %d\n", i, res);
    }
  }

  // Se siamo uno slave senza peer (master) configurato, avvia discovery
  if (panelRole == 2 && peerCount == 0) {
    Serial.println("[DUAL] Slave senza master, avvio discovery automatico...");
    scanForPanels();
  }

  dualDisplayInitialized = true;
  dualFrameCounter = 0;

  // Attiva/disattiva proxy DualGFX in base alla griglia
  initDualGfxProxy();

  Serial.println("[DUAL] === Inizializzazione completata ===");
}

// ===== Reinizializza solo ESP-NOW senza ri-leggere config da EEPROM =====
// Usata dal webserver POST handler per evitare di sovrascrivere i valori appena salvati
void reinitDualDisplayEspNow() {
  Serial.println("[DUAL] === Reinizializzazione ESP-NOW (config gia' in RAM) ===");

  if (!dualDisplayEnabled) {
    dualDisplayInitialized = false;
    Serial.println("[DUAL] Dual Display disabilitato");

    // SLAVE: NON deinizializzare ESP-NOW, resta in ascolto per comandi master
    if (panelRole == 2) {
      Serial.println("[DUAL] Slave: ESP-NOW resta attivo per ricevere comandi master");
      // Inizializza ESP-NOW se non era attivo
      if (!espNowInitialized) {
        esp_err_t result = esp_now_init();
        if (result == ESP_OK) {
          espNowInitialized = true;
          esp_now_register_send_cb(onDualDataSent);
          esp_now_register_recv_cb(onDualDataRecv);
        }
      }
      // Registra broadcast peer se necessario
      if (espNowInitialized && !broadcastPeerAdded) {
        esp_now_peer_info_t peerInfo;
        memset(&peerInfo, 0, sizeof(peerInfo));
        memcpy(peerInfo.peer_addr, broadcastMAC, 6);
        peerInfo.channel = 0; peerInfo.encrypt = false;
        esp_err_t res = esp_now_add_peer(&peerInfo);
        if (res == ESP_OK || res == ESP_ERR_ESPNOW_EXIST) broadcastPeerAdded = true;
      }
      // Registra peer salvati
      if (espNowInitialized) {
        for (int i = 0; i < (int)peerCount; i++) {
          esp_now_peer_info_t pi;
          memset(&pi, 0, sizeof(pi));
          memcpy(pi.peer_addr, peerMACs[i], 6);
          pi.channel = 0; pi.encrypt = false;
          esp_now_add_peer(&pi); // OK se gia' esiste
        }
      }
    } else {
      // MASTER/STANDALONE: deinizializza ESP-NOW se era attivo
      if (espNowInitialized) {
        esp_now_deinit();
        espNowInitialized = false;
        broadcastPeerAdded = false;
      }
    }

    initDualGfxProxy(); // Ripristina gfx = realGfx
    return;
  }

  // Inizializza ESP-NOW se necessario
  if (!espNowInitialized) {
    esp_err_t result = esp_now_init();
    if (result != ESP_OK) {
      Serial.printf("[DUAL] ERRORE inizializzazione ESP-NOW: %d\n", result);
      dualDisplayInitialized = false;
      return;
    }
    espNowInitialized = true;
    esp_now_register_send_cb(onDualDataSent);
    esp_now_register_recv_cb(onDualDataRecv);
  }

  // Registra peer broadcast
  if (!broadcastPeerAdded) {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_err_t res = esp_now_add_peer(&peerInfo);
    if (res == ESP_OK || res == ESP_ERR_ESPNOW_EXIST) {
      broadcastPeerAdded = true;
    }
  }

  // Registra i peer
  for (int i = 0; i < (int)peerCount; i++) {
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, peerMACs[i], 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo); // OK se gia' esiste
    peerStates[i].active = false;
    peerStates[i].lastHeartbeat = 0;
    memcpy(peerStates[i].mac, peerMACs[i], 6);
  }

  if (panelRole == 2 && peerCount == 0) {
    scanForPanels();
  }

  dualDisplayInitialized = true;

  // Attiva/disattiva proxy DualGFX in base alla griglia
  initDualGfxProxy();

  Serial.printf("[DUAL] Reinizializzazione completata: pos=(%d,%d) grid=%dx%d role=%d\n",
                panelX, panelY, gridW, gridH, panelRole);
}

// ========================================================================
// =================== UPDATE LOOP ========================================
// ========================================================================

// ===== Aggiornamento dual display (chiamato dal loop principale) =====
void updateDualDisplay() {

  // === Gestione flash pannello (richiesto dal webserver, eseguito nel loop per thread-safety SPI) ===
  if (dualFlashRequested) {
    dualFlashRequested = false;
    Serial.println("[DUAL] Flash pannello in corso...");
    for (int i = 0; i < 3; i++) {
      gfx->fillScreen(WHITE);
      delay(150);
      gfx->fillScreen(BLACK);
      delay(150);
    }
    // Forza ridisegno della modalita' corrente
    resetModeInitFlags();
    Serial.println("[DUAL] Flash completato, ridisegno forzato");
  }

  // === Test sync richiesto dal webserver ===
  if (dualTestSyncRequested) {
    dualTestSyncRequested = false;
    if (dualDisplayEnabled && dualDisplayInitialized && peerCount > 0) {
      sendSyncPacket();
      sendHeartbeat();
      Serial.printf("[DUAL] Test sync inviato a %d peer\n", peerCount);
    }
  }

  // Se disabilitato o non inizializzato, esci subito
  if (!dualDisplayEnabled || !dualDisplayInitialized) return;

  static unsigned long lastSync = 0;
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastPeerCheck = 0;
  unsigned long now = millis();

  // === MASTER: Invia stato sincronizzato a tutti gli slave ===
  if (panelRole == 1 && now - lastSync >= DUAL_SYNC_INTERVAL_MS) {
    sendSyncPacket();
    lastSync = now;
  }

  // === SLAVE: Processa pacchetti sync ricevuti ===
  // Copia locale protetta da mutex per evitare race condition con callback ESP-NOW
  if (panelRole == 2 && newSyncAvailable) {
    SyncPacket localPkt;
    portENTER_CRITICAL(&syncMux);
    memcpy(&localPkt, &lastReceivedSync, sizeof(SyncPacket));
    newSyncAvailable = false;
    portEXIT_CRITICAL(&syncMux);
    processSyncPacket(localPkt);
  }

  // === TUTTI: Invia heartbeat ogni secondo ===
  if (now - lastHeartbeat >= DUAL_HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeat = now;
  }

  // === Discovery in corso: invia broadcast periodici ===
  if (discoveryInProgress) {
    // Timeout discovery dopo 30 secondi
    if (now - discoveryStart > 30000) {
      discoveryInProgress = false;
      Serial.println("[DUAL] Discovery terminato (timeout 30s)");
    } else if (now - lastDiscoveryBroadcast >= DUAL_DISCOVERY_INTERVAL) {
      sendDiscoveryBroadcast();
      lastDiscoveryBroadcast = now;
    }
  }

  // === Controllo stato peer ogni 2 secondi ===
  if (now - lastPeerCheck >= 2000) {
    lastPeerCheck = now;
    checkPeerHealth();
  }
}

// ===== Verifica lo stato di salute dei peer =====
// Controlla i timeout degli heartbeat e marca i peer disconnessi
void checkPeerHealth() {
  unsigned long now = millis();

  for (int i = 0; i < (int)peerCount; i++) {
    if (peerStates[i].active) {
      // Verifica se il peer ha inviato un heartbeat recente
      if (peerStates[i].lastHeartbeat > 0 &&
          now - peerStates[i].lastHeartbeat > DUAL_PEER_TIMEOUT) {
        peerStates[i].active = false;
        Serial.printf("[DUAL] AVVISO: Peer[%d] %s non risponde (timeout %ds)\n",
                      i, macToString(peerMACs[i]).c_str(), DUAL_PEER_TIMEOUT / 1000);
      }
    } else {
      // Peer inattivo: controlla se ha ripreso a rispondere
      if (peerStates[i].lastHeartbeat > 0 &&
          now - peerStates[i].lastHeartbeat <= DUAL_PEER_TIMEOUT) {
        peerStates[i].active = true;
        Serial.printf("[DUAL] Peer[%d] %s riconnesso!\n",
                      i, macToString(peerMACs[i]).c_str());
      }
    }
  }
}

// ========================================================================
// =================== UTILITA' ===========================================
// ========================================================================

// ===== Ottieni il numero di peer attivi =====
int getActivePeerCount() {
  int count = 0;
  for (int i = 0; i < (int)peerCount; i++) {
    if (peerStates[i].active) count++;
  }
  return count;
}

// ===== Verifica se il sistema dual e' operativo =====
// Ritorna true se dual display e' abilitato E almeno un peer e' connesso
bool isDualDisplayActive() {
  return dualDisplayEnabled && dualDisplayInitialized && getActivePeerCount() > 0;
}

// ===== Verifica se questo pannello e' il master =====
bool isDualMaster() {
  return dualDisplayEnabled && panelRole == 1;
}

// ===== Verifica se questo pannello e' uno slave =====
bool isDualSlave() {
  return dualDisplayEnabled && panelRole == 2;
}

// ===== Ottieni informazioni di stato per la pagina web =====
String getDualDisplayStatusJson() {
  String json = "{";
  json += "\"enabled\":" + String(dualDisplayEnabled ? "true" : "false") + ",";
  json += "\"initialized\":" + String(dualDisplayInitialized ? "true" : "false") + ",";
  json += "\"role\":" + String(panelRole) + ",";
  json += "\"panelX\":" + String(panelX) + ",";
  json += "\"panelY\":" + String(panelY) + ",";
  json += "\"gridW\":" + String(gridW) + ",";
  json += "\"gridH\":" + String(gridH) + ",";

  uint8_t myMAC[6];
  WiFi.macAddress(myMAC);
  json += "\"mac\":\"" + macToString(myMAC) + "\",";

  json += "\"frameCounter\":" + String(dualFrameCounter) + ",";
  json += "\"peerCount\":" + String(peerCount) + ",";
  json += "\"activePeers\":" + String(getActivePeerCount()) + ",";
  json += "\"discovery\":" + String(discoveryInProgress ? "true" : "false") + ",";

  json += "\"peers\":[";
  for (int i = 0; i < (int)peerCount; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"mac\":\"" + macToString(peerMACs[i]) + "\",";
    json += "\"active\":" + String(peerStates[i].active ? "true" : "false") + ",";
    json += "\"panelX\":" + String(peerStates[i].peerPanelX) + ",";
    json += "\"panelY\":" + String(peerStates[i].peerPanelY);
    json += "}";
  }
  json += "]";

  json += "}";
  return json;
}

// ===== Reset configurazione dual display =====
// Riporta tutto ai valori di default e disabilita il dual display
void resetDualDisplay() {
  Serial.println("[DUAL] Reset configurazione completa");

  // Rimuovi tutti i peer da ESP-NOW
  for (int i = (int)peerCount - 1; i >= 0; i--) {
    esp_now_del_peer(peerMACs[i]);
  }

  // Reset variabili
  dualDisplayEnabled = false;
  panelX    = 0;
  panelY    = 0;
  gridW     = 1;
  gridH     = 1;
  panelRole = 0;
  peerCount = 0;
  dualFrameCounter = 0;
  dualDisplayInitialized = false;
  discoveryInProgress = false;
  newSyncAvailable = false;

  memset(peerMACs, 0, sizeof(peerMACs));
  memset(peerStates, 0, sizeof(peerStates));

  // Salva lo stato resettato
  saveDualDisplaySettings();

  // Deinizializza ESP-NOW se era attivo
  if (espNowInitialized) {
    esp_now_deinit();
    espNowInitialized = false;
    broadcastPeerAdded = false;
    Serial.println("[DUAL] ESP-NOW deinizializzato");
  }

  Serial.println("[DUAL] Reset completato");
}

// ===== Configurazione rapida come master =====
// Imposta questo pannello come master in posizione (0,0) con griglia specificata
void setupAsMaster(uint8_t gw, uint8_t gh) {
  Serial.printf("[DUAL] Configurazione come MASTER in griglia %dx%d\n", gw, gh);

  dualDisplayEnabled = true;
  panelRole = 1;      // Master
  panelX    = 0;      // Angolo in alto a sinistra
  panelY    = 0;
  gridW     = (gw >= 1 && gw <= 2) ? gw : 2;
  gridH     = (gh >= 1 && gh <= 2) ? gh : 1;

  saveDualDisplaySettings();

  // Se ESP-NOW non e' inizializzato, inizializzalo
  if (!espNowInitialized) {
    initDualDisplay();
  }

  Serial.printf("[DUAL] Master configurato: griglia %dx%d, canvas virtuale %dx%d\n",
                gridW, gridH, virtualWidth(), virtualHeight());
}

// ===== Configurazione rapida come slave =====
// Imposta questo pannello come slave e avvia la discovery del master
void setupAsSlave() {
  Serial.println("[DUAL] Configurazione come SLAVE");

  dualDisplayEnabled = true;
  panelRole = 2;      // Slave
  panelX    = 0;      // Sara' assegnato dal master durante discovery
  panelY    = 0;
  gridW     = 1;      // Sara' aggiornato dal master
  gridH     = 1;

  saveDualDisplaySettings();

  // Se ESP-NOW non e' inizializzato, inizializzalo
  if (!espNowInitialized) {
    initDualDisplay();
  } else {
    // Gia' inizializzato, avvia solo discovery
    scanForPanels();
  }

  Serial.println("[DUAL] Slave configurato, discovery in corso...");
}
