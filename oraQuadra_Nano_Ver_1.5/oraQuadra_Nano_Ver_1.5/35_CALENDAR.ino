#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ezTime.h>

// ============================================================================
// DEFINIZIONI GLOBALI - STABILI (disponibili anche senza EFFECT_CALENDAR)
// ============================================================================
String calendarGoogleUrl = "";
String calendarApiKey = "";

#ifdef EFFECT_CALENDAR

extern bool calendarStationInitialized;
extern uint8_t currentHour, currentMinute, currentSecond;
extern Timezone myTZ;

// ============================================================================
// COLORI
// ============================================================================
#define CAL_BG_COLOR      0x0000   // Nero
#define CAL_ACCENT        0x07FF   // Cyan
#define CAL_ACCENT_DARK   0x0575   // Cyan scuro
#define CAL_CARD          0x1082   // Blu-grigio scuro
#define CAL_TEXT          0xFFFF   // Bianco
#define CAL_URGENT        0xF800   // Rosso
#define CAL_TIME_DIM      0x07E0   // Verde
#define CAL_LOCAL_DOT     0x07E0   // Verde (evento locale)
#define CAL_GOOGLE_DOT    0x43FF   // Blu Google
#define CAL_SYNCED_DOT    0x07FF   // Cyan (sincronizzato)
#define CAL_GRID_LINE     0x2104   // Grigio scuro per griglia
#define CAL_OTHER_MONTH   0x3186   // Grigio per giorni fuori mese
#define CAL_TODAY_BG      0x0292   // Sfondo giorno corrente
#define CAL_BADGE_BG      0xF800   // Sfondo badge conteggio

// ============================================================================
// STRUCT EVENTO ESTESA
// ============================================================================
struct CalendarEvent {
  uint16_t id;        // ID locale (0 per Google-only)
  String title;
  String date;        // DD/MM/YYYY
  String dateShort;   // DD/MM (display)
  String start;       // HH:mm
  String end;         // HH:mm
  int startMinutes;
  int endMinutes;
  uint8_t source;     // 0=locale, 1=google, 2=sincronizzato
  String googleId;    // ID Google Calendar
};

// ============================================================================
// BUFFER
// ============================================================================
CalendarEvent googleEventsBuffer[10];   // solo Google (da fetch)
int googleEventCount = 0;
CalendarEvent mergedEventsBuffer[30];   // locale + Google mergiati
int mergedEventCount = 0;
bool isCalendarUpdating = false;

// ============================================================================
// VARIABILI STATO GRIGLIA
// ============================================================================
bool calendarGridView = true;      // true=griglia mese, false=dettaglio giorno
int calendarViewMonth = 0;         // 1-12 (0 = usa mese corrente)
int calendarViewYear = 0;          // anno visualizzato
int calendarSelectedDay = 0;       // giorno selezionato per dettaglio
static bool calendarNeedsFullRedraw = true;

// ============================================================================
// LAYOUT COSTANTI (480x480)
// ============================================================================
#define CAL_HEADER_H       45
#define CAL_WEEKDAY_H      25
#define CAL_GRID_TOP       (CAL_HEADER_H + CAL_WEEKDAY_H)  // 70
#define CAL_GRID_BOTTOM    470
#define CAL_GRID_ROWS      6
#define CAL_GRID_COLS      7
#define CAL_CELL_W         (480 / CAL_GRID_COLS)   // ~68
#define CAL_CELL_H         ((CAL_GRID_BOTTOM - CAL_GRID_TOP) / CAL_GRID_ROWS) // ~66
#define CAL_STATUS_Y       471

// ============================================================================
// FUNZIONI UTILITA' DATA
// ============================================================================

int getDaysInMonth(int month, int year) {
  if (month == 2) {
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) return 29;
    return 28;
  }
  if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
  return 31;
}

// Ritorna giorno della settimana del primo del mese (0=Lunedi, 6=Domenica)
// Usa algoritmo di Tomohiko Sakamoto
int getFirstDayOfWeek(int month, int year) {
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  int y = year;
  if (month < 3) y--;
  int dow = (y + y/4 - y/100 + y/400 + t[month - 1] + 1) % 7;
  // dow: 0=Domenica, 1=Lunedi, ... 6=Sabato
  // Converti a 0=Lunedi, 6=Domenica
  return (dow == 0) ? 6 : dow - 1;
}

// Restituisce il mese/anno corrente da visualizzare
void getViewMonthYear(int &m, int &y) {
  if (calendarViewMonth >= 1 && calendarViewMonth <= 12 && calendarViewYear > 2000) {
    m = calendarViewMonth;
    y = calendarViewYear;
  } else {
    m = myTZ.month();
    y = myTZ.year();
    calendarViewMonth = m;
    calendarViewYear = y;
  }
}

