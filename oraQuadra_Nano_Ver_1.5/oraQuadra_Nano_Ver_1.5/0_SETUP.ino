// ================== IMPLEMENTAZIONE FUNZIONI DI SETUP ==================
void setup_eeprom() {
  EEPROM.begin(EEPROM_SIZE); // Inizializza la libreria EEPROM con la dimensione definita.

  // Verifica se il layout EEPROM è aggiornato (nuova versione per evitare conflitti indirizzi)
  bool needsReset = (EEPROM.read(0) != EEPROM_CONFIGURED_MARKER) ||
                    (EEPROM.read(EEPROM_VERSION_ADDR) != EEPROM_VERSION_VALUE);

  if (needsReset) {
    Serial.println("[EEPROM] Layout versione nuova - reset impostazioni");
    // Resetta EEPROM ai nuovi indirizzi
    EEPROM.write(EEPROM_VERSION_ADDR, EEPROM_VERSION_VALUE);  // Marca la nuova versione
    // Prima configurazione: esegue solo se il marker non è presente.
    EEPROM.write(0, EEPROM_CONFIGURED_MARKER);     // Scrive un marker per indicare che la EEPROM è stata configurata.
    EEPROM.write(EEPROM_PRESET_ADDR, 0);          // Imposta il preset predefinito a 0.
    EEPROM.write(EEPROM_MODE_ADDR, MODE_FAST);     // Imposta la modalità di visualizzazione predefinita a FAST.
    EEPROM.write(EEPROM_WORD_E_STATE_ADDR, 1);    // Imposta lo stato predefinito della parola "E" a visibile e fissa.
    // Imposta i colori predefiniti a bianco (R=255, G=255, B=255).
    EEPROM.write(EEPROM_COLOR_R_ADDR, 255);
    EEPROM.write(EEPROM_COLOR_G_ADDR, 255);
    EEPROM.write(EEPROM_COLOR_B_ADDR, 255);
    // Imposta la data dell'orologio analogico come visibile per default.
    EEPROM.write(EEPROM_CLOCK_DATE_ADDR, 1);

    // Imposta orari giorno/notte di default
    EEPROM.write(EEPROM_DAY_START_HOUR_ADDR, 8);   // Giorno inizia alle 8:00
    EEPROM.write(EEPROM_NIGHT_START_HOUR_ADDR, 22); // Notte inizia alle 22:00

    // Imposta luminosità e annuncio orario di default
    EEPROM.write(EEPROM_HOURLY_ANNOUNCE_ADDR, 1);          // Annuncio orario abilitato
    EEPROM.write(EEPROM_BRIGHTNESS_DAY_ADDR, BRIGHTNESS_DAY_DEFAULT);    // Luminosità giorno 250
    EEPROM.write(EEPROM_BRIGHTNESS_NIGHT_ADDR, BRIGHTNESS_NIGHT_DEFAULT); // Luminosità notte 90

#ifdef EFFECT_ANALOG_CLOCK
    // Inizializza colori default lancette (RGB565)
    EEPROM.write(EEPROM_CLOCK_HOUR_COLOR_ADDR, BLACK >> 8);
    EEPROM.write(EEPROM_CLOCK_HOUR_COLOR_ADDR + 1, BLACK & 0xFF);
    EEPROM.write(EEPROM_CLOCK_MINUTE_COLOR_ADDR, BLACK >> 8);
    EEPROM.write(EEPROM_CLOCK_MINUTE_COLOR_ADDR + 1, BLACK & 0xFF);
    EEPROM.write(EEPROM_CLOCK_SECOND_COLOR_ADDR, RED >> 8);
    EEPROM.write(EEPROM_CLOCK_SECOND_COLOR_ADDR + 1, RED & 0xFF);

    // Inizializza posizione Y data default
    EEPROM.write(EEPROM_CLOCK_DATE_Y_ADDR, 320 >> 8);
    EEPROM.write(EEPROM_CLOCK_DATE_Y_ADDR + 1, 320 & 0xFF);

    // Inizializza nome skin default
    const char* defaultSkin = "orologio.jpg";
    for (int i = 0; i < 32; i++) {
      EEPROM.write(EEPROM_CLOCK_SKIN_NAME_ADDR + i, i < strlen(defaultSkin) ? defaultSkin[i] : 0);
    }
#endif

#ifdef EFFECT_BTTF
    // Inizializza date BTTF con valori iconici del film
    // DESTINATION TIME: Oct 26, 1985 - 1:20 AM
    EEPROM.write(EEPROM_BTTF_DEST_MONTH_ADDR, 10);
    EEPROM.write(EEPROM_BTTF_DEST_DAY_ADDR, 26);
    EEPROM.write(EEPROM_BTTF_DEST_YEAR_ADDR, 1985 >> 8);
    EEPROM.write(EEPROM_BTTF_DEST_YEAR_ADDR + 1, 1985 & 0xFF);
    EEPROM.write(EEPROM_BTTF_DEST_HOUR_ADDR, 1);
    EEPROM.write(EEPROM_BTTF_DEST_MINUTE_ADDR, 20);
    EEPROM.write(EEPROM_BTTF_DEST_AMPM_ADDR, 0); // 0 = AM

    // LAST TIME DEPARTED: Nov 5, 1955 - 6:00 AM
    EEPROM.write(EEPROM_BTTF_LAST_MONTH_ADDR, 11);
    EEPROM.write(EEPROM_BTTF_LAST_DAY_ADDR, 5);
    EEPROM.write(EEPROM_BTTF_LAST_YEAR_ADDR, 1955 >> 8);
    EEPROM.write(EEPROM_BTTF_LAST_YEAR_ADDR + 1, 1955 & 0xFF);
    EEPROM.write(EEPROM_BTTF_LAST_HOUR_ADDR, 6);
    EEPROM.write(EEPROM_BTTF_LAST_MINUTE_ADDR, 0);
    EEPROM.write(EEPROM_BTTF_LAST_AMPM_ADDR, 0); // 0 = AM

    // Inizializza font sizes BTTF
    EEPROM.write(EEPROM_BTTF_MONTH_FONT_SIZE_ADDR, 2);   // 2 = medium (default)
    EEPROM.write(EEPROM_BTTF_NUMBER_FONT_SIZE_ADDR, 2);  // 2 = medium (default)

    // Helper per scrivere int16
    auto writeInt16 = [](uint16_t addr, int16_t value) {
      EEPROM.write(addr, value >> 8);
      EEPROM.write(addr + 1, value & 0xFF);
    };

    // Inizializza coordinate INDIVIDUALI per ogni campo di ogni pannello
    // Valori ottimizzati per bttf.jpg (480x480px)

    // PANEL 1 (DESTINATION TIME) - Y base: 75
    writeInt16(EEPROM_BTTF_P1_MONTH_X_ADDR, 30);
    writeInt16(EEPROM_BTTF_P1_MONTH_Y_ADDR, 75);
    writeInt16(EEPROM_BTTF_P1_DAY_X_ADDR, 122);
    writeInt16(EEPROM_BTTF_P1_DAY_Y_ADDR, 75);
    writeInt16(EEPROM_BTTF_P1_YEAR_X_ADDR, 198);
    writeInt16(EEPROM_BTTF_P1_YEAR_Y_ADDR, 75);
    writeInt16(EEPROM_BTTF_P1_AMPM_X_ADDR, 295);
    writeInt16(EEPROM_BTTF_P1_AMPM_Y_ADDR, 57);  // LED leggermente più alto
    writeInt16(EEPROM_BTTF_P1_HOUR_X_ADDR, 320);
    writeInt16(EEPROM_BTTF_P1_HOUR_Y_ADDR, 75);
    writeInt16(EEPROM_BTTF_P1_MIN_X_ADDR, 400);
    writeInt16(EEPROM_BTTF_P1_MIN_Y_ADDR, 75);

    // PANEL 2 (PRESENT TIME) - Y base: 235
    writeInt16(EEPROM_BTTF_P2_MONTH_X_ADDR, 30);
    writeInt16(EEPROM_BTTF_P2_MONTH_Y_ADDR, 235);
    writeInt16(EEPROM_BTTF_P2_DAY_X_ADDR, 122);
    writeInt16(EEPROM_BTTF_P2_DAY_Y_ADDR, 235);
    writeInt16(EEPROM_BTTF_P2_YEAR_X_ADDR, 198);
    writeInt16(EEPROM_BTTF_P2_YEAR_Y_ADDR, 235);
    writeInt16(EEPROM_BTTF_P2_AMPM_X_ADDR, 295);
    writeInt16(EEPROM_BTTF_P2_AMPM_Y_ADDR, 217);  // LED leggermente più alto
    writeInt16(EEPROM_BTTF_P2_HOUR_X_ADDR, 320);
    writeInt16(EEPROM_BTTF_P2_HOUR_Y_ADDR, 235);
    writeInt16(EEPROM_BTTF_P2_MIN_X_ADDR, 400);
    writeInt16(EEPROM_BTTF_P2_MIN_Y_ADDR, 235);

    // PANEL 3 (LAST TIME DEPARTED) - Y base: 395
    writeInt16(EEPROM_BTTF_P3_MONTH_X_ADDR, 30);
    writeInt16(EEPROM_BTTF_P3_MONTH_Y_ADDR, 395);
    writeInt16(EEPROM_BTTF_P3_DAY_X_ADDR, 122);
    writeInt16(EEPROM_BTTF_P3_DAY_Y_ADDR, 395);
    writeInt16(EEPROM_BTTF_P3_YEAR_X_ADDR, 198);
    writeInt16(EEPROM_BTTF_P3_YEAR_Y_ADDR, 395);
    writeInt16(EEPROM_BTTF_P3_AMPM_X_ADDR, 295);
    writeInt16(EEPROM_BTTF_P3_AMPM_Y_ADDR, 377);  // LED leggermente più alto
    writeInt16(EEPROM_BTTF_P3_HOUR_X_ADDR, 320);
    writeInt16(EEPROM_BTTF_P3_HOUR_Y_ADDR, 395);
    writeInt16(EEPROM_BTTF_P3_MIN_X_ADDR, 400);
    writeInt16(EEPROM_BTTF_P3_MIN_Y_ADDR, 395);
#endif

    EEPROM.commit(); // Scrive i dati dalla cache della EEPROM alla memoria fisica.
  }

  // Carica il preset salvato dalla EEPROM.
  currentPreset = EEPROM.read(EEPROM_PRESET_ADDR);

  // Carica la modalità di visualizzazione salvata dalla EEPROM e la converte al tipo DisplayMode.
  userMode = (DisplayMode)EEPROM.read(EEPROM_MODE_ADDR);


  // Carica lo stato della parola "E" dalla EEPROM.
  word_E_state = EEPROM.read(EEPROM_WORD_E_STATE_ADDR);
  if (word_E_state > 1) word_E_state = 1;  // Imposta un valore di sicurezza a 1 se il valore letto è fuori range (0 o 1).

  // Carica i valori dei colori salvati dalla EEPROM (vecchio metodo per compatibilità)
  userColor.r = EEPROM.read(EEPROM_COLOR_R_ADDR);
  userColor.g = EEPROM.read(EEPROM_COLOR_G_ADDR);
  userColor.b = EEPROM.read(EEPROM_COLOR_B_ADDR);
  currentColor = userColor;  // Applica il colore caricato

  // Carica orari giorno/notte dalla EEPROM
  uint8_t loadedDayStart = EEPROM.read(EEPROM_DAY_START_HOUR_ADDR);
  uint8_t loadedNightStart = EEPROM.read(EEPROM_NIGHT_START_HOUR_ADDR);
  uint8_t loadedDayStartMin = EEPROM.read(EEPROM_DAY_START_MIN_ADDR);
  uint8_t loadedNightStartMin = EEPROM.read(EEPROM_NIGHT_START_MIN_ADDR);
  // Valida i valori (ore devono essere 0-23, minuti 0-59)
  if (loadedDayStart < 24) {
    dayStartHour = loadedDayStart;
  } else {
    dayStartHour = 8; // Default
    EEPROM.write(EEPROM_DAY_START_HOUR_ADDR, 8);
    EEPROM.commit();
  }
  if (loadedDayStartMin < 60) {
    dayStartMinute = loadedDayStartMin;
  } else {
    dayStartMinute = 0; // Default
    EEPROM.write(EEPROM_DAY_START_MIN_ADDR, 0);
    EEPROM.commit();
  }
  if (loadedNightStart < 24) {
    nightStartHour = loadedNightStart;
  } else {
    nightStartHour = 22; // Default
    EEPROM.write(EEPROM_NIGHT_START_HOUR_ADDR, 22);
    EEPROM.commit();
  }
  if (loadedNightStartMin < 60) {
    nightStartMinute = loadedNightStartMin;
  } else {
    nightStartMinute = 0; // Default
    EEPROM.write(EEPROM_NIGHT_START_MIN_ADDR, 0);
    EEPROM.commit();
  }
  Serial.printf("[SETUP] Orari giorno/notte: Giorno dalle %02d:%02d, Notte dalle %02d:%02d\n", dayStartHour, dayStartMinute, nightStartHour, nightStartMinute);

  // Carica luminosità giorno/notte e annuncio orario dalla EEPROM
  uint8_t loadedBrightnessDay = EEPROM.read(EEPROM_BRIGHTNESS_DAY_ADDR);
  uint8_t loadedBrightnessNight = EEPROM.read(EEPROM_BRIGHTNESS_NIGHT_ADDR);
  uint8_t loadedHourlyAnnounce = EEPROM.read(EEPROM_HOURLY_ANNOUNCE_ADDR);

  // Valida luminosità (valori 0xFF indicano EEPROM non inizializzata)
  // IMPORTANTE: Valore minimo 10 per evitare schermo spento al boot
  if (loadedBrightnessDay != 0xFF && loadedBrightnessDay >= 10) {
    brightnessDay = loadedBrightnessDay;
  } else {
    brightnessDay = BRIGHTNESS_DAY_DEFAULT;
    EEPROM.write(EEPROM_BRIGHTNESS_DAY_ADDR, brightnessDay);
  }
  if (loadedBrightnessNight != 0xFF && loadedBrightnessNight >= 10) {
    brightnessNight = loadedBrightnessNight;
  } else {
    brightnessNight = BRIGHTNESS_NIGHT_DEFAULT;
    EEPROM.write(EEPROM_BRIGHTNESS_NIGHT_ADDR, brightnessNight);
  }
  if (loadedHourlyAnnounce != 0xFF) {
    hourlyAnnounceEnabled = (loadedHourlyAnnounce == 1);
  } else {
    hourlyAnnounceEnabled = true;
    EEPROM.write(EEPROM_HOURLY_ANNOUNCE_ADDR, 1);
  }

  // Carica volume audio dalla EEPROM
  uint8_t loadedAudioVolume = EEPROM.read(EEPROM_AUDIO_VOLUME_ADDR);
  if (loadedAudioVolume != 0xFF && loadedAudioVolume <= 100) {
    audioVolume = loadedAudioVolume;
  } else {
    audioVolume = 80;  // Default 80%
    EEPROM.write(EEPROM_AUDIO_VOLUME_ADDR, audioVolume);
  }

  // Carica impostazioni audio giorno/notte dalla EEPROM
  uint8_t loadedAudioDayEnabled = EEPROM.read(EEPROM_AUDIO_DAY_ENABLED_ADDR);
  uint8_t loadedAudioNightEnabled = EEPROM.read(EEPROM_AUDIO_NIGHT_ENABLED_ADDR);
  uint8_t loadedVolumeDay = EEPROM.read(EEPROM_VOLUME_DAY_ADDR);
  uint8_t loadedVolumeNight = EEPROM.read(EEPROM_VOLUME_NIGHT_ADDR);

  if (loadedAudioDayEnabled != 0xFF) {
    audioDayEnabled = (loadedAudioDayEnabled == 1);
  } else {
    audioDayEnabled = true;  // Default ON
    EEPROM.write(EEPROM_AUDIO_DAY_ENABLED_ADDR, 1);
  }

  if (loadedAudioNightEnabled != 0xFF) {
    audioNightEnabled = (loadedAudioNightEnabled == 1);
  } else {
    audioNightEnabled = false;  // Default OFF
    EEPROM.write(EEPROM_AUDIO_NIGHT_ENABLED_ADDR, 0);
  }

  if (loadedVolumeDay != 0xFF && loadedVolumeDay <= 100) {
    volumeDay = loadedVolumeDay;
  } else {
    volumeDay = 80;  // Default 80%
    EEPROM.write(EEPROM_VOLUME_DAY_ADDR, volumeDay);
  }

  if (loadedVolumeNight != 0xFF && loadedVolumeNight <= 100) {
    volumeNight = loadedVolumeNight;
  } else {
    volumeNight = 30;  // Default 30%
    EEPROM.write(EEPROM_VOLUME_NIGHT_ADDR, volumeNight);
  }

  Serial.printf("[SETUP] Audio Giorno: %s (vol %d%%), Notte: %s (vol %d%%)\n",
                audioDayEnabled ? "ON" : "OFF", volumeDay,
                audioNightEnabled ? "ON" : "OFF", volumeNight);

  // Carica impostazioni cambio modalità random dalla EEPROM
  uint8_t loadedRandomMode = EEPROM.read(EEPROM_RANDOM_MODE_ADDR);
  uint8_t loadedRandomInterval = EEPROM.read(EEPROM_RANDOM_INTERVAL_ADDR);
  if (loadedRandomMode != 0xFF) {
    randomModeEnabled = (loadedRandomMode == 1);
  } else {
    randomModeEnabled = false;
    EEPROM.write(EEPROM_RANDOM_MODE_ADDR, 0);
  }
  if (loadedRandomInterval != 0xFF && loadedRandomInterval >= 1 && loadedRandomInterval <= 60) {
    randomModeInterval = loadedRandomInterval;
  } else {
    randomModeInterval = 5;  // Default 5 minuti
    EEPROM.write(EEPROM_RANDOM_INTERVAL_ADDR, 5);
  }
  Serial.printf("[SETUP] Random mode: %s, intervallo: %d minuti\n", randomModeEnabled ? "ON" : "OFF", randomModeInterval);

  EEPROM.commit();
  Serial.printf("[SETUP] Luminosità: Giorno=%d, Notte=%d | Annuncio orario: %s\n",
                brightnessDay, brightnessNight, hourlyAnnounceEnabled ? "ON" : "OFF");
  Serial.printf("[SETUP] Volume audio: %d%%\n", audioVolume);

#ifdef EFFECT_ANALOG_CLOCK
  // Carica lo stato della data dell'orologio analogico dalla EEPROM (impostazione globale)
  uint8_t dateState = EEPROM.read(EEPROM_CLOCK_DATE_ADDR);
  showClockDate = (dateState == 1); // 1 = visibile, 0 = nascosta

  // Carica lo stato smooth seconds dalla EEPROM
  uint8_t smoothState = EEPROM.read(EEPROM_CLOCK_SMOOTH_SECONDS_ADDR);
  if (smoothState == 0xFF) {
    // EEPROM non inizializzata, usa default (false)
    clockSmoothSeconds = false;
    EEPROM.write(EEPROM_CLOCK_SMOOTH_SECONDS_ADDR, 0);
    EEPROM.commit();
  } else {
    clockSmoothSeconds = (smoothState == 1);
  }
  Serial.printf("[SETUP] Smooth seconds: %s\n", clockSmoothSeconds ? "ABILITATO" : "DISABILITATO");

  // Carica lo stato rainbow hands dalla EEPROM
  uint8_t rainbowState = EEPROM.read(EEPROM_CLOCK_RAINBOW_ADDR);
  if (rainbowState == 0xFF) {
    // EEPROM non inizializzata, usa default (false)
    clockHandsRainbow = false;
    EEPROM.write(EEPROM_CLOCK_RAINBOW_ADDR, 0);
    EEPROM.commit();
  } else {
    clockHandsRainbow = (rainbowState == 1);
  }
  Serial.printf("[SETUP] Rainbow hands: %s\n", clockHandsRainbow ? "ABILITATO" : "DISABILITATO");

  // Carica nome skin attiva (32 bytes)
  Serial.println("[SETUP] Caricamento nome skin da EEPROM...");
  for (int i = 0; i < 32; i++) {
    clockActiveSkin[i] = EEPROM.read(EEPROM_CLOCK_SKIN_NAME_ADDR + i);
    if (clockActiveSkin[i] == 0 || clockActiveSkin[i] == 0xFF) {
      clockActiveSkin[i] = 0;
      break;
    }
  }
  clockActiveSkin[31] = 0; // Assicura terminazione stringa

  Serial.printf("[SETUP] Skin caricata da EEPROM: '%s'\n", clockActiveSkin);

  // Se vuoto o invalido, usa default
  if (clockActiveSkin[0] == 0 || clockActiveSkin[0] == 0xFF) {
    Serial.println("[SETUP] EEPROM vuoto o invalido, uso skin di default: 'orologio.jpg'");
    strcpy(clockActiveSkin, "orologio.jpg");
  }

  Serial.printf("[SETUP] Skin attiva finale: '%s'\n", clockActiveSkin);

  // NOTA: Colori lancette e posizione data NON vengono più caricati da EEPROM
  // Vengono caricati dal file .cfg specifico della skin quando viene caricata l'immagine
  // Questo permette ad ogni skin di avere la sua configurazione personalizzata
#endif

#ifdef EFFECT_BTTF
  // NOTA: La configurazione BTTF (date, coordinate, font sizes) viene caricata
  // automaticamente da /bttf_config.json su SD card quando si inizializza
  // la modalità BTTF tramite initBTTF() in 12_BTTF.ino
  Serial.println("[SETUP] Configurazione BTTF verrà caricata da SD all'attivazione modalità");
#endif

#ifdef EFFECT_LED_RING
  // Carica impostazioni LED Ring dalla EEPROM
  uint8_t ledRingValid = EEPROM.read(EEPROM_LEDRING_VALID_ADDR);
  if (ledRingValid == EEPROM_LEDRING_VALID_MARKER) {
    // Carica colori ore
    ledRingHoursR = EEPROM.read(EEPROM_LEDRING_HOURS_R_ADDR);
    ledRingHoursG = EEPROM.read(EEPROM_LEDRING_HOURS_G_ADDR);
    ledRingHoursB = EEPROM.read(EEPROM_LEDRING_HOURS_B_ADDR);
    // Carica colori minuti
    ledRingMinutesR = EEPROM.read(EEPROM_LEDRING_MINUTES_R_ADDR);
    ledRingMinutesG = EEPROM.read(EEPROM_LEDRING_MINUTES_G_ADDR);
    ledRingMinutesB = EEPROM.read(EEPROM_LEDRING_MINUTES_B_ADDR);
    // Carica colori secondi
    ledRingSecondsR = EEPROM.read(EEPROM_LEDRING_SECONDS_R_ADDR);
    ledRingSecondsG = EEPROM.read(EEPROM_LEDRING_SECONDS_G_ADDR);
    ledRingSecondsB = EEPROM.read(EEPROM_LEDRING_SECONDS_B_ADDR);
    // Carica colori display digitale
    ledRingDigitalR = EEPROM.read(EEPROM_LEDRING_DIGITAL_R_ADDR);
    ledRingDigitalG = EEPROM.read(EEPROM_LEDRING_DIGITAL_G_ADDR);
    ledRingDigitalB = EEPROM.read(EEPROM_LEDRING_DIGITAL_B_ADDR);
    // Carica flag rainbow
    ledRingHoursRainbow = (EEPROM.read(EEPROM_LEDRING_HOURS_RAINBOW) == 1);
    ledRingMinutesRainbow = (EEPROM.read(EEPROM_LEDRING_MINUTES_RAINBOW) == 1);
    ledRingSecondsRainbow = (EEPROM.read(EEPROM_LEDRING_SECONDS_RAINBOW) == 1);
    Serial.printf("[SETUP] LED Ring config caricata: Ore RGB(%d,%d,%d) Rainbow=%d, Min RGB(%d,%d,%d) Rainbow=%d, Sec RGB(%d,%d,%d) Rainbow=%d\n",
                  ledRingHoursR, ledRingHoursG, ledRingHoursB, ledRingHoursRainbow,
                  ledRingMinutesR, ledRingMinutesG, ledRingMinutesB, ledRingMinutesRainbow,
                  ledRingSecondsR, ledRingSecondsG, ledRingSecondsB, ledRingSecondsRainbow);
  } else {
    // Prima inizializzazione - salva valori default
    Serial.println("[SETUP] LED Ring prima inizializzazione - salvo valori default");
    EEPROM.write(EEPROM_LEDRING_HOURS_R_ADDR, ledRingHoursR);
    EEPROM.write(EEPROM_LEDRING_HOURS_G_ADDR, ledRingHoursG);
    EEPROM.write(EEPROM_LEDRING_HOURS_B_ADDR, ledRingHoursB);
    EEPROM.write(EEPROM_LEDRING_MINUTES_R_ADDR, ledRingMinutesR);
    EEPROM.write(EEPROM_LEDRING_MINUTES_G_ADDR, ledRingMinutesG);
    EEPROM.write(EEPROM_LEDRING_MINUTES_B_ADDR, ledRingMinutesB);
    EEPROM.write(EEPROM_LEDRING_SECONDS_R_ADDR, ledRingSecondsR);
    EEPROM.write(EEPROM_LEDRING_SECONDS_G_ADDR, ledRingSecondsG);
    EEPROM.write(EEPROM_LEDRING_SECONDS_B_ADDR, ledRingSecondsB);
    EEPROM.write(EEPROM_LEDRING_DIGITAL_R_ADDR, ledRingDigitalR);
    EEPROM.write(EEPROM_LEDRING_DIGITAL_G_ADDR, ledRingDigitalG);
    EEPROM.write(EEPROM_LEDRING_DIGITAL_B_ADDR, ledRingDigitalB);
    EEPROM.write(EEPROM_LEDRING_HOURS_RAINBOW, 0);
    EEPROM.write(EEPROM_LEDRING_MINUTES_RAINBOW, 0);
    EEPROM.write(EEPROM_LEDRING_SECONDS_RAINBOW, 0);
    EEPROM.write(EEPROM_LEDRING_VALID_ADDR, EEPROM_LEDRING_VALID_MARKER);
    EEPROM.commit();
  }
#endif

  loadSavedSettings(); // Chiama una funzione per caricare altre impostazioni salvate (se presenti).
}

