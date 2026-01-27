// =========================================================================
// word_mappings_en.h - English Word Clock Layout for 16x16 LED Matrix
// ENGLISH VERSION - Usa le STESSE posizioni/indici di word_mappings.h
// =========================================================================
//
// CONCETTO: Il sistema italiano accende posizioni specifiche (es. 0,1,2,3 per "SONO")
// Per l'inglese, mettiamo lettere INGLESI in quelle STESSE posizioni.
// Quando il sistema accende posizioni 0,1,2,3 vedremo "IT I" invece di "SONO"
//
// PROBLEMA: Le parole italiane e inglesi hanno lunghezze diverse!
// Soluzione: Usiamo le stesse posizioni ma adattiamo le parole inglesi
//
// =========================================================================

#ifndef WORD_MAPPINGS_EN_H
#define WORD_MAPPINGS_EN_H

#include <stdint.h>
#include <Arduino.h>

// ===============================================
// ANALISI POSIZIONI ITALIANE -> MAPPATURA INGLESE
// ===============================================
//
// SONO LE: {0,1,2,3, 5,6, 8,9,10} = 9 pos -> "IT IS" (4 lettere) - useremo solo alcune
// E: {116} = 1 pos -> "AND" non entra, usiamo altro sistema
// MINUTI: {250,251,252,253,254,255} = 6 pos -> "MINUTE" (6 lettere) OK!
//
// ORE ITALIANE -> ORE INGLESI (formato 24h -> mostriamo numero)
// UNA {57,73,89} = 3 pos verticali -> "ONE" OK!
// DUE {96,112,128} = 3 pos verticali -> "TWO" OK!
// TRE {21,22,23} = 3 pos -> "THR" (THREE troncato)
// QUATTRO {56,57,58,59,60,61,62} = 7 pos -> "FOURXXX" (FOUR + filler)
// CINQUE {33,49,65,81,97,113} = 6 pos verticali -> "FIVEXE" hmm
// SEI {45,46,47} = 3 pos -> "SIX" OK!
// SETTE {91,92,93,94,95} = 5 pos -> "SEVEN" OK!
// OTTO {28,29,30,31} = 4 pos -> "FOUR" OK! (ma già usato per QUATTRO...)
// NOVE {106,107,108,109} = 4 pos -> "NINE" OK!
// DIECI {75,76,77,78,79} = 5 pos -> "TENXX" (TEN + filler)
// UNDICI {50,51,52,53,54,55} = 6 pos -> "ELEVEN" OK!
// DODICI {98,99,100,101,102,103} = 6 pos -> "TWELVE" OK!
// ecc.
//
// MINUTI: Stessa logica per i numeri dei minuti
//
// ===============================================

// Per semplicità, creo la griglia inglese con le parole posizionate
// in modo che le STESSE funzioni italiane funzionino anche per l'inglese.
//
// Nota: Alcune parole dovranno essere troncate o adattate.

// Layout italiano originale (per riferimento):
// Pos 0-15:   S O N O U L E Y O R E X Z E R O
// Pos 16-31:  V E N T I T R E D I C I O T T O
// Pos 32-47:  E C Q U A T T O R D I C I S E I
// Pos 48-63:  N I U N D I C I Q U A T T R O X
// Pos 64-79:  O T N I J V E N T U N O D I E C I
// Pos 80-95:  I Q N S E D I C I A S S E T T E
// Pos 96-111: D U D O D I C I A N N O V E L F
// Pos 112-127: U E I H E L P Q U A R A N T A X
// Pos 128-143: E R C K U V E N T I T R E N T A
// Pos 144-159: G R I N C I N Q U A N T A U N O
// Pos 160-175: S E D I C I D O D I C I O T T O
// Pos 176-191: D I E C I Q U A T T O R D I C I
// Pos 192-207: Q U A T T R O Q U I N D I C I O
// Pos 208-223: A R T R E D I C I A S S E T T E
// Pos 224-239: U N D I C I A N N O V E O S E I
// Pos 240-255: C I N Q U E D U E U M I N U T I

// ===============================================
// GRIGLIA INGLESE 16x16
// ===============================================
// Layout ottimizzato per word clock inglese
// Formato: IT IS [minuti] PAST/TO [ora] O'CLOCK
//
// Posizioni (riga*16 + colonna):
// Row 0:  0-15    Row 4:  64-79   Row 8:  128-143  Row 12: 192-207
// Row 1:  16-31   Row 5:  80-95   Row 9:  144-159  Row 13: 208-223
// Row 2:  32-47   Row 6:  96-111  Row 10: 160-175  Row 14: 224-239
// Row 3:  48-63   Row 7:  112-127 Row 11: 176-191  Row 15: 240-255