// Conta eventi per un dato giorno
int getEventCountForDay(int day, int month, int year) {
  char dateBuf[12];
  sprintf(dateBuf, "%02d/%02d/%04d", day, month, year);
  String dateStr = String(dateBuf);
  int count = 0;
  for (int i = 0; i < mergedEventCount; i++) {
    if (mergedEventsBuffer[i].date == dateStr) count++;
  }
  return count;
}

// Nome mese italiano (per display calendario)
const char* calGetMonthName(int month) {
  static const char* calMonths[] = {"", "GENNAIO", "FEBBRAIO", "MARZO", "APRILE", "MAGGIO", "GIUGNO",
                                     "LUGLIO", "AGOSTO", "SETTEMBRE", "OTTOBRE", "NOVEMBRE", "DICEMBRE"};
  if (month >= 1 && month <= 12) return calMonths[month];
  return "---";
}

// ============================================================================
// DISEGNO - HEADER MESE
// ============================================================================
void drawCalendarHeader() {
  int m, y;
  getViewMonthYear(m, y);

  // Sfondo header
  gfx->fillRect(0, 0, 480, CAL_HEADER_H, CAL_BG_COLOR);

  // Freccia sinistra <
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setUTF8Print(true);
  gfx->setTextColor(CAL_ACCENT);
  gfx->setCursor(15, 32);
  gfx->print("<");

  // Titolo mese anno centrato
  char titleBuf[30];
  sprintf(titleBuf, "%s %d", calGetMonthName(m), y);
  // Centra il testo
  int titleLen = strlen(titleBuf);
  int titleX = (480 - titleLen * 12) / 2;
  if (titleX < 60) titleX = 60;
  gfx->setCursor(titleX, 32);
  gfx->print(titleBuf);

  // Freccia destra >
  gfx->setCursor(450, 32);
  gfx->print(">");

  // Linea separatore
  gfx->drawFastHLine(0, CAL_HEADER_H - 1, 480, CAL_ACCENT_DARK);
}

// ============================================================================
// DISEGNO - INTESTAZIONI GIORNI SETTIMANA
// ============================================================================
void drawCalendarWeekdays() {
  static const char* days[] = {"LUN", "MAR", "MER", "GIO", "VEN", "SAB", "DOM"};

  gfx->fillRect(0, CAL_HEADER_H, 480, CAL_WEEKDAY_H, CAL_BG_COLOR);
  gfx->setFont(u8g2_font_helvB10_tr);
  gfx->setUTF8Print(true);

  for (int i = 0; i < 7; i++) {
    int x = i * CAL_CELL_W + (CAL_CELL_W - 24) / 2; // centra "LUN" (~24px)
    uint16_t color = (i >= 5) ? CAL_URGENT : CAL_ACCENT_DARK; // weekend in rosso
    gfx->setTextColor(color);
    gfx->setCursor(x, CAL_HEADER_H + 18);
    gfx->print(days[i]);
  }

  gfx->drawFastHLine(0, CAL_GRID_TOP - 1, 480, CAL_GRID_LINE);
}

// ============================================================================
// DISEGNO - SINGOLA CELLA GIORNO
// ============================================================================
void drawCalendarCell(int col, int row, int dayNum, bool isCurrentMonth, bool isToday, int eventCount) {
  int x = col * CAL_CELL_W;
  int y = CAL_GRID_TOP + row * CAL_CELL_H;

  // Sfondo cella
  if (isToday) {
    gfx->fillRect(x + 1, y + 1, CAL_CELL_W - 2, CAL_CELL_H - 2, CAL_TODAY_BG);
    gfx->drawRect(x, y, CAL_CELL_W, CAL_CELL_H, CAL_ACCENT);
  } else {
    gfx->fillRect(x + 1, y + 1, CAL_CELL_W - 2, CAL_CELL_H - 2, CAL_BG_COLOR);
    gfx->drawRect(x, y, CAL_CELL_W, CAL_CELL_H, CAL_GRID_LINE);
  }

  if (dayNum <= 0) return;

  // Numero giorno
  gfx->setFont(u8g2_font_helvB14_tr);
  uint16_t numColor = isToday ? CAL_ACCENT : (isCurrentMonth ? CAL_TEXT : CAL_OTHER_MONTH);
  gfx->setTextColor(numColor);

  char dayStr[4];
  sprintf(dayStr, "%d", dayNum);
  int textX = x + (CAL_CELL_W - strlen(dayStr) * 10) / 2;
  int textY = y + 28;
  gfx->setCursor(textX, textY);
  gfx->print(dayStr);

  // Badge conteggio eventi
  if (eventCount > 0 && isCurrentMonth) {
    int badgeX = x + CAL_CELL_W - 18;
    int badgeY = y + CAL_CELL_H - 18;
    uint16_t badgeColor = CAL_LOCAL_DOT; // Verde di default

    gfx->fillCircle(badgeX, badgeY, 8, badgeColor);
    gfx->setFont(u8g2_font_helvB08_tr);
    gfx->setTextColor(CAL_BG_COLOR);
    char countStr[4];
    sprintf(countStr, "%d", eventCount > 9 ? 9 : eventCount);
    gfx->setCursor(badgeX - 3, badgeY + 4);
    gfx->print(countStr);
  }
}

