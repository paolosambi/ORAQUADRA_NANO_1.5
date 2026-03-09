// ================== BLUETOOTH A2DP AUDIO STREAMING ==================
// Modulo per streaming audio via Bluetooth A2DP su ESP32-S3
// L'ESP32-S3 riceve audio PCM dal server Python e lo trasmette via BT
// a cuffie/speaker Bluetooth
//
// FUNZIONALITÀ:
// - Ricezione audio PCM via HTTP dal server Python (sincronizzato con video)
// - Trasmissione via Bluetooth A2DP come sorgente audio
// - Pairing con dispositivi BT tramite interfaccia web
// - Sincronizzazione audio/video tramite timestamp
// - Discovery dispositivi BT Classic tramite BluetoothSerial
//
// REQUISITI:
// - ESP32-S3 (supporta Bluetooth Classic A2DP)
// - Per streaming audio: Libreria ESP32-A2DP di Phil Schatzmann
//   Installa da: https://github.com/pschatzmann/ESP32-A2DP
//
// NOTA: Questo modulo è attivo solo quando EFFECT_MJPEG_STREAM è abilitato

#ifdef EFFECT_MJPEG_STREAM

// Prova a includere la libreria A2DP se disponibile
#if __has_include("BluetoothA2DPSource.h")
  #define BT_A2DP_AVAILABLE
  #include "BluetoothA2DPSource.h"
  #pragma message "ESP32-A2DP library found - Bluetooth audio streaming ENABLED"
#endif

// Prova a includere BluetoothSerial per discovery (richiede BT Classic abilitato)
#if __has_include("BluetoothSerial.h") && defined(CONFIG_BT_CLASSIC_ENABLED)
  #define BT_SCAN_AVAILABLE
  #include "BluetoothSerial.h"
  #pragma message "BluetoothSerial available - BT discovery ENABLED"
#else
  #pragma message "BluetoothSerial NOT available - BT discovery DISABLED (enable Bluetooth Classic in board config)"
#endif

// ================== CONFIGURAZIONE AUDIO BT ==================
#define BT_AUDIO_SAMPLE_RATE    44100   // Sample rate audio (44.1kHz standard)
#define BT_AUDIO_CHANNELS       2       // Stereo
#define BT_AUDIO_BITS           16      // 16-bit audio
#define BT_AUDIO_BUFFER_SIZE    4096    // Buffer audio circolare (samples)
#define BT_DEVICE_NAME          "OraQuadra_Audio"  // Nome dispositivo BT visibile

// Buffer audio per sincronizzazione
#define AUDIO_SYNC_BUFFER_MS    100     // Buffer di sincronizzazione (ms)
#define AUDIO_HTTP_TIMEOUT      5000    // Timeout connessione HTTP audio (ms)

// ================== VARIABILI STATO BT AUDIO ==================
bool btAudioEnabled = false;            // Audio BT abilitato
bool btAudioConnected = false;          // Dispositivo BT connesso
bool btAudioStreaming = false;          // Streaming audio attivo
String btTargetDevice = "";             // Nome/MAC dispositivo target per auto-connect
String btConnectedDevice = "";          // Nome dispositivo attualmente connesso

// Buffer audio circolare
int16_t *btAudioBuffer = nullptr;       // Buffer audio PCM in PSRAM
volatile int btAudioWritePos = 0;       // Posizione scrittura buffer
volatile int btAudioReadPos = 0;        // Posizione lettura buffer
volatile int btAudioBufferCount = 0;    // Samples nel buffer

// Sincronizzazione audio/video
unsigned long audioTimestamp = 0;       // Timestamp corrente audio (ms dal server)
unsigned long videoTimestamp = 0;       // Timestamp corrente video (ms dal server)
int audioVideoOffset = 0;               // Offset sincronizzazione (ms)

// Client HTTP per audio stream
WiFiClient btAudioClient;
HTTPClient btAudioHttp;
bool btAudioHttpConnected = false;

// Statistiche
unsigned long btAudioBytesReceived = 0;
unsigned long btAudioBytesSent = 0;
float btAudioBufferLevel = 0;           // Livello buffer 0-100%

#ifdef BT_A2DP_AVAILABLE
BluetoothA2DPSource a2dp_source;

