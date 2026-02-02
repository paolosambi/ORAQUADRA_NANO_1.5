// ================== ESP32-CAM STREAM MODE ==================
// Streaming video MJPEG da ESP32-CAM Freenove (o compatibili)
// Visualizza il feed della camera direttamente sul display 480x480
//
// La ESP32-CAM genera uno stream MJPEG all'URL: http://<IP>:81/stream
// Questo modulo si connette direttamente alla camera senza server intermedio
//
// UTILIZZO:
// 1. Programma la ESP32-CAM con lo sketch CameraWebServer
// 2. Configura l'IP della camera via web (/espcam)
// 3. Seleziona modalita' ESP32-CAM Stream
//
// La modalita' si riconnette automaticamente se lo stream viene interrotto
// Tocca lo schermo per tornare alla modalita' precedente

#ifdef EFFECT_ESP32CAM

// ================== LISTA CAMERE ESP32-CAM ==================
// Struttura per memorizzare nome e URL di ogni camera
struct Esp32Camera {
  char name[33];   // Nome descrittivo (max 32 caratteri)
  char url[101];   // URL stream (max 100 caratteri)
};

#define MAX_ESP32_CAMERAS 10
#define ESP32CAM_CAMERAS_FILE "/espcam_cameras.json"

Esp32Camera esp32Cameras[MAX_ESP32_CAMERAS];
int esp32CameraCount = 0;
int esp32CameraSelected = 0;

// Dichiarazioni forward per funzioni di gestione camere
void loadCamerasFromLittleFS();
void saveCamerasToLittleFS();
bool addEsp32Camera(const char* name, const char* url);
bool removeEsp32Camera(int index);
bool selectEsp32Camera(int index);
void clearAllEsp32Cameras();

// ================== EXTERN PER GESTIONE CONNESSIONI SIMULTANEE ==================
extern bool webRadioEnabled;
extern void stopWebRadio();
extern void startWebRadio();
extern bool radarServerEnabled;

// Stato salvato per ripristino dopo uscita da ESP32-CAM
static bool esp32camSavedWebRadioState = false;
static bool esp32camSavedRadarState = false;
static bool esp32camServicesSuspended = false;

// ================== CONFIGURAZIONE STREAMING ==================
#define ESPCAM_JPEG_BUFFER_SIZE 300000   // Buffer per singolo frame JPEG (300KB) - supporta fino a 1600x1200
#define ESPCAM_RECONNECT_DELAY 2000      // Delay tra tentativi di riconnessione (ms)
#define ESPCAM_FRAME_TIMEOUT 15000       // Timeout per frame singolo (ms) - aumentato per reti lente
#define ESPCAM_CONNECTION_TIMEOUT 10000  // Timeout connessione stream (ms)
#define ESPCAM_READ_BUFFER_SIZE 8192     // Buffer lettura TCP (8KB) - più veloce per frame grandi
#define ESPCAM_DISPLAY_SIZE 480          // Dimensione display (480x480)
#define ESPCAM_FRAME_BUFFER_SIZE (ESPCAM_DISPLAY_SIZE * ESPCAM_DISPLAY_SIZE) // 480x480 pixels

// URL dello stream ESP32-CAM - configurabile via web
String esp32camStreamUrl = "http://192.168.1.100:81/stream";  // Default, modificabile via web

// ================== VARIABILI STATO ESP32-CAM ==================
bool esp32camInitialized = false;          // Flag inizializzazione
bool esp32camStreaming = false;            // Flag streaming attivo
bool esp32camShowOverlay = true;           // Mostra overlay FPS/info (default ON per debug)
bool esp32camFullscreen = true;            // Modalita' fullscreen (scala a 480x480)

// Buffer ESP32-CAM (allocati in PSRAM) - DOUBLE BUFFERING
uint8_t *esp32camJpegBuffer = nullptr;     // Buffer dati JPEG compressi
uint8_t *esp32camReadBuffer = nullptr;     // Buffer lettura TCP veloce
uint16_t *esp32camFrameBuffer = nullptr;   // Frame buffer per double buffering (480x480 RGB565)

// Dimensioni immagine corrente (rilevate dal JPEG)
int esp32camImageWidth = 0;
int esp32camImageHeight = 0;
int esp32camOffsetX = 0;                   // Offset X per centrare
int esp32camOffsetY = 0;                   // Offset Y per centrare
float esp32camScale = 1.0;

// Statistiche
unsigned long esp32camFrameCount = 0;      // Contatore frame totali
unsigned long esp32camLastFpsUpdate = 0;   // Timestamp ultimo calcolo FPS
float esp32camCurrentFps = 0;              // FPS corrente
int esp32camFramesThisSecond = 0;          // Frame nel secondo corrente
unsigned long esp32camLastFrameTime = 0;   // Timestamp ultimo frame ricevuto

// Stato schermata per evitare ridisegno continuo
int esp32camScreenState = -1;              // -1=nessuno, 0=waiting, 1=error, 2=streaming

// Client HTTP per streaming
WiFiClient esp32camWifiClient;
HTTPClient esp32camHttp;

// Decoder JPEG
JPEGDEC esp32camDecoder;