// ============================================================================
// DISEGNO - GRIGLIA MESE COMPLETA
// ============================================================================
void drawCalendarGrid() {
  int m, y;
  getViewMonthYear(m, y);

  int daysInMonth = getDaysInMonth(m, y);
  int firstDow = getFirstDayOfWeek(m, y); // 0=Lun

  int todayDay = myTZ.day();
  int todayMonth = myTZ.month();
  int todayYear = myTZ.year();

  // Sfondo area griglia
  gfx->fillRect(0, CAL_GRID_TOP, 480, CAL_GRID_BOTTOM - CAL_GRID_TOP, CAL_BG_COLOR);

  int day = 1;
  for (int row = 0; row < CAL_GRID_ROWS; row++) {
    for (int col = 0; col < CAL_GRID_COLS; col++) {
      int cellIndex = row * CAL_GRID_COLS + col;

      if (cellIndex < firstDow) {
        // Celle vuote prima del primo giorno
        drawCalendarCell(col, row, 0, false, false, 0);
      } else if (day <= daysInMonth) {
        bool isToday = (day == todayDay && m == todayMonth && y == todayYear);
        int evCount = getEventCountForDay(day, m, y);
        drawCalendarCell(col, row, day, true, isToday, evCount);
        day++;
      } else {
        // Celle vuote dopo l'ultimo giorno
        drawCalendarCell(col, row, 0, false, false, 0);
      }
    }
  }
}

