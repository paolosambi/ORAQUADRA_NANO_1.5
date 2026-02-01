// ================== WEB SERVER PER CONFIGURAZIONE ESP32-CAM STREAM ==================
#ifdef EFFECT_ESP32CAM

extern String esp32camStreamUrl;
extern void setEsp32camStreamUrl(const String& url);
extern String getEsp32camStreamUrl();

// Pagina HTML per configurazione ESP32-CAM
const char ESP32CAM_CONFIG_HTML[] PROGMEM =
"<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>ESP32-CAM Config</title><style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:Arial,sans-serif;background:#1a1a2e;min-height:100vh;padding:20px;color:#eee}"
".c{max-width:600px;margin:0 auto;background:rgba(255,255,255,.1);border-radius:20px;overflow:hidden}"
".h{background:linear-gradient(135deg,#00d4ff,#0099cc);padding:25px;text-align:center}"
".h h1{font-size:1.5em;margin-bottom:5px}.h p{opacity:.8;font-size:.9em}"
".ct{padding:25px}"
".s{background:rgba(0,0,0,.3);border-radius:15px;padding:20px;margin-bottom:20px}"
".s h3{margin-bottom:15px;color:#00d4ff}"
".fg{margin-bottom:15px}.fg label{display:block;margin-bottom:8px;opacity:.8;font-size:.9em}"
".fg input{width:100%;padding:12px;border:2px solid #555;border-radius:8px;background:#2a2a4a;color:#fff;font-size:1em}"
".fg input:focus{outline:none;border-color:#00d4ff}"
".ex{font-size:.8em;color:#888;margin-top:5px;font-family:monospace}"
".btn{width:100%;padding:15px;border:none;border-radius:10px;font-weight:bold;cursor:pointer;font-size:1em;margin-top:10px}"
".b1{background:linear-gradient(135deg,#00d4ff,#0099cc);color:#fff}"
".b2{background:linear-gradient(135deg,#4CAF50,#45a049);color:#fff}"
".b3{background:linear-gradient(135deg,#ff9800,#f57c00);color:#fff}"
".b4{background:linear-gradient(135deg,#9c27b0,#7b1fa2);color:#fff}"
".sr{display:flex;align-items:center;gap:10px;margin-bottom:15px}"
".dot{width:12px;height:12px;border-radius:50%;background:#f44336}"
".dot.on{background:#4CAF50;box-shadow:0 0 10px #4CAF50}"
".cur{font-size:.9em;padding:10px;background:rgba(0,212,255,.1);border-radius:8px;margin-top:10px;word-break:break-all}"
".cur b{color:#00d4ff}"
".msg{padding:12px;border-radius:8px;margin-top:15px;display:none}"
".msg.ok{background:rgba(76,175,80,.2);border:1px solid #4CAF50;color:#4CAF50;display:block}"
".msg.err{background:rgba(244,67,54,.2);border:1px solid #f44336;color:#f44336;display:block}"
".msg.inf{background:rgba(33,150,243,.2);border:1px solid #2196F3;color:#2196F3;display:block}"
".hint{background:rgba(0,200,255,.1);border-left:3px solid #00d4ff;padding:15px;margin-top:15px;border-radius:0 8px 8px 0}"
".hint h4{color:#00d4ff;margin-bottom:8px}.hint p{font-size:.85em;opacity:.8;line-height:1.6}"
".hint code{background:rgba(0,0,0,.3);padding:2px 6px;border-radius:4px;font-family:monospace}"
".nav{display:flex;justify-content:space-between;padding:10px 15px}"
".nav a{color:#94a3b8;text-decoration:none;font-size:.9em;padding:5px 10px}"
".nav a:hover{color:#fff}"
".cl{margin-top:15px;padding:10px;background:rgba(156,39,176,.2);border:1px solid #9c27b0;border-radius:8px}"
".cl a{color:#00d4ff;text-decoration:none}"
"</style></head><body><div class=\"c\">"
"<div class=\"nav\"><a href=\"/\">&larr; Home</a><a href=\"/settings\">Settings &rarr;</a></div>"
"<div class=\"h\"><h1>ESP32-CAM Stream</h1><p>Configurazione camera Freenove</p></div>"
"<div class=\"ct\">"
"<div class=\"s\"><h3>Stato Connessione</h3>"
"<div class=\"sr\"><div class=\"dot\" id=\"dot\"></div><span id=\"st\">Non connesso</span></div>"
"<div class=\"cur\"><b>URL attuale:</b> <span id=\"cu\">Nessuno configurato</span></div></div>"
"<div class=\"s\"><h3>Configurazione URL Stream</h3>"
"<div class=\"fg\"><label>Inserisci l'URL dello stream (porta 81)</label>"
"<input type=\"text\" id=\"url\"><div class=\"ex\">Esempio: http://192.168.1.100:81/stream</div></div>"
"<button class=\"btn b1\" onclick=\"sv()\">Salva URL</button>"
"<button class=\"btn b2\" onclick=\"ts()\">Test Connessione</button>"
"<div class=\"msg\" id=\"msg\"></div></div>"
"<div class=\"s\"><h3>Impostazioni Camera</h3>"
"<p style=\"font-size:.85em;opacity:.8;margin-bottom:15px\">Apri l'interfaccia web della ESP32-CAM per regolare risoluzione e altri parametri.</p>"
"<button class=\"btn b4\" onclick=\"oc()\">Apri Interfaccia Camera</button>"
"<div class=\"cl\" id=\"cl\" style=\"display:none\"><a href=\"#\" id=\"clu\" target=\"_blank\">Apri in nuova scheda</a></div></div>"
"<div class=\"s\"><h3>Avvia Streaming</h3>"
"<button class=\"btn b3\" onclick=\"go()\">Avvia ESP32-CAM sul Display</button></div>"
"<div class=\"hint\"><h4>Come configurare</h4><p>"
"1. Carica <code>CameraWebServer</code> sulla ESP32-CAM<br>"
"2. Apri Serial Monitor per vedere l'IP<br>"
"3. URL stream: <code>http://IP:81/stream</code><br>"
"4. Interfaccia: <code>http://IP</code> (porta 80)</p></div>"
"</div></div>"
"<script>"
"var I=document.getElementById.bind(document),url,dot,st,cu,msg,cl,clu;"
"function init(){url=I('url');dot=I('dot');st=I('st');cu=I('cu');msg=I('msg');cl=I('cl');clu=I('clu');ld();}"
"function sm(t,c){msg.textContent=t;msg.className='msg '+c;}"
"function ip(u){var m=u.match(/:[\\/][\\/]([^:\\/]+)/);return m?m[1]:null;}"
"function uc(u){var i=ip(u);if(i){clu.href='http://'+i;clu.textContent='http://'+i;cl.style.display='block';}}"
"function ld(){fetch('/espcam/config').then(function(r){return r.json();}).then(function(d){"
"var u=(d.url||'').trim();if(u.length>10){url.value=u;cu.textContent=u;uc(u);}else{url.value='';cu.textContent='Nessuno configurato';}}"
").catch(function(){cu.textContent='Errore caricamento';});}"
"function sv(){var u=url.value.trim();if(!u||u.length<15||u.indexOf('http')!=0){sm('URL non valido','err');return;}"
"sm('Salvataggio...','inf');fetch('/espcam/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({url:u})})"
".then(function(r){return r.json();}).then(function(d){if(d.success){sm('URL salvato!','ok');cu.textContent=u;uc(u);}"
"else{sm('Errore: '+(d.message||'fallito'),'err');}}).catch(function(){sm('Errore connessione','err');});}"
"function ts(){var u=url.value.trim();if(!u||u.length<15){sm('Inserisci URL valido','err');return;}"
"sm('Test...','inf');st.textContent='Test...';dot.className='dot';"
"fetch('/espcam/test',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({url:u})})"
".then(function(r){return r.json();}).then(function(d){if(d.success){dot.className='dot on';st.textContent='Raggiungibile!';sm('OK: '+d.message,'ok');}"
"else{dot.className='dot';st.textContent='Non raggiungibile';sm('Errore: '+d.message,'err');}}).catch(function(){dot.className='dot';st.textContent='Errore';sm('Errore connessione','err');});}"
"function oc(){var u=url.value.trim();if(!u){alert('Inserisci prima URL');return;}var i=ip(u);if(i){window.open('http://'+i,'_blank');}else{alert('URL non valido');}}"
"function go(){fetch('/espcam/start',{method:'POST'}).then(function(r){return r.json();}).then(function(d){"
"if(d.success){alert('ESP32-CAM avviata!');}else{alert('Errore: '+(d.message||''));}}).catch(function(){alert('Errore connessione');});}"
"document.addEventListener('DOMContentLoaded',init);"
"</script></body></html>";

