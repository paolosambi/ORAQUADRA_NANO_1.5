// =========================================================================
// translations.h - Tabella Traduzioni Centralizzata per OraQuadra
// =========================================================================
//
// COME USARE:
// 1. Includere questo file: #include "translations.h"
// 2. Usare la macro TR(ID) per ottenere la stringa tradotta
//    Esempio: gfx->print(TR(TXT_WEATHER_STATION));
//
// COME AGGIUNGERE NUOVE STRINGHE:
// 1. Aggiungere l'ID nell'enum TextID
// 2. Aggiungere le traduzioni nell'array TRANSLATIONS[]
//    nell'ESATTO STESSO ORDINE dell'enum
//
// COME AGGIUNGERE NUOVE LINGUE:
// 1. Aggiungere la lingua in language_config.h (enum Language)
// 2. Aggiungere la colonna nell'array TRANSLATIONS[]
//
// =========================================================================

#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#include "language_config.h"

// ===============================================
// ENUM ID TESTI - Aggiungi qui nuovi ID
// ===============================================
enum TextID {
    // ----- WEATHER STATION -----
    TXT_WEATHER_STATION,        // "STAZIONE METEO" / "WEATHER STATION"
    TXT_DATE_TIME,              // "DATA E ORA" / "DATE & TIME"
    TXT_CONDITIONS,             // "CONDIZIONI" / "CONDITIONS"
    TXT_TEMPERATURES,           // "TEMPERATURE" / "TEMPERATURES"
    TXT_WIND,                   // "VENTO" / "WIND"
    TXT_DETAILS,                // "DETTAGLI" / "DETAILS"
    TXT_TOMORROW,               // "DOMANI" / "TOMORROW"
    TXT_SUN,                    // "SOLE" / "SUN"
    TXT_INDOOR,                 // "INTERNO:" / "INDOOR:"
    TXT_OUTDOOR,                // "ESTERNO:" / "OUTDOOR:"
    TXT_FEELS_LIKE,             // "PERCEP.:" / "FEELS:"
    TXT_PRESSURE,               // "Pressione:" / "Pressure:"
    TXT_CLOUDS,                 // "Nuvole:" / "Clouds:"
    TXT_VISIBILITY,             // "Visibilita:" / "Visibility:"
    TXT_HUMIDITY,               // "Umidita:" / "Humidity:"
    TXT_NOT_AVAILABLE,          // "N.D." / "N/A"
    TXT_CALIBRATION,            // "CALIBRAZIONE" / "CALIBRATION"

    // ----- GIORNI ABBREVIATI -----
    TXT_DAY_SUN,                // "Dom" / "Sun"
    TXT_DAY_MON,                // "Lun" / "Mon"
    TXT_DAY_TUE,                // "Mar" / "Tue"
    TXT_DAY_WED,                // "Mer" / "Wed"
    TXT_DAY_THU,                // "Gio" / "Thu"
    TXT_DAY_FRI,                // "Ven" / "Fri"
    TXT_DAY_SAT,                // "Sab" / "Sat"

    // ----- MESI ABBREVIATI -----
    TXT_MON_JAN,                // "Gen" / "Jan"
    TXT_MON_FEB,                // "Feb" / "Feb"
    TXT_MON_MAR,                // "Mar" / "Mar"
    TXT_MON_APR,                // "Apr" / "Apr"
    TXT_MON_MAY,                // "Mag" / "May"
    TXT_MON_JUN,                // "Giu" / "Jun"
    TXT_MON_JUL,                // "Lug" / "Jul"
    TXT_MON_AUG,                // "Ago" / "Aug"
    TXT_MON_SEP,                // "Set" / "Sep"
    TXT_MON_OCT,                // "Ott" / "Oct"
    TXT_MON_NOV,                // "Nov" / "Nov"
    TXT_MON_DEC,                // "Dic" / "Dec"

    // ----- DIREZIONI VENTO -----
    TXT_WIND_N,                 // "N" / "N"
    TXT_WIND_NE,                // "NE" / "NE"
    TXT_WIND_E,                 // "E" / "E"
    TXT_WIND_SE,                // "SE" / "SE"
    TXT_WIND_S,                 // "S" / "S"
    TXT_WIND_SW,                // "SO" / "SW"
    TXT_WIND_W,                 // "O" / "W"
    TXT_WIND_NW,                // "NO" / "NW"

    // ----- PUNTI CARDINALI BUSSOLA -----
    TXT_COMPASS_N,              // "N" / "N"
    TXT_COMPASS_S,              // "S" / "S"
    TXT_COMPASS_E,              // "E" / "E"
    TXT_COMPASS_W,              // "O" / "W"

    // ----- FLIP CLOCK -----
    TXT_FC_INDOOR,              // "INT:" / "IN:"
    TXT_FC_OUTDOOR,             // "EST:" / "OUT:"
    TXT_FC_TOMORROW,            // "DOM:" / "TOM:"

    // Contatore (DEVE essere ultimo!)
    TXT_COUNT
};

