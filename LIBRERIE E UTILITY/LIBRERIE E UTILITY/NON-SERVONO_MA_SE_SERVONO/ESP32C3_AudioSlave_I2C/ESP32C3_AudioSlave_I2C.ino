/*
 * ORAQUADRA NANO - ESP32C3 AUDIO SLAVE MODULE (I2C Version)
 *
 * Questo modulo gestisce ESCLUSIVAMENTE l'audio, ricevendo comandi dall'ESP32S3
 * tramite I2C condiviso (stesso bus del touchscreen GT911).
 *
 * Hardware:
 * - ESP32C3 (qualsiasi modulo con almeno 4MB flash)
 * - Amplificatore I2S (MAX98357A o compatibile)
 *
 * Connessioni I2C (con ESP32S3):
 * - SDA: Pin 8 → ESP32S3 Pin 19 (condiviso con GT911 touchscreen)
 * - SCL: Pin 9 → ESP32S3 Pin 45 (condiviso con GT911 touchscreen)
 * - GND: Comune con ESP32S3
 * - Indirizzo I2C Slave: 0x08
 *
 * Connessioni I2S (MAX98357A):
 * - BCLK: Pin 4
 * - LRC:  Pin 5
 * - DOUT: Pin 6
 * - SD (Enable): Pin 7
 *
 * By Paolo Sambinello - 2025
 */

// ================== LIBRERIE ==================
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <AudioFileSourceSPIFFS.h>
#include <AudioFileSourceHTTPStream.h>  // Per streaming HTTP semplice
#include <AudioFileSourceICYStream.h>   // Per streaming ICY/Shoutcast
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

// ================== CONFIGURAZIONE WIFI ==================
// Stesse credenziali dell'ESP32S3 principale
const char* WIFI_SSID = "SambinelloLan";
const char* WIFI_PASSWORD = "Smbpla62h02l872U=";

// ================== CONFIGURAZIONE I2C ==================
#define I2C_SDA_PIN       10     // Pin SDA per I2C slave
#define I2C_SCL_PIN       8      // Pin SCL per I2C slave
#define I2C_SLAVE_ADDR    0x08   // Indirizzo I2C slave

// ================== CONFIGURAZIONE PIN I2S ==================
#define I2S_BCLK        4
#define I2S_LRC         5
#define I2S_DOUT        6
#define I2S_ENABLE      7     // Pin di abilitazione amplificatore

// LED di stato
#define LED_STATUS      2     // LED integrato ESP32C3

// ================== CONFIGURAZIONE AUDIO ==================
#define VOLUME_LEVEL    0.85  // Volume da 0.0 a 1.0 (ridotto per evitare clipping/distorsione)
#define MAX_FILENAME    64
#define AUDIO_LOOP_YIELD_INTERVAL  200  // Chiamata yield ogni 200 iterazioni (riduce interruzioni audio)
#define AUDIO_TIMEOUT_MS          30000 // Timeout massimo per un file audio (30s)

// ================== COMANDI I2C ==================
#define CMD_ANNOUNCE_TIME    0x01  // Annuncia ora: [CMD, hour, minute]
#define CMD_SET_VOLUME       0x03  // Imposta volume: [CMD, volume(0-100)]
#define CMD_STOP_AUDIO       0x04  // Ferma audio
#define CMD_PLAY_FILE        0x05  // Riproduci file: [CMD, fileID]
#define CMD_PING             0x06  // Ping per verificare connessione
#define CMD_PLAY_SEQUENCE    0x07  // Riproduci sequenza: [CMD, count, file1, file2, ...]
#define CMD_BOOT_ANNOUNCE    0x08  // Annuncio boot: [CMD, day_of_week(0=Dom-6=Sab), day(1-31), month(1-12), hour, minute, year_high, year_low]
#define CMD_PLAY_BEEP        0x09  // Riproduci beep singolo
#define CMD_PLAY_CLACK       0x0A  // Riproduci clack flip clock
#define CMD_SET_AUDIO_ENABLE 0x0B  // Abilita/disabilita audio: [CMD, enable(1/0)]
#define CMD_SPEAK_TEXT       0x0C  // Parla testo generico via Google TTS: [CMD, textLen, text...]
#define CMD_ASK_GEMINI       0x0D  // Domanda a Gemini AI: [CMD, questionLen, question...]
#define CMD_VOICE_START      0x0E  // Inizia registrazione vocale dal microfono
#define CMD_VOICE_STOP       0x0F  // Ferma registrazione e processa (STT → Gemini → TTS)
#define CMD_VOICE_ENABLE     0x10  // Abilita voice assistant (solo quando in MODE_GEMINI_AI)
#define CMD_VOICE_DISABLE    0x11  // Disabilita voice assistant (quando si esce da MODE_GEMINI_AI)
#define CMD_PLAY_STREAM      0x12  // Avvia streaming audio da URL: [CMD, urlLen, url...]
#define CMD_STOP_STREAM      0x13  // Ferma streaming audio

// ================== RISPOSTE I2C ==================
#define RESP_PONG            0xFF  // Risposta a CMD_PING
#define RESP_AUDIO_COMPLETED 0xAA  // Segnale che l'audio è terminato

// ================== VARIABILI GLOBALI ==================
AudioOutputI2S *output = nullptr;
AudioGeneratorMP3 *mp3 = nullptr;
AudioFileSourceSPIFFS *file = nullptr;
AudioFileSourceHTTPStream *streamSource = nullptr;  // Per streaming audio HTTP
AudioFileSourceBuffer *buff = nullptr;
bool isStreaming = false;  // Flag per streaming audio attivo
String currentStreamUrl = "";  // URL streaming corrente
int currentStreamBufferSize = 8192;  // Dimensione buffer streaming corrente