void setup_esp32cam_webserver(AsyncWebServer* server) {
  Serial.println("[WEBSERVER] Configurazione ESP32-CAM Stream...");

  // Pagina principale configurazione
  server->on("/espcam", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", ESP32CAM_CONFIG_HTML);
  });

  // GET configurazione attuale
  server->on("/espcam/config", HTTP_GET, [](AsyncWebServerRequest *request){
    esp32camStreamUrl.trim();
    String json = "{\"url\":\"" + esp32camStreamUrl + "\"}";
    Serial.printf("[ESP32-CAM] GET config: %s\n", json.c_str());
    request->send(200, "application/json", json);
  });

  // POST salva configurazione
  static String configBody;
  server->on("/espcam/config", HTTP_POST,
    [](AsyncWebServerRequest *request){
      Serial.printf("[ESP32-CAM] POST body: %s\n", configBody.c_str());
      int start = configBody.indexOf("\"url\":\"");
      if (start >= 0) {
        start += 7;
        int end = configBody.indexOf("\"", start);
        if (end > start) {
          String newUrl = configBody.substring(start, end);
          newUrl.trim();
          if (newUrl.length() > 10 && newUrl.startsWith("http")) {
            setEsp32camStreamUrl(newUrl);
            Serial.printf("[ESP32-CAM] URL salvato: '%s'\n", newUrl.c_str());
            configBody = "";
            request->send(200, "application/json", "{\"success\":true}");
            return;
          }
        }
      }
      configBody = "";
      request->send(400, "application/json", "{\"success\":false,\"message\":\"URL non valido\"}");
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      if (index == 0) configBody = "";
      for (size_t i = 0; i < len; i++) configBody += (char)data[i];
    }
  );

  // POST test connessione
  static String testBody;
  server->on("/espcam/test", HTTP_POST,
    [](AsyncWebServerRequest *request){
      String testUrl = esp32camStreamUrl;
      int start = testBody.indexOf("\"url\":\"");
      if (start >= 0) {
        start += 7;
        int end = testBody.indexOf("\"", start);
        if (end > start) testUrl = testBody.substring(start, end);
      }
      testBody = "";
      testUrl.trim();

      if (testUrl.length() < 10 || !testUrl.startsWith("http")) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"URL non valido\"}");
        return;
      }

      Serial.printf("[ESP32-CAM] Test: '%s'\n", testUrl.c_str());

      WiFiClient testClient;
      HTTPClient testHttp;
      testHttp.begin(testClient, testUrl);
      testHttp.setTimeout(8000);

      int httpCode = testHttp.GET();
      testHttp.end();

      if (httpCode == HTTP_CODE_OK || httpCode == 206) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Stream attivo\"}");
      } else if (httpCode > 0) {
        String msg = "{\"success\":false,\"message\":\"HTTP " + String(httpCode) + "\"}";
        request->send(200, "application/json", msg);
      } else {
        request->send(200, "application/json", "{\"success\":false,\"message\":\"Non raggiungibile\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      if (index == 0) testBody = "";
      for (size_t i = 0; i < len; i++) testBody += (char)data[i];
    }
  );

  // POST avvia modalita
  server->on("/espcam/start", HTTP_POST, [](AsyncWebServerRequest *request){
    extern DisplayMode currentMode;
    extern DisplayMode userMode;
    extern void forceDisplayUpdate();

    currentMode = MODE_ESP32CAM;
    userMode = MODE_ESP32CAM;
    EEPROM.write(EEPROM_MODE_ADDR, currentMode);
    EEPROM.commit();
    esp32camInitialized = false;

    Serial.println("[ESP32-CAM] Avviata da web");
    request->send(200, "application/json", "{\"success\":true}");
    forceDisplayUpdate();
  });

  Serial.println("[WEBSERVER] ESP32-CAM su /espcam");
}

#endif // EFFECT_ESP32CAM
