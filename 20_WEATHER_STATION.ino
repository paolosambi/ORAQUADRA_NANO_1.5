// ================== WEATHER STATION - STAZIONE METEO COMPLETA ==================
/*
 * Visualizzazione completa di tutti i dati meteo:
 * - Data e ora
 * - Temperatura interna/esterna con icone
 * - Umidità interna/esterna
 * - Pressione atmosferica
 * - Vento (velocità e direzione con bussola grafica)
 * - Condizioni meteo con icone grandi
 * - Previsioni domani
 * - Alba/Tramonto
 */

#ifdef EFFECT_WEATHER_STATION

// Include sistema traduzioni
#include "translations.h"

// ================== CONFIGURAZIONE ==================
#define WS_BG_COLOR         0x0000  // Nero sfondo
#define WS_TEXT_COLOR       0xFFFF  // Bianco testo
#define WS_ACCENT_COLOR     0x07FF  // Ciano accenti
#define WS_TEMP_HOT         0xF800  // Rosso per caldo
#define WS_TEMP_COLD        0x001F  // Blu per freddo
#define WS_YELLOW           0xFFE0  // Giallo
#define WS_ORANGE           0xFD20  // Arancione
#define WS_GREEN            0x07E0  // Verde

// Extern variabili meteo da 19_WEATHER.ino
extern float weatherTempOutdoor;
extern float weatherHumOutdoor;
extern String weatherDescription;
extern int weatherCode;
extern float weatherWindSpeed;
extern int weatherWindDeg;
extern float weatherPressure;
extern float weatherFeelsLike;
extern int weatherClouds;
extern float weatherVisibility;
extern long weatherSunrise;
extern long weatherSunset;
extern float weatherTempTomorrow;
extern float weatherTempTomorrowMin;
extern float weatherTempTomorrowMax;
extern String weatherDescTomorrow;
extern int weatherCodeTomorrow;
extern bool weatherDataValid;

// Extern variabili fase lunare da 19_WEATHER.ino
extern float moonPhase;
extern int moonPhaseIndex;
extern String moonPhaseName;
extern float moonIllumination;
extern int moonAge;

// Extern variabili sensore interno (BME280 o Radar remoto)
extern float temperatureIndoor;
extern float humidityIndoor;
extern bool bme280Available;
extern uint8_t indoorSensorSource;  // 0=nessuno, 1=BME280, 2=radar

// Flag inizializzazione (dichiarato nel file principale)
extern bool weatherStationInitialized;

// Ultimo aggiornamento display
unsigned long lastWSUpdate = 0;
#define WS_UPDATE_INTERVAL 1000  // Aggiorna ogni secondo

// ================== PROTOTIPI ==================
void initWeatherStation();
void updateWeatherStation();
void drawWSBackground();
void drawWSDateTime();
void drawWSTemperatures();
void drawWSWeatherIcon(int16_t x, int16_t y, int code, int size);
void drawWSMoonIcon(int16_t x, int16_t y, int size, float phase);
void drawWSWindCompass(int16_t x, int16_t y, int deg, float speed);
void drawWSDetails();
void drawWSForecast();
void drawWSSunMoon();
const char* getWindDirection(int deg);

// ================== DIREZIONE VENTO ==================
// Usa getTRWindDir() da translations.h per le traduzioni
const char* getWindDirection(int deg) {
  return getTRWindDir(deg);
}

// ================== INIZIALIZZAZIONE ==================
void initWeatherStation() {
  Serial.println("[WEATHER STATION] Inizializzazione...");

  // Sfondo nero - SEMPRE ridisegna tutto quando si entra nella modalità
  gfx->fillScreen(WS_BG_COLOR);

  // Disegna sfondo con bordi e labels
  drawWSBackground();

  // Disegna tutti i dati
  drawWSDateTime();
  drawWSTemperatures();
  drawWSDetails();
  drawWSForecast();

  weatherStationInitialized = true;
  lastWSUpdate = millis();

  Serial.println("[WEATHER STATION] Inizializzato!");
}

// ================== UPDATE ==================
void updateWeatherStation() {
  unsigned long now = millis();

  // Aggiorna solo ogni secondo
  if (now - lastWSUpdate < WS_UPDATE_INTERVAL) return;
  lastWSUpdate = now;

  // Aggiorna le sezioni che cambiano
  drawWSDateTime();
  drawWSTemperatures();
  drawWSDetails();
  drawWSForecast();
}

