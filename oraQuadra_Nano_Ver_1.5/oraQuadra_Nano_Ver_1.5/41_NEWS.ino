// ================== NEWS FEED - DISPLAY MODE ==================
// Mostra notizie da feed RSS (ANSA, BBC, Repubblica)
// Nessuna API key necessaria - feed RSS gratuiti e senza limiti
// Fetch ogni 10 minuti, smart redraw solo se dati cambiano
// Fonte configurabile: ANSA (IT), BBC (UK), Repubblica (IT) via touch o web

#ifdef EFFECT_NEWS

// Decommentare per abilitare log seriali di debug NEWS
// #define DEBUG_NEWS

#ifdef DEBUG_NEWS
  #define NEWS_LOG(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
  #define NEWS_LOGLN(msg)    Serial.println(msg)
#else
  #define NEWS_LOG(fmt, ...)
  #define NEWS_LOGLN(msg)
#endif

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ===== Configurazione =====
#define NEWS_FETCH_INTERVAL   600000  // 10 minuti
#define NEWS_MAX_ARTICLES     15

// ===== Colori display =====
#define NW_BG_COLOR     0x0000  // Nero
#define NW_ACCENT       0x05BF  // Azzurro news (0,180,255)
#define NW_WHITE        0xFFFF
#define NW_GRAY         0x7BEF
#define NW_DARK_GRAY    0x2104
#define NW_CARD_BG      0x1082
#define NW_TAB_ACTIVE   0x05BF
#define NW_TAB_INACTIVE 0x2104
#define NW_SEPARATOR    0x2945
#define NW_SOURCE_BG    0x18E3

// ===== Struttura articolo =====
struct NewsArticle {
  String title;
  String source;
  String description;
  String publishedAt;
  String link;
};

// ===== Definizione fonti RSS =====
#define NEWS_SOURCE_ANSA  0
#define NEWS_SOURCE_BBC   1
#define NEWS_SOURCE_REPUBBLICA 2
#define NEWS_NUM_SOURCES  3

const char* newsSourceNames[] = {"ANSA", "BBC", "Repubblica"};
const char* newsSourceFlags[] = {"IT", "UK", "IT"};

// --- ANSA (8 categorie) ---
#define ANSA_NUM_CATS 8
const char* ansaCatNames[] = {"Generale", "Cronaca", "Politica", "Economia", "Sport", "Tech", "Cultura", "Mondo"};
const char* ansaCatUrls[] = {
  "https://www.ansa.it/sito/ansait_rss.xml",
  "https://www.ansa.it/sito/notizie/cronaca/cronaca_rss.xml",
  "https://www.ansa.it/sito/notizie/politica/politica_rss.xml",
  "https://www.ansa.it/sito/notizie/economia/economia_rss.xml",
  "https://www.ansa.it/sito/notizie/sport/sport_rss.xml",
  "https://www.ansa.it/sito/notizie/tecnologia/tecnologia_rss.xml",
  "https://www.ansa.it/sito/notizie/cultura/cultura_rss.xml",
  "https://www.ansa.it/sito/notizie/mondo/mondo_rss.xml"
};

// --- BBC (7 categorie) ---
#define BBC_NUM_CATS 7
const char* bbcCatNames[] = {"Top", "World", "Business", "Tech", "Sport", "Arts", "Science"};
const char* bbcCatUrls[] = {
  "https://feeds.bbci.co.uk/news/rss.xml",
  "https://feeds.bbci.co.uk/news/world/rss.xml",
  "https://feeds.bbci.co.uk/news/business/rss.xml",
  "https://feeds.bbci.co.uk/news/technology/rss.xml",
  "https://feeds.bbci.co.uk/news/sport/rss.xml",
  "https://feeds.bbci.co.uk/news/entertainment_and_arts/rss.xml",
  "https://feeds.bbci.co.uk/news/science_and_environment/rss.xml"
};