const char TFT_L_EN[] PROGMEM =
//   0         1
//   0123456789012345
    "AITBISCDEFGHIJKL"  // Row 0  (0-15):   IT(1-2) IS(4-5)
    "MONEMTWONTHREEOP"  // Row 1  (16-31):  ONE(17-19) TWO(21-23) THREE(25-29)
    "FOURRFIVESQSTUAB"  // Row 2  (32-47):  FOUR(32-35) FIVE(37-40)
    "SIXCDSEVENEFGHIJ"  // Row 3  (48-63):  SIX(48-50) SEVEN(53-57)
    "KEIGHTLMNINENOPQ"  // Row 4  (64-79):  EIGHT(65-69) NINE(72-75)
    "RTENABELEVENSCDE"  // Row 5  (80-95):  TEN(81-83) ELEVEN(85-90)
    "FGTWELVEHIJKLMNO"  // Row 6  (96-111): TWELVE(98-103)
    "POCLOCKQRSTUVWXY"  // Row 7  (112-127):OCLOCK(113-118)
    "ZABQUARTERABCDEF"  // Row 8  (128-143):QUARTER(131-137)
    "GHALFIJTWENTYKLM"  // Row 9  (144-159):HALF(148-151) TWENTY(153-158)
    "NFIVEOPQTENABRST"  // Row 10 (160-175):FIVE(161-164) TEN(168-170)
    "UVPASTWATOXYZBCD"  // Row 11 (176-191):PAST(178-181) TO(184-185)
    "EFGHIJKLMNOPQRST"  // Row 12 (192-207):filler
    "UVWXYZABCDEFGHIJ"  // Row 13 (208-223):filler
    "KLMNOPQRSTUVWXYZ"  // Row 14 (224-239):filler
    "ABCDEFGHIJKLMNOP"; // Row 15 (240-255):filler

// ===============================================
// PAROLE INGLESI CON INDICI CORRISPONDENTI AL LAYOUT
// ===============================================
// LAYOUT: IT IS [ORA] [OCLOCK] poi [MINUTI] [PAST/TO] nella parte bassa
// Row 0:  "AITBISCDEFGHIJKL"  IT(1-2) IS(4-5)
// Row 1:  "MONEMTWONTHREEOP"  ONE(17-19) TWO(21-23) THREE(25-29)
// Row 2:  "FOURRFIVESQSTUAB"  FOUR(32-35) FIVE(37-40)
// Row 3:  "SIXCDSEVENEFGHIJ"  SIX(48-50) SEVEN(53-57)
// Row 4:  "KEIGHTLMNINENOPQ"  EIGHT(65-69) NINE(72-75)
// Row 5:  "RTENABELEVENSCDE"  TEN(81-83) ELEVEN(86-91)
// Row 6:  "FGTWELVEHIJKLMNO"  TWELVE(98-103)
// Row 7:  "POCLOCKQRSTUVWXY"  OCLOCK(113-118)
// Row 8:  "ZABQUARTERABCDEF"  QUARTER(131-137)
// Row 9:  "GHALFIJTWENTYKLM"  HALF(145-148) TWENTY(151-156)
// Row 10: "NFIVEOPQTENABRST"  FIVE(161-164) TEN(168-170)
// Row 11: "UVPASTWATOXYZBCD"  PAST(178-181) TO(184-185)

// "IT IS" - sempre mostrato
const uint8_t PROGMEM WORD_EN_IT_IS[] = {1, 2, 4, 5, 4};  // I(1)T(2) I(4)S(5) - spazio tra IT e IS!

// Ore (1-12) - PARTE ALTA del display
const uint8_t PROGMEM WORD_EN_ONE[] = {17, 18, 19, 4};             // O(17)N(18)E(19)
const uint8_t PROGMEM WORD_EN_TWO[] = {21, 22, 23, 4};             // T(21)W(22)O(23)
const uint8_t PROGMEM WORD_EN_THREE[] = {25, 26, 27, 28, 29, 4};   // T(25)H(26)R(27)E(28)E(29)
const uint8_t PROGMEM WORD_EN_FOUR[] = {32, 33, 34, 35, 4};        // F(32)O(33)U(34)R(35)
const uint8_t PROGMEM WORD_EN_FIVE[] = {37, 38, 39, 40, 4};        // F(37)I(38)V(39)E(40)
const uint8_t PROGMEM WORD_EN_SIX[] = {48, 49, 50, 4};             // S(48)I(49)X(50)
const uint8_t PROGMEM WORD_EN_SEVEN[] = {53, 54, 55, 56, 57, 4};   // S(53)E(54)V(55)E(56)N(57)
const uint8_t PROGMEM WORD_EN_EIGHT[] = {65, 66, 67, 68, 69, 4};   // E(65)I(66)G(67)H(68)T(69)
const uint8_t PROGMEM WORD_EN_NINE[] = {72, 73, 74, 75, 4};        // N(72)I(73)N(74)E(75)
const uint8_t PROGMEM WORD_EN_TEN[] = {81, 82, 83, 4};             // T(81)E(82)N(83)
const uint8_t PROGMEM WORD_EN_ELEVEN[] = {86, 87, 88, 89, 90, 91, 4}; // E(86)L(87)E(88)V(89)E(90)N(91)
const uint8_t PROGMEM WORD_EN_TWELVE[] = {98, 99, 100, 101, 102, 103, 4}; // T(98)W(99)E(100)L(101)V(102)E(103)