// ================== SFONDO E CORNICI ==================
void drawWSBackground() {
  // Titolo con città
  extern String apiOpenWeatherCity;
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(WS_ACCENT_COLOR);

  // Calcola larghezza totale: "STAZIONE METEO - Città" / "WEATHER STATION - City"
  String titleText = TR(TXT_WEATHER_STATION);
  String fullTitle = titleText + " - " + apiOpenWeatherCity;
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(fullTitle.c_str(), 0, 0, &x1, &y1, &w, &h);
  int16_t startX = (480 - w) / 2;

  // Stampa titolo in ciano
  gfx->setCursor(startX, 22);
  gfx->print(titleText.c_str());

  // Stampa " - " in grigio
  gfx->setTextColor(0x8410);
  gfx->print(" - ");

  // Stampa città in giallo
  gfx->setTextColor(WS_YELLOW);
  gfx->print(apiOpenWeatherCity.c_str());

  // Linea separatore sotto titolo
  gfx->drawFastHLine(20, 30, 440, WS_ACCENT_COLOR);

  // Box per le sezioni
  // Box ORA/DATA (in alto a sinistra)
  gfx->drawRoundRect(10, 40, 230, 70, 5, 0x4208);

  // Box METEO ATTUALE (in alto a destra)
  gfx->drawRoundRect(250, 40, 220, 70, 5, 0x4208);

  // Box TEMPERATURE (centro sinistra)
  gfx->drawRoundRect(10, 120, 230, 100, 5, 0x4208);

  // Box VENTO (centro destra)
  gfx->drawRoundRect(250, 120, 220, 100, 5, 0x4208);

  // Box DETTAGLI (in basso sinistra)
  gfx->drawRoundRect(10, 230, 230, 100, 5, 0x4208);

  // Box DOMANI (in basso destra)
  gfx->drawRoundRect(250, 230, 220, 100, 5, 0x4208);

  // Box SOLE (sinistra in fondo)
  gfx->drawRoundRect(10, 340, 290, 130, 5, 0x4208);

  // Box LUNA (destra in fondo)
  gfx->drawRoundRect(310, 340, 160, 130, 5, 0x4208);

  // Labels delle sezioni (tradotte)
  gfx->setFont(u8g2_font_helvB08_tr);
  gfx->setTextColor(WS_YELLOW);

  gfx->setCursor(20, 52);
  gfx->print(TR(TXT_DATE_TIME));

  gfx->setCursor(260, 52);
  gfx->print(TR(TXT_CONDITIONS));

  gfx->setCursor(20, 132);
  gfx->print(TR(TXT_TEMPERATURES));

  gfx->setCursor(260, 132);
  gfx->print(TR(TXT_WIND));

  gfx->setCursor(20, 242);
  gfx->print(TR(TXT_DETAILS));

  gfx->setCursor(260, 242);
  gfx->print(TR(TXT_TOMORROW));

  // Etichetta SOLE rimossa - alba/tramonto/ore luce mostrate direttamente nel box

  gfx->setCursor(320, 352);
  gfx->print("LUNA");
}

// ================== DATA E ORA ==================
void drawWSDateTime() {
  // Cancella SOLO area interna (non i bordi) - box è da Y=40 a Y=110
  gfx->fillRect(12, 56, 226, 52, WS_BG_COLOR);

  // Ora grande centrata
  gfx->setFont(u8g2_font_logisoso24_tn);  // Font più piccolo per entrare
  gfx->setTextColor(WS_TEXT_COLOR);

  char timeStr[10];
  sprintf(timeStr, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);

  // Centra l'ora nel box
  int16_t x1, y1;
  uint16_t w, h;
  gfx->getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  gfx->setCursor(15 + (226 - w) / 2, 80);
  gfx->print(timeStr);

  // Data sotto l'ora (tradotta)
  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(WS_ACCENT_COLOR);

  uint8_t dow = myTZ.weekday() - 1;  // 0-6
  uint8_t day = myTZ.day();
  uint8_t month = myTZ.month() - 1;  // 0-11

  char dateStr[30];
  sprintf(dateStr, "%s %d %s %d", getTRDay(dow), day, getTRMonth(month), myTZ.year());

  // Centra la data
  gfx->getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  gfx->setCursor(15 + (226 - w) / 2, 102);
  gfx->print(dateStr);
}

