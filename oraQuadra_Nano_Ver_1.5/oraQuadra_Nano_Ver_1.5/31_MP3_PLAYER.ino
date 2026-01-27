// ================== LETTORE MP3/WAV DA SD CARD ==================
// Modalità lettore musicale con VU meter verticali e controlli touch
// Legge file MP3 e WAV dalla cartella /MP3 sulla SD card
// WAV è preferito perché non richiede decodifica CPU

#ifdef EFFECT_MP3_PLAYER

#include "AudioGeneratorWAV.h"      // Generatore per file WAV
#include "AudioFileSourceFS.h"      // Sorgente audio da filesystem (SD)

// ================== CONFIGURAZIONE MP3 PLAYER ==================
#define MP3_FOLDER "/MP3"           // Cartella sulla SD dove cercare i file
#define MP3_MAX_FILES 100           // Numero massimo di file MP3 gestibili
#define MP3_TITLE_MAX_LEN 40        // Lunghezza massima del titolo visualizzato
#define MP3_SCROLL_SPEED 150        // Velocità scroll titolo lungo (ms)

// ================== LAYOUT DISPLAY 480x480 ==================
// VU Meter sinistro: x=10, larghezza=40, altezza=380
// VU Meter destro: x=430, larghezza=40, altezza=380
// Area centrale controlli: x=60 a x=420 (360px)
// Preview brano: y=50 a y=200
// Controlli: y=250 a y=380
// Tasto uscita: y=420 a y=470

#define VU_LEFT_X       10
#define VU_RIGHT_X      430
#define VU_WIDTH        40
#define VU_HEIGHT       300
#define VU_Y_START      90
#define VU_SEGMENTS     20          // Numero di segmenti per VU meter

#define PREVIEW_X       70
#define PREVIEW_Y       50
#define PREVIEW_W       340
#define PREVIEW_H       150

#define CONTROLS_Y      260
#define BTN_SIZE        70
#define BTN_SPACING     20

#define EXIT_BTN_Y      420
#define EXIT_BTN_H      50

// ================== COLORI ==================
#define MP3_BG_COLOR      0x0000    // Nero
#define MP3_PANEL_COLOR   0x18E3    // Grigio scuro
#define MP3_ACCENT_COLOR  0x07FF    // Ciano
#define MP3_TEXT_COLOR    0xFFFF    // Bianco
#define MP3_PLAY_COLOR    0x07E0    // Verde
#define MP3_PAUSE_COLOR   0xFD20    // Arancione
#define MP3_STOP_COLOR    0xF800    // Rosso
#define MP3_VU_LOW        0x07E0    // Verde
#define MP3_VU_MID        0xFFE0    // Giallo
#define MP3_VU_HIGH       0xF800    // Rosso
#define MP3_VU_BG         0x2104    // Grigio scuro

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
  int currentTrack;                 // Indice traccia corrente
  int totalTracks;                  // Numero totale tracce
  MP3File* tracks;                  // Array dinamico delle tracce
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
AudioFileSourceFS* mp3FileSD = nullptr;  // Sorgente audio da SD (usa FS generico)
AudioGeneratorWAV* wav = nullptr;        // Generatore WAV (piu' leggero di MP3)


// Variabili per VU meter (evita flickering)
static int8_t prevActiveLeft = -1;
static int8_t prevActiveRight = -1;
static int8_t prevPeakLeft = -1;
static int8_t prevPeakRight = -1;
static bool vuBackgroundDrawn = false;

// ================== TASK AUDIO (opzionale) ==================
#define USE_AUDIO_TASK 0  // 0 = disabilitato, 1 = abilitato

#if USE_AUDIO_TASK
TaskHandle_t mp3TaskHandle = nullptr;
volatile bool mp3TaskRunning = false;
volatile bool mp3RequestStop = false;
volatile bool mp3TrackEnded = false;
SemaphoreHandle_t mp3Mutex = nullptr;
#endif

// ================== PROTOTIPI FUNZIONI ==================
void initMP3Player();
void updateMP3Player();
void drawMP3PlayerUI();
void drawVUBackground();
void drawVUMeterVertical(int x, int y, int w, int h, uint8_t level, uint8_t peak, bool leftSide);
void drawMP3Controls();
void drawMP3Preview();
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
#if USE_AUDIO_TASK
void mp3AudioTask(void* parameter);
void startMP3Task();
void stopMP3Task();
#endif