// ================== CALLBACK JPEG CON DOUBLE BUFFER E SCALING ==================
// Scrive i pixel nel frame buffer con scaling per fullscreen
// Usa approccio "source-driven" per evitare gap/righe durante lo scaling
int esp32camDrawCallbackBuffered(JPEGDRAW *pDraw) {
  if (esp32camFrameBuffer == nullptr) return 0;

  int srcX = pDraw->x;
  int srcY = pDraw->y;
  int srcW = pDraw->iWidth;
  int srcH = pDraw->iHeight;

  if (esp32camFullscreen && esp32camScale != 1.0f) {
    // Approccio SOURCE-DRIVEN: per ogni pixel sorgente, riempi i pixel destinazione
    // Questo evita gap sia in upscaling che downscaling

    for (int sy = 0; sy < srcH; sy++) {
      // Calcola range Y destinazione per questo pixel sorgente
      int dstY1 = (int)((srcY + sy) * esp32camScale) + esp32camOffsetY;
      int dstY2 = (int)((srcY + sy + 1) * esp32camScale) + esp32camOffsetY;
      if (dstY2 <= dstY1) dstY2 = dstY1 + 1;  // Almeno 1 pixel

      // Clipping Y
      if (dstY2 <= 0 || dstY1 >= ESPCAM_DISPLAY_SIZE) continue;
      if (dstY1 < 0) dstY1 = 0;
      if (dstY2 > ESPCAM_DISPLAY_SIZE) dstY2 = ESPCAM_DISPLAY_SIZE;

      uint16_t *srcRow = &pDraw->pPixels[sy * srcW];

      for (int sx = 0; sx < srcW; sx++) {
        // Calcola range X destinazione per questo pixel sorgente
        int dstX1 = (int)((srcX + sx) * esp32camScale) + esp32camOffsetX;
        int dstX2 = (int)((srcX + sx + 1) * esp32camScale) + esp32camOffsetX;
        if (dstX2 <= dstX1) dstX2 = dstX1 + 1;  // Almeno 1 pixel

        // Clipping X
        if (dstX2 <= 0 || dstX1 >= ESPCAM_DISPLAY_SIZE) continue;
        if (dstX1 < 0) dstX1 = 0;
        if (dstX2 > ESPCAM_DISPLAY_SIZE) dstX2 = ESPCAM_DISPLAY_SIZE;

        uint16_t pixel = srcRow[sx];

        // Riempi tutti i pixel destinazione coperti da questo pixel sorgente
        for (int dy = dstY1; dy < dstY2; dy++) {
          uint16_t *dstRow = &esp32camFrameBuffer[dy * ESPCAM_DISPLAY_SIZE];
          for (int dx = dstX1; dx < dstX2; dx++) {
            dstRow[dx] = pixel;
          }
        }
      }
    }
  } else {
    // Senza scaling - copia diretta nel buffer con offset
    int dstX = srcX + esp32camOffsetX;
    int dstY = srcY + esp32camOffsetY;

    for (int y = 0; y < srcH; y++) {
      int bufY = dstY + y;
      if (bufY < 0 || bufY >= ESPCAM_DISPLAY_SIZE) continue;

      for (int x = 0; x < srcW; x++) {
        int bufX = dstX + x;
        if (bufX < 0 || bufX >= ESPCAM_DISPLAY_SIZE) continue;

        esp32camFrameBuffer[bufY * ESPCAM_DISPLAY_SIZE + bufX] = pDraw->pPixels[y * srcW + x];
      }
    }
  }

  return 1;
}

