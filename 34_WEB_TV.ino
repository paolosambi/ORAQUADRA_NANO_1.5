// ================== WEB TV - STREAMING MJPEG ==================
// Visualizza streaming TV da server Python via MJPEG
// Richiede StreamServer in esecuzione sul PC
// DISABILITATO - Troppo lag per ESP32

#ifdef EFFECT_WEB_TV  // Abilitare nel file principale se necessario

#include <HTTPClient.h>
#include <JPEGDEC.h>

// ================== CONFIGURAZIONE ==================
#define TV_SERVER_IP      "192.168.1.24"  // IP del PC con StreamServer
#define TV_SERVER_PORT    5000
#define TV_STREAM_URL     "/stream"
#define TV_BUFFER_SIZE    131072  // 128KB buffer per frame JPEG (PSRAM)

// ================== TEMA - COERENTE CON WEB RADIO ==================
#define TV_BG_COLOR       0x0000  // Nero puro
#define TV_ACCENT_COLOR   0x07FF  // Ciano brillante
#define TV_ACCENT_DARK    0x0575  // Ciano scuro
#define TV_TEXT_COLOR     0xFFFF  // Bianco
#define TV_TEXT_DIM       0x7BCF  // Grigio
#define TV_BUTTON_COLOR   0x1082  // Grigio-blu scuro
#define TV_BUTTON_BORDER  0x07FF  // Ciano

// ================== LAYOUT 480x480 - VIDEO A SCHERMO PIENO ==================
#define TV_HEADER_Y       5
#define TV_VIDEO_Y        0       // Video da y=0 per schermo pieno
#define TV_VIDEO_H        480     // Altezza piena
#define TV_CONTROLS_Y     400
#define TV_EXIT_Y         440
#define TV_FULLSCREEN     true    // Modalità schermo pieno per video

// ================== CANALI TV ==================
struct TVChannel {
  const char* name;
  const char* logo;
  int index;
};

TVChannel tvChannels[] = {
  {"Rai 1", "1", 0},
  {"Rai 2", "2", 1},
  {"Rai 3", "3", 2},
  {"Rai News", "N", 3},
  {"LA7", "7", 5},
  {"Sky TG24", "S", 6},
  {"TGCOM24", "T", 7},
  {"RTL 102.5", "R", 8},
};
#define TV_CHANNEL_COUNT 8

// ================== VARIABILI GLOBALI ==================
bool webTVInitialized = false;
bool webTVNeedsRedraw = true;
bool webTVConnected = false;
bool webTVStreaming = false;
int webTVCurrentChannel = 0;
String webTVServerIP = TV_SERVER_IP;
uint16_t webTVServerPort = TV_SERVER_PORT;

// Buffer per frame JPEG
uint8_t* tvJpegBuffer = nullptr;
size_t tvJpegBufferSize = 0;

// JPEG decoder
JPEGDEC tvJpeg;

// HTTP client per streaming
WiFiClient tvStreamClient;
HTTPClient tvHttp;

// ================== PROTOTIPI ==================
void initWebTV();
void updateWebTV();
void drawWebTVUI();
void drawTVHeader();
void drawTVVideo();
void drawTVChannelBar();
void drawTVControls();
void drawTVExitButton();
bool handleWebTVTouch(int16_t x, int16_t y);
bool connectToTVStream();
void disconnectTVStream();
bool readTVFrame();
void selectTVChannel(int channel);
int tvJpegDraw(JPEGDRAW *pDraw);

// ================== CALLBACK JPEG DECODER ==================
int tvJpegDraw(JPEGDRAW *pDraw) {
  // Disegna direttamente sul display - schermo pieno
  gfx->draw16bitRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}