void setup_display() {
  Serial.println("Setup Display"); // Stampa un messaggio sulla seriale.

  // Configurazione del pin di backlight con controllo PWM (Pulse Width Modulation).
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION); // Imposta il canale PWM, la frequenza e la risoluzione.
  ledcAttachPin(GFX_BL, PWM_CHANNEL);              // Associa il pin della backlight al canale PWM.

  gfx->begin(6600000);
  //gfx->begin(1000000);

  gfx->fillScreen(BLACK); // Riempi lo schermo di nero all'inizio.

  // Mostra il LOGO all'avvio.
  uint16_t* buffer = (uint16_t*)malloc(480 * 480 * 2);  // Alloca memoria RAM temporanea per l'immagine (2 byte per pixel).
  memcpy_P(buffer, sh_logo_480x480, 480 * 480 * 2);     // Copia i dati del logo dalla memoria flash (PROGMEM) alla RAM.
  gfx->draw16bitRGBBitmap(0, 0, buffer, 480, 480);      // Disegna l'immagine del logo sullo schermo.
  free(buffer);                                         // Libera la memoria RAM allocata per il buffer.
  gfx->setFont(u8g2_font_helvB08_tr);                 // Imposta un font per il testo.
  gfx->setCursor(10, 470);                             // Imposta la posizione del cursore per il testo.
  gfx->print("Versione Firmware: ");                   // Stampa l'etichetta.
  gfx->println(ino);                                   // Stampa la versione del firmware.

  backLightPwmFadeIn(); // Effettua un fade-in della backlight.
  delay(1000);
  backLightPwmFadeOut(); // Effettua un fade-out della backlight.
  gfx->fillScreen(BLACK); // Riempi nuovamente lo schermo di nero.

  gfx->setTextColor(WHITE);             // Imposta il colore del testo a bianco.
  gfx->setFont(u8g2_font_maniac_te);    // Imposta un font specifico.
  gfx->setCursor(120, 200);            // Imposta la posizione del cursore.
  gfx->println(F("ORAQUADRA NANO"));   // Stampa il nome del prodotto.
  gfx->setCursor(220, 270);            // Imposta un'altra posizione del cursore.
  gfx->println(F("BY"));               // Stampa "BY".

  backLightPwmFadeIn(); // Altro fade-in della backlight.
  delay(1000);
  backLightPwmFadeOut(); // Altro fade-out della backlight.
  gfx->fillScreen(BLACK);

  int duty = 0;
  // Esegue un'animazione di introduzione finché updateIntro() restituisce vero.
  while (!updateIntro()) {
    if(duty <= 250){
      ledcWrite(PWM_CHANNEL, duty); // Imposta il duty cycle della PWM per la backlight (aumento graduale).
      duty=duty+10;
    }
    delay(20);
  }
  delay(1000);
  backLightPwmFadeOut();
  gfx->fillScreen(BLACK);
  delay(100);


  ledcWrite(PWM_CHANNEL, 250); // Imposta la backlight a un livello fisso (250).
  gfx->setFont(u8g2_font_crox5hb_tr); // Imposta un font diverso.
  delay(100);
}