// ===============================================
// TABELLA TRADUZIONI
// Ordine colonne: [LANG_IT] [LANG_EN]
// ===============================================
static const char* const TRANSLATIONS[][2] PROGMEM = {
    // TXT_WEATHER_STATION
    {"STAZIONE METEO", "WEATHER STATION"},
    // TXT_DATE_TIME
    {"DATA E ORA", "DATE & TIME"},
    // TXT_CONDITIONS
    {"CONDIZIONI", "CONDITIONS"},
    // TXT_TEMPERATURES
    {"TEMPERATURE", "TEMPERATURES"},
    // TXT_WIND
    {"VENTO", "WIND"},
    // TXT_DETAILS
    {"DETTAGLI", "DETAILS"},
    // TXT_TOMORROW
    {"DOMANI", "TOMORROW"},
    // TXT_SUN
    {"SOLE", "SUN"},
    // TXT_INDOOR
    {"INTERNO:", "INDOOR:"},
    // TXT_OUTDOOR
    {"ESTERNO:", "OUTDOOR:"},
    // TXT_FEELS_LIKE
    {"PERCEP.:", "FEELS:"},
    // TXT_PRESSURE
    {"Pressione:", "Pressure:"},
    // TXT_CLOUDS
    {"Nuvole:", "Clouds:"},
    // TXT_VISIBILITY
    {"Visibilita:", "Visibility:"},
    // TXT_HUMIDITY
    {"Umidita:", "Humidity:"},
    // TXT_NOT_AVAILABLE
    {"N.D.", "N/A"},
    // TXT_CALIBRATION
    {"CALIBRAZIONE", "CALIBRATION"},

    // ----- GIORNI ABBREVIATI -----
    // TXT_DAY_SUN
    {"Dom", "Sun"},
    // TXT_DAY_MON
    {"Lun", "Mon"},
    // TXT_DAY_TUE
    {"Mar", "Tue"},
    // TXT_DAY_WED
    {"Mer", "Wed"},
    // TXT_DAY_THU
    {"Gio", "Thu"},
    // TXT_DAY_FRI
    {"Ven", "Fri"},
    // TXT_DAY_SAT
    {"Sab", "Sat"},

    // ----- MESI ABBREVIATI -----
    // TXT_MON_JAN
    {"Gen", "Jan"},
    // TXT_MON_FEB
    {"Feb", "Feb"},
    // TXT_MON_MAR
    {"Mar", "Mar"},
    // TXT_MON_APR
    {"Apr", "Apr"},
    // TXT_MON_MAY
    {"Mag", "May"},
    // TXT_MON_JUN
    {"Giu", "Jun"},
    // TXT_MON_JUL
    {"Lug", "Jul"},
    // TXT_MON_AUG
    {"Ago", "Aug"},
    // TXT_MON_SEP
    {"Set", "Sep"},
    // TXT_MON_OCT
    {"Ott", "Oct"},
    // TXT_MON_NOV
    {"Nov", "Nov"},
    // TXT_MON_DEC
    {"Dic", "Dec"},

    // ----- DIREZIONI VENTO -----
    // TXT_WIND_N
    {"N", "N"},
    // TXT_WIND_NE
    {"NE", "NE"},
    // TXT_WIND_E
    {"E", "E"},
    // TXT_WIND_SE
    {"SE", "SE"},
    // TXT_WIND_S
    {"S", "S"},
    // TXT_WIND_SW
    {"SO", "SW"},
    // TXT_WIND_W
    {"O", "W"},
    // TXT_WIND_NW
    {"NO", "NW"},

    // ----- PUNTI CARDINALI BUSSOLA -----
    // TXT_COMPASS_N
    {"N", "N"},
    // TXT_COMPASS_S
    {"S", "S"},
    // TXT_COMPASS_E
    {"E", "E"},
    // TXT_COMPASS_W
    {"O", "W"},

    // ----- FLIP CLOCK -----
    // TXT_FC_INDOOR
    {"INT:", "IN:"},
    // TXT_FC_OUTDOOR
    {"EST:", "OUT:"},
    // TXT_FC_TOMORROW
    {"DOM:", "TOM:"},
};

// ===============================================
// MACRO TR() - Ottieni traduzione
// ===============================================
#ifdef ENABLE_MULTILANGUAGE
    #define TR(id) (TRANSLATIONS[id][currentLanguage])
#else
    #define TR(id) (TRANSLATIONS[id][0])
#endif

// ===============================================
// FUNZIONI HELPER
// ===============================================

// Ottieni giorno abbreviato (0=Dom, 1=Lun, ...)
inline const char* getTRDay(uint8_t dayIndex) {
    if (dayIndex > 6) dayIndex = 0;
    return TR((TextID)(TXT_DAY_SUN + dayIndex));
}

// Ottieni mese abbreviato (0=Gen, 1=Feb, ...)
inline const char* getTRMonth(uint8_t monthIndex) {
    if (monthIndex > 11) monthIndex = 0;
    return TR((TextID)(TXT_MON_JAN + monthIndex));
}

// Ottieni direzione vento (gradi -> stringa)
inline const char* getTRWindDir(int deg) {
    if (deg >= 337 || deg < 23) return TR(TXT_WIND_N);
    if (deg >= 23 && deg < 67) return TR(TXT_WIND_NE);
    if (deg >= 67 && deg < 113) return TR(TXT_WIND_E);
    if (deg >= 113 && deg < 157) return TR(TXT_WIND_SE);
    if (deg >= 157 && deg < 203) return TR(TXT_WIND_S);
    if (deg >= 203 && deg < 247) return TR(TXT_WIND_SW);
    if (deg >= 247 && deg < 293) return TR(TXT_WIND_W);
    if (deg >= 293 && deg < 337) return TR(TXT_WIND_NW);
    return TR(TXT_WIND_N);
}

#endif // TRANSLATIONS_H