// --- Repubblica (8 categorie) ---
#define REPUBBLICA_NUM_CATS 8
const char* repubblicaCatNames[] = {"Generale", "Cronaca", "Politica", "Economia", "Sport", "Tech", "Spettacoli", "Esteri"};
const char* repubblicaCatUrls[] = {
  "https://www.repubblica.it/rss/homepage/rss2.0.xml",
  "https://www.repubblica.it/rss/cronaca/rss2.0.xml",
  "https://www.repubblica.it/rss/politica/rss2.0.xml",
  "https://www.repubblica.it/rss/economia/rss2.0.xml",
  "https://www.repubblica.it/rss/sport/rss2.0.xml",
  "https://www.repubblica.it/rss/tecnologia/rss2.0.xml",
  "https://www.repubblica.it/rss/spettacoli/rss2.0.xml",
  "https://www.repubblica.it/rss/esteri/rss2.0.xml"
};

// ===== Variabili globali =====
NewsArticle newsArticles[NEWS_MAX_ARTICLES];
int    newsArticleCount = 0;
String newsCategory = "Generale";
String newsLastUpdate = "--";
String newsError = "";
bool   newsInitialized = false;
int    newsCategoryIndex = 0;
int    newsSourceIndex = 0;  // Default: ANSA
int    newsScrollOffset = 0; // Indice primo articolo visibile (scroll)
String newsOpenUrl = "";     // URL articolo toccato, servito a /news/openarticle

// ===== Layout costanti fisse =====
#define TAB_ROW1_Y      80
#define TAB_H           28
#define TAB_GAP         6
#define TAB_MARGIN      8
#define SOURCE_PILL_H   22
#define SOURCE_PILL_Y   10

// Tab layout dinamico (calcolato per fonte corrente)
static int newsRow1Count = 4;
static int newsRow2Count = 4;
static int newsRow1W = 111;
static int newsRow2W = 111;
static int newsRow2X = 8;

// Cache per smart redraw
static int    prevNewsSourceIndex = -1;
static int    prevNewsCategoryIndex = -1;
static int    prevNewsArticleCount = -1;
static String prevNewsFirstTitle = "";
static uint8_t prevNewsMinute = 255;

static unsigned long lastNewsFetch = 0;
static bool newsFirstFetch = true;

extern uint8_t currentHour, currentMinute, currentSecond;
extern Timezone myTZ;

// ===== Funzioni helper per fonti =====

// Pill larghezza dinamica in base al nome fonte
static int getSourcePillW() {
  return strlen(newsSourceNames[newsSourceIndex]) * 9 + 20;
}
static int getSourcePillX() {
  return 480 - getSourcePillW() - 10;
}

int getSourceCatCount(int src) {
  switch(src) {
    case NEWS_SOURCE_ANSA: return ANSA_NUM_CATS;
    case NEWS_SOURCE_BBC:  return BBC_NUM_CATS;
    case NEWS_SOURCE_REPUBBLICA:  return REPUBBLICA_NUM_CATS;
    default: return 0;
  }
}

const char* getSourceCatName(int src, int cat) {
  switch(src) {
    case NEWS_SOURCE_ANSA: return (cat < ANSA_NUM_CATS) ? ansaCatNames[cat] : "";
    case NEWS_SOURCE_BBC:  return (cat < BBC_NUM_CATS)  ? bbcCatNames[cat]  : "";
    case NEWS_SOURCE_REPUBBLICA:  return (cat < REPUBBLICA_NUM_CATS)  ? repubblicaCatNames[cat]  : "";
    default: return "";
  }
}

const char* getSourceCatUrl(int src, int cat) {
  switch(src) {
    case NEWS_SOURCE_ANSA: return (cat < ANSA_NUM_CATS) ? ansaCatUrls[cat] : "";
    case NEWS_SOURCE_BBC:  return (cat < BBC_NUM_CATS)  ? bbcCatUrls[cat]  : "";
    case NEWS_SOURCE_REPUBBLICA:  return (cat < REPUBBLICA_NUM_CATS)  ? repubblicaCatUrls[cat]  : "";
    default: return "";
  }
}