void backLightPwmFadeIn() {
  // Aumenta gradualmente la luminosità della backlight usando la PWM.
  for (int duty = 0; duty <= 250; duty++) {
    ledcWrite(PWM_CHANNEL, duty); // Imposta il duty cycle della PWM.
    delay(10);
  }
}

void backLightPwmFadeOut() {
  // Diminuisce gradualmente la luminosità della backlight usando la PWM.
  for (int duty = 250 ; duty >= 0; duty--) {
    ledcWrite(PWM_CHANNEL, duty); // Imposta il duty cycle della PWM.
    delay(10);
  }
}

void setup_touch() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // Inizializza la comunicazione I2C.
  ts.begin();                         // Inizializza il controller del touch screen.
  ts.setRotation(TOUCH_ROTATION);     // Imposta la rotazione del touch screen in base alla configurazione.
}


void setup_wifi() {

  // Imposta la modalità WiFi su STATION (necessario per Espalexa)
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // Disabilita risparmio energetico WiFi per Espalexa
  WiFi.disconnect();
  delay(100);

  displayWifiInit(); // Inizializza la visualizzazione per mostrare informazioni sul WiFi.

  gfx->setTextColor(WHITE);
  displayWordWifi(16, "CONFIGURAZIONE"); // Mostra "CONFIGURAZIONE".
  displayWordWifi(32, "WIFI:");         // Mostra "WIFI:".
  delay(250);

  // Verifica se ci sono credenziali WiFi salvate nella EEPROM.
  bool hasCredentials = false;
  char ssid[33] = {0};  // Buffer per memorizzare l'SSID (max 32 caratteri + terminatore null).
  char pass[65] = {0};  // Buffer per memorizzare la password (max 64 caratteri + terminatore null).

  // Legge un marker dalla EEPROM per verificare se le credenziali WiFi sono valide.
  if (EEPROM.read(EEPROM_WIFI_VALID_ADDR) == EEPROM_WIFI_VALID_VALUE) {
    // Se il marker è valido, leggi l'SSID e la password dalla EEPROM.
    for (int i = 0; i < 32; i++) {
      ssid[i] = EEPROM.read(EEPROM_WIFI_SSID_ADDR + i);
    }
    ssid[32] = 0; // Assicura che la stringa SSID sia terminata correttamente.

    for (int i = 0; i < 64; i++) {
      pass[i] = EEPROM.read(EEPROM_WIFI_PASS_ADDR + i);
    }
    pass[64] = 0; // Assicura che la stringa della password sia terminata correttamente.

    // Se l'SSID letto dalla EEPROM non è vuoto, prova a connettersi alla rete WiFi.
    if (strlen(ssid) > 0) {
      hasCredentials = true;
      gfx->setTextColor(GREEN);
      displayWordWifi(64, "CONNESSIONE A:"); // Mostra "CONNESSIONE A:".
      displayWordWifi(80, ssid);           // Mostra l'SSID.
      delay(500);

      // Tenta di connettersi alla rete WiFi con le credenziali salvate.
      WiFi.begin(ssid, pass);

      int retries = 0;

      // Attende la connessione o un timeout (60 tentativi * 250ms = 15 secondi).
      while (WiFi.status() != WL_CONNECTED && retries < 60) {
        delay(250);
        displayWifiDot(retries); // Mostra un punto animato durante il tentativo di connessione.
        retries++;

        // Se i tentativi superano un certo limite (30), riavvia il dispositivo.
        if (retries >= 30) {
          // Prima di riavviare, mostra un messaggio di errore.
          displayWifiInit();
          gfx->setTextColor(RED);
          displayWordWifi(96, "NON CONNESSO!");   // Mostra "NON CONNESSO!".
          displayWordWifi(128, "RIAVVIO IN CORSO"); // Mostra "RIAVVIO IN CORSO".
          delay(2000);
          ESP.restart();
         }

      }

      // Se la connessione WiFi ha successo, mostra un messaggio e esce dalla funzione.
      if (WiFi.status() == WL_CONNECTED) {
        // Assicurati che la modalità sia STATION
        WiFi.mode(WIFI_STA);

        gfx->setTextColor(BLUE);
        displayWordWifi(128, "CONNESSO!"); // Mostra "CONNESSO!".
        displayWordWifi(144, "IP:");       // Mostra "IP:".
        displayWordWifi(160, WiFi.localIP().toString()); // Mostra l'indirizzo IP.
        delay(2000);
        gfx->fillScreen(BLACK); // Pulisce lo schermo.
        return;
      }
    }
  }

  // Se si arriva qui, non ci sono credenziali salvate o la connessione è fallita.
  // Crea un'istanza della libreria WiFiManager per gestire la configurazione.
  WiFiManager wifiManager;

  // Imposta il timeout del portale di configurazione (Access Point) a 3 minuti (180 secondi).
  wifiManager.setConfigPortalTimeout(180);

  // Imposta una callback da eseguire quando le credenziali WiFi vengono salvate tramite il portale.
  wifiManager.setSaveConfigCallback([]() {
    // Questa lambda function viene chiamata dopo il salvataggio delle credenziali.
    String current_ssid = WiFi.SSID(); // Ottiene l'SSID a cui si è connesso.
    String current_pass = WiFi.psk();  // Ottiene la password della rete WiFi.

    // Salva l'SSID nella EEPROM.
    for (size_t i = 0; i < 32; i++) {
      EEPROM.write(EEPROM_WIFI_SSID_ADDR + i, (i < current_ssid.length()) ? current_ssid[i] : 0);
    }

    // Salva la password nella EEPROM.
    for (size_t i = 0; i < 64; i++) {
      EEPROM.write(EEPROM_WIFI_PASS_ADDR + i, (i < current_pass.length()) ? current_pass[i] : 0);
    }

    // Imposta il flag di validità delle credenziali WiFi nella EEPROM.
    EEPROM.write(EEPROM_WIFI_VALID_ADDR, EEPROM_WIFI_VALID_VALUE);

    // Scrive i dati dalla cache della EEPROM alla memoria fisica.
    EEPROM.commit();
  });

  // Genera un nome per l'Access Point (AP) basato sull'indirizzo MAC dell'ESP32.
  String apName = "OraQuadra_" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFF), HEX);

  // Disabilita la configurazione IP statica per forzare l'uso di DHCP.
  wifiManager.setSTAStaticIPConfig(IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0), IPAddress(0, 0, 0, 0));

  // Mostra QR code sul display per configurazione WiFi
  displayWiFiQRCode(apName);
  delay(1000);  // Mostra per 1 secondo prima di avviare WiFiManager

  // Avvia il portale di configurazione automatico. Tenta di connettersi a reti salvate,
  // altrimenti avvia un AP per la configurazione tramite browser.
  bool connected = wifiManager.autoConnect(apName.c_str());

  // Se la connessione fallisce dopo il timeout del portale.
  if (!connected) {
    gfx->setTextColor(RED);
    gfx->setCursor(120, 300);
    gfx->println(F("Errore connessione")); // Mostra "Errore connessione".
    gfx->setCursor(120, 330);
    gfx->println(F("Riavvio fra 5 secondi...")); // Mostra "Riavvio fra 5 secondi...".
    delay(5000);
    ESP.restart(); // Riavvia il dispositivo.
    return;
  }

  // Se si arriva qui, la connessione WiFi è stata stabilita.
  // Assicurati che la modalità sia STATION (WiFiManager può lasciarla in AP_STA)
  WiFi.mode(WIFI_STA);

  // Salva le credenziali correnti nella EEPROM (backup nel caso la callback non fosse chiamata).
  String current_ssid = WiFi.SSID();
  String current_pass = WiFi.psk();

  // Salva l'SSID nella EEPROM.
  for (size_t i = 0; i < 32; i++) {
    EEPROM.write(EEPROM_WIFI_SSID_ADDR + i, (i < current_ssid.length()) ? current_ssid[i] : 0);
  }

  // Salva la password nella EEPROM.
  for (size_t i = 0; i < 64; i++) {
    EEPROM.write(EEPROM_WIFI_PASS_ADDR + i, (i < current_pass.length()) ? current_pass[i] : 0);
  }

  // Imposta il flag di validità delle credenziali WiFi.
  EEPROM.write(EEPROM_WIFI_VALID_ADDR, EEPROM_WIFI_VALID_VALUE);

  // Scrive i dati nella EEPROM.
  EEPROM.commit();

  displayWifiInit(); // Reinizializza la visualizzazione WiFi.
  gfx->setTextColor(WHITE);
  displayWordWifi(16, "CONFIGURAZIONE"); // Mostra "CONFIGURAZIONE".
  displayWordWifi(32, "WIFI:");         // Mostra "WIFI:".
  gfx->setTextColor(GREEN);
  displayWordWifi(64, "CONNESSO A:"); // Mostra "CONNESSO A:".
  displayWordWifi(80, WiFi.SSID());   // Mostra l'SSID della rete connessa.
  gfx->setTextColor(BLUE);
  displayWordWifi(144, "IP:");       // Mostra "IP:".
  displayWordWifi(160, WiFi.localIP().toString()); // Mostra l'indirizzo IP assegnato.
  delay(2000);
  gfx->fillScreen(BLACK); // Pulisce lo schermo.
}