bool isPlaying = false;
volatile bool interruptAudioRequested = false;  // Flag per interrompere audio in corso
volatile bool forceInterruptNow = false;        // 🔥 Flag per interruzione IMMEDIATA aggressiva

// Buffer I2C per comandi
#define I2C_BUFFER_SIZE 256
volatile uint8_t i2cCommandBuffer[I2C_BUFFER_SIZE];
volatile int i2cCommandLength = 0;
volatile bool i2cCommandReady = false;

// ================== SISTEMA LOG DEBUG (per debug senza serial) ==================
#define MAX_DEBUG_LOGS 50  // Ultimi 50 log salvati in memoria
String debugLogs[MAX_DEBUG_LOGS];
int debugLogIndex = 0;
unsigned long debugLogTimestamps[MAX_DEBUG_LOGS];

bool isAnnouncingBoot = false;  // Flag per proteggere boot announce da interruzioni
bool audioEnabled = true;
float currentVolume = VOLUME_LEVEL;
uint8_t lastHour = 255;
uint8_t lastMinute = 255;

// Tutte le funzionalità Gemini AI e Voice Assistant sono state rimosse per ridurre dimensioni firmware

// ================== MAPPATURA FILE AUDIO ==================
// Nomi dei file MP3 per i numeri
const char* numberFiles[] = {
  "0.mp3", "1.mp3", "2.mp3", "3.mp3", "4.mp3", "5.mp3",
  "6.mp3", "7.mp3", "8.mp3", "9.mp3", "10.mp3", "11.mp3",
  "T-12.mp3", "13.mp3", "14.mp3", "15.mp3", "16.mp3", "17.mp3",
  "18.mp3", "19.mp3", "20.mp3", "21.mp3", "22.mp3", "23.mp3",
  "24.mp3", "25.mp3", "26.mp3", "27.mp3", "28.mp3", "29.mp3",
  "30.mp3", "31.mp3", "32.mp3", "33.mp3", "34.mp3", "35.mp3",
  "36.mp3", "37.mp3", "38.mp3", "39.mp3", "40.mp3", "41.mp3",
  "42.mp3", "43.mp3", "44.mp3", "45.mp3", "46.mp3", "47.mp3",
  "48.mp3", "49.mp3", "50.mp3", "51.mp3", "52.mp3", "53.mp3",
  "54.mp3", "55.mp3", "56.mp3", "57.mp3", "58.mp3", "59.mp3"
};

// Parole speciali
const char* WORD_SONO_LE = "sonole.mp3";
const char* WORD_E = "e.mp3";
const char* WORD_MINUTI = "minuti.mp3";
const char* WORD_MEZZANOTTE = "mezzanotte.mp3";
const char* WORD_MEZZOGIORNO = "mezzogiorno.mp3";
const char* WORD_BEEP = "beep.mp3";
const char* WORD_CLACK = "clack.mp3";  // Suono flip clock (MAIUSCOLO)

// Parole per boot announce
//const char* WORD_CIAO = "ciao.mp3";
//const char* WORD_SONO = "sono.mp3";
const char* WORD_ORAQUADRA = "saluto.mp3";
//const char* WORD_NANO = "nano.mp3";
const char* WORD_OGGI = "oggieilxx.mp3";
const char* WORD_ORE = "ore.mp3";

// Giorni della settimana (0=Domenica, 1=Lunedì, ..., 6=Sabato)
const char* daysOfWeek[] = {
  "domenica.mp3",
  "lunedi.mp3",
  "martedi.mp3",
  "mercoledi.mp3",
  "giovedi.mp3",
  "venerdi.mp3",
  "sabato.mp3"
};

// Mesi (1=Gennaio, ..., 12=Dicembre)
const char* months[] = {
  "",  // Index 0 non usato
  "gennaio.mp3",
  "febbraio.mp3",
  "marzo.mp3",
  "aprile.mp3",
  "maggio.mp3",
  "giugno.mp3",
  "luglio.mp3",
  "agosto.mp3",
  "settembre.mp3",
  "ottobre.mp3",
  "novembre.mp3",
  "dicembre.mp3"
};

