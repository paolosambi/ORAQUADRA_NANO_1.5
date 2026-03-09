// ================== FUNZIONI UTILITY PER FLIP CLOCK ==================

#ifdef EFFECT_FLIP_CLOCK

// Calcola l'orario test attuale basandosi sul tempo trascorso dall'impostazione
void calculateTestTime(uint8_t &hour, uint8_t &minute, uint8_t &second) {
  // Calcola l'orario test attuale basandosi sul tempo trascorso dall'impostazione

  if (WiFi.status() != WL_CONNECTED) {
    // Senza WiFi, usa solo il tempo trascorso da millis()
    uint32_t elapsedMillis = millis() - testModeStartTime;
    uint32_t elapsedSeconds = elapsedMillis / 1000;

    // Calcola ore, minuti e secondi totali dall'orario base (inclusi i secondi impostati)
    uint32_t totalSeconds = (testHour * 3600) + (testMinute * 60) + testSecond + elapsedSeconds;

    hour = (totalSeconds / 3600) % 24;
    minute = (totalSeconds / 60) % 60;
    second = totalSeconds % 60;
  } else {
    // Con WiFi, usa i secondi NTP per maggiore precisione
    uint8_t currentSecondNTP = myTZ.second();

    // Calcola secondi trascorsi (gestendo il wrap a 60)
    int secondsPassed;
    if (currentSecondNTP >= testSecond) {
      secondsPassed = currentSecondNTP - testSecond;
    } else {
      secondsPassed = (60 - testSecond) + currentSecondNTP;
    }

    // Aggiungi i secondi trascorsi all'orario base
    uint32_t totalSeconds = (testHour * 3600) + (testMinute * 60) + secondsPassed;

    hour = (totalSeconds / 3600) % 24;
    minute = (totalSeconds / 60) % 60;
    second = totalSeconds % 60;
  }
}

// Funzione per riprodurre suono clack flip clock
// Usa audio LOCALE (I2S) se #define AUDIO, altrimenti I2C (ESP32C3)
// NOTA: VU meter NON viene mostrato (disabilitato di default, abilitato solo negli annunci)
bool playClackViaI2C() {
  // Se il radar è attivo e non c'è nessuno nella stanza, non riprodurre clack
  if (radarConnectedOnce && !presenceDetected) {
    return false;
  }

  #ifdef AUDIO
  // ========== AUDIO LOCALE (I2S diretto) ==========
  extern bool playLocalMP3(const char* filename);
  return playLocalMP3("clack.mp3");

  #else
  // ========== AUDIO VIA I2C (ESP32C3) ==========
  extern bool sendI2CCommand(uint8_t* data, size_t length);
  uint8_t cmd[] = {0x0A}; // CMD_PLAY_CLACK = 0x0A
  Serial.println("🔊 Invio CMD_PLAY_CLACK via I2C...");
  return sendI2CCommand(cmd, sizeof(cmd));
  #endif
}

#endif // EFFECT_FLIP_CLOCK