// ================== ICONA METEO DETTAGLIATA ==================
void drawWSWeatherIcon(int16_t x, int16_t y, int code, int size) {
  // Cancella area icona
  gfx->fillRect(x - size/2 - 2, y - size/2 - 2, size + 4, size + 4, WS_BG_COLOR);

  int r = size / 3;  // Raggio base

  if (code == 800) {
    // ☀️ SOLE - dettagliato con gradiente
    // Cerchio centrale con effetto 3D
    gfx->fillCircle(x, y, r, 0xFD20);      // Arancione esterno
    gfx->fillCircle(x, y, r - 2, WS_YELLOW); // Giallo medio
    gfx->fillCircle(x - 2, y - 2, r/3, 0xFFE0); // Riflesso chiaro

    // Raggi lunghi e corti alternati
    for (int i = 0; i < 12; i++) {
      float angle = i * 30 * DEG_TO_RAD;
      int len = (i % 2 == 0) ? r + 8 : r + 5;
      int rx1 = x + cos(angle) * (r + 3);
      int ry1 = y + sin(angle) * (r + 3);
      int rx2 = x + cos(angle) * len;
      int ry2 = y + sin(angle) * len;
      gfx->drawLine(rx1, ry1, rx2, ry2, WS_YELLOW);
      if (i % 2 == 0) {
        // Raggi principali più spessi
        gfx->drawLine(rx1 + 1, ry1, rx2 + 1, ry2, WS_YELLOW);
      }
    }

  } else if (code == 801) {
    // 🌤️ POCO NUVOLOSO - sole con nuvoletta
    // Sole piccolo in alto a destra
    int sx = x + r/2;
    int sy = y - r/2;
    gfx->fillCircle(sx, sy, r/2, WS_YELLOW);
    for (int i = 0; i < 8; i++) {
      float angle = i * 45 * DEG_TO_RAD;
      gfx->drawLine(sx + cos(angle) * (r/2 + 2), sy + sin(angle) * (r/2 + 2),
                    sx + cos(angle) * (r/2 + 5), sy + sin(angle) * (r/2 + 5), WS_YELLOW);
    }
    // Nuvoletta in primo piano
    gfx->fillCircle(x - r/3, y + r/4, r/2, 0xE71C);
    gfx->fillCircle(x + r/4, y + r/4, r/2, 0xE71C);
    gfx->fillCircle(x, y, r/2, 0xFFFF);
    gfx->fillRoundRect(x - r/2, y, r, r/2, 3, 0xE71C);

  } else if (code >= 802 && code <= 804) {
    // ☁️ NUVOLOSO - nuvole dettagliate
    uint16_t c1 = (code == 802) ? 0xC618 : 0x8410;
    uint16_t c2 = (code == 802) ? 0xE71C : 0xA514;

    // Nuvola posteriore (più scura)
    gfx->fillCircle(x - r/2, y + 2, r/2, c1);
    gfx->fillCircle(x + r/3, y + 2, r/2, c1);

    // Nuvola principale (più chiara)
    gfx->fillCircle(x - r/3, y - r/4, r/2 + 2, c2);
    gfx->fillCircle(x + r/4, y - r/4, r/2 + 2, c2);
    gfx->fillCircle(x, y - r/2, r/2, c2);
    gfx->fillRoundRect(x - r/2, y - r/3, r, r/2, 4, c2);

  } else if (code >= 500 && code < 600) {
    // 🌧️ PIOGGIA - nuvola con gocce animate
    // Nuvola
    gfx->fillCircle(x - r/4, y - r/3, r/2, 0x6B4D);
    gfx->fillCircle(x + r/4, y - r/3, r/2, 0x6B4D);
    gfx->fillCircle(x, y - r/2, r/2 - 2, 0x8410);
    gfx->fillRoundRect(x - r/2, y - r/2, r, r/3, 3, 0x6B4D);

    // Gocce di pioggia stilizzate
    for (int i = 0; i < 4; i++) {
      int gx = x - r/2 + i * r/3 + 3;
      int gy = y + r/4 + (i % 2) * 5;
      // Goccia con forma a lacrima
      gfx->fillCircle(gx, gy + 3, 2, 0x04BF);
      gfx->fillTriangle(gx, gy - 2, gx - 2, gy + 3, gx + 2, gy + 3, 0x04BF);
    }

  } else if (code >= 600 && code < 700) {
    // ❄️ NEVE - nuvola con fiocchi
    // Nuvola grigia chiara
    gfx->fillCircle(x - r/4, y - r/3, r/2, 0xBDF7);
    gfx->fillCircle(x + r/4, y - r/3, r/2, 0xBDF7);
    gfx->fillRoundRect(x - r/2, y - r/2, r, r/3, 3, 0xBDF7);

    // Fiocchi di neve (asterischi)
    for (int i = 0; i < 3; i++) {
      int fx = x - r/3 + i * r/3;
      int fy = y + r/4 + (i % 2) * 4;
      // Disegna un fiocco a 6 punte
      for (int j = 0; j < 3; j++) {
        float angle = j * 60 * DEG_TO_RAD;
        gfx->drawLine(fx - cos(angle) * 3, fy - sin(angle) * 3,
                      fx + cos(angle) * 3, fy + sin(angle) * 3, WS_TEXT_COLOR);
      }
    }

  } else if (code >= 200 && code < 300) {
    // ⛈️ TEMPORALE - nuvola scura con fulmine
    // Nuvola scura
    gfx->fillCircle(x - r/3, y - r/3, r/2, 0x4208);
    gfx->fillCircle(x + r/3, y - r/3, r/2, 0x4208);
    gfx->fillCircle(x, y - r/2, r/2, 0x528A);
    gfx->fillRoundRect(x - r/2, y - r/2, r, r/3, 3, 0x4208);

    // Fulmine dettagliato (zig-zag)
    int16_t lx = x;
    int16_t ly = y;
    gfx->fillTriangle(lx - 2, ly - 3, lx + 6, ly - 3, lx + 2, ly + 4, WS_YELLOW);
    gfx->fillTriangle(lx - 4, ly + 2, lx + 4, ly + 2, lx, ly + 10, WS_YELLOW);
    // Contorno per effetto 3D
    gfx->drawTriangle(lx - 2, ly - 3, lx + 6, ly - 3, lx + 2, ly + 4, 0xFD20);

  } else if (code >= 700 && code < 800) {
    // 🌫️ NEBBIA - linee sfumate
    for (int i = 0; i < 5; i++) {
      int ly = y - r/2 + i * r/4;
      int lw = r + 10 - abs(i - 2) * 4;  // Più largo al centro
      uint16_t color = 0x8410 + i * 0x0821;  // Sfumatura
      gfx->drawFastHLine(x - lw/2, ly, lw, color);
      gfx->drawFastHLine(x - lw/2 + 1, ly + 1, lw - 2, color);
    }

  } else {
    // DEFAULT - icona sconosciuta
    gfx->drawCircle(x, y, r, 0x8410);
    gfx->setFont(u8g2_font_helvB14_tr);
    gfx->setTextColor(0x8410);
    gfx->setCursor(x - 5, y + 5);
    gfx->print("?");
  }
}