// ===== Calcolo layout tab dinamico =====

void newsComputeTabLayout() {
  int total = getSourceCatCount(newsSourceIndex);
  if (total <= 5) {
    newsRow1Count = total;
    newsRow2Count = 0;
  } else {
    newsRow1Count = (total + 1) / 2;  // 8→4, 7→4
    newsRow2Count = total - newsRow1Count;  // 8→4, 7→3
  }
  if (newsRow1Count > 0)
    newsRow1W = (480 - 2 * TAB_MARGIN - (newsRow1Count - 1) * TAB_GAP) / newsRow1Count;
  if (newsRow2Count > 0) {
    newsRow2W = (480 - 2 * TAB_MARGIN - (newsRow2Count - 1) * TAB_GAP) / newsRow2Count;
    newsRow2X = (480 - newsRow2Count * newsRow2W - (newsRow2Count - 1) * TAB_GAP) / 2;
  }
}

int newsGetArticlesStartY() {
  if (newsRow2Count > 0) {
    return TAB_ROW1_Y + TAB_H + TAB_GAP + TAB_H + 10;
  }
  return TAB_ROW1_Y + TAB_H + 10;
}

// ===== Parser RSS XML =====

static String extractXmlTag(const String& xml, const char* tag, int startPos = 0) {
  String openTag = "<" + String(tag);
  String closeTag = "</" + String(tag) + ">";

  int start = xml.indexOf(openTag, startPos);
  if (start < 0) return "";

  int tagClose = xml.indexOf('>', start + openTag.length());
  if (tagClose < 0) return "";

  int contentStart = tagClose + 1;
  int end = xml.indexOf(closeTag, contentStart);
  if (end < 0) return "";

  String content = xml.substring(contentStart, end);
  content.trim();

  // Strip CDATA wrapper
  if (content.startsWith("<![CDATA[")) {
    content = content.substring(9);
    int cdataEnd = content.lastIndexOf("]]>");
    if (cdataEnd >= 0) content = content.substring(0, cdataEnd);
  }

  return content;
}

static String stripHtmlTags(const String& html) {
  String result;
  result.reserve(html.length());
  bool inTag = false;
  for (unsigned int i = 0; i < html.length(); i++) {
    char c = html.charAt(i);
    if (c == '<') { inTag = true; continue; }
    if (c == '>') { inTag = false; continue; }
    if (!inTag) result += c;
  }
  // Decodifica entita' HTML comuni
  result.replace("&amp;", "&");
  result.replace("&lt;", "<");
  result.replace("&gt;", ">");
  result.replace("&quot;", "\"");
  result.replace("&#39;", "'");
  result.replace("&apos;", "'");
  result.replace("&nbsp;", " ");
  result.replace("&#8217;", "'");
  result.replace("&#8220;", "\"");
  result.replace("&#8221;", "\"");
  result.trim();
  return result;
}

static int parseRssItems(const String& xml, NewsArticle* articles, int maxArticles) {
  int count = 0;
  int pos = 0;
  const char* srcName = newsSourceNames[newsSourceIndex];

  while (count < maxArticles) {
    int itemStart = xml.indexOf("<item>", pos);
    if (itemStart < 0) itemStart = xml.indexOf("<item ", pos);
    if (itemStart < 0) break;

    int itemEnd = xml.indexOf("</item>", itemStart);
    if (itemEnd < 0) break;

    String item = xml.substring(itemStart, itemEnd + 7);

    String title = stripHtmlTags(extractXmlTag(item, "title"));
    String desc = stripHtmlTags(extractXmlTag(item, "description"));
    String pubDate = extractXmlTag(item, "pubDate");
    String link = extractXmlTag(item, "link");

    // Salta titoli vuoti o rimossi
    if (title.length() == 0 || title == "[Removed]") {
      pos = itemEnd + 7;
      continue;
    }

    // Tronca testi lunghi
    if (title.length() > 100) title = title.substring(0, 97) + "...";
    if (desc.length() > 120) desc = desc.substring(0, 117) + "...";

    articles[count].title = title;
    articles[count].source = String(srcName);
    articles[count].description = desc;
    articles[count].publishedAt = pubDate;
    articles[count].link = link;
    count++;

    pos = itemEnd + 7;
  }

  return count;
}