// ================== INIZIALIZZAZIONE ==================
void initWebTV() {
  if (webTVInitialized) return;

  Serial.println("[WEBTV] Inizializzazione...");

  // Alloca buffer JPEG in PSRAM
  if (tvJpegBuffer == nullptr) {
    tvJpegBuffer = (uint8_t*)heap_caps_malloc(TV_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    if (tvJpegBuffer == nullptr) {
      tvJpegBuffer = (uint8_t*)malloc(TV_BUFFER_SIZE);
    }
    if (tvJpegBuffer == nullptr) {
      Serial.println("[WEBTV] ERRORE: Impossibile allocare buffer!");
      return;
    }
  }

  webTVConnected = false;
  webTVStreaming = false;
  webTVNeedsRedraw = true;
  webTVInitialized = true;

  Serial.println("[WEBTV] Inizializzato");
}

// ================== CONNESSIONE STREAM ==================
bool connectToTVStream() {
  if (webTVConnected) return true;

  String url = "http://" + webTVServerIP + ":" + String(webTVServerPort) + TV_STREAM_URL;
  Serial.printf("[WEBTV] Connessione a %s\n", url.c_str());

  tvHttp.begin(tvStreamClient, url);
  tvHttp.setTimeout(5000);

  int httpCode = tvHttp.GET();

  if (httpCode == HTTP_CODE_OK) {
    webTVConnected = true;
    webTVStreaming = true;
    Serial.println("[WEBTV] Connesso!");
    return true;
  } else {
    Serial.printf("[WEBTV] Errore connessione: %d\n", httpCode);
    tvHttp.end();
    return false;
  }
}

void disconnectTVStream() {
  webTVStreaming = false;
  webTVConnected = false;
  tvHttp.end();
  Serial.println("[WEBTV] Disconnesso");
}

// ================== LETTURA FRAME ==================
bool readTVFrame() {
  static uint32_t frameCount = 0;
  static uint32_t lastDebug = 0;

  if (!webTVConnected || !webTVStreaming) return false;

  WiFiClient* stream = tvHttp.getStreamPtr();
  if (!stream || !stream->connected()) {
    Serial.println("[WEBTV] Stream disconnesso");
    webTVConnected = false;
    return false;
  }

  int available = stream->available();
  if (available > 0 && millis() - lastDebug > 5000) {
    Serial.printf("[WEBTV] Bytes disponibili: %d\n", available);
    lastDebug = millis();
  }

  // Cerca header MJPEG
  String line;
  unsigned long startWait = millis();

  while (millis() - startWait < 500) {
    if (stream->available()) {
      line = stream->readStringUntil('\n');
      line.trim();  // Rimuovi \r e spazi

      // Ignora righe vuote e chunk size (hex numbers del chunked transfer)
      if (line.length() == 0) continue;

      // Salta chunk size markers (sono numeri hex come "55a0", "5597")
      bool isChunkSize = true;
      for (int i = 0; i < line.length() && isChunkSize; i++) {
        char c = line.charAt(i);
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
          isChunkSize = false;
        }
      }
      if (isChunkSize && line.length() <= 8) continue;  // Skip chunk markers

      // Salta boundary marker
      if (line.startsWith("--")) continue;

      // Salta Content-Type
      if (line.startsWith("Content-Type")) continue;

      // Debug prime righe
      if (frameCount < 3) {
        Serial.printf("[WEBTV] Line: '%s'\n", line.c_str());
      }

      if (line.startsWith("Content-Length")) {
        int colonPos = line.indexOf(':');
        if (colonPos > 0) {
          int contentLength = line.substring(colonPos + 1).toInt();
          if (contentLength > 0 && contentLength < TV_BUFFER_SIZE) {
            // Leggi eventuali righe vuote prima dei dati
            while (stream->available() && stream->peek() == '\r') {
              stream->read();
            }
            if (stream->available() && stream->peek() == '\n') {
              stream->read();
            }

            // Leggi dati JPEG
            size_t bytesRead = 0;
            startWait = millis();
            while (bytesRead < contentLength && millis() - startWait < 2000) {
              if (stream->available()) {
                int toRead = min((int)(contentLength - bytesRead), stream->available());
                int read = stream->readBytes(tvJpegBuffer + bytesRead, toRead);
                bytesRead += read;
              }
              yield();
            }

            if (bytesRead == contentLength) {
              tvJpegBufferSize = bytesRead;
              frameCount++;
              if (frameCount == 1 || frameCount % 100 == 0) {
                Serial.printf("[WEBTV] Frame %d ricevuto, size: %d\n", frameCount, bytesRead);
              }
              return true;
            } else {
              Serial.printf("[WEBTV] Frame incompleto: %d/%d bytes\n", bytesRead, contentLength);
            }
          }
        }
      }
    }
    yield();
  }

  return false;
}

// ================== SELEZIONE CANALE ==================
void selectTVChannel(int channel) {
  if (channel < 0 || channel >= TV_CHANNEL_COUNT) return;

  webTVCurrentChannel = channel;

  // Invia comando al server
  HTTPClient http;
  String url = "http://" + webTVServerIP + ":" + String(webTVServerPort) + "/api/tv/play";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"channel\":" + String(tvChannels[channel].index) + "}";
  int httpCode = http.POST(body);

  if (httpCode == HTTP_CODE_OK) {
    Serial.printf("[WEBTV] Canale selezionato: %s\n", tvChannels[channel].name);

    // Riconnetti allo stream
    disconnectTVStream();
    delay(500);
    connectToTVStream();
  } else {
    Serial.printf("[WEBTV] Errore selezione canale: %d\n", httpCode);
  }

  http.end();
  webTVNeedsRedraw = true;
}