// ================== ICONA LUNA DETTAGLIATA ==================
void drawWSMoonIcon(int16_t x, int16_t y, int size, float phase) {
  // Colori luna
  uint16_t moonLight = 0xFFDF;   // Bianco caldo (parte illuminata)
  uint16_t moonDark = 0x2104;    // Grigio scuro (parte in ombra)
  uint16_t moonCrater = 0xC618;  // Grigio per crateri

  int radius = size / 2;

  // Disegna la luna base (cerchio grigio scuro)
  gfx->fillCircle(x, y, radius, moonDark);

  // Calcola la parte illuminata in base alla fase
  // phase: 0.0 = luna nuova, 0.5 = luna piena, 1.0 = luna nuova

  if (phase < 0.5) {
    // Luna crescente (0 -> 0.5)
    // L'illuminazione cresce da destra a sinistra

    // Disegna la parte illuminata
    float illumRatio = phase * 2.0;  // 0 a 1

    if (illumRatio > 0.02) {
      // Disegna l'illuminazione crescente da destra
      for (int dy = -radius; dy <= radius; dy++) {
        float rowWidth = sqrt(radius * radius - dy * dy);
        int startX;

        if (illumRatio < 0.5) {
          // Prima metà: falce sottile a destra
          float darkWidth = rowWidth * (1.0 - illumRatio * 2.0);
          startX = x + (int)darkWidth;
        } else {
          // Seconda metà: oltre la metà
          float lightWidth = rowWidth * ((illumRatio - 0.5) * 2.0);
          startX = x - (int)lightWidth;
        }

        int endX = x + (int)rowWidth;
        if (startX < endX) {
          gfx->drawFastHLine(startX, y + dy, endX - startX, moonLight);
        }
      }
    }
  } else {
    // Luna calante (0.5 -> 1.0)
    // L'illuminazione decresce da sinistra a destra

    float illumRatio = (1.0 - phase) * 2.0;  // 1 a 0

    // Prima disegna tutta la luna illuminata
    gfx->fillCircle(x, y, radius, moonLight);

    if (illumRatio < 0.98) {
      // Disegna l'ombra crescente da destra
      for (int dy = -radius; dy <= radius; dy++) {
        float rowWidth = sqrt(radius * radius - dy * dy);
        int endX;

        if (illumRatio > 0.5) {
          // Prima metà: ombra sottile a destra
          float darkWidth = rowWidth * (1.0 - (illumRatio - 0.5) * 2.0);
          endX = x + (int)rowWidth - (int)darkWidth;
        } else {
          // Seconda metà: ombra oltre la metà
          float darkStart = rowWidth * (illumRatio * 2.0);
          endX = x - (int)darkStart;
        }

        int startX = x + (int)rowWidth;
        if (endX < startX) {
          gfx->drawFastHLine(endX, y + dy, startX - endX, moonDark);
        }
      }
    }
  }

  // Aggiungi dettagli crateri sulla parte illuminata
  if (phase > 0.1 && phase < 0.9) {
    // Crateri piccoli (solo se la luna è abbastanza illuminata)
    int craterSize = radius / 8;
    if (craterSize < 2) craterSize = 2;

    // Posizioni relative dei crateri
    int cx1 = x - radius/4;
    int cy1 = y - radius/4;
    int cx2 = x + radius/5;
    int cy2 = y + radius/6;
    int cx3 = x - radius/6;
    int cy3 = y + radius/3;

    // Disegna crateri solo se nella parte illuminata
    if (phase >= 0.25 && phase <= 0.75) {
      gfx->drawCircle(cx1, cy1, craterSize, moonCrater);
      gfx->drawCircle(cx2, cy2, craterSize - 1, moonCrater);
      gfx->drawCircle(cx3, cy3, craterSize, moonCrater);
    }
  }

  // Bordo sottile per definire meglio la luna
  gfx->drawCircle(x, y, radius, 0x8410);
}

// ================== TEMPERATURE ==================
void drawWSTemperatures() {
  // Cancella SOLO area contenuto (sotto il label, dentro i bordi)
  // Espansa per includere la riga PERCEPITA
  gfx->fillRect(12, 145, 226, 73, WS_BG_COLOR);

  // INTERNO (BME280 o Radar remoto)
  gfx->setFont(u8g2_font_helvB10_tr);
  gfx->setTextColor(WS_ORANGE);
  gfx->setCursor(20, 160);
  gfx->print(TR(TXT_INDOOR));

  gfx->setFont(u8g2_font_helvB14_tr);
  if (indoorSensorSource > 0) {
    // Colore in base alla temperatura
    uint16_t tempColor = (temperatureIndoor > 25) ? WS_TEMP_HOT :
                         (temperatureIndoor < 18) ? WS_TEMP_COLD : WS_TEXT_COLOR;
    gfx->setTextColor(tempColor);
    char tempStr[15];
    sprintf(tempStr, "%.1f", temperatureIndoor);
    gfx->setCursor(100, 160);
    gfx->print(tempStr);
    int16_t dx = gfx->getCursorX();
    gfx->drawCircle(dx + 2, 148, 2, tempColor);
    gfx->setCursor(dx + 6, 160);
    gfx->print("C");

    // Umidità
    gfx->setTextColor(WS_ACCENT_COLOR);
    char humStr[10];
    sprintf(humStr, "%.0f%%", humidityIndoor);
    gfx->setCursor(170, 160);
    gfx->print(humStr);
  } else {
    gfx->setTextColor(0x8410);
    gfx->setCursor(100, 160);
    gfx->print(TR(TXT_NOT_AVAILABLE));
  }

  // ESTERNO
  gfx->setFont(u8g2_font_helvB10_tr);
  gfx->setTextColor(WS_ACCENT_COLOR);
  gfx->setCursor(20, 185);
  gfx->print(TR(TXT_OUTDOOR));

  gfx->setFont(u8g2_font_helvB14_tr);
  if (weatherDataValid) {
    uint16_t tempColor = (weatherTempOutdoor > 25) ? WS_TEMP_HOT :
                         (weatherTempOutdoor < 10) ? WS_TEMP_COLD : WS_TEXT_COLOR;
    gfx->setTextColor(tempColor);
    char tempStr[15];
    sprintf(tempStr, "%.1f", weatherTempOutdoor);
    gfx->setCursor(100, 185);
    gfx->print(tempStr);
    int16_t dx = gfx->getCursorX();
    gfx->drawCircle(dx + 2, 173, 2, tempColor);
    gfx->setCursor(dx + 6, 185);
    gfx->print("C");

    // Umidità
    gfx->setTextColor(WS_ACCENT_COLOR);
    char humStr[10];
    sprintf(humStr, "%.0f%%", weatherHumOutdoor);
    gfx->setCursor(170, 185);
    gfx->print(humStr);

    // Percepita - stesso stile di INTERNO/ESTERNO
    gfx->setFont(u8g2_font_helvB10_tr);
    gfx->setTextColor(WS_GREEN);
    gfx->setCursor(20, 210);
    gfx->print(TR(TXT_FEELS_LIKE));

    gfx->setFont(u8g2_font_helvB14_tr);
    uint16_t feelsColor = (weatherFeelsLike > 25) ? WS_TEMP_HOT :
                          (weatherFeelsLike < 10) ? WS_TEMP_COLD : WS_TEXT_COLOR;
    gfx->setTextColor(feelsColor);
    char feelsStr[15];
    sprintf(feelsStr, "%.1f", weatherFeelsLike);
    gfx->setCursor(100, 210);
    gfx->print(feelsStr);
    int16_t dxf = gfx->getCursorX();
    gfx->drawCircle(dxf + 2, 198, 2, feelsColor);
    gfx->setCursor(dxf + 6, 210);
    gfx->print("C");
  } else {
    gfx->setTextColor(0x8410);
    gfx->setCursor(100, 185);
    gfx->print(TR(TXT_NOT_AVAILABLE));
  }

  // Condizioni meteo (in alto a destra)
  gfx->fillRect(252, 56, 216, 52, WS_BG_COLOR);
  drawWSWeatherIcon(300, 80, weatherCode, 50);  // Alzato di 5 pixel

  gfx->setFont(u8g2_font_helvB10_tr);
  gfx->setTextColor(WS_TEXT_COLOR);

  if (weatherDataValid) {
    String desc = weatherDescription;
    // Se contiene spazio e lunghezza > 10, spezza su due righe
    int spaceIdx = desc.indexOf(' ');
    if (spaceIdx > 0 && desc.length() > 10) {
      // Prima riga
      gfx->setCursor(335, 75);
      gfx->print(desc.substring(0, spaceIdx).c_str());
      // Seconda riga
      gfx->setCursor(335, 92);
      gfx->print(desc.substring(spaceIdx + 1).c_str());
    } else {
      gfx->setCursor(335, 85);
      gfx->print(desc.c_str());
    }
  }
}