// ===== Fetch notizie via RSS =====

void fetchNews() {
  if (WiFi.status() != WL_CONNECTED) {
    newsError = "WiFi non connesso";
    NEWS_LOGLN("[NEWS] WiFi non connesso");
    return;
  }

  const char* url = getSourceCatUrl(newsSourceIndex, newsCategoryIndex);
  if (!url || strlen(url) == 0) {
    newsError = "Feed non disponibile";
    NEWS_LOGLN("[NEWS] URL feed vuoto");
    return;
  }

  NEWS_LOG("[NEWS] Fetch RSS: %s [%s] url=%s\n",
                newsSourceNames[newsSourceIndex],
                getSourceCatName(newsSourceIndex, newsCategoryIndex), url);

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15);

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(15000);
  http.setUserAgent("OraQuadra/1.5");

  if (!http.begin(client, url)) {
    newsError = "Connessione fallita";
    NEWS_LOGLN("[NEWS] http.begin() fallito");
    return;
  }

  int httpCode = http.GET();
  NEWS_LOG("[NEWS] HTTP code: %d\n", httpCode);

  if (httpCode != 200) {
    newsError = "HTTP " + String(httpCode);
    NEWS_LOG("[NEWS] Errore HTTP: %d\n", httpCode);
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  NEWS_LOG("[NEWS] RSS ricevuto: %d bytes\n", payload.length());

  newsArticleCount = parseRssItems(payload, newsArticles, NEWS_MAX_ARTICLES);
  newsCategory = String(getSourceCatName(newsSourceIndex, newsCategoryIndex));

  if (newsArticleCount > 0) {
    newsError = "";
    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", currentHour, currentMinute);
    newsLastUpdate = String(timeBuf);
  } else {
    newsError = "Nessun articolo nel feed";
  }

  NEWS_LOG("[NEWS] %d articoli parsati da %s\n", newsArticleCount,
                newsSourceNames[newsSourceIndex]);
}

// ===== Disegna icona giornale =====

static void drawNewsIcon(int cx, int cy) {
  gfx->fillRoundRect(cx - 25, cy - 18, 50, 36, 4, NW_ACCENT);
  gfx->fillRect(cx - 18, cy - 11, 26, 3, NW_BG_COLOR);
  gfx->fillRect(cx - 18, cy - 5, 36, 2, NW_BG_COLOR);
  gfx->fillRect(cx - 18, cy + 0, 36, 2, NW_BG_COLOR);
  gfx->fillRect(cx - 18, cy + 5, 30, 2, NW_BG_COLOR);
  gfx->fillRect(cx - 18, cy + 10, 20, 2, NW_BG_COLOR);
}

// ===== Disegna pill fonte (tocco per cambiare) =====

static void drawSourcePill() {
  int pw = getSourcePillW();
  int px = getSourcePillX();
  gfx->fillRoundRect(px, SOURCE_PILL_Y, pw, SOURCE_PILL_H, 8, NW_SOURCE_BG);
  gfx->drawRoundRect(px, SOURCE_PILL_Y, pw, SOURCE_PILL_H, 8, NW_ACCENT);
  gfx->setFont(u8g2_font_helvB10_tr);
  gfx->setTextColor(NW_WHITE);
  const char* name = newsSourceNames[newsSourceIndex];
  int tw = strlen(name) * 8;
  gfx->setCursor(px + (pw - tw) / 2, SOURCE_PILL_Y + 16);
  gfx->print(name);
}