// ================== PROTOTIPI FUNZIONI ==================
void setupI2C();
void setupAudio();
void setupSPIFFS();
void onI2CReceive(int numBytes);
void onI2CRequest();
void processI2CCommand(uint8_t* buffer, int length);
bool playLocalMP3(const char* filename);
bool playAudioWithTimeout(unsigned long timeoutMs);
bool announceTime(uint8_t hour, uint8_t minute);
bool announceBootMessage(uint8_t dayOfWeek, uint8_t day, uint8_t month, uint8_t hour, uint8_t minute, uint16_t year);
void cleanupAudio(bool destroyOutput = true);
void setVolume(uint8_t volume);
void blinkLED(int times);
void printHeapStatus();
void addDebugLog(const String& message);
bool startAudioStream(const char* url);
void stopAudioStream();
void setupWiFi();

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  delay(2000); // Attendi stabilizzazione seriale

  Serial.println("\n\n=================================");
  Serial.println("ORAQUADRA NANO - ESP32C3 AUDIO SLAVE");
  Serial.println("     I2C VERSION - FIRMWARE V3.0");
  Serial.println("     Audio Playback Only");
  Serial.println("=================================");
  Serial.printf("ESP32 Chip ID: %llX\n", ESP.getEfuseMac());
  Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
  Serial.printf("Free Heap: %d bytes\n\n", ESP.getFreeHeap());

  // LED di stato
  Serial.println("[1/4] Inizializzazione LED...");
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_STATUS, LOW);
  Serial.println("      OK - LED pronto");

  // Pin enable amplificatore
  Serial.println("[2/4] Inizializzazione pin amplificatore...");
  pinMode(I2S_ENABLE, OUTPUT);
  digitalWrite(I2S_ENABLE, LOW);
  Serial.println("      OK - Pin amplificatore configurato");

  // Inizializza SPIFFS
  Serial.println("[3/4] Inizializzazione SPIFFS...");
  setupSPIFFS();
  Serial.println("      OK - SPIFFS inizializzato");

  // Inizializza audio
  Serial.println("[4/4] Inizializzazione audio I2S...");
  setupAudio();
  Serial.println("      OK - Audio I2S pronto");

  // Connetti WiFi PRIMA di I2C (così il master non attende durante il WiFi connect)
  Serial.println("[5/6] Connessione WiFi...");
  setupWiFi();

  // Inizializza I2C Slave (DOPO WiFi, così è subito pronto a ricevere comandi)
  Serial.println("[6/6] Inizializzazione I2C Slave...");
  setupI2C();
  Serial.println("      OK - I2C Slave attivo");

  Serial.println("\n=================================");
  Serial.println("✓ SISTEMA PRONTO E OPERATIVO");
  Serial.println("=================================");
  Serial.println("Comunicazione I2C:");
  Serial.printf("  Indirizzo Slave: 0x%02X\n", I2C_SLAVE_ADDR);
  Serial.printf("  SDA Pin: %d (→ S3 Pin 19)\n", I2C_SDA_PIN);
  Serial.printf("  SCL Pin: %d (→ S3 Pin 45)\n", I2C_SCL_PIN);
  Serial.println("\nCapacità:");
  Serial.println("  ✓ Annunci orari vocali");
  Serial.println("  ✓ Riproduzione MP3 da SPIFFS");
  Serial.println("  ✓ Beep e Clack suoni");
  Serial.println("  ✓ Comunicazione I2C affidabile");
  Serial.println("=================================\n");

  Serial.println(">>> FIRMWARE V3.0 - I2C Audio Playback <<<");
  blinkLED(3); // Lampeggia 3 volte per indicare che è pronto
}

// ================== LOOP PRINCIPALE ==================
void loop() {
  // Processa comandi I2C ricevuti
  if (i2cCommandReady) {
    // Copia comando in buffer locale (interruzioni I2C possono arrivare durante il processing)
    uint8_t localBuffer[I2C_BUFFER_SIZE];
    int localLength = i2cCommandLength;
    memcpy(localBuffer, (const uint8_t*)i2cCommandBuffer, localLength);

    // Reset flag PRIMA del processing per non perdere nuovi comandi
    i2cCommandReady = false;
    i2cCommandLength = 0;

    // Processa comando
    processI2CCommand(localBuffer, localLength);
  }

  // Mantieni attiva la riproduzione audio (file locale o streaming)
  if (mp3 && mp3->isRunning()) {
    if (!mp3->loop()) {
      if (isStreaming) {
        // Streaming terminato o errore
        Serial.println("Streaming audio terminato/errore");
        stopAudioStream();
      } else {
        mp3->stop();
        cleanupAudio();
        Serial.println("Riproduzione completata");
        blinkLED(1);
      }
    }
  }

  // Yield per watchdog - CRITICO
  yield();

  // Durante streaming: nessun delay per massima fluidità
  // Durante idle: delay minimo per non saturare CPU
  if (!isStreaming) {
    delay(1);
  }
}

// ================== CONFIGURAZIONE I2C SLAVE ==================
void setupI2C() {
  // Inizializza I2C come SLAVE
  // IMPORTANTE: Per modalità slave, l'ordine è: (slaveAddr, sda, scl, frequency)
  // frequency=0 usa il default
  Wire.begin((uint8_t)I2C_SLAVE_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, 0);

  // Registra callback per ricezione comandi dal master
  Wire.onReceive(onI2CReceive);

  // Registra callback per richieste del master (es. PING → PONG)
  Wire.onRequest(onI2CRequest);

  Serial.println("I2C SLAVE MODE configurato:");
  Serial.printf("  Indirizzo: 0x%02X\n", I2C_SLAVE_ADDR);
  Serial.printf("  SDA: Pin %d\n", I2C_SDA_PIN);
  Serial.printf("  SCL: Pin %d\n", I2C_SCL_PIN);
}

// ================== CALLBACK I2C: RICEZIONE COMANDI ==================
void onI2CReceive(int numBytes) {
  if (numBytes == 0 || numBytes > I2C_BUFFER_SIZE) {
    Serial.printf("ERRORE: Dimensione comando I2C non valida: %d bytes\n", numBytes);
    return;
  }

  // Leggi comando in buffer volatile
  i2cCommandLength = 0;
  while (Wire.available() && i2cCommandLength < I2C_BUFFER_SIZE) {
    i2cCommandBuffer[i2cCommandLength++] = Wire.read();
  }

  // Segnala che comando è pronto
  i2cCommandReady = true;

  // Debug immediato (nella ISR)
  Serial.printf("\n📦 I2C RX: %d bytes | CMD: 0x%02X\n", i2cCommandLength, i2cCommandBuffer[0]);
}

// ================== CALLBACK I2C: RICHIESTE DAL MASTER ==================
void onI2CRequest() {
  // Il master chiede una risposta (es. PONG per CMD_PING)
  // Invia RESP_PONG
  Wire.write(RESP_PONG);
  Serial.println("I2C TX: PONG (0xFF)");
}

