//Il problema è che quasi tutti i social hanno chiuso le API gratuite:
//
//  ┌─────────────┬────────────────────┬────────────────────────────────────────┐
//  │ Piattaforma │    API Follower    │                 Costo                  │
//  ├─────────────┼────────────────────┼────────────────────────────────────────┤
//  │ YouTube     │ API Key gratuita   │ Gratis (10K req/giorno)                │
//  ├─────────────┼────────────────────┼────────────────────────────────────────┤
//  │ Twitter/X   │ API v2             │ $100/mese (Basic)                      │
//  ├─────────────┼────────────────────┼────────────────────────────────────────┤
//  │ Instagram   │ Facebook Graph API │ Richiede OAuth + App review            │
//  ├─────────────┼────────────────────┼────────────────────────────────────────┤
//  │ TikTok      │ Research API       │ Richiede approvazione                  │
//  ├─────────────┼────────────────────┼────────────────────────────────────────┤
//  │ Twitch      │ Helix API          │ Gratis con Client ID + OAuth           │
//  ├─────────────┼────────────────────┼────────────────────────────────────────┤
//  │ GitHub      │ REST API           │ Gratis, nessuna auth per dati pubblici │
//  └─────────────┴────────────────────┴────────────────────────────────────────┘
//
//  Le uniche opzioni realistiche per ESP32 senza spendere sono:
//
//  1. YouTube — già implementato, funziona con API key gratuita
//  2. GitHub — API pubblica e gratuita, mostra followers/repos/stars
//  3. Twitch — fattibile ma serve OAuth (client ID + secret)
//  4. Scraping pagine pubbliche — funziona per TikTok/Instagram/X leggendo l'HTML del profilo pubblico, ma è fragile (può interrompersi se cambiano il sito)
// ================== YOUTUBE STATS - DISPLAY MODE ==================
// Mostra statistiche canale YouTube: iscritti, visualizzazioni, video
// Dati recuperati tramite YouTube Data API v3
// Fetch ogni 5 minuti, smart redraw solo se dati cambiano

#ifdef EFFECT_YOUTUBE

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ===== Configurazione API =====
// API Key e Channel ID configurabili da /settings (salvate su LittleFS)
String youtubeApiKey = "";
String youtubeChannelId = "";
#define YOUTUBE_FETCH_INTERVAL 300000  // 5 minuti in ms

// ===== Colori display =====
#define YT_BG_COLOR     0x0000  // Nero
#define YT_RED          0xF800  // Rosso YouTube
#define YT_RED_DARK     0xA000  // Rosso scuro
#define YT_WHITE        0xFFFF  // Bianco
#define YT_ORANGE       0xFD20  // Arancione
#define YT_GRAY         0x7BEF  // Grigio
#define YT_DARK_GRAY    0x2104  // Grigio scuro
#define YT_CARD_BG      0x1082  // Sfondo card

// ===== Variabili globali =====
String ytChannelName = "";
long   ytSubscribers = 0;
long   ytViews = 0;
long   ytVideos = 0;
String ytLastUpdate = "--";
String ytError = "";
bool   youtubeInitialized = false;

// Cache per smart redraw
static long   prevYtSubscribers = -1;
static long   prevYtViews = -1;
static long   prevYtVideos = -1;
static String prevYtChannelName = "";
static uint8_t prevMinute = 255;

static unsigned long lastYtFetch = 0;
static bool ytFirstFetch = true;

extern uint8_t currentHour, currentMinute, currentSecond;
extern Timezone myTZ;

// ===== Helper: formatta numeri grandi =====
String formatYtNumber(long n) {
  if (n >= 1000000) {
    float m = n / 1000000.0;
    if (m >= 100) return String((int)m) + "M";
    return String(m, 1) + "M";
  }
  if (n >= 1000) {
    float k = n / 1000.0;
    if (k >= 100) return String((int)k) + "K";
    return String(k, 1) + "K";
  }
  return String(n);
}

