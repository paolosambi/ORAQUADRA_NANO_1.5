// ================== LETTORE MP3/WAV DA SD CARD ==================
// Modalità lettore musicale con VU meter verticali e controlli touch
// Legge file MP3 e WAV dalla cartella /MP3 sulla SD card
// WAV è preferito perché non richiede decodifica CPU

#ifdef EFFECT_MP3_PLAYER

// Audio unificato: usa Audio.h (ESP32-audioI2S) tramite oggetto globale 'audio'

// Prototipi funzioni salvataggio/caricamento (definite più avanti)
void saveMP3PlayerSettings();
void loadMP3PlayerSettings();

// ================== CONFIGURAZIONE MP3 PLAYER ==================
#define MP3_FOLDER "/MP3"           // Cartella sulla SD dove cercare i file
#define MP3_MAX_FILES 100           // Numero massimo di file MP3 gestibili
#define MP3_TITLE_MAX_LEN 40        // Lunghezza massima del titolo visualizzato
#define MP3_SCROLL_SPEED 150        // Velocità scroll titolo lungo (ms)

// ================== LAYOUT DISPLAY 480x480 - TEMA MODERNO ==================
// VU Meter sinistro: x=0, larghezza=48
// VU Meter destro: x=432, larghezza=48
// Area centrale: x=55 a x=425 (370px)

#define VU_LEFT_X       0
#define VU_RIGHT_X      432
#define VU_WIDTH        48
#define VU_HEIGHT       390   // Ridotto per lasciare spazio alle lettere L/R
#define VU_Y_START      55
#define VU_SEGMENTS     24          // Numero di segmenti per VU meter

#define PREVIEW_X       55
#define PREVIEW_Y       58
#define PREVIEW_W       370
#define PREVIEW_H       115

#define CONTROLS_Y      185
#define BTN_SIZE        62
#define BTN_SPACING     10

#define VOLUME_BAR_Y    265
#define VOLUME_BAR_H    36

#define MODE_BTN_Y      310
#define MODE_BTN_H      42

#define PLAYLIST_Y      360
#define PLAYLIST_H      50

// Area centrale
#define MP3_CENTER_X    55
#define MP3_CENTER_W    370

// ================== TEMA MODERNO - PALETTE COLORI ==================
// Sfondo e base
#define MP3_BG_COLOR      0x0841    // Blu molto scuro
#define MP3_BG_DARK       0x0000    // Nero puro
#define MP3_BG_CARD       0x1082    // Grigio-blu scuro per cards
#define MP3_PANEL_COLOR   0x1082    // Alias per compatibilità

// Testo
#define MP3_TEXT_COLOR    0xFFFF    // Bianco
#define MP3_TEXT_DIM      0xB5B6    // Grigio chiaro
#define MP3_TEXT_MUTED    0x7BCF    // Grigio medio

// Accenti - Ciano/Turchese moderno
#define MP3_ACCENT_COLOR  0x07FF    // Ciano brillante
#define MP3_ACCENT_DARK   0x0575    // Ciano scuro
#define MP3_ACCENT_GLOW   0x5FFF    // Ciano chiaro (glow)

// Controlli
#define MP3_PLAY_COLOR    0x07E0    // Verde
#define MP3_PAUSE_COLOR   0xFD20    // Arancione
#define MP3_STOP_COLOR    0xF800    // Rosso

// Bottoni
#define MP3_BUTTON_COLOR  0x2124    // Grigio scuro
#define MP3_BUTTON_HOVER  0x3186    // Grigio medio
#define MP3_BUTTON_BORDER 0x4A69    // Grigio bordo
#define MP3_BUTTON_SHADOW 0x1082    // Ombra bottone

// VU meter - Gradiente moderno
#define MP3_VU_LOW        0x07E0    // Verde brillante
#define MP3_VU_LOW2       0x2FE0    // Verde-ciano
#define MP3_VU_MID        0xFFE0    // Giallo
#define MP3_VU_MID2       0xFD20    // Arancione
#define MP3_VU_HIGH       0xF800    // Rosso
#define MP3_VU_PEAK       0xF81F    // Magenta (peak)
#define MP3_VU_BG         0x18C3    // Grigio-blu VU background

// ================== STRUTTURE DATI ==================
enum AudioFileType {
  AUDIO_TYPE_MP3,
  AUDIO_TYPE_WAV
};

struct MP3File {
  char filename[64];               // Nome file completo
  char title[MP3_TITLE_MAX_LEN];   // Titolo estratto (senza estensione)
  AudioFileType type;              // Tipo file (MP3 o WAV)
};

struct MP3PlayerState {
  bool initialized;                 // Player inizializzato
  bool playing;                     // In riproduzione
  bool paused;                      // In pausa
  bool playAll;                     // Modalita' riproduzione: false=singolo brano, true=tutti i brani
  bool trackEnded;                  // Flag: traccia appena terminata (per gestire passaggio automatico)
  int currentTrack;                 // Indice traccia corrente
  int totalTracks;                  // Numero totale tracce
  MP3File* tracks;                  // Array dinamico delle tracce
  uint8_t volume;                   // Volume (0-21)
  uint8_t vuLeft;                   // Livello VU sinistro (0-100)
  uint8_t vuRight;                  // Livello VU destro (0-100)
  uint8_t vuPeakLeft;               // Picco VU sinistro
  uint8_t vuPeakRight;              // Picco VU destro
  uint32_t lastVuUpdate;            // Timestamp ultimo aggiornamento VU
  int scrollOffset;                 // Offset per scroll titolo lungo
  uint32_t lastScrollUpdate;        // Timestamp ultimo scroll
  bool needsRedraw;                 // Flag per ridisegno completo
};

// ================== VARIABILI GLOBALI ==================
MP3PlayerState mp3Player;
bool mp3PlayerInitialized = false;
bool mp3VolumeNeedsRedraw = false;




// Variabili per VU meter (evita flickering)
static int8_t prevActiveLeft = -1;
static int8_t prevActiveRight = -1;
static int8_t prevPeakLeft = -1;
static int8_t prevPeakRight = -1;
static bool vuBackgroundDrawn = false;

// Task audio separato rimosso: audioTask globale (audio.loop()) gestisce tutto

// ================== CALLBACK AUDIO EOF ==================
// Chiamato dalla libreria Audio.h quando un file MP3 termina
void audio_eof_mp3(const char *info) {
  Serial.printf("[AUDIO] EOF: %s\n", info);
  if (mp3Player.playing) {
    mp3Player.trackEnded = true;  // Segnala che la traccia e' terminata
    mp3Player.playing = false;
    extern bool isPlaying;
    isPlaying = false;
    // La logica per passare alla traccia successiva e' in updateMP3Player()
  }
}

// ================== PROTOTIPI FUNZIONI ==================
void initMP3Player();
void updateMP3Player();
void drawMP3PlayerUI();
void drawMP3Header();
void drawVUBackground();
void drawVUMeterVertical(int x, int y, int w, int h, uint8_t level, uint8_t peak, bool leftSide);
void drawMP3ModernButton(int bx, int by, int bw, int bh, bool active, uint16_t activeColor);
void drawMP3Controls();
void drawMP3Preview();
void drawMP3VolumeBar();
void drawMP3ModeButton();
void drawMP3ExitButton();
bool handleMP3PlayerTouch(int16_t x, int16_t y);
void scanMP3Files();
void playMP3Track(int index);
void stopMP3Track();
void pauseMP3Track();
void resumeMP3Track();
void nextMP3Track();
void prevMP3Track();
void updateMP3VUMeters();
void cleanupMP3Player();

String extractTitle(const char* filename);

// ================== HELPER FUNCTION ==================
// Usata da altri file per controllare se MP3 sta riproducendo
bool isMP3Playing() {
  return mp3Player.playing;
}

// ================== INIZIALIZZAZIONE ==================
void initMP3Player() {
  Serial.println("[MP3] Inizializzazione lettore MP3...");

  // Reset stato
  mp3Player.initialized = false;
  mp3Player.playing = false;
  mp3Player.paused = false;
  mp3Player.playAll = false;    // Default: singolo brano
  mp3Player.trackEnded = false;
  mp3Player.currentTrack = 0;
  mp3Player.totalTracks = 0;
  mp3Player.tracks = nullptr;
  mp3Player.volume = 70;  // Volume default (0-100)
  mp3Player.vuLeft = 0;
  mp3Player.vuRight = 0;
  mp3Player.vuPeakLeft = 0;
  mp3Player.vuPeakRight = 0;
  mp3Player.lastVuUpdate = 0;
  mp3Player.scrollOffset = 0;
  mp3Player.lastScrollUpdate = 0;
  mp3Player.needsRedraw = true;

  // Verifica SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[MP3] ERRORE: SD card non trovata!");
    gfx->fillScreen(MP3_BG_COLOR);
    gfx->setTextColor(MP3_STOP_COLOR);
    gfx->setFont(u8g2_font_helvB18_tr);
    gfx->setCursor(100, 240);
    gfx->print("SD CARD NON TROVATA");
    delay(2000);
    return;
  }

  // Crea cartella MP3 se non esiste
  if (!SD.exists(MP3_FOLDER)) {
    SD.mkdir(MP3_FOLDER);
    Serial.println("[MP3] Cartella /MP3 creata");
  }

  // Scansiona file MP3
  scanMP3Files();

  if (mp3Player.totalTracks == 0) {
    Serial.println("[MP3] Nessun file MP3 trovato!");
    gfx->fillScreen(MP3_BG_COLOR);
    gfx->setTextColor(MP3_PAUSE_COLOR);
    gfx->setFont(u8g2_font_helvB18_tr);
    gfx->setCursor(80, 220);
    gfx->print("NESSUN FILE MP3");
    gfx->setCursor(60, 260);
    gfx->print("Inserisci file in /MP3");
    delay(2000);
    return;
  }

  Serial.printf("[MP3] Trovati %d file MP3\n", mp3Player.totalTracks);

  mp3Player.initialized = true;
  mp3PlayerInitialized = true;

  // Carica impostazioni salvate (traccia, stato riproduzione)
  loadMP3PlayerSettings();

  // Reset variabili statiche VU per evitare flickering
  vuBackgroundDrawn = false;
  prevActiveLeft = -1;
  prevActiveRight = -1;
  prevPeakLeft = -1;
  prevPeakRight = -1;

  // Disegna interfaccia iniziale
  drawMP3PlayerUI();
}