void setup_OTA() {
  // Configura gli aggiornamenti Over-The-Air (OTA) se la connessione WiFi è attiva.
  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.setHostname("ORAQUADRA"); // Imposta l'hostname per l'OTA.

    // Funzione da eseguire all'inizio dell'aggiornamento OTA.
    ArduinoOTA.onStart([]() {
      gfx->fillScreen(BLACK);
      gfx->setTextColor(WHITE);
      gfx->setFont(u8g2_font_inb21_mr);
      gfx->setCursor(120, 180);
      gfx->print(F("OTA UPDATE")); // Mostra "OTA UPDATE".
      gfx->drawRect(120, 240, 240, 30, WHITE); // Disegna un rettangolo per la barra di progresso.
    });

    // Funzione da eseguire durante l'aggiornamento OTA per mostrare la progressione.
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      uint8_t percentComplete = (progress / (total / 100)); // Calcola la percentuale di completamento.
      gfx->fillRect(122, 242, percentComplete * 236 / 100, 26, BLUE); // Riempi la barra di progresso.
      gfx->fillRect(350, 200, 320, 30, BLACK); // Cancella il testo della percentuale precedente.
      gfx->setCursor(350, 230);
      gfx->print(percentComplete); // Stampa la percentuale.
      gfx->print("%");
    });

    // Funzione da eseguire al termine dell'aggiornamento OTA.
    ArduinoOTA.onEnd([]() {
      gfx->fillScreen(GREEN);
      gfx->setTextColor(BLACK);
      gfx->setCursor(120, 200);
      gfx->print(F("UPDATE COMPLETATO")); // Mostra "UPDATE COMPLETATO".
      delay(2000);
    });

    // Funzione da eseguire in caso di errore durante l'aggiornamento OTA.
    ArduinoOTA.onError([](ota_error_t error) {
      gfx->fillScreen(RED);
      gfx->setTextColor(WHITE);
      gfx->setCursor(120, 180);
      gfx->print(F("ERRORE OTA")); // Mostra "ERRORE OTA".
      delay(3000);
      ESP.restart(); // Riavvia il dispositivo in caso di errore.
    });

    ArduinoOTA.begin(); // Inizializza il servizio OTA.
  }
}