// ================== BUSSOLA VENTO ==================
// Variabili extern dal magnetometro
extern bool magnetometerConnected;
extern float magnetometerHeading;

// Variabili per salvare la posizione precedente del triangolo magnetometro
static float lastMagHeading = -1000;  // Valore invalido iniziale
static int16_t lastCompassCX = 0, lastCompassCY = 0;

void drawWSWindCompass(int16_t cx, int16_t cy, int deg, float speed) {
  int radius = 30;  // Bussola leggermente più piccola
  int triRadius = radius + 8;  // Distanza del triangolo dal centro
  int triSize = 6;

  // ========== CANCELLA TRIANGOLO PRECEDENTE ==========
  if (lastMagHeading > -999 && magnetometerConnected) {
    // Calcola posizione vecchia
    float oldAngleRad = (-lastMagHeading - 90) * DEG_TO_RAD;
    int oldTriCenterX = lastCompassCX + cos(oldAngleRad) * triRadius;
    int oldTriCenterY = lastCompassCY + sin(oldAngleRad) * triRadius;

    // Cancella area del triangolo vecchio (quadrato che lo contiene)
    gfx->fillRect(oldTriCenterX - triSize - 2, oldTriCenterY - triSize - 2,
                  triSize * 2 + 4, triSize * 2 + 4, WS_BG_COLOR);
  }

  // Salva posizione attuale della bussola
  lastCompassCX = cx;
  lastCompassCY = cy;

  // Cerchio bussola
  gfx->drawCircle(cx, cy, radius, 0x4208);
  gfx->drawCircle(cx, cy, radius - 1, 0x4208);

  // ========== TRIANGOLO MAGNETOMETRO (Nord magnetico) ==========
  if (magnetometerConnected) {
    // Il triangolo indica dove punta il Nord magnetico
    // Angolo: magnetometerHeading indica dove il dispositivo sta puntando rispetto al Nord
    // Per mostrare dove sia il Nord, dobbiamo invertire: il Nord e' a -heading
    float magAngleRad = (-magnetometerHeading - 90) * DEG_TO_RAD;  // -90 perche' 0° e' in alto

    // Posizione del triangolo sulla circonferenza esterna
    int triCenterX = cx + cos(magAngleRad) * triRadius;
    int triCenterY = cy + sin(magAngleRad) * triRadius;

    // Triangolo che punta verso il centro (indica il Nord)
    float pointAngle = magAngleRad + PI;  // Punta verso il centro

    int tipX = triCenterX + cos(pointAngle) * triSize;
    int tipY = triCenterY + sin(pointAngle) * triSize;
    int base1X = triCenterX + cos(pointAngle + 2.3) * triSize;
    int base1Y = triCenterY + sin(pointAngle + 2.3) * triSize;
    int base2X = triCenterX + cos(pointAngle - 2.3) * triSize;
    int base2Y = triCenterY + sin(pointAngle - 2.3) * triSize;

    // Disegna triangolo rosso per il Nord magnetico
    gfx->fillTriangle(tipX, tipY, base1X, base1Y, base2X, base2Y, 0xF800);  // Rosso
    gfx->drawTriangle(tipX, tipY, base1X, base1Y, base2X, base2Y, 0xFFFF);  // Bordo bianco

    // Salva heading attuale per la prossima cancellazione
    lastMagHeading = magnetometerHeading;
  }

  // Punti cardinali (tradotti)
  gfx->setFont(u8g2_font_helvB08_tr);
  gfx->setTextColor(WS_TEXT_COLOR);
  gfx->setCursor(cx - 4, cy - radius + 10);
  gfx->print(TR(TXT_COMPASS_N));
  gfx->setCursor(cx - 4, cy + radius - 2);
  gfx->print(TR(TXT_COMPASS_S));
  gfx->setCursor(cx + radius - 8, cy + 4);
  gfx->print(TR(TXT_COMPASS_E));
  gfx->setCursor(cx - radius + 2, cy + 4);
  gfx->print(TR(TXT_COMPASS_W));

  // Freccia direzione vento
  float angleRad = (deg - 90) * DEG_TO_RAD;  // -90 perché 0° è Nord (in alto)
  int arrowLen = radius - 10;

  int tipX = cx + cos(angleRad) * arrowLen;
  int tipY = cy + sin(angleRad) * arrowLen;
  int baseX = cx - cos(angleRad) * 6;
  int baseY = cy - sin(angleRad) * 6;

  // Freccia
  gfx->fillTriangle(tipX, tipY,
                    baseX + cos(angleRad + 2.5) * 6, baseY + sin(angleRad + 2.5) * 6,
                    baseX + cos(angleRad - 2.5) * 6, baseY + sin(angleRad - 2.5) * 6,
                    WS_ORANGE);

  // Info vento a DESTRA della bussola (in colonna)
  int16_t infoX = cx + radius + 15;  // A destra della bussola
  int16_t infoY = cy - 20;           // Inizio info

  // Gradi
  gfx->setFont(u8g2_font_helvB10_tr);
  gfx->setTextColor(WS_TEXT_COLOR);
  char degStr[15];
  sprintf(degStr, "%d", deg);
  gfx->setCursor(infoX, infoY);
  gfx->print(degStr);
  // Simbolo gradi
  int16_t dx = gfx->getCursorX();
  gfx->drawCircle(dx + 2, infoY - 8, 2, WS_TEXT_COLOR);

  // Velocità m/s (sotto i gradi)
  gfx->setFont(u8g2_font_helvB10_tr);
  gfx->setTextColor(WS_ACCENT_COLOR);
  char speedStr[15];
  sprintf(speedStr, "%.1f m/s", speed);
  gfx->setCursor(infoX, infoY + 18);
  gfx->print(speedStr);

  // Direzione cardinale (sotto m/s)
  gfx->setFont(u8g2_font_helvB12_tr);
  gfx->setTextColor(WS_ORANGE);
  const char* dirStr = getWindDirection(deg);
  gfx->setCursor(infoX, infoY + 38);
  gfx->print(dirStr);
}