// ============================================================================
// DISEGNO - BARRA STATO
// ============================================================================
void drawCalendarStatusBar() {
  gfx->fillRect(0, CAL_STATUS_Y, 480, 10, CAL_BG_COLOR);
  gfx->setFont(u8g2_font_helvR08_tr);
  gfx->setTextColor(CAL_ACCENT_DARK);

  char timeBuf[12];
  sprintf(timeBuf, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
  gfx->setCursor(5, 479);
  gfx->print(timeBuf);

  // Conteggio eventi
  char countBuf[20];
  sprintf(countBuf, "%d eventi", mergedEventCount);
  gfx->setCursor(400, 479);
  gfx->print(countBuf);
}

// ============================================================================
// DISEGNO - DETTAGLIO GIORNO (VISTA 2)
// ============================================================================
void drawCalendarDayDetail(int day) {
  int m, y;
  getViewMonthYear(m, y);

  // Sfondo pulito
  gfx->fillScreen(CAL_BG_COLOR);

  // Header con freccia indietro e data
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setUTF8Print(true);
  gfx->setTextColor(CAL_ACCENT);
  gfx->setCursor(15, 32);
  gfx->print("<");

  // Nome giorno e data
  static const char* dayNames[] = {"LUN", "MAR", "MER", "GIO", "VEN", "SAB", "DOM"};
  int dow = getFirstDayOfWeek(m, y);
  dow = (dow + day - 1) % 7;

  char dateBuf[30];
  sprintf(dateBuf, "%s %02d/%02d/%04d", dayNames[dow], day, m, y);
  gfx->setCursor(60, 32);
  gfx->print(dateBuf);

  gfx->drawFastHLine(0, CAL_HEADER_H - 1, 480, CAL_ACCENT_DARK);

  // Orologio corrente
  gfx->fillRoundRect(12, 50, 456, 50, 8, CAL_CARD);
  gfx->setFont(u8g2_font_logisoso30_tn);
  gfx->setTextColor(CAL_TEXT);
  char timeStr[12];
  sprintf(timeStr, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
  gfx->setCursor(155, 88);
  gfx->print(timeStr);

  // Filtra eventi per questo giorno
  char filterDate[12];
  sprintf(filterDate, "%02d/%02d/%04d", day, m, y);
  String filterStr = String(filterDate);

  int dayEventCount = 0;
  int dayEventIndices[10];

  for (int i = 0; i < mergedEventCount && dayEventCount < 10; i++) {
    if (mergedEventsBuffer[i].date == filterStr) {
      dayEventIndices[dayEventCount++] = i;
    }
  }

  // Area eventi
  gfx->drawRoundRect(10, 110, 460, 355, 12, CAL_ACCENT_DARK);

  if (dayEventCount == 0) {
    gfx->setFont(u8g2_font_helvR14_tr);
    gfx->setTextColor(CAL_OTHER_MONTH);
    gfx->setCursor(150, 300);
    gfx->print("Nessun evento");
    return;
  }

  // Lista eventi (max 7 visibili)
  int nowMinutes = currentHour * 60 + currentMinute;

  for (int i = 0; i < dayEventCount && i < 7; i++) {
    int idx = dayEventIndices[i];
    CalendarEvent& ev = mergedEventsBuffer[idx];
    int evY = 138 + (i * 48);

    // Urgenza/in corso
    bool isUrgent = (ev.startMinutes - nowMinutes >= 0 && ev.startMinutes - nowMinutes <= 15);
    bool isInProgress = (nowMinutes >= ev.startMinutes && nowMinutes < ev.endMinutes);
    bool blink = (currentSecond % 2 == 0);
    bool highlight = (isUrgent || isInProgress) && blink;

    // Pallino fonte (verde=locale, blu=Google, cyan=sync)
    uint16_t dotColor = CAL_LOCAL_DOT;
    if (ev.source == 1) dotColor = CAL_GOOGLE_DOT;
    else if (ev.source == 2) dotColor = CAL_SYNCED_DOT;
    gfx->fillCircle(25, evY, 4, dotColor);

    // Orario
    gfx->setFont(u8g2_font_helvB12_tr);
    gfx->setTextColor(highlight ? CAL_URGENT : CAL_ACCENT);
    gfx->setCursor(40, evY + 5);
    String timeDisplay = ev.start;
    if (ev.end.length() > 0) timeDisplay += "-" + ev.end;
    gfx->print(timeDisplay);

    // Titolo
    gfx->setFont(u8g2_font_helvR12_tr);
    gfx->setTextColor(highlight ? CAL_URGENT : CAL_TEXT);
    int titleX = 175;
    int maxChars = (445 - titleX) / 9;
    String t = ev.title;
    if ((int)t.length() > maxChars) {
      int startIdx = (currentSecond / 2) % (t.length() - maxChars + 1);
      gfx->setCursor(titleX, evY + 5);
      gfx->print(t.substring(startIdx, startIdx + maxChars));
    } else {
      gfx->setCursor(titleX, evY + 5);
      gfx->print(t);
    }

    // Separatore
    if (i < dayEventCount - 1 && i < 6) {
      gfx->drawFastHLine(20, evY + 22, 440, CAL_GRID_LINE);
    }
  }

  // Indicatore se ci sono piu' eventi
  if (dayEventCount > 7) {
    gfx->setFont(u8g2_font_helvR08_tr);
    gfx->setTextColor(CAL_OTHER_MONTH);
    char moreBuf[20];
    sprintf(moreBuf, "+%d altri eventi", dayEventCount - 7);
    gfx->setCursor(180, 460);
    gfx->print(moreBuf);
  }
}

// ============================================================================
// AGGIORNAMENTO DETTAGLIO GIORNO (solo parti dinamiche)
// ============================================================================
void updateCalendarDayDetail() {
  int m, y;
  getViewMonthYear(m, y);

  // Aggiorna solo l'orologio
  gfx->fillRoundRect(13, 51, 454, 48, 7, CAL_CARD);
  gfx->setFont(u8g2_font_logisoso30_tn);
  gfx->setTextColor(CAL_TEXT);
  char timeStr[12];
  sprintf(timeStr, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
  gfx->setCursor(155, 88);
  gfx->print(timeStr);

  // Ridisegna eventi per aggiornare blinking urgenza
  char filterDate[12];
  sprintf(filterDate, "%02d/%02d/%04d", calendarSelectedDay, m, y);
  String filterStr = String(filterDate);

  int nowMinutes = currentHour * 60 + currentMinute;
  int dayEventCount = 0;

  for (int i = 0; i < mergedEventCount; i++) {
    if (mergedEventsBuffer[i].date != filterStr) continue;
    if (dayEventCount >= 7) break;

    CalendarEvent& ev = mergedEventsBuffer[i];
    int evY = 138 + (dayEventCount * 48);

    bool isUrgent = (ev.startMinutes - nowMinutes >= 0 && ev.startMinutes - nowMinutes <= 15);
    bool isInProgress = (nowMinutes >= ev.startMinutes && nowMinutes < ev.endMinutes);
    bool blink = (currentSecond % 2 == 0);
    bool highlight = (isUrgent || isInProgress) && blink;

    // Ridisegna solo se evidenziato (blinking)
    if (isUrgent || isInProgress) {
      // Pulisci riga
      gfx->fillRect(15, evY - 20, 450, 40, CAL_BG_COLOR);

      // Pallino
      uint16_t dotColor = CAL_LOCAL_DOT;
      if (ev.source == 1) dotColor = CAL_GOOGLE_DOT;
      else if (ev.source == 2) dotColor = CAL_SYNCED_DOT;
      gfx->fillCircle(25, evY, 4, dotColor);

      // Orario
      gfx->setFont(u8g2_font_helvB12_tr);
      gfx->setTextColor(highlight ? CAL_URGENT : CAL_ACCENT);
      gfx->setCursor(40, evY + 5);
      String timeDisplay = ev.start;
      if (ev.end.length() > 0) timeDisplay += "-" + ev.end;
      gfx->print(timeDisplay);

      // Titolo
      gfx->setFont(u8g2_font_helvR12_tr);
      gfx->setTextColor(highlight ? CAL_URGENT : CAL_TEXT);
      int titleX = 175;
      int maxChars = (445 - titleX) / 9;
      String t = ev.title;
      if ((int)t.length() > maxChars) {
        int startIdx = (currentSecond / 2) % (t.length() - maxChars + 1);
        gfx->setCursor(titleX, evY + 5);
        gfx->print(t.substring(startIdx, startIdx + maxChars));
      } else {
        gfx->setCursor(titleX, evY + 5);
        gfx->print(t);
      }

      // Separatore
      if (dayEventCount < 6) {
        gfx->drawFastHLine(20, evY + 22, 440, CAL_GRID_LINE);
      }
    }

    dayEventCount++;
  }
}

// ============================================================================
// TOUCH HANDLER
// ============================================================================
void handleCalendarTouch(int x, int y) {
  if (calendarGridView) {
    // === VISTA GRIGLIA MESE ===

    // Freccia < (mese precedente)
    if (x < 60 && y < CAL_HEADER_H) {
      calendarViewMonth--;
      if (calendarViewMonth < 1) { calendarViewMonth = 12; calendarViewYear--; }
      calendarNeedsFullRedraw = true;
      calendarStationInitialized = false;
      Serial.printf("[CAL-TOUCH] Mese precedente: %d/%d\n", calendarViewMonth, calendarViewYear);
      return;
    }

    // Freccia > (mese successivo)
    if (x > 420 && y < CAL_HEADER_H) {
      calendarViewMonth++;
      if (calendarViewMonth > 12) { calendarViewMonth = 1; calendarViewYear++; }
      calendarNeedsFullRedraw = true;
      calendarStationInitialized = false;
      Serial.printf("[CAL-TOUCH] Mese successivo: %d/%d\n", calendarViewMonth, calendarViewYear);
      return;
    }

    // Touch su griglia -> seleziona giorno
    if (y >= CAL_GRID_TOP && y < CAL_GRID_BOTTOM) {
      int col = x / CAL_CELL_W;
      int row = (y - CAL_GRID_TOP) / CAL_CELL_H;

      if (col >= 0 && col < CAL_GRID_COLS && row >= 0 && row < CAL_GRID_ROWS) {
        int m, yr;
        getViewMonthYear(m, yr);
        int firstDow = getFirstDayOfWeek(m, yr);
        int cellIndex = row * CAL_GRID_COLS + col;
        int day = cellIndex - firstDow + 1;
        int daysInMonth = getDaysInMonth(m, yr);

        if (day >= 1 && day <= daysInMonth) {
          calendarSelectedDay = day;
          calendarGridView = false;
          calendarNeedsFullRedraw = true;
          calendarStationInitialized = false;
          Serial.printf("[CAL-TOUCH] Selezionato giorno %d/%d/%d\n", day, m, yr);
        }
      }
    }

  } else {
    // === VISTA DETTAGLIO GIORNO ===

    // Freccia < (torna a griglia)
    if (x < 60 && y < CAL_HEADER_H) {
      calendarGridView = true;
      calendarNeedsFullRedraw = true;
      calendarStationInitialized = false;
      Serial.println("[CAL-TOUCH] Torna a griglia mese");
      return;
    }
  }
}

// ============================================================================
// FETCH GOOGLE CALENDAR DATA (RISCRITTA CON STRUCT ESTESA)
// ============================================================================
void fetchGoogleCalendarData() {
  if (WiFi.status() != WL_CONNECTED || isCalendarUpdating) return;
  if (calendarGoogleUrl.length() == 0) return;
  isCalendarUpdating = true;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(10000);

  String baseUrl = calendarGoogleUrl;
  int keyIdx = baseUrl.indexOf("?key=");
  if (keyIdx > 0) baseUrl = baseUrl.substring(0, keyIdx);

  String url = baseUrl + "?key=" + calendarApiKey;
  Serial.printf("[CALENDAR] Fetch URL: %s\n", url.c_str());

  if (http.begin(client, url)) {
    int httpCode = http.GET();
    Serial.printf("[CALENDAR] HTTP code: %d\n", httpCode);
    if (httpCode == 200) {
      DynamicJsonDocument doc(8192);
      deserializeJson(doc, http.getString());
      JsonArray arr = doc.as<JsonArray>();
      googleEventCount = 0;
      for (JsonObject obj : arr) {
        if (googleEventCount < 10) {
          googleEventsBuffer[googleEventCount].id = 0;
          googleEventsBuffer[googleEventCount].title = obj["title"].as<String>();

          // Supporta sia il vecchio formato "DD/MM" sia il nuovo "DD/MM/YYYY"
          String dateVal = obj["date"].as<String>();
          if (dateVal.length() <= 5) {
            // Vecchio formato DD/MM -> aggiungi anno corrente
            dateVal += "/" + String(myTZ.year());
          }
          googleEventsBuffer[googleEventCount].date = dateVal;
          googleEventsBuffer[googleEventCount].dateShort = dateVal.substring(0, 5);

          googleEventsBuffer[googleEventCount].start = obj["start"].as<String>();

          // End time (nuovo campo, opzionale)
          if (obj.containsKey("end")) {
            googleEventsBuffer[googleEventCount].end = obj["end"].as<String>();
          } else {
            googleEventsBuffer[googleEventCount].end = "";
          }

          // Google ID (nuovo campo, opzionale)
          if (obj.containsKey("id")) {
            googleEventsBuffer[googleEventCount].googleId = obj["id"].as<String>();
          } else {
            googleEventsBuffer[googleEventCount].googleId = "";
          }

          // Calcola minutes
          String startStr = googleEventsBuffer[googleEventCount].start;
          if (startStr.length() >= 5) {
            int hh = startStr.substring(0, 2).toInt();
            int mm = startStr.substring(3, 5).toInt();
            googleEventsBuffer[googleEventCount].startMinutes = (hh * 60) + mm;
          } else {
            googleEventsBuffer[googleEventCount].startMinutes = 0;
          }

          String endStr = googleEventsBuffer[googleEventCount].end;
          if (endStr.length() >= 5) {
            int hh = endStr.substring(0, 2).toInt();
            int mm = endStr.substring(3, 5).toInt();
            googleEventsBuffer[googleEventCount].endMinutes = (hh * 60) + mm;
          } else {
            googleEventsBuffer[googleEventCount].endMinutes = googleEventsBuffer[googleEventCount].startMinutes + 60;
          }

          googleEventsBuffer[googleEventCount].source = 1; // Google
          googleEventCount++;
        }
      }
      Serial.printf("[CALENDAR] Ricevuti %d eventi Google\n", googleEventCount);
    }
    http.end();
  }
  isCalendarUpdating = false;
}

// drawCalendarEvents() - Compatibilita' backward per chiamate dal main loop
void drawCalendarEvents() {
  if (calendarGridView) {
    drawCalendarGrid();
  }
}

// ============================================================================
// INIT E UPDATE PRINCIPALI
// ============================================================================
bool initCalendarStation() {
  // Inizializza mese/anno se non impostati
  if (calendarViewMonth == 0 || calendarViewYear == 0) {
    calendarViewMonth = myTZ.month();
    calendarViewYear = myTZ.year();
  }

  gfx->fillScreen(CAL_BG_COLOR);

  if (calendarGridView) {
    drawCalendarHeader();
    drawCalendarWeekdays();
    drawCalendarGrid();
    drawCalendarStatusBar();
  } else {
    drawCalendarDayDetail(calendarSelectedDay);
  }

  calendarStationInitialized = true;
  calendarNeedsFullRedraw = false;
  return true;
}

void updateCalendarStation() {
  static uint8_t lastSec = 255;

  if (currentSecond != lastSec) {
    if (calendarGridView) {
      // Aggiorna solo barra stato (orologio)
      drawCalendarStatusBar();
    } else {
      // Aggiorna dettaglio giorno (orologio + blinking)
      updateCalendarDayDetail();
    }
    lastSec = currentSecond;
  }
}

// ============================================================================
// SISTEMA ALLARME CALENDARIO
// ============================================================================
bool calendarAlarmActive = false;          // Allarme in corso
bool calendarAlarmEnabled = true;          // Abilitato globalmente
unsigned long calendarAlarmLastBeep = 0;   // Timestamp ultimo beep
unsigned long calendarAlarmStartTime = 0;  // Quando e' scattato
String calendarAlarmTitle = "";            // Titolo evento
String calendarAlarmTime = "";             // Orario "HH:mm"
String calendarAlarmEnd = "";              // Fine "HH:mm"
int calendarAlarmSnoozeUntil = -1;         // minuti mezzanotte per snooze (-1=nessuno)
int calendarAlarmLastTriggeredMinute = -1; // Evita re-trigger stesso minuto
uint16_t calendarAlarmTriggeredId = 0;     // ID evento che ha triggerato
static bool calendarAlarmFlashState = false; // Per lampeggio barra

// Dati evento snoozato (per re-trigger dopo scadenza snooze)
String snoozedEventTitle = "";
String snoozedEventStart = "";
String snoozedEventEnd = "";
String snoozedEventDate = "";

#define CAL_ALARM_BEEP_INTERVAL 1500       // Beep ogni 1.5 secondi
#define CAL_ALARM_TIMEOUT 300000           // Auto-stop dopo 5 minuti

// Snooze configurabile via web - EEPROM addr 580
#define EEPROM_CAL_SNOOZE_ADDR  580
#define EEPROM_CAL_SNOOZE_VALID 581        // Marker validita' (0xBB)
int calAlarmSnoozeMinutes = 5;             // Default 5 minuti, caricato da EEPROM

extern void forceDisplayUpdate();

// ---------- Load/Save snooze da EEPROM ----------
void loadCalSnoozeFromEEPROM() {
  if (EEPROM.read(EEPROM_CAL_SNOOZE_VALID) == 0xBB) {
    int val = EEPROM.read(EEPROM_CAL_SNOOZE_ADDR);
    if (val >= 1 && val <= 60) {
      calAlarmSnoozeMinutes = val;
    }
  }
  Serial.printf("[CAL ALARM] Snooze caricato: %d minuti\n", calAlarmSnoozeMinutes);
}

void saveCalSnoozeToEEPROM() {
  EEPROM.write(EEPROM_CAL_SNOOZE_ADDR, (uint8_t)calAlarmSnoozeMinutes);
  EEPROM.write(EEPROM_CAL_SNOOZE_VALID, 0xBB);
  EEPROM.commit();
  Serial.printf("[CAL ALARM] Snooze salvato: %d minuti\n", calAlarmSnoozeMinutes);
}

// ---------- Schermata overlay allarme ----------
void showCalendarAlarmScreen() {
  gfx->fillScreen(BLACK);

  // Barra superiore rossa con "APPUNTAMENTO!"
  gfx->fillRect(0, 0, 480, 80, RED);
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setUTF8Print(true);
  gfx->setTextColor(WHITE);
  gfx->setCursor(100, 52);
  gfx->print("APPUNTAMENTO!");

  // Titolo evento grande al centro
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(CAL_ACCENT);
  // Centra il titolo (max ~25 caratteri visibili)
  String displayTitle = calendarAlarmTitle;
  if (displayTitle.length() > 25) {
    displayTitle = displayTitle.substring(0, 24) + "..";
  }
  int titleLen = displayTitle.length();
  int titleX = (480 - titleLen * 12) / 2;
  if (titleX < 10) titleX = 10;
  gfx->setCursor(titleX, 220);
  gfx->print(displayTitle);

  // Orario sotto al titolo
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(WHITE);
  String timeStr = calendarAlarmTime;
  if (calendarAlarmEnd.length() > 0) {
    timeStr += " - " + calendarAlarmEnd;
  }
  int timeLen = timeStr.length();
  int timeX = (480 - timeLen * 10) / 2;
  if (timeX < 10) timeX = 10;
  gfx->setCursor(timeX, 280);
  gfx->print(timeStr);

  // Pulsante STOP (verde, lato sinistro)
  gfx->fillRoundRect(20, 370, 210, 80, 12, 0x07E0);  // Verde
  gfx->setFont(u8g2_font_helvB18_tr);
  gfx->setTextColor(BLACK);
  gfx->setCursor(80, 420);
  gfx->print("STOP");

  // Pulsante POSTICIPA (arancione, lato destro) con minuti
  gfx->fillRoundRect(250, 370, 210, 80, 12, 0xFD20);  // Arancione
  gfx->setTextColor(BLACK);
  char snzLabel[20];
  sprintf(snzLabel, "+%d min", calAlarmSnoozeMinutes);
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setCursor(310, 408);
  gfx->print(snzLabel);
  gfx->setFont(u8g2_font_helvB10_tr);
  gfx->setCursor(305, 440);
  gfx->print("POSTICIPA");
}

// ---------- Trigger allarme ----------
void triggerCalendarAlarm(CalendarEvent& ev) {
  int nowMinutes = currentHour * 60 + currentMinute;

  calendarAlarmActive = true;
  calendarAlarmTitle = ev.title;
  calendarAlarmTime = ev.start;
  calendarAlarmEnd = ev.end;
  calendarAlarmTriggeredId = ev.id;
  calendarAlarmLastBeep = 0;  // Forza beep immediato
  calendarAlarmStartTime = millis();
  calendarAlarmLastTriggeredMinute = nowMinutes;
  calendarAlarmSnoozeUntil = -1;
  calendarAlarmFlashState = false;

  Serial.printf("[CAL ALARM] Allarme scattato: %s alle %s\n",
                ev.title.c_str(), ev.start.c_str());

  showCalendarAlarmScreen();
  playLocalMP3("beep.mp3");
}

// ---------- Check trigger (chiamata nel loop ogni ciclo) ----------
void checkCalendarAlarm() {
  if (!calendarAlarmEnabled || calendarAlarmActive) return;

  int nowMinutes = currentHour * 60 + currentMinute;

  // Evita re-trigger nello stesso minuto
  if (nowMinutes == calendarAlarmLastTriggeredMinute) return;

  // Gestione snooze: se scaduto, ri-triggera l'evento snoozato direttamente
  if (calendarAlarmSnoozeUntil >= 0) {
    if (nowMinutes < calendarAlarmSnoozeUntil) return;
    // Snooze scaduto! Ri-triggera l'evento snoozato
    calendarAlarmSnoozeUntil = -1;
    if (snoozedEventTitle.length() > 0) {
      calendarAlarmActive = true;
      calendarAlarmTitle = snoozedEventTitle;
      calendarAlarmTime = snoozedEventStart;
      calendarAlarmEnd = snoozedEventEnd;
      calendarAlarmLastBeep = 0;
      calendarAlarmStartTime = millis();
      calendarAlarmLastTriggeredMinute = nowMinutes;
      calendarAlarmFlashState = false;
      snoozedEventTitle = "";
      snoozedEventStart = "";
      snoozedEventEnd = "";
      snoozedEventDate = "";
      Serial.printf("[CAL ALARM] Snooze scaduto, ri-allarme: %s\n", calendarAlarmTitle.c_str());
      showCalendarAlarmScreen();
      playLocalMP3("beep.mp3");
      return;
    }
  }

  // Cerca eventi che matchano l'orario corrente
  int todayDay = myTZ.day();
  int todayMonth = myTZ.month();
  int todayYear = myTZ.year();
  char todayStr[12];
  sprintf(todayStr, "%02d/%02d/%04d", todayDay, todayMonth, todayYear);
  String todayDate = String(todayStr);

  for (int i = 0; i < mergedEventCount; i++) {
    CalendarEvent& ev = mergedEventsBuffer[i];
    if (ev.date == todayDate && ev.startMinutes == nowMinutes) {
      triggerCalendarAlarm(ev);
      return;
    }
  }
}

// ---------- Beep loop + lampeggio (chiamata nel loop) ----------
void updateCalendarAlarmSound() {
  if (!calendarAlarmActive) return;

  unsigned long currentTime = millis();

  // Timeout automatico 5 minuti
  if (currentTime - calendarAlarmStartTime >= CAL_ALARM_TIMEOUT) {
    Serial.println("[CAL ALARM] Timeout 5 minuti - allarme spento");
    stopCalendarAlarm();
    return;
  }

  // Beep ogni CAL_ALARM_BEEP_INTERVAL
  if (currentTime - calendarAlarmLastBeep >= CAL_ALARM_BEEP_INTERVAL) {
    playLocalMP3("beep.mp3");
    calendarAlarmLastBeep = currentTime;

    // Lampeggio barra superiore
    calendarAlarmFlashState = !calendarAlarmFlashState;
    gfx->fillRect(0, 0, 480, 80, calendarAlarmFlashState ? BLACK : RED);
    if (!calendarAlarmFlashState) {
      gfx->setFont(u8g2_font_helvB18_tr);
      gfx->setUTF8Print(true);
      gfx->setTextColor(WHITE);
      gfx->setCursor(100, 52);
      gfx->print("APPUNTAMENTO!");
    }
  }
}

// ---------- Stop allarme ----------
void stopCalendarAlarm() {
  if (!calendarAlarmActive) return;
  calendarAlarmActive = false;
  calendarAlarmTitle = "";
  calendarAlarmTime = "";
  calendarAlarmEnd = "";
  // Cancella anche eventuali dati snooze pendenti
  snoozedEventTitle = "";
  snoozedEventStart = "";
  snoozedEventEnd = "";
  snoozedEventDate = "";
  calendarAlarmSnoozeUntil = -1;
  Serial.println("[CAL ALARM] Allarme fermato");

  // Forza ridisegno del modo corrente
  forceDisplayUpdate();
}

// ---------- Snooze (posticipa) ----------
void snoozeCalendarAlarm() {
  int nowMinutes = currentHour * 60 + currentMinute;
  calendarAlarmSnoozeUntil = nowMinutes + calAlarmSnoozeMinutes;

  // Salva dati evento PRIMA di pulire (per re-trigger dopo snooze)
  snoozedEventTitle = calendarAlarmTitle;
  snoozedEventStart = calendarAlarmTime;
  snoozedEventEnd = calendarAlarmEnd;

  calendarAlarmActive = false;
  calendarAlarmLastTriggeredMinute = -1;  // Permette re-trigger dopo snooze
  calendarAlarmTitle = "";
  calendarAlarmTime = "";
  calendarAlarmEnd = "";

  Serial.printf("[CAL ALARM] Posticipato di %d minuti (snooze fino a %d:%02d)\n",
                calAlarmSnoozeMinutes,
                calendarAlarmSnoozeUntil / 60, calendarAlarmSnoozeUntil % 60);

  // Forza ridisegno del modo corrente
  forceDisplayUpdate();
}

#endif // EFFECT_CALENDAR