// ================== SCANSIONE FILE MP3 ==================
void scanMP3Files() {
  // Libera memoria precedente
  if (mp3Player.tracks != nullptr) {
    free(mp3Player.tracks);
    mp3Player.tracks = nullptr;
  }

  // Alloca memoria per l'array di file (usa PSRAM se disponibile)
  mp3Player.tracks = (MP3File*)heap_caps_malloc(MP3_MAX_FILES * sizeof(MP3File), MALLOC_CAP_SPIRAM);
  if (mp3Player.tracks == nullptr) {
    // Fallback a RAM normale
    mp3Player.tracks = (MP3File*)malloc(MP3_MAX_FILES * sizeof(MP3File));
    if (mp3Player.tracks == nullptr) {
      Serial.println("[MP3] ERRORE: Impossibile allocare memoria!");
      return;
    }
  }

  mp3Player.totalTracks = 0;

  File root = SD.open(MP3_FOLDER);
  if (!root) {
    Serial.println("[MP3] ERRORE: Impossibile aprire cartella /MP3");
    return;
  }

  File file = root.openNextFile();
  while (file && mp3Player.totalTracks < MP3_MAX_FILES) {
    if (!file.isDirectory()) {
      String filename = String(file.name());
      filename.toLowerCase();

      // Controlla se e' un file audio supportato
      bool isMP3 = filename.endsWith(".mp3");
      bool isWAV = filename.endsWith(".wav");

      if (isMP3 || isWAV) {
        // Copia nome file
        strncpy(mp3Player.tracks[mp3Player.totalTracks].filename, file.name(), 63);
        mp3Player.tracks[mp3Player.totalTracks].filename[63] = '\0';

        // Imposta tipo file
        mp3Player.tracks[mp3Player.totalTracks].type = isWAV ? AUDIO_TYPE_WAV : AUDIO_TYPE_MP3;

        // Estrai titolo (rimuovi estensione)
        String title = extractTitle(file.name());
        strncpy(mp3Player.tracks[mp3Player.totalTracks].title, title.c_str(), MP3_TITLE_MAX_LEN - 1);
        mp3Player.tracks[mp3Player.totalTracks].title[MP3_TITLE_MAX_LEN - 1] = '\0';

        const char* typeStr = isWAV ? "WAV" : "MP3";
        Serial.printf("[AUDIO] %d: %s [%s]\n", mp3Player.totalTracks + 1,
                      mp3Player.tracks[mp3Player.totalTracks].title, typeStr);
        mp3Player.totalTracks++;
      }
    }
    file = root.openNextFile();
  }
  root.close();
}

// ================== ESTRAI TITOLO DA FILENAME ==================
String extractTitle(const char* filename) {
  String name = String(filename);

  // Rimuovi estensione .mp3
  int dotPos = name.lastIndexOf('.');
  if (dotPos > 0) {
    name = name.substring(0, dotPos);
  }

  // Sostituisci underscore con spazi
  name.replace("_", " ");

  // Rimuovi eventuali numeri iniziali tipo "01 - " o "01_"
  if (name.length() > 3 && isDigit(name[0]) && isDigit(name[1])) {
    if (name[2] == ' ' || name[2] == '-' || name[2] == '_') {
      int start = 2;
      while (start < name.length() && (name[start] == ' ' || name[start] == '-' || name[start] == '_')) {
        start++;
      }
      name = name.substring(start);
    }
  }

  return name;
}

// ================== DISEGNA INTERFACCIA COMPLETA ==================
void drawMP3PlayerUI() {
  // Sfondo con gradiente simulato (bande orizzontali)
  for (int y = 0; y < 480; y += 4) {
    uint16_t shade = (y < 240) ? MP3_BG_COLOR : MP3_BG_DARK;
    gfx->fillRect(0, y, 480, 4, shade);
  }

  // Linee decorative sottili in alto e in basso
  gfx->drawFastHLine(0, 2, 480, MP3_ACCENT_DARK);
  gfx->drawFastHLine(0, 477, 480, MP3_ACCENT_DARK);

  // --- TASTI VOLUME AI LATI DEL BOTTONE CENTRALE ---
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(MP3_ACCENT_COLOR);

  // VOL - (Posizionato a sinistra del bottone)
  gfx->setCursor(55, 325); 
  gfx->print("VOL -");

  // VOL + (Posizionato a destra del bottone)
  gfx->setCursor(375, 325); 
  gfx->print("VOL +");

  // Header moderno
  drawMP3Header();

  // Pannello preview
  drawMP3Preview();

  // VU meters - disegna solo sfondo (i livelli verranno aggiornati dopo)
  drawVUBackground();

  // Controlli
  drawMP3Controls();

  // Barra volume
  drawMP3VolumeBar();

  // Pulsante modalita' riproduzione
  drawMP3ModeButton();

  // Tasto uscita
  drawMP3ExitButton();

  mp3Player.needsRedraw = false;
}

// ================== HEADER MODERNO ==================
void drawMP3Header() {
  int centerX = 240;

  // Icona nota musicale sinistra - Abbassata di soli 3px (Y: 28 -> 31)
  int iconL = 75;
  int iconY = 32; 
  gfx->fillCircle(iconL, iconY, 7, MP3_ACCENT_COLOR);
  gfx->fillCircle(iconL + 14, iconY - 4, 7, MP3_ACCENT_COLOR);
  gfx->fillRect(iconL + 5, iconY - 20, 3, 20, MP3_ACCENT_COLOR);
  gfx->fillRect(iconL + 19, iconY - 24, 3, 20, MP3_ACCENT_COLOR);
  gfx->fillRect(iconL + 5, iconY - 22, 17, 3, MP3_ACCENT_COLOR);

  // Titolo con font originale
  gfx->setFont(u8g2_font_helvB24_tr);
  
  // Centratura calcolata: "MP3 PLAYER" con questo font è largo circa 190px
  int textWidth = 190;
  int startX = centerX - (textWidth / 2);

  // Glow effect (testo sfumato dietro)
  gfx->setTextColor(MP3_ACCENT_DARK);
  gfx->setCursor(startX + 1, 38);
  gfx->print("MP3 PLAYER");

  // Testo principale
  gfx->setTextColor(MP3_ACCENT_COLOR);
  gfx->setCursor(startX, 37);
  gfx->print("MP3 PLAYER");

  // Linea separatrice originale
  for (int i = 0; i < 3; i++) {
    uint16_t lineColor = (i == 1) ? MP3_ACCENT_COLOR : MP3_ACCENT_DARK;
    gfx->drawFastHLine(MP3_CENTER_X, 48 + i, MP3_CENTER_W, lineColor);
  }
}