// ================== PROCESSAMENTO COMANDI I2C ==================
void processI2CCommand(uint8_t* buffer, int length) {
  if (length < 1) {
    Serial.println("ERRORE: Comando I2C troppo corto");
    return;
  }

  // Lampeggia LED per segnalare ricezione comando
  digitalWrite(LED_STATUS, HIGH);

  // LOG comando ricevuto
  Serial.print(">>> I2C CMD: ");
  for (int i = 0; i < length; i++) {
    Serial.printf("0x%02X ", buffer[i]);
  }
  Serial.println();

  uint8_t cmd = buffer[0];

  switch (cmd) {
    case CMD_PING:
      Serial.println(">>> CMD_PING (risposta gestita da onRequest)");
      // La risposta PONG è gestita automaticamente da onI2CRequest()
      break;

    case CMD_ANNOUNCE_TIME:
      if (length >= 3) {
        uint8_t hour = buffer[1];
        uint8_t minute = buffer[2];
        Serial.printf(">>> CMD_ANNOUNCE_TIME: %02d:%02d\n", hour, minute);
        announceTime(hour, minute);
      } else {
        Serial.println("ERRORE: CMD_ANNOUNCE_TIME - dati insufficienti");
      }
      break;

    case CMD_SET_VOLUME:
      if (length >= 2) {
        uint8_t volume = buffer[1];
        Serial.printf(">>> CMD_SET_VOLUME: %d%%\n", volume);
        setVolume(volume);
      } else {
        Serial.println("ERRORE: CMD_SET_VOLUME - dati insufficienti");
      }
      break;

    case CMD_STOP_AUDIO:
      Serial.println(">>> CMD_STOP_AUDIO");
      if (mp3) {
        mp3->stop();
        cleanupAudio();
      }
      break;

    case CMD_PLAY_FILE:
      if (length >= 2) {
        uint8_t fileID = buffer[1];
        Serial.printf(">>> CMD_PLAY_FILE: ID %d\n", fileID);
        if (fileID < 60) {
          playLocalMP3(numberFiles[fileID]);
        }
      } else {
        Serial.println("ERRORE: CMD_PLAY_FILE - dati insufficienti");
      }
      break;

    case CMD_BOOT_ANNOUNCE:
      if (length >= 8) {
        uint8_t dayOfWeek = buffer[1];  // 0-6 (0=Domenica)
        uint8_t day = buffer[2];        // 1-31
        uint8_t month = buffer[3];      // 1-12
        uint8_t hour = buffer[4];       // 0-23
        uint8_t minute = buffer[5];     // 0-59
        uint8_t yearHigh = buffer[6];   // Anno byte alto
        uint8_t yearLow = buffer[7];    // Anno byte basso
        uint16_t year = (yearHigh << 8) | yearLow;

        Serial.printf(">>> CMD_BOOT_ANNOUNCE: %s %d %s %d, %02d:%02d\n",
                      daysOfWeek[dayOfWeek],
                      day,
                      months[month],
                      year,
                      hour,
                      minute);
        announceBootMessage(dayOfWeek, day, month, hour, minute, year);
      } else {
        Serial.println("ERRORE: CMD_BOOT_ANNOUNCE - dati insufficienti");
      }
      break;

    case CMD_PLAY_BEEP:
      Serial.println(">>> CMD_PLAY_BEEP");
      if (isPlaying) {
        Serial.println("⚠️ Beep già in corso, interrompo e riavvio");
        cleanupAudio(false);  // Mantieni output I2S
      }
      playLocalMP3(WORD_BEEP);
      delay(30); // Attendi che il decoder parta
      break;

    case CMD_PLAY_CLACK:
      Serial.println(">>> CMD_PLAY_CLACK");
      if (isPlaying) {
        Serial.println("⚠️ Clack già in corso, interrompo e riavvio");
        cleanupAudio(false);  // Mantieni output I2S
      }
      playLocalMP3(WORD_CLACK);
      delay(30); // Attendi che il decoder parta
      break;

    case CMD_SET_AUDIO_ENABLE:
      if (length >= 2) {
        bool enable = buffer[1] != 0;
        audioEnabled = enable;
        Serial.printf(">>> CMD_SET_AUDIO_ENABLE: %s\n", enable ? "ABILITATO" : "DISABILITATO");
      } else {
        Serial.println("ERRORE: CMD_SET_AUDIO_ENABLE richiede 1 parametro");
      }
      break;

    case CMD_PLAY_STREAM:
      if (length >= 3) {
        uint8_t urlLen = buffer[1];
        if (urlLen > 0 && length >= 2 + urlLen) {
          char url[256];
          memcpy(url, &buffer[2], min((int)urlLen, 255));
          url[min((int)urlLen, 255)] = '\0';
          Serial.printf(">>> CMD_PLAY_STREAM: %s\n", url);
          startAudioStream(url);
        } else {
          Serial.println("ERRORE: CMD_PLAY_STREAM - URL troppo corto");
        }
      } else {
        Serial.println("ERRORE: CMD_PLAY_STREAM - dati insufficienti");
      }
      break;

    case CMD_STOP_STREAM:
      Serial.println(">>> CMD_STOP_STREAM");
      stopAudioStream();
      break;

    default:
      Serial.printf("ERRORE: Comando I2C sconosciuto 0x%02X\n", cmd);
      break;
  }

  digitalWrite(LED_STATUS, LOW);
}

// ================== CONFIGURAZIONE SPIFFS ==================
void setupSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("ERRORE: Montaggio SPIFFS fallito!");
    return;
  }

  Serial.printf("SPIFFS montato correttamente\n");
  Serial.printf("Spazio totale: %d bytes\n", SPIFFS.totalBytes());
  Serial.printf("Spazio usato: %d bytes\n\n", SPIFFS.usedBytes());

  // Elenca file MP3 disponibili
  Serial.println("File MP3 trovati:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  int count = 0;
  while (file) {
    if (!file.isDirectory()) {
      String filename = file.name();
      if (filename.endsWith(".mp3")) {
        Serial.printf("  - %s (%d bytes)\n", filename.c_str(), file.size());
        count++;
      }
    }
    file = root.openNextFile();
  }
  Serial.printf("Totale: %d file MP3\n", count);
}

