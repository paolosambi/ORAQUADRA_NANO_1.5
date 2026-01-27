// ================== COMUNICAZIONE I2C CON ESP32C3 AUDIO SLAVE ==================
/*
 * Questo file gestisce la comunicazione I2C tra ESP32S3 (Master) e ESP32C3 (Slave)
 * per l'invio di comandi audio.
 *
 * L'ESP32C3 è configurato come I2C Slave (indirizzo 0x08) sullo STESSO BUS
 * del touchscreen GT911, condividendo i pin SDA/SCL.
 *
 * Connessioni fisiche:
 * - ESP32-S3 Pin 19 (SDA) → ESP32-C3 Pin 8 (SDA)
 * - ESP32-S3 Pin 45 (SCL) → ESP32-C3 Pin 9 (SCL)
 * - GND comune
 */

// ================== CONFIGURAZIONE I2C AUDIO ==================
#define AUDIO_I2C_ADDR       0x08    // Indirizzo I2C slave ESP32C3
#define I2C_RETRY_COUNT      3       // Numero di tentativi per comando I2C
#define I2C_TIMEOUT          100     // Timeout risposta I2C (ms)

// Prototipi funzioni
bool pingAudioSlave();
bool setVolumeViaI2C(uint8_t volume);
bool bootAnnounceViaI2C(uint8_t dayOfWeek, uint8_t day, uint8_t month, uint8_t hour, uint8_t minute, uint16_t year);

// Comandi I2C (devono corrispondere a quelli in ESP32C3)
#define CMD_ANNOUNCE_TIME    0x01  // Annuncia orario corrente
#define CMD_SET_VOLUME       0x03  // Imposta volume (0-100)
#define CMD_STOP_AUDIO       0x04  // Ferma riproduzione audio
#define CMD_PLAY_FILE        0x05  // Riproduci file specifico (ID)
#define CMD_PING             0x06  // Verifica connessione ESP32C3
#define CMD_BOOT_ANNOUNCE    0x08  // Annuncio boot con data/ora completa
#define CMD_PLAY_BEEP        0x09  // Riproduci beep.mp3 (feedback tocco)
#define CMD_PLAY_CLACK       0x0A  // Riproduci clack.mp3 (flip clock)
#define CMD_SET_AUDIO_ENABLE 0x0B  // Abilita/disabilita audio (1=abilita, 0=disabilita)
#define CMD_PLAY_STREAM      0x12  // Avvia streaming audio da URL
#define CMD_STOP_STREAM      0x13  // Ferma streaming audio

// Risposte I2C dall'ESP32C3
#define RESP_PONG            0xFF  // Risposta a CMD_PING
#define RESP_AUDIO_COMPLETED 0xAA  // Segnale che l'audio è terminato

// ================== VARIABILI STATO I2C ==================
// NOTA: Wire già inizializzato in 0_SETUP.ino per touchscreen GT911
// Riusiamo lo stesso bus I2C per comunicare con ESP32C3
// audioSlaveConnected è definita in oraQuadraNano_V1_4.ino
extern bool audioSlaveConnected;

// ================== SETUP I2C AUDIO ==================
/**
 * Inizializza comunicazione I2C con ESP32C3 (opzionale, Wire già init da touchscreen)
 * Test PING per verificare che ESP32C3 sia presente sul bus
 */
void setup_audio_i2c() {
  Serial.println("\n=== Inizializzazione I2C Audio Slave ===");
  Serial.printf("Indirizzo I2C: 0x%02X\n", AUDIO_I2C_ADDR);
  Serial.printf("Bus condiviso con GT911 touchscreen\n");

  // Test PING per verificare presenza ESP32C3
  Serial.println("\nTest PING ESP32C3...");
  if (pingAudioSlave()) {
    Serial.println("✓ ESP32C3 risponde!");
    audioSlaveConnected = true;

    // Imposta volume iniziale in base alla fascia oraria (giorno/notte)
    Serial.println("Configurazione iniziale audio...");
    extern bool lastWasNightTime;
    extern uint8_t lastAppliedVolume;
    extern uint8_t volumeDay;
    extern uint8_t volumeNight;

    // Determina se è notte basandosi sull'ora corrente
    lastWasNightTime = checkIsNightTime(myTZ.hour(), myTZ.minute());
    uint8_t initialVolume = lastWasNightTime ? volumeNight : volumeDay;
    lastAppliedVolume = initialVolume;
    setVolumeViaI2C(initialVolume);
    Serial.printf("Volume iniziale: %d%% (%s)\n", initialVolume, lastWasNightTime ? "NOTTE" : "GIORNO");
    delay(50);

    // Boot announce spostato in funzione separata (chiamata dopo display inizializzato)

  } else {
    Serial.println("\n>>> ESP32C3 AUDIO: NON TROVATO <<<");
    Serial.println("Il sistema funzionera' comunque senza audio.");
    Serial.println("\nSe vuoi usare l'audio, verifica:");
    Serial.println("  - ESP32C3 acceso e programmato");
    Serial.println("  - Cavi I2C collegati (SDA/SCL/GND)");
    Serial.println("  - Indirizzo I2C corretto (0x08)");
    audioSlaveConnected = false;
  }

  Serial.println("=== I2C Audio Slave Configurazione Completata ===\n");
}