// ================== DISEGNA PREVIEW BRANO E STATO ==================
void drawMP3Preview() {
  int statusY = PREVIEW_Y + 90;
  int centerX = PREVIEW_X + PREVIEW_W / 2;
  static String lastStatusStr = ""; 
  static int lastScroll = -1;

  // 1. DISEGNO CARD E ICONA DISCO (Solo su ridisegno completo - MAI FLICKER)
  if (mp3Player.needsRedraw) {
    // Ombra della card
    gfx->fillRoundRect(PREVIEW_X + 3, PREVIEW_Y + 3, PREVIEW_W, PREVIEW_H, 12, MP3_BUTTON_SHADOW);

    // Card principale
    gfx->fillRoundRect(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, 12, MP3_BG_CARD);

    // Bordo con effetto glow
    gfx->drawRoundRect(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, 12, MP3_ACCENT_DARK);
    gfx->drawRoundRect(PREVIEW_X + 1, PREVIEW_Y + 1, PREVIEW_W - 2, PREVIEW_H - 2, 11, MP3_ACCENT_COLOR);

    // Icona disco/CD animata (RIPRISTINATA)
    int discX = PREVIEW_X + 30;
    int discY = PREVIEW_Y + 55;
    gfx->drawCircle(discX, discY, 20, MP3_ACCENT_COLOR);
    gfx->drawCircle(discX, discY, 15, MP3_ACCENT_DARK);
    gfx->drawCircle(discX, discY, 8, MP3_ACCENT_COLOR);
    gfx->fillCircle(discX, discY, 4, MP3_ACCENT_GLOW);

    // Tipo file (MP3/WAV)
    gfx->setFont(u8g2_font_helvR10_tr);
    gfx->setTextColor(MP3_ACCENT_DARK);
    const char* typeStr = (mp3Player.tracks[mp3Player.currentTrack].type == AUDIO_TYPE_WAV) ? "WAV" : "MP3";
    gfx->setCursor(PREVIEW_X + PREVIEW_W - 45, PREVIEW_Y + 22);
    gfx->print(typeStr);

    // Numero traccia
    gfx->setFont(u8g2_font_helvB12_tr);
    gfx->setTextColor(MP3_ACCENT_COLOR);
    char trackNum[32];
    snprintf(trackNum, sizeof(trackNum), "Track %d / %d", mp3Player.currentTrack + 1, mp3Player.totalTracks);
    gfx->setCursor(PREVIEW_X + 60, PREVIEW_Y + 22);
    gfx->print(trackNum);

    lastStatusStr = ""; // Forza ridisegno testi stato
    lastScroll = -1;    // Forza ridisegno titolo
  }

  // 2. TITOLO (Ridisegno solo se lo scroll è cambiato - 500ms)
  if (mp3Player.scrollOffset != lastScroll || mp3Player.needsRedraw) {
    gfx->fillRect(PREVIEW_X + 55, PREVIEW_Y + 42, PREVIEW_W - 65, 30, MP3_BG_CARD);
    gfx->setFont(u8g2_font_helvB18_tr);
    gfx->setTextColor(MP3_TEXT_COLOR);

    String title = String(mp3Player.tracks[mp3Player.currentTrack].title);
    int textWidth = title.length() * 11;
    int maxWidth = PREVIEW_W - 80;

    if (textWidth <= maxWidth) {
      int xPos = PREVIEW_X + 60 + (PREVIEW_W - 70 - textWidth) / 2;
      gfx->setCursor(xPos, PREVIEW_Y + 62);
      gfx->print(title);
    } else {
      gfx->setCursor(PREVIEW_X + 60, PREVIEW_Y + 62);
      String visiblePart = title.substring(mp3Player.scrollOffset);
      if (visiblePart.length() > 24) visiblePart = visiblePart.substring(0, 24);
      gfx->print(visiblePart);
    }
    lastScroll = mp3Player.scrollOffset;
  }

  // 3. STATO E PALLINO SX (Fissi - Ridisegno solo al cambio stato)
  String currentStatus = mp3Player.playing ? (mp3Player.paused ? "PAUSA" : "PLAY") : "STOP";
  int dotX = centerX - 75;

  if (currentStatus != lastStatusStr || mp3Player.needsRedraw) {
    // Pulizia area riga stato (Senza toccare area icona DX animata)
    gfx->fillRect(dotX - 12, statusY - 5, 125, 25, MP3_BG_CARD); 
    
    uint16_t color = (currentStatus == "PLAY") ? MP3_PLAY_COLOR : (currentStatus == "PAUSA" ? MP3_PAUSE_COLOR : MP3_TEXT_DIM);
    
    // Pallino SX Fisso
    gfx->fillCircle(dotX, statusY + 8, 6, color);
    if (currentStatus != "STOP") {
      uint16_t ring = (currentStatus == "PLAY") ? 0x2FE4 : 0x7A00;
      gfx->drawCircle(dotX, statusY + 8, 9, ring);
    }

    // Testo centrato
    gfx->setFont(u8g2_font_helvB14_tr);
    gfx->setTextColor(color);
    int xOff = (currentStatus == "PLAY") ? 20 : (currentStatus == "PAUSA" ? 27 : 22);
    gfx->setCursor(centerX - xOff, statusY + 14);
    gfx->print(currentStatus);

    // Se STOP, disegna anche il quadratino fisso a destra
    if (currentStatus == "STOP") {
      gfx->fillRect(centerX + 60 - 2, statusY - 5, 25, 25, MP3_BG_CARD);
      gfx->fillRoundRect(centerX + 60, statusY + 2, 12, 12, 2, MP3_TEXT_MUTED);
    }
    lastStatusStr = currentStatus;
  }

  // 4. ICONA DX ANIMATA (Chiamata per disegnare il primo frame o aggiornare)
  if (currentStatus != "STOP") {
    drawMP3StatusAnim();
  }
}

// ================== DISEGNA VU METER VERTICALE - MODERNO ==================
// Disegna solo lo sfondo dei VU (chiamare una volta)
void drawVUBackground() {
  // VU sinistro - design moderno con bordo doppio
  gfx->fillRoundRect(VU_LEFT_X + 2, VU_Y_START, VU_WIDTH - 4, VU_HEIGHT, 8, MP3_BG_DARK);
  gfx->drawRoundRect(VU_LEFT_X + 2, VU_Y_START, VU_WIDTH - 4, VU_HEIGHT, 8, MP3_BUTTON_BORDER);
  gfx->drawRoundRect(VU_LEFT_X + 4, VU_Y_START + 2, VU_WIDTH - 8, VU_HEIGHT - 4, 6, MP3_ACCENT_DARK);

  // Label "L" - posizionata sotto il VU meter
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(MP3_ACCENT_COLOR);
  gfx->setCursor(VU_LEFT_X + 18, VU_Y_START + VU_HEIGHT + 15);
  gfx->print("L");

  // VU destro
  gfx->fillRoundRect(VU_RIGHT_X + 2, VU_Y_START, VU_WIDTH - 4, VU_HEIGHT, 8, MP3_BG_DARK);
  gfx->drawRoundRect(VU_RIGHT_X + 2, VU_Y_START, VU_WIDTH - 4, VU_HEIGHT, 8, MP3_BUTTON_BORDER);
  gfx->drawRoundRect(VU_RIGHT_X + 4, VU_Y_START + 2, VU_WIDTH - 8, VU_HEIGHT - 4, 6, MP3_ACCENT_DARK);

  // Label "R" - posizionata sotto il VU meter
  gfx->setCursor(VU_RIGHT_X + 18, VU_Y_START + VU_HEIGHT + 15);
  gfx->print("R");

  vuBackgroundDrawn = true;
  prevActiveLeft = -1;
  prevActiveRight = -1;
  prevPeakLeft = -1;
  prevPeakRight = -1;
}

void drawVUMeterVertical(int x, int y, int w, int h, uint8_t level, uint8_t peak, bool leftSide) {
  // Calcola dimensioni segmenti con gap
  int segmentH = (h - 30) / VU_SEGMENTS;
  int segmentW = w - 16;
  int segmentX = x + 8;
  int startY = y + h - 15;  // Parte dal basso

  // Calcola quanti segmenti accendere
  int activeSegments = (level * VU_SEGMENTS) / 100;
  int peakSegment = (peak * VU_SEGMENTS) / 100;

  // Ottieni stato precedente
  int8_t prevActive = leftSide ? prevActiveLeft : prevActiveRight;
  int8_t prevPeak = leftSide ? prevPeakLeft : prevPeakRight;

  // Se nulla e' cambiato, esci
  if (activeSegments == prevActive && peakSegment == prevPeak) {
    return;
  }

  // Aggiorna solo i segmenti che cambiano
  int minSeg = (activeSegments < prevActive) ? activeSegments : prevActive;
  int maxSeg = (activeSegments > prevActive) ? activeSegments : prevActive;
  if (minSeg < 0) minSeg = 0;

  for (int i = minSeg; i <= maxSeg && i < VU_SEGMENTS; i++) {
    int segY = startY - (i + 1) * segmentH;
    uint16_t color;
    uint16_t dimColor;

    // Gradiente colore moderno: verde -> ciano -> giallo -> arancione -> rosso
    float ratio = (float)i / VU_SEGMENTS;
    if (ratio < 0.4) {
      color = MP3_VU_LOW;       // Verde (0-40%)
      dimColor = 0x0320;
    } else if (ratio < 0.55) {
      color = MP3_VU_LOW2;      // Verde-ciano (40-55%)
      dimColor = 0x0220;
    } else if (ratio < 0.70) {
      color = MP3_VU_MID;       // Giallo (55-70%)
      dimColor = 0x4200;
    } else if (ratio < 0.85) {
      color = MP3_VU_MID2;      // Arancione (70-85%)
      dimColor = 0x4100;
    } else {
      color = MP3_VU_HIGH;      // Rosso (85-100%)
      dimColor = 0x4000;
    }

    if (i < activeSegments) {
      // Segmento acceso con highlight
      gfx->fillRoundRect(segmentX, segY, segmentW, segmentH - 3, 3, color);
      // Highlight superiore per effetto 3D
      gfx->drawFastHLine(segmentX + 2, segY + 1, segmentW - 4, MP3_TEXT_COLOR);
    } else {
      // Segmento spento (visibile ma dim)
      gfx->fillRoundRect(segmentX, segY, segmentW, segmentH - 3, 3, dimColor);
    }
  }

  // Gestione picco con stile moderno (linea luminosa)
  if (peakSegment != prevPeak && peakSegment > 0 && peakSegment < VU_SEGMENTS) {
    // Cancella vecchio picco
    if (prevPeak > 0 && prevPeak < VU_SEGMENTS) {
      int oldPeakY = startY - (prevPeak + 1) * segmentH;
      float ratio = (float)prevPeak / VU_SEGMENTS;
      uint16_t oldColor;
      if (prevPeak < activeSegments) {
        if (ratio < 0.4) oldColor = MP3_VU_LOW;
        else if (ratio < 0.55) oldColor = MP3_VU_LOW2;
        else if (ratio < 0.70) oldColor = MP3_VU_MID;
        else if (ratio < 0.85) oldColor = MP3_VU_MID2;
        else oldColor = MP3_VU_HIGH;
      } else {
        if (ratio < 0.4) oldColor = 0x0320;
        else if (ratio < 0.55) oldColor = 0x0220;
        else if (ratio < 0.70) oldColor = 0x4200;
        else if (ratio < 0.85) oldColor = 0x4100;
        else oldColor = 0x4000;
      }
      gfx->fillRoundRect(segmentX, oldPeakY, segmentW, segmentH - 3, 3, oldColor);
    }

    // Disegna nuovo picco (magenta brillante)
    int newPeakY = startY - (peakSegment + 1) * segmentH;
    gfx->fillRoundRect(segmentX, newPeakY, segmentW, segmentH - 3, 3, MP3_VU_PEAK);
    gfx->drawRoundRect(segmentX - 1, newPeakY - 1, segmentW + 2, segmentH - 1, 3, MP3_TEXT_COLOR);
  }

  // Salva stato corrente
  if (leftSide) {
    prevActiveLeft = activeSegments;
    prevPeakLeft = peakSegment;
  } else {
    prevActiveRight = activeSegments;
    prevPeakRight = peakSegment;
  }
}