// ===== Disegna tabs categorie (layout dinamico per fonte) =====

static void drawCategoryTabs() {
  newsComputeTabLayout();
  int total = getSourceCatCount(newsSourceIndex);

  // Riga 1
  for (int i = 0; i < newsRow1Count && i < total; i++) {
    int x = TAB_MARGIN + i * (newsRow1W + TAB_GAP);
    uint16_t bg = (i == newsCategoryIndex) ? NW_TAB_ACTIVE : NW_TAB_INACTIVE;
    uint16_t fg = (i == newsCategoryIndex) ? NW_WHITE : NW_GRAY;
    gfx->fillRoundRect(x, TAB_ROW1_Y, newsRow1W, TAB_H, 10, bg);
    gfx->setFont(u8g2_font_helvR10_tr);
    gfx->setTextColor(fg);
    const char* catName = getSourceCatName(newsSourceIndex, i);
    int tw = strlen(catName) * 7;
    gfx->setCursor(x + (newsRow1W - tw) / 2, TAB_ROW1_Y + 19);
    gfx->print(catName);
  }

  // Riga 2 (se necessaria)
  if (newsRow2Count > 0) {
    int row2Y = TAB_ROW1_Y + TAB_H + TAB_GAP;
    for (int i = newsRow1Count; i < total; i++) {
      int j = i - newsRow1Count;
      int x = newsRow2X + j * (newsRow2W + TAB_GAP);
      uint16_t bg = (i == newsCategoryIndex) ? NW_TAB_ACTIVE : NW_TAB_INACTIVE;
      uint16_t fg = (i == newsCategoryIndex) ? NW_WHITE : NW_GRAY;
      gfx->fillRoundRect(x, row2Y, newsRow2W, TAB_H, 10, bg);
      gfx->setFont(u8g2_font_helvR10_tr);
      gfx->setTextColor(fg);
      const char* catName = getSourceCatName(newsSourceIndex, i);
      int tw = strlen(catName) * 7;
      gfx->setCursor(x + (newsRow2W - tw) / 2, row2Y + 19);
      gfx->print(catName);
    }
  }
}

// ===== Disegna lista articoli =====