// ================== INIZIALIZZAZIONE ==================
void initMP3Player() {
  Serial.println("[MP3] Inizializzazione lettore MP3...");

  // Reset stato
  mp3Player.initialized = false;
  mp3Player.playing = false;
  mp3Player.paused = false;
  mp3Player.currentTrack = 0;
  mp3Player.totalTracks = 0;
  mp3Player.tracks = nullptr;
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
  gfx->fillScreen(MP3_BG_COLOR);

  // Titolo in alto
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(MP3_ACCENT_COLOR);
  gfx->setCursor(160, 30);
  gfx->print("MP3 PLAYER");

  // Pannello preview
  drawMP3Preview();

  // VU meters - disegna solo sfondo (i livelli verranno aggiornati dopo)
  drawVUBackground();

  // Controlli
  drawMP3Controls();

  // Tasto uscita
  drawMP3ExitButton();

  mp3Player.needsRedraw = false;
}

// ================== DISEGNA PREVIEW BRANO ==================
void drawMP3Preview() {
  // Sfondo pannello
  gfx->fillRoundRect(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, 10, MP3_PANEL_COLOR);
  gfx->drawRoundRect(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, 10, MP3_ACCENT_COLOR);

  if (mp3Player.totalTracks == 0) {
    gfx->setFont(u8g2_font_helvB14_tr);
    gfx->setTextColor(MP3_TEXT_COLOR);
    gfx->setCursor(PREVIEW_X + 80, PREVIEW_Y + 80);
    gfx->print("Nessuna traccia");
    return;
  }

  // Numero traccia
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(MP3_ACCENT_COLOR);
  char trackNum[32];
  snprintf(trackNum, sizeof(trackNum), "Traccia %d / %d", mp3Player.currentTrack + 1, mp3Player.totalTracks);
  gfx->setCursor(PREVIEW_X + 100, PREVIEW_Y + 30);
  gfx->print(trackNum);

  // Titolo brano (con scroll se troppo lungo)
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(MP3_TEXT_COLOR);

  String title = String(mp3Player.tracks[mp3Player.currentTrack].title);

  // Calcola larghezza testo approssimativa
  int textWidth = title.length() * 11;  // ~11px per carattere con questo font
  int maxWidth = PREVIEW_W - 40;

  if (textWidth <= maxWidth) {
    // Titolo corto: centra
    int xPos = PREVIEW_X + (PREVIEW_W - textWidth) / 2;
    gfx->setCursor(xPos, PREVIEW_Y + 80);
    gfx->print(title);
  } else {
    // Titolo lungo: mostra con scroll offset
    gfx->setCursor(PREVIEW_X + 20, PREVIEW_Y + 80);
    // Clipping manuale - mostra solo porzione visibile
    String visiblePart = title.substring(mp3Player.scrollOffset);
    if (visiblePart.length() > 25) {
      visiblePart = visiblePart.substring(0, 25);
    }
    gfx->print(visiblePart);
  }

  // Stato riproduzione
  gfx->setFont(u8g2_font_helvB14_tr);
  const char* status;
  uint16_t statusColor;

  if (mp3Player.playing && !mp3Player.paused) {
    status = "IN RIPRODUZIONE";
    statusColor = MP3_PLAY_COLOR;
  } else if (mp3Player.paused) {
    status = "IN PAUSA";
    statusColor = MP3_PAUSE_COLOR;
  } else {
    status = "FERMO";
    statusColor = MP3_TEXT_COLOR;
  }

  gfx->setTextColor(statusColor);
  int statusX = PREVIEW_X + (PREVIEW_W - strlen(status) * 9) / 2;
  gfx->setCursor(statusX, PREVIEW_Y + 130);
  gfx->print(status);
}

