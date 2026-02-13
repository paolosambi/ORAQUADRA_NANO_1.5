// ================== WEB SERVER NEWS FEED ==================
// Endpoint per pagina web News e API status JSON
// GET /news         -> Pagina HTML
// GET /news/status  -> JSON con articles[], source, categories, lastUpdate, error
// Parametri: ?src=0&cat=0 (indice fonte e categoria)

#include "news_web_html.h"

#ifdef EFFECT_NEWS

extern NewsArticle newsArticles[];
extern int    newsArticleCount;
extern String newsCategory;
extern String newsLastUpdate;
extern String newsError;
extern int    newsCategoryIndex;
extern int    newsSourceIndex;
extern const char* newsSourceNames[];
extern const char* newsSourceFlags[];

// Funzioni da 41_NEWS.ino
extern int getSourceCatCount(int src);
extern const char* getSourceCatName(int src, int cat);
extern void fetchNews();

void setup_news_webserver(AsyncWebServer* server) {
  // Endpoint status JSON (registrato PRIMA di /news per evitare conflitti)
  server->on("/news/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Parametro fonte opzionale
    int reqSrc = newsSourceIndex;
    int reqCat = newsCategoryIndex;

    if (request->hasParam("src")) {
      reqSrc = request->getParam("src")->value().toInt();
      if (reqSrc < 0 || reqSrc >= NEWS_NUM_SOURCES) reqSrc = 0;
    }
    if (request->hasParam("cat")) {
      reqCat = request->getParam("cat")->value().toInt();
      if (reqCat < 0 || reqCat >= getSourceCatCount(reqSrc)) reqCat = 0;
    }

    // Se fonte o categoria cambiate, aggiorna
    bool needFetch = false;
    if (reqSrc != newsSourceIndex) {
      newsSourceIndex = reqSrc;
      needFetch = true;
    }
    if (reqCat != newsCategoryIndex) {
      newsCategoryIndex = reqCat;
      needFetch = true;
    }
    if (needFetch || newsArticleCount == 0) {
      fetchNews();
    }

    // Costruisci JSON
    int numCats = getSourceCatCount(newsSourceIndex);

    String json = "{";
    json += "\"source\":" + String(newsSourceIndex) + ",";
    json += "\"sourceName\":\"" + String(newsSourceNames[newsSourceIndex]) + "\",";
    json += "\"sourceFlag\":\"" + String(newsSourceFlags[newsSourceIndex]) + "\",";
    json += "\"catIndex\":" + String(newsCategoryIndex) + ",";
    json += "\"catName\":\"" + String(getSourceCatName(newsSourceIndex, newsCategoryIndex)) + "\",";
    json += "\"numCats\":" + String(numCats) + ",";

    // Lista categorie per fonte corrente
    json += "\"cats\":[";
    for (int i = 0; i < numCats; i++) {
      if (i > 0) json += ",";
      json += "\"" + String(getSourceCatName(newsSourceIndex, i)) + "\"";
    }
    json += "],";

    // Lista fonti disponibili
    json += "\"sources\":[";
    for (int i = 0; i < NEWS_NUM_SOURCES; i++) {
      if (i > 0) json += ",";
      json += "{\"name\":\"" + String(newsSourceNames[i]) + "\",";
      json += "\"flag\":\"" + String(newsSourceFlags[i]) + "\"}";
    }
    json += "],";

    json += "\"articleCount\":" + String(newsArticleCount) + ",";
    json += "\"lastUpdate\":\"" + newsLastUpdate + "\",";
    json += "\"error\":\"" + newsError + "\",";
    json += "\"articles\":[";

    for (int i = 0; i < newsArticleCount; i++) {
      if (i > 0) json += ",";
      json += "{";
      // Escape doppi apici nei campi stringa
      String title = newsArticles[i].title;
      title.replace("\"", "\\\"");
      String source = newsArticles[i].source;
      source.replace("\"", "\\\"");
      String desc = newsArticles[i].description;
      desc.replace("\"", "\\\"");
      String link = newsArticles[i].link;
      link.replace("\"", "\\\"");

      json += "\"title\":\"" + title + "\",";
      json += "\"source\":\"" + source + "\",";
      json += "\"description\":\"" + desc + "\",";
      json += "\"link\":\"" + link + "\",";
      json += "\"publishedAt\":\"" + newsArticles[i].publishedAt + "\"";
      json += "}";
    }

    json += "]}";
    request->send(200, "application/json", json);
  });

  // Pagina HTML (registrato DOPO /news/status)
  server->on("/news", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", NEWS_HTML);
  });

  Serial.println("[NEWS] Webserver endpoints registrati (/news, /news/status)");
}

#endif // EFFECT_NEWS