// ================== DETTAGLI ==================
void drawWSDetails() {
  // Cancella area (box ridotto a 100 di altezza)
  gfx->fillRect(12, 255, 226, 73, WS_BG_COLOR);

  gfx->setFont(u8g2_font_helvR10_tr);

  if (weatherDataValid) {
    // Pressione
    gfx->setTextColor(WS_TEXT_COLOR);
    gfx->setCursor(20, 265);
    gfx->print(TR(TXT_PRESSURE));
    gfx->setTextColor(WS_GREEN);
    char pressStr[15];
    sprintf(pressStr, "%.0f hPa", weatherPressure);
    gfx->setCursor(120, 265);
    gfx->print(pressStr);

    // Nuvolosità
    gfx->setTextColor(WS_TEXT_COLOR);
    gfx->setCursor(20, 285);
    gfx->print(TR(TXT_CLOUDS));
    gfx->setTextColor(0xC618);
    char cloudStr[10];
    sprintf(cloudStr, "%d%%", weatherClouds);
    gfx->setCursor(120, 285);
    gfx->print(cloudStr);

    // Visibilità
    gfx->setTextColor(WS_TEXT_COLOR);
    gfx->setCursor(20, 305);
    gfx->print(TR(TXT_VISIBILITY));
    gfx->setTextColor(WS_ACCENT_COLOR);
    char visStr[15];
    sprintf(visStr, "%.1f km", weatherVisibility);
    gfx->setCursor(120, 305);
    gfx->print(visStr);

    // Umidità esterna
    gfx->setTextColor(WS_TEXT_COLOR);
    gfx->setCursor(20, 325);
    gfx->print(TR(TXT_HUMIDITY));
    gfx->setTextColor(WS_ACCENT_COLOR);
    char humStr[10];
    sprintf(humStr, "%.0f%%", weatherHumOutdoor);
    gfx->setCursor(120, 325);
    gfx->print(humStr);
  } else {
    gfx->setTextColor(0x8410);
    gfx->setCursor(80, 290);
    gfx->print(TR(TXT_NOT_AVAILABLE));
  }

  // VENTO (box destra)
  gfx->fillRect(252, 145, 216, 73, WS_BG_COLOR);

  if (weatherDataValid) {
    drawWSWindCompass(310, 175, weatherWindDeg, weatherWindSpeed);
  }
}