// ================== INIZIALIZZAZIONE ==================
bool initEsp32camStream() {
  Serial.println("[ESP32-CAM] Inizializzazione con DOUBLE BUFFERING...");

  // ========== SOSPENDI SERVIZI CHE USANO CONNESSIONI HTTP ==========
  // Questo libera banda WiFi e connessioni per lo streaming video
  if (!esp32camServicesSuspended) {
    // Salva stato Web Radio
    esp32camSavedWebRadioState = webRadioEnabled;
    if (webRadioEnabled) {
      Serial.println("[ESP32-CAM] Sospendo Web Radio per liberare banda...");
      stopWebRadio();
      delay(100);  // Attendi chiusura connessione
    }

    // Salva stato Radar Remote
    esp32camSavedRadarState = radarServerEnabled;
    if (radarServerEnabled) {
      Serial.println("[ESP32-CAM] Sospendo Radar Remote...");
      radarServerEnabled = false;  // Disabilita temporaneamente
    }

    esp32camServicesSuspended = true;
    Serial.println("[ESP32-CAM] Servizi sospesi - connessioni liberate");
  }

  // Alloca buffer JPEG in PSRAM
  if (esp32camJpegBuffer == nullptr) {
    esp32camJpegBuffer = (uint8_t *)heap_caps_malloc(ESPCAM_JPEG_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (esp32camJpegBuffer == nullptr) {
      Serial.println("[ESP32-CAM] ERRORE: Impossibile allocare JPEG buffer!");
      return false;
    }
    Serial.printf("[ESP32-CAM] JPEG buffer allocato: %d KB\n", ESPCAM_JPEG_BUFFER_SIZE / 1024);
  }

  // Alloca buffer lettura TCP in PSRAM
  if (esp32camReadBuffer == nullptr) {
    esp32camReadBuffer = (uint8_t *)heap_caps_malloc(ESPCAM_READ_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (esp32camReadBuffer == nullptr) {
      Serial.println("[ESP32-CAM] ERRORE: Impossibile allocare read buffer!");
      return false;
    }
    Serial.printf("[ESP32-CAM] Read buffer allocato: %d KB\n", ESPCAM_READ_BUFFER_SIZE / 1024);
  }

  // Alloca FRAME BUFFER per double buffering in PSRAM (480x480x2 = 460KB)
  if (esp32camFrameBuffer == nullptr) {
    esp32camFrameBuffer = (uint16_t *)heap_caps_malloc(ESPCAM_FRAME_BUFFER_SIZE * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (esp32camFrameBuffer == nullptr) {
      Serial.println("[ESP32-CAM] ERRORE: Impossibile allocare frame buffer!");
      return false;
    }
    // Inizializza a nero
    memset(esp32camFrameBuffer, 0, ESPCAM_FRAME_BUFFER_SIZE * sizeof(uint16_t));
    Serial.printf("[ESP32-CAM] Frame buffer allocato: %d KB (double buffer)\n",
                  (ESPCAM_FRAME_BUFFER_SIZE * sizeof(uint16_t)) / 1024);
  }

  // Reset statistiche
  esp32camFrameCount = 0;
  esp32camCurrentFps = 0;
  esp32camFramesThisSecond = 0;
  esp32camLastFpsUpdate = millis();
  esp32camLastFrameTime = millis();
  esp32camScreenState = -1;  // Reset stato schermata per forzare ridisegno

  esp32camInitialized = true;
  Serial.println("[ESP32-CAM] Inizializzazione completata - Double Buffer ATTIVO");
  return true;
}

// ================== CONNESSIONE STREAM ==================
bool connectEsp32camStream() {
  // Assicura che l'URL sia pulito (senza spazi iniziali/finali)
  esp32camStreamUrl.trim();

  if (esp32camStreamUrl.length() < 15 || !esp32camStreamUrl.startsWith("http")) {
    Serial.printf("[ESP32-CAM] URL non valido: '%s'\n", esp32camStreamUrl.c_str());
    return false;
  }

  Serial.printf("[ESP32-CAM] Connessione a: '%s'\n", esp32camStreamUrl.c_str());

  // Chiudi eventuali connessioni precedenti per evitare errore "already connected"
  esp32camHttp.end();
  esp32camWifiClient.stop();
  delay(100);  // Piccola pausa per assicurare chiusura completa

  // Configura HTTPClient
  esp32camHttp.begin(esp32camWifiClient, esp32camStreamUrl);
  esp32camHttp.setTimeout(ESPCAM_CONNECTION_TIMEOUT);
  esp32camHttp.setReuse(false);  // Non riusare connessioni - evita errore -2

  // Aggiungi header per raccogliere Content-Type
  const char* headerKeys[] = {"Content-Type"};
  esp32camHttp.collectHeaders(headerKeys, 1);

  int httpCode = esp32camHttp.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[ESP32-CAM] Errore HTTP: %d\n", httpCode);
    esp32camHttp.end();
    return false;
  }

  // Verifica content-type MJPEG
  String contentType = esp32camHttp.header("Content-Type");
  Serial.printf("[ESP32-CAM] Content-Type: '%s'\n", contentType.c_str());

  Serial.println("[ESP32-CAM] Stream connesso!");
  esp32camStreaming = true;
  esp32camLastFrameTime = millis();
  esp32camScreenState = 2;  // In streaming

  return true;
}

// ================== DISCONNESSIONE STREAM ==================
void disconnectEsp32camStream() {
  esp32camHttp.end();
  esp32camWifiClient.stop();
  esp32camStreaming = false;
  Serial.println("[ESP32-CAM] Stream disconnesso");
}

// ================== LETTURA FRAME MJPEG ==================
// Legge un singolo frame JPEG dallo stream MJPEG
int readEsp32camFrame() {
  WiFiClient* stream = esp32camHttp.getStreamPtr();

  if (!stream || !stream->connected()) {
    return -1;  // Connessione persa
  }

  // Attendi dati disponibili con timeout minimo
  int waitCount = 0;
  while (!stream->available() && waitCount < 200) {
    delayMicroseconds(500);
    waitCount++;
  }

  if (!stream->available()) {
    return 0;  // Nessun dato
  }

  int jpegSize = 0;
  bool foundSOI = false;
  uint8_t prevByte = 0;
  unsigned long timeout = millis() + 2000;  // 2 secondi timeout

  while (millis() < timeout && stream->connected()) {
    int available = stream->available();
    if (available > 0) {
      // Leggi a blocchi per massima velocita'
      int toRead = min(available, ESPCAM_READ_BUFFER_SIZE);
      int bytesRead = stream->readBytes(esp32camReadBuffer, toRead);

      for (int i = 0; i < bytesRead; i++) {
        uint8_t b = esp32camReadBuffer[i];

        if (!foundSOI) {
          // Cerca marker SOI (0xFFD8)
          if (prevByte == 0xFF && b == 0xD8) {
            esp32camJpegBuffer[0] = 0xFF;
            esp32camJpegBuffer[1] = 0xD8;
            jpegSize = 2;
            foundSOI = true;
          }
          prevByte = b;
        } else {
          // Copia dati JPEG
          if (jpegSize < ESPCAM_JPEG_BUFFER_SIZE - 1) {
            esp32camJpegBuffer[jpegSize++] = b;

            // Cerca marker EOI (End Of Image): 0xFFD9
            if (esp32camJpegBuffer[jpegSize-2] == 0xFF && esp32camJpegBuffer[jpegSize-1] == 0xD9) {
              return jpegSize;  // Frame completo!
            }
          } else {
            return 0;  // Buffer overflow
          }
        }
      }
    }
  }

  return 0;  // Timeout o frame incompleto
}

// Variabile per scaling hardware del decoder
static int esp32camHwScale = 0;  // 0=none, JPEG_SCALE_HALF, JPEG_SCALE_QUARTER, JPEG_SCALE_EIGHTH

// ================== DECODIFICA E VISUALIZZA FRAME CON DOUBLE BUFFER ==================
bool decodeAndDisplayEsp32camFrame(uint8_t* jpegData, int jpegSize) {
  if (esp32camFrameBuffer == nullptr) return false;

  // Apri il JPEG per ottenere le dimensioni
  if (!esp32camDecoder.openRAM(jpegData, jpegSize, esp32camDrawCallbackBuffered)) {
    return false;
  }

  // Ottieni dimensioni immagine ORIGINALI
  int imgWidth = esp32camDecoder.getWidth();
  int imgHeight = esp32camDecoder.getHeight();

  // Se le dimensioni sono cambiate, ricalcola scaling
  if (imgWidth != esp32camImageWidth || imgHeight != esp32camImageHeight) {
    esp32camImageWidth = imgWidth;
    esp32camImageHeight = imgHeight;

    // ========== CALCOLA SCALING HARDWARE OTTIMALE ==========
    // Strategia: scalare HW fino ad avere un'immagine <= 480 su almeno un lato
    // Poi fare UPSCALING software (che non salta pixel e non crea righe)
    int maxDim = max(imgWidth, imgHeight);

    if (maxDim > 1600) {
      // Per risoluzioni molto alte (es. 1920x1080): scala 1/4
      esp32camHwScale = JPEG_SCALE_QUARTER;
      imgWidth /= 4;
      imgHeight /= 4;
      Serial.printf("[ESP32-CAM] Usando HW scale 1/4: %dx%d -> %dx%d\n",
                    esp32camImageWidth, esp32camImageHeight, imgWidth, imgHeight);
    } else if (maxDim > 960) {
      // Per 1600x1200, 1280x720, 1024x768: scala 1/4 -> risultato ~400x300
      // L'upscaling a 480 è più pulito del downscaling
      esp32camHwScale = JPEG_SCALE_QUARTER;
      imgWidth /= 4;
      imgHeight /= 4;
      Serial.printf("[ESP32-CAM] Usando HW scale 1/4: %dx%d -> %dx%d\n",
                    esp32camImageWidth, esp32camImageHeight, imgWidth, imgHeight);
    } else if (maxDim > 640) {
      // Per 800x600, 640x480: scala 1/2 -> risultato ~400x300
      esp32camHwScale = JPEG_SCALE_HALF;
      imgWidth /= 2;
      imgHeight /= 2;
      Serial.printf("[ESP32-CAM] Usando HW scale 1/2: %dx%d -> %dx%d\n",
                    esp32camImageWidth, esp32camImageHeight, imgWidth, imgHeight);
    } else {
      // Per risoluzioni <= 640: nessuno scaling hardware
      esp32camHwScale = 0;
    }

    if (esp32camFullscreen && (imgWidth != ESPCAM_DISPLAY_SIZE || imgHeight != ESPCAM_DISPLAY_SIZE)) {
      // Calcola scala SOFTWARE per riempire il display (fill, non fit)
      // Ora imgWidth/imgHeight sono già scalati dall'HW
      float scaleX = (float)ESPCAM_DISPLAY_SIZE / imgWidth;
      float scaleY = (float)ESPCAM_DISPLAY_SIZE / imgHeight;
      // Usa la scala maggiore per riempire tutto (crop ai bordi)
      esp32camScale = max(scaleX, scaleY);

      // Calcola offset per centrare
      int scaledW = (int)(imgWidth * esp32camScale);
      int scaledH = (int)(imgHeight * esp32camScale);
      esp32camOffsetX = (ESPCAM_DISPLAY_SIZE - scaledW) / 2;
      esp32camOffsetY = (ESPCAM_DISPLAY_SIZE - scaledH) / 2;

      Serial.printf("[ESP32-CAM] Immagine %dx%d (HW:%d) -> SW scala %.2f, offset (%d,%d)\n",
                    imgWidth, imgHeight, esp32camHwScale, esp32camScale, esp32camOffsetX, esp32camOffsetY);

      // Pulisci il frame buffer quando cambiano le dimensioni
      memset(esp32camFrameBuffer, 0, ESPCAM_FRAME_BUFFER_SIZE * sizeof(uint16_t));
    } else {
      // Nessuno scaling software, centra solo
      esp32camScale = 1.0;
      esp32camOffsetX = (ESPCAM_DISPLAY_SIZE - imgWidth) / 2;
      esp32camOffsetY = (ESPCAM_DISPLAY_SIZE - imgHeight) / 2;
      if (esp32camOffsetX < 0) esp32camOffsetX = 0;
      if (esp32camOffsetY < 0) esp32camOffsetY = 0;

      // Pulisci il frame buffer
      memset(esp32camFrameBuffer, 0, ESPCAM_FRAME_BUFFER_SIZE * sizeof(uint16_t));
    }
  }

  esp32camDecoder.close();

  // Decodifica nel frame buffer CON SCALING HARDWARE
  if (!esp32camDecoder.openRAM(jpegData, jpegSize, esp32camDrawCallbackBuffered)) {
    return false;
  }

  esp32camDecoder.setPixelType(RGB565_LITTLE_ENDIAN);
  esp32camDecoder.decode(0, 0, esp32camHwScale);  // Usa scaling hardware!
  esp32camDecoder.close();

  // ========== DOUBLE BUFFER: Copia il frame buffer sul display in un colpo solo ==========
  // Usa draw16bitRGBBitmap per trasferire tutto il buffer al display
  // Offset Y di 36 pixel per lasciare spazio alla barra info in alto
  // Il buffer contiene 480x480 pixel, ma mostriamo solo dalla riga 0 del buffer
  // partendo dalla Y=36 del display, per 444 righe (480-36)
  gfx->draw16bitRGBBitmap(0, 36, esp32camFrameBuffer, ESPCAM_DISPLAY_SIZE, 444);

  return true;
}

// ================== ESTRAI IP DALLA URL ==================
String extractIpFromUrl(const String& url) {
  // Estrae l'IP da URL tipo "http://192.168.1.100:81/stream"
  int startIdx = url.indexOf("://");
  if (startIdx < 0) startIdx = 0;
  else startIdx += 3;  // Salta "://"

  int endIdx = url.indexOf(":", startIdx);
  if (endIdx < 0) endIdx = url.indexOf("/", startIdx);
  if (endIdx < 0) endIdx = url.length();

  return url.substring(startIdx, endIdx);
}

// ================== OVERLAY INFORMAZIONI ==================
void drawEsp32camOverlay() {
  if (!esp32camShowOverlay) return;

  // Barra superiore - SEMPRE visibile e fissa
  gfx->fillRect(0, 0, 480, 36, BLACK);

  // Usa font U8G2 piccolo
  gfx->setFont(u8g2_font_helvR08_tr);

  // === RIGA 1: Info streaming ===

  // Double Buffer indicator
  gfx->setTextColor(GREEN);
  gfx->setCursor(5, 12);
  gfx->print("DB");

  // FPS
  gfx->setTextColor(GREEN);
  gfx->setCursor(25, 12);
  gfx->printf("%.0f FPS", esp32camCurrentFps);

  // Risoluzione
  gfx->setTextColor(CYAN);
  gfx->setCursor(85, 12);
  gfx->printf("%dx%d", esp32camImageWidth, esp32camImageHeight);

  // Scala
  gfx->setTextColor(YELLOW);
  gfx->setCursor(160, 12);
  if (esp32camFullscreen) {
    gfx->printf("x%.1f", esp32camScale);
  } else {
    gfx->print("1:1");
  }

  // Frame count
  gfx->setTextColor(WHITE);
  gfx->setCursor(210, 12);
  gfx->printf("F:%lu", esp32camFrameCount);

  // WiFi signal
  int rssi = WiFi.RSSI();
  gfx->setCursor(300, 12);
  if (rssi > -50) {
    gfx->setTextColor(GREEN);
    gfx->print("WiFi:+++");
  } else if (rssi > -70) {
    gfx->setTextColor(YELLOW);
    gfx->print("WiFi:++");
  } else {
    gfx->setTextColor(RED);
    gfx->print("WiFi:+");
  }

  // PSRAM libero
  gfx->setTextColor(MAGENTA);
  gfx->setCursor(400, 12);
  gfx->printf("PS:%dK", ESP.getFreePsram() / 1024);

  // === RIGA 2: IP Camera ===
  gfx->setTextColor(CYAN);
  gfx->setCursor(5, 30);
  gfx->print("CAM: ");
  gfx->setTextColor(WHITE);
  String camIp = extractIpFromUrl(esp32camStreamUrl);
  gfx->print(camIp);

  // Stato connessione
  gfx->setCursor(200, 30);
  if (esp32camStreaming) {
    gfx->setTextColor(GREEN);
    gfx->print("STREAMING");
  } else {
    gfx->setTextColor(YELLOW);
    gfx->print("CONNECTING...");
  }

  // Ripristina font default
  gfx->setFont(u8g2_font_inb21_mr);
}

// ================== MOSTRA STATO ATTESA ==================
void showEsp32camWaitingScreen() {
  // Evita ridisegno se gia' in questo stato
  if (esp32camScreenState == 0) return;
  esp32camScreenState = 0;

  gfx->fillScreen(BLACK);

  // Icona camera stilizzata
  gfx->fillRoundRect(180, 80, 120, 80, 10, DARKGREY);
  gfx->fillCircle(240, 120, 25, WHITE);
  gfx->fillCircle(240, 120, 18, BLACK);
  gfx->fillRect(280, 90, 30, 20, DARKGREY);

  // Titolo
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(CYAN);
  gfx->setCursor(140, 200);
  gfx->println("ESP32-CAM STREAM");

  gfx->setFont(u8g2_font_helvR12_tr);
  gfx->setTextColor(WHITE);
  gfx->setCursor(150, 240);
  gfx->println("Connessione in corso...");

  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(LIGHTGREY);
  gfx->setCursor(80, 300);
  gfx->println("Verifica che la camera sia accesa:");

  gfx->setTextColor(GREEN);
  gfx->setCursor(60, 330);
  gfx->println(esp32camStreamUrl.c_str());

  gfx->setTextColor(YELLOW);
  gfx->setCursor(100, 380);
  gfx->println("Configura IP: /espcam");

  // Ripristina font default
  gfx->setFont(u8g2_font_inb21_mr);
}

// ================== MOSTRA ERRORE CONNESSIONE ==================
void showEsp32camConnectionError() {
  // Evita ridisegno se gia' in questo stato
  if (esp32camScreenState == 1) return;
  esp32camScreenState = 1;

  gfx->fillScreen(BLACK);

  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(RED);
  gfx->setCursor(120, 180);
  gfx->println("Camera non raggiungibile");

  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(YELLOW);
  gfx->setCursor(140, 230);
  gfx->println("Tentativo riconnessione...");

  gfx->setTextColor(LIGHTGREY);
  gfx->setCursor(80, 300);
  gfx->println("Verifica:");
  gfx->setCursor(80, 320);
  gfx->println("1. Camera accesa e connessa al WiFi");
  gfx->setCursor(80, 340);
  gfx->println("2. IP corretto configurato");
  gfx->setCursor(80, 360);
  gfx->println("3. Porta 81 accessibile");

  gfx->setTextColor(CYAN);
  gfx->setCursor(120, 420);
  gfx->println("Tocca per cambiare modo");

  // Ripristina font default
  gfx->setFont(u8g2_font_inb21_mr);
}

// ================== UPDATE PRINCIPALE ESP32-CAM ==================
void updateEsp32camStream() {
  // Controlla se c'è un URL da salvare in EEPROM (differito dal web handler)
  esp32camCheckPendingSave();

  static unsigned long lastConnectAttempt = 0;

  // Inizializza se necessario
  if (!esp32camInitialized) {
    if (!initEsp32camStream()) {
      Serial.println("[ESP32-CAM] Inizializzazione fallita!");
      gfx->fillScreen(BLACK);
      gfx->setFont(u8g2_font_helvB12_tr);
      gfx->setTextColor(RED);
      gfx->setCursor(100, 200);
      gfx->println("ERRORE MEMORIA!");
      gfx->setFont(u8g2_font_helvR10_tr);
      gfx->setCursor(80, 250);
      gfx->println("PSRAM non disponibile");
      gfx->setFont(u8g2_font_inb21_mr);
      delay(3000);
      handleModeChange();
      return;
    }
  }

  // Se non connesso allo stream, prova a connettersi
  if (!esp32camStreaming) {
    // Mostra schermata di attesa
    showEsp32camWaitingScreen();

    // Limita tentativi di connessione
    if (millis() - lastConnectAttempt < ESPCAM_RECONNECT_DELAY) {
      return;
    }
    lastConnectAttempt = millis();

    Serial.println("[ESP32-CAM] Tentativo connessione...");
    if (connectEsp32camStream()) {
      Serial.println("[ESP32-CAM] Stream avviato!");
    } else {
      Serial.println("[ESP32-CAM] Connessione fallita, riprovo...");
      showEsp32camConnectionError();
      return;
    }
  }

  // Ricevi e visualizza frame
  int jpegSize = readEsp32camFrame();

  if (jpegSize > 0) {
    if (decodeAndDisplayEsp32camFrame(esp32camJpegBuffer, jpegSize)) {
      esp32camFrameCount++;
      esp32camFramesThisSecond++;
      esp32camLastFrameTime = millis();

      // Disegna overlay sempre sopra il video (per renderlo fisso, non lampeggiante)
      drawEsp32camOverlay();
    }
  } else if (jpegSize < 0) {
    // Connessione persa
    Serial.println("[ESP32-CAM] Connessione persa!");
    esp32camStreaming = false;
    esp32camScreenState = -1;
    esp32camHttp.end();
    showEsp32camWaitingScreen();
    delay(500);
    return;
  }

  // Timeout: nessun frame ricevuto per troppo tempo
  if (millis() - esp32camLastFrameTime > ESPCAM_FRAME_TIMEOUT) {
    Serial.println("[ESP32-CAM] Timeout frame!");
    esp32camStreaming = false;
    esp32camScreenState = -1;
    esp32camHttp.end();
    showEsp32camWaitingScreen();
    return;
  }

  // Calcola FPS ogni secondo
  unsigned long now = millis();
  if (now - esp32camLastFpsUpdate >= 1000) {
    esp32camCurrentFps = esp32camFramesThisSecond;
    esp32camFramesThisSecond = 0;
    esp32camLastFpsUpdate = now;
  }
}

// ================== CLEANUP ESP32-CAM ==================
void cleanupEsp32camStream() {
  Serial.println("[ESP32-CAM] Cleanup...");

  // Disconnetti stream
  disconnectEsp32camStream();

  // Libera frame buffer (mantieni gli altri buffer per riuso)
  if (esp32camFrameBuffer != nullptr) {
    heap_caps_free(esp32camFrameBuffer);
    esp32camFrameBuffer = nullptr;
    Serial.println("[ESP32-CAM] Frame buffer liberato");
  }

  esp32camInitialized = false;
  esp32camStreaming = false;
  esp32camScreenState = -1;
  esp32camImageWidth = 0;
  esp32camImageHeight = 0;

  // ========== RIPRISTINA SERVIZI SOSPESI ==========
  if (esp32camServicesSuspended) {
    // Ripristina Radar Remote
    if (esp32camSavedRadarState) {
      Serial.println("[ESP32-CAM] Ripristino Radar Remote...");
      radarServerEnabled = true;
    }

    // Ripristina Web Radio
    if (esp32camSavedWebRadioState) {
      Serial.println("[ESP32-CAM] Ripristino Web Radio...");
      delay(200);  // Attendi che le connessioni si chiudano
      startWebRadio();
    }

    esp32camServicesSuspended = false;
    Serial.println("[ESP32-CAM] Servizi ripristinati");
  }

  Serial.println("[ESP32-CAM] Cleanup completato");
}

// ================== GESTIONE TOUCH ESP32-CAM ==================
void handleEsp32camTouch(int x, int y) {
  // Tocco in alto a sinistra: toggle overlay
  if (y < 50 && x < 240) {
    esp32camShowOverlay = !esp32camShowOverlay;
    Serial.printf("[ESP32-CAM] Overlay: %s\n", esp32camShowOverlay ? "ON" : "OFF");
  }
  // Tocco in alto a destra: toggle fullscreen
  else if (y < 50 && x >= 240) {
    esp32camFullscreen = !esp32camFullscreen;
    // Reset dimensioni per forzare ricalcolo scala
    esp32camImageWidth = 0;
    esp32camImageHeight = 0;
    gfx->fillScreen(BLACK);  // Pulisci schermo per evitare artefatti
    Serial.printf("[ESP32-CAM] Fullscreen: %s\n", esp32camFullscreen ? "ON" : "OFF");
  }
  // Tocco centrale: info debug
  else if (y > 200 && y < 300) {
    Serial.printf("[ESP32-CAM] Info: %dx%d scala=%.2f FPS=%.1f Frames=%lu\n",
                  esp32camImageWidth, esp32camImageHeight, esp32camScale,
                  esp32camCurrentFps, esp32camFrameCount);
  }
}

// ================== INDIRIZZI EEPROM PER URL ==================
#define EEPROM_ESPCAM_URL_ADDR 850      // Indirizzo inizio URL (100 byte: 850-949)
#define EEPROM_ESPCAM_URL_LEN 100       // Lunghezza massima URL
#define EEPROM_ESPCAM_URL_VALID 949     // Marker validità (0xCA = valido)
#define EEPROM_ESPCAM_URL_VALID_VALUE 0xCA

// ================== SALVA URL IN EEPROM ==================
// Flag per salvare in modo differito (evita crash nel context asincrono)
static bool esp32camUrlNeedsSave = false;

// Salvataggio SINCRONO dell'URL in EEPROM (chiamato direttamente)
void saveEsp32camUrlToEEPROMNow() {
  // Salva lunghezza e caratteri dell'URL
  int len = esp32camStreamUrl.length();
  if (len > EEPROM_ESPCAM_URL_LEN - 2) len = EEPROM_ESPCAM_URL_LEN - 2;

  // Scrivi lunghezza
  EEPROM.write(EEPROM_ESPCAM_URL_ADDR, (uint8_t)len);
  yield();  // Previene watchdog

  // Scrivi caratteri
  for (int i = 0; i < len; i++) {
    EEPROM.write(EEPROM_ESPCAM_URL_ADDR + 1 + i, esp32camStreamUrl[i]);
    if (i % 20 == 0) yield();  // Previene watchdog ogni 20 caratteri
  }

  // Scrivi marker validità
  EEPROM.write(EEPROM_ESPCAM_URL_VALID, EEPROM_ESPCAM_URL_VALID_VALUE);
  yield();

  EEPROM.commit();

  Serial.printf("[ESP32-CAM] URL salvato in EEPROM: %s\n", esp32camStreamUrl.c_str());
}

void saveEsp32camUrlToEEPROM() {
  // Salva IMMEDIATAMENTE invece di schedulare per il loop
  // Questo garantisce che l'URL sia persistito anche se il sistema viene riavviato subito
  saveEsp32camUrlToEEPROMNow();
}

// Mantenuto per compatibilità - ora chiama direttamente il salvataggio se necessario
void esp32camCheckPendingSave() {
  if (!esp32camUrlNeedsSave) return;
  esp32camUrlNeedsSave = false;
  saveEsp32camUrlToEEPROMNow();
}

// ================== CARICA URL DA EEPROM ==================
void loadEsp32camUrlFromEEPROM() {
  // Verifica marker validità
  if (EEPROM.read(EEPROM_ESPCAM_URL_VALID) != EEPROM_ESPCAM_URL_VALID_VALUE) {
    Serial.println("[ESP32-CAM] Nessun URL salvato in EEPROM, uso default");
    return;
  }

  // Leggi lunghezza
  int len = EEPROM.read(EEPROM_ESPCAM_URL_ADDR);
  if (len <= 0 || len > EEPROM_ESPCAM_URL_LEN - 2) {
    Serial.println("[ESP32-CAM] Lunghezza URL non valida in EEPROM");
    return;
  }

  // Leggi caratteri
  String url = "";
  for (int i = 0; i < len; i++) {
    char c = EEPROM.read(EEPROM_ESPCAM_URL_ADDR + 1 + i);
    url += c;
  }

  url.trim();  // Rimuovi spazi iniziali e finali
  if (url.length() > 10) {  // URL minimo valido
    esp32camStreamUrl = url;
    Serial.printf("[ESP32-CAM] URL caricato da EEPROM: '%s'\n", esp32camStreamUrl.c_str());
  }
}

// ================== SET STREAM URL ==================
void setEsp32camStreamUrl(const String& url) {
  // Rimuovi spazi iniziali e finali
  esp32camStreamUrl = url;
  esp32camStreamUrl.trim();
  Serial.printf("[ESP32-CAM] URL impostato: '%s'\n", esp32camStreamUrl.c_str());

  // Salva in EEPROM per persistenza
  saveEsp32camUrlToEEPROM();

  // Se in streaming, disconnetti per usare nuovo URL
  if (esp32camStreaming) {
    disconnectEsp32camStream();
  }
}

// ================== GET STREAM URL ==================
String getEsp32camStreamUrl() {
  return esp32camStreamUrl;
}

// ================== GESTIONE LISTA CAMERE CON LITTLEFS ==================

// Carica la lista delle camere da LittleFS
void loadCamerasFromLittleFS() {
  esp32CameraCount = 0;
  esp32CameraSelected = 0;

  if (!LittleFS.exists(ESP32CAM_CAMERAS_FILE)) {
    Serial.println("[ESP32-CAM] File camere non trovato, controllo migrazione da EEPROM...");

    // Migrazione: se c'è un URL in EEPROM, importalo come prima camera
    if (EEPROM.read(EEPROM_ESPCAM_URL_VALID) == EEPROM_ESPCAM_URL_VALID_VALUE) {
      int len = EEPROM.read(EEPROM_ESPCAM_URL_ADDR);
      if (len > 0 && len < EEPROM_ESPCAM_URL_LEN - 2) {
        String url = "";
        for (int i = 0; i < len; i++) {
          url += (char)EEPROM.read(EEPROM_ESPCAM_URL_ADDR + 1 + i);
        }
        url.trim();
        if (url.length() > 10) {
          strncpy(esp32Cameras[0].name, "Camera 1", 32);
          esp32Cameras[0].name[32] = '\0';
          strncpy(esp32Cameras[0].url, url.c_str(), 100);
          esp32Cameras[0].url[100] = '\0';
          esp32CameraCount = 1;
          esp32CameraSelected = 0;
          esp32camStreamUrl = url;
          saveCamerasToLittleFS();
          Serial.printf("[ESP32-CAM] Migrata camera da EEPROM: %s\n", url.c_str());
        }
      }
    }
    return;
  }

  File file = LittleFS.open(ESP32CAM_CAMERAS_FILE, "r");
  if (!file) {
    Serial.println("[ESP32-CAM] Errore apertura file camere");
    return;
  }

  // Leggi contenuto file
  String content = file.readString();
  file.close();

  Serial.printf("[ESP32-CAM] File camere letto: %d bytes\n", content.length());

  // Parse JSON manualmente (ArduinoJson non disponibile)
  // Formato: {"cameras":[{"name":"...","url":"..."},...],"selected":N}

  // Estrai selected
  int selIdx = content.indexOf("\"selected\":");
  if (selIdx >= 0) {
    int selStart = selIdx + 11;
    int selEnd = content.indexOf("}", selStart);
    if (selEnd < 0) selEnd = content.length();
    String selStr = content.substring(selStart, selEnd);
    selStr.trim();
    // Rimuovi eventuali caratteri non numerici
    int endNum = 0;
    while (endNum < selStr.length() && (selStr[endNum] >= '0' && selStr[endNum] <= '9')) endNum++;
    selStr = selStr.substring(0, endNum);
    esp32CameraSelected = selStr.toInt();
  }

  // Estrai array cameras
  int arrStart = content.indexOf("\"cameras\":[");
  if (arrStart < 0) {
    Serial.println("[ESP32-CAM] Array cameras non trovato");
    return;
  }
  arrStart += 11;  // salta "cameras":[

  int arrEnd = content.indexOf("]", arrStart);
  if (arrEnd < 0) arrEnd = content.length();

  String camerasStr = content.substring(arrStart, arrEnd);

  // Parse ogni oggetto camera
  int pos = 0;
  while (esp32CameraCount < MAX_ESP32_CAMERAS) {
    int objStart = camerasStr.indexOf("{", pos);
    if (objStart < 0) break;

    int objEnd = camerasStr.indexOf("}", objStart);
    if (objEnd < 0) break;

    String camObj = camerasStr.substring(objStart, objEnd + 1);

    // Estrai name
    int nameStart = camObj.indexOf("\"name\":\"");
    if (nameStart >= 0) {
      nameStart += 8;
      int nameEnd = camObj.indexOf("\"", nameStart);
      if (nameEnd > nameStart) {
        String name = camObj.substring(nameStart, nameEnd);
        strncpy(esp32Cameras[esp32CameraCount].name, name.c_str(), 32);
        esp32Cameras[esp32CameraCount].name[32] = '\0';
      }
    }

    // Estrai url
    int urlStart = camObj.indexOf("\"url\":\"");
    if (urlStart >= 0) {
      urlStart += 7;
      int urlEnd = camObj.indexOf("\"", urlStart);
      if (urlEnd > urlStart) {
        String url = camObj.substring(urlStart, urlEnd);
        strncpy(esp32Cameras[esp32CameraCount].url, url.c_str(), 100);
        esp32Cameras[esp32CameraCount].url[100] = '\0';
      }
    }

    esp32CameraCount++;
    pos = objEnd + 1;
  }

  // Valida selected
  if (esp32CameraSelected >= esp32CameraCount) {
    esp32CameraSelected = 0;
  }

  // Imposta URL attivo dalla camera selezionata
  if (esp32CameraCount > 0) {
    esp32camStreamUrl = String(esp32Cameras[esp32CameraSelected].url);
  }

  Serial.printf("[ESP32-CAM] Caricate %d camere, selezionata: %d\n", esp32CameraCount, esp32CameraSelected);
}

// Salva la lista delle camere su LittleFS
void saveCamerasToLittleFS() {
  File file = LittleFS.open(ESP32CAM_CAMERAS_FILE, "w");
  if (!file) {
    Serial.println("[ESP32-CAM] Errore apertura file camere per scrittura");
    return;
  }

  // Costruisci JSON manualmente
  String json = "{\"cameras\":[";

  for (int i = 0; i < esp32CameraCount; i++) {
    if (i > 0) json += ",";
    json += "{\"name\":\"";
    json += esp32Cameras[i].name;
    json += "\",\"url\":\"";
    json += esp32Cameras[i].url;
    json += "\"}";
  }

  json += "],\"selected\":";
  json += String(esp32CameraSelected);
  json += "}";

  file.print(json);
  file.close();

  Serial.printf("[ESP32-CAM] Salvate %d camere su LittleFS\n", esp32CameraCount);
}

// Aggiunge una nuova camera alla lista
bool addEsp32Camera(const char* name, const char* url) {
  if (esp32CameraCount >= MAX_ESP32_CAMERAS) {
    Serial.println("[ESP32-CAM] Lista camere piena!");
    return false;
  }

  if (strlen(name) == 0 || strlen(url) < 10) {
    Serial.println("[ESP32-CAM] Nome o URL non validi");
    return false;
  }

  strncpy(esp32Cameras[esp32CameraCount].name, name, 32);
  esp32Cameras[esp32CameraCount].name[32] = '\0';
  strncpy(esp32Cameras[esp32CameraCount].url, url, 100);
  esp32Cameras[esp32CameraCount].url[100] = '\0';

  esp32CameraCount++;
  saveCamerasToLittleFS();

  // Sincronizza anche con EEPROM se è la prima camera o quella selezionata
  if (esp32CameraCount == 1) {
    esp32CameraSelected = 0;
    esp32camStreamUrl = String(url);
    saveEsp32camUrlToEEPROM();
  }

  Serial.printf("[ESP32-CAM] Aggiunta camera '%s': %s\n", name, url);
  return true;
}

// Rimuove una camera dalla lista
bool removeEsp32Camera(int index) {
  if (index < 0 || index >= esp32CameraCount) {
    Serial.println("[ESP32-CAM] Indice camera non valido");
    return false;
  }

  Serial.printf("[ESP32-CAM] Rimozione camera %d: '%s'\n", index, esp32Cameras[index].name);

  // Sposta le camere successive
  for (int i = index; i < esp32CameraCount - 1; i++) {
    memcpy(&esp32Cameras[i], &esp32Cameras[i + 1], sizeof(Esp32Camera));
  }

  esp32CameraCount--;

  // Aggiusta l'indice selezionato
  if (esp32CameraCount == 0) {
    esp32CameraSelected = 0;
    esp32camStreamUrl = "";
  } else {
    if (esp32CameraSelected >= esp32CameraCount) {
      esp32CameraSelected = esp32CameraCount - 1;
    } else if (esp32CameraSelected > index) {
      esp32CameraSelected--;
    }
    // Aggiorna URL attivo
    esp32camStreamUrl = String(esp32Cameras[esp32CameraSelected].url);
    saveEsp32camUrlToEEPROM();
  }

  saveCamerasToLittleFS();
  return true;
}

// Cancella tutte le camere
void clearAllEsp32Cameras() {
  Serial.println("[ESP32-CAM] Cancellazione di tutte le camere");

  esp32CameraCount = 0;
  esp32CameraSelected = 0;
  esp32camStreamUrl = "";

  // Cancella il file
  if (LittleFS.exists(ESP32CAM_CAMERAS_FILE)) {
    LittleFS.remove(ESP32CAM_CAMERAS_FILE);
  }

  Serial.println("[ESP32-CAM] Tutte le camere cancellate");
}

// Seleziona una camera come attiva
bool selectEsp32Camera(int index) {
  if (index < 0 || index >= esp32CameraCount) {
    Serial.println("[ESP32-CAM] Indice camera non valido per selezione");
    return false;
  }

  esp32CameraSelected = index;
  esp32camStreamUrl = String(esp32Cameras[index].url);

  // Salva selezione e aggiorna EEPROM
  saveCamerasToLittleFS();
  saveEsp32camUrlToEEPROM();

  // Se in streaming, disconnetti per usare il nuovo URL
  if (esp32camStreaming) {
    disconnectEsp32camStream();
  }

  Serial.printf("[ESP32-CAM] Selezionata camera %d: '%s'\n", index, esp32Cameras[index].name);
  return true;
}

#endif // EFFECT_ESP32CAM