void setup_alexa() {
  // Configura l'integrazione con Amazon Alexa se la connessione WiFi è attiva.
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Inizializzazione Espalexa...");

    // Inizializza mDNS (necessario per Espalexa)
    if (!MDNS.begin("oraquadra")) {
      Serial.println("Errore inizializzazione mDNS!");
      return; // Non proseguire se mDNS fallisce
    } else {
      Serial.println("mDNS inizializzato su oraquadra.local");
      // Annuncia servizio radardevice per auto-discovery dal radar server
      MDNS.addService("radardevice", "tcp", 8080);  // oraQuadraNano usa porta 8080
      MDNS.addServiceTxt("radardevice", "tcp", "name", "OraQuadraNano");
      MDNS.addServiceTxt("radardevice", "tcp", "type", "clock");
      MDNS.addServiceTxt("radardevice", "tcp", "port", "8080");
      Serial.println("mDNS: Servizio _radardevice._tcp annunciato (porta 8080)");
    }

    // Piccolo delay per stabilizzare mDNS
    delay(100);

    // Aggiunge un dispositivo Alexa con il nome specificato e la callback per il cambio colore.
    // Specifica il tipo come "extendedcolor" per supportare RGB
    EspalexaDevice* alexaDevice = new EspalexaDevice("ORAQUADRANANO", colorLightChanged, EspalexaDeviceType::extendedcolor);
    espalexa.addDevice(alexaDevice);

    // Inizializza il servizio ESPalexa (crea il suo server interno sulla porta 80)
    espalexa.begin();

    Serial.println("Espalexa inizializzato - Dispositivo: ORAQUADRANANO");
    Serial.print("IP dispositivo: ");
    Serial.println(WiFi.localIP());
    Serial.println("Pronto per discovery Alexa!");
  } else {
    Serial.println("WiFi non connesso - Espalexa disabilitato");
  }
}

