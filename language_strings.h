// =========================================================================
// language_strings.h - Stringhe UI Multilingua per OraQuadra
// =========================================================================
//
// Contiene tutte le stringhe dell'interfaccia utente in più lingue.
// Usare con le macro STR() e STR_SINGLE() definite in language_config.h
//
// NOTA: Sistema NON attivo di default.
// Per attivare: decommentare #define ENABLE_MULTILANGUAGE in language_config.h
//
// =========================================================================

#ifndef LANGUAGE_STRINGS_H
#define LANGUAGE_STRINGS_H

#include <Arduino.h>
#include "language_config.h"

// ===============================================
// GIORNI DELLA SETTIMANA
// ===============================================
// Indice: 0=Domenica, 1=Lunedì, ..., 6=Sabato

const char* const DAYS_OF_WEEK[][7] PROGMEM = {
    // LANG_IT (Italiano)
    {"Domenica", "Lunedi", "Martedi", "Mercoledi", "Giovedi", "Venerdi", "Sabato"},
    // LANG_EN (English)
    {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}
};

// Versione abbreviata (3 lettere)
const char* const DAYS_SHORT[][7] PROGMEM = {
    // LANG_IT
    {"Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"},
    // LANG_EN
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}
};

// ===============================================
// MESI DELL'ANNO
// ===============================================
// Indice: 0=Gennaio, ..., 11=Dicembre

const char* const MONTHS[][12] PROGMEM = {
    // LANG_IT (Italiano)
    {"Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno",
     "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre"},
    // LANG_EN (English)
    {"January", "February", "March", "April", "May", "June",
     "July", "August", "September", "October", "November", "December"}
};

// Versione abbreviata (3 lettere)
const char* const MONTHS_SHORT[][12] PROGMEM = {
    // LANG_IT
    {"Gen", "Feb", "Mar", "Apr", "Mag", "Giu", "Lug", "Ago", "Set", "Ott", "Nov", "Dic"},
    // LANG_EN
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}
};

// ===============================================
// DESCRIZIONI METEO
// ===============================================
// Codici OpenWeatherMap semplificati

const char* const WEATHER_CLEAR[] PROGMEM = {"Sereno", "Clear"};
const char* const WEATHER_CLOUDS_FEW[] PROGMEM = {"Poco nuvoloso", "Few clouds"};
const char* const WEATHER_CLOUDS_SCATTERED[] PROGMEM = {"Parzialmente nuvoloso", "Partly cloudy"};
const char* const WEATHER_CLOUDS[] PROGMEM = {"Nuvoloso", "Cloudy"};
const char* const WEATHER_OVERCAST[] PROGMEM = {"Coperto", "Overcast"};
const char* const WEATHER_RAIN_LIGHT[] PROGMEM = {"Pioggia leggera", "Light rain"};
const char* const WEATHER_RAIN[] PROGMEM = {"Pioggia", "Rain"};
const char* const WEATHER_RAIN_HEAVY[] PROGMEM = {"Pioggia forte", "Heavy rain"};
const char* const WEATHER_DRIZZLE[] PROGMEM = {"Pioggerella", "Drizzle"};
const char* const WEATHER_THUNDERSTORM[] PROGMEM = {"Temporale", "Thunderstorm"};
const char* const WEATHER_SNOW_LIGHT[] PROGMEM = {"Neve leggera", "Light snow"};
const char* const WEATHER_SNOW[] PROGMEM = {"Neve", "Snow"};
const char* const WEATHER_SNOW_HEAVY[] PROGMEM = {"Neve forte", "Heavy snow"};
const char* const WEATHER_FOG[] PROGMEM = {"Nebbia", "Fog"};
const char* const WEATHER_MIST[] PROGMEM = {"Foschia", "Mist"};

// ===============================================
// MENU SETUP
// ===============================================

const char* const STR_SETUP[] PROGMEM = {"SETUP", "SETUP"};
const char* const STR_TOUCH_TO_CHANGE[] PROGMEM = {"Tocca per cambiare", "Touch to change"};
const char* const STR_SETUP_ERROR[] PROGMEM = {"ERRORE SETUP", "SETUP ERROR"};

// Voci menu setup
const char* const STR_AUTO_NIGHT_MODE[] PROGMEM = {"Auto Night Mode", "Auto Night Mode"};
const char* const STR_TOUCH_SOUNDS[] PROGMEM = {"Suoni Touch", "Touch Sounds"};
const char* const STR_POWER_SAVE[] PROGMEM = {"Risparmio Energia", "Power Save"};
const char* const STR_RADAR_BRIGHTNESS[] PROGMEM = {"Radar Luminosita", "Radar Brightness"};
const char* const STR_DISPLAY_MODE[] PROGMEM = {"Modalita Display", "Display Mode"};
const char* const STR_LANGUAGE[] PROGMEM = {"Lingua", "Language"};

// Stati ON/OFF
const char* const STR_ON[] PROGMEM = {"ON", "ON"};
const char* const STR_OFF[] PROGMEM = {"OFF", "OFF"};

// ===============================================
// MESSAGGI DI SISTEMA
// ===============================================