// ================== HELPER BOTTONE MODERNO ==================
void drawMP3ModernButton(int bx, int by, int bw, int bh, bool active, uint16_t activeColor) {
  // Ombra
  gfx->fillRoundRect(bx + 2, by + 2, bw, bh, 12, MP3_BUTTON_SHADOW);
  // Sfondo
  uint16_t bgColor = active ? activeColor : MP3_BUTTON_COLOR;
  gfx->fillRoundRect(bx, by, bw, bh, 12, bgColor);
  // Bordo esterno
  uint16_t borderColor = active ? activeColor : MP3_BUTTON_BORDER;
  gfx->drawRoundRect(bx, by, bw, bh, 12, borderColor);
  // Highlight interno superiore (effetto 3D)
  if (!active) {
    gfx->drawFastHLine(bx + 8, by + 4, bw - 16, MP3_BUTTON_HOVER);
  }
}

// ================== DISEGNA CONTROLLI - DESIGN MODERNO ==================
void drawMP3Controls() {
  int centerX = 240;
  int btnY = CONTROLS_Y;

  // Calcola posizioni pulsanti (4 pulsanti: prev, stop, play/pause, next)
  int totalWidth = 4 * BTN_SIZE + 3 * BTN_SPACING;
  int startX = centerX - totalWidth / 2;

  // Pulsante PREV (<<)
  int prevX = startX;
  drawMP3ModernButton(prevX, btnY, BTN_SIZE, BTN_SIZE, false, 0);
  // Icona freccia doppia sinistra
  int cx = prevX + BTN_SIZE / 2;
  int cy = btnY + BTN_SIZE / 2;
  gfx->fillTriangle(cx - 5, cy, cx + 5, cy - 10, cx + 5, cy + 10, MP3_ACCENT_COLOR);
  gfx->fillTriangle(cx - 15, cy, cx - 5, cy - 10, cx - 5, cy + 10, MP3_ACCENT_COLOR);

  // Pulsante STOP
  int stopX = startX + BTN_SIZE + BTN_SPACING;
  drawMP3ModernButton(stopX, btnY, BTN_SIZE, BTN_SIZE, false, 0);
  // Icona stop (quadrato)
  cx = stopX + BTN_SIZE / 2;
  cy = btnY + BTN_SIZE / 2;
  gfx->fillRoundRect(cx - 10, cy - 10, 20, 20, 3, MP3_STOP_COLOR);

  // Pulsante PLAY/PAUSE (centrale - più grande)
  int playX = startX + 2 * (BTN_SIZE + BTN_SPACING);
  int playBtnW = BTN_SIZE + 4;
  bool isPlaying = mp3Player.playing && !mp3Player.paused;
  uint16_t playColor = isPlaying ? MP3_PAUSE_COLOR : MP3_PLAY_COLOR;
  drawMP3ModernButton(playX - 2, btnY - 3, playBtnW, BTN_SIZE + 6, true, playColor);

  cx = playX + BTN_SIZE / 2;
  cy = btnY + BTN_SIZE / 2;
  if (isPlaying) {
    // Icona pausa ||
    gfx->fillRoundRect(cx - 12, cy - 12, 8, 24, 2, MP3_TEXT_COLOR);
    gfx->fillRoundRect(cx + 4, cy - 12, 8, 24, 2, MP3_TEXT_COLOR);
  } else {
    // Icona play (triangolo)
    gfx->fillTriangle(cx - 8, cy - 14, cx - 8, cy + 14, cx + 14, cy, MP3_TEXT_COLOR);
  }

  // Pulsante NEXT (>>)
  int nextX = startX + 3 * (BTN_SIZE + BTN_SPACING);
  drawMP3ModernButton(nextX, btnY, BTN_SIZE, BTN_SIZE, false, 0);
  // Icona freccia doppia destra
  cx = nextX + BTN_SIZE / 2;
  cy = btnY + BTN_SIZE / 2;
  gfx->fillTriangle(cx + 5, cy, cx - 5, cy - 10, cx - 5, cy + 10, MP3_ACCENT_COLOR);
  gfx->fillTriangle(cx + 15, cy, cx + 5, cy - 10, cx + 5, cy + 10, MP3_ACCENT_COLOR);
}

// ================== DISEGNA BARRA VOLUME - DESIGN MODERNO ==================
// ================== DISEGNA BARRA VOLUME - DESIGN MODERNO ==================
void drawMP3VolumeBar() {
  int barX = 95;
  int barW = 280; // Ridotto da 290 a 280 per non sbordare nel VU Meter destro
  int barH = 30; // Altezza fissa per le tacchette
  int y = VOLUME_BAR_Y;
  int numSteps = 21; // Numero di tacchette

  // 1. PULIZIA AREA (Larghezza ricalcolata per non toccare i VU Meter laterali)
  gfx->fillRect(barX - 45, y - 5, barW + 82, barH + 10, MP3_BG_COLOR);

  // 2. ICONA SPEAKER (Stile Web Radio)
  int spkX = barX - 35;
  int spkY = y + (barH / 2);
  gfx->fillRect(spkX, spkY - 4, 5, 8, MP3_ACCENT_COLOR);
  gfx->fillTriangle(spkX + 5, spkY - 8, spkX + 5, spkY + 8, spkX + 12, spkY, MP3_ACCENT_COLOR);
  gfx->drawFastVLine(spkX + 15, spkY - 3, 6, MP3_ACCENT_COLOR);
  gfx->drawFastVLine(spkX + 18, spkY - 6, 12, MP3_ACCENT_COLOR);

  // 3. DISEGNO TACCHETTE
  // Converte il volume 0-100 nel numero di tacchette 0-21
  int activeSteps = map(mp3Player.volume, 0, 100, 0, numSteps);
  int spacing = barW / numSteps;

  for (int i = 1; i <= numSteps; i++) {
    int tickH = map(i, 1, numSteps, 4, barH); // Tacchette che crescono in altezza
    int tickX = barX + (i - 1) * spacing;
    int tickY = y + barH - tickH;

    uint16_t color = (i <= activeSteps) ? MP3_ACCENT_COLOR : MP3_BG_CARD;
    gfx->fillRect(tickX, tickY, 3, tickH, color);
  }

  // 4. VALORE NUMERICO
  int valX = barX + barW + 10;
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(MP3_ACCENT_COLOR);
  
  String volStr = String(mp3Player.volume);
  // Allineamento ottimizzato per 1, 2 o 3 cifre per evitare di toccare il VU meter
  int textOffset = (mp3Player.volume < 10) ? 10 : (mp3Player.volume == 100 ? -5 : 0);
  gfx->setCursor(valX + textOffset, y + barH - 2);
  gfx->print(volStr);
}