// OCLOCK - dopo l'ora
const uint8_t PROGMEM WORD_EN_OCLOCK[] = {113, 114, 115, 116, 117, 118, 4}; // O(113)C(114)L(115)O(116)C(117)K(118)

// Minuti prefisso - PARTE BASSA del display
// Row 8: "ZABQUARTERABCDEF" Q(131)U(132)A(133)R(134)T(135)E(136)R(137)
const uint8_t PROGMEM WORD_EN_QUARTER[] = {131, 132, 133, 134, 135, 136, 137, 4};
// Row 9: "GHALFIJTWENTYKLM" H(145)A(146)L(147)F(148) T(151)W(152)E(153)N(154)T(155)Y(156)
const uint8_t PROGMEM WORD_EN_HALF[] = {145, 146, 147, 148, 4};
const uint8_t PROGMEM WORD_EN_TWENTY[] = {151, 152, 153, 154, 155, 156, 4};
// Row 10: "NFIVEOPQTENABRST" F(161)I(162)V(163)E(164) T(168)E(169)N(170)
const uint8_t PROGMEM WORD_EN_MFIVE[] = {161, 162, 163, 164, 4};
const uint8_t PROGMEM WORD_EN_MTEN[] = {168, 169, 170, 4};

// Connettivi - PARTE BASSA
// Row 11: "UVPASTWATOXYZBCD" P(178)A(179)S(180)T(181) T(184)O(185)
const uint8_t PROGMEM WORD_EN_PAST[] = {178, 179, 180, 181, 4};
const uint8_t PROGMEM WORD_EN_TO[] = {184, 185, 4};

// Array ore (indice 0=12, 1-11=ore)
const uint8_t* const HOUR_WORDS_EN[] PROGMEM = {
    WORD_EN_TWELVE,   // 0 -> 12
    WORD_EN_ONE,      // 1
    WORD_EN_TWO,      // 2
    WORD_EN_THREE,    // 3
    WORD_EN_FOUR,     // 4
    WORD_EN_FIVE,     // 5
    WORD_EN_SIX,      // 6
    WORD_EN_SEVEN,    // 7
    WORD_EN_EIGHT,    // 8
    WORD_EN_NINE,     // 9
    WORD_EN_TEN,      // 10
    WORD_EN_ELEVEN,   // 11
    WORD_EN_TWELVE    // 12
};

// ===============================================
// FUNZIONI HELPER
// ===============================================

// Converti ora 24h -> 12h (restituisce 1-12)
inline uint8_t hour24to12(uint8_t hour24) {
    if (hour24 == 0) return 12;
    if (hour24 > 12) return hour24 - 12;
    return hour24;
}

// Tipo di visualizzazione minuti
enum EnglishMinuteType {
    EN_MIN_OCLOCK = 0,      // :00
    EN_MIN_FIVE = 1,        // :05, :55
    EN_MIN_TEN = 2,         // :10, :50
    EN_MIN_QUARTER = 3,     // :15, :45
    EN_MIN_TWENTY = 4,      // :20, :40
    EN_MIN_TWENTYFIVE = 5,  // :25, :35
    EN_MIN_HALF = 6,        // :30
    EN_MIN_EXACT = 7        // altri minuti
};

// Struttura display inglese
struct EnglishTimeDisplay {
    bool showItIs;
    EnglishMinuteType minType;
    bool useTo;              // true = TO, false = PAST
    uint8_t displayHour;     // ora da mostrare (1-12)
    uint8_t exactMinutes;
};

// Calcola cosa visualizzare
inline EnglishTimeDisplay calculateEnglishDisplay(uint8_t hour24, uint8_t minutes) {
    EnglishTimeDisplay result;
    result.showItIs = true;
    result.exactMinutes = minutes;

    // Arrotonda ai 5 minuti più vicini
    uint8_t rounded = ((minutes + 2) / 5) * 5;
    if (rounded == 60) rounded = 0;

    // PAST o TO?
    if (rounded <= 30) {
        result.useTo = false;  // PAST
        result.displayHour = hour24to12(hour24);
    } else {
        result.useTo = true;   // TO -> ora successiva
        uint8_t nextHour = (hour24 + 1) % 24;
        result.displayHour = hour24to12(nextHour);
        rounded = 60 - rounded;
    }

    // Tipo minuto
    switch (rounded) {
        case 0:  result.minType = EN_MIN_OCLOCK; break;
        case 5:  result.minType = EN_MIN_FIVE; break;
        case 10: result.minType = EN_MIN_TEN; break;
        case 15: result.minType = EN_MIN_QUARTER; break;
        case 20: result.minType = EN_MIN_TWENTY; break;
        case 25: result.minType = EN_MIN_TWENTYFIVE; break;
        case 30: result.minType = EN_MIN_HALF; break;
        default: result.minType = EN_MIN_EXACT; break;
    }

    return result;
}

#endif // WORD_MAPPINGS_EN_H