const char* const STR_CONFIGURATION[] PROGMEM = {"CONFIGURAZIONE", "CONFIGURATION"};
const char* const STR_RESTARTING[] PROGMEM = {"RIAVVIO IN CORSO", "RESTARTING"};
const char* const STR_CONNECTION_ERROR[] PROGMEM = {"Errore connessione", "Connection error"};
const char* const STR_RESTART_IN_5SEC[] PROGMEM = {"Riavvio fra 5 secondi...", "Restarting in 5 seconds..."};
const char* const STR_WIFI_CONFIG[] PROGMEM = {"Configurazione WiFi", "WiFi Configuration"};
const char* const STR_RESET_WIFI[] PROGMEM = {"RESET WIFI", "RESET WIFI"};
const char* const STR_IN_PROGRESS[] PROGMEM = {"IN CORSO...", "IN PROGRESS..."};
const char* const STR_RESET_COMPLETE[] PROGMEM = {"RESET COMPLETATO", "RESET COMPLETE"};

// OTA Update
const char* const STR_OTA_UPDATE[] PROGMEM = {"OTA UPDATE", "OTA UPDATE"};
const char* const STR_UPDATE_COMPLETE[] PROGMEM = {"UPDATE COMPLETATO", "UPDATE COMPLETE"};
const char* const STR_OTA_ERROR[] PROGMEM = {"ERRORE OTA", "OTA ERROR"};

// SD Card
const char* const STR_SD_NOT_FOUND[] PROGMEM = {"SD CARD NON RILEVATA", "SD CARD NOT FOUND"};
const char* const STR_INSERT_SD[] PROGMEM = {"Inserire scheda SD", "Insert SD card"};
const char* const STR_FOR_CLOCK[] PROGMEM = {"per usare orologio", "to use clock"};
const char* const STR_CHECK_SD[] PROGMEM = {"Verifica inserimento", "Check insertion"};
const char* const STR_MICROSD[] PROGMEM = {"scheda microSD", "microSD card"};

// ===============================================
// WEBSERVER / SETTINGS
// ===============================================

const char* const STR_SETTINGS[] PROGMEM = {"Impostazioni", "Settings"};
const char* const STR_BRIGHTNESS[] PROGMEM = {"Luminosita", "Brightness"};
const char* const STR_DAY[] PROGMEM = {"Giorno", "Day"};
const char* const STR_NIGHT[] PROGMEM = {"Notte", "Night"};
const char* const STR_SAVE[] PROGMEM = {"Salva", "Save"};
const char* const STR_CANCEL[] PROGMEM = {"Annulla", "Cancel"};
const char* const STR_APPLY[] PROGMEM = {"Applica", "Apply"};

// ===============================================
// EFFETTI / MODALITA
// ===============================================

const char* const STR_MODE_FADE[] PROGMEM = {"FADE", "FADE"};
const char* const STR_MODE_SLOW[] PROGMEM = {"SLOW", "SLOW"};
const char* const STR_MODE_FAST[] PROGMEM = {"FAST", "FAST"};
const char* const STR_MODE_MATRIX[] PROGMEM = {"MATRIX", "MATRIX"};
const char* const STR_MODE_SNAKE[] PROGMEM = {"SNAKE", "SNAKE"};
const char* const STR_MODE_WATER[] PROGMEM = {"WATER", "WATER"};

// ===============================================
// FUNZIONE HELPER: Ottieni descrizione meteo
// ===============================================
// Restituisce la stringa meteo nella lingua corrente

inline const char* getWeatherString(int code) {
    #ifdef ENABLE_MULTILANGUAGE
        uint8_t lang = currentLanguage;
    #else
        uint8_t lang = 0;
    #endif

    if (code >= 200 && code < 300) return WEATHER_THUNDERSTORM[lang];
    if (code >= 300 && code < 400) return WEATHER_DRIZZLE[lang];
    if (code >= 400 && code < 500) return WEATHER_RAIN[lang];
    if (code >= 500 && code < 600) {
        if (code == 500) return WEATHER_RAIN_LIGHT[lang];
        if (code == 501) return WEATHER_RAIN[lang];
        if (code >= 502) return WEATHER_RAIN_HEAVY[lang];
        return WEATHER_RAIN[lang];
    }
    if (code >= 600 && code < 700) {
        if (code == 600) return WEATHER_SNOW_LIGHT[lang];
        if (code == 601) return WEATHER_SNOW[lang];
        if (code >= 602) return WEATHER_SNOW_HEAVY[lang];
        return WEATHER_SNOW[lang];
    }
    if (code >= 700 && code < 800) {
        if (code == 701 || code == 741) return WEATHER_FOG[lang];
        if (code == 721) return WEATHER_MIST[lang];
        return WEATHER_FOG[lang];
    }
    if (code == 800) return WEATHER_CLEAR[lang];
    if (code == 801) return WEATHER_CLOUDS_FEW[lang];
    if (code == 802) return WEATHER_CLOUDS_SCATTERED[lang];
    if (code == 803) return WEATHER_CLOUDS[lang];
    if (code == 804) return WEATHER_OVERCAST[lang];

    return WEATHER_CLEAR[lang];  // Default
}

// ===============================================
// ESEMPI DI UTILIZZO
// ===============================================
//
// Per ottenere un giorno della settimana:
//   const char* day = STR(DAYS_OF_WEEK, dayIndex);
//   // oppure: DAYS_OF_WEEK[currentLanguage][dayIndex]
//
// Per ottenere un mese:
//   const char* month = STR(MONTHS, monthIndex);
//
// Per stringhe singole:
//   gfx->print(STR_SINGLE(STR_SETUP));
//   // oppure: STR_SETUP[currentLanguage]
//
// Per descrizione meteo:
//   const char* weather = getWeatherString(weatherCode);
//
// ===============================================

#endif // LANGUAGE_STRINGS_H