static void drawArticlesList() {
  int startY = newsGetArticlesStartY();
  int maxArticlesVisible = 4;

  // Pulisci area articoli (necessario per scroll)
  gfx->fillRect(0, startY, 480, 440 - startY, NW_BG_COLOR);

  if (newsArticleCount == 0) {
    gfx->setFont(u8g2_font_helvR10_tr);
    gfx->setTextColor(NW_GRAY);
    if (newsError.length() > 0) {
      String errDisp = newsError;
      if (errDisp.length() > 45) errDisp = errDisp.substring(0, 42) + "...";
      int ew = errDisp.length() * 7;
      gfx->setCursor((480 - ew) / 2, startY + 40);
      gfx->print(errDisp);
    } else {
      gfx->setCursor(140, startY + 40);
      gfx->print("Caricamento notizie...");
    }
    return;
  }

  // Clamp scroll offset
  if (newsScrollOffset >= newsArticleCount) newsScrollOffset = 0;
  if (newsScrollOffset < 0) newsScrollOffset = 0;

  int availH = 440 - startY;
  int articleH = (availH - (maxArticlesVisible - 1) * 5) / maxArticlesVisible;
  if (articleH > 70) articleH = 70;
  int gap = 5;

  int endIndex = newsScrollOffset + maxArticlesVisible;
  if (endIndex > newsArticleCount) endIndex = newsArticleCount;

  for (int idx = newsScrollOffset; idx < endIndex; idx++) {
    int i = idx - newsScrollOffset;  // posizione visiva 0-3
    int y = startY + i * (articleH + gap);

    // Sfondo articolo
    gfx->fillRoundRect(10, y, 460, articleH, 6, NW_CARD_BG);

    // Fonte (piccolo, azzurro)
    gfx->setFont(u8g2_font_helvR08_tr);
    gfx->setTextColor(NW_ACCENT);
    String src = newsArticles[idx].source;
    if (src.length() > 35) src = src.substring(0, 32) + "...";
    gfx->setCursor(20, y + 14);
    gfx->print(src);

    // Titolo (bold, bianco) - max 2 righe
    gfx->setFont(u8g2_font_helvB10_tr);
    gfx->setTextColor(NW_WHITE);
    String title = newsArticles[idx].title;
    int maxChars = 48;
    if ((int)title.length() <= maxChars) {
      gfx->setCursor(20, y + 30);
      gfx->print(title);
    } else {
      int cut = maxChars;
      while (cut > 0 && title.charAt(cut) != ' ') cut--;
      if (cut == 0) cut = maxChars;
      gfx->setCursor(20, y + 30);
      gfx->print(title.substring(0, cut));
      String line2 = title.substring(cut + 1);
      if ((int)line2.length() > maxChars) line2 = line2.substring(0, maxChars - 3) + "...";
      gfx->setFont(u8g2_font_helvR08_tr);
      gfx->setCursor(20, y + 44);
      gfx->print(line2);
    }

    // Descrizione breve (grigio, piccolo)
    if (articleH >= 65) {
      gfx->setFont(u8g2_font_helvR08_tr);
      gfx->setTextColor(NW_GRAY);
      String desc = newsArticles[idx].description;
      if (desc.length() > 0 && desc != "null") {
        if ((int)desc.length() > 60) desc = desc.substring(0, 57) + "...";
        gfx->setCursor(20, y + 60);
        gfx->print(desc);
      }
    }

    // Separatore
    if (idx < endIndex - 1) {
      gfx->drawFastHLine(30, y + articleH + 2, 420, NW_SEPARATOR);
    }
  }

  // Scrollbar sottile a destra (stile smartphone)
  if (newsArticleCount > maxArticlesVisible) {
    int barX = 474;
    int barW = 3;
    int trackH = 440 - startY - 10;
    int thumbH = trackH * maxArticlesVisible / newsArticleCount;
    if (thumbH < 20) thumbH = 20;
    int maxOffset = newsArticleCount - maxArticlesVisible;
    int thumbY = startY + 5;
    if (maxOffset > 0) {
      thumbY += (trackH - thumbH) * newsScrollOffset / maxOffset;
    }
    gfx->fillRect(barX, startY + 5, barW, trackH, NW_DARK_GRAY);
    gfx->fillRoundRect(barX, thumbY, barW, thumbH, 1, NW_ACCENT);
  }
}

// ===== Status bar in basso =====

static void drawNewsStatusBar() {
  gfx->fillRect(0, 440, 480, 40, NW_CARD_BG);
  gfx->setFont(u8g2_font_helvR10_tr);
  gfx->setTextColor(NW_GRAY);

  // Orologio a sinistra
  char timeStr[6];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", currentHour, currentMinute);
  gfx->setCursor(15, 465);
  gfx->print(timeStr);

  // Pulsante MODE >> al centro della status bar
  gfx->setTextColor(NW_ACCENT);
  gfx->setCursor(195, 465);
  gfx->print("MODE >>");

  // Indicatore posizione scroll (es. "3-6/12")
  if (newsArticleCount > 4) {
    int first = newsScrollOffset + 1;
    int last = newsScrollOffset + 4;
    if (last > newsArticleCount) last = newsArticleCount;
    char pageBuf[12];
    snprintf(pageBuf, sizeof(pageBuf), "%d-%d/%d", first, last, newsArticleCount);
    gfx->setCursor(285, 465);
    gfx->print(pageBuf);
  }

  // Fonte + ultimo aggiornamento a destra
  gfx->setTextColor(NW_GRAY);
  String info = String(newsSourceFlags[newsSourceIndex]) + " | " + newsLastUpdate;
  int infoW = info.length() * 7;
  gfx->setCursor(480 - infoW - 15, 465);
  gfx->print(info);
}