// ===== Fetch statistiche da YouTube API =====
void fetchYoutubeStats() {
  if (youtubeApiKey.length() == 0 || youtubeChannelId.length() == 0) {
    ytError = "API Key o Channel ID non configurati";
    ytChannelName = "Non configurato";
    Serial.println("[YOUTUBE] API Key o Channel ID non configurati - configura da /settings");
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    ytError = "WiFi non connesso";
    Serial.println("[YOUTUBE] WiFi non connesso");
    return;
  }

  Serial.println("[YOUTUBE] Fetch statistiche...");

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15);

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);

  String url = "https://www.googleapis.com/youtube/v3/channels?part=statistics,snippet&id="
               + youtubeChannelId + "&key=" + youtubeApiKey;

  if (!http.begin(client, url)) {
    ytError = "Connessione fallita";
    Serial.println("[YOUTUBE] http.begin() fallito");
    return;
  }

  int httpCode = http.GET();
  Serial.printf("[YOUTUBE] HTTP code: %d\n", httpCode);

  if (httpCode != 200) {
    if (httpCode == 403) {
      ytError = "API quota esaurita o chiave non valida";
    } else if (httpCode < 0) {
      ytError = "Errore connessione: " + http.errorToString(httpCode);
    } else {
      ytError = "Errore HTTP " + String(httpCode);
    }
    Serial.printf("[YOUTUBE] Errore: %s\n", ytError.c_str());
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    ytError = "Errore parsing JSON";
    Serial.printf("[YOUTUBE] JSON error: %s\n", err.c_str());
    return;
  }

  JsonArray items = doc["items"];
  if (items.size() == 0) {
    ytError = "Canale non trovato";
    Serial.println("[YOUTUBE] Nessun canale trovato");
    return;
  }

  JsonObject channel = items[0];
  ytChannelName = channel["snippet"]["title"].as<String>();
  ytSubscribers = channel["statistics"]["subscriberCount"].as<long>();
  ytViews = channel["statistics"]["viewCount"].as<long>();
  ytVideos = channel["statistics"]["videoCount"].as<long>();
  ytError = "";

  // Timestamp ultimo aggiornamento
  char timeBuf[6];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", currentHour, currentMinute);
  ytLastUpdate = String(timeBuf);

  Serial.printf("[YOUTUBE] %s - Subs: %ld, Views: %ld, Videos: %ld\n",
                ytChannelName.c_str(), ytSubscribers, ytViews, ytVideos);
}

// ===== Disegna logo YouTube (rettangolo rosso + triangolo bianco) =====
static void drawYoutubeLogo(int cx, int cy, int w, int h) {
  // Rettangolo rosso arrotondato
  gfx->fillRoundRect(cx - w/2, cy - h/2, w, h, 12, YT_RED);
  // Triangolo play bianco
  int triW = w / 4;
  int triH = h / 2;
  int x0 = cx - triW / 3;
  int y0 = cy - triH / 2;
  int x1 = cx - triW / 3;
  int y1 = cy + triH / 2;
  int x2 = cx + triW * 2 / 3;
  int y2 = cy;
  gfx->fillTriangle(x0, y0, x1, y1, x2, y2, YT_WHITE);
}

// ===== Disegna una card statistica =====
static void drawStatCard(int x, int y, int w, int h, const char* value, const char* label, uint16_t valueColor) {
  // Sfondo card
  gfx->fillRoundRect(x, y, w, h, 10, YT_CARD_BG);
  // Bordo sottile
  gfx->drawRoundRect(x, y, w, h, 10, YT_DARK_GRAY);

  // Valore grande centrato
  gfx->setFont(u8g2_font_logisoso30_tn);
  // Calcolo larghezza con font numerico - conta i caratteri e stima
  int charCount = strlen(value);
  int valWidth = 0;
  for (int i = 0; i < charCount; i++) {
    if (value[i] >= '0' && value[i] <= '9') valWidth += 20;
    else if (value[i] == '.') valWidth += 10;
    else valWidth += 16; // lettere K, M
  }
  // Per lettere (K, M) che non sono in _tn, usiamo un font diverso
  // Usiamo solo il font numerico per il numero, poi il suffisso con font testo
  String valStr = String(value);
  String numPart = "";
  String suffixPart = "";
  for (unsigned int i = 0; i < valStr.length(); i++) {
    char c = valStr.charAt(i);
    if ((c >= '0' && c <= '9') || c == '.' || c == ',') {
      numPart += c;
    } else {
      suffixPart = valStr.substring(i);
      break;
    }
  }

  // Disegna numero con font grande
  gfx->setFont(u8g2_font_logisoso30_tn);
  int numW = numPart.length() * 20;
  int suffW = suffixPart.length() * 14;
  int totalW = numW + suffW;
  int startX = x + (w - totalW) / 2;
  int valY = y + h / 2 + 5;

  gfx->setTextColor(valueColor);
  gfx->setCursor(startX, valY);
  gfx->print(numPart);

  // Suffisso (K, M) con font testo
  if (suffixPart.length() > 0) {
    gfx->setFont(u8g2_font_helvB14_tr);
    gfx->setCursor(startX + numW, valY);
    gfx->print(suffixPart);
  }

  // Label sotto
  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(YT_GRAY);
  int lblW = strlen(label) * 7;
  gfx->setCursor(x + (w - lblW) / 2, y + h - 12);
  gfx->print(label);
}