// ================== CONFIGURAZIONE AUDIO I2S ==================
void setupAudio() {
  // Crea output I2S
  output = new AudioOutputI2S();
  output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  output->SetGain(currentVolume);

  Serial.println("Audio I2S configurato:");
  Serial.printf("  BCLK: Pin %d\n", I2S_BCLK);
  Serial.printf("  LRC:  Pin %d\n", I2S_LRC);
  Serial.printf("  DOUT: Pin %d\n", I2S_DOUT);
  Serial.printf("  Enable: Pin %d\n", I2S_ENABLE);
  Serial.printf("  Volume: %.2f\n", currentVolume);
}

// ================== RIPRODUZIONE MP3 ==================
// NOTA: Questa funzione è ASINCRONA - non aspetta la fine della riproduzione
// Il loop() principale gestisce mp3->loop() per continuare la riproduzione
bool playLocalMP3(const char* filename) {
  Serial.printf("\n🎵🎵🎵 playLocalMP3() CHIAMATO: %s (heap: %d) 🎵🎵🎵\n", filename, ESP.getFreeHeap());

  if (!audioEnabled) {
    Serial.println("❌ Audio disabilitato");
    return false;
  }

  // 🔧 OTTIMIZZAZIONE: Cleanup "soft" mantiene output I2S per file consecutivi
  Serial.println("  🧹 Pre-cleanup...");
  cleanupAudio(false);  // NON distruggere output tra file MP3

  Serial.printf("  📂 Apertura file: /%s\n", filename);

  // 🔧 CRITICO: Ricrea output se è stato liberato
  if (!output) {
    Serial.println("🔄 Output I2S null - ricreo");
    output = new AudioOutputI2S();
    if (!output) {
      Serial.println("ERRORE: Impossibile allocare output I2S");
      return false;
    }
    output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    output->SetGain(currentVolume);
    Serial.println("✅ Output I2S ricreato");
  }

  // Verifica esistenza file
  String fullPath = String("/") + filename;
  if (!SPIFFS.exists(fullPath)) {
    Serial.printf("ERRORE: File non trovato: %s\n", fullPath.c_str());
    return false;
  }

  // Abilita amplificatore
  digitalWrite(I2S_ENABLE, HIGH);

  // Crea sorgente file
  file = new AudioFileSourceSPIFFS(fullPath.c_str());
  if (!file) {
    Serial.println("ERRORE: Impossibile aprire file");
    digitalWrite(I2S_ENABLE, LOW);
    return false;
  }

  // Crea buffer per streaming fluido (4KB - aumentato per ridurre gracchiamenti)
  buff = new AudioFileSourceBuffer(file, 4096);
  if (!buff) {
    Serial.println("ERRORE: Impossibile creare buffer");
    delete file;
    file = nullptr;
    digitalWrite(I2S_ENABLE, LOW);
    return false;
  }

  // Crea decoder MP3
  mp3 = new AudioGeneratorMP3();
  if (!mp3) {
    Serial.println("ERRORE: Impossibile creare decoder MP3");
    delete buff;
    delete file;
    buff = nullptr;
    file = nullptr;
    digitalWrite(I2S_ENABLE, LOW);
    return false;
  }

  // Avvia riproduzione
  if (!mp3->begin(buff, output)) {
    Serial.println("ERRORE: Impossibile avviare riproduzione");
    delete mp3;
    delete buff;
    delete file;
    mp3 = nullptr;
    buff = nullptr;
    file = nullptr;
    digitalWrite(I2S_ENABLE, LOW);
    return false;
  }

  isPlaying = true;
  Serial.println("Riproduzione avviata!");
  return true;
}

// ================== AUDIO PLAYBACK WITH TIMEOUT ==================
bool playAudioWithTimeout(unsigned long timeoutMs) {
  if (!mp3 || !mp3->isRunning()) {
    addDebugLog("playAudio: Decoder NULL o non running");
    return false;
  }

  unsigned long startTime = millis();
  unsigned int loopCount = 0;
  unsigned int totalLoops = 0;

  addDebugLog("playAudio: Inizio loop riproduzione");

  // Loop principale con gestione watchdog e timeout
  while (mp3 && mp3->isRunning()) {
    // Controlla timeout
    if (millis() - startTime > timeoutMs) {
      Serial.println("⚠️ TIMEOUT: Riproduzione interrotta!");
      addDebugLog("playAudio: TIMEOUT dopo " + String(totalLoops) + " loops");
      cleanupAudio();
      return false;
    }

    // Processa audio
    if (!mp3->loop()) {
      // Fine del file MP3
      addDebugLog("playAudio: Fine file dopo " + String(totalLoops) + " loops");
      Serial.printf("✓ File completato (Heap: %d bytes)\n", ESP.getFreeHeap());
      return true;
    }

    totalLoops++;

    // Ogni N iterazioni, gestisci watchdog (NO comandi I2C durante audio!)
    loopCount++;
    if (loopCount >= AUDIO_LOOP_YIELD_INTERVAL) {
      loopCount = 0;

      // Yield per watchdog - CRITICO per ESP32-C3!
      yield();
      delay(1);  // Yield extra per evitare watchdog reset
    }
  }

  return true;
}