// ================== DISEGNA PULSANTE MODALITA' - SINGOLO/TUTTI ==================
void drawMP3ModeButton() {
  int btnW = 280; // Ripristinato larghezza originale
  int btnX = 240 - btnW / 2;
  int y = 360;    // Posizionato sotto l'area dove ora metterai i tasti Volume

  // Ombra
  gfx->fillRoundRect(btnX + 2, y + 2, btnW, MODE_BTN_H, 12, MP3_BUTTON_SHADOW);

  // Sfondo pulsante
  uint16_t bgColor = mp3Player.playAll ? MP3_PLAY_COLOR : MP3_ACCENT_DARK;
  gfx->fillRoundRect(btnX, y, btnW, MODE_BTN_H, 12, bgColor);

  // Bordo
  gfx->drawRoundRect(btnX, y, btnW, MODE_BTN_H, 12, mp3Player.playAll ? 0x2FE4 : MP3_ACCENT_COLOR);

  // Highlight superiore
  gfx->drawFastHLine(btnX + 10, y + 3, btnW - 20, mp3Player.playAll ? 0x5FE0 : MP3_ACCENT_GLOW);

  // Icona e testo
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(MP3_TEXT_COLOR);

  if (mp3Player.playAll) {
    // Icona repeat all (due frecce circolari)
    int iconX = btnX + 25;
    int iconY = y + MODE_BTN_H / 2;
    gfx->drawCircle(iconX, iconY, 8, MP3_TEXT_COLOR);
    gfx->fillTriangle(iconX + 6, iconY - 2, iconX + 6, iconY + 4, iconX + 12, iconY + 1, MP3_TEXT_COLOR);

    gfx->setCursor(btnX + 55, y + 28);
    gfx->print("TUTTI I BRANI");
  } else {
    // Icona singolo (nota musicale)
    int iconX = btnX + 25;
    int iconY = y + MODE_BTN_H / 2;
    gfx->fillCircle(iconX, iconY + 4, 5, MP3_TEXT_COLOR);
    gfx->fillRect(iconX + 3, iconY - 8, 2, 14, MP3_TEXT_COLOR);

    gfx->setCursor(btnX + 55, y + 28);
    gfx->print("SINGOLO BRANO");
  }

  // Indicatore stato a destra
  int indX = btnX + btnW - 35;
  int indY = y + MODE_BTN_H / 2;
  gfx->fillCircle(indX, indY, 8, mp3Player.playAll ? MP3_PLAY_COLOR : MP3_TEXT_MUTED);
  gfx->drawCircle(indX, indY, 10, MP3_TEXT_COLOR);
}

// ================== PULSANTE MODE >> (basso centro) ==================
void drawMP3ExitButton() {
  gfx->setFont(u8g2_font_helvR08_tr);
  gfx->setTextColor(MP3_ACCENT_COLOR);
  gfx->setCursor(210, 479);
  gfx->print("MODE >>");
}

// ================== GESTIONE TOUCH ==================
bool handleMP3PlayerTouch(int16_t x, int16_t y) {
  if (!mp3Player.initialized) return false;

  // Pulsante MODE >> (basso centro y>455, x 180-310)
  if (y > 455 && x >= 180 && x <= 310) {
    Serial.println("[MP3] Pulsante MODE >> premuto");
    stopMP3Track();
    saveMP3PlayerSettings();
    cleanupMP3Player();
    return true;  // Segnala di uscire dalla modalita'
  }

  // Area controlli
  if (y >= CONTROLS_Y && y <= CONTROLS_Y + BTN_SIZE + 6) {
    int centerX = 240;
    int totalWidth = 4 * BTN_SIZE + 3 * BTN_SPACING;
    int startX = centerX - totalWidth / 2;

    // PREV
    if (x >= startX && x <= startX + BTN_SIZE) {
      Serial.println("[MP3] Tasto PREV premuto");
      prevMP3Track();
      // NO playTouchSound() - interferirebbe con la riproduzione MP3
      return false;
    }

    // STOP
    int stopX = startX + BTN_SIZE + BTN_SPACING;
    if (x >= stopX && x <= stopX + BTN_SIZE) {
      Serial.println("[MP3] Tasto STOP premuto");
      stopMP3Track();
      // NO playTouchSound() - interferirebbe con la riproduzione MP3
      mp3Player.needsRedraw = true;
      return false;
    }

    // PLAY/PAUSE - debounce lungo per evitare tocchi multipli
    int playX = startX + 2 * (BTN_SIZE + BTN_SPACING);
    if (x >= playX && x <= playX + BTN_SIZE) {
      static uint32_t lastPlayPauseTouch = 0;
      uint32_t now = millis();
      if (now - lastPlayPauseTouch < 700) {
        // Ignora tocco troppo ravvicinato
        return false;
      }
      lastPlayPauseTouch = now;

      if (mp3Player.playing && !mp3Player.paused) {
        Serial.println("[MP3] Tasto PAUSE premuto");
        pauseMP3Track();
      } else if (mp3Player.paused) {
        Serial.println("[MP3] Tasto RESUME premuto");
        resumeMP3Track();
      } else {
        Serial.println("[MP3] Tasto PLAY premuto");
        playMP3Track(mp3Player.currentTrack);
      }
      // NO playTouchSound() - interferirebbe con la riproduzione MP3
      mp3Player.needsRedraw = true;
      return false;
    }

    // NEXT
    int nextX = startX + 3 * (BTN_SIZE + BTN_SPACING);
    if (x >= nextX && x <= nextX + BTN_SIZE) {
      Serial.println("[MP3] Tasto NEXT premuto");
      nextMP3Track();
      // NO playTouchSound() - interferirebbe con la riproduzione MP3
      return false;
    }
  }
    int volStep = 5; // Corrisponde a una tacchetta della barra da 21 step

    // Area VOL - (Sinistra del selettore)
    if (x > 40 && x < 120 && y > 300 && y < 350) {
        if (mp3Player.volume >= volStep) {
            mp3Player.volume -= volStep;
        } else {
            mp3Player.volume = 0;
        }
       #ifdef AUDIO
        extern Audio audio;
        // Sincronizza l'hardware: mappa il volume 0-100 sulla scala 0-21
        audio.setVolume(map(mp3Player.volume, 0, 100, 0, 21));
        #endif

        Serial.printf("[MP3] Volume ridotto a %d%%\n", mp3Player.volume);
        mp3VolumeNeedsRedraw = true;
        return false;
    }

    // Area VOL + (Destra del selettore)
    if (x > 360 && x < 440 && y > 300 && y < 350) {
        if (mp3Player.volume <= (100 - volStep)) {
            mp3Player.volume += volStep;
        } else {
            mp3Player.volume = 100;
        }
        #ifdef AUDIO
        extern Audio audio;
        // Sincronizza l'hardware: mappa il volume 0-100 sulla scala 0-21
        audio.setVolume(map(mp3Player.volume, 0, 100, 0, 21));
        #endif

        Serial.printf("[MP3] Volume aumentato a %d%%\n", mp3Player.volume);
        mp3VolumeNeedsRedraw = true;
        return false;
    }

  // Area barra volume (touch diretto)
  int volBarX = 95;
  int volBarW = 290;
  if (y >= VOLUME_BAR_Y && y <= VOLUME_BAR_Y + VOLUME_BAR_H) {
    if (x >= volBarX && x <= volBarX + volBarW) {
      // Calcola nuovo volume dalla posizione X (0-100)
      int newVol = map(x - volBarX, 0, volBarW, 0, 100);
      newVol = constrain(newVol, 0, 100);
      if (newVol != mp3Player.volume) {
        mp3Player.volume = newVol;
        #ifdef AUDIO
        extern Audio audio;
        audio.setVolume(map(mp3Player.volume, 0, 100, 0, 21));  // Converte 0-100 a 0-21
        #endif
        //saveMP3PlayerSettings();
        Serial.printf("[MP3] Volume impostato a %d%%\n", mp3Player.volume);
        mp3VolumeNeedsRedraw = true;
      }
      return false;
    }
  }

// Area pulsante modalita' (SINGOLO/TUTTI)
  int modeBtnW = 280;
  int modeBtnX = 240 - modeBtnW / 2;
  int targetY = 360; // La nuova coordinata Y che abbiamo scelto
  if (y >= targetY && y <= targetY + MODE_BTN_H) {
    if (x >= modeBtnX && x <= modeBtnX + modeBtnW) {
      // Toggle modalita'
      mp3Player.playAll = !mp3Player.playAll;
      Serial.printf("[MP3] Modalita' cambiata: %s\n", mp3Player.playAll ? "TUTTI I BRANI" : "SINGOLO BRANO");
      //saveMP3PlayerSettings();
      mp3Player.needsRedraw = true;
      return false;
    }
  }

  // Touch nella preview (cambia traccia)
  if (y >= PREVIEW_Y && y <= PREVIEW_Y + PREVIEW_H) {
    if (x < 240) {
      // Meta' sinistra = traccia precedente
      prevMP3Track();
    } else {
      // Meta' destra = traccia successiva
      nextMP3Track();
    }
    // NO playTouchSound() - interferirebbe con la riproduzione MP3
    mp3Player.needsRedraw = true;
    return false;
  }

  return false;
}

// mp3AudioTask, startMP3Task, stopMP3Task rimossi
// audioTask globale (audio.loop()) gestisce la riproduzione

// ================== RIPRODUZIONE ==================
void playMP3Track(int index) {
  #ifdef AUDIO
  if (index < 0 || index >= mp3Player.totalTracks) return;

  extern Audio audio;
  extern bool webRadioEnabled;

  // Ferma web radio se attiva
  if (webRadioEnabled) {
    audio.stopSong();
    delay(50);
  }

  // Ferma eventuale riproduzione
  stopMP3Track();
  mp3Player.currentTrack = index;

  String fullPath = String(MP3_FOLDER) + "/" + String(mp3Player.tracks[index].filename);
  Serial.printf("[MP3] Riproduzione: %s (volume: %d)\n", fullPath.c_str(), mp3Player.volume);

  if (!SD.exists(fullPath)) {
    Serial.println("[MP3] ERRORE: File non trovato!");
    return;
  }

  // Imposta volume MP3 Player (converte 0-100 a 0-21)
  audio.setVolume(map(mp3Player.volume, 0, 100, 0, 21));

  // Usa audio.connecttoFS con SD
  if (!audio.connecttoFS(SD, fullPath.c_str())) {
    Serial.println("[MP3] Errore connessione file!");
    return;
  }

  mp3Player.playing = true;
  mp3Player.paused = false;
  isPlaying = true;
  //saveMP3PlayerSettings();
  mp3Player.needsRedraw = true;
  #endif
}