// ================== PREVISIONI DOMANI ==================
void drawWSForecast() {
  // Cancella area (box ridotto a 100 di altezza)
  gfx->fillRect(252, 255, 216, 73, WS_BG_COLOR);

  if (weatherDataValid && weatherTempTomorrowMin > -100) {
    // Icona meteo domani
    drawWSWeatherIcon(295, 290, weatherCodeTomorrow, 45);  // Alzato di 5 pixel

    // Temperature
    gfx->setFont(u8g2_font_helvB12_tr);
    gfx->setTextColor(WS_TEMP_COLD);
    char minStr[10];
    sprintf(minStr, "%.0f", weatherTempTomorrowMin);
    gfx->setCursor(330, 280);
    gfx->print(minStr);
    int16_t dx = gfx->getCursorX();
    gfx->drawCircle(dx + 2, 268, 2, WS_TEMP_COLD);

    gfx->setTextColor(WS_TEXT_COLOR);
    gfx->setCursor(dx + 6, 280);
    gfx->print(" / ");

    gfx->setTextColor(WS_TEMP_HOT);
    char maxStr[10];
    sprintf(maxStr, "%.0f", weatherTempTomorrowMax);
    gfx->print(maxStr);
    dx = gfx->getCursorX();
    gfx->drawCircle(dx + 2, 268, 2, WS_TEMP_HOT);

    // Descrizione
    gfx->setFont(u8g2_font_helvR10_tr);
    gfx->setTextColor(WS_TEXT_COLOR);
    String descTom = weatherDescTomorrow;
    // Se contiene spazio e lunghezza > 10, spezza su due righe
    int spaceIdx = descTom.indexOf(' ');
    if (spaceIdx > 0 && descTom.length() > 10) {
      // Prima riga
      gfx->setCursor(330, 300);
      gfx->print(descTom.substring(0, spaceIdx).c_str());
      // Seconda riga
      gfx->setCursor(330, 317);
      gfx->print(descTom.substring(spaceIdx + 1).c_str());
    } else {
      gfx->setCursor(330, 305);
      gfx->print(descTom.c_str());
    }
  } else {
    gfx->setFont(u8g2_font_helvR10_tr);
    gfx->setTextColor(0x8410);
    gfx->setCursor(320, 295);
    gfx->print(TR(TXT_NOT_AVAILABLE));
  }

  // ========== BOX SOLE E LUNA (sinistra) ==========
  gfx->fillRect(12, 342, 286, 126, WS_BG_COLOR);  // Pulisce da Y=342 per includere alba/tramonto

  if (weatherDataValid && weatherSunrise > 0) {
    // Calcola ore alba e tramonto
    time_t sunriseTime = weatherSunrise;
    time_t sunsetTime = weatherSunset;
    struct tm tmSunriseCopy, tmSunsetCopy;

    struct tm* tmSunrise = localtime(&sunriseTime);
    memcpy(&tmSunriseCopy, tmSunrise, sizeof(struct tm));

    struct tm* tmSunset = localtime(&sunsetTime);
    memcpy(&tmSunsetCopy, tmSunset, sizeof(struct tm));

    int sunriseMinutes = tmSunriseCopy.tm_hour * 60 + tmSunriseCopy.tm_min;
    int sunsetMinutes = tmSunsetCopy.tm_hour * 60 + tmSunsetCopy.tm_min;
    int currentMinutes = currentHour * 60 + currentMinute;

    int daylightMinutes = sunsetMinutes - sunriseMinutes;
    int daylightHours = daylightMinutes / 60;
    int daylightMins = daylightMinutes % 60;

    bool isDayTime = (currentMinutes >= sunriseMinutes && currentMinutes <= sunsetMinutes);

    // ===== RIGA 1: ALBA - ORE SOLE - TRAMONTO (in alto nel box, Y = 350-355) =====
    char sunriseStr[10], sunsetStr[10];
    sprintf(sunriseStr, "%02d:%02d", tmSunriseCopy.tm_hour, tmSunriseCopy.tm_min);
    sprintf(sunsetStr, "%02d:%02d", tmSunsetCopy.tm_hour, tmSunsetCopy.tm_min);

    // Alba a sinistra (icona sole che sorge)
    gfx->fillCircle(22, 352, 4, WS_YELLOW);
    gfx->drawFastHLine(15, 356, 14, WS_YELLOW);  // Linea orizzonte sotto il sole
    gfx->setFont(u8g2_font_helvB10_tr);
    gfx->setTextColor(WS_YELLOW);
    gfx->setCursor(32, 357);
    gfx->print(sunriseStr);

    // Ore di sole al centro (con icona sole)
    gfx->fillCircle(115, 352, 3, 0xFFE0);
    gfx->setFont(u8g2_font_helvB12_tr);
    gfx->setTextColor(WS_TEXT_COLOR);
    char daylightStr[15];
    sprintf(daylightStr, "%dh%02dm", daylightHours, daylightMins);
    gfx->setCursor(125, 357);
    gfx->print(daylightStr);

    // Tramonto a destra (icona sole che tramonta)
    gfx->fillCircle(220, 354, 4, WS_ORANGE);
    gfx->fillRect(214, 354, 12, 6, WS_BG_COLOR);  // Mezzo sole (tramonto)
    gfx->drawFastHLine(214, 354, 12, WS_ORANGE);  // Linea orizzonte
    gfx->setTextColor(WS_ORANGE);
    gfx->setCursor(234, 357);
    gfx->print(sunsetStr);

    // ===== GRAFICO SOLE/LUNA (Y = 370-460) =====
    int16_t arcCenterX = 150;
    int16_t arcBaseY = 425;        // Linea orizzonte
    int16_t arcRadius = 30;        // Raggio arco
    int16_t arcStartX = 30;
    int16_t arcEndX = 270;

    // Linea orizzonte
    gfx->drawFastHLine(arcStartX, arcBaseY, arcEndX - arcStartX, 0x528A);

    // Etichette orizzonte
    gfx->setFont(u8g2_font_helvR08_tr);
    gfx->setTextColor(0x4208);
    gfx->setCursor(arcStartX - 8, arcBaseY + 4);
    gfx->print("E");
    gfx->setCursor(arcEndX + 2, arcBaseY + 4);
    gfx->print("O");

    // Arco sole (sopra orizzonte) - tratteggiato
    for (float angle = PI; angle >= 0; angle -= 0.08) {
      int16_t x = arcCenterX + cos(angle) * (arcRadius + 60);
      int16_t y = arcBaseY - sin(angle) * arcRadius;
      if (x >= arcStartX && x <= arcEndX && y >= 370) {
        gfx->drawPixel(x, y, 0x4208);
      }
    }

    // Arco luna (sotto orizzonte) - tratteggiato
    for (float angle = 0; angle >= -PI; angle -= 0.08) {
      int16_t x = arcCenterX + cos(angle) * (arcRadius + 60);
      int16_t y = arcBaseY - sin(angle) * (arcRadius * 0.4);
      if (x >= arcStartX && x <= arcEndX && y <= arcBaseY + 25 && y >= arcBaseY) {
        gfx->drawPixel(x, y, 0x2104);
      }
    }

    // ===== CALCOLA E DISEGNA POSIZIONE SOLE =====
    float sunProgress = 0;
    if (isDayTime) {
      sunProgress = (float)(currentMinutes - sunriseMinutes) / (sunsetMinutes - sunriseMinutes);
    } else if (currentMinutes < sunriseMinutes) {
      int nightDuration = sunriseMinutes + (1440 - sunsetMinutes);
      sunProgress = (float)((1440 - sunsetMinutes) + currentMinutes) / nightDuration;
    } else {
      int nightDuration = sunriseMinutes + (1440 - sunsetMinutes);
      sunProgress = (float)(currentMinutes - sunsetMinutes) / nightDuration;
    }

    int16_t sunX, sunY;
    if (isDayTime) {
      float sunAngle = PI * (1.0 - sunProgress);
      sunX = arcCenterX + cos(sunAngle) * (arcRadius + 60);
      sunY = arcBaseY - sin(sunAngle) * arcRadius;
      if (sunY < 375) sunY = 375;  // Non sopra il testo
    } else {
      float sunAngle = PI * sunProgress;
      sunX = arcCenterX - cos(sunAngle) * (arcRadius + 60);
      sunY = arcBaseY + sin(sunAngle) * (arcRadius * 0.35) + 3;
    }
    if (sunX < arcStartX + 10) sunX = arcStartX + 10;
    if (sunX > arcEndX - 10) sunX = arcEndX - 10;

    // Disegna sole
    if (isDayTime) {
      gfx->fillCircle(sunX, sunY, 7, WS_YELLOW);
      gfx->fillCircle(sunX, sunY, 5, 0xFFE0);
      for (int i = 0; i < 8; i++) {
        float rayAngle = i * 45 * DEG_TO_RAD;
        gfx->drawLine(sunX + cos(rayAngle) * 9, sunY + sin(rayAngle) * 9,
                      sunX + cos(rayAngle) * 12, sunY + sin(rayAngle) * 12, WS_YELLOW);
      }
    } else {
      gfx->fillCircle(sunX, sunY, 4, 0x4200);
    }

    // ===== CALCOLA E DISEGNA POSIZIONE LUNA =====
    int16_t moonX, moonY;
    if (!isDayTime) {
      float moonAngle = PI * (1.0 - sunProgress);
      moonX = arcCenterX + cos(moonAngle) * (arcRadius + 60);
      moonY = arcBaseY - sin(moonAngle) * arcRadius;
      if (moonY < 375) moonY = 375;  // Non sopra il testo
    } else {
      float moonAngle = PI * sunProgress;
      moonX = arcCenterX - cos(moonAngle) * (arcRadius + 60);
      moonY = arcBaseY + sin(moonAngle) * (arcRadius * 0.35) + 3;
    }
    if (moonX < arcStartX + 10) moonX = arcStartX + 10;
    if (moonX > arcEndX - 10) moonX = arcEndX - 10;

    // Disegna luna
    if (!isDayTime) {
      drawWSMoonIcon(moonX, moonY, 16, moonPhase);
    } else {
      gfx->fillCircle(moonX, moonY, 4, 0x3186);
      gfx->fillCircle(moonX + 2, moonY - 1, 3, WS_BG_COLOR);
    }

    // ===== STATO GIORNO/NOTTE (in basso) =====
    gfx->setFont(u8g2_font_helvB10_tr);
    if (isDayTime) {
      gfx->setTextColor(WS_YELLOW);
      gfx->setCursor(20, 458);
      gfx->print("GIORNO");
    } else {
      gfx->setTextColor(0x528A);
      gfx->setCursor(20, 458);
      gfx->print("NOTTE");
    }

    // Tempo rimanente
    int timeRemaining;
    const char* nextEvent;
    if (isDayTime) {
      timeRemaining = sunsetMinutes - currentMinutes;
      nextEvent = "Tramonto";
    } else if (currentMinutes < sunriseMinutes) {
      timeRemaining = sunriseMinutes - currentMinutes;
      nextEvent = "Alba";
    } else {
      timeRemaining = (1440 - currentMinutes) + sunriseMinutes;
      nextEvent = "Alba";
    }
    int remHours = timeRemaining / 60;
    int remMins = timeRemaining % 60;

    gfx->setFont(u8g2_font_helvR10_tr);
    gfx->setTextColor(0x8410);
    char remStr[25];
    sprintf(remStr, "%s -%dh%02dm", nextEvent, remHours, remMins);
    gfx->setCursor(100, 458);
    gfx->print(remStr);
  }

  // ========== BOX LUNA (destra) ==========
  gfx->fillRect(312, 360, 156, 108, WS_BG_COLOR);

  // Disegna icona luna grande
  drawWSMoonIcon(390, 410, 50, moonPhase);

  // Nome fase lunare
  gfx->setFont(u8g2_font_helvB10_tr);
  gfx->setTextColor(0xC618);  // Grigio chiaro
  gfx->setCursor(320, 375);
  gfx->print(moonPhaseName.c_str());

  // Illuminazione
  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(WS_ACCENT_COLOR);
  char illumStr[15];
  sprintf(illumStr, "%.0f%% luce", moonIllumination);
  gfx->setCursor(320, 450);
  gfx->print(illumStr);

  // Età luna
  gfx->setTextColor(0x8410);
  char ageStr[15];
  sprintf(ageStr, "Giorno %d", moonAge);
  gfx->setCursor(320, 465);
  gfx->print(ageStr);
}

#endif // EFFECT_WEATHER_STATION