void resetWiFiSettings() {
  // Funzione per resettare le impostazioni WiFi salvate.
  // Mostra un messaggio sullo schermo.
  gfx->fillScreen(RED);
  gfx->setTextColor(WHITE);
  gfx->setFont(u8g2_font_inb21_mr);
  gfx->setCursor(100, 180);
  gfx->println(F("RESET WIFI")); // Mostra "RESET WIFI".
  gfx->setCursor(100, 220);
  gfx->println(F("IN CORSO...")); // Mostra "IN CORSO...".

  // Crea un'istanza della libreria WiFiManager.
  WiFiManager wm;
  wm.resetSettings(); // Resetta le impostazioni WiFi memorizzate da WiFiManager.

  // Resetta anche il flag di validità delle credenziali WiFi nella EEPROM.
  EEPROM.write(EEPROM_WIFI_VALID_ADDR, 0);

  // Opzionalmente, cancella anche i dati di SSID e password dalla EEPROM.
  for (size_t i = 0; i < 32; i++) {
    EEPROM.write(EEPROM_WIFI_SSID_ADDR + i, 0);
  }

  for (size_t i = 0; i < 64; i++) {
    EEPROM.write(EEPROM_WIFI_PASS_ADDR + i, 0);
  }

  // Salva le modifiche nella EEPROM.
  EEPROM.commit();

  delay(2000);
  gfx->fillScreen(GREEN);
  gfx->setTextColor(BLACK);
  gfx->setCursor(100, 200);
  gfx->println(F("RESET COMPLETATO")); // Mostra "RESET COMPLETATO".
  delay(1000);

  ESP.restart(); // Riavvia il dispositivo dopo il reset del WiFi.
}

void loadSavedSettings() {
  // Funzione per caricare le impostazioni salvate dalla EEPROM.
  // Verifica se la EEPROM è stata configurata in precedenza.
  uint8_t configMarker = EEPROM.read(0);

  if (configMarker != EEPROM_CONFIGURED_MARKER) {
    // EEPROM non configurata, imposta i valori predefiniti.
    Serial.println("EEPROM non configurata, inizializzazione in corso...");

    // Valori predefiniti.
    currentMode = MODE_FAST;
    userMode = currentMode;
    currentPreset = 0;  // Preset 0 (random) - IMPORTANTE: assegnare anche la variabile!
    currentColor = Color(255, 255, 255);  // Bianco come colore predefinito.
    userColor = currentColor;

    // Salva i valori predefiniti nella EEPROM.
    EEPROM.write(0, EEPROM_CONFIGURED_MARKER);
    EEPROM.write(EEPROM_PRESET_ADDR, 0);  // Preset 0 (random).
    EEPROM.write(EEPROM_MODE_ADDR, currentMode);
    EEPROM.write(EEPROM_COLOR_R_ADDR, 255);
    EEPROM.write(EEPROM_COLOR_G_ADDR, 255);
    EEPROM.write(EEPROM_COLOR_B_ADDR, 255);
    EEPROM.commit();

    Serial.printf("[SETUP] Inizializzato con Mode=%d, Preset=%d, Color=Bianco\n", currentMode, currentPreset);
    return;
  }

  // Carica la modalità di visualizzazione salvata dalla EEPROM.
  uint8_t savedMode = EEPROM.read(EEPROM_MODE_ADDR);
  // Verifica che il mode sia sia nel range valido che effettivamente compilato (#ifdef)
  if (savedMode < NUM_MODES && isValidMode((DisplayMode)savedMode)) {
    currentMode = (DisplayMode)savedMode;
    userMode = currentMode;
  } else {
    // Imposta la modalità predefinita se il valore letto non è valido o il mode non è compilato.
    Serial.printf("Mode %d non valido o non compilato, uso MODE_FAST\n", savedMode);
    currentMode = MODE_FAST;
    userMode = currentMode;
    // Salva il mode corretto in EEPROM per evitare questo problema al prossimo riavvio
    EEPROM.write(EEPROM_MODE_ADDR, currentMode);
    EEPROM.commit();
  }

  // Carica il preset salvato dalla EEPROM
  uint8_t savedPreset = EEPROM.read(EEPROM_PRESET_ADDR);
  if (savedPreset <= 13) {
    currentPreset = savedPreset;
  } else {
    currentPreset = 0;  // Default
  }

  // Carica i valori dei colori salvati dalla EEPROM.
  uint8_t r = EEPROM.read(EEPROM_COLOR_R_ADDR);
  uint8_t g = EEPROM.read(EEPROM_COLOR_G_ADDR);
  uint8_t b = EEPROM.read(EEPROM_COLOR_B_ADDR);
  currentColor = Color(r, g, b);
  userColor = currentColor;  // Aggiorna anche la variabile userColor.

  // Informazioni di debug (opzionale).
  Serial.println("Impostazioni caricate da EEPROM:");
  Serial.print("Modalità: "); Serial.println(currentMode);
  Serial.print("Preset: "); Serial.println(currentPreset);
  Serial.print("Colore: R="); Serial.print(r);
  Serial.print(" G="); Serial.print(g);
  Serial.print(" B="); Serial.println(b);
}


void resetWiFi() {
  // Alias per la funzione resetWiFiSettings().
  resetWiFiSettings();
}

void colorLightChanged(EspalexaDevice* device) {
  // Callback per Amazon Alexa - NON BLOCCANTE
  if (device == nullptr) return;

  uint8_t brightness = device->getValue();

  Serial.printf("Alexa callback - Brightness: %d\n", brightness);

  if (brightness == 0) {
    alexaOff = 1; // Imposta un flag per indicare che Alexa ha spento la luce.
    alexaUpdatePending = true; // Segnala che serve un aggiornamento
    Serial.println("Alexa: dispositivo SPENTO");
    return;
  } else {
    alexaOff = 0; // Resetta il flag se la luce è accesa.
  }

  // Estrae i valori RGB dal dispositivo Espalexa
  userColor.r = device->getR();
  userColor.g = device->getG();
  userColor.b = device->getB();

  currentColor = userColor; // Aggiorna anche il colore corrente

  // NON chiamare forceDisplayUpdate() qui! Usa un flag invece
  alexaUpdatePending = true; // Segnala che serve un aggiornamento

  Serial.printf("Alexa color: R=%d G=%d B=%d\n", userColor.r, userColor.g, userColor.b);
}

void displayWiFiQRCode(String ssid) {
  Serial.println("Generazione QR Code WiFi...");

  // Prepara stringa QR WiFi (formato standard)
  String qrData = "WIFI:T:nopass;S:" + ssid + ";P:;;";

  // Crea QR code (versione 5 = 37x37 moduli) - allocazione dinamica per evitare stack overflow
  QRCode qrcode;
  uint8_t *qrcodeData = (uint8_t *)malloc(qrcode_getBufferSize(5));
  if (qrcodeData == NULL) {
    Serial.println("Errore allocazione memoria QR code");
    return;
  }
  qrcode_initText(&qrcode, qrcodeData, 5, 0, qrData.c_str());  // 0 = ECC_LOW

  // Layout per display 480x480
  int scale = 7;  // Ogni modulo QR = 7x7 pixel (37*7 = 259px)
  int qrSize = qrcode.size * scale;
  int offsetX = (480 - qrSize) / 2;  // Centro orizzontale
  int offsetY = 120;  // Margine superiore maggiore per dare spazio al testo

  // Sfondo nero
  gfx->fillScreen(BLACK);

  // Testo SOPRA il QR code - MINIMALE
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);

  // "CONFIG WiFi" 
  gfx->setCursor(10, 30);
  gfx->print("Configurazione WiFi");

  // "Scansiona QR code:" 
  gfx->setCursor(10, 50);
  gfx->print("Scansiona QR code:");

  // Box bianco per QR code
  gfx->fillRect(offsetX - 15, offsetY - 15, qrSize + 30, qrSize + 30, WHITE);

  // Disegna QR code
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        gfx->fillRect(
          offsetX + x * scale,
          offsetY + y * scale,
          scale,
          scale,
          BLACK
        );
      }
    }
  }

  // Nome rete sotto QR code 
  gfx->setTextColor(CYAN);
  gfx->setTextSize(1);
  gfx->setCursor(10, offsetY + qrSize + 60);
  gfx->print(ssid);

  // IP alternativo
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(10, offsetY + qrSize + 90);
  gfx->print("IP: 192.168.4.1");

  // Libera memoria allocata
  free(qrcodeData);

  Serial.println("QR Code generato!");
}