// ================== ANNUNCIO BOOT (chiamato dopo display inizializzato) ==================
/**
 * Annuncia data e ora al boot - chiamare DOPO che il display è inizializzato
 * così l'utente vede l'orologio durante l'annuncio audio
 *
 * USA AUDIO LOCALE (I2S diretto) se disponibile (#ifdef AUDIO)
 * Altrimenti usa audio via I2C (ESP32C3)
 */
void playBootAnnounce() {
  Serial.println("\n[BOOT ANNOUNCE] Avvio annuncio boot...");

  // Verifica che NTP sia sincronizzato
  uint16_t year = myTZ.year();
  uint8_t month = myTZ.month();
  uint8_t day = myTZ.day();

  if (year < 2024 || year > 2030 || month < 1 || month > 12 || day < 1 || day > 31) {
    Serial.println("[BOOT ANNOUNCE] ⚠️ NTP non sincronizzato - skip");
    return;
  }

  #ifdef AUDIO
  // ========== AUDIO LOCALE (I2S diretto) ==========
  Serial.println("[BOOT ANNOUNCE] Usando audio LOCALE (I2S)");
  extern bool announceBootLocal();  // Definita in 5_AUDIO.ino
  announceBootLocal();
  Serial.println("[BOOT ANNOUNCE] ✓ Annuncio locale completato");

  #else
  // ========== AUDIO VIA I2C (ESP32C3) ==========
  if (!audioSlaveConnected) {
    Serial.println("[BOOT ANNOUNCE] Audio slave non connesso, skip");
    return;
  }

  uint8_t dayOfWeekRaw = myTZ.weekday();  // 1=Dom, 2=Lun, ..., 7=Sab
  uint8_t dayOfWeek = (dayOfWeekRaw >= 1 && dayOfWeekRaw <= 7) ? dayOfWeekRaw - 1 : 0;
  uint8_t hour = myTZ.hour();
  uint8_t minute = myTZ.minute();

  Serial.printf("[BOOT ANNOUNCE] Usando audio I2C: %d/%d/%d %02d:%02d\n",
                day, month, year, hour, minute);
  bootAnnounceViaI2C(dayOfWeek, day, month, hour, minute, year);
  Serial.println("[BOOT ANNOUNCE] ✓ Comando I2C inviato");
  #endif
}

// ================== INVIO COMANDO I2C GENERICO ==================
/**
 * Invia un comando I2C generico all'ESP32C3
 * @param data Buffer contenente il comando
 * @param length Lunghezza del comando in bytes
 * @return true se invio riuscito, false altrimenti
 */
bool sendI2CCommand(uint8_t* data, size_t length) {
  if (length == 0 || length > 32) {  // Limite buffer I2C: 32 bytes
    Serial.printf("❌ ERRORE: Lunghezza comando I2C non valida: %d bytes\n", length);
    return false;
  }

  for (int retry = 0; retry < I2C_RETRY_COUNT; retry++) {
    Wire.beginTransmission(AUDIO_I2C_ADDR);
    size_t written = Wire.write(data, length);
    uint8_t error = Wire.endTransmission();

    if (error == 0 && written == length) {
      Serial.printf("✓ I2C TX: CMD 0x%02X (%d bytes)\n", data[0], length);
      return true;
    }

    Serial.printf("⚠️  I2C TX fallito (tentativo %d/%d, error=%d, written=%d)\n",
                  retry + 1, I2C_RETRY_COUNT, error, written);
    delay(10);  // Breve pausa prima del retry
  }

  Serial.printf("❌ I2C TX FALLITO dopo %d tentativi\n", I2C_RETRY_COUNT);
  audioSlaveConnected = false;
  return false;
}

// ================== PING I2C SLAVE ==================
/**
 * Verifica connessione ESP32C3 tramite PING/PONG
 * @return true se ESP32C3 risponde, false altrimenti
 */
