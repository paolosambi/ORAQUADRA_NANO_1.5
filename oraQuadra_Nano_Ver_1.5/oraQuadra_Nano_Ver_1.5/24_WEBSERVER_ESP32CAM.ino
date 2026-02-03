// ================== WEB SERVER PER CONFIGURAZIONE ESP32-CAM STREAM ==================
#ifdef EFFECT_ESP32CAM

extern String esp32camStreamUrl;
extern void setEsp32camStreamUrl(const String& url);
extern String getEsp32camStreamUrl();

// Extern per gestione lista camere multiple
// La struct Esp32Camera è definita in 23_ESP32CAM_STREAM.ino
extern Esp32Camera esp32Cameras[];
extern int esp32CameraCount;
extern int esp32CameraSelected;
extern bool addEsp32Camera(const char* name, const char* url);
extern bool removeEsp32Camera(int index);
extern bool selectEsp32Camera(int index);

// Pagina HTML per configurazione ESP32-CAM con lista camere
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
".b5{background:linear-gradient(135deg,#e91e63,#c2185b);color:#fff}"
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
".clist{list-style:none;margin:0;padding:0}"
".clist li{display:flex;align-items:center;padding:12px;background:rgba(0,0,0,.2);border-radius:8px;margin-bottom:8px;gap:10px}"
".clist li.sel{background:rgba(0,212,255,.2);border:1px solid #00d4ff}"
".clist input[type=radio]{width:18px;height:18px;accent-color:#00d4ff}"
".clist .cn{flex:1;font-weight:bold;color:#fff}"
".clist .cu{font-size:.8em;color:#888;word-break:break-all}"
".clist .del{background:#f44336;border:none;color:#fff;padding:8px 12px;border-radius:5px;cursor:pointer;font-size:.85em;white-space:nowrap}"
".clist .del:hover{background:#d32f2f}"
".empty{text-align:center;padding:20px;color:#888;font-style:italic}"
".btns{display:flex;gap:10px}.btns .btn{flex:1}"
".clearall{background:#ff5722;margin-top:10px}"
"</style></head><body><div class=\"c\">"
"<div class=\"nav\"><a href=\"/\">&larr; Home</a><a href=\"/settings\">Settings &rarr;</a></div>"
"<div class=\"h\"><h1>ESP32-CAM Stream</h1><p>Gestione camere multiple</p></div>"
"<div class=\"ct\">"
"<div class=\"s\"><h3>Camere Salvate</h3>"
"<ul class=\"clist\" id=\"clist\"><li class=\"empty\">Caricamento...</li></ul>"
"<button class=\"btn clearall\" id=\"clearall\" style=\"display:none\" onclick=\"clearAll()\">Elimina Tutte le Camere</button>"
"<div class=\"msg\" id=\"msg1\"></div></div>"
"<div class=\"s\"><h3>Aggiungi Nuova Camera</h3>"
"<div class=\"fg\"><label>Nome (es: Camera Garage)</label>"
"<input type=\"text\" id=\"nname\" placeholder=\"Nome descrittivo\" maxlength=\"32\"></div>"
"<div class=\"fg\"><label>URL Stream (porta 81)</label>"
"<input type=\"text\" id=\"nurl\" placeholder=\"http://192.168.1.100:81/stream\"><div class=\"ex\">Formato: http://IP:81/stream</div></div>"
"<button class=\"btn b1\" onclick=\"addC()\">Aggiungi alla Lista</button>"
"<div class=\"msg\" id=\"msg2\"></div></div>"
"<div class=\"s\"><h3>Azioni Camera Selezionata</h3>"
"<div class=\"cur\"><b>Camera attiva:</b> <span id=\"cu\">Nessuna</span></div>"
"<div class=\"btns\"><button class=\"btn b2\" onclick=\"ts()\">Test Connessione</button>"
"<button class=\"btn b4\" onclick=\"oc()\">Interfaccia Camera</button></div>"
"<button class=\"btn b3\" onclick=\"go()\">Avvia sul Display</button></div>"
"<div class=\"hint\"><h4>Come configurare</h4><p>"
"1. Carica <code>CameraWebServer</code> sulla ESP32-CAM<br>"
"2. Apri Serial Monitor per vedere l'IP<br>"
"3. URL stream: <code>http://IP:81/stream</code><br>"
"4. Interfaccia: <code>http://IP</code> (porta 80)</p></div>"
"</div></div>"
"<script>"
"var I=document.getElementById.bind(document),clist,msg1,msg2,cu,nname,nurl,clearBtn,cameras=[],selIdx=0;"
"function init(){clist=I('clist');msg1=I('msg1');msg2=I('msg2');cu=I('cu');nname=I('nname');nurl=I('nurl');clearBtn=I('clearall');ldCams();}"
"function sm(el,t,c){el.textContent=t;el.className='msg '+c;}"
"function ip(u){var m=u.match(/:[\\/][\\/]([^:\\/]+)/);return m?m[1]:null;}"
"function ldCams(){fetch('/api/espcam/cameras').then(function(r){return r.json();}).then(function(d){"
"cameras=d.cameras||[];selIdx=d.selected||0;renderCams();}).catch(function(){clist.innerHTML='<li class=\"empty\">Errore caricamento</li>';});}"
"function renderCams(){clearBtn.style.display=cameras.length>0?'block':'none';"
"if(cameras.length==0){clist.innerHTML='<li class=\"empty\">Nessuna camera salvata</li>';cu.textContent='Nessuna';return;}"
"var h='';for(var i=0;i<cameras.length;i++){var c=cameras[i],sel=i==selIdx;"
"h+='<li class=\"'+(sel?'sel':'')+'\">';"
"h+='<input type=\"radio\" name=\"cam\" '+(sel?'checked':'')+' onchange=\"selC('+i+')\">';"
"h+='<div style=\"flex:1\"><div class=\"cn\">'+c.name+'</div><div class=\"cu\">'+c.url+'</div></div>';"
"h+='<button class=\"del\" onclick=\"delC('+i+')\">Elimina</button>';h+='</li>';}"
"clist.innerHTML=h;cu.textContent=cameras[selIdx].name+' ('+cameras[selIdx].url+')';}"
"function selC(idx){sm(msg1,'Selezione...','inf');fetch('/api/espcam/cameras/select',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({index:idx})})"
".then(function(r){return r.json();}).then(function(d){if(d.success){selIdx=idx;renderCams();sm(msg1,'Camera selezionata!','ok');}else{sm(msg1,'Errore: '+(d.message||''),'err');}}).catch(function(){sm(msg1,'Errore connessione','err');});}"
"function addC(){var n=nname.value.trim(),u=nurl.value.trim();if(!n){sm(msg2,'Inserisci un nome','err');return;}"
"if(!u||u.length<15||u.indexOf('http')!=0){sm(msg2,'URL non valido','err');return;}"
"sm(msg2,'Aggiunta...','inf');fetch('/api/espcam/cameras/add',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({name:n,url:u})})"
".then(function(r){return r.json();}).then(function(d){if(d.success){nname.value='';nurl.value='';ldCams();sm(msg2,'Camera aggiunta!','ok');}else{sm(msg2,'Errore: '+(d.message||''),'err');}}).catch(function(){sm(msg2,'Errore connessione','err');});}"
"function delC(idx){if(!confirm('Rimuovere questa camera?'))return;sm(msg1,'Rimozione...','inf');"
"fetch('/api/espcam/cameras/remove',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({index:idx})})"
".then(function(r){return r.json();}).then(function(d){if(d.success){ldCams();sm(msg1,'Camera rimossa!','ok');}else{sm(msg1,'Errore: '+(d.message||''),'err');}}).catch(function(){sm(msg1,'Errore connessione','err');});}"
"function clearAll(){if(!confirm('Eliminare TUTTE le camere salvate?'))return;sm(msg1,'Eliminazione...','inf');"
"fetch('/api/espcam/cameras/clear',{method:'POST'})"
".then(function(r){return r.json();}).then(function(d){if(d.success){ldCams();sm(msg1,'Tutte le camere eliminate!','ok');}else{sm(msg1,'Errore: '+(d.message||''),'err');}}).catch(function(){sm(msg1,'Errore connessione','err');});}"
"function ts(){if(cameras.length==0){alert('Nessuna camera');return;}var u=cameras[selIdx].url;sm(msg1,'Test...','inf');"
"fetch('/espcam/test',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({url:u})})"
".then(function(r){return r.json();}).then(function(d){if(d.success){sm(msg1,'Connessione OK!','ok');}else{sm(msg1,'Non raggiungibile: '+d.message,'err');}}).catch(function(){sm(msg1,'Errore test','err');});}"
"function oc(){if(cameras.length==0){alert('Nessuna camera');return;}var u=cameras[selIdx].url,i=ip(u);if(i){window.open('http://'+i,'_blank');}else{alert('URL non valido');}}"
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

  // GET configurazione attuale - usando path alternativo per evitare conflitti
  server->on("/api/espcam/config", HTTP_GET, [](AsyncWebServerRequest *request){
    String currentUrl = getEsp32camStreamUrl();
    currentUrl.trim();
    String json = "{\"url\":\"" + currentUrl + "\"}";
    Serial.printf("[ESP32-CAM] GET config - URL: '%s'\n", currentUrl.c_str());
    request->send(200, "application/json", json);
  });

  // POST salva configurazione
  // Variabili static per comunicare tra body handler e request handler
  static String pendingUrl = "";
  static bool urlParsedOk = false;

  server->on("/api/espcam/config", HTTP_POST,
    [](AsyncWebServerRequest *request){
      Serial.printf("[ESP32-CAM] POST handler - urlParsedOk=%d, pendingUrl='%s'\n", urlParsedOk, pendingUrl.c_str());

      if (urlParsedOk && pendingUrl.length() > 10) {
        // URL già estratto dal body handler, salva ora
        setEsp32camStreamUrl(pendingUrl);
        Serial.printf("[ESP32-CAM] URL SALVATO: '%s'\n", pendingUrl.c_str());

        // Verifica immediata
        String verify = getEsp32camStreamUrl();
        Serial.printf("[ESP32-CAM] VERIFICA dopo salvataggio: '%s'\n", verify.c_str());

        // Reset per prossima richiesta
        pendingUrl = "";
        urlParsedOk = false;

        request->send(200, "application/json", "{\"success\":true}");
      } else {
        Serial.println("[ESP32-CAM] POST FALLITO - URL non valido o body non parsato");
        pendingUrl = "";
        urlParsedOk = false;
        request->send(400, "application/json", "{\"success\":false,\"message\":\"URL non valido\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      // Body handler - viene chiamato PRIMA del request handler
      Serial.printf("[ESP32-CAM] Body handler: index=%d, len=%d, total=%d\n", (int)index, (int)len, (int)total);

      // Reset all'inizio di una nuova richiesta
      if (index == 0) {
        pendingUrl = "";
        urlParsedOk = false;
      }

      // Accumula body (per body piccoli, viene chiamato una sola volta)
      if (len > 0 && len < 500) {
        String body = "";
        for (size_t i = 0; i < len; i++) body += (char)data[i];
        Serial.printf("[ESP32-CAM] Body: %s\n", body.c_str());

        // Parse JSON: cerca "url":"valore"
        int start = body.indexOf("\"url\":\"");
        if (start >= 0) {
          start += 7;  // salta "url":"
          int end = body.indexOf("\"", start);
          if (end > start) {
            pendingUrl = body.substring(start, end);
            pendingUrl.trim();
            if (pendingUrl.length() > 10 && pendingUrl.startsWith("http")) {
              urlParsedOk = true;
              Serial.printf("[ESP32-CAM] URL estratto OK: '%s'\n", pendingUrl.c_str());
            } else {
              Serial.printf("[ESP32-CAM] URL non valido: '%s'\n", pendingUrl.c_str());
            }
          }
        }
      }
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
    extern void cleanupPreviousMode(DisplayMode);

    // Cleanup della modalità precedente prima di cambiare
    cleanupPreviousMode(currentMode);

    currentMode = MODE_ESP32CAM;
    userMode = MODE_ESP32CAM;
    EEPROM.write(EEPROM_MODE_ADDR, currentMode);
    EEPROM.commit();
    esp32camInitialized = false;

    Serial.println("[ESP32-CAM] Avviata da web");
    request->send(200, "application/json", "{\"success\":true}");
    forceDisplayUpdate();
  });

  // ================== ENDPOINT GESTIONE LISTA CAMERE ==================

  // GET lista camere salvate
  server->on("/api/espcam/cameras", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"cameras\":[";
    for (int i = 0; i < esp32CameraCount; i++) {
      if (i > 0) json += ",";
      json += "{\"name\":\"";
      json += esp32Cameras[i].name;
      json += "\",\"url\":\"";
      json += esp32Cameras[i].url;
      json += "\"}";
    }
    json += "],\"selected\":";
    json += String(esp32CameraSelected);
    json += "}";

    Serial.printf("[ESP32-CAM] GET cameras - %d camere, selezionata: %d\n", esp32CameraCount, esp32CameraSelected);
    request->send(200, "application/json", json);
  });

  // POST aggiungi camera
  static String addCameraBody;
  server->on("/api/espcam/cameras/add", HTTP_POST,
    [](AsyncWebServerRequest *request){
      // Estrai name e url dal body
      String name = "";
      String url = "";

      int nameStart = addCameraBody.indexOf("\"name\":\"");
      if (nameStart >= 0) {
        nameStart += 8;
        int nameEnd = addCameraBody.indexOf("\"", nameStart);
        if (nameEnd > nameStart) name = addCameraBody.substring(nameStart, nameEnd);
      }

      int urlStart = addCameraBody.indexOf("\"url\":\"");
      if (urlStart >= 0) {
        urlStart += 7;
        int urlEnd = addCameraBody.indexOf("\"", urlStart);
        if (urlEnd > urlStart) url = addCameraBody.substring(urlStart, urlEnd);
      }

      addCameraBody = "";
      name.trim();
      url.trim();

      if (name.length() == 0 || url.length() < 10 || !url.startsWith("http")) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Nome o URL non valido\"}");
        return;
      }

      if (addEsp32Camera(name.c_str(), url.c_str())) {
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Lista piena o errore\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      if (index == 0) addCameraBody = "";
      for (size_t i = 0; i < len; i++) addCameraBody += (char)data[i];
    }
  );

  // POST rimuovi camera
  static String removeCameraBody;
  server->on("/api/espcam/cameras/remove", HTTP_POST,
    [](AsyncWebServerRequest *request){
      int index = -1;

      int idxStart = removeCameraBody.indexOf("\"index\":");
      if (idxStart >= 0) {
        idxStart += 8;
        String idxStr = "";
        while (idxStart < removeCameraBody.length()) {
          char c = removeCameraBody[idxStart];
          if (c >= '0' && c <= '9') idxStr += c;
          else if (idxStr.length() > 0) break;
          idxStart++;
        }
        if (idxStr.length() > 0) index = idxStr.toInt();
      }

      removeCameraBody = "";

      if (index < 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Indice non valido\"}");
        return;
      }

      if (removeEsp32Camera(index)) {
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Impossibile rimuovere\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      if (index == 0) removeCameraBody = "";
      for (size_t i = 0; i < len; i++) removeCameraBody += (char)data[i];
    }
  );

  // POST seleziona camera attiva
  static String selectCameraBody;
  server->on("/api/espcam/cameras/select", HTTP_POST,
    [](AsyncWebServerRequest *request){
      int index = -1;

      int idxStart = selectCameraBody.indexOf("\"index\":");
      if (idxStart >= 0) {
        idxStart += 8;
        String idxStr = "";
        while (idxStart < selectCameraBody.length()) {
          char c = selectCameraBody[idxStart];
          if (c >= '0' && c <= '9') idxStr += c;
          else if (idxStr.length() > 0) break;
          idxStart++;
        }
        if (idxStr.length() > 0) index = idxStr.toInt();
      }

      selectCameraBody = "";

      if (index < 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Indice non valido\"}");
        return;
      }

      if (selectEsp32Camera(index)) {
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Impossibile selezionare\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      if (index == 0) selectCameraBody = "";
      for (size_t i = 0; i < len; i++) selectCameraBody += (char)data[i];
    }
  );

  // POST cancella tutte le camere
  server->on("/api/espcam/cameras/clear", HTTP_POST, [](AsyncWebServerRequest *request){
    extern void clearAllEsp32Cameras();
    clearAllEsp32Cameras();
    request->send(200, "application/json", "{\"success\":true}");
  });

  Serial.println("[WEBSERVER] ESP32-CAM su /espcam");
}

#endif // EFFECT_ESP32CAM