// ================== CALLBACK A2DP ==================
// Callback chiamata quando A2DP richiede dati audio
int32_t btAudioDataCallback(Frame *frame, int32_t frame_count) {
  if (!btAudioStreaming || btAudioBuffer == nullptr) {
    // Nessun audio disponibile - invia silenzio
    memset(frame, 0, frame_count * sizeof(Frame));
    return frame_count;
  }

  int samplesNeeded = frame_count * 2;  // Stereo = 2 samples per frame
  int samplesAvailable = btAudioBufferCount;
  int samplesToCopy = min(samplesNeeded, samplesAvailable);

  if (samplesToCopy < samplesNeeded) {
    // Buffer underrun - riempi con silenzio quello che manca
    memset(frame, 0, frame_count * sizeof(Frame));
  }

  // Copia samples dal buffer circolare
  for (int i = 0; i < samplesToCopy / 2; i++) {
    frame[i].channel1 = btAudioBuffer[btAudioReadPos];
    btAudioReadPos = (btAudioReadPos + 1) % BT_AUDIO_BUFFER_SIZE;
    frame[i].channel2 = btAudioBuffer[btAudioReadPos];
    btAudioReadPos = (btAudioReadPos + 1) % BT_AUDIO_BUFFER_SIZE;
    btAudioBufferCount -= 2;
  }

  btAudioBytesSent += samplesToCopy * sizeof(int16_t);
  btAudioBufferLevel = (float)btAudioBufferCount / BT_AUDIO_BUFFER_SIZE * 100.0;

  return frame_count;
}

// Callback stato connessione A2DP
void btConnectionStateCallback(esp_a2d_connection_state_t state, void *ptr) {
  switch (state) {
    case ESP_A2D_CONNECTION_STATE_DISCONNECTED:
      Serial.println("[BT-A2DP] Disconnesso");
      btAudioConnected = false;
      btConnectedDevice = "";
      break;
    case ESP_A2D_CONNECTION_STATE_CONNECTING:
      Serial.println("[BT-A2DP] Connessione in corso...");
      break;
    case ESP_A2D_CONNECTION_STATE_CONNECTED:
      Serial.println("[BT-A2DP] Connesso!");
      btAudioConnected = true;
      break;
    case ESP_A2D_CONNECTION_STATE_DISCONNECTING:
      Serial.println("[BT-A2DP] Disconnessione in corso...");
      break;
  }
}
#endif // BT_A2DP_AVAILABLE

