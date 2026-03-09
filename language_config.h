// =========================================================================
// language_config.h - Configurazione Sistema Multilingua OraQuadra
// =========================================================================
//
// Questo file gestisce la selezione della lingua per tutto il sistema.
// Supporta attualmente: Italiano (IT) e Inglese (EN)
//
// NOTA: Sistema NON attivo di default. Per attivare il multilingua:
// 1. Decommentare #define ENABLE_MULTILANGUAGE
// 2. Aggiungere la selezione lingua nel menu setup
// 3. Modificare il codice di rendering per usare la lingua selezionata
//
// =========================================================================

#ifndef LANGUAGE_CONFIG_H
#define LANGUAGE_CONFIG_H

#include <stdint.h>
#include <Arduino.h>

// ===============================================
// ABILITAZIONE SISTEMA MULTILINGUA
// ===============================================
// Decommentare per abilitare il supporto multilingua
// Se commentato, il sistema usa solo italiano (comportamento originale)

#define ENABLE_MULTILANGUAGE

// ===============================================
// DEFINIZIONE LINGUE SUPPORTATE
// ===============================================

enum Language : uint8_t {
    LANG_IT = 0,    // Italiano (default)
    LANG_EN = 1,    // English
    // LANG_DE = 2, // Deutsch (futuro)
    // LANG_FR = 3, // Français (futuro)
    // LANG_ES = 4, // Español (futuro)
    LANG_COUNT      // Numero totale lingue
};

// ===============================================
// VARIABILE GLOBALE LINGUA CORRENTE
// ===============================================
// Definita in language_config.cpp o nel file .ino principale

#ifdef ENABLE_MULTILANGUAGE
    extern Language currentLanguage;
#else
    // Se multilingua disabilitato, forza sempre italiano
    #define currentLanguage LANG_IT
#endif

// ===============================================
// INDIRIZZO EEPROM PER SALVARE LA LINGUA
// ===============================================
// NOTA: Verificare che non collida con altri indirizzi EEPROM usati!
// Controllare gli indirizzi in uso nel progetto principale

#define EEPROM_LANGUAGE_ADDR 180  // Indirizzo EEPROM per lingua (1 byte)

// ===============================================
// NOMI LINGUE (per menu)
// ===============================================

const char* const LANGUAGE_NAMES[] PROGMEM = {
    "Italiano",
    "English"
    // "Deutsch",
    // "Français",
    // "Español"
};

// Codici brevi per visualizzazione compatta
const char* const LANGUAGE_CODES[] PROGMEM = {
    "IT",
    "EN"
    // "DE",
    // "FR",
    // "ES"
};

// ===============================================
// FUNZIONI DI GESTIONE LINGUA
// ===============================================

#ifdef ENABLE_MULTILANGUAGE

// Variabile globale lingua corrente - DICHIARAZIONE extern
// La definizione effettiva è in 26_WEBSERVER_SETTINGS.ino
extern Language currentLanguage;

// Dichiarazioni funzioni (implementate in 26_WEBSERVER_SETTINGS.ino)
void initLanguage();
void setLanguage(Language lang);
Language getLanguage();
void nextLanguage();
const char* getLanguageName();
const char* getLanguageCode();

#endif // ENABLE_MULTILANGUAGE

// ===============================================
// MACRO PER SELEZIONE STRINGHE
// ===============================================
// Uso: STR(DAYS, dayIndex) restituisce il giorno nella lingua corrente

#ifdef ENABLE_MULTILANGUAGE
    #define STR(array, index) (array[currentLanguage][index])
    #define STR_SINGLE(array) (array[currentLanguage])
#else
    // Se multilingua disabilitato, usa sempre indice 0 (italiano)
    #define STR(array, index) (array[0][index])
    #define STR_SINGLE(array) (array[0])
#endif

// ===============================================
// INCLUSIONE CONDIZIONALE DEI LAYOUT
// ===============================================

// Il layout italiano è sempre incluso (è il default)
// #include "word_mappings.h"  // Incluso nel file principale

// Il layout inglese è incluso solo se multilingua è abilitato
#ifdef ENABLE_MULTILANGUAGE
    #include "word_mappings_en.h"
#endif

#endif // LANGUAGE_CONFIG_H