// ================== ANNUNCIO ORARIO ==================
bool announceTime(uint8_t hour, uint8_t minute) {
  Serial.printf("\n=== ANNUNCIO ORARIO: %02d:%02d ===\n", hour, minute);

  lastHour = hour;
  lastMinute = minute;

  // Array di file da riprodurre in sequenza
  const char* sequence[10];
  int seqLen = 0;

  // 1. "BEEP"
  sequence[seqLen++] = WORD_BEEP;

  // 2. Caso speciale mezzanotte (00:00)
  if (hour == 0 && minute == 0) {
    sequence[seqLen++] = WORD_MEZZANOTTE;
  }
  // 3. Mezzanotte con minuti (00:01-00:59)
  else if (hour == 0 && minute > 0) {
    sequence[seqLen++] = WORD_MEZZANOTTE;
    sequence[seqLen++] = WORD_E;
    if (minute < 60) {
      sequence[seqLen++] = numberFiles[minute];
    }
    sequence[seqLen++] = WORD_MINUTI;
  }
  // 4. Caso speciale mezzogiorno (12:00)
  else if (hour == 12 && minute == 0) {
    sequence[seqLen++] = WORD_MEZZOGIORNO;
  }
  // 5. Mezzogiorno con minuti (12:01-12:59)
  else if (hour == 12 && minute > 0) {
    sequence[seqLen++] = WORD_MEZZOGIORNO;
    sequence[seqLen++] = WORD_E;
    if (minute < 60) {
      sequence[seqLen++] = numberFiles[minute];
    }
    sequence[seqLen++] = WORD_MINUTI;
  }
  // 6. Orario normale (1-11, 13-23) - con "sono le"
  else {
    sequence[seqLen++] = WORD_SONO_LE;

    // Ora - usa numberFiles solo per ore 1-11 e 13-23
    if (hour > 0 && hour < 60 && hour != 12) {
      sequence[seqLen++] = numberFiles[hour];
    }

    // Se ci sono minuti, aggiungi "e X minuti"
    if (minute > 0) {
      sequence[seqLen++] = WORD_E;
      if (minute < 60) {
        sequence[seqLen++] = numberFiles[minute];
      }
      sequence[seqLen++] = WORD_MINUTI;
    }
  }

  // Riproduci sequenza
  Serial.printf("Riproduzione sequenza di %d file:\n", seqLen);
  Serial.printf("Heap libero prima sequenza: %d bytes\n", ESP.getFreeHeap());

  for (int i = 0; i < seqLen; i++) {
    Serial.printf("  %d. %s\n", i + 1, sequence[i]);

    // Avvia riproduzione file
    if (!playLocalMP3(sequence[i])) {
      Serial.println("ERRORE: Impossibile riprodurre file, salto");
      continue;
    }

    // Attendi che il decoder sia effettivamente partito
    delay(30);

    // Usa la nuova funzione con timeout e gestione watchdog
    if (!playAudioWithTimeout(AUDIO_TIMEOUT_MS)) {
      Serial.println("ERRORE: Timeout durante riproduzione");
      cleanupAudio(false);  // Mantieni output per prossimo file
      continue;
    }

    // Attendi che l'I2S svuoti il buffer audio (ridotto)
    delay(50);

    // 🔧 OTTIMIZZAZIONE: Cleanup "soft" mantiene output tra file
    cleanupAudio(false);  // NON distruggere output tra file della sequenza

    // Pausa ridotta tra i file
    delay(150);

    // Yield per watchdog tra un file e l'altro
    yield();
  }

  // 🧹 Cleanup "soft" anche alla fine - mantieni output per comandi successivi
  cleanupAudio(false);  // NON distruggere output, lascialo pronto

  Serial.println("=== ANNUNCIO COMPLETATO ===");
  Serial.printf("Heap libero dopo sequenza: %d bytes\n\n", ESP.getFreeHeap());

  // Segnale completamento non necessario con I2C (comunicazione sincrona)

  return true;
}