void clearDisplay() {
  // Cancella lo schermo riempiendolo con il colore di sfondo.
  gfx->fillScreen(BLACK);

  // Inizializza il buffer dei LED con il colore di sfondo.
  for (uint16_t idx = 0; idx < NUM_LEDS; idx++) {
    displayBuffer[idx] = convertColor(TextBackColor);
    pixelChanged[idx] = true;

    // Resetta anche gli array di stato dei pixel attivi.
    activePixels[idx] = false;
  }

  // Imposta tutti i caratteri sullo schermo con il colore di sfondo, di fatto "cancellandoli".
  gfx->setFont(u8g2_font_inb21_mr);
  for (uint16_t idx = 0; idx < NUM_LEDS; idx++) {
    gfx->setTextColor(displayBuffer[idx]);
    gfx->setCursor(pgm_read_word(&TFT_X[idx]), pgm_read_word(&TFT_Y[idx]));
    gfx->write(pgm_read_byte(&TFT_L[idx]));
  }
}

void setup_sd() {
  // Inizializza SD Card con pin personalizzati
  // IMPORTANTE: Chiamare DOPO setup_audio_wifi() per evitare conflitto PIN 41
  Serial.println("Inizializzazione SD Card...");

  // Messaggio sul display durante il controllo
  gfx->fillScreen(BLACK);
  gfx->setTextColor(CYAN);
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setCursor(80, 200);
  gfx->println("CONTROLLO SD CARD...");
  delay(500);

  SPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SPI)) {
    // ERRORE: SD Card non inizializzata
    Serial.println("Errore inizializzazione SD Card!");
    Serial.println("Verifica che la scheda SD sia inserita correttamente.");

    // Messaggio di ERRORE sul display
    gfx->fillScreen(BLACK);
    gfx->setTextColor(RED);
    gfx->setFont(u8g2_font_helvB18_tr);
    gfx->setCursor(50, 180);
    gfx->println("ERRORE SD CARD!");
    gfx->setFont(u8g2_font_helvB12_tr);
    gfx->setTextColor(YELLOW);
    gfx->setCursor(40, 250);
    gfx->println("Verifica inserimento");
    gfx->setCursor(90, 280);
    gfx->println("scheda microSD");
    delay(3000);

  } else {
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
      // NESSUNA SD Card rilevata
      Serial.println("Nessuna SD Card rilevata!");

      // Messaggio sul display: SD NON RILEVATA
      gfx->fillScreen(BLACK);
      gfx->setTextColor(ORANGE);
      gfx->setFont(u8g2_font_helvB18_tr);
      gfx->setCursor(30, 180);
      gfx->println("SD CARD NON RILEVATA");
      gfx->setFont(u8g2_font_helvB12_tr);
      gfx->setTextColor(YELLOW);
      gfx->setCursor(70, 250);
      gfx->println("Inserire scheda SD");
      gfx->setCursor(50, 280);
      gfx->println("per usare orologio");
      delay(3000);

    } else {
      // SD Card RILEVATA CON SUCCESSO
      String cardTypeStr = "";
      Serial.print("SD Card Type: ");
      if (cardType == CARD_MMC) {
        Serial.println("MMC");
        cardTypeStr = "MMC";
      } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
        cardTypeStr = "SDSC";
      } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
        cardTypeStr = "SDHC";
      } else {
        Serial.println("UNKNOWN");
        cardTypeStr = "UNKNOWN";
      }

      uint64_t cardSize = SD.cardSize() / (1024 * 1024);
      Serial.printf("SD Card Size: %lluMB\n", cardSize);
      Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
      Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));

#ifdef EFFECT_ANALOG_CLOCK
      // Imposta lo stato iniziale della SD Card come presente
      sdCardPresent = true;
      Serial.println("[SD SETUP] SD Card presente - monitoraggio attivo");
#endif

      // Messaggio di SUCCESSO sul display
      gfx->fillScreen(BLACK);
      gfx->setTextColor(GREEN);
      gfx->setFont(u8g2_font_helvB18_tr);
      gfx->setCursor(60, 150);
      gfx->println("SD CARD OK!");

      gfx->setFont(u8g2_font_helvB12_tr);
      gfx->setTextColor(WHITE);
      gfx->setCursor(120, 200);
      gfx->print("Tipo: ");
      gfx->println(cardTypeStr);

      gfx->setCursor(80, 230);
      gfx->print("Dimensione: ");
      gfx->print((uint32_t)cardSize);
      gfx->println(" MB");

      uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
      gfx->setCursor(80, 260);
      gfx->print("Spazio usato: ");
      gfx->print((uint32_t)usedBytes);
      gfx->println(" MB");

      delay(2000);

      // Lista i file nella root della SD card
      Serial.println("\n=== File nella root della SD Card ===");
      File root = SD.open("/");
      if (root) {
        File file = root.openNextFile();
        while (file) {
          if (!file.isDirectory()) {
            Serial.print("  - ");
            Serial.print(file.name());
            Serial.print(" (");
            Serial.print(file.size());
            Serial.println(" bytes)");
          }
          file = root.openNextFile();
        }
        root.close();
        Serial.println("=====================================\n");
      } else {
        Serial.println("Errore apertura root SD!");
      }
    }
  }

  // PULIZIA CACHE IMMAGINE - Cancella eventuali dati residui nel buffer PSRAM
  // per evitare immagini latenti quando la SD non è presente
  #ifdef EFFECT_ANALOG_CLOCK
  if (clockBackgroundBuffer != nullptr) {
    Serial.println("[CLEANUP] Cancellazione buffer PSRAM immagine...");
    // Riempi tutto il buffer con nero (0x0000 in RGB565)
    memset(clockBackgroundBuffer, 0, CLOCK_BUFFER_SIZE * sizeof(uint16_t));
    Serial.println("[CLEANUP] Buffer PSRAM azzerato");
  }
  #endif

  // Pulisce lo schermo dopo i messaggi di setup SD
  gfx->fillScreen(BLACK);
  Serial.println("[CLEANUP] Schermo pulito - Setup SD completato");
}