bool pingAudioSlave() {
  uint8_t cmd[] = {CMD_PING};

  // Invia CMD_PING
  Wire.beginTransmission(AUDIO_I2C_ADDR);
  Wire.write(cmd, sizeof(cmd));
  uint8_t error = Wire.endTransmission();

  if (error != 0) {
    Serial.printf("❌ PING fallito (endTransmission error=%d)\n", error);
    audioSlaveConnected = false;
    return false;
  }

  // Richiedi PONG response (cast espliciti per evitare warning ambiguità)
  uint8_t bytesReceived = Wire.requestFrom((uint8_t)AUDIO_I2C_ADDR, (uint8_t)1);

  if (bytesReceived == 0) {
    Serial.println("❌ PONG non ricevuto (timeout)");
    audioSlaveConnected = false;
    return false;
  }

  uint8_t response = Wire.read();

  if (response == RESP_PONG) {
    Serial.println("✓ PONG ricevuto (0xFF)");
    audioSlaveConnected = true;
    return true;
  }

  Serial.printf("❌ Risposta non valida: 0x%02X (atteso 0xFF)\n", response);
  audioSlaveConnected = false;
  return false;
}

// ================== ANNUNCIO ORARIO ==================
/**
 * Richiede annuncio orario vocale
 */
bool announceTimeViaI2C(uint8_t hour, uint8_t minute) {
  uint8_t cmd[] = {CMD_ANNOUNCE_TIME, hour, minute};
  Serial.printf("📢 Annuncio orario: %02d:%02d\n", hour, minute);
  return sendI2CCommand(cmd, sizeof(cmd));
}

// ================== IMPOSTA VOLUME ==================
/**
 * Imposta volume (0-100)
 * Usa audio LOCALE (I2S) se #define AUDIO, altrimenti I2C (ESP32C3)
 */
bool setVolumeViaI2C(uint8_t volume) {
  if (volume > 100) volume = 100;

  #ifdef AUDIO
  // ========== AUDIO LOCALE ==========
  extern void setVolumeLocal(uint8_t volume);
  setVolumeLocal(volume);
  return true;

  #else
  // ========== AUDIO VIA I2C (ESP32C3) ==========
  uint8_t cmd[] = {CMD_SET_VOLUME, volume};
  Serial.printf("🔊 Imposta volume I2C: %d%%\n", volume);
  return sendI2CCommand(cmd, sizeof(cmd));
  #endif
}

// ================== FERMA AUDIO ==================
/**
 * Ferma riproduzione audio corrente
 */
bool stopAudioViaI2C() {
  uint8_t cmd[] = {CMD_STOP_AUDIO};
  Serial.println("⏹️  Stop audio");
  return sendI2CCommand(cmd, sizeof(cmd));
}

// ================== ABILITA/DISABILITA AUDIO ==================
/**
 * Abilita o disabilita completamente l'audio
 */
bool setAudioEnableViaI2C(bool enable) {
  uint8_t cmd[] = {CMD_SET_AUDIO_ENABLE, enable ? (uint8_t)1 : (uint8_t)0};
  Serial.printf("%s audio\n", enable ? "✓ Abilita" : "✗ Disabilita");
  return sendI2CCommand(cmd, sizeof(cmd));
}

// ================== RIPRODUCI FILE ==================
/**
 * Riproduci file MP3 specifico per ID (0-59)
 */
bool playFileViaI2C(uint8_t fileID) {
  if (fileID >= 60) {
    Serial.printf("❌ File ID non valido: %d (max 59)\n", fileID);
    return false;
  }
  uint8_t cmd[] = {CMD_PLAY_FILE, fileID};
  Serial.printf("🎵 Play file ID: %d\n", fileID);
  return sendI2CCommand(cmd, sizeof(cmd));
}

// ================== ANNUNCIO BOOT ==================
/**
 * Annuncio boot con data e ora completa
 */
bool bootAnnounceViaI2C(uint8_t dayOfWeek, uint8_t day, uint8_t month, uint8_t hour, uint8_t minute, uint16_t year) {
  uint8_t yearHigh = (year >> 8) & 0xFF;
  uint8_t yearLow = year & 0xFF;

  uint8_t cmd[] = {
    CMD_BOOT_ANNOUNCE,
    dayOfWeek,  // 0-6 (0=Domenica)
    day,        // 1-31
    month,      // 1-12
    hour,       // 0-23
    minute,     // 0-59
    yearHigh,   // Anno byte alto
    yearLow     // Anno byte basso
  };

  Serial.printf("📢 Annuncio boot: giorno %d, %02d/%02d/%04d %02d:%02d\n",
                dayOfWeek, day, month, year, hour, minute);
  return sendI2CCommand(cmd, sizeof(cmd));
}