// ================== ANNUNCIO BOOT ==================
bool announceBootMessage(uint8_t dayOfWeek, uint8_t day, uint8_t month, uint8_t hour, uint8_t minute, uint16_t year) {
  Serial.printf("\n=== ANNUNCIO BOOT: %s %d %s %d, %02d:%02d ===\n",
                daysOfWeek[dayOfWeek], day, months[month], year, hour, minute);

  // Attiva flag per proteggere l'annuncio da interruzioni
  isAnnouncingBoot = true;

  // Array di file da riprodurre in sequenza
  const char* sequence[20];  // Array più grande per boot message
  int seqLen = 0;

  // Costruisce la frase:
  // "Ciao sono OraQuadra Nano. Oggi è domenica 20 ottobre 2025. Sono le ore 20 e 30 minuti."

  // 1. Saluto completo: "Ciao sono OraQuadra Nano"
  sequence[seqLen++] = WORD_ORAQUADRA;  // saluto.mp3

  // 2. Giorno della settimana (es. "oggi è domenica")
  if (dayOfWeek < 7) {
    sequence[seqLen++] = daysOfWeek[dayOfWeek];
  }

  // 4. Numero del giorno (es. "20")
  if (day > 0 && day < 60) {
    sequence[seqLen++] = numberFiles[day];
  }

  // 5. Mese (es. "ottobre")
  if (month >= 1 && month <= 12) {
    sequence[seqLen++] = months[month];
  }

  // 6. Anno (es. "2025") - da RTC
  if (year >= 2024 && year <= 2030) {
    // Buffer statico per il nome file anno
    static char yearFile[16];
    snprintf(yearFile, sizeof(yearFile), "H-%d.mp3", year);
    sequence[seqLen++] = yearFile;
  }

  // 7. "Sono le"
  sequence[seqLen++] = WORD_SONO_LE;

  // 8. "ore"
  sequence[seqLen++] = WORD_ORE;

  // 9. Ora (es. "20")
  if (hour >= 0 && hour < 60) {
    sequence[seqLen++] = numberFiles[hour];
  }

  // 10. "e X minuti"
  sequence[seqLen++] = WORD_E;
  if (minute >= 0 && minute < 60) {
    sequence[seqLen++] = numberFiles[minute];
  }
  sequence[seqLen++] = WORD_MINUTI;

  // Riproduci sequenza
  Serial.printf("Riproduzione sequenza di %d file:\n", seqLen);
  Serial.printf("Heap libero prima sequenza boot: %d bytes\n", ESP.getFreeHeap());

  for (int i = 0; i < seqLen; i++) {
    Serial.printf("\n[%d/%d] File: %s\n", i + 1, seqLen, sequence[i]);
    Serial.printf("  📊 Heap PRIMA playLocalMP3: %d bytes\n", ESP.getFreeHeap());

    // Avvia riproduzione file
    if (!playLocalMP3(sequence[i])) {
      Serial.println("  ❌ ERRORE: Impossibile riprodurre file, salto");
      Serial.printf("  📊 Heap DOPO errore: %d bytes\n", ESP.getFreeHeap());
      continue;
    }

    Serial.printf("  📊 Heap DOPO playLocalMP3: %d bytes\n", ESP.getFreeHeap());

    // Attendi che il decoder sia effettivamente partito
    delay(30);

    Serial.println("  ▶️  Inizio playAudioWithTimeout...");
    // Usa la nuova funzione con timeout e gestione watchdog
    if (!playAudioWithTimeout(AUDIO_TIMEOUT_MS)) {
      Serial.println("  ❌ ERRORE: Timeout durante riproduzione");
      Serial.printf("  📊 Heap DOPO timeout: %d bytes\n", ESP.getFreeHeap());
      cleanupAudio(false);  // Mantieni output per prossimo file
      continue;
    }

    Serial.println("  ✅ File completato");
    Serial.printf("  📊 Heap DOPO playAudioWithTimeout: %d bytes\n", ESP.getFreeHeap());

    // Attendi che l'I2S svuoti il buffer audio (ridotto)
    delay(50);

    // 🔧 OTTIMIZZAZIONE: Cleanup "soft" mantiene output tra file
    Serial.println("  🧹 Cleanup soft...");
    cleanupAudio(false);  // NON distruggere output tra file della sequenza
    Serial.printf("  📊 Heap DOPO cleanup: %d bytes\n", ESP.getFreeHeap());

    // Pausa ridotta tra i file
    delay(150);

    // Yield per watchdog tra un file e l'altro
    yield();
    Serial.println("  ✓ Pronto per prossimo file");
  }

  // 🧹 Cleanup "soft" anche alla fine - mantieni output per comandi successivi
  cleanupAudio(false);  // NON distruggere output, lascialo pronto

  Serial.println("=== ANNUNCIO BOOT COMPLETATO ===");
  Serial.printf("Heap libero dopo sequenza boot: %d bytes\n\n", ESP.getFreeHeap());

  // Disattiva flag per permettere altri comandi audio
  isAnnouncingBoot = false;

  // Segnale completamento non necessario con I2C (comunicazione sincrona)

  return true;
}

// ================== CLEANUP AUDIO ==================
void cleanupAudio(bool destroyOutput) {
  if (mp3) {
    mp3->stop();  // Forza stop prima di delete
    delete mp3;
    mp3 = nullptr;
  }
  if (buff) {
    delete buff;
    buff = nullptr;
  }
  if (file) {
    delete file;
    file = nullptr;
  }

  // 🔧 OTTIMIZZAZIONE: Distruggi output solo se richiesto
  // Per MP3 locali consecutivi, mantieni output per evitare ricreazione
  if (destroyOutput) {
    if (output) {
      output->stop();  // Flush buffer I2S
      delete output;
      output = nullptr;
    }
    // 🔇 Spegni amplificatore quando distruggiamo tutto
    digitalWrite(I2S_ENABLE, LOW);
    Serial.printf("🗑️ Output I2S distrutto + amplificatore SPENTO (pin %d = LOW)\n", I2S_ENABLE);
  } else {
    // 🔧 CRITICO: Ferma output SOLO se stava riproducendo!
    // Fermare un output già fermo corrompe l'I2S driver!
    if (output && isPlaying) {
      output->stop();  // Ferma solo il buffer I2S
      // 🔊 Amplificatore RIMANE ACCESO per file consecutivi!
      Serial.printf("⏸️ Output I2S fermato (amplificatore ACCESO pin %d = HIGH)\n", I2S_ENABLE);
    } else if (output) {
      // Output esiste ma non sta riproducendo - NON chiamare stop()!
      Serial.printf("⏭️ Output I2S già fermo (skip stop, amplificatore ACCESO pin %d = HIGH)\n", I2S_ENABLE);
    } else {
      // Se non c'è output ma chiediamo soft cleanup, spegni amplificatore
      digitalWrite(I2S_ENABLE, LOW);
      Serial.printf("⏸️ Nessun output, amplificatore SPENTO (pin %d = LOW)\n", I2S_ENABLE);
    }
  }

  isPlaying = false;
}

// ================== VOLUME CONTROL ==================
void setVolume(uint8_t volume) {
  if (volume > 100) volume = 100;

  currentVolume = (float)volume / 100.0;

  if (output) {
    output->SetGain(currentVolume);
  }

  Serial.printf("Volume impostato: %d%% (%.2f)\n", volume, currentVolume);
}

// ================== LED DI STATO ==================
void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_STATUS, HIGH);
    delay(100);
    digitalWrite(LED_STATUS, LOW);
    delay(100);
  }
}

// ================== DEBUG LOGGING ==================
void addDebugLog(const String& message) {
  debugLogs[debugLogIndex] = message;
  debugLogTimestamps[debugLogIndex] = millis();
  debugLogIndex = (debugLogIndex + 1) % MAX_DEBUG_LOGS;

  // Stampa anche su seriale se disponibile
  Serial.println("[DEBUG LOG] " + message);
}

