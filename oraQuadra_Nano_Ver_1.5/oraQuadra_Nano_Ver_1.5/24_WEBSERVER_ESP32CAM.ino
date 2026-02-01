// ================== WEB SERVER PER CONFIGURAZIONE ESP32-CAM STREAM ==================
#ifdef EFFECT_ESP32CAM

extern String esp32camStreamUrl;
extern void setEsp32camStreamUrl(const String& url);
extern String getEsp32camStreamUrl();

// Pagina HTML per configurazione ESP32-CAM (minificata)
const char ESP32CAM_CONFIG_HTML[] PROGMEM =
"<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>ESP32-CAM Config</title><style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:sans-serif;background:#1a1a2e;min-height:100vh;padding:20px;color:#eee}"
".c{max-width:600px;margin:0 auto;background:rgba(255,255,255,.1);border-radius:20px;overflow:hidden}"
".h{background:linear-gradient(135deg,#00d4ff,#0099cc);padding:25px;text-align:center}"
".h h1{font-size:1.5em;margin-bottom:5px}.h p{opacity:.8;font-size:.9em}"
".ct{padding:25px}"
".s{background:rgba(0,0,0,.3);border-radius:15px;padding:20px;margin-bottom:20px}"
".s h3{margin-bottom:15px;color:#00d4ff}"
".ig{margin-bottom:15px}.ig label{display:block;margin-bottom:8px;opacity:.8;font-size:.9em}"
".ig input{width:100%;padding:12px;border:none;border-radius:8px;background:rgba(255,255,255,.1);color:#fff;font-size:1em}"
".ig input:focus{outline:2px solid #00d4ff}"
".b{width:100%;padding:15px;border:none;border-radius:10px;font-weight:bold;cursor:pointer;font-size:1em;margin-top:10px}"
".b1{background:linear-gradient(135deg,#00d4ff,#0099cc);color:#fff}"
".b2{background:linear-gradient(135deg,#4CAF50,#45a049);color:#fff}"
".b3{background:linear-gradient(135deg,#ff9800,#f57c00);color:#fff}"
".b4{background:linear-gradient(135deg,#9c27b0,#7b1fa2);color:#fff}"
".hint{background:rgba(0,200,255,.1);border-left:3px solid #00d4ff;padding:15px;margin-top:15px;border-radius:0 8px 8px 0}"
".hint h4{color:#00d4ff;margin-bottom:8px}.hint p{font-size:.85em;opacity:.8;line-height:1.5}"
".hint code{background:rgba(0,0,0,.3);padding:2px 6px;border-radius:4px;font-family:monospace}"
".st{display:flex;align-items:center;gap:10px;margin-bottom:15px}"
".st .dot{width:12px;height:12px;border-radius:50%}"
".st .on{background:#4CAF50;box-shadow:0 0 10px #4CAF50}"
".st .off{background:#f44336}"
".preview{background:#000;border-radius:10px;padding:10px;margin-top:15px;text-align:center}"
".preview img{max-width:100%;border-radius:5px}"
".msg{padding:10px;border-radius:8px;margin-top:10px;display:none}"
".msg.ok{background:rgba(76,175,80,.2);border:1px solid #4CAF50;display:block}"
".msg.err{background:rgba(244,67,54,.2);border:1px solid #f44336;display:block}"
".hm{display:inline-block;text-align:center;color:#94a3b8;padding:10px;text-decoration:none;font-size:.9em}.hm:hover{color:#fff}"
".nav{display:flex;justify-content:space-between;padding:5px 15px}"
".cam-info{background:rgba(156,39,176,.2);border:1px solid #9c27b0;border-radius:10px;padding:15px;margin-top:15px}"
".cam-info p{margin:5px 0;font-size:.9em}"
".cam-ip{font-family:monospace;color:#00d4ff;font-weight:bold}"
"</style></head><body><div class=\"c\"><div class=\"nav\"><a href=\"/\" class=\"hm\">&larr; Home</a><a href=\"/settings\" class=\"hm\">Settings &rarr;</a></div><div class=\"h\">"
"<h1>ESP32-CAM Stream</h1><p>Configurazione camera Freenove</p></div><div class=\"ct\">"
"<div class=\"s\"><h3>Stato Connessione</h3>"
"<div class=\"st\"><div class=\"dot off\" id=\"dot\"></div><span id=\"status\">Non connesso</span></div>"
"<div id=\"info\" style=\"font-size:.85em;opacity:.7\"></div></div>"
"<div class=\"s\"><h3>Configurazione URL Stream</h3>"
"<div class=\"ig\"><label>URL Stream ESP32-CAM (porta 81)</label>"
"<input type=\"text\" id=\"url\" placeholder=\"http://192.168.1.100:81/stream\"></div>"
"<button class=\"b b1\" onclick=\"save()\">Salva URL</button>"
"<button class=\"b b2\" onclick=\"test()\" style=\"margin-top:10px\">Test Connessione</button>"
"<div id=\"msg\" class=\"msg\"></div></div>"
"<div class=\"s\"><h3>Interfaccia Camera</h3>"
"<p style=\"font-size:.85em;opacity:.8;margin-bottom:15px\">Apri l'interfaccia web della ESP32-CAM per cambiare risoluzione, effetti, luminosita', contrasto e altre impostazioni.</p>"
"<button class=\"b b4\" onclick=\"openCamInterface()\">Apri Impostazioni Camera</button>"
"<div class=\"cam-info\" id=\"camInfo\" style=\"display:none\">"
"<p>IP Camera: <span class=\"cam-ip\" id=\"camIp\">--</span></p>"
"<p>Interfaccia: <a href=\"#\" id=\"camLink\" target=\"_blank\" style=\"color:#00d4ff\">Apri in nuova scheda</a></p>"
"</div></div>"
"<div class=\"s\"><h3>Avvia Modalita'</h3>"
"<button class=\"b b3\" onclick=\"start()\">Avvia ESP32-CAM Stream</button></div>"
"<div class=\"hint\"><h4>Come configurare</h4>"
"<p>1. Programma la ESP32-CAM con <code>CameraWebServer</code><br>"
"2. Apri Serial Monitor per vedere l'IP assegnato<br>"
"3. L'URL stream e': <code>http://IP:81/stream</code><br>"
"4. L'interfaccia camera e': <code>http://IP</code> (porta 80)</p></div>"
"<div class=\"hint\"><h4>Impostazioni Camera consigliate</h4>"
"<p>- <b>Risoluzione:</b> 800x600 o 640x480 per migliori FPS<br>"
"- <b>Quality:</b> 10-15 per buon compromesso qualita'/velocita'<br>"
"- <b>Brightness/Contrast:</b> regola in base all'ambiente</p></div>"
"</div></div><script>"
"var camBaseUrl='';"
"function extractCamIp(url){"
"try{var u=new URL(url);return u.hostname;}catch(e){var m=url.match(/:\\/\\/([^:\\/]+)/);return m?m[1]:null;}}"
"function load(){fetch('/espcam/config').then(r=>r.json()).then(d=>{"
"var url=d.url||'';"
"document.getElementById('url').value=url;"
"document.getElementById('info').textContent='URL attuale: '+(url||'non configurato');"
"if(url){var ip=extractCamIp(url);if(ip){camBaseUrl='http://'+ip;"
"document.getElementById('camIp').textContent=ip;"
"document.getElementById('camLink').href=camBaseUrl;"
"document.getElementById('camInfo').style.display='block';}}"
"}).catch(()=>{})}"
"function save(){var u=document.getElementById('url').value,m=document.getElementById('msg');"
"if(!u){m.className='msg err';m.textContent='Inserisci un URL valido';return}"
"fetch('/espcam/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({url:u})})"
".then(r=>r.json()).then(d=>{if(d.success){m.className='msg ok';m.textContent='URL salvato!';"
"document.getElementById('info').textContent='URL attuale: '+u;"
"var ip=extractCamIp(u);if(ip){camBaseUrl='http://'+ip;"
"document.getElementById('camIp').textContent=ip;"
"document.getElementById('camLink').href=camBaseUrl;"
"document.getElementById('camInfo').style.display='block';}"
"}else{m.className='msg err';m.textContent='Errore salvataggio'}})"
".catch(()=>{m.className='msg err';m.textContent='Errore connessione'})}"
"function test(){var u=document.getElementById('url').value,d=document.getElementById('dot'),s=document.getElementById('status'),m=document.getElementById('msg');"
"if(!u){m.className='msg err';m.textContent='Inserisci un URL da testare';return}"
"m.className='msg';m.textContent='';s.textContent='Test in corso...';"
"fetch('/espcam/test',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({url:u})})"
".then(r=>r.json()).then(r=>{if(r.success){d.className='dot on';s.textContent='Camera raggiungibile!';"
"m.className='msg ok';m.textContent='Connessione riuscita - '+r.message}else{d.className='dot off';"
"s.textContent='Non raggiungibile';m.className='msg err';m.textContent='Errore: '+r.message}})"
".catch(()=>{d.className='dot off';s.textContent='Errore test';m.className='msg err';m.textContent='Errore connessione al server'})}"
"function openCamInterface(){"
"var url=document.getElementById('url').value;"
"if(!url){alert('Inserisci prima l\\'URL dello stream');return;}"
"var ip=extractCamIp(url);"
"if(ip){window.open('http://'+ip,'_blank');}else{alert('URL non valido');}}"
"function start(){fetch('/espcam/start',{method:'POST'}).then(r=>r.json()).then(d=>{"
"if(d.success)alert('Modalita\\' ESP32-CAM avviata!');else alert('Errore: '+d.message)}).catch(()=>alert('Errore connessione'))}"
"load();</script></body></html>";