void stopMP3Track() {
  #ifdef AUDIO
  extern Audio audio;
  extern bool webRadioEnabled;
  extern String webRadioUrl;
  extern uint8_t webRadioVolume;

  audio.stopSong();

  mp3Player.playing = false;
  mp3Player.paused = false;
  isPlaying = false;
  mp3Player.vuLeft = 0;
  mp3Player.vuRight = 0;
  mp3Player.vuPeakLeft = 0;
  mp3Player.vuPeakRight = 0;
  //saveMP3PlayerSettings();

  // Riprendi web radio se era attiva
  if (webRadioEnabled) {
    Serial.println("[MP3] Riprendo web radio...");
    audio.setVolume(map(webRadioVolume, 0, 100, 0, 21));  // Ripristina volume web radio
    delay(100);
    audio.connecttohost(webRadioUrl.c_str());
  }

  Serial.println("[MP3] Riproduzione fermata");
  #endif
}

void pauseMP3Track() {
  #ifdef AUDIO
  extern Audio audio;
  if (mp3Player.playing && !mp3Player.paused) {
    audio.pauseResume();  // Audio.h supporta pause nativo
    mp3Player.paused = true;
    //saveMP3PlayerSettings();
    Serial.println("[MP3] Riproduzione in pausa");
  }
  #endif
}

void resumeMP3Track() {
  #ifdef AUDIO
  extern Audio audio;
  if (mp3Player.paused) {
    audio.pauseResume();
    mp3Player.paused = false;
    //saveMP3PlayerSettings();
    Serial.println("[MP3] Riproduzione ripresa");
  }
  #endif
}

void nextMP3Track() {
  if (mp3Player.totalTracks == 0) return;

  int nextIndex = (mp3Player.currentTrack + 1) % mp3Player.totalTracks;

  if (mp3Player.playing) {
    playMP3Track(nextIndex);
  } else {
    mp3Player.currentTrack = nextIndex;
    mp3Player.needsRedraw = true;
  }

  mp3Player.scrollOffset = 0;  // Reset scroll per nuovo titolo
}

void prevMP3Track() {
  if (mp3Player.totalTracks == 0) return;

  int prevIndex = mp3Player.currentTrack - 1;
  if (prevIndex < 0) prevIndex = mp3Player.totalTracks - 1;

  if (mp3Player.playing) {
    playMP3Track(prevIndex);
  } else {
    mp3Player.currentTrack = prevIndex;
    mp3Player.needsRedraw = true;
  }

  mp3Player.scrollOffset = 0;  // Reset scroll per nuovo titolo
}

// ================== AGGIORNAMENTO VU METERS ==================
void updateMP3VUMeters() {
  uint32_t now = millis();

  // Aggiorna VU ogni 30ms
  if (now - mp3Player.lastVuUpdate < 30) return;
  mp3Player.lastVuUpdate = now;

  // Controlla se c'e' riproduzione attiva (MP3 o Web Radio)
  extern bool webRadioEnabled;
  #ifdef AUDIO
  extern Audio audio;
  bool audioRunning = audio.isRunning();
  #else
  bool audioRunning = false;
  #endif

  bool mp3Active = mp3Player.playing && !mp3Player.paused;
  bool webRadioActive = webRadioEnabled && audioRunning && !mp3Active;
  bool anyAudioPlaying = mp3Active || webRadioActive;

  if (anyAudioPlaying) {
    // Simula livelli VU con variazione pseudo-casuale
    static uint8_t targetLeft = 0;
    static uint8_t targetRight = 0;

    // Genera nuovi target periodicamente
    if (random(100) < 30) {
      targetLeft = random(40, 95);
      targetRight = random(40, 95);
    }

    // Smooth attack/decay
    if (mp3Player.vuLeft < targetLeft) {
      int newVal = mp3Player.vuLeft + 8;
      mp3Player.vuLeft = (newVal > targetLeft) ? targetLeft : (uint8_t)newVal;
    } else {
      int newVal = mp3Player.vuLeft - 3;
      mp3Player.vuLeft = (newVal < targetLeft) ? targetLeft : (uint8_t)newVal;
    }

    if (mp3Player.vuRight < targetRight) {
      int newVal = mp3Player.vuRight + 8;
      mp3Player.vuRight = (newVal > targetRight) ? targetRight : (uint8_t)newVal;
    } else {
      int newVal = mp3Player.vuRight - 3;
      mp3Player.vuRight = (newVal < targetRight) ? targetRight : (uint8_t)newVal;
    }

    // Aggiorna picchi
    if (mp3Player.vuLeft > mp3Player.vuPeakLeft) {
      mp3Player.vuPeakLeft = mp3Player.vuLeft;
    } else if (mp3Player.vuPeakLeft > 0) {
      mp3Player.vuPeakLeft--;
    }

    if (mp3Player.vuRight > mp3Player.vuPeakRight) {
      mp3Player.vuPeakRight = mp3Player.vuRight;
    } else if (mp3Player.vuPeakRight > 0) {
      mp3Player.vuPeakRight--;
    }
  } else {
    // Nessuna riproduzione attiva: azzera immediatamente i VU
    mp3Player.vuLeft = 0;
    mp3Player.vuRight = 0;
    mp3Player.vuPeakLeft = 0;
    mp3Player.vuPeakRight = 0;
  }
}

// ================== AGGIORNAMENTO PRINCIPALE ==================
// Variabili statiche per throttling display
static uint32_t lastDisplayUpdate = 0;
static uint32_t lastVUUpdate = 0;
#define DISPLAY_UPDATE_INTERVAL 500   // Aggiorna display ogni 200ms
#define VU_UPDATE_INTERVAL 80         // Aggiorna VU ogni 80ms

void updateMP3Player() {
  static int lastActiveMode = -1;
  if (lastActiveMode != MODE_MP3_PLAYER) {
    mp3Player.needsRedraw = true;
    mp3Player.initialized = false;
  }
  lastActiveMode = currentMode;

  if (!mp3Player.initialized) {
    initMP3Player();
    return;
  }

  uint32_t now = millis();

  // 1. GESTIONE SCROLL (Deve essere fuori dai blocchi if playing/paused)
  // Lo facciamo ogni 500ms come hai richiesto
  static uint32_t lastScrollTime = 0;
  if (now - lastScrollTime >= 500) { 
    if (mp3Player.totalTracks > 0) {
      String title = String(mp3Player.tracks[mp3Player.currentTrack].title);
      // Se il titolo è lungo, sposta l'offset e ridisegna la preview
      if (title.length() > 20) {
        mp3Player.scrollOffset++;
        if (mp3Player.scrollOffset > (int)(title.length() - 15)) {
          mp3Player.scrollOffset = 0;
        }
        drawMP3Preview(); // Aggiorna il titolo che scorre
      }
    }
    lastScrollTime = now;
  }

  // 2. GESTIONE AUDIO (EOF e Track Ended)
  #ifdef AUDIO
  extern Audio audio;
  if (mp3Player.trackEnded) {
    mp3Player.trackEnded = false;
    if (mp3Player.playAll) {
      nextMP3Track();
      if (mp3Player.totalTracks > 0) playMP3Track(mp3Player.currentTrack);
    } else {
      mp3Player.needsRedraw = true;
    }
  }
  #endif

  // 3. TOUCH gestito interamente da checkButtons() in 1_TOUCH.ino
  // (evita doppio ts.read() che causa perdita touch events)

  // 4. RIDISEGNO COMPLETO (Solo se necessario)
  if (mp3Player.needsRedraw) {
    drawMP3PlayerUI();
    mp3Player.needsRedraw = false;
    mp3VolumeNeedsRedraw = false;
    // Disegniamo la preview la prima volta
    drawMP3Preview();
  }

  if (mp3VolumeNeedsRedraw) {
    drawMP3VolumeBar();
    mp3VolumeNeedsRedraw = false;
  }

  // 5. ANIMAZIONE SIMBOLI (Solo in Play o Pausa)
  static uint32_t lastStatusAnim = 0;
  if ((mp3Player.playing || mp3Player.paused) && (now - lastStatusAnim >= 50)) {
    drawMP3StatusAnim(); // Aggiorna solo le icone che pulsano
    lastStatusAnim = now;
  }

  // 6. VU METERS (Solo se in play)
  static uint32_t lastVUUpdate = 0;
  if (mp3Player.playing && !mp3Player.paused && (now - lastVUUpdate >= 100)) {
    updateMP3VUMeters();
    drawVUMeterVertical(VU_LEFT_X, VU_Y_START, VU_WIDTH, VU_HEIGHT, mp3Player.vuLeft, mp3Player.vuPeakLeft, true);
    drawVUMeterVertical(VU_RIGHT_X, VU_Y_START, VU_WIDTH, VU_HEIGHT, mp3Player.vuRight, mp3Player.vuPeakRight, false);
    lastVUUpdate = now;
  }
}