// ================== UPDATE LOOP ==================
void updateWebTV() {
  static int lastActiveMode = -1;
  static uint32_t lastTouchCheck = 0;

  if (lastActiveMode != MODE_WEB_TV) {
    webTVNeedsRedraw = true;
    webTVInitialized = false;
  }
  lastActiveMode = currentMode;

  if (!webTVInitialized) {
    initWebTV();
    return;
  }

  // Gestione touch - meno frequente durante streaming per performance
  uint32_t now = millis();
  if (now - lastTouchCheck > (webTVStreaming ? 200 : 50)) {
    lastTouchCheck = now;
    ts.read();
    if (ts.isTouched && ts.touches > 0) {
      static uint32_t lastTouch = 0;
      if (now - lastTouch > 400) {
        int x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 479);
        int y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 479);

        x = constrain(x, 0, 479);
        y = constrain(y, 0, 479);

        if (handleWebTVTouch(x, y)) {
          disconnectTVStream();
          webTVInitialized = false;
          handleModeChange();
          return;
        }
        lastTouch = now;
      }
    }
  }

  // Ridisegna UI solo se non in streaming
  if (webTVNeedsRedraw && !webTVStreaming) {
    drawWebTVUI();
    webTVNeedsRedraw = false;
  }

  // Auto-connetti allo stream se non connesso
  if (!webTVStreaming && !webTVConnected) {
    static uint32_t lastConnectAttempt = 0;
    // Riprova ogni 2 secondi
    if (now - lastConnectAttempt > 2000) {
      lastConnectAttempt = now;
      if (connectToTVStream()) {
        Serial.println("[WEBTV] Auto-connesso allo stream!");
      }
    }
  }

  // Leggi e visualizza frame se connesso - processa multipli frame per loop
  if (webTVStreaming) {
    static uint32_t lastFrameTime = 0;
    int framesThisLoop = 0;
    const int maxFramesPerLoop = 3;  // Max frame per iterazione

    while (framesThisLoop < maxFramesPerLoop && readTVFrame()) {
      lastFrameTime = millis();
      // Decodifica e visualizza JPEG con PSRAM
      if (tvJpeg.openRAM(tvJpegBuffer, tvJpegBufferSize, tvJpegDraw)) {
        tvJpeg.decode(0, 0, 0);  // Decode a schermo pieno
        tvJpeg.close();
      }
      framesThisLoop++;
    }

    // Controlla timeout solo se non abbiamo ricevuto frame
    if (framesThisLoop == 0) {
      if (lastFrameTime == 0) lastFrameTime = millis();
      if (millis() - lastFrameTime > 5000) {
        // Nessun frame per 5 secondi, riconnetti
        Serial.println("[WEBTV] Timeout frame, riconnessione...");
        disconnectTVStream();
        lastFrameTime = 0;
      }
    }
  }
}

// ================== DISEGNO UI ==================
void drawWebTVUI() {
  gfx->fillScreen(TV_BG_COLOR);

  // Bordi ciano
  gfx->drawFastHLine(0, 0, 480, TV_ACCENT_COLOR);
  gfx->drawFastHLine(0, 479, 480, TV_ACCENT_COLOR);
  gfx->drawFastVLine(0, 0, 480, TV_ACCENT_COLOR);
  gfx->drawFastVLine(479, 0, 480, TV_ACCENT_COLOR);

  drawTVHeader();
  drawTVVideo();
  drawTVChannelBar();
  drawTVControls();
  drawTVExitButton();
}

// ================== HEADER ==================
void drawTVHeader() {
  int centerX = 240;

  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(TV_ACCENT_COLOR);
  gfx->setCursor(centerX - 95, TV_HEADER_Y + 28);
  gfx->print(")) WEB TV ((");

  // Linea separatrice
  gfx->drawFastHLine(30, TV_HEADER_Y + 40, 420, TV_ACCENT_COLOR);
}