// ===== Inizializzazione schermo completo =====
void initYoutubeStation() {
  gfx->fillScreen(YT_BG_COLOR);

  // Logo YouTube in alto
  drawYoutubeLogo(240, 60, 120, 80);

  // Nome canale
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(YT_WHITE);
  String name = ytChannelName.length() > 0 ? ytChannelName : "YouTube Stats";
  int nameW = name.length() * 10;
  gfx->setCursor((480 - nameW) / 2, 125);
  gfx->print(name);

  // 3 Card statistiche (distribuite verticalmente)
  int cardW = 400;
  int cardH = 75;
  int cardX = (480 - cardW) / 2;
  int cardStartY = 150;
  int cardGap = 12;

  String subsStr = formatYtNumber(ytSubscribers);
  String viewsStr = formatYtNumber(ytViews);
  String videosStr = formatYtNumber(ytVideos);

  drawStatCard(cardX, cardStartY, cardW, cardH,
               subsStr.c_str(), "ISCRITTI", YT_RED);
  drawStatCard(cardX, cardStartY + cardH + cardGap, cardW, cardH,
               viewsStr.c_str(), "VISUALIZZAZIONI", YT_WHITE);
  drawStatCard(cardX, cardStartY + 2 * (cardH + cardGap), cardW, cardH,
               videosStr.c_str(), "VIDEO PUBBLICATI", YT_ORANGE);

  // Status bar in basso
  gfx->fillRect(0, 440, 480, 40, YT_CARD_BG);
  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(YT_GRAY);

  // Orologio a sinistra
  char timeBuf[6];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", currentHour, currentMinute);
  gfx->setCursor(15, 465);
  gfx->print(timeBuf);

  // Ultimo aggiornamento a destra
  String updateStr = "Agg: " + ytLastUpdate;
  int updW = updateStr.length() * 7;
  gfx->setCursor(480 - updW - 15, 465);
  gfx->print(updateStr);

  // Errore (se presente)
  if (ytError.length() > 0) {
    gfx->setFont(u8g2_font_helvR10_tr);
    gfx->setTextColor(YT_RED);
    int errW = ytError.length() * 7;
    gfx->setCursor((480 - errW) / 2, 420);
    gfx->print(ytError);
  }

  youtubeInitialized = true;
  Serial.println("[YOUTUBE] Display inizializzato");
}

// ===== Aggiornamento (chiamato nel loop) =====
void updateYoutubeStation() {
  unsigned long now = millis();

  // Primo fetch appena possibile
  if (ytFirstFetch) {
    ytFirstFetch = false;
    fetchYoutubeStats();
    // Forza ridisegno dopo primo fetch
    youtubeInitialized = false;
    initYoutubeStation();
    prevYtSubscribers = ytSubscribers;
    prevYtViews = ytViews;
    prevYtVideos = ytVideos;
    prevYtChannelName = ytChannelName;
    prevMinute = currentMinute;
    lastYtFetch = now;
    return;
  }

  // Fetch periodico ogni 5 minuti
  if (now - lastYtFetch >= YOUTUBE_FETCH_INTERVAL) {
    lastYtFetch = now;
    fetchYoutubeStats();
  }

  // Smart redraw: solo se dati cambiano
  bool dataChanged = (ytSubscribers != prevYtSubscribers ||
                      ytViews != prevYtViews ||
                      ytVideos != prevYtVideos ||
                      ytChannelName != prevYtChannelName);

  if (dataChanged) {
    // Ridisegno completo se dati cambiano
    initYoutubeStation();
    prevYtSubscribers = ytSubscribers;
    prevYtViews = ytViews;
    prevYtVideos = ytVideos;
    prevYtChannelName = ytChannelName;
    prevMinute = currentMinute;
    return;
  }

  // Aggiorna solo orologio nella status bar ogni minuto
  if (currentMinute != prevMinute) {
    prevMinute = currentMinute;
    // Ridisegna solo la status bar
    gfx->fillRect(0, 440, 480, 40, YT_CARD_BG);
    gfx->setFont(u8g2_font_helvR10_tr);
    gfx->setTextColor(YT_GRAY);

    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", currentHour, currentMinute);
    gfx->setCursor(15, 465);
    gfx->print(timeBuf);

    String updateStr = "Agg: " + ytLastUpdate;
    int updW = updateStr.length() * 7;
    gfx->setCursor(480 - updW - 15, 465);
    gfx->print(updateStr);
  }
}

#endif // EFFECT_YOUTUBE
