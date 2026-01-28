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

// ================== LAYOUT DISPLAY 480x480 FULL SCREEN ==================
// VU Meter sinistro: x=0, larghezza=55
// VU Meter destro: x=425, larghezza=55
// Area centrale: x=60 a x=420 (360px)

#define VU_LEFT_X       0
#define VU_RIGHT_X      425
#define VU_WIDTH        55
#define VU_HEIGHT       420
#define VU_Y_START      50
#define VU_SEGMENTS     20          // Numero di segmenti per VU meter

#define PREVIEW_X       60
#define PREVIEW_Y       50
#define PREVIEW_W       360
#define PREVIEW_H       130

#define CONTROLS_Y      195
#define BTN_SIZE        65
#define BTN_SPACING     10

#define VOLUME_BAR_Y    280
#define VOLUME_BAR_H    35

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
    mp3Player.playing = false;
    extern bool isPlaying;
    isPlaying = false;
    // nextMP3Track() verra' chiamato da updateMP3Player()
  }
}

// ================== PROTOTIPI FUNZIONI ==================
void initMP3Player();
void updateMP3Player();
void drawMP3PlayerUI();
void drawVUBackground();
void drawVUMeterVertical(int x, int y, int w, int h, uint8_t level, uint8_t peak, bool leftSide);
void drawMP3Controls();
void drawMP3Preview();
void drawMP3VolumeBar();
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
  mp3Player.currentTrack = 0;
  mp3Player.totalTracks = 0;
  mp3Player.tracks = nullptr;
  mp3Player.volume = 15;  // Volume default (0-21)
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
  gfx->fillScreen(MP3_BG_COLOR);

  // Titolo in alto (centrato)
  gfx->setFont(u8g2_font_helvB24_tr);
  gfx->setTextColor(MP3_ACCENT_COLOR);
  gfx->setCursor(140, 38);
  gfx->print("MP3 PLAYER");

  // Pannello preview
  drawMP3Preview();

  // VU meters - disegna solo sfondo (i livelli verranno aggiornati dopo)
  drawVUBackground();

  // Controlli
  drawMP3Controls();

  // Barra volume
  drawMP3VolumeBar();

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
    gfx->setCursor(PREVIEW_X + 80, PREVIEW_Y + 70);
    gfx->print("Nessuna traccia");
    return;
  }

  // Numero traccia
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(MP3_ACCENT_COLOR);
  char trackNum[32];
  snprintf(trackNum, sizeof(trackNum), "Traccia %d / %d", mp3Player.currentTrack + 1, mp3Player.totalTracks);
  gfx->setCursor(PREVIEW_X + 100, PREVIEW_Y + 28);
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
    gfx->setCursor(xPos, PREVIEW_Y + 70);
    gfx->print(title);
  } else {
    // Titolo lungo: mostra con scroll offset
    gfx->setCursor(PREVIEW_X + 20, PREVIEW_Y + 70);
    // Clipping manuale - mostra solo porzione visibile
    String visiblePart = title.substring(mp3Player.scrollOffset);
    if (visiblePart.length() > 26) {
      visiblePart = visiblePart.substring(0, 26);
    }
    gfx->print(visiblePart);
  }

  // Stato riproduzione
  gfx->setFont(u8g2_font_helvB14_tr);
  const char* status;
  uint16_t statusColor;

  if (mp3Player.playing && !mp3Player.paused) {
    status = "RIPRODUZIONE";
    statusColor = MP3_PLAY_COLOR;
  } else if (mp3Player.paused) {
    status = "PAUSA";
    statusColor = MP3_PAUSE_COLOR;
  } else {
    status = "FERMO";
    statusColor = MP3_TEXT_COLOR;
  }

  gfx->setTextColor(statusColor);
  int statusX = PREVIEW_X + (PREVIEW_W - strlen(status) * 10) / 2;
  gfx->setCursor(statusX, PREVIEW_Y + 110);
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

  // Calcola posizioni pulsanti (4 pulsanti: prev, stop, play/pause, next)
  int totalWidth = 4 * BTN_SIZE + 3 * BTN_SPACING;
  int startX = centerX - totalWidth / 2;

  // Pulsante PREV (<<)
  int prevX = startX;
  gfx->fillRoundRect(prevX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_PANEL_COLOR);
  gfx->drawRoundRect(prevX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_ACCENT_COLOR);
  gfx->setTextColor(MP3_TEXT_COLOR);
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setCursor(prevX + 17, btnY + 42);
  gfx->print("<<");

  // Pulsante STOP
  int stopX = startX + BTN_SIZE + BTN_SPACING;
  gfx->fillRoundRect(stopX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_PANEL_COLOR);
  gfx->drawRoundRect(stopX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_STOP_COLOR);
  // Quadrato stop
  gfx->fillRect(stopX + 20, btnY + 20, 25, 25, MP3_STOP_COLOR);

  // Pulsante PLAY/PAUSE
  int playX = startX + 2 * (BTN_SIZE + BTN_SPACING);
  uint16_t playColor = mp3Player.playing && !mp3Player.paused ? MP3_PAUSE_COLOR : MP3_PLAY_COLOR;
  gfx->fillRoundRect(playX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_PANEL_COLOR);
  gfx->drawRoundRect(playX, btnY, BTN_SIZE, BTN_SIZE, 10, playColor);

  if (mp3Player.playing && !mp3Player.paused) {
    // Simbolo pausa ||
    gfx->fillRect(playX + 20, btnY + 17, 9, 32, playColor);
    gfx->fillRect(playX + 36, btnY + 17, 9, 32, playColor);
  } else {
    // Triangolo play
    for (int i = 0; i < 26; i++) {
      gfx->drawFastVLine(playX + 20 + i, btnY + 20 + i/2, 26 - i, playColor);
    }
  }

  // Pulsante NEXT (>>)
  int nextX = startX + 3 * (BTN_SIZE + BTN_SPACING);
  gfx->fillRoundRect(nextX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_PANEL_COLOR);
  gfx->drawRoundRect(nextX, btnY, BTN_SIZE, BTN_SIZE, 10, MP3_ACCENT_COLOR);
  gfx->setTextColor(MP3_TEXT_COLOR);
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setCursor(nextX + 17, btnY + 42);
  gfx->print(">>");
}