// ================== AREA VIDEO ==================
void drawTVVideo() {
  // Sfondo area video
  gfx->fillRect(0, TV_VIDEO_Y, 480, TV_VIDEO_H, TV_BG_COLOR);

  // Bordo area video
  gfx->drawRect(0, TV_VIDEO_Y, 480, TV_VIDEO_H, TV_ACCENT_DARK);

  if (!webTVStreaming) {
    // Messaggio quando non connesso
    gfx->setFont(u8g2_font_helvB14_tr);
    gfx->setTextColor(TV_TEXT_DIM);
    gfx->setCursor(140, TV_VIDEO_Y + TV_VIDEO_H/2 - 20);
    gfx->print("Seleziona un canale");

    gfx->setFont(u8g2_font_helvR12_tr);
    gfx->setCursor(100, TV_VIDEO_Y + TV_VIDEO_H/2 + 10);
    gfx->print("Server: ");
    gfx->setTextColor(TV_ACCENT_COLOR);
    gfx->print(webTVServerIP);
    gfx->print(":");
    gfx->print(webTVServerPort);
  }
}

// ================== BARRA CANALI ==================
void drawTVChannelBar() {
  int y = TV_VIDEO_Y + TV_VIDEO_H + 5;
  int btnW = 55;
  int btnH = 35;
  int spacing = 4;
  int startX = (480 - (TV_CHANNEL_COUNT * btnW + (TV_CHANNEL_COUNT-1) * spacing)) / 2;

  for (int i = 0; i < TV_CHANNEL_COUNT; i++) {
    int bx = startX + i * (btnW + spacing);
    bool selected = (i == webTVCurrentChannel && webTVStreaming);

    // Sfondo bottone
    if (selected) {
      gfx->fillRoundRect(bx, y, btnW, btnH, 6, TV_ACCENT_DARK);
      gfx->drawRoundRect(bx, y, btnW, btnH, 6, TV_ACCENT_COLOR);
    } else {
      gfx->fillRoundRect(bx, y, btnW, btnH, 6, TV_BUTTON_COLOR);
      gfx->drawRoundRect(bx, y, btnW, btnH, 6, TV_BUTTON_BORDER);
    }

    // Logo/numero canale
    gfx->setFont(u8g2_font_helvB12_tr);
    gfx->setTextColor(selected ? TV_TEXT_COLOR : TV_TEXT_DIM);
    int textW = strlen(tvChannels[i].logo) * 8;
    gfx->setCursor(bx + (btnW - textW) / 2, y + 24);
    gfx->print(tvChannels[i].logo);
  }
}

// ================== CONTROLLI ==================
void drawTVControls() {
  int y = TV_CONTROLS_Y;
  int centerX = 240;

  // Nome canale corrente
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(TV_TEXT_COLOR);

  String channelName = webTVStreaming ? tvChannels[webTVCurrentChannel].name : "Nessun canale";
  int textW = channelName.length() * 9;
  gfx->setCursor(centerX - textW/2, y + 18);
  gfx->print(channelName);

  // Stato connessione
  gfx->setFont(u8g2_font_helvR10_tr);
  if (webTVStreaming) {
    gfx->setTextColor(0x07E0);  // Verde
    gfx->setCursor(centerX - 40, y + 35);
    gfx->print("STREAMING");
  } else {
    gfx->setTextColor(TV_TEXT_DIM);
    gfx->setCursor(centerX - 30, y + 35);
    gfx->print("OFFLINE");
  }
}

// ================== EXIT BUTTON ==================
void drawTVExitButton() {
  int btnW = 100;
  int btnX = 240 - btnW / 2;
  int y = TV_EXIT_Y;
  int btnH = 36;

  gfx->fillRoundRect(btnX, y, btnW, btnH, 8, 0xFB20);  // Arancione
  gfx->drawRoundRect(btnX, y, btnW, btnH, 8, 0xFD60);

  // Icona X
  int iconX = btnX + 20;
  int iconY = y + btnH / 2;
  gfx->drawLine(iconX - 5, iconY - 5, iconX + 5, iconY + 5, TV_TEXT_COLOR);
  gfx->drawLine(iconX - 5, iconY + 5, iconX + 5, iconY - 5, TV_TEXT_COLOR);

  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(TV_TEXT_COLOR);
  gfx->setCursor(btnX + 38, y + 24);
  gfx->print("EXIT");
}