void drawMP3StatusAnim() {
  int statusY = PREVIEW_Y + 90;
  int centerX = PREVIEW_X + PREVIEW_W / 2;
  int iconX = centerX + 60;

  float breath = 0.5 + 0.5 * sin(2.0 * 3.14159 * (millis() % 1500) / 1500.0);
  uint8_t s = (uint8_t)(50 + 205 * breath); // Minimo 50 per non farlo sparire del tutto

  // --- SOLUZIONE: NON USARE fillRect QUI! ---
  // Se non cancelli lo sfondo, il flicker sparisce.
  
  if (mp3Player.playing && !mp3Player.paused) {
    uint8_t r = (uint8_t)((MP3_PLAY_COLOR >> 11) & 0x1F);
    uint8_t g = (uint8_t)((MP3_PLAY_COLOR >> 5) & 0x3F);
    uint8_t b = (uint8_t)(MP3_PLAY_COLOR & 0x1F);
    uint16_t bColor = ((uint16_t)(r * s / 255) << 11) | ((uint16_t)(g * s / 255) << 5) | ((uint16_t)(b * s / 255));
    
    // Disegna il triangolo direttamente. 
    // Il nuovo colore coprirà il vecchio senza passare dal nero.
    gfx->fillTriangle(iconX, statusY + 2, iconX, statusY + 14, iconX + 12, statusY + 8, bColor);
  } 
  else if (mp3Player.paused) {
    uint8_t r = (uint8_t)((MP3_PAUSE_COLOR >> 11) & 0x1F);
    uint8_t g = (uint8_t)((MP3_PAUSE_COLOR >> 5) & 0x3F);
    uint8_t b = (uint8_t)(MP3_PAUSE_COLOR & 0x1F);
    uint16_t bColor = ((uint16_t)(r * s / 255) << 11) | ((uint16_t)(g * s / 255) << 5) | ((uint16_t)(b * s / 255));
    
    gfx->fillRect(iconX, statusY + 2, 5, 12, bColor);
    gfx->fillRect(iconX + 8, statusY + 2, 5, 12, bColor);
  }
}
// ================== CLEANUP ==================
void cleanupMP3Player() {
  stopMP3Track();

  if (mp3Player.tracks != nullptr) {
    heap_caps_free(mp3Player.tracks);
    mp3Player.tracks = nullptr;
  }

  mp3Player.initialized = false;
  mp3PlayerInitialized = false;
  mp3Player.totalTracks = 0;

  Serial.println("[MP3] Lettore MP3 chiuso");
}