// ================== DISEGNA BARRA VOLUME ==================
void drawMP3VolumeBar() {
  int barX = 100;
  int barW = 280;
  int barH = VOLUME_BAR_H;
  int y = VOLUME_BAR_Y;

  // Etichetta VOL
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(MP3_TEXT_COLOR);
  gfx->setCursor(60, y + 24);
  gfx->print("VOL");

  // Sfondo barra
  gfx->fillRoundRect(barX, y, barW, barH, 8, MP3_PANEL_COLOR);
  gfx->drawRoundRect(barX, y, barW, barH, 8, MP3_ACCENT_COLOR);

  // Barra di riempimento (volume va da 0 a 21)
  int fillW = map(mp3Player.volume, 0, 21, 0, barW - 8);
  if (fillW > 0) {
    gfx->fillRoundRect(barX + 4, y + 4, fillW, barH - 8, 5, MP3_PLAY_COLOR);
  }

  // Valore numerico
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(MP3_TEXT_COLOR);
  String volStr = String(mp3Player.volume);
  gfx->setCursor(barX + barW + 10, y + 24);
  gfx->print(volStr);
}

// ================== DISEGNA TASTO USCITA ==================
void drawMP3ExitButton() {
  int btnW = 180;
  int btnX = 240 - btnW / 2;

  gfx->fillRoundRect(btnX, EXIT_BTN_Y, btnW, EXIT_BTN_H, 10, MP3_STOP_COLOR);
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(MP3_TEXT_COLOR);
  gfx->setCursor(btnX + 60, EXIT_BTN_Y + 34);
  gfx->print("ESCI");
}