// ================== INIZIALIZZAZIONE BT AUDIO ==================
bool initBtAudio() {
#ifdef BT_A2DP_AVAILABLE
  Serial.println("[BT-A2DP] Inizializzazione Bluetooth A2DP Source...");

  // Alloca buffer audio in PSRAM
  if (btAudioBuffer == nullptr) {
    btAudioBuffer = (int16_t *)heap_caps_malloc(BT_AUDIO_BUFFER_SIZE * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (btAudioBuffer == nullptr) {
      Serial.println("[BT-A2DP] ERRORE: Impossibile allocare buffer audio!");
      return false;
    }
    memset(btAudioBuffer, 0, BT_AUDIO_BUFFER_SIZE * sizeof(int16_t));
    Serial.printf("[BT-A2DP] Buffer audio allocato: %d KB\n", (BT_AUDIO_BUFFER_SIZE * sizeof(int16_t)) / 1024);
  }

  // Reset posizioni buffer
  btAudioWritePos = 0;
  btAudioReadPos = 0;
  btAudioBufferCount = 0;

  // Configura A2DP source
  a2dp_source.set_auto_reconnect(true);
  a2dp_source.set_volume(100);  // Volume massimo (controllato dal dispositivo ricevente)

  btAudioEnabled = true;
  Serial.println("[BT-A2DP] Inizializzazione completata");
  Serial.println("[BT-A2DP] Usa la pagina web per connettere un dispositivo BT");

  return true;
#else
  Serial.println("[BT-A2DP] ATTENZIONE: Libreria ESP32-A2DP non installata!");
  Serial.println("[BT-A2DP] Installa da: https://github.com/pschatzmann/ESP32-A2DP");
  Serial.println("[BT-A2DP] Audio Bluetooth non disponibile");
  return false;
#endif
}

// ================== AVVIA STREAMING BT ==================
bool startBtAudioStream(const char* deviceName) {
#ifdef BT_A2DP_AVAILABLE
  if (!btAudioEnabled) {
    Serial.println("[BT-A2DP] Audio BT non inizializzato!");
    return false;
  }

  Serial.printf("[BT-A2DP] Connessione a: %s\n", deviceName);
  btTargetDevice = String(deviceName);

  // Avvia A2DP source con callback per dati audio
  a2dp_source.start(deviceName, btAudioDataCallback);
  a2dp_source.set_on_connection_state_changed(btConnectionStateCallback);

  // Attendi connessione (max 10 secondi)
  unsigned long startTime = millis();
  while (!btAudioConnected && millis() - startTime < 10000) {
    delay(100);
  }

  if (btAudioConnected) {
    btConnectedDevice = String(deviceName);
    btAudioStreaming = true;
    Serial.println("[BT-A2DP] Streaming audio avviato!");
    return true;
  } else {
    Serial.println("[BT-A2DP] Timeout connessione!");
    return false;
  }
#else
  return false;
#endif
}

// ================== FERMA STREAMING BT ==================
void stopBtAudioStream() {
#ifdef BT_A2DP_AVAILABLE
  if (btAudioStreaming) {
    Serial.println("[BT-A2DP] Stop streaming audio...");
    a2dp_source.end();
    btAudioStreaming = false;
    btAudioConnected = false;
    btConnectedDevice = "";
  }
#endif
}

// ================== CONNETTI AUDIO HTTP DAL SERVER ==================
bool connectBtAudioHttp(const String& baseUrl) {
  // Costruisci URL audio PCM sincronizzato
  String audioUrl = baseUrl + "/audio_pcm";

  Serial.printf("[BT-AUDIO] Connessione audio HTTP: %s\n", audioUrl.c_str());

  btAudioHttp.begin(btAudioClient, audioUrl);
  btAudioHttp.setTimeout(AUDIO_HTTP_TIMEOUT);

  int httpCode = btAudioHttp.GET();

  if (httpCode == HTTP_CODE_OK) {
    btAudioHttpConnected = true;
    Serial.println("[BT-AUDIO] Stream audio HTTP connesso!");
    return true;
  } else {
    Serial.printf("[BT-AUDIO] Errore HTTP: %d\n", httpCode);
    btAudioHttp.end();
    return false;
  }
}

// ================== DISCONNETTI AUDIO HTTP ==================
void disconnectBtAudioHttp() {
  if (btAudioHttpConnected) {
    btAudioHttp.end();
    btAudioHttpConnected = false;
    Serial.println("[BT-AUDIO] Stream audio HTTP disconnesso");
  }
}

// ================== RICEVI DATI AUDIO DAL SERVER ==================
// Chiamare regolarmente nel loop per riempire il buffer audio
void updateBtAudioReceive() {
  if (!btAudioHttpConnected || btAudioBuffer == nullptr) return;

  WiFiClient* stream = btAudioHttp.getStreamPtr();
  if (!stream || !stream->connected()) {
    btAudioHttpConnected = false;
    return;
  }

  // Leggi dati disponibili
  int available = stream->available();
  if (available > 0) {
    // Limita lettura per non bloccare troppo
    int toRead = min(available, 1024);

    // Leggi come bytes, converti in int16_t
    uint8_t tempBuffer[1024];
    int bytesRead = stream->readBytes(tempBuffer, toRead);

    // Converti e aggiungi al buffer circolare
    int16_t* samples = (int16_t*)tempBuffer;
    int sampleCount = bytesRead / sizeof(int16_t);

    for (int i = 0; i < sampleCount && btAudioBufferCount < BT_AUDIO_BUFFER_SIZE - 1; i++) {
      btAudioBuffer[btAudioWritePos] = samples[i];
      btAudioWritePos = (btAudioWritePos + 1) % BT_AUDIO_BUFFER_SIZE;
      btAudioBufferCount++;
    }

    btAudioBytesReceived += bytesRead;
  }
}

// ================== IMPOSTA TIMESTAMP VIDEO ==================
// Chiamata da MJPEG stream quando riceve un nuovo frame con timestamp
void setBtVideoTimestamp(unsigned long timestamp) {
  videoTimestamp = timestamp;

  // Calcola offset se abbiamo entrambi i timestamp
  if (audioTimestamp > 0 && videoTimestamp > 0) {
    audioVideoOffset = (int)audioTimestamp - (int)videoTimestamp;
  }
}

// ================== IMPOSTA TIMESTAMP AUDIO ==================
void setBtAudioTimestamp(unsigned long timestamp) {
  audioTimestamp = timestamp;
}

// ================== CLEANUP BT AUDIO ==================
void cleanupBtAudio() {
  Serial.println("[BT-A2DP] Cleanup...");

  stopBtAudioStream();
  disconnectBtAudioHttp();

  // Non liberiamo il buffer per riuso rapido
  btAudioEnabled = false;

  Serial.println("[BT-A2DP] Cleanup completato");
}

// ================== GET STATO BT AUDIO ==================
String getBtAudioStatus() {
  String status = "{";
  status += "\"enabled\":" + String(btAudioEnabled ? "true" : "false") + ",";
  status += "\"connected\":" + String(btAudioConnected ? "true" : "false") + ",";
  status += "\"streaming\":" + String(btAudioStreaming ? "true" : "false") + ",";
  status += "\"device\":\"" + btConnectedDevice + "\",";
  status += "\"bufferLevel\":" + String(btAudioBufferLevel, 1) + ",";
  status += "\"bytesReceived\":" + String(btAudioBytesReceived) + ",";
  status += "\"bytesSent\":" + String(btAudioBytesSent) + ",";
  status += "\"syncOffset\":" + String(audioVideoOffset);
  status += "}";
  return status;
}

// ================== SCAN DISPOSITIVI BT ==================
// Struttura per memorizzare dispositivi trovati (usata anche senza scan)
#define MAX_BT_DEVICES 15
struct BtDeviceInfo {
  String name;
  String address;
  int rssi;
  bool valid;
};
BtDeviceInfo btFoundDevices[MAX_BT_DEVICES];
volatile int btFoundDeviceCount = 0;
volatile bool btScanInProgress = false;
volatile bool btScanComplete = false;

#ifdef BT_SCAN_AVAILABLE
// BluetoothSerial per discovery
BluetoothSerial SerialBT;

// Task handle per scansione asincrona
TaskHandle_t btScanTaskHandle = NULL;

// Task per scansione BT in background (non blocca il web server)
void btScanTask(void* parameter) {
  Serial.println("[BT-SCAN] Task avviato");

  // Inizializza BluetoothSerial
  if (!SerialBT.begin("OraQuadra_Scan", true)) {  // true = master mode
    Serial.println("[BT-SCAN] Errore inizializzazione BluetoothSerial");
    btScanInProgress = false;
    btScanComplete = true;
    vTaskDelete(NULL);
    return;
  }
  Serial.println("[BT-SCAN] BluetoothSerial inizializzato");

  // Scansione sincrona
  Serial.println("[BT-SCAN] Avvio discovery BT Classic (10 sec)...");
  BTScanResults* results = SerialBT.discover(10000);  // 10 secondi

  if (results) {
    int count = results->getCount();
    Serial.printf("[BT-SCAN] Trovati %d dispositivi\n", count);

    for (int i = 0; i < count && btFoundDeviceCount < MAX_BT_DEVICES; i++) {
      BTAdvertisedDevice* device = results->getDevice(i);
      if (device) {
        String addr = device->getAddress().toString().c_str();
        String name = device->haveName() ? device->getName().c_str() : addr;
        int rssi = device->haveRSSI() ? device->getRSSI() : 0;

        btFoundDevices[btFoundDeviceCount].name = name;
        btFoundDevices[btFoundDeviceCount].address = addr;
        btFoundDevices[btFoundDeviceCount].rssi = rssi;
        btFoundDevices[btFoundDeviceCount].valid = true;
        btFoundDeviceCount++;

        Serial.printf("[BT-SCAN] %d: %s (%s) RSSI:%d\n",
                      i+1, name.c_str(), addr.c_str(), rssi);
      }
    }
  } else {
    Serial.println("[BT-SCAN] Nessun dispositivo trovato");
  }

  // Chiudi BluetoothSerial per liberare risorse
  SerialBT.end();

  btScanInProgress = false;
  btScanComplete = true;
  btScanTaskHandle = NULL;

  Serial.printf("[BT-SCAN] Scansione completata. Totale: %d dispositivi\n", btFoundDeviceCount);

  vTaskDelete(NULL);
}
#endif // BT_SCAN_AVAILABLE

// Avvia scansione dispositivi BT (asincrona)
bool startBtScan() {
#ifdef BT_SCAN_AVAILABLE
  if (btScanInProgress) {
    Serial.println("[BT-SCAN] Scansione gia in corso");
    return false;
  }

  Serial.println("[BT-SCAN] Avvio scansione dispositivi Bluetooth...");

  // Reset lista dispositivi
  btFoundDeviceCount = 0;
  btScanComplete = false;
  for (int i = 0; i < MAX_BT_DEVICES; i++) {
    btFoundDevices[i].valid = false;
    btFoundDevices[i].name = "";
    btFoundDevices[i].address = "";
    btFoundDevices[i].rssi = 0;
  }

  btScanInProgress = true;

  // Crea task per scansione in background
  BaseType_t result = xTaskCreatePinnedToCore(
    btScanTask,           // Funzione task
    "BT_Scan",            // Nome task
    8192,                 // Stack size
    NULL,                 // Parametri
    1,                    // Priorità
    &btScanTaskHandle,    // Handle
    0                     // Core 0 (lascia Core 1 per display)
  );

  if (result != pdPASS) {
    Serial.println("[BT-SCAN] Errore creazione task");
    btScanInProgress = false;
    return false;
  }

  Serial.println("[BT-SCAN] Task scansione avviato");
  return true;
#else
  Serial.println("[BT-SCAN] Bluetooth Classic non abilitato nella configurazione scheda");
  Serial.println("[BT-SCAN] Abilita 'Bluetooth Classic' in Tools menu Arduino IDE");
  return false;
#endif
}

// Ferma scansione
void stopBtScan() {
#ifdef BT_SCAN_AVAILABLE
  if (btScanTaskHandle != NULL) {
    vTaskDelete(btScanTaskHandle);
    btScanTaskHandle = NULL;
  }
  SerialBT.end();
#endif
  btScanInProgress = false;
}

// Restituisce lista JSON dei dispositivi BT trovati
String getBtDevicesJson() {
  String result = "[";
  bool first = true;

  for (int i = 0; i < btFoundDeviceCount; i++) {
    if (btFoundDevices[i].valid) {
      if (!first) result += ",";
      first = false;
      result += "{\"name\":\"" + btFoundDevices[i].name + "\",";
      result += "\"address\":\"" + btFoundDevices[i].address + "\",";
      result += "\"rssi\":" + String(btFoundDevices[i].rssi) + "}";
    }
  }

  result += "]";
  return result;
}

// Restituisce stato scansione
String getBtScanStatus() {
  String status = "{";
  status += "\"scanning\":" + String(btScanInProgress ? "true" : "false") + ",";
  status += "\"complete\":" + String(btScanComplete ? "true" : "false") + ",";
  status += "\"deviceCount\":" + String(btFoundDeviceCount) + ",";
  status += "\"devices\":" + getBtDevicesJson();
  status += "}";
  return status;
}

// HTML per pagina configurazione Bluetooth Audio (minificata)
const char BT_AUDIO_CONFIG_HTML[] PROGMEM =
"<!DOCTYPE html><html><head>"
"<meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>Bluetooth Audio</title><style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:sans-serif;background:#1a1a2e;min-height:100vh;padding:20px;color:#eee}"
".c{max-width:600px;margin:0 auto;background:rgba(255,255,255,.1);border-radius:20px;overflow:hidden}"
".h{background:linear-gradient(135deg,#9b59b6,#8e44ad);padding:25px;text-align:center}"
".h h1{font-size:1.5em;margin-bottom:5px}"
".ct{padding:25px}"
".s{background:rgba(0,0,0,.3);border-radius:15px;padding:20px;margin-bottom:20px}"
".s h3{margin-bottom:15px;color:#9b59b6}"
".ig{margin-bottom:15px}"
".ig label{display:block;margin-bottom:8px;opacity:.8;font-size:.9em}"
".ig input{width:100%;padding:12px;border:none;border-radius:8px;background:rgba(255,255,255,.1);color:#fff;font-size:1em}"
".b{width:100%;padding:15px;border:none;border-radius:10px;font-weight:bold;cursor:pointer;font-size:1em;margin-top:10px}"
".b1{background:linear-gradient(135deg,#9b59b6,#8e44ad);color:#fff}"
".b2{background:linear-gradient(135deg,#e74c3c,#c0392b);color:#fff}"
".b3{background:linear-gradient(135deg,#3498db,#2980b9);color:#fff}"
".st{display:flex;align-items:center;gap:10px;margin-bottom:15px}"
".st .dot{width:12px;height:12px;border-radius:50%}"
".st .on{background:#2ecc71;box-shadow:0 0 10px #2ecc71}"
".st .off{background:#e74c3c}"
".stats{font-size:.9em;opacity:.8;margin-top:10px}"
".msg{padding:10px;border-radius:8px;margin-top:10px;display:none}"
".msg.ok{background:rgba(46,204,113,.2);border:1px solid #2ecc71;display:block}"
".msg.err{background:rgba(231,76,60,.2);border:1px solid #e74c3c;display:block}"
".msg.info{background:rgba(52,152,219,.2);border:1px solid #3498db;display:block}"
".devlist{max-height:200px;overflow-y:auto;margin-top:10px}"
".dev{padding:10px;background:rgba(255,255,255,.1);border-radius:8px;margin-bottom:5px;cursor:pointer}"
".dev:hover{background:rgba(155,89,182,.3)}"
".dev .n{font-weight:bold}.dev .a{font-size:.8em;opacity:.6}"
".hm{display:block;text-align:center;color:#94a3b8;padding:10px;text-decoration:none;font-size:.9em}.hm:hover{color:#fff}"
"</style></head><body><div class=\"c\"><a href=\"/\" class=\"hm\">&larr; Home</a><div class=\"h\">"
"<h1>Bluetooth Audio</h1><p>Streaming audio sincronizzato</p></div><div class=\"ct\">"
"<div class=\"s\"><h3>Stato Connessione</h3>"
"<div class=\"st\"><div class=\"dot off\" id=\"dot\"></div><span id=\"status\">Non connesso</span></div>"
"<div id=\"stats\" class=\"stats\"></div></div>"
"<div class=\"s\"><h3>Cerca Dispositivi</h3>"
"<button class=\"b b3\" onclick=\"scan()\" id=\"scanBtn\">Cerca dispositivi BT</button>"
"<div id=\"scanMsg\" class=\"msg\"></div>"
"<div id=\"devlist\" class=\"devlist\"></div></div>"
"<div class=\"s\"><h3>Connetti Dispositivo</h3>"
"<div class=\"ig\"><label>Nome dispositivo Bluetooth</label>"
"<input type=\"text\" id=\"device\" placeholder=\"Seleziona dalla lista o scrivi...\"></div>"
"<button class=\"b b1\" onclick=\"conn()\">Connetti</button>"
"<button class=\"b b2\" onclick=\"disc()\" style=\"margin-top:10px\">Disconnetti</button>"
"<div id=\"msg\" class=\"msg\"></div></div>"
"</div></div><script>"
"var scanTimer=null;"
"function upd(){fetch('/btaudio/status').then(r=>r.json()).then(d=>{"
"document.getElementById('dot').className='dot '+(d.connected?'on':'off');"
"document.getElementById('status').textContent=d.connected?'Connesso a '+d.device:'Non connesso';"
"document.getElementById('stats').innerHTML="
"'Buffer: '+d.bufferLevel.toFixed(1)+'%% | '+"
"'Ricevuti: '+(d.bytesReceived/1024).toFixed(1)+'KB | '+"
"'Inviati: '+(d.bytesSent/1024).toFixed(1)+'KB | '+"
"'Sync: '+d.syncOffset+'ms';"
"}).catch(()=>{})}"
"function scan(){var m=document.getElementById('scanMsg'),b=document.getElementById('scanBtn');"
"m.className='msg info';m.textContent='Scansione in corso (10 sec)...';"
"b.disabled=true;b.textContent='Scansione...';"
"fetch('/btaudio/scan',{method:'POST'}).then(r=>r.json()).then(d=>{"
"if(d.success){scanTimer=setInterval(chkScan,1000)}"
"else{m.className='msg err';m.textContent='Errore: '+d.message;b.disabled=false;b.textContent='Cerca dispositivi BT'}"
"}).catch(()=>{m.className='msg err';m.textContent='Errore';b.disabled=false;b.textContent='Cerca dispositivi BT'})}"
"function chkScan(){fetch('/btaudio/scan/status').then(r=>r.json()).then(d=>{"
"var m=document.getElementById('scanMsg'),b=document.getElementById('scanBtn'),l=document.getElementById('devlist');"
"if(d.complete||!d.scanning){clearInterval(scanTimer);b.disabled=false;b.textContent='Cerca dispositivi BT';"
"m.className='msg ok';m.textContent='Trovati '+d.deviceCount+' dispositivi'}"
"else{m.textContent='Scansione... trovati '+d.deviceCount}"
"l.innerHTML='';for(var i=0;i<d.devices.length;i++){var dev=d.devices[i];"
"l.innerHTML+='<div class=\"dev\" onclick=\"selDev(\\''+dev.name+'\\'\"><span class=\"n\">'+dev.name+'</span><br><span class=\"a\">'+dev.address+' ('+dev.rssi+' dBm)</span></div>'}"
"}).catch(()=>{})}"
"function selDev(n){document.getElementById('device').value=n}"
"function conn(){var n=document.getElementById('device').value,m=document.getElementById('msg');"
"if(!n){m.className='msg err';m.textContent='Inserisci nome dispositivo';return}"
"m.className='msg info';m.textContent='Connessione...';"
"fetch('/btaudio/connect',{method:'POST',headers:{'Content-Type':'application/json'},"
"body:JSON.stringify({device:n})}).then(r=>r.json()).then(d=>{"
"if(d.success){m.className='msg ok';m.textContent='Connesso!'}"
"else{m.className='msg err';m.textContent='Errore: '+d.message}}).catch(()=>{"
"m.className='msg err';m.textContent='Errore connessione'})}"
"function disc(){fetch('/btaudio/disconnect',{method:'POST'}).then(r=>r.json()).then(d=>{"
"var m=document.getElementById('msg');"
"if(d.success){m.className='msg ok';m.textContent='Disconnesso'}"
"else{m.className='msg err';m.textContent='Errore'}}).catch(()=>{})}"
"setInterval(upd,1000);upd();"
"</script></body></html>";

// ================== SETUP WEB SERVER BT AUDIO ==================
void setup_bt_audio_webserver(AsyncWebServer* server) {
  Serial.println("[BT-AUDIO] Configurazione endpoints web...");

  // Pagina configurazione Bluetooth Audio
  server->on("/btaudio", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", BT_AUDIO_CONFIG_HTML);
  });

  // API stato
  server->on("/btaudio/status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", getBtAudioStatus());
  });

  // API connetti
  server->on("/btaudio/connect", HTTP_POST, [](AsyncWebServerRequest *request){},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String body = "";
      for (size_t i = 0; i < len; i++) body += (char)data[i];

      // Parse device name
      int devStart = body.indexOf("\"device\":\"");
      if (devStart >= 0) {
        devStart += 10;
        int devEnd = body.indexOf("\"", devStart);
        if (devEnd > devStart) {
          String deviceName = body.substring(devStart, devEnd);

          // Inizializza BT se necessario
          if (!btAudioEnabled) {
            initBtAudio();
          }

          if (startBtAudioStream(deviceName.c_str())) {
            request->send(200, "application/json", "{\"success\":true}");
          } else {
            request->send(200, "application/json", "{\"success\":false,\"message\":\"Connessione fallita\"}");
          }
          return;
        }
      }
      request->send(400, "application/json", "{\"success\":false,\"message\":\"Device name required\"}");
    });

  // API disconnetti
  server->on("/btaudio/disconnect", HTTP_POST, [](AsyncWebServerRequest *request){
    stopBtAudioStream();
    request->send(200, "application/json", "{\"success\":true}");
  });

  // API avvia scansione dispositivi BT
  server->on("/btaudio/scan", HTTP_POST, [](AsyncWebServerRequest *request){
    if (btScanInProgress) {
      request->send(200, "application/json", "{\"success\":false,\"message\":\"Scansione gia in corso\"}");
      return;
    }

    if (startBtScan()) {
      request->send(200, "application/json", "{\"success\":true,\"message\":\"Scansione avviata\"}");
    } else {
      request->send(200, "application/json", "{\"success\":false,\"message\":\"Errore avvio scansione\"}");
    }
  });

  // API stato scansione e lista dispositivi
  server->on("/btaudio/scan/status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", getBtScanStatus());
  });

  // API ferma scansione
  server->on("/btaudio/scan/stop", HTTP_POST, [](AsyncWebServerRequest *request){
    stopBtScan();
    request->send(200, "application/json", "{\"success\":true}");
  });

  Serial.println("[BT-AUDIO] Endpoints web configurati su /btaudio");
}

#endif // EFFECT_MJPEG_STREAM