// ===== Inizializzazione schermo completo =====

void initNewsStation() {
  newsComputeTabLayout();
  gfx->fillScreen(NW_BG_COLOR);

  // Header con icona
  drawNewsIcon(240, 35);

  // Titolo
  gfx->setFont(u8g2_font_helvB14_tr);
  gfx->setTextColor(NW_WHITE);
  String title = "NEWS " + String(newsSourceNames[newsSourceIndex]);
  int tw = title.length() * 10;
  gfx->setCursor((480 - tw) / 2, 70);
  gfx->print(title);

  // Pill fonte in alto a destra
  drawSourcePill();

  // Tabs categorie
  drawCategoryTabs();

  // Lista articoli
  drawArticlesList();

  // Status bar
  drawNewsStatusBar();

  newsInitialized = true;
  prevNewsSourceIndex = newsSourceIndex;
  prevNewsCategoryIndex = newsCategoryIndex;
  prevNewsArticleCount = newsArticleCount;
  if (newsArticleCount > 0) prevNewsFirstTitle = newsArticles[0].title;
  prevNewsMinute = currentMinute;

  NEWS_LOGLN("[NEWS] Display inizializzato");
}

// ===== Aggiornamento smart =====

void updateNewsStation() {
  unsigned long now = millis();

  // Fetch periodico
  if (newsFirstFetch || (now - lastNewsFetch >= NEWS_FETCH_INTERVAL)) {
    fetchNews();
    lastNewsFetch = now;
    newsFirstFetch = false;
  }

  // Verifica se serve ridisegnare
  bool needRedraw = false;
  if (newsSourceIndex != prevNewsSourceIndex) needRedraw = true;
  if (newsCategoryIndex != prevNewsCategoryIndex) needRedraw = true;
  if (newsArticleCount != prevNewsArticleCount) needRedraw = true;
  if (newsArticleCount > 0 && newsArticles[0].title != prevNewsFirstTitle) needRedraw = true;

  if (needRedraw) {
    newsScrollOffset = 0;
    newsInitialized = false;
    initNewsStation();
  }

  // Aggiorna status bar ogni minuto
  if (currentMinute != prevNewsMinute) {
    drawNewsStatusBar();
    prevNewsMinute = currentMinute;
  }
}

// ===== Cambio categoria =====

void newsSetCategory(int index) {
  int maxCats = getSourceCatCount(newsSourceIndex);
  if (index < 0 || index >= maxCats) return;
  if (index == newsCategoryIndex) return;
  newsCategoryIndex = index;
  newsScrollOffset = 0;
  newsFirstFetch = true;
  lastNewsFetch = 0;
  newsInitialized = false;
  NEWS_LOG("[NEWS] Categoria: %s\n", getSourceCatName(newsSourceIndex, newsCategoryIndex));
}

void newsNextCategory() {
  int maxCats = getSourceCatCount(newsSourceIndex);
  newsSetCategory((newsCategoryIndex + 1) % maxCats);
}

void newsPrevCategory() {
  int maxCats = getSourceCatCount(newsSourceIndex);
  newsSetCategory((newsCategoryIndex - 1 + maxCats) % maxCats);
}

// ===== Cambio fonte =====

void newsNextSource() {
  newsSourceIndex = (newsSourceIndex + 1) % NEWS_NUM_SOURCES;
  // Reset categoria e scroll se fuori range per nuova fonte
  int maxCats = getSourceCatCount(newsSourceIndex);
  if (newsCategoryIndex >= maxCats) newsCategoryIndex = 0;
  newsScrollOffset = 0;
  newsFirstFetch = true;
  lastNewsFetch = 0;
  newsInitialized = false;
  NEWS_LOG("[NEWS] Fonte: %s\n", newsSourceNames[newsSourceIndex]);
}