// ================== BEEP (TOUCH FEEDBACK) ==================
/**
 * Riproduci beep.mp3 per feedback touch
 * Usa audio LOCALE (I2S) se #define AUDIO, altrimenti I2C (ESP32C3)
 * NOTA: VU meter NON viene mostrato (disabilitato di default, abilitato solo negli annunci)
 */
bool playBeepViaI2C() {
  #ifdef AUDIO
  // ========== AUDIO LOCALE (I2S diretto) ==========
  extern bool playLocalMP3(const char* filename);
  Serial.println("🔔 Play BEEP (locale)");
  return playLocalMP3("beep.mp3");

  #else
  // ========== AUDIO VIA I2C (ESP32C3) ==========
  uint8_t cmd[] = {CMD_PLAY_BEEP};
  Serial.println("🔔 Play BEEP (I2C)");
  return sendI2CCommand(cmd, sizeof(cmd));
  #endif
}

// ================== CLACK (FLIP CLOCK) ==================
// NOTA: playClackViaI2C() è definita in 11_FLIP_CLOCK_UTILS.ino
// Dichiarazione esterna per evitare duplicazione
extern bool playClackViaI2C();

// ================== STREAMING AUDIO DA URL ==================
/**
 * Avvia streaming audio da URL sull'ESP32C3
 * @param url URL dello stream audio (max 200 caratteri)
 */
bool playStreamViaI2C(const char* url) {
  if (!audioSlaveConnected) {
    Serial.println("[STREAM] ESP32C3 non connesso");
    return false;
  }

  int urlLen = strlen(url);
  if (urlLen > 200) {
    Serial.println("[STREAM] URL troppo lungo (max 200)");
    return false;
  }

  // Costruisci comando: [CMD_PLAY_STREAM, urlLen, url...]
  uint8_t cmd[256];
  cmd[0] = CMD_PLAY_STREAM;
  cmd[1] = (uint8_t)urlLen;
  memcpy(&cmd[2], url, urlLen);

  Serial.printf("🎵 Avvio streaming: %s\n", url);
  return sendI2CCommand(cmd, 2 + urlLen);
}

/**
 * Ferma streaming audio sull'ESP32C3
 */
bool stopStreamViaI2C() {
  if (!audioSlaveConnected) {
    return false;
  }

  uint8_t cmd[] = {CMD_STOP_STREAM};
  Serial.println("⏹️ Stop streaming audio");
  return sendI2CCommand(cmd, sizeof(cmd));
}

// ================== UPDATE I2C AUDIO (LOOP) ==================
/**
 * Aggiorna stato comunicazione I2C audio (chiamato nel loop principale)
 * Verifica periodicamente connessione con PING e tenta riconnessione se necessario
 */
void updateAudioI2C() {
  static unsigned long lastPing = 0;
  static bool wasConnected = false;  // Per rilevare riconnessione

  // Se non connesso, tenta riconnessione ogni 10 secondi
  // Se connesso, verifica ogni 60 secondi che la connessione sia ancora attiva
  unsigned long checkInterval = audioSlaveConnected ? 60000 : 10000;

  if (millis() - lastPing > checkInterval) {
    lastPing = millis();

    bool previousState = audioSlaveConnected;

    // Tenta PING
    if (pingAudioSlave()) {
      // Connessione OK
      if (!previousState) {
        // RICONNESSIONE RIUSCITA! ESP32C3 trovato dopo boot
        Serial.println("\n========================================");
        Serial.println(">>> ESP32C3 AUDIO: RICONNESSO! <<<");
        Serial.println("Il parlato ora è disponibile.");
        Serial.println("========================================\n");

        // Imposta volume (da EEPROM, default 80%)
        setVolumeViaI2C(audioVolume);
        delay(50);

        // Esegui annuncio boot se non è stato fatto prima
        playBootAnnounce();
      }
    } else {
      // Connessione persa o non disponibile
      if (previousState) {
        Serial.println("\n>>> ESP32C3 AUDIO: DISCONNESSO! <<<");
        Serial.println("Il parlato non è disponibile.\n");
      }
    }

    wasConnected = audioSlaveConnected;
  }
}

// ================== PRINT STATUS ==================
/**
 * Stampa stato connessione I2C audio
 */
void printI2CStatus() {
  Serial.println("\n=== Stato I2C Audio ===");
  Serial.printf("ESP32C3 indirizzo: 0x%02X\n", AUDIO_I2C_ADDR);
  Serial.printf("Connesso: %s\n", audioSlaveConnected ? "SI" : "NO");
  Serial.println("=======================\n");
}