// ================== FUNZIONI DI UTILITÀ ==================
void printHeapStatus() {
  Serial.println("\n=== STATO MEMORIA ===");
  Serial.printf("Heap libero: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Heap minimo: %d bytes\n", ESP.getMinFreeHeap());
  Serial.printf("Heap dimensione: %d bytes\n", ESP.getHeapSize());

  // Calcola percentuale memoria libera
  float percentFree = (float)ESP.getFreeHeap() / (float)ESP.getHeapSize() * 100.0;
  Serial.printf("Memoria libera: %.1f%%\n", percentFree);

  // Warning se memoria bassa
  if (percentFree < 20.0) {
    Serial.println("⚠️  ATTENZIONE: Memoria bassa!");
  }

  Serial.println("====================\n");
}

// ================== STREAMING AUDIO DA URL ==================
/**
 * Avvia streaming audio MP3 da URL HTTP
 * @param url URL dello stream audio (es. http://192.168.1.24:5000/audio)
 * @return true se avviato con successo
 */
bool startAudioStream(const char* url) {
  Serial.printf("\n🎵 Avvio streaming audio: %s\n", url);

  if (!audioEnabled) {
    Serial.println("❌ Audio disabilitato");
    return false;
  }

  // Ferma eventuale streaming/riproduzione in corso
  stopAudioStream();
  cleanupAudio(false);

  // Verifica connessione WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi non connesso!");
    return false;
  }

  // Ricrea output se necessario
  if (!output) {
    output = new AudioOutputI2S();
    if (!output) {
      Serial.println("❌ Impossibile allocare output I2S");
      return false;
    }
    output->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    output->SetGain(currentVolume);
  }

  // Abilita amplificatore
  digitalWrite(I2S_ENABLE, HIGH);

  // Crea sorgente stream HTTP (usa HTTPStream per compatibilità con server Flask)
  Serial.printf("Connessione a: %s\n", url);
  streamSource = new AudioFileSourceHTTPStream(url);
  if (!streamSource) {
    Serial.println("❌ Impossibile allocare stream source");
    digitalWrite(I2S_ENABLE, LOW);
    return false;
  }

  if (!streamSource->isOpen()) {
    Serial.println("❌ Impossibile aprire stream URL");
    Serial.println("   Verifica che il server sia avviato e lo stream attivo");
    delete streamSource;
    streamSource = nullptr;
    digitalWrite(I2S_ENABLE, LOW);
    return false;
  }

  Serial.println("✓ Stream HTTP aperto");

  // Crea buffer per streaming - dimensione dinamica in base a memoria disponibile
  // ESP32-C3 ha RAM limitata, usiamo buffer più grande possibile senza compromettere stabilità
  currentStreamBufferSize = 16384;  // 16KB default per streaming
  if (ESP.getFreeHeap() < 50000) {
    currentStreamBufferSize = 8192;  // Fallback a 8KB se poca memoria
    Serial.println("⚠️ Memoria bassa, buffer ridotto a 8KB");
  }
  Serial.printf("Buffer streaming: %d bytes (Heap: %d)\n", currentStreamBufferSize, ESP.getFreeHeap());
  buff = new AudioFileSourceBuffer(streamSource, currentStreamBufferSize);
  if (!buff) {
    Serial.println("❌ Impossibile creare buffer");
    delete streamSource;
    streamSource = nullptr;
    digitalWrite(I2S_ENABLE, LOW);
    return false;
  }

  // Crea decoder MP3
  mp3 = new AudioGeneratorMP3();
  if (!mp3) {
    Serial.println("❌ Impossibile creare decoder MP3");
    delete buff;
    delete streamSource;
    buff = nullptr;
    streamSource = nullptr;
    digitalWrite(I2S_ENABLE, LOW);
    return false;
  }

  // Pre-buffering: attendi che il buffer si riempia prima di iniziare playback
  // Questo riduce lo stuttering iniziale
  Serial.println("⏳ Pre-buffering (1 secondo)...");
  unsigned long prebufStart = millis();
  while (millis() - prebufStart < 1000) {
    yield();
    delay(50);
  }
  Serial.println("✓ Pre-buffer completato");

  // Avvia riproduzione
  if (!mp3->begin(buff, output)) {
    Serial.println("❌ Impossibile avviare streaming");
    delete mp3;
    delete buff;
    delete streamSource;
    mp3 = nullptr;
    buff = nullptr;
    streamSource = nullptr;
    digitalWrite(I2S_ENABLE, LOW);
    return false;
  }

  isStreaming = true;
  isPlaying = true;
  currentStreamUrl = url;

  Serial.println("✅ Streaming audio avviato!");
  return true;
}

/**
 * Ferma streaming audio in corso
 */
void stopAudioStream() {
  if (!isStreaming) return;

  Serial.println("⏹️ Arresto streaming audio...");

  if (mp3) {
    mp3->stop();
    delete mp3;
    mp3 = nullptr;
  }

  if (buff) {
    delete buff;
    buff = nullptr;
  }

  if (streamSource) {
    delete streamSource;
    streamSource = nullptr;
  }

  isStreaming = false;
  isPlaying = false;
  currentStreamUrl = "";

  // Spegni amplificatore
  digitalWrite(I2S_ENABLE, LOW);

  Serial.println("✅ Streaming audio fermato");
}

// ================== SETUP WIFI ==================
void setupWiFi() {
  Serial.printf("Connessione a: %s\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.printf("✅ WiFi connesso! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println();
    Serial.println("⚠️ WiFi non connesso - streaming audio non disponibile");
    Serial.println("   Le altre funzioni audio (locale) funzioneranno comunque.");
  }
}
