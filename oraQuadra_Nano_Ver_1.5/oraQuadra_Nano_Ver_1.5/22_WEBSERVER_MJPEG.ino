// ================== WEB SERVER PER CONTROLLO MJPEG STREAM ==================
#ifdef EFFECT_MJPEG_STREAM

extern String mjpegStreamUrl;
extern void setMjpegAudioMute(bool muted);
extern void startMjpegServerStream();
extern void stopMjpegServerStream();

// Pagina HTML per controllo MJPEG Stream (minificata)
const char MJPEG_CONTROL_HTML[] PROGMEM =
"<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>MJPEG Control</title><style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:sans-serif;background:#1a1a2e;min-height:100vh;padding:20px;color:#eee}"
".c{max-width:600px;margin:0 auto;background:rgba(255,255,255,.1);border-radius:20px;overflow:hidden}"
".h{background:linear-gradient(135deg,#e94560,#0f3460);padding:25px;text-align:center}"
".h h1{font-size:1.5em}.ct{padding:25px}"
".s{background:rgba(0,0,0,.3);border-radius:15px;padding:20px;margin-bottom:20px}"
".sh{display:flex;align-items:center;margin-bottom:15px}"
".si{width:12px;height:12px;border-radius:50%;margin-right:10px}"
".on{background:#4CAF50;box-shadow:0 0 10px #4CAF50}"
".off{background:#f44336}.pa{background:#ff9800;box-shadow:0 0 10px #ff9800}"
".g{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}"
".gi{text-align:center;padding:10px;background:rgba(255,255,255,.05);border-radius:10px}"
".gv{font-size:1.5em;font-weight:bold;color:#e94560}.gl{font-size:.8em;opacity:.7}"
".cs{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;margin-bottom:20px}"
".b{padding:15px;border:none;border-radius:10px;font-weight:bold;cursor:pointer}"
".b1{background:linear-gradient(135deg,#4CAF50,#45a049);color:#fff}"
".b2{background:linear-gradient(135deg,#f44336,#da190b);color:#fff}"
".b3{background:linear-gradient(135deg,#ff9800,#f57c00);color:#fff}"
".b4{background:linear-gradient(135deg,#2196F3,#1976D2);color:#fff}"
".b5{background:linear-gradient(135deg,#9c27b0,#7b1fa2);color:#fff;grid-column:span 2}"
".ig{margin-bottom:15px}.ig label{display:block;margin-bottom:5px;opacity:.8}"
".ig input{width:100%;padding:12px;border:none;border-radius:8px;background:rgba(255,255,255,.1);color:#fff}"
".la{background:rgba(0,0,0,.5);border-radius:10px;padding:15px;max-height:150px;overflow-y:auto;font-family:monospace;font-size:.8em}"
".le{padding:3px 0;border-bottom:1px solid rgba(255,255,255,.1)}.lt{color:#888}.ls{color:#4CAF50}.lr{color:#f44336}"
".hm{display:block;text-align:center;color:#94a3b8;padding:10px;text-decoration:none;font-size:.9em}.hm:hover{color:#fff}"
"</style></head><body><div class=\"c\"><a href=\"/\" class=\"hm\">&larr; Home</a><div class=\"h\"><h1>MJPEG Stream Control</h1></div><div class=\"ct\">"
"<div class=\"s\"><div class=\"sh\"><div class=\"si off\" id=\"ind\"></div><span id=\"st\">...</span></div>"
"<div class=\"g\"><div class=\"gi\"><div class=\"gv\" id=\"fc\">-</div><div class=\"gl\">Frame</div></div>"
"<div class=\"gi\"><div class=\"gv\" id=\"fp\">-</div><div class=\"gl\">FPS</div></div>"
"<div class=\"gi\"><div class=\"gv\" id=\"cl\">-</div><div class=\"gl\">Client</div></div></div></div>"
"<div class=\"cs\"><button class=\"b b1\" onclick=\"ss()\">Avvia</button><button class=\"b b2\" onclick=\"sp()\">Stop</button>"
"<button class=\"b b3\" onclick=\"pa()\">Pausa</button><button class=\"b b4\" onclick=\"re()\">Riprendi</button></div>"
"<div class=\"s\"><h3 style=\"margin-bottom:15px\">Configurazione</h3><div class=\"ig\">"
"<label>URL Server Stream</label><input type=\"text\" id=\"url\" placeholder=\"http://192.168.1.24:5000/stream\"></div>"
"<div style=\"margin-bottom:10px;opacity:.7;font-size:.9em\">Server: <span id=\"srv\">-</span></div>"
"<button class=\"b b5\" onclick=\"sv()\">Salva</button></div>"
"<div class=\"la\" id=\"log\"><div class=\"le\"><span class=\"lt\">[--:--:--]</span> Init...</div></div>"
"</div></div><script>"
"var bu='';"
"function lg(m,t){var d=new Date(),tm=d.toTimeString().split(' ')[0],l=document.getElementById('log'),"
"e=document.createElement('div');e.className='le';e.innerHTML='<span class=\"lt\">['+tm+']</span> <span class=\"l'+(t||'t')+'\">'+m+'</span>';"
"l.appendChild(e);l.scrollTop=l.scrollHeight}"
"function us(){if(!bu)return;fetch(bu+'/api/status').then(function(r){return r.json()}).then(function(d){"
"var i=document.getElementById('ind'),s=document.getElementById('st');"
"if(d.is_streaming){if(d.audio_muted){i.className='si pa';s.textContent='Pausa'}else{i.className='si on';s.textContent='Attivo'}}"
"else{i.className='si off';s.textContent=d.status||'Fermo'}"
"document.getElementById('fc').textContent=d.frame_count+'/'+(d.total_frames||'-');"
"document.getElementById('fp').textContent=Math.round(d.fps||0);"
"document.getElementById('cl').textContent=d.clients||0}).catch(function(){document.getElementById('ind').className='si off';"
"document.getElementById('st').textContent='Offline'})}"
"function ss(){lg('Avvio...');fetch('/mjpeg/start',{method:'POST'}).then(function(r){return r.json()}).then(function(d){"
"lg(d.success?'Avviato':'Errore','s')}).catch(function(){lg('Errore','r')})}"
"function sp(){lg('Stop...');fetch('/mjpeg/stop',{method:'POST'}).then(function(r){return r.json()}).then(function(d){"
"lg(d.success?'Fermato':'Errore','s')}).catch(function(){lg('Errore','r')})}"
"function pa(){lg('Pausa...');fetch('/mjpeg/pause',{method:'POST'}).then(function(r){return r.json()}).then(function(d){"
"lg(d.success?'In pausa':'Errore','s')}).catch(function(){lg('Errore','r')})}"
"function re(){lg('Riprendo...');fetch('/mjpeg/resume',{method:'POST'}).then(function(r){return r.json()}).then(function(d){"
"lg(d.success?'Ripreso':'Errore','s')}).catch(function(){lg('Errore','r')})}"
"function sv(){var u=document.getElementById('url').value;if(!u){lg('URL vuoto','r');return}lg('Salvo...');"
"fetch('/mjpeg/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({url:u})}).then(function(r){"
"return r.json()}).then(function(d){if(d.success){lg('Salvato','s');lc()}else lg('Errore','r')}).catch(function(){lg('Errore','r')})}"
"function lc(){fetch('/mjpeg/config').then(function(r){return r.json()}).then(function(d){"
"document.getElementById('url').value=d.url||'';document.getElementById('srv').textContent=d.url||'-';"
"if(d.url){var p=d.url.indexOf('/stream');bu=p>0?d.url.substring(0,p):d.url}lg('Config OK','s')}).catch(function(){lg('Errore config','r')})}"
"lc();setInterval(us,2000);setTimeout(us,500);"
"</script></body></html>";

