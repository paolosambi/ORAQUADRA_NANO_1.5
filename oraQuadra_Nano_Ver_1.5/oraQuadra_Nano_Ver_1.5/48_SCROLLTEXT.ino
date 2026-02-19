// ================== SCROLL TEXT - DISPLAY MODE ==================
// Mostra testo scorrevole animato sul display 480x480
// Supporta fino a 10 messaggi con rotazione automatica
// Scorrimento orizzontale (sx/dx), font scalabili, effetti rainbow e fade
// Configurazione completa via pagina web /scrolltext

#ifdef EFFECT_SCROLLTEXT

// ===== Configurazione =====
#define MAX_SCROLL_MESSAGES  10
#define MAX_SCROLL_TEXT_LEN  200
#define SCROLL_FRAME_MS      16   // ~60fps
#define SCROLL_FADE_PIXELS   60   // Larghezza zona fade bordi

// ===== EEPROM Addresses =====
#define EEPROM_SCROLLTEXT_MARKER_ADDR  933
#define EEPROM_SCROLLTEXT_MARKER_VAL   0xAE
#define EEPROM_SCROLLTEXT_COUNT        934
#define EEPROM_SCROLLTEXT_CURRENT      935
#define EEPROM_SCROLLTEXT_ROT_HI       936
#define EEPROM_SCROLLTEXT_ROT_LO       937
#define EEPROM_SCROLLTEXT_BG_R         938
#define EEPROM_SCROLLTEXT_BG_G         939
#define EEPROM_SCROLLTEXT_BG_B         940
#define EEPROM_SCROLLTEXT_SHOW_TIME    941
#define EEPROM_SCROLLTEXT_SHOW_DATE    942

// ===== Direzioni scroll (solo orizzontale) =====
enum ScrollDir {
  SDIR_LEFT = 0, SDIR_RIGHT = 1
};

// ===== Effetti (bit flags) =====
#define SFLAG_RAINBOW  0x01
#define SFLAG_FADE     0x02

// ===== Font =====
// Font 0 = "Pixel" (built-in 6x8, scalabile con setTextSize 1-20)
// Font 1+ = U8g2 a dimensione nativa (smooth)
struct ScrollFont {
  const uint8_t* font;   // NULL = built-in pixel font
  const char* name;
  uint8_t height;         // altezza nativa px
};

const ScrollFont SCROLL_FONTS[] = {
  {NULL,                       "Pixel",          8},   // 0: built-in scalabile!
  {u8g2_font_helvB14_tf,       "Sans Bold 14",  14},   // 1
  {u8g2_font_helvB18_tf,       "Sans Bold 18",  18},   // 2
  {u8g2_font_helvB24_tf,       "Sans Bold 24",  24},   // 3
  {u8g2_font_helvR18_tf,       "Sans 18",       18},   // 4
  {u8g2_font_helvR24_tf,       "Sans 24",       24},   // 5
  {u8g2_font_timR18_tf,        "Serif 18",      18},   // 6
  {u8g2_font_timR24_tf,        "Serif 24",      24},   // 7
  {u8g2_font_courR18_tf,       "Mono 18",       18},   // 8
  {u8g2_font_courR24_tf,       "Mono 24",       24},   // 9
  {u8g2_font_fub20_tf,         "Fat 20",        20},   // 10
  {u8g2_font_fub30_tf,         "Fat 30",        30},   // 11
  {u8g2_font_logisoso22_tf,    "Display 22",    22},   // 12
  {u8g2_font_logisoso32_tf,    "Display 32",    32},   // 13
  {u8g2_font_profont22_tf,     "Retro 22",      22},   // 14
  {u8g2_font_fub35_tf,         "Fat 35",        35},   // 15
  {u8g2_font_logisoso38_tf,    "Display 38",    38},   // 16
  {u8g2_font_logisoso42_tr,    "Display 42",    42},   // 17
  {u8g2_font_logisoso54_tr,    "Display 54",    54},   // 18
};
#define SCROLL_FONT_COUNT 19

// ===== Struttura messaggio =====
struct ScrollMessage {
  char text[MAX_SCROLL_TEXT_LEN + 1];
  uint8_t fontIndex;
  uint8_t scale;         // 1-20 (per font Pixel) / ignorato per U8g2
  uint8_t colorR, colorG, colorB;
  uint8_t speed;         // 1-10
  uint8_t direction;     // 0=sinistra, 1=destra
  uint8_t effects;       // SFLAG_RAINBOW | SFLAG_FADE
  bool enabled;
};

// ===== Variabili globali =====
bool scrollTextInitialized = false;
ScrollMessage scrollMessages[MAX_SCROLL_MESSAGES];
uint8_t scrollMessageCount = 0;
uint8_t scrollCurrentMsg = 0;
bool scrollPaused = false;
float scrollPosX = 0, scrollPosY = 0;
unsigned long scrollLastFrame = 0;
unsigned long scrollMsgStartTime = 0;
uint32_t scrollRotationInterval = 10000;
uint8_t scrollBgR = 0, scrollBgG = 0, scrollBgB = 0;
bool scrollShowTime = true;
bool scrollShowDate = true;
float scrollRainbowHue = 0;
char scrollDisplayBuf[MAX_SCROLL_TEXT_LEN + 60]; // buffer testo composto (data/ora + messaggio)