// ================== GESTIONE TOUCH ==================
bool handleWebTVTouch(int16_t x, int16_t y) {
  playTouchSound();

  // Tocco su area canali
  int channelY = TV_VIDEO_Y + TV_VIDEO_H + 5;
  int btnW = 55;
  int btnH = 35;
  int spacing = 4;
  int startX = (480 - (TV_CHANNEL_COUNT * btnW + (TV_CHANNEL_COUNT-1) * spacing)) / 2;

  if (y >= channelY && y <= channelY + btnH) {
    for (int i = 0; i < TV_CHANNEL_COUNT; i++) {
      int bx = startX + i * (btnW + spacing);
      if (x >= bx && x <= bx + btnW) {
        selectTVChannel(i);
        return false;
      }
    }
  }

  // EXIT
  int exitBtnW = 100;
  int exitX = 240 - exitBtnW / 2;
  if (y >= TV_EXIT_Y && y <= TV_EXIT_Y + 36) {
    if (x >= exitX && x <= exitX + exitBtnW) {
      Serial.println("[WEBTV] EXIT");
      return true;
    }
  }

  return false;
}

// ================== WEBSERVER ==================
const char WEB_TV_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Web TV</title><style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:sans-serif;background:#1a1a2e;min-height:100vh;padding:20px;color:#eee}
.c{max-width:500px;margin:0 auto;background:rgba(255,255,255,.1);border-radius:20px;overflow:hidden}
.h{background:linear-gradient(135deg,#e74c3c,#c0392b);padding:25px;text-align:center}
.h h1{font-size:1.5em;margin-bottom:5px}
.ct{padding:25px}
.s{background:rgba(0,0,0,.3);border-radius:15px;padding:20px;margin-bottom:15px}
.s h3{margin-bottom:15px;color:#e74c3c}
.channel{padding:15px;background:rgba(255,255,255,.1);border-radius:10px;margin-bottom:8px;cursor:pointer;display:flex;align-items:center;gap:15px}
.channel:hover{background:rgba(231,76,60,.3)}
.channel.active{background:rgba(231,76,60,.5);border:2px solid #e74c3c}
.channel .logo{font-size:1.5em;width:40px;text-align:center}
.channel .name{flex:1;font-weight:bold}
.btn{width:100%;padding:15px;border:none;border-radius:10px;font-weight:bold;cursor:pointer;font-size:1em;margin-top:10px}
.btn-primary{background:linear-gradient(135deg,#e74c3c,#c0392b);color:#fff}
.btn-stop{background:#666;color:#fff}
.status{text-align:center;padding:15px;background:rgba(0,0,0,.2);border-radius:10px;margin-bottom:15px}
.status.on{color:#2ecc71}.status.off{color:#e74c3c}
.hm{display:block;text-align:center;color:#94a3b8;padding:10px;text-decoration:none;font-size:.9em}.hm:hover{color:#fff}
.server-config{display:flex;gap:10px;margin-bottom:15px}
.server-config input{flex:1;padding:10px;border-radius:8px;border:1px solid #444;background:#2a2a4a;color:#fff}
</style></head><body><div class="c"><a href="/" class="hm">&larr; Home</a><div class="h">
<h1>📺 Web TV</h1><p>Streaming TV Italiane</p></div><div class="ct">
<div class="status" id="status">Offline</div>
<div class="s"><h3>Server StreamServer</h3>
<div class="server-config">
<input type="text" id="serverIP" placeholder="IP Server" value="192.168.1.24">
<input type="text" id="serverPort" placeholder="Porta" value="5000" style="max-width:80px">
</div>
<p style="color:#888;font-size:0.9em">Assicurati che StreamServer sia in esecuzione sul PC</p>
</div>
<div class="s"><h3>Canali</h3>
<div id="channels"></div>
</div>
<button class="btn btn-stop" onclick="stopTV()">Stop Streaming</button>
<button class="btn btn-primary" onclick="activateMode()">Mostra su Display</button>
<button class="btn" style="background:#333;margin-top:5px" onclick="loadChannels()">Ricarica Canali</button>
</div></div><script>
var currentChannel=-1;
var channels=[];
function getServerUrl(){
  return 'http://'+document.getElementById('serverIP').value+':'+document.getElementById('serverPort').value;
}
function loadChannels(){
  document.getElementById('status').textContent='Caricamento canali...';
  fetch(getServerUrl()+'/api/tv/channels')
    .then(r=>r.json())
    .then(d=>{
      channels=d.channels||[];
      render();
      document.getElementById('status').textContent=channels.length+' canali disponibili';
    })
    .catch(e=>{
      document.getElementById('status').textContent='Errore: server non raggiungibile';
      document.getElementById('status').className='status off';
    });
}
function render(){
  var html='';
  if(channels.length==0){
    html='<p style="color:#888;text-align:center">Nessun canale. Clicca "Ricarica Canali"</p>';
  }else{
    for(var i=0;i<channels.length;i++){
      var ch=channels[i];
      html+='<div class="channel'+(i==currentChannel?' active':'')+'" onclick="playChannel('+i+')">';
      html+='<span class="logo">'+(ch.icon||'📺')+'</span>';
      html+='<span class="name">'+ch.name+'</span></div>';
    }
  }
  document.getElementById('channels').innerHTML=html;
  if(currentChannel>=0&&channels[currentChannel]){
    document.getElementById('status').textContent='Streaming: '+channels[currentChannel].name;
    document.getElementById('status').className='status on';
  }
}
function playChannel(idx){
  if(idx<0||idx>=channels.length)return;
  document.getElementById('status').textContent='Avvio '+channels[idx].name+'...';
  fetch(getServerUrl()+'/api/tv/play',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({channel:idx})
  }).then(r=>r.json()).then(d=>{
    if(d.success){
      currentChannel=idx;
      render();
      fetch('/webtv/reconnect');
    }else{
      alert('Errore: '+(d.error||'sconosciuto'));
      document.getElementById('status').textContent='Errore avvio';
    }
  }).catch(e=>{
    alert('Errore connessione: '+e);
    document.getElementById('status').textContent='Server non raggiungibile';
  });
}
function stopTV(){
  fetch(getServerUrl()+'/api/tv/stop',{method:'POST'})
    .then(()=>{
      currentChannel=-1;
      render();
      fetch('/webtv/disconnect');
      document.getElementById('status').textContent='Streaming fermato';
      document.getElementById('status').className='status off';
    })
    .catch(()=>{currentChannel=-1;render();});
}
function activateMode(){
  fetch('/webtv/activate').then(()=>{
    if(currentChannel>=0)fetch('/webtv/reconnect');
  });
}
loadChannels();
</script></body></html>
)rawliteral";

void setup_webtv_webserver(AsyncWebServer* server) {
  Serial.println("[WEBTV-WEB] Configurazione endpoints...");

  server->on("/webtv", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", WEB_TV_HTML);
  });

  server->on("/webtv/activate", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("[WEBTV] Attivazione modalità WebTV da web");
    currentMode = MODE_WEB_TV;
    userMode = MODE_WEB_TV;
    webTVNeedsRedraw = true;
    webTVInitialized = false;  // Forza reinizializzazione
    // Pulisci display per forzare ridisegno
    gfx->fillScreen(BLACK);
    lastHour = 255;
    lastMinute = 255;
    request->send(200, "application/json", "{\"success\":true}");
  });

  server->on("/webtv/reconnect", HTTP_GET, [](AsyncWebServerRequest *request){
    // Forza riconnessione allo stream (per cambio canale da web)
    Serial.println("[WEBTV-WEB] Riconnessione richiesta da web");
    disconnectTVStream();
    webTVNeedsRedraw = true;
    // La riconnessione avverrà automaticamente nel loop updateWebTV
    request->send(200, "application/json", "{\"success\":true}");
  });

  server->on("/webtv/disconnect", HTTP_GET, [](AsyncWebServerRequest *request){
    // Disconnetti dallo stream (stop da web)
    Serial.println("[WEBTV-WEB] Disconnessione richiesta da web");
    disconnectTVStream();
    webTVNeedsRedraw = true;
    request->send(200, "application/json", "{\"success\":true}");
  });

  server->on("/webtv/config", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"ip\":\"" + webTVServerIP + "\",\"port\":" + String(webTVServerPort) + "}";
    request->send(200, "application/json", json);
  });

  server->on("/webtv/config", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      // Parse JSON body per configurazione server
      String body = String((char*)data).substring(0, len);
      // Semplice parsing
      int ipStart = body.indexOf("\"ip\":\"") + 6;
      int ipEnd = body.indexOf("\"", ipStart);
      if (ipStart > 5 && ipEnd > ipStart) {
        webTVServerIP = body.substring(ipStart, ipEnd);
      }
      int portStart = body.indexOf("\"port\":") + 7;
      if (portStart > 6) {
        webTVServerPort = body.substring(portStart).toInt();
      }
      request->send(200, "application/json", "{\"success\":true}");
    });

  Serial.println("[WEBTV-WEB] Endpoints configurati su /webtv");
}

#endif // EFFECT_WEB_TV