// ================== WEBSERVER MP3 PLAYER ==================
// HTML per pagina controllo MP3 Player
const char MP3_PLAYER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>MP3 Player</title><style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,sans-serif;background:#1a1a2e;min-height:100vh;padding:20px;color:#eee}
.c{max-width:600px;margin:0 auto;background:rgba(255,255,255,.1);border-radius:20px;overflow:hidden}
.h{background:linear-gradient(135deg,#9c27b0,#7b1fa2);padding:25px;text-align:center}
.h h1{font-size:1.5em;margin-bottom:5px}.h p{opacity:.8;font-size:.9em}
.ct{padding:25px}
.s{background:rgba(0,0,0,.3);border-radius:15px;padding:20px;margin-bottom:20px}
.s h3{margin-bottom:15px;color:#ce93d8}
.nav{display:flex;justify-content:space-between;padding:10px 15px}
.nav a{color:#94a3b8;text-decoration:none;font-size:.9em;padding:5px 10px}
.nav a:hover{color:#fff}
.track{padding:12px;background:rgba(255,255,255,.1);border-radius:8px;margin-bottom:8px;cursor:pointer;display:flex;justify-content:space-between;align-items:center}
.track:hover{background:rgba(156,39,176,.3)}
.track.active{background:rgba(156,39,176,.5);border:2px solid #ce93d8}
.track .name{flex:1}
.track .type{font-size:0.8em;opacity:0.7;padding:2px 8px;background:rgba(255,255,255,.1);border-radius:4px}
.controls{display:flex;justify-content:center;gap:15px;margin:20px 0}
.btn{width:60px;height:60px;border-radius:50%;border:none;cursor:pointer;font-size:1.5em;display:flex;align-items:center;justify-content:center;transition:all 0.3s}
.btn-prev,.btn-next{background:rgba(255,255,255,.1);color:#fff}
.btn-prev:hover,.btn-next:hover{background:rgba(255,255,255,.2)}
.btn-play{background:#4CAF50;color:#fff;width:70px;height:70px}
.btn-play.playing{background:#ff9800}
.btn-stop{background:#f44336;color:#fff}
.status{text-align:center;padding:20px;background:rgba(0,0,0,.2);border-radius:15px;margin-bottom:20px}
.status.playing{border:2px solid #4CAF50;background:rgba(76,175,80,.1)}
.status .now{font-size:1.2em;font-weight:bold;margin-bottom:5px;color:#ce93d8}
.status .state{opacity:0.7}
.vu{display:flex;justify-content:center;gap:4px;height:50px;align-items:flex-end;margin:15px 0}
.vu-bar{width:10px;background:linear-gradient(to top,#9c27b0,#e91e63,#ff5722);border-radius:3px;transition:height 0.1s}
.empty{text-align:center;padding:40px;opacity:0.6}
.mode-btn{width:100%;padding:15px;border:none;border-radius:10px;font-weight:bold;cursor:pointer;font-size:1em;margin-top:10px;background:linear-gradient(135deg,#9c27b0,#7b1fa2);color:#fff}
.mode-btn:hover{transform:scale(1.02)}
.playmode{display:flex;gap:10px;margin:15px 0}
.playmode-btn{flex:1;padding:12px;border:2px solid #444;border-radius:10px;background:rgba(0,0,0,.3);color:#fff;cursor:pointer;text-align:center;transition:all 0.3s}
.playmode-btn.active{border-color:#4CAF50;background:rgba(76,175,80,.3)}
.playmode-btn:hover{background:rgba(255,255,255,.1)}
.volume-row{display:flex;align-items:center;gap:15px;margin-top:15px}
.volume-slider{flex:1;height:10px;-webkit-appearance:none;background:#2a2a4a;border-radius:5px;border:1px solid #555}
.volume-slider::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;background:#ce93d8;border-radius:50%;cursor:pointer;box-shadow:0 0 10px rgba(206,147,216,.5)}
.vol-val{min-width:50px;text-align:center;color:#ce93d8;font-weight:bold;font-size:1.2em}
</style></head><body><div class="c">
<div class="nav"><a href="/">&larr; Home</a><a href="/settings">Settings &rarr;</a></div>
<div class="h"><h1>🎵 MP3 PLAYER</h1><p>Riproduci musica dalla SD card</p></div><div class="ct">
<div class="status" id="statusBox">
<div class="now" id="nowPlaying">Nessun brano selezionato</div>
<div class="state" id="playState">Fermo</div>
</div>
<div class="vu" id="vuMeter"></div>
<div class="controls">
<button class="btn btn-prev" onclick="cmd('prev')">⏮</button>
<button class="btn btn-stop" onclick="cmd('stop')">⏹</button>
<button class="btn btn-play" id="playBtn" onclick="cmd('toggle')">▶</button>
<button class="btn btn-next" onclick="cmd('next')">⏭</button>
</div>
<div class="s"><h3>Volume</h3>
<div class="volume-row">
<span style="font-size:1.3em">🔊</span>
<input type="range" class="volume-slider" id="volume" min="0" max="100" onchange="setVolume()">
<span class="vol-val" id="volVal">70%</span>
</div>
</div>
<div class="playmode">
<div class="playmode-btn" id="modeSingle" onclick="setMode(false)">🎵 Singolo</div>
<div class="playmode-btn" id="modeAll" onclick="setMode(true)">🔁 Tutti</div>
</div>
<div class="s"><h3>Playlist</h3>
<div id="playlist"></div>
</div>
<button class="mode-btn" onclick="activateMode()">Mostra su Display</button>
</div></div><script>
var cfg={playing:false,paused:false,current:-1,playAll:false,volume:70,tracks:[]};
function cmd(c,idx){
  var url='/mp3player/cmd?action='+c;
  if(typeof idx!=='undefined')url+='&track='+idx;
  fetch(url).then(r=>r.json()).then(d=>{update(d)});
}
function setMode(all){
  fetch('/mp3player/cmd?action=setmode&playall='+(all?1:0)).then(r=>r.json()).then(d=>{update(d)});
}
function setVolume(){
  var vol=document.getElementById('volume').value;
  document.getElementById('volVal').textContent=vol+'%';
  fetch('/mp3player/cmd?action=volume&value='+vol);
  cfg.volume=vol;
}
function update(d){
  cfg.playing=d.playing;cfg.paused=d.paused;cfg.current=d.current;cfg.playAll=d.playAll;
  cfg.volume=d.volume||70;cfg.tracks=d.tracks||[];
  document.getElementById('playBtn').innerHTML=cfg.playing&&!cfg.paused?'⏸':'▶';
  document.getElementById('playBtn').className='btn btn-play'+(cfg.playing&&!cfg.paused?' playing':'');
  var stateText=cfg.playing?(cfg.paused?'In pausa':'In riproduzione'):'Fermo';
  document.getElementById('playState').textContent=stateText;
  document.getElementById('statusBox').className='status'+(cfg.playing&&!cfg.paused?' playing':'');
  document.getElementById('modeSingle').className='playmode-btn'+(cfg.playAll?'':' active');
  document.getElementById('modeAll').className='playmode-btn'+(cfg.playAll?' active':'');
  document.getElementById('volume').value=cfg.volume;
  document.getElementById('volVal').textContent=cfg.volume+'%';
  if(cfg.tracks.length>0){
    document.getElementById('nowPlaying').textContent=cfg.current>=0?cfg.tracks[cfg.current].title:'Seleziona un brano';
    var html='';
    for(var i=0;i<cfg.tracks.length;i++){
      html+='<div class="track'+(i==cfg.current?' active':'')+'" onclick="cmd(\'play\','+i+')">';
      html+='<span class="name">'+(i+1)+'. '+cfg.tracks[i].title+'</span>';
      html+='<span class="type">'+cfg.tracks[i].type+'</span></div>';
    }
    document.getElementById('playlist').innerHTML=html;
  }else{
    document.getElementById('playlist').innerHTML='<div class="empty">Nessun file MP3/WAV trovato nella cartella /MP3</div>';
  }
  updateVU(d.vuL||0,d.vuR||0);
}
function updateVU(l,r){
  var vu=document.getElementById('vuMeter');
  var html='';
  for(var i=0;i<8;i++){html+='<div class="vu-bar" style="height:'+Math.max(5,l>i*12?Math.min(50,l/2):5)+'px"></div>';}
  for(var i=7;i>=0;i--){html+='<div class="vu-bar" style="height:'+Math.max(5,r>i*12?Math.min(50,r/2):5)+'px"></div>';}
  vu.innerHTML=html;
}
function activateMode(){
  fetch('/mp3player/activate');
}
function poll(){fetch('/mp3player/status').then(r=>r.json()).then(d=>{update(d)}).catch(()=>{});}
poll();setInterval(poll,2000);
</script></body></html>
)rawliteral";

// Funzione per ottenere lo stato JSON del player
String getMP3PlayerStatusJSON() {
  String json = "{";
  json += "\"playing\":" + String(mp3Player.playing ? "true" : "false") + ",";
  json += "\"paused\":" + String(mp3Player.paused ? "true" : "false") + ",";
  json += "\"playAll\":" + String(mp3Player.playAll ? "true" : "false") + ",";
  json += "\"current\":" + String(mp3Player.currentTrack) + ",";
  json += "\"total\":" + String(mp3Player.totalTracks) + ",";
  json += "\"volume\":" + String(mp3Player.volume) + ",";
  json += "\"vuL\":" + String(mp3Player.vuLeft) + ",";
  json += "\"vuR\":" + String(mp3Player.vuRight) + ",";
  json += "\"tracks\":[";

  for (int i = 0; i < mp3Player.totalTracks && i < 50; i++) {
    if (i > 0) json += ",";
    json += "{\"title\":\"" + String(mp3Player.tracks[i].title) + "\",";
    json += "\"type\":\"" + String(mp3Player.tracks[i].type == AUDIO_TYPE_WAV ? "WAV" : "MP3") + "\"}";
  }

  json += "]}";
  return json;
}

// ================== SALVATAGGIO/CARICAMENTO IMPOSTAZIONI MP3 ==================
// Salva impostazioni MP3 Player in EEPROM
void saveMP3PlayerSettings() {
  EEPROM.write(EEPROM_MP3PLAYER_TRACK_ADDR, (uint8_t)mp3Player.currentTrack);
  EEPROM.write(EEPROM_MP3PLAYER_PLAYING_ADDR, mp3Player.playing ? 1 : 0);
  EEPROM.write(EEPROM_MP3PLAYER_VOLUME_ADDR, mp3Player.volume);
  EEPROM.write(EEPROM_MP3PLAYER_PLAYALL_ADDR, mp3Player.playAll ? 1 : 0);
  EEPROM.commit();
  Serial.printf("[MP3] Impostazioni salvate: track=%d, playing=%d, volume=%d, playAll=%d\n",
                mp3Player.currentTrack, mp3Player.playing, mp3Player.volume, mp3Player.playAll);
}

// Carica impostazioni MP3 Player da EEPROM
void loadMP3PlayerSettings() {
  uint8_t savedTrack = EEPROM.read(EEPROM_MP3PLAYER_TRACK_ADDR);
  uint8_t savedVolume = EEPROM.read(EEPROM_MP3PLAYER_VOLUME_ADDR);
  uint8_t savedPlayAll = EEPROM.read(EEPROM_MP3PLAYER_PLAYALL_ADDR);

  // Valida volume (0-100)
  if (savedVolume > 100) savedVolume = 70;  // Default
  mp3Player.volume = savedVolume;

  // Valida playAll (0 o 1)
  mp3Player.playAll = (savedPlayAll == 1);

  Serial.printf("[MP3] Impostazioni caricate: track=%d, volume=%d, playAll=%d (riproduzione SPENTA al boot)\n",
                savedTrack, savedVolume, mp3Player.playAll);

  // Applica solo traccia salvata (se valida), ma NON avviare riproduzione
  if (mp3Player.initialized && savedTrack < mp3Player.totalTracks) {
    mp3Player.currentTrack = savedTrack;
    Serial.printf("[MP3] Traccia ripristinata: %d (non avviata)\n", savedTrack);
  }

  // Applica volume all'audio (converte 0-100 a 0-21)
  #ifdef AUDIO
  extern Audio audio;
  audio.setVolume(map(mp3Player.volume, 0, 100, 0, 21));
  #endif
}

// Setup WebServer per MP3 Player
void setup_mp3player_webserver(AsyncWebServer* server) {
  Serial.println("[MP3-WEB] Configurazione endpoints web...");

  // IMPORTANTE: Endpoint specifici PRIMA di quello generico!

  // API stato player
  server->on("/mp3player/status", HTTP_GET, [](AsyncWebServerRequest *request){
    // Se non inizializzato, scansiona prima i file
    if (!mp3Player.initialized && mp3Player.totalTracks == 0) {
      scanMP3Files();
    }
    request->send(200, "application/json", getMP3PlayerStatusJSON());
  });

  // API comandi
  server->on("/mp3player/cmd", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("action")) {
      String action = request->getParam("action")->value();

      // Se non inizializzato, inizializza prima
      if (!mp3Player.initialized) {
        scanMP3Files();
        mp3Player.initialized = true;
      }

      if (action == "play" && request->hasParam("track")) {
        int track = request->getParam("track")->value().toInt();
        playMP3Track(track);
        if (currentMode == MODE_MP3_PLAYER) mp3Player.needsRedraw = true;
      }
      else if (action == "toggle") {
        if (mp3Player.playing && !mp3Player.paused) {
          pauseMP3Track();
        } else if (mp3Player.paused) {
          resumeMP3Track();
        } else {
          playMP3Track(mp3Player.currentTrack);
        }
        if (currentMode == MODE_MP3_PLAYER) mp3Player.needsRedraw = true;
      }
      else if (action == "stop") {
        stopMP3Track();
        if (currentMode == MODE_MP3_PLAYER) mp3Player.needsRedraw = true;
      }
      else if (action == "next") {
        nextMP3Track();
        if (currentMode == MODE_MP3_PLAYER) mp3Player.needsRedraw = true;
      }
      else if (action == "prev") {
        prevMP3Track();
        if (currentMode == MODE_MP3_PLAYER) mp3Player.needsRedraw = true;
      }
      else if (action == "volume" && request->hasParam("value")) {
        int vol = request->getParam("value")->value().toInt();
        vol = constrain(vol, 0, 100);
        mp3Player.volume = vol;
        #ifdef AUDIO
        extern Audio audio;
        audio.setVolume(map(vol, 0, 100, 0, 21));
        #endif
        saveMP3PlayerSettings();
        if (currentMode == MODE_MP3_PLAYER) mp3Player.needsRedraw = true;
        Serial.printf("[MP3-WEB] Volume: %d%%\n", vol);
      }
      else if (action == "setmode" && request->hasParam("playall")) {
        int mode = request->getParam("playall")->value().toInt();
        mp3Player.playAll = (mode == 1);
        saveMP3PlayerSettings();
        if (currentMode == MODE_MP3_PLAYER) mp3Player.needsRedraw = true;
        Serial.printf("[MP3-WEB] Modalita' cambiata: %s\n", mp3Player.playAll ? "TUTTI" : "SINGOLO");
      }
    }
    request->send(200, "application/json", getMP3PlayerStatusJSON());
  });

  // API attiva modalità MP3 sul display
  server->on("/mp3player/activate", HTTP_GET, [](AsyncWebServerRequest *request){
    currentMode = MODE_MP3_PLAYER;
    mp3Player.needsRedraw = true;
    request->send(200, "application/json", "{\"success\":true}");
  });

  // Pagina principale MP3 Player - DEVE essere registrata DOPO gli endpoint specifici!
  server->on("/mp3player", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", MP3_PLAYER_HTML);
  });

  Serial.println("[MP3-WEB] Endpoints configurati su /mp3player");
}

#endif // EFFECT_MP3_PLAYER