void setup_radar() {
  // Inizializza il radar LD2410 (MyLD2410) per rilevamento presenza e luminosità
  // COMPATIBILE con audio WiFi esterno (ESP32C3) - nessun conflitto pin
  Serial.println("\n========================================");
  Serial.println("INIZIALIZZAZIONE RADAR LD2410 (MyLD2410)");
  Serial.println("========================================");

  Serial.print("Pin RX (ESP32): ");
  Serial.print(LD2410_RX_PIN);
  Serial.println(" (collegato a TX del radar)");

  Serial.print("Pin TX (ESP32): ");
  Serial.print(LD2410_TX_PIN);
  Serial.println(" (collegato a RX del radar)");

  Serial.println("\nCollegamenti richiesti:");
  Serial.println("  LD2410 VCC  -> ESP32 5V (o 3.3V)");
  Serial.println("  LD2410 GND  -> ESP32 GND");
  Serial.println("  LD2410 TX   -> ESP32 Pin 2 (IO2 - RX radar)");
  Serial.println("  LD2410 RX   -> ESP32 Pin 1 (IO1 - TX radar)");
  Serial.println("\nNOTA: Pin IO1/IO2 compatibili con audio WiFi esterno (ESP32C3)");

  // ========== INIZIALIZZAZIONE RADAR (MyLD2410) ==========
  Serial.println("\nInizializzazione UART1...");
  radarSerial.begin(LD2410_BAUD_RATE, SERIAL_8N1, LD2410_RX_PIN, LD2410_TX_PIN);
  delay(500);

  Serial.println("Connessione al radar...");
  if (radar.begin()) {
    Serial.println("\n>>> RADAR LD2410 CONNESSO! <<<");
    radarAvailable = true;
    radarConnectedOnce = true;

    // Inizializza presenza come rilevata per tenere display acceso all'avvio
    // Il timeout di 30 secondi partirà da qui
    presenceDetected = true;
    lastPresenceTime = millis();
    Serial.println("Display: ACCESO (presenza iniziale simulata)");

    // Richiedi versione firmware
    Serial.println("\nRichiesta versione firmware...");
    if (radar.requestFirmware()) {
      Serial.print(">>> Firmware: ");
      Serial.print(radar.getFirmware());
      Serial.println(" <<<");
    } else {
      Serial.println("ATTENZIONE: Impossibile leggere versione firmware");
    }

    // Richiedi MAC address
    Serial.println("\nRichiesta MAC address...");
    if (radar.requestMAC()) {
      Serial.print(">>> MAC: ");
      Serial.print(radar.getMACstr());
      Serial.println(" <<<");
    }

    // Richiedi configurazione corrente
    Serial.println("\nRichiesta configurazione corrente...");
    if (radar.requestParameters()) {
      Serial.println(">>> Configurazione radar:");
      Serial.print("  - Gate massimo: ");
      Serial.println(radar.getRange());
      Serial.print("  - Distanza max: ");
      Serial.print(radar.getRange_cm());
      Serial.println(" cm");
      Serial.print("  - Timeout inattività: ");
      Serial.print(radar.getNoOneWindow());
      Serial.println(" secondi");
      Serial.println("<<< Configurazione caricata");
    } else {
      Serial.println("ATTENZIONE: Impossibile leggere configurazione");
    }

    // Configurazione sensibilità radar
    Serial.println("\nConfigurazione parametri radar...");
    Serial.println("Riduzione sensibilità per evitare rilevamenti di pareti/oggetti");

    // max_moving_gate: 6 (circa 4.5 metri), max_stationary_gate: 4 (circa 3m), timeout: 5s
    if (radar.setMaxGate(6, 4, 5)) {
      Serial.println("✓ Parametri radar configurati:");
      Serial.println("  - Gate movimento: 6 (~4.5m)");
      Serial.println("  - Gate statico: 4 (~3m)");
      Serial.println("  - Timeout: 5s");
    } else {
      Serial.println("⚠ Impossibile configurare parametri");
    }

    // ========== ATTIVA ENHANCED MODE ==========
    // Modalità avanzata con dati dettagliati su ogni gate
    Serial.println("\n>>> ATTIVAZIONE ENHANCED MODE <<<");
    if (radar.enhancedMode(true)) {
      Serial.println("Enhanced Mode ATTIVO!");
      Serial.println("Dati disponibili:");
      Serial.println("  - Segnali per ogni gate");
      Serial.println("  - Distanza precisa target");
      Serial.println("  - Livello luminosità (getLightLevel())");
    } else {
      Serial.println("ATTENZIONE: Impossibile attivare Enhanced Mode");
    }

    // Richiedi configurazione ausiliaria (luce)
    Serial.println("\nRichiesta configurazione ausiliaria...");
    if (radar.requestAuxConfig()) {
      Serial.print(">>> Livello luminosità: ");
      Serial.println(radar.getLightLevel());
      Serial.print(">>> Controllo luce: ");
      Serial.println((int)radar.getLightControl());
      Serial.print(">>> Soglia luce: ");
      Serial.println(radar.getLightThreshold());
    }

    // L'impostazione radarBrightnessControl viene caricata da initSetupOptions()
    Serial.print("\nControllo luminosità radar: ");
    Serial.println(radarBrightnessControl ? "ABILITATO (Menu Setup)" : "DISABILITATO (Menu Setup)");

    Serial.println("\n>>> RADAR PRONTO PER IL RILEVAMENTO <<<");
    Serial.println("Libreria: MyLD2410");
    Serial.println("Modalità: Enhanced (Advanced)");
    Serial.println("Supporto luminosità: SÌ (getLightLevel)");
    Serial.println("\n--- GESTIONE LUMINOSITÀ DISPLAY ---");
    if (radarBrightnessControl) {
      Serial.println("AUTOMATICA: Sensore luce radar");
      Serial.println("  - Buio (<50): 90");
      Serial.println("  - Poco luce (<100): 120");
      Serial.println("  - Media luce (<150): 180");
      Serial.println("  - Molta luce (≥150): 250");
    } else {
      Serial.println("MANUALE: Basata su ora");
      Serial.println("  - Notte (19-7): 90");
      Serial.println("  - Giorno (7-19): 250");
    }
    Serial.println("\nCambia modalità: Menu Setup → Radar Brightness");
    Serial.println("========================================\n");

  } else {
    Serial.println("\n>>> RADAR: NON TROVATO <<<");
    Serial.println("Il sistema funzionera' comunque senza radar.");
    Serial.println("Luminosita' display: modalita' GIORNO/NOTTE automatica");
    Serial.println("\nSe vuoi usare il radar, verifica:");
    Serial.println("  1. LED sul radar acceso?");
    Serial.println("  2. Alimentazione 5V stabile");
    Serial.println("  3. TX radar -> Pin 2 (IO2)");
    Serial.println("  4. RX radar -> Pin 1 (IO1)");
    Serial.println("========================================\n");
    radarAvailable = false;  // Radar non disponibile - sistema funziona senza
    radarConnectedOnce = false;
  }
}

void setup_bme280() {
  // Inizializza sensore BME280 per temperatura, umidità e pressione interna (opzionale)
  Serial.println("\n========================================");
  Serial.println("INIZIALIZZAZIONE SENSORE BME280");
  Serial.println("========================================");

  bme280Available = false;  // Inizializza come non disponibile

  // Carica offset calibrazione da EEPROM
  if (EEPROM.read(EEPROM_BME280_CALIB_VALID_ADDR) == EEPROM_BME280_CALIB_VALID_VALUE) {
    // Leggi offset temperatura (int16 * 10)
    int16_t tempOffsetRaw = (int16_t)((EEPROM.read(EEPROM_BME280_TEMP_OFFSET_ADDR) << 8) |
                                       EEPROM.read(EEPROM_BME280_TEMP_OFFSET_ADDR + 1));
    bme280TempOffset = tempOffsetRaw / 10.0f;

    // Leggi offset umidità (int16 * 10)
    int16_t humOffsetRaw = (int16_t)((EEPROM.read(EEPROM_BME280_HUM_OFFSET_ADDR) << 8) |
                                      EEPROM.read(EEPROM_BME280_HUM_OFFSET_ADDR + 1));
    bme280HumOffset = humOffsetRaw / 10.0f;

    Serial.printf("[BME280] Offset calibrazione caricati: Temp=%.1f°C, Hum=%.1f%%\n",
                  bme280TempOffset, bme280HumOffset);
  } else {
    // EEPROM non inizializzata, usa valori di default (0)
    bme280TempOffset = 0.0f;
    bme280HumOffset = 0.0f;
    Serial.println("[BME280] Nessuna calibrazione salvata, offset a 0");
  }

  // Prova entrambi gli indirizzi I2C comuni per BME280
  try {
    Serial.println("Ricerca sensore BME280 su bus I2C...");

    // Prova prima con indirizzo 0x76
    if (bme.begin(0x76)) {
      bme280Available = true;
      Serial.println(">>> BME280 trovato all'indirizzo 0x76! <<<");
    }
    // Se fallisce, prova con indirizzo 0x77
    else if (bme.begin(0x77)) {
      bme280Available = true;
      Serial.println(">>> BME280 trovato all'indirizzo 0x77! <<<");
    }

    // Se trovato, configura il sensore
    if (bme280Available) {
      bme.setSampling(Adafruit_BME280::MODE_NORMAL,     // Modalità operativa
                      Adafruit_BME280::SAMPLING_X2,     // Temp. oversampling
                      Adafruit_BME280::SAMPLING_X16,    // Pressure oversampling
                      Adafruit_BME280::SAMPLING_X16,    // Humidity oversampling
                      Adafruit_BME280::FILTER_X16,      // Filtering
                      Adafruit_BME280::STANDBY_MS_500); // Standby time

      Serial.println("\nConfigurazione sensore BME280 completata!");
      Serial.println("Dati disponibili:");
      Serial.println("  - Temperatura interna (°C)");
      Serial.println("  - Umidità interna (%)");
      Serial.println("  - Pressione atmosferica (hPa)");

      // Prima lettura test
      Serial.println("\nPrima lettura sensore:");
      temperatureIndoor = bme.readTemperature();
      humidityIndoor = bme.readHumidity();
      pressureIndoor = bme.readPressure() / 100.0F;  // Converte Pa in hPa

      Serial.print("  Temperatura: ");
      Serial.print(temperatureIndoor, 1);
      Serial.println(" °C");
      Serial.print("  Umidità: ");
      Serial.print(humidityIndoor, 1);
      Serial.println(" %");
      Serial.print("  Pressione: ");
      Serial.print(pressureIndoor, 1);
      Serial.println(" hPa");
    } else {
      Serial.println("\n>>> BME280 NON TROVATO <<<");
      Serial.println("Indirizzi testati: 0x76, 0x77");
      Serial.println("Verifica:");
      Serial.println("  1. Collegamento I2C (SDA=19, SCL=45)");
      Serial.println("  2. Alimentazione sensore (3.3V o 5V)");
      Serial.println("  3. Indirizzo I2C corretto");
      Serial.println("\nL'orologio continuerà normalmente");
      Serial.println("Temperatura/Umidità mostrerà: N.D.");
    }
  }
  catch (...) {
    // Gestisce eventuali errori durante l'inizializzazione
    bme280Available = false;
    Serial.println("\n>>> ERRORE durante inizializzazione BME280 <<<");
    Serial.println("Sensore ignorato - L'orologio continuerà normalmente");
  }

  Serial.println("========================================\n");
}