// Double buffer: OffscreenGFX (gia' nel progetto) + framebuffer esplicito in PSRAM
uint16_t *scrollFrameBuf = NULL;       // framebuffer 480x480 in PSRAM (~460KB)
OffscreenGFX *scrollOffscreen = NULL;  // wrapper GFX per disegnare sul buffer

// ===== Dual Display: bypass/restore DualGFX proxy =====
#ifdef EFFECT_DUAL_DISPLAY
extern Arduino_RGB_Display *realGfx;
static Arduino_GFX *scrollSavedGfx = nullptr;

static inline void scrollBypassProxy() {
  scrollSavedGfx = gfx;
  if (isDualDisplayActive() && realGfx) {
    gfx = (Arduino_GFX*)realGfx;
  }
}
static inline void scrollRestoreProxy() {
  if (scrollSavedGfx) {
    gfx = scrollSavedGfx;
    scrollSavedGfx = nullptr;
  }
}
#endif

// ===== Helper: HSV to RGB565 =====
uint16_t scrollHsvToRgb565(float h, float s, float v) {
  float r, g, b;
  int i = (int)(h * 6.0f);
  float f = h * 6.0f - i;
  float p = v * (1.0f - s);
  float q = v * (1.0f - f * s);
  float t = v * (1.0f - (1.0f - f) * s);
  switch (i % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
  return ((((uint8_t)(r*255)) & 0xF8) << 8) | ((((uint8_t)(g*255)) & 0xFC) << 3) | (((uint8_t)(b*255)) >> 3);
}

// ===== Helper: speed (1-10) -> pixel/frame =====
float scrollSpeedToPixels(uint8_t speed) {
  if (speed < 1) speed = 1;
  if (speed > 10) speed = 10;
  return 1.0f + (speed - 1) * 2.0f;  // 1.0 a 19.0 px/frame (~60-1140 px/sec)
}

// ===== Helper: RGB888 to RGB565 =====
uint16_t scrollRgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ===== Helper: larghezza canvas (virtuale se dual) =====
int scrollDisplayWidth() {
#ifdef EFFECT_DUAL_DISPLAY
  if (isDualDisplayActive()) return virtualWidth();
#endif
  return 480;
}

// ===== Compone testo display: [ora] [data] [separatore] [messaggio] =====
void scrollBuildDisplayText(ScrollMessage& msg) {
  scrollDisplayBuf[0] = '\0';

  if (scrollShowTime) {
    char tBuf[6];
    sprintf(tBuf, "%02d:%02d", myTZ.hour(), myTZ.minute());
    strcat(scrollDisplayBuf, tBuf);
  }

  if (scrollShowDate) {
    if (scrollShowTime) strcat(scrollDisplayBuf, " ");
    const char* giorni[] = {"Dom","Lun","Mar","Mer","Gio","Ven","Sab"};
    char dBuf[16];
    sprintf(dBuf, "%s %02d/%02d", giorni[myTZ.weekday()-1], myTZ.day(), myTZ.month());
    strcat(scrollDisplayBuf, dBuf);
  }

  if ((scrollShowTime || scrollShowDate) && msg.text[0] != '\0') {
    strcat(scrollDisplayBuf, "  -  ");
  }

  strncat(scrollDisplayBuf, msg.text, MAX_SCROLL_TEXT_LEN);
}

// ===== Imposta font e scala per un messaggio =====
// La scala (setTextSize) funziona con TUTTI i font: Pixel e U8g2
// Per U8g2, scala il glifo bitmap (es. Sans Bold 24 x3 = 72px)
void scrollApplyFont(ScrollMessage& msg, Arduino_GFX* g = NULL) {
  if (!g) g = gfx;
  uint8_t fi = msg.fontIndex;
  if (fi >= SCROLL_FONT_COUNT) fi = 0;

  uint8_t sc = msg.scale;
  if (sc < 1) sc = 1;
  if (sc > 20) sc = 20;

  if (SCROLL_FONTS[fi].font == NULL) {
    g->setFont((const GFXfont *)NULL);
  } else {
    g->setFont(SCROLL_FONTS[fi].font);
  }
  g->setTextSize(sc);
}

// ===== Calcola altezza effettiva testo =====
int16_t scrollGetTextHeight(ScrollMessage& msg) {
  uint8_t fi = msg.fontIndex;
  if (fi >= SCROLL_FONT_COUNT) fi = 0;
  uint8_t sc = msg.scale < 1 ? 1 : (msg.scale > 20 ? 20 : msg.scale);
  if (SCROLL_FONTS[fi].font == NULL) {
    return 8 * sc;  // built-in font = 8px * scale
  }
  return SCROLL_FONTS[fi].height * sc;  // U8g2 font * scala
}

// ===== Calcola larghezza effettiva testo (con data/ora composta) =====
int16_t scrollGetTextWidth(ScrollMessage& msg) {
  scrollApplyFont(msg);
  scrollBuildDisplayText(msg);
  // CRITICO: disabilita textWrap per la misurazione!
  // Con wrap=true, getTextBounds tronca la larghezza a 480px (display width)
  // invece di restituire la larghezza reale del testo intero
  gfx->setTextWrap(false);
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(scrollDisplayBuf, 0, 0, &x1, &y1, &w, &h);
  return (int16_t)w;
}

// ===== Inizializza messaggi di default =====
void scrollSetDefaults() {
  scrollMessageCount = 1;
  scrollCurrentMsg = 0;
  scrollRotationInterval = 10000;
  scrollBgR = 0; scrollBgG = 0; scrollBgB = 0;

  memset(scrollMessages, 0, sizeof(scrollMessages));
  strcpy(scrollMessages[0].text, "OraQuadra!");
  scrollMessages[0].fontIndex = 0;    // Pixel
  scrollMessages[0].scale = 6;        // size 6 = 48px
  scrollMessages[0].colorR = 0;
  scrollMessages[0].colorG = 255;
  scrollMessages[0].colorB = 0;
  scrollMessages[0].speed = 5;
  scrollMessages[0].direction = SDIR_LEFT;
  scrollMessages[0].effects = SFLAG_RAINBOW;
  scrollMessages[0].enabled = true;
}

// ===== LittleFS: Carica config =====
void loadScrollTextFromFS() {
  if (!LittleFS.exists("/scrolltext.json")) {
    Serial.println("[SCROLLTEXT] Nessun file config, provo EEPROM...");
    if (EEPROM.read(EEPROM_SCROLLTEXT_MARKER_ADDR) == EEPROM_SCROLLTEXT_MARKER_VAL) {
      scrollMessageCount = EEPROM.read(EEPROM_SCROLLTEXT_COUNT);
      if (scrollMessageCount > MAX_SCROLL_MESSAGES) scrollMessageCount = 0;
      scrollCurrentMsg = EEPROM.read(EEPROM_SCROLLTEXT_CURRENT);
      scrollRotationInterval = ((uint32_t)EEPROM.read(EEPROM_SCROLLTEXT_ROT_HI) << 8) | EEPROM.read(EEPROM_SCROLLTEXT_ROT_LO);
      if (scrollRotationInterval < 3000) scrollRotationInterval = 10000;
      scrollBgR = EEPROM.read(EEPROM_SCROLLTEXT_BG_R);
      scrollBgG = EEPROM.read(EEPROM_SCROLLTEXT_BG_G);
      scrollBgB = EEPROM.read(EEPROM_SCROLLTEXT_BG_B);
      scrollShowTime = (EEPROM.read(EEPROM_SCROLLTEXT_SHOW_TIME) != 0);
      scrollShowDate = (EEPROM.read(EEPROM_SCROLLTEXT_SHOW_DATE) != 0);
      Serial.printf("[SCROLLTEXT] Ripristinato da EEPROM: %d msg\n", scrollMessageCount);
      if (scrollMessageCount == 0) scrollSetDefaults();
    } else {
      scrollSetDefaults();
    }
    return;
  }

  File f = LittleFS.open("/scrolltext.json", "r");
  if (!f) {
    Serial.println("[SCROLLTEXT] Errore apertura file");
    scrollSetDefaults();
    return;
  }

  String json = f.readString();
  f.close();

  int idx;
  idx = json.indexOf("\"count\":");
  if (idx >= 0) scrollMessageCount = json.substring(idx + 8).toInt();
  if (scrollMessageCount > MAX_SCROLL_MESSAGES) scrollMessageCount = MAX_SCROLL_MESSAGES;

  idx = json.indexOf("\"rotation\":");
  if (idx >= 0) scrollRotationInterval = json.substring(idx + 11).toInt();

  idx = json.indexOf("\"bgR\":");
  if (idx >= 0) scrollBgR = json.substring(idx + 6).toInt();
  idx = json.indexOf("\"bgG\":");
  if (idx >= 0) scrollBgG = json.substring(idx + 6).toInt();
  idx = json.indexOf("\"bgB\":");
  if (idx >= 0) scrollBgB = json.substring(idx + 6).toInt();

  idx = json.indexOf("\"showTime\":");
  if (idx >= 0) scrollShowTime = (json.substring(idx + 11).toInt() != 0);
  idx = json.indexOf("\"showDate\":");
  if (idx >= 0) scrollShowDate = (json.substring(idx + 11).toInt() != 0);

  int msgStart = 0;
  for (int m = 0; m < scrollMessageCount; m++) {
    int blockStart = json.indexOf("{", (m == 0) ? json.indexOf("\"msgs\"") : msgStart);
    if (blockStart < 0) break;
    int blockEnd = json.indexOf("}", blockStart);
    if (blockEnd < 0) break;
    msgStart = blockEnd + 1;

    String block = json.substring(blockStart, blockEnd + 1);

    // text
    int ti = block.indexOf("\"text\":\"");
    if (ti >= 0) {
      int ts = ti + 8;
      int te = block.indexOf("\"", ts);
      if (te > ts) {
        String txt = block.substring(ts, te);
        txt.replace("\\\"", "\"");
        txt.replace("\\n", "\n");
        txt.replace("\\\\", "\\");
        strncpy(scrollMessages[m].text, txt.c_str(), MAX_SCROLL_TEXT_LEN);
        scrollMessages[m].text[MAX_SCROLL_TEXT_LEN] = '\0';
      }
    }

    idx = block.indexOf("\"font\":");
    if (idx >= 0) scrollMessages[m].fontIndex = block.substring(idx + 7).toInt();
    if (scrollMessages[m].fontIndex >= SCROLL_FONT_COUNT) scrollMessages[m].fontIndex = 0;

    idx = block.indexOf("\"sc\":");
    if (idx >= 0) scrollMessages[m].scale = block.substring(idx + 5).toInt();
    if (scrollMessages[m].scale < 1) scrollMessages[m].scale = 1;
    if (scrollMessages[m].scale > 20) scrollMessages[m].scale = 20;

    idx = block.indexOf("\"r\":");
    if (idx >= 0) scrollMessages[m].colorR = block.substring(idx + 4).toInt();
    idx = block.indexOf("\"g\":");
    if (idx >= 0) scrollMessages[m].colorG = block.substring(idx + 4).toInt();
    idx = block.indexOf("\"b\":");
    if (idx >= 0) scrollMessages[m].colorB = block.substring(idx + 4).toInt();

    idx = block.indexOf("\"speed\":");
    if (idx >= 0) scrollMessages[m].speed = block.substring(idx + 8).toInt();
    idx = block.indexOf("\"dir\":");
    if (idx >= 0) scrollMessages[m].direction = block.substring(idx + 6).toInt();
    if (scrollMessages[m].direction > 1) scrollMessages[m].direction = 0;
    idx = block.indexOf("\"fx\":");
    if (idx >= 0) scrollMessages[m].effects = block.substring(idx + 5).toInt();
    idx = block.indexOf("\"on\":");
    if (idx >= 0) scrollMessages[m].enabled = (block.substring(idx + 5).toInt() != 0);
  }

  Serial.printf("[SCROLLTEXT] Caricati %d messaggi da LittleFS\n", scrollMessageCount);
}

// ===== LittleFS: Salva config =====
void saveScrollTextToFS() {
  File f = LittleFS.open("/scrolltext.json", "w");
  if (!f) {
    Serial.println("[SCROLLTEXT] Errore salvataggio");
    return;
  }

  f.print("{\"count\":");
  f.print(scrollMessageCount);
  f.print(",\"rotation\":");
  f.print(scrollRotationInterval);
  f.printf(",\"bgR\":%d,\"bgG\":%d,\"bgB\":%d", scrollBgR, scrollBgG, scrollBgB);
  f.printf(",\"showTime\":%d,\"showDate\":%d", scrollShowTime ? 1 : 0, scrollShowDate ? 1 : 0);
  f.print(",\"msgs\":[");

  for (int m = 0; m < scrollMessageCount; m++) {
    if (m > 0) f.print(",");
    f.print("{\"text\":\"");
    for (int c = 0; scrollMessages[m].text[c]; c++) {
      char ch = scrollMessages[m].text[c];
      if (ch == '"') f.print("\\\"");
      else if (ch == '\\') f.print("\\\\");
      else if (ch == '\n') f.print("\\n");
      else f.print(ch);
    }
    f.printf("\",\"font\":%d,\"sc\":%d", scrollMessages[m].fontIndex, scrollMessages[m].scale);
    f.printf(",\"r\":%d,\"g\":%d,\"b\":%d", scrollMessages[m].colorR, scrollMessages[m].colorG, scrollMessages[m].colorB);
    f.printf(",\"speed\":%d,\"dir\":%d,\"fx\":%d,\"on\":%d}",
             scrollMessages[m].speed, scrollMessages[m].direction,
             scrollMessages[m].effects, scrollMessages[m].enabled ? 1 : 0);
  }

  f.print("]}");
  f.close();

  EEPROM.write(EEPROM_SCROLLTEXT_MARKER_ADDR, EEPROM_SCROLLTEXT_MARKER_VAL);
  EEPROM.write(EEPROM_SCROLLTEXT_COUNT, scrollMessageCount);
  EEPROM.write(EEPROM_SCROLLTEXT_CURRENT, scrollCurrentMsg);
  EEPROM.write(EEPROM_SCROLLTEXT_ROT_HI, (scrollRotationInterval >> 8) & 0xFF);
  EEPROM.write(EEPROM_SCROLLTEXT_ROT_LO, scrollRotationInterval & 0xFF);
  EEPROM.write(EEPROM_SCROLLTEXT_BG_R, scrollBgR);
  EEPROM.write(EEPROM_SCROLLTEXT_BG_G, scrollBgG);
  EEPROM.write(EEPROM_SCROLLTEXT_BG_B, scrollBgB);
  EEPROM.write(EEPROM_SCROLLTEXT_SHOW_TIME, scrollShowTime ? 1 : 0);
  EEPROM.write(EEPROM_SCROLLTEXT_SHOW_DATE, scrollShowDate ? 1 : 0);
  EEPROM.commit();

  Serial.printf("[SCROLLTEXT] Salvati %d messaggi su LittleFS + EEPROM\n", scrollMessageCount);
}

// ===== Trova prossimo messaggio abilitato =====
int scrollFindNextEnabled(int startFrom) {
  if (scrollMessageCount == 0) return -1;
  for (int i = 0; i < scrollMessageCount; i++) {
    int idx = (startFrom + i) % scrollMessageCount;
    if (scrollMessages[idx].enabled && scrollMessages[idx].text[0] != '\0') {
      return idx;
    }
  }
  return -1;
}

// ===== Calcola posizione iniziale per un messaggio =====
void scrollResetPosition() {
  if (scrollMessageCount == 0) return;
  int mi = scrollCurrentMsg;
  if (mi >= scrollMessageCount) mi = 0;

  ScrollMessage& msg = scrollMessages[mi];
  int16_t textW = scrollGetTextWidth(msg);
  int16_t textH = scrollGetTextHeight(msg);

  // Centra verticalmente
  scrollPosY = (480 - textH) / 2 + textH;

  if (msg.direction == SDIR_RIGHT) {
    scrollPosX = -textW;  // entra da sinistra
  } else {
    scrollPosX = scrollDisplayWidth();  // entra da destra (default: verso sinistra)
  }
}

// ===== Controlla se il testo e' uscito dallo schermo =====
bool scrollCheckWrap() {
  if (scrollMessageCount == 0) return true;
  int mi = scrollCurrentMsg;
  if (mi >= scrollMessageCount) return true;

  ScrollMessage& msg = scrollMessages[mi];
  int16_t textW = scrollGetTextWidth(msg);

  if (msg.direction == SDIR_RIGHT) {
    return scrollPosX > scrollDisplayWidth();  // uscito a destra
  } else {
    return scrollPosX < -textW;  // uscito a sinistra
  }
}

// ===== Avanza posizione in base a direzione e velocita =====
void scrollAdvancePosition() {
  if (scrollMessageCount == 0) return;
  int mi = scrollCurrentMsg;
  if (mi >= scrollMessageCount) return;

  float spd = scrollSpeedToPixels(scrollMessages[mi].speed);

  if (scrollMessages[mi].direction == SDIR_RIGHT) {
    scrollPosX += spd;
  } else {
    scrollPosX -= spd;
  }
}

// ===== Disegna testo rainbow (per-carattere con hue ciclico) =====
void drawScrollRainbowText(int x, int y, const char* text, float hueOffset, Arduino_GFX* g) {
  g->setCursor(x, y);
  for (int i = 0; text[i]; i++) {
    float hue = fmod(hueOffset + i * 0.08f, 1.0f);
    uint16_t color = scrollHsvToRgb565(hue, 1.0f, 1.0f);
    g->setTextColor(color);
    char buf[2] = {text[i], '\0'};
    g->print(buf);
  }
}

// ===== Disegna bande fade sui bordi (solo orizzontale) =====
void drawScrollFade(uint16_t bgColor, Arduino_GFX* g) {
  uint8_t bgR = (bgColor >> 11) << 3;
  uint8_t bgG = ((bgColor >> 5) & 0x3F) << 2;
  uint8_t bgB = (bgColor & 0x1F) << 3;

  int bandW = SCROLL_FADE_PIXELS / 4;
  for (int band = 0; band < 4; band++) {
    float alpha = (band + 1) * 0.25f;
    uint16_t bandColor = scrollRgb565((uint8_t)(bgR * alpha), (uint8_t)(bgG * alpha), (uint8_t)(bgB * alpha));
    int offset = band * bandW;
    g->fillRect(offset, 0, bandW, 480, bandColor);
    g->fillRect(480 - offset - bandW, 0, bandW, 480, bandColor);
  }
}

// ===== Fade ai bordi del canvas virtuale (dual display) =====
#ifdef EFFECT_DUAL_DISPLAY
void drawScrollFadeDual(uint16_t bgColor, Arduino_GFX* g) {
  extern uint8_t panelX, panelY;
  int vw = scrollDisplayWidth();
  int bandW = SCROLL_FADE_PIXELS / 4;
  uint8_t bgR = (bgColor >> 11) << 3;
  uint8_t bgG = ((bgColor >> 5) & 0x3F) << 2;
  uint8_t bgB = (bgColor & 0x1F) << 3;
  for (int band = 0; band < 4; band++) {
    float alpha = (band + 1) * 0.25f;
    uint16_t bandColor = scrollRgb565((uint8_t)(bgR * alpha), (uint8_t)(bgG * alpha), (uint8_t)(bgB * alpha));
    int offset = band * bandW;
    int leftEdge = 0 - panelX * 480;
    int rightEdge = vw - panelX * 480;
    g->fillRect(leftEdge + offset, 0, bandW, 480, bandColor);
    g->fillRect(rightEdge - offset - bandW, 0, bandW, 480, bandColor);
  }
}
#endif

// ===== Disegna un frame di scroll (con double buffer) =====
void drawScrollFrame() {
  if (scrollMessageCount == 0) return;
  int mi = scrollCurrentMsg;
  if (mi >= scrollMessageCount) return;

  ScrollMessage& msg = scrollMessages[mi];

#ifdef EFFECT_DUAL_DISPLAY
  if (isDualDisplayActive()) {
    // === DUAL MODE: double buffer su offscreen, poi blit su realGfx ===
    Arduino_GFX *g = scrollOffscreen ? (Arduino_GFX*)scrollOffscreen : gfx;

    uint16_t bgColor = scrollRgb565(scrollBgR, scrollBgG, scrollBgB);
    g->fillScreen(bgColor);

    scrollApplyFont(msg, g);
    g->setTextWrap(false);
    scrollBuildDisplayText(msg);

    // Calcola posizione locale per questo pannello
    extern uint8_t panelX, panelY;
    int localX = (int)scrollPosX - panelX * 480;
    int localY = (int)scrollPosY;

    if (msg.effects & SFLAG_RAINBOW) {
      g->setCursor(localX, localY);
      for (int i = 0; scrollDisplayBuf[i]; i++) {
        float hue = fmod(scrollRainbowHue + i * 0.08f, 1.0f);
        g->setTextColor(scrollHsvToRgb565(hue, 1.0f, 1.0f));
        g->write((uint8_t)scrollDisplayBuf[i]);
      }
      scrollRainbowHue += 0.005f;
      if (scrollRainbowHue > 1.0f) scrollRainbowHue -= 1.0f;
    } else {
      g->setTextColor(scrollRgb565(msg.colorR, msg.colorG, msg.colorB));
      g->setCursor(localX, localY);
      for (int i = 0; scrollDisplayBuf[i]; i++) {
        g->write((uint8_t)scrollDisplayBuf[i]);
      }
    }

    if (msg.effects & SFLAG_FADE) {
      drawScrollFadeDual(bgColor, g);
    }

    if (scrollPaused) {
      g->setFont(u8g2_font_helvR08_tf);
      g->setTextSize(1);
      g->fillRect(0, 468, 480, 12, bgColor);
      g->setCursor(5, 479);
      g->setTextColor(scrollRgb565(255, 200, 0));
      g->print("PAUSA");
    }

    // Blit atomico: offscreen -> realGfx (bypassa DualGFX)
    if (scrollFrameBuf) {
      scrollBypassProxy();
      gfx->draw16bitRGBBitmap(0, 0, scrollFrameBuf, 480, 480);
      scrollRestoreProxy();
    }
    return;
  }
#endif

  // === SINGLE DISPLAY: double buffer offscreen PSRAM, poi blit atomico ===
  Arduino_GFX *g = scrollOffscreen ? (Arduino_GFX*)scrollOffscreen : gfx;

  uint16_t bgColor = scrollRgb565(scrollBgR, scrollBgG, scrollBgB);
  g->fillScreen(bgColor);

  // Imposta font + scala + no wrap
  scrollApplyFont(msg, g);
  g->setTextWrap(false);

  // Componi testo con data/ora
  scrollBuildDisplayText(msg);

  int x = (int)scrollPosX;
  int y = (int)scrollPosY;

  if (msg.effects & SFLAG_RAINBOW) {
    drawScrollRainbowText(x, y, scrollDisplayBuf, scrollRainbowHue, g);
    scrollRainbowHue += 0.005f;
    if (scrollRainbowHue > 1.0f) scrollRainbowHue -= 1.0f;
  } else {
    uint16_t textColor = scrollRgb565(msg.colorR, msg.colorG, msg.colorB);
    g->setTextColor(textColor);
    g->setCursor(x, y);
    // Stampa carattere per carattere: evita che Print::write() si fermi
    // se un byte UTF-8 (è, à, ù) non è nel font e write() ritorna 0
    for (int i = 0; scrollDisplayBuf[i]; i++) {
      g->write((uint8_t)scrollDisplayBuf[i]);
    }
  }

  if (msg.effects & SFLAG_FADE) {
    drawScrollFade(bgColor, g);
  }

  // Indicatore pausa (disegnato nel canvas prima del flush)
  if (scrollPaused) {
    g->setFont(u8g2_font_helvR08_tf);
    g->setTextSize(1);
    g->fillRect(0, 468, 480, 12, bgColor);
    g->setCursor(5, 479);
    g->setTextColor(scrollRgb565(255, 200, 0));
    g->print("PAUSA");
    if (scrollMessageCount > 1) {
      char msgStr[8];
      sprintf(msgStr, "%d/%d", scrollCurrentMsg + 1, scrollMessageCount);
      g->setCursor(440, 479);
      g->setTextColor(scrollRgb565(80, 80, 80));
      g->print(msgStr);
    }
  }

  // Flush atomico: copia framebuffer PSRAM -> display (zero flicker)
  if (scrollFrameBuf) {
    gfx->draw16bitRGBBitmap(0, 0, scrollFrameBuf, 480, 480);
  }
}

// ===== Passa al messaggio successivo =====
void scrollNextMessage() {
  int next = scrollFindNextEnabled(scrollCurrentMsg + 1);
  if (next >= 0) {
    scrollCurrentMsg = next;
  } else {
    next = scrollFindNextEnabled(0);
    if (next >= 0) scrollCurrentMsg = next;
  }
  scrollResetPosition();
  scrollMsgStartTime = millis();
}

// ===== Passa al messaggio precedente =====
void scrollPrevMessage() {
  if (scrollMessageCount == 0) return;
  for (int i = scrollMessageCount - 1; i >= 0; i--) {
    int idx = (scrollCurrentMsg - 1 + scrollMessageCount + i) % scrollMessageCount;
    if (idx == scrollCurrentMsg) continue;
    if (scrollMessages[idx].enabled && scrollMessages[idx].text[0] != '\0') {
      scrollCurrentMsg = idx;
      scrollResetPosition();
      scrollMsgStartTime = millis();
      return;
    }
  }
}

// ===== Libera risorse double buffer =====
void cleanupScrollText() {
  if (scrollOffscreen) {
    delete scrollOffscreen;
    scrollOffscreen = NULL;
  }
  if (scrollFrameBuf) {
    free(scrollFrameBuf);
    scrollFrameBuf = NULL;
  }
  // Reset stato gfx per evitare corruzione display in altre modalità
  // (scrollApplyFont imposta textSize su gfx per misurazioni)
  gfx->setTextSize(1);
  gfx->setTextWrap(true);
  Serial.println("[SCROLLTEXT] Double buffer liberato + gfx reset");
}

// ===== Inizializzazione =====
void initScrollText() {
  loadScrollTextFromFS();

  if (scrollMessageCount == 0) {
    scrollSetDefaults();
    saveScrollTextToFS();
  }

  // Alloca double buffer in PSRAM esplicito (480x480x2 = 460KB)
  // Usato sia in single che dual mode per evitare flicker
  if (!scrollFrameBuf) {
    scrollFrameBuf = (uint16_t*)ps_malloc(480 * 480 * sizeof(uint16_t));
    if (scrollFrameBuf) {
      scrollOffscreen = new OffscreenGFX(scrollFrameBuf, 480, 480);
      scrollOffscreen->begin();
      Serial.printf("[SCROLLTEXT] Double buffer PSRAM attivo (%d KB)\n", (480*480*2)/1024);
    } else {
      Serial.println("[SCROLLTEXT] PSRAM insufficiente, drawing diretto");
    }
  }

  int first = scrollFindNextEnabled(0);
  if (first >= 0) {
    scrollCurrentMsg = first;
  } else {
    scrollCurrentMsg = 0;
  }

  scrollResetPosition();
  scrollPaused = false;
  scrollLastFrame = millis();
  scrollMsgStartTime = millis();
  scrollRainbowHue = 0;

  if (scrollMessageCount > 0 && first >= 0) {
    drawScrollFrame();
  } else {
    Arduino_GFX *g = scrollOffscreen ? (Arduino_GFX*)scrollOffscreen : gfx;
    uint16_t bgColor = scrollRgb565(scrollBgR, scrollBgG, scrollBgB);
    g->fillScreen(bgColor);
    g->setFont(u8g2_font_helvB18_tf);
    g->setTextSize(1);
    g->setTextColor(WHITE);
    g->setCursor(80, 220);
    g->print("Nessun messaggio");
    g->setFont(u8g2_font_helvR14_tf);
    g->setCursor(100, 260);
    g->print("Configura su /scrolltext");
    if (scrollFrameBuf) gfx->draw16bitRGBBitmap(0, 0, scrollFrameBuf, 480, 480);
  }

  scrollTextInitialized = true;
  Serial.printf("[SCROLLTEXT] Init OK, %d messaggi, corrente=%d\n", scrollMessageCount, scrollCurrentMsg);
}

// ===== Update chiamata nel loop =====
void updateScrollText() {
  if (!scrollTextInitialized) {
    initScrollText();
    return;
  }

#ifdef EFFECT_DUAL_DISPLAY
  // Slave: solo rendering, la logica viene dal master via sync
  if (isDualDisplayActive() && isDualSlave()) {
    unsigned long now = millis();
    if (now - scrollLastFrame < SCROLL_FRAME_MS) return;
    scrollLastFrame = now;
    if (scrollMessageCount == 0 || scrollFindNextEnabled(0) < 0) return;
    drawScrollFrame();
    return;
  }
#endif

  // Master / Standalone: logica + rendering
  unsigned long now = millis();
  if (now - scrollLastFrame < SCROLL_FRAME_MS) return;
  scrollLastFrame = now;

  if (scrollMessageCount == 0 || scrollFindNextEnabled(0) < 0) return;

  if (!scrollPaused) {
    scrollAdvancePosition();

    if (scrollCheckWrap()) {
      // Il testo ha completato un passaggio completo sullo schermo
      bool shouldAdvance = false;
      if (scrollMessageCount > 1 && scrollRotationInterval > 0) {
        // Avanza al prossimo solo se il timer di rotazione è scaduto
        if (now - scrollMsgStartTime >= scrollRotationInterval) {
          shouldAdvance = true;
        }
      }

      if (shouldAdvance) {
        scrollNextMessage();  // Prossimo messaggio (timer scaduto + scroll finito)
      } else {
        scrollResetPosition();  // Ricomincia lo stesso messaggio
      }
    }
  }

  drawScrollFrame();
}

// ===== Touch handler =====
void handleScrollTextTouch(int x, int y) {
  if (x < 160) { scrollPrevMessage(); return; }
  if (x > 320) { scrollNextMessage(); return; }
  scrollPaused = !scrollPaused;
}

// ===== Dual Display: pack sync data (chiamata da 44_DUAL_DISPLAY.ino) =====
#ifdef EFFECT_DUAL_DISPLAY
int scrollPackSyncData(uint8_t* data, int maxLen) {
  uint8_t mi = scrollCurrentMsg;
  if (mi >= scrollMessageCount) return 0;

  ScrollMessage& msg = scrollMessages[mi];
  scrollBuildDisplayText(msg);

  int off = 0;
  memcpy(data + off, &scrollPosX, 4);       off += 4;  // 0-3
  memcpy(data + off, &scrollPosY, 4);       off += 4;  // 4-7
  memcpy(data + off, &scrollRainbowHue, 4); off += 4;  // 8-11
  data[off++] = msg.fontIndex;                          // 12
  data[off++] = msg.scale;                              // 13
  data[off++] = msg.effects;                            // 14
  data[off++] = scrollPaused ? 1 : 0;                  // 15
  data[off++] = msg.colorR;                             // 16
  data[off++] = msg.colorG;                             // 17
  data[off++] = msg.colorB;                             // 18
  data[off++] = scrollBgR;                              // 19
  data[off++] = scrollBgG;                              // 20
  data[off++] = scrollBgB;                              // 21
  data[off++] = msg.direction;                          // 22
  data[off++] = msg.speed;                              // 23

  uint8_t textLen = strlen(scrollDisplayBuf);
  if (textLen > 175) textLen = 175;
  data[off++] = textLen;                                // 24
  memcpy(data + off, scrollDisplayBuf, textLen);        // 25+
  off += textLen;

  return off;
}

void scrollUnpackSyncData(const uint8_t* data, int dataLen) {
  if (dataLen < 25) return;

  int off = 0;
  memcpy(&scrollPosX, data + off, 4);       off += 4;
  memcpy(&scrollPosY, data + off, 4);       off += 4;
  memcpy(&scrollRainbowHue, data + off, 4); off += 4;

  // Assicura che ci sia almeno 1 messaggio sullo slave per il rendering
  if (scrollMessageCount == 0) {
    scrollMessageCount = 1;
    scrollCurrentMsg = 0;
    memset(&scrollMessages[0], 0, sizeof(ScrollMessage));
    scrollMessages[0].enabled = true;
    scrollMessages[0].scale = 1;
  }

  ScrollMessage& msg = scrollMessages[scrollCurrentMsg < scrollMessageCount ? scrollCurrentMsg : 0];
  msg.fontIndex = data[off++];
  msg.scale = data[off++];
  msg.effects = data[off++];
  scrollPaused = (data[off++] != 0);
  msg.colorR = data[off++];
  msg.colorG = data[off++];
  msg.colorB = data[off++];
  scrollBgR = data[off++];
  scrollBgG = data[off++];
  scrollBgB = data[off++];
  msg.direction = data[off++];
  msg.speed = data[off++];

  uint8_t textLen = data[off++];
  if (textLen > 175) textLen = 175;
  memcpy(scrollDisplayBuf, data + off, textLen);
  scrollDisplayBuf[textLen] = '\0';
}

// ===== Config sync: invia config completa agli slave via ESP-NOW chunked =====
// Pacchetto: [magic][pktType=7][configId=0x01][chunkIdx][totalChunks][chunkLen][data...]
#define SCROLL_CFG_CHUNK_SIZE 244

static char* scrollCfgRecvBuf = nullptr;
static int scrollCfgRecvLen = 0;
static uint8_t scrollCfgTotalChunks = 0;
static uint8_t scrollCfgRecvCount = 0;

void scrollSendConfigToSlaves() {
  if (!isDualDisplayActive() || !isDualMaster()) return;

  File f = LittleFS.open("/scrolltext.json", "r");
  if (!f) return;
  String json = f.readString();
  f.close();

  int totalLen = json.length();
  int totalChunks = (totalLen + SCROLL_CFG_CHUNK_SIZE - 1) / SCROLL_CFG_CHUNK_SIZE;
  if (totalChunks > 255 || totalChunks == 0) return;

  extern uint8_t peerMACs[3][6];
  extern uint8_t peerCount;

  uint8_t pkt[250];
  pkt[0] = 0xDC;   // DUAL_MAGIC
  pkt[1] = 7;      // PKT_MODE_CONFIG
  pkt[2] = 0x01;   // MODE_CFG_SCROLLTEXT

  for (int c = 0; c < totalChunks; c++) {
    pkt[3] = (uint8_t)c;
    pkt[4] = (uint8_t)totalChunks;
    int offset = c * SCROLL_CFG_CHUNK_SIZE;
    int len = totalLen - offset;
    if (len > SCROLL_CFG_CHUNK_SIZE) len = SCROLL_CFG_CHUNK_SIZE;
    pkt[5] = (uint8_t)len;
    memcpy(pkt + 6, json.c_str() + offset, len);

    for (int p = 0; p < (int)peerCount; p++) {
      esp_now_send(peerMACs[p], pkt, 6 + len);
    }
    if (c < totalChunks - 1) delay(3);
  }

  Serial.printf("[SCROLLTEXT] Config inviata slave: %d bytes, %d chunks\n", totalLen, totalChunks);
}

void scrollReceiveConfigChunk(const uint8_t* data, int len) {
  if (len < 6) return;

  uint8_t chunkIdx = data[3];
  uint8_t totalChunks = data[4];
  uint8_t chunkLen = data[5];
  if (chunkLen > SCROLL_CFG_CHUNK_SIZE) chunkLen = SCROLL_CFG_CHUNK_SIZE;

  // Primo chunk: alloca buffer
  if (chunkIdx == 0) {
    if (scrollCfgRecvBuf) { free(scrollCfgRecvBuf); scrollCfgRecvBuf = nullptr; }
    int maxSize = totalChunks * SCROLL_CFG_CHUNK_SIZE + 1;
    scrollCfgRecvBuf = (char*)malloc(maxSize);
    if (!scrollCfgRecvBuf) return;
    scrollCfgRecvLen = 0;
    scrollCfgTotalChunks = totalChunks;
    scrollCfgRecvCount = 0;
  }

  if (!scrollCfgRecvBuf || chunkIdx >= scrollCfgTotalChunks) return;

  int offset = chunkIdx * SCROLL_CFG_CHUNK_SIZE;
  memcpy(scrollCfgRecvBuf + offset, data + 6, chunkLen);
  int endPos = offset + chunkLen;
  if (endPos > scrollCfgRecvLen) scrollCfgRecvLen = endPos;
  scrollCfgRecvCount++;

  // Tutti i chunk ricevuti: salva su LittleFS e ricarica
  if (scrollCfgRecvCount >= scrollCfgTotalChunks) {
    scrollCfgRecvBuf[scrollCfgRecvLen] = '\0';

    File f = LittleFS.open("/scrolltext.json", "w");
    if (f) {
      f.write((uint8_t*)scrollCfgRecvBuf, scrollCfgRecvLen);
      f.close();
      Serial.printf("[SCROLLTEXT] Config ricevuta dal master: %d bytes\n", scrollCfgRecvLen);
      loadScrollTextFromFS();
      if (currentMode == MODE_SCROLLTEXT) {
        scrollTextInitialized = false;
      }
    }

    free(scrollCfgRecvBuf);
    scrollCfgRecvBuf = nullptr;
  }
}
#endif

#endif // EFFECT_SCROLLTEXT