// ================== DISEGNA VU METER VERTICALE ==================
// Disegna solo lo sfondo dei VU (chiamare una volta)
void drawVUBackground() {
  // VU sinistro
  gfx->fillRoundRect(VU_LEFT_X, VU_Y_START, VU_WIDTH, VU_HEIGHT, 5, MP3_PANEL_COLOR);
  gfx->drawRoundRect(VU_LEFT_X, VU_Y_START, VU_WIDTH, VU_HEIGHT, 5, MP3_ACCENT_COLOR);

  // VU destro
  gfx->fillRoundRect(VU_RIGHT_X, VU_Y_START, VU_WIDTH, VU_HEIGHT, 5, MP3_PANEL_COLOR);
  gfx->drawRoundRect(VU_RIGHT_X, VU_Y_START, VU_WIDTH, VU_HEIGHT, 5, MP3_ACCENT_COLOR);

  // Disegna tutti i segmenti spenti
  int segmentH = (VU_HEIGHT - 20) / VU_SEGMENTS;
  int segmentW = VU_WIDTH - 10;

  for (int i = 0; i < VU_SEGMENTS; i++) {
    int segY = VU_Y_START + VU_HEIGHT - 10 - (i + 1) * segmentH;
    gfx->fillRoundRect(VU_LEFT_X + 5, segY, segmentW, segmentH - 2, 2, MP3_VU_BG);
    gfx->fillRoundRect(VU_RIGHT_X + 5, segY, segmentW, segmentH - 2, 2, MP3_VU_BG);
  }

  vuBackgroundDrawn = true;
  prevActiveLeft = 0;
  prevActiveRight = 0;
  prevPeakLeft = 0;
  prevPeakRight = 0;
}