// ===== Scroll articoli (swipe naturale, 1 alla volta) =====

void newsScrollDown() {
  if (newsScrollOffset + 4 < newsArticleCount) {
    newsScrollOffset++;
    drawArticlesList();
    drawNewsStatusBar();
  }
}

void newsScrollUp() {
  if (newsScrollOffset > 0) {
    newsScrollOffset--;
    drawArticlesList();
    drawNewsStatusBar();
  }
}

// ===== Tap su articolo: ritorna indice articolo toccato (-1 se nessuno) =====

int newsGetTappedArticle(int tapY) {
  int startY = newsGetArticlesStartY();
  int maxVis = 4;
  int availH = 440 - startY;
  int articleH = (availH - (maxVis - 1) * 5) / maxVis;
  if (articleH > 70) articleH = 70;
  int gap = 5;

  for (int i = 0; i < maxVis; i++) {
    int idx = newsScrollOffset + i;
    if (idx >= newsArticleCount) break;
    int y = startY + i * (articleH + gap);
    if (tapY >= y && tapY <= y + articleH) {
      return idx;
    }
  }
  return -1;
}

// ===== Apri articolo: salva URL e feedback visivo =====

void newsOpenArticle(int index) {
  if (index < 0 || index >= newsArticleCount) return;
  if (newsArticles[index].link.length() == 0) return;

  newsOpenUrl = newsArticles[index].link;

  // Feedback visivo: bordo azzurro sull'articolo toccato
  int startY = newsGetArticlesStartY();
  int maxVis = 4;
  int availH = 440 - startY;
  int articleH = (availH - (maxVis - 1) * 5) / maxVis;
  if (articleH > 70) articleH = 70;
  int gap = 5;
  int i = index - newsScrollOffset;
  int y = startY + i * (articleH + gap);

  gfx->drawRoundRect(10, y, 460, articleH, 6, NW_ACCENT);
  gfx->drawRoundRect(11, y + 1, 458, articleH - 2, 5, NW_ACCENT);

  NEWS_LOG("[NEWS] Articolo selezionato: %s\n", newsOpenUrl.c_str());
}

// ===== Hit-test: quale tab toccata? Ritorna -1 se nessuna =====

int newsGetTappedTab(int tx, int ty) {
  const int tolerance = 5;
  int total = getSourceCatCount(newsSourceIndex);

  // Riga 1
  if (ty >= (TAB_ROW1_Y - tolerance) && ty <= (TAB_ROW1_Y + TAB_H + tolerance)) {
    for (int i = 0; i < newsRow1Count && i < total; i++) {
      int x = TAB_MARGIN + i * (newsRow1W + TAB_GAP);
      if (tx >= (x - tolerance) && tx <= (x + newsRow1W + tolerance)) return i;
    }
  }

  // Riga 2
  if (newsRow2Count > 0) {
    int row2Y = TAB_ROW1_Y + TAB_H + TAB_GAP;
    if (ty >= (row2Y - tolerance) && ty <= (row2Y + TAB_H + tolerance)) {
      for (int i = newsRow1Count; i < total; i++) {
        int j = i - newsRow1Count;
        int x = newsRow2X + j * (newsRow2W + TAB_GAP);
        if (tx >= (x - tolerance) && tx <= (x + newsRow2W + tolerance)) return i;
      }
    }
  }

  return -1;
}

// ===== Hit-test: pill fonte toccata? =====

bool newsIsSourcePillTapped(int tx, int ty) {
  int px = getSourcePillX();
  int pw = getSourcePillW();
  return (tx >= px - 10 && tx <= px + pw + 10 &&
          ty >= SOURCE_PILL_Y - 10 && ty <= SOURCE_PILL_Y + SOURCE_PILL_H + 10);
}

#endif // EFFECT_NEWS