void setup_esp32cam_webserver(AsyncWebServer* server) {
  Serial.println("[WEBSERVER] Configurazione ESP32-CAM Stream...");

  // Pagina principale configurazione
  server->on("/espcam", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", ESP32CAM_CONFIG_HTML);
  });

  // GET configurazione attuale
  server->on("/espcam/config", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"url\":\"" + esp32camStreamUrl + "\"}";
    request->send(200, "application/json", json);
  });

  // POST salva configurazione
  server->on("/espcam/config", HTTP_POST, [](AsyncWebServerRequest *request){},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String body = "";
      for (size_t i = 0; i < len; i++) body += (char)data[i];
      int urlStart = body.indexOf("\"url\":\"");
      if (urlStart >= 0) {
        urlStart += 7;
        int urlEnd = body.indexOf("\"", urlStart);
        if (urlEnd > urlStart) {
          String newUrl = body.substring(urlStart, urlEnd);
          setEsp32camStreamUrl(newUrl);
          Serial.printf("[ESP32-CAM] URL salvato: %s\n", newUrl.c_str());
          request->send(200, "application/json", "{\"success\":true}");
          return;
        }
      }
      request->send(400, "application/json", "{\"success\":false,\"message\":\"URL non valido\"}");
    });

  // POST test connessione
  server->on("/espcam/test", HTTP_POST, [](AsyncWebServerRequest *request){},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String body = "";
      for (size_t i = 0; i < len; i++) body += (char)data[i];
      int urlStart = body.indexOf("\"url\":\"");
      String testUrl = esp32camStreamUrl;

      if (urlStart >= 0) {
        urlStart += 7;
        int urlEnd = body.indexOf("\"", urlStart);
        if (urlEnd > urlStart) {
          testUrl = body.substring(urlStart, urlEnd);
        }
      }

      Serial.printf("[ESP32-CAM] Test connessione a: %s\n", testUrl.c_str());

      // Prova a connettersi all'URL
      WiFiClient testClient;
      HTTPClient testHttp;
      testHttp.begin(testClient, testUrl);
      testHttp.setTimeout(10000);  // 10 secondi per dare tempo alla camera

      int httpCode = testHttp.GET();
      testHttp.end();

      if (httpCode == HTTP_CODE_OK || httpCode == 206) {
        Serial.println("[ESP32-CAM] Test OK!");
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Stream MJPEG attivo\"}");
      } else if (httpCode > 0) {
        Serial.printf("[ESP32-CAM] Test HTTP code: %d\n", httpCode);
        String msg = "{\"success\":false,\"message\":\"HTTP " + String(httpCode) + "\"}";
        request->send(200, "application/json", msg);
      } else {
        Serial.printf("[ESP32-CAM] Test fallito: %d\n", httpCode);
        request->send(200, "application/json", "{\"success\":false,\"message\":\"Camera non raggiungibile\"}");
      }
    });

  // POST avvia modalità ESP32-CAM
  server->on("/espcam/start", HTTP_POST, [](AsyncWebServerRequest *request){
    extern DisplayMode currentMode;
    extern DisplayMode userMode;
    extern void forceDisplayUpdate();

    currentMode = MODE_ESP32CAM;
    userMode = MODE_ESP32CAM;

    // Salva in EEPROM
    EEPROM.write(EEPROM_MODE_ADDR, currentMode);
    EEPROM.commit();

    // Forza reinizializzazione
    esp32camInitialized = false;

    Serial.println("[ESP32-CAM] Modalita' avviata da web");
    request->send(200, "application/json", "{\"success\":true}");

    // Forza aggiornamento display
    forceDisplayUpdate();
  });

  Serial.println("[WEBSERVER] ESP32-CAM configurato su /espcam");
}

#endif // EFFECT_ESP32CAM