void drawVUMeterVertical(int x, int y, int w, int h, uint8_t level, uint8_t peak, bool leftSide) {
  // Calcola dimensioni segmenti
  int segmentH = (h - 20) / VU_SEGMENTS;
  int segmentW = w - 10;
  int segmentX = x + 5;
  int startY = y + h - 10;  // Parte dal basso

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

  for (int i = minSeg; i <= maxSeg && i < VU_SEGMENTS; i++) {
    int segY = startY - (i + 1) * segmentH;
    uint16_t color;

    // Determina colore in base alla posizione
    if (i < VU_SEGMENTS * 0.6) {
      color = MP3_VU_LOW;    // Verde (60% inferiore)
    } else if (i < VU_SEGMENTS * 0.85) {
      color = MP3_VU_MID;    // Giallo (25% centrale)
    } else {
      color = MP3_VU_HIGH;   // Rosso (15% superiore)
    }

    if (i < activeSegments) {
      // Segmento attivo
      gfx->fillRoundRect(segmentX, segY, segmentW, segmentH - 2, 2, color);
    } else {
      // Segmento spento
      gfx->fillRoundRect(segmentX, segY, segmentW, segmentH - 2, 2, MP3_VU_BG);
    }
  }

  // Gestione picco separata (solo se cambiato)
  if (peakSegment != prevPeak && peakSegment > 0 && peakSegment < VU_SEGMENTS) {
    // Cancella vecchio picco
    if (prevPeak > 0 && prevPeak < VU_SEGMENTS) {
      int oldPeakY = startY - (prevPeak + 1) * segmentH;
      // Ridisegna segmento senza bordo picco
      uint16_t oldColor = (prevPeak < activeSegments) ?
        ((prevPeak < VU_SEGMENTS * 0.6) ? MP3_VU_LOW :
         (prevPeak < VU_SEGMENTS * 0.85) ? MP3_VU_MID : MP3_VU_HIGH) : MP3_VU_BG;
      gfx->fillRoundRect(segmentX, oldPeakY, segmentW, segmentH - 2, 2, oldColor);
    }
    // Disegna nuovo picco
    int newPeakY = startY - (peakSegment + 1) * segmentH;
    gfx->drawRect(segmentX, newPeakY, segmentW, segmentH - 2, 0xFFFF);
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

// ================== DISEGNA CONTROLLI ==================
void drawMP3Controls() {
  int centerX = 240;
  int btnY = CONTROLS_Y;

  // Calcola posizioni pulsanti (5 pulsanti: prev, stop, play/pause, next + spazio)
  int totalWidth = 4 * BTN_SIZE + 3 * BTN_SPACING;
  int startX = centerX - totalWidth / 2;

  // Pulsante PREV (<<)
  int prevX = startX;
  gfx->fillRoundRect(prevX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_PANEL_COLOR);
  gfx->drawRoundRect(prevX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_ACCENT_COLOR);
  // Disegna simbolo <<
  gfx->setTextColor(MP3_TEXT_COLOR);
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setCursor(prevX + 15, btnY + 45);
  gfx->print("<<");

  // Pulsante STOP
  int stopX = startX + BTN_SIZE + BTN_SPACING;
  gfx->fillRoundRect(stopX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_PANEL_COLOR);
  gfx->drawRoundRect(stopX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_STOP_COLOR);
  // Quadrato stop
  gfx->fillRect(stopX + 20, btnY + 20, 30, 30, MP3_STOP_COLOR);

  // Pulsante PLAY/PAUSE
  int playX = startX + 2 * (BTN_SIZE + BTN_SPACING);
  uint16_t playColor = mp3Player.playing && !mp3Player.paused ? MP3_PAUSE_COLOR : MP3_PLAY_COLOR;
  gfx->fillRoundRect(playX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_PANEL_COLOR);
  gfx->drawRoundRect(playX, btnY, BTN_SIZE, BTN_SIZE, 10, playColor);

  if (mp3Player.playing && !mp3Player.paused) {
    // Simbolo pausa ||
    gfx->fillRect(playX + 20, btnY + 18, 10, 34, playColor);
    gfx->fillRect(playX + 40, btnY + 18, 10, 34, playColor);
  } else {
    // Triangolo play
    for (int i = 0; i < 30; i++) {
      gfx->drawFastVLine(playX + 20 + i, btnY + 20 + i/2, 30 - i, playColor);
    }
  }

  // Pulsante NEXT (>>)
  int nextX = startX + 3 * (BTN_SIZE + BTN_SPACING);
  gfx->fillRoundRect(nextX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_PANEL_COLOR);
  gfx->drawRoundRect(nextX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_ACCENT_COLOR);
  gfx->setTextColor(MP3_TEXT_COLOR);
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setCursor(nextX + 15, btnY + 45);
  gfx->print(">>");

  // Barra progresso sotto i controlli
  int progressY = btnY + BTN_SIZE + 30;
  int progressW = 300;
  int progressH = 10;
  int progressX = centerX - progressW / 2;

  gfx->fillRoundRect(progressX, progressY, progressW, progressH, 3, MP3_VU_BG);
  gfx->drawRoundRect(progressX, progressY, progressW, progressH, 3, MP3_ACCENT_COLOR);

  // TODO: Aggiungi progresso reale quando disponibile dal decoder
  // Per ora mostra una barra statica
  if (mp3Player.playing) {
    static int fakeProgress = 0;
    fakeProgress = (fakeProgress + 1) % 100;
    int progressFill = (progressW - 4) * fakeProgress / 100;
    gfx->fillRoundRect(progressX + 2, progressY + 2, progressFill, progressH - 4, 2, MP3_ACCENT_COLOR);
  }
}

// ================== DISEGNA TASTO USCITA ==================
void drawMP3ExitButton() {
  int btnW = 200;
  int btnX = 240 - btnW / 2;

  gfx->fillRoundRect(btnX, EXIT_BTN_Y, btnW, EXIT_BTN_H, 8, MP3_STOP_COLOR);
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(MP3_TEXT_COLOR);
  gfx->setCursor(btnX + 60, EXIT_BTN_Y + 32);
  gfx->print("ESCI");
}

// ================== GESTIONE TOUCH ==================
bool handleMP3PlayerTouch(int16_t x, int16_t y) {
  if (!mp3Player.initialized) return false;

  // Area tasto ESCI
  if (y >= EXIT_BTN_Y && y <= EXIT_BTN_Y + EXIT_BTN_H) {
    if (x >= 140 && x <= 340) {
      Serial.println("[MP3] Tasto ESCI premuto");
      stopMP3Track();
      cleanupMP3Player();
      return true;  // Segnala di uscire dalla modalita'
    }
  }

  // Area controlli
  if (y >= CONTROLS_Y && y <= CONTROLS_Y + BTN_SIZE) {
    int centerX = 240;
    int totalWidth = 4 * BTN_SIZE + 3 * BTN_SPACING;
    int startX = centerX - totalWidth / 2;

    // PREV
    if (x >= startX && x <= startX + BTN_SIZE) {
      Serial.println("[MP3] Tasto PREV premuto");
      prevMP3Track();
      playTouchSound();
      return false;
    }

    // STOP
    int stopX = startX + BTN_SIZE + BTN_SPACING;
    if (x >= stopX && x <= stopX + BTN_SIZE) {
      Serial.println("[MP3] Tasto STOP premuto");
      stopMP3Track();
      playTouchSound();
      mp3Player.needsRedraw = true;
      return false;
    }

    // PLAY/PAUSE
    int playX = startX + 2 * (BTN_SIZE + BTN_SPACING);
    if (x >= playX && x <= playX + BTN_SIZE) {
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
      playTouchSound();
      mp3Player.needsRedraw = true;
      return false;
    }

    // NEXT
    int nextX = startX + 3 * (BTN_SIZE + BTN_SPACING);
    if (x >= nextX && x <= nextX + BTN_SIZE) {
      Serial.println("[MP3] Tasto NEXT premuto");
      nextMP3Track();
      playTouchSound();
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
    playTouchSound();
    mp3Player.needsRedraw = true;
    return false;
  }

  return false;
}

#if USE_AUDIO_TASK
// ================== FREERTOS AUDIO TASK (CORE 0) ==================
// Task dedicato all'audio che gira su Core 0
// Core 1 e' usato dal display, separando si evitano conflitti
void mp3AudioTask(void* parameter) {
  Serial.println("[MP3] Task audio avviato su Core 0");

  while (mp3TaskRunning) {
    // Controlla se c'e' una richiesta di stop
    if (mp3RequestStop) {
      mp3RequestStop = false;
      vTaskDelay(10);
      continue;
    }

    // Controlla se in riproduzione
    if (mp3Player.playing && !mp3Player.paused && mp3 != nullptr) {
      if (mp3->isRunning()) {
        // Chiama loop per alimentare il buffer I2S
        bool ok = mp3->loop();
        if (!ok) {
          // Traccia terminata
          mp3TrackEnded = true;
          vTaskDelay(10);
        }
      } else {
        vTaskDelay(5);
      }
    } else {
      // Non in riproduzione - attendi piu' a lungo
      vTaskDelay(10);
    }

    // Yield per permettere ad altri task di girare
    taskYIELD();
  }

  Serial.println("[MP3] Task audio terminato");
  vTaskDelete(NULL);
}

void startMP3Task() {
  if (mp3TaskHandle != nullptr) {
    return;  // Task gia' in esecuzione
  }

  // Crea mutex se non esiste
  if (mp3Mutex == nullptr) {
    mp3Mutex = xSemaphoreCreateMutex();
  }

  mp3TaskRunning = true;
  mp3TrackEnded = false;
  mp3RequestStop = false;

  // Crea task su Core 0 con priorita' alta
  // Core 0: Audio
  // Core 1: Display/loop principale
  xTaskCreatePinnedToCore(
    mp3AudioTask,      // Funzione task
    "MP3Audio",        // Nome
    16384,             // Stack size (16KB - MP3 decoder usa molta memoria)
    NULL,              // Parametri
    5,                 // Priorita' alta ma non massima (evita watchdog)
    &mp3TaskHandle,    // Handle
    0                  // Core 0
  );

  Serial.println("[MP3] Task audio creato su Core 0");
}

void stopMP3Task() {
  if (mp3TaskHandle == nullptr) {
    return;
  }

  mp3TaskRunning = false;

  // Attendi che il task termini
  delay(100);

  mp3TaskHandle = nullptr;
  Serial.println("[MP3] Task audio fermato");
}
#endif // USE_AUDIO_TASK

// ================== RIPRODUZIONE ==================
void playMP3Track(int index) {
  #ifdef AUDIO
  if (index < 0 || index >= mp3Player.totalTracks) {
    Serial.println("[MP3] Indice traccia non valido");
    return;
  }

  // Ferma eventuale riproduzione in corso
  stopMP3Track();

  mp3Player.currentTrack = index;

  // Costruisci percorso completo
  String fullPath = String(MP3_FOLDER) + "/" + String(mp3Player.tracks[index].filename);
  Serial.printf("[MP3] Riproduzione: %s\n", fullPath.c_str());

  // Verifica esistenza file
  if (!SD.exists(fullPath)) {
    Serial.println("[MP3] ERRORE: File non trovato!");
    return;
  }

  // Pulisci risorse precedenti
  if (mp3FileSD != nullptr) {
    delete mp3FileSD;
    mp3FileSD = nullptr;
  }
  if (mp3 != nullptr) {
    mp3->stop();
    delete mp3;
    mp3 = nullptr;
  }
  if (buff != nullptr) {
    delete buff;
    buff = nullptr;
  }

  // Pulisce output precedente e ricrea (come fa TTS)
  if (output != nullptr) {
    output->stop();
    delete output;
    output = nullptr;
  }

  // Crea output audio
  output = new AudioOutputI2S();
  output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  output->SetGain(VOLUME_LEVEL);
  output->SetChannels(1);  // MONO
  delay(50);

  // Apri file da SD
  mp3FileSD = new AudioFileSourceFS(SD, fullPath.c_str());
  if (!mp3FileSD->isOpen()) {
    Serial.println("[AUDIO] ERRORE: Impossibile aprire file!");
    delete mp3FileSD;
    mp3FileSD = nullptr;
    return;
  }

  // Determina tipo file
  AudioFileType fileType = mp3Player.tracks[index].type;
  bool started = false;

  if (fileType == AUDIO_TYPE_WAV) {
    // ========== WAV: Semplice, da SD con buffer ==========
    Serial.println("[AUDIO] Usando decoder WAV");

    // Buffer per WAV
    buff = new AudioFileSourceBuffer(mp3FileSD, 32768);

    // Crea decoder WAV
    wav = new AudioGeneratorWAV();

    // Avvia riproduzione WAV
    if (wav->begin(buff, output)) {
      started = true;
      Serial.println("[AUDIO] Riproduzione WAV avviata!");
    }
  } else {
    // ========== MP3: Richiede decodifica CPU ==========
    Serial.println("[AUDIO] Usando decoder MP3 (pesante)");

    // Buffer grande per MP3
    buff = new AudioFileSourceBuffer(mp3FileSD, 65536);

    // Crea decoder MP3
    mp3 = new AudioGeneratorMP3();

    // Avvia riproduzione MP3
    if (mp3->begin(buff, output)) {
      started = true;
      Serial.println("[AUDIO] Riproduzione MP3 avviata!");
    }
  }

  if (started) {
    mp3Player.playing = true;
    mp3Player.paused = false;
    isPlaying = true;

    #if USE_AUDIO_TASK
    mp3TrackEnded = false;
    startMP3Task();
    #endif
  } else {
    Serial.println("[AUDIO] ERRORE: Impossibile avviare riproduzione!");
    stopMP3Track();
  }

  mp3Player.needsRedraw = true;
  #endif
}

void stopMP3Track() {
  #ifdef AUDIO
  #if USE_AUDIO_TASK
  // Prima ferma il task audio
  mp3RequestStop = true;
  stopMP3Task();
  #endif

  // Pulisci decoder MP3
  if (mp3 != nullptr) {
    mp3->stop();
    delete mp3;
    mp3 = nullptr;
  }

  // Pulisci decoder WAV
  if (wav != nullptr) {
    wav->stop();
    delete wav;
    wav = nullptr;
  }

  // Pulisci buffer e sorgente SD
  if (buff != nullptr) {
    delete buff;
    buff = nullptr;
  }
  if (mp3FileSD != nullptr) {
    delete mp3FileSD;
    mp3FileSD = nullptr;
  }

  mp3Player.playing = false;
  mp3Player.paused = false;
  isPlaying = false;
  mp3Player.vuLeft = 0;
  mp3Player.vuRight = 0;
  mp3Player.vuPeakLeft = 0;
  mp3Player.vuPeakRight = 0;

  Serial.println("[MP3] Riproduzione fermata");
  #endif
}

void pauseMP3Track() {
  #ifdef AUDIO
  if (mp3Player.playing && !mp3Player.paused) {
    mp3Player.paused = true;
    // Il decoder MP3 non ha un metodo pause nativo,
    // quindi smettiamo di chiamare loop() nel ciclo principale
    Serial.println("[MP3] Riproduzione in pausa");
  }
  #endif
}

void resumeMP3Track() {
  #ifdef AUDIO
  if (mp3Player.paused) {
    mp3Player.paused = false;
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

  if (mp3Player.playing && !mp3Player.paused) {
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
    // Decadimento quando non in riproduzione
    if (mp3Player.vuLeft > 0) mp3Player.vuLeft -= 2;
    if (mp3Player.vuRight > 0) mp3Player.vuRight -= 2;
    if (mp3Player.vuPeakLeft > 0) mp3Player.vuPeakLeft--;
    if (mp3Player.vuPeakRight > 0) mp3Player.vuPeakRight--;
  }
}

// ================== AGGIORNAMENTO PRINCIPALE ==================
// Variabili statiche per throttling display
static uint32_t lastDisplayUpdate = 0;
static uint32_t lastVUUpdate = 0;
#define DISPLAY_UPDATE_INTERVAL 200   // Aggiorna display ogni 200ms
#define VU_UPDATE_INTERVAL 80         // Aggiorna VU ogni 80ms

void updateMP3Player() {
  if (!mp3Player.initialized) {
    initMP3Player();
    return;
  }

  uint32_t now = millis();

  #ifdef AUDIO
  // ========== PRIORITA' ASSOLUTA: AUDIO ==========
  // Durante la riproduzione, l'audio ha la massima priorita'
  if (mp3Player.playing && !mp3Player.paused) {
    bool isRunning = false;
    bool loopOk = false;

    // Controlla quale decoder e' attivo
    if (wav != nullptr) {
      // ===== WAV: Molto leggero, nessuna decodifica =====
      isRunning = wav->isRunning();
      if (isRunning) {
        loopOk = wav->loop();
      }
    } else if (mp3 != nullptr) {
      // ===== MP3: Richiede decodifica =====
      isRunning = mp3->isRunning();
      if (isRunning) {
        loopOk = mp3->loop();
      }
    }

    if (isRunning) {
      if (!loopOk) {
        Serial.println("[AUDIO] Traccia terminata");
        nextMP3Track();
        return;
      }

      // Touch per controlli durante riproduzione
      ts.read();
      if (ts.isTouched) {
        static uint32_t lastTouchPlay = 0;
        if (now - lastTouchPlay > 400) {
          int x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 479);
          int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 479);
          if (handleMP3PlayerTouch(x, y)) {
            currentMode = MODE_FADE;
            forceDisplayUpdate();
            return;
          }
          lastTouchPlay = now;
        }
      }
      return;

    } else {
      // Decoder si e' fermato
      mp3Player.playing = false;
      isPlaying = false;
      mp3Player.needsRedraw = true;
    }
  }
  #endif

  // ========== QUANDO NON IN RIPRODUZIONE ==========
  // Gestione touch normale
  ts.read();
  if (ts.isTouched) {
    static uint32_t lastTouch = 0;
    if (now - lastTouch > 300) {
      int x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 479);
      int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 479);

      if (handleMP3PlayerTouch(x, y)) {
        currentMode = MODE_FADE;
        forceDisplayUpdate();
        return;
      }
      lastTouch = now;
    }
  }

  // Ridisegno completo se necessario
  if (mp3Player.needsRedraw) {
    drawMP3PlayerUI();
    lastDisplayUpdate = now;
    lastVUUpdate = now;
    return;
  }

  // Aggiorna VU meters (solo quando non in riproduzione)
  if (now - lastVUUpdate >= VU_UPDATE_INTERVAL) {
    updateMP3VUMeters();
    drawVUMeterVertical(VU_LEFT_X, VU_Y_START, VU_WIDTH, VU_HEIGHT,
                        mp3Player.vuLeft, mp3Player.vuPeakLeft, true);
    drawVUMeterVertical(VU_RIGHT_X, VU_Y_START, VU_WIDTH, VU_HEIGHT,
                        mp3Player.vuRight, mp3Player.vuPeakRight, false);
    lastVUUpdate = now;
  }

  // Aggiorna display (preview, scroll)
  if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    if (mp3Player.totalTracks > 0) {
      String title = String(mp3Player.tracks[mp3Player.currentTrack].title);
      if (title.length() > 25) {
        mp3Player.scrollOffset++;
        if (mp3Player.scrollOffset > (int)(title.length() - 20)) {
          mp3Player.scrollOffset = 0;
        }
        drawMP3Preview();
      }
    }
    lastDisplayUpdate = now;
  }
}

// ================== CLEANUP ==================
void cleanupMP3Player() {
  // Ferma la riproduzione e il task audio
  stopMP3Track();

  #if USE_AUDIO_TASK
  // Libera il mutex se esiste
  if (mp3Mutex != nullptr) {
    vSemaphoreDelete(mp3Mutex);
    mp3Mutex = nullptr;
  }
  #endif

  if (mp3Player.tracks != nullptr) {
    heap_caps_free(mp3Player.tracks);
    mp3Player.tracks = nullptr;
  }

  mp3Player.initialized = false;
  mp3PlayerInitialized = false;
  mp3Player.totalTracks = 0;

  Serial.println("[MP3] Lettore MP3 chiuso");
}

#endif // EFFECT_MP3_PLAYER