// ================== GESTIONE TOUCH ==================
bool handleMP3PlayerTouch(int16_t x, int16_t y) {
  if (!mp3Player.initialized) return false;

  // Area tasto ESCI (btnW=180, btnX=150)
  if (y >= EXIT_BTN_Y && y <= EXIT_BTN_Y + EXIT_BTN_H) {
    if (x >= 150 && x <= 330) {
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

  // Area barra volume (touch diretto)
  int volBarX = 100;
  int volBarW = 280;
  if (y >= VOLUME_BAR_Y && y <= VOLUME_BAR_Y + VOLUME_BAR_H) {
    if (x >= volBarX && x <= volBarX + volBarW) {
      // Calcola nuovo volume dalla posizione X
      int newVol = map(x - volBarX, 0, volBarW, 0, 21);
      newVol = constrain(newVol, 0, 21);
      if (newVol != mp3Player.volume) {
        mp3Player.volume = newVol;
        #ifdef AUDIO
        extern Audio audio;
        audio.setVolume(mp3Player.volume);
        #endif
        saveMP3PlayerSettings();
        Serial.printf("[MP3] Volume impostato a %d\n", mp3Player.volume);
        mp3Player.needsRedraw = true;
      }
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
  Serial.printf("[MP3] Riproduzione: %s\n", fullPath.c_str());

  if (!SD.exists(fullPath)) {
    Serial.println("[MP3] ERRORE: File non trovato!");
    return;
  }

  // Usa audio.connecttoFS con SD
  if (!audio.connecttoFS(SD, fullPath.c_str())) {
    Serial.println("[MP3] Errore connessione file!");
    return;
  }

  mp3Player.playing = true;
  mp3Player.paused = false;
  isPlaying = true;
  saveMP3PlayerSettings();
  mp3Player.needsRedraw = true;
  #endif
}

void stopMP3Track() {
  #ifdef AUDIO
  extern Audio audio;
  extern bool webRadioEnabled;
  extern String webRadioUrl;

  audio.stopSong();

  mp3Player.playing = false;
  mp3Player.paused = false;
  isPlaying = false;
  mp3Player.vuLeft = 0;
  mp3Player.vuRight = 0;
  mp3Player.vuPeakLeft = 0;
  mp3Player.vuPeakRight = 0;
  saveMP3PlayerSettings();

  // Riprendi web radio se era attiva
  if (webRadioEnabled) {
    Serial.println("[MP3] Riprendo web radio...");
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
    saveMP3PlayerSettings();
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
    saveMP3PlayerSettings();
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
  // Traccia ultimo modo per forzare ridisegno al rientro
  static int lastActiveMode = -1;

  // Forza ridisegno quando si entra nella modalità da un'altra
  if (lastActiveMode != MODE_MP3_PLAYER) {
    mp3Player.needsRedraw = true;
    mp3Player.initialized = false;  // Re-inizializza
  }
  lastActiveMode = currentMode;

  if (!mp3Player.initialized) {
    initMP3Player();
    return;
  }

  uint32_t now = millis();

  #ifdef AUDIO
  extern Audio audio;

  // ========== GESTIONE RIPRODUZIONE AUDIO ==========
  if (mp3Player.playing && !mp3Player.paused) {
    // audio.loop() e' chiamato da audioTask - qui controlliamo solo stato
    if (!audio.isRunning() && !mp3Player.paused) {
      // Traccia terminata (EOF callback ha gia' aggiornato i flag)
      Serial.println("[MP3] Traccia terminata - prossima");
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
          // Passa alla modalità successiva nel ciclo (non direttamente a FADE)
          handleModeChange();
          return;
        }
        lastTouchPlay = now;
      }
    }

    // Ridisegna UI se necessario
    if (mp3Player.needsRedraw) {
      drawMP3PlayerUI();
      mp3Player.needsRedraw = false;
    }

    // Aggiorna VU meters durante riproduzione
    static uint32_t lastVUUpdatePlay = 0;
    if (now - lastVUUpdatePlay >= 100) {
      updateMP3VUMeters();
      drawVUMeterVertical(VU_LEFT_X, VU_Y_START, VU_WIDTH, VU_HEIGHT,
                          mp3Player.vuLeft, mp3Player.vuPeakLeft, true);
      drawVUMeterVertical(VU_RIGHT_X, VU_Y_START, VU_WIDTH, VU_HEIGHT,
                          mp3Player.vuRight, mp3Player.vuPeakRight, false);
      lastVUUpdatePlay = now;
    }

    return;
  }
  #endif

  // ========== QUANDO NON IN RIPRODUZIONE ==========
  ts.read();
  if (ts.isTouched) {
    static uint32_t lastTouch = 0;
    if (now - lastTouch > 300) {
      int x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 479);
      int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 479);

      if (handleMP3PlayerTouch(x, y)) {
        // Passa alla modalità successiva nel ciclo (non direttamente a FADE)
        handleModeChange();
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

  // Aggiorna VU meters
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
body{font-family:sans-serif;background:#1a1a2e;min-height:100vh;padding:20px;color:#eee}
.c{max-width:600px;margin:0 auto;background:rgba(255,255,255,.1);border-radius:20px;overflow:hidden}
.h{background:linear-gradient(135deg,#2196F3,#1976D2);padding:25px;text-align:center}
.h h1{font-size:1.5em;margin-bottom:5px}
.ct{padding:25px}
.s{background:rgba(0,0,0,.3);border-radius:15px;padding:20px;margin-bottom:20px}
.s h3{margin-bottom:15px;color:#2196F3}
.track{padding:12px;background:rgba(255,255,255,.1);border-radius:8px;margin-bottom:8px;cursor:pointer;display:flex;justify-content:space-between;align-items:center}
.track:hover{background:rgba(33,150,243,.3)}
.track.active{background:rgba(33,150,243,.5);border:1px solid #2196F3}
.track .name{flex:1}
.track .type{font-size:0.8em;opacity:0.7;padding:2px 8px;background:rgba(255,255,255,.1);border-radius:4px}
.controls{display:flex;justify-content:center;gap:15px;margin:20px 0}
.btn{width:60px;height:60px;border-radius:50%;border:none;cursor:pointer;font-size:1.5em;display:flex;align-items:center;justify-content:center;transition:all 0.3s}
.btn-prev,.btn-next{background:rgba(255,255,255,.1);color:#fff}
.btn-prev:hover,.btn-next:hover{background:rgba(255,255,255,.2)}
.btn-play{background:#4CAF50;color:#fff;width:70px;height:70px}
.btn-play.playing{background:#ff9800}
.btn-stop{background:#f44336;color:#fff}
.status{text-align:center;padding:15px;background:rgba(0,0,0,.2);border-radius:10px;margin-bottom:20px}
.status .now{font-size:1.2em;font-weight:bold;margin-bottom:5px}
.status .state{opacity:0.7}
.vu{display:flex;justify-content:center;gap:3px;height:40px;align-items:flex-end;margin:15px 0}
.vu-bar{width:8px;background:linear-gradient(to top,#4CAF50,#FFEB3B,#f44336);border-radius:2px;transition:height 0.1s}
.hm{display:block;text-align:center;color:#94a3b8;padding:10px;text-decoration:none;font-size:.9em}.hm:hover{color:#fff}
.empty{text-align:center;padding:40px;opacity:0.6}
.mode-btn{width:100%;padding:15px;border:none;border-radius:10px;font-weight:bold;cursor:pointer;font-size:1em;margin-top:10px;background:linear-gradient(135deg,#2196F3,#1976D2);color:#fff}
</style></head><body><div class="c"><a href="/" class="hm">&larr; Home</a><div class="h">
<h1>🎵 MP3 Player</h1><p>Play music from SD card</p></div><div class="ct">
<div class="status">
<div class="now" id="nowPlaying">No track selected</div>
<div class="state" id="playState">Stopped</div>
</div>
<div class="vu" id="vuMeter"></div>
<div class="controls">
<button class="btn btn-prev" onclick="cmd('prev')">⏮</button>
<button class="btn btn-stop" onclick="cmd('stop')">⏹</button>
<button class="btn btn-play" id="playBtn" onclick="cmd('toggle')">▶</button>
<button class="btn btn-next" onclick="cmd('next')">⏭</button>
</div>
<div class="s"><h3>Playlist</h3>
<div id="playlist"></div>
</div>
<button class="mode-btn" onclick="activateMode()">Attiva modalità MP3 Player</button>
</div></div><script>
var isPlaying=false,currentTrack=-1;
function cmd(c,idx){
  var url='/mp3player/cmd?action='+c;
  if(typeof idx!=='undefined')url+='&track='+idx;
  fetch(url).then(r=>r.json()).then(d=>{update(d)});
}
function update(d){
  isPlaying=d.playing;currentTrack=d.current;
  document.getElementById('playBtn').innerHTML=isPlaying?'⏸':'▶';
  document.getElementById('playBtn').className='btn btn-play'+(isPlaying?' playing':'');
  document.getElementById('playState').textContent=isPlaying?(d.paused?'Paused':'Playing'):'Stopped';
  if(d.tracks&&d.tracks.length>0){
    document.getElementById('nowPlaying').textContent=currentTrack>=0?d.tracks[currentTrack].title:'Select a track';
    var html='';
    for(var i=0;i<d.tracks.length;i++){
      html+='<div class="track'+(i==currentTrack?' active':'')+'" onclick="cmd(\'play\','+i+')">';
      html+='<span class="name">'+(i+1)+'. '+d.tracks[i].title+'</span>';
      html+='<span class="type">'+d.tracks[i].type+'</span></div>';
    }
    document.getElementById('playlist').innerHTML=html;
  }else{
    document.getElementById('playlist').innerHTML='<div class="empty">No MP3/WAV files found in /MP3 folder</div>';
  }
  updateVU(d.vuL||0,d.vuR||0);
}
function updateVU(l,r){
  var vu=document.getElementById('vuMeter');
  var html='';
  for(var i=0;i<10;i++){
    var h1=Math.min(40,l*40/100*(i<l/10?1:0.3));
    var h2=Math.min(40,r*40/100*(i<r/10?1:0.3));
    html+='<div class="vu-bar" style="height:'+Math.max(4,l>i*10?h1:4)+'px"></div>';
  }
  for(var i=9;i>=0;i--){
    var h2=Math.min(40,r*40/100*(i<r/10?1:0.3));
    html+='<div class="vu-bar" style="height:'+Math.max(4,r>i*10?h2:4)+'px"></div>';
  }
  vu.innerHTML=html;
}
function activateMode(){
  fetch('/mp3player/activate').then(()=>{alert('MP3 Player mode activated on display!')});
}
function poll(){fetch('/mp3player/status').then(r=>r.json()).then(d=>{update(d)}).catch(()=>{});}
poll();setInterval(poll,1000);
</script></body></html>
)rawliteral";

// Funzione per ottenere lo stato JSON del player
String getMP3PlayerStatusJSON() {
  String json = "{";
  json += "\"playing\":" + String(mp3Player.playing ? "true" : "false") + ",";
  json += "\"paused\":" + String(mp3Player.paused ? "true" : "false") + ",";
  json += "\"current\":" + String(mp3Player.currentTrack) + ",";
  json += "\"total\":" + String(mp3Player.totalTracks) + ",";
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
  EEPROM.commit();
  Serial.printf("[MP3] Impostazioni salvate: track=%d, playing=%d, volume=%d\n",
                mp3Player.currentTrack, mp3Player.playing, mp3Player.volume);
}

// Carica impostazioni MP3 Player da EEPROM
void loadMP3PlayerSettings() {
  uint8_t savedTrack = EEPROM.read(EEPROM_MP3PLAYER_TRACK_ADDR);
  uint8_t savedVolume = EEPROM.read(EEPROM_MP3PLAYER_VOLUME_ADDR);

  // Valida volume (0-21)
  if (savedVolume > 21) savedVolume = 15;  // Default
  mp3Player.volume = savedVolume;

  Serial.printf("[MP3] Impostazioni caricate: track=%d, volume=%d (riproduzione SPENTA al boot)\n",
                savedTrack, savedVolume);

  // Applica solo traccia salvata (se valida), ma NON avviare riproduzione
  if (mp3Player.initialized && savedTrack < mp3Player.totalTracks) {
    mp3Player.currentTrack = savedTrack;
    Serial.printf("[MP3] Traccia ripristinata: %d (non avviata)\n", savedTrack);
  }

  // Applica volume all'audio
  #ifdef AUDIO
  extern Audio audio;
  audio.setVolume(mp3Player.volume);
  #endif
}

// Setup WebServer per MP3 Player
void setup_mp3player_webserver(AsyncWebServer* server) {
  Serial.println("[MP3-WEB] Configurazione endpoints web...");

  // Pagina principale MP3 Player
  server->on("/mp3player", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", MP3_PLAYER_HTML);
  });

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
      }
      else if (action == "toggle") {
        if (mp3Player.playing && !mp3Player.paused) {
          pauseMP3Track();
        } else if (mp3Player.paused) {
          resumeMP3Track();
        } else {
          playMP3Track(mp3Player.currentTrack);
        }
      }
      else if (action == "stop") {
        stopMP3Track();
      }
      else if (action == "next") {
        nextMP3Track();
      }
      else if (action == "prev") {
        prevMP3Track();
      }
    }
    request->send(200, "application/json", getMP3PlayerStatusJSON());
  });

  // API attiva modalità MP3 sul display
  server->on("/mp3player/activate", HTTP_GET, [](AsyncWebServerRequest *request){
    currentMode = MODE_MP3_PLAYER;
    mp3Player.needsRedraw = true;
    forceDisplayUpdate();
    request->send(200, "application/json", "{\"success\":true}");
  });

  Serial.println("[MP3-WEB] Endpoints configurati su /mp3player");
}

#endif // EFFECT_MP3_PLAYER