void setup_mjpeg_webserver(AsyncWebServer* server) {
  Serial.println("[WEBSERVER] Configurazione MJPEG Stream...");

  server->on("/mjpeg", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", MJPEG_CONTROL_HTML);
  });

  server->on("/mjpeg/config", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"url\":\"" + mjpegStreamUrl + "\"}";
    request->send(200, "application/json", json);
  });

  server->on("/mjpeg/config", HTTP_POST, [](AsyncWebServerRequest *request){},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String body = "";
      for (size_t i = 0; i < len; i++) body += (char)data[i];
      int urlStart = body.indexOf("\"url\":\"");
      if (urlStart >= 0) {
        urlStart += 7;
        int urlEnd = body.indexOf("\"", urlStart);
        if (urlEnd > urlStart) {
          mjpegStreamUrl = body.substring(urlStart, urlEnd);
          Serial.printf("[MJPEG] URL: %s\n", mjpegStreamUrl.c_str());
          request->send(200, "application/json", "{\"success\":true}");
          return;
        }
      }
      request->send(400, "application/json", "{\"success\":false}");
    });

  server->on("/mjpeg/start", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("[MJPEG] START");
    startMjpegServerStream();
    request->send(200, "application/json", "{\"success\":true}");
  });

  server->on("/mjpeg/stop", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("[MJPEG] STOP");
    stopMjpegServerStream();
    request->send(200, "application/json", "{\"success\":true}");
  });

  server->on("/mjpeg/pause", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("[MJPEG] PAUSE");
    setMjpegAudioMute(true);
    request->send(200, "application/json", "{\"success\":true}");
  });

  server->on("/mjpeg/resume", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("[MJPEG] RESUME");
    setMjpegAudioMute(false);
    request->send(200, "application/json", "{\"success\":true}");
  });

  Serial.println("[WEBSERVER] MJPEG configurato");
}

#endif
