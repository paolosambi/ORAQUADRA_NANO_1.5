// ================== WEB SERVER LED RGB ==================
// Pagina configurazione LED RGB WS2812
// Endpoint: /ledrgb, /ledrgb/status, /ledrgb/set

#ifdef EFFECT_LED_RGB

// Pagina HTML inline
static const char LED_RGB_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>LED RGB - OraQuadra Nano</title>
  <style>
    *{margin:0;padding:0;box-sizing:border-box}
    :root{--accent:#00d9ff;--bg:#0a0a1a;--card:rgba(20,20,40,0.9)}
    body{font-family:'Segoe UI',system-ui,sans-serif;background:var(--bg);color:#fff;min-height:100vh;
      background:linear-gradient(135deg,#0a0a1a 0%,#1a1a3a 50%,#0a1a2a 100%)}
    .container{max-width:500px;margin:0 auto;padding:20px}
    .header{text-align:center;margin-bottom:25px}
    .header h1{font-size:1.8rem;background:linear-gradient(90deg,#00d9ff,#ff00ff);
      -webkit-background-clip:text;-webkit-text-fill-color:transparent;margin-bottom:5px}
    .header p{color:#64748b;font-size:0.85rem}
    .back-link{display:inline-block;color:var(--accent);text-decoration:none;margin-bottom:15px;font-size:0.9rem}
    .back-link:hover{text-decoration:underline}
    .card{background:var(--card);border:1px solid rgba(255,255,255,0.08);border-radius:16px;padding:20px;margin-bottom:16px;
      backdrop-filter:blur(10px)}
    .card h3{margin-bottom:15px;color:#94a3b8;letter-spacing:1px;text-transform:uppercase;font-size:0.75rem}
    .row{display:flex;align-items:center;justify-content:space-between;margin-bottom:14px}
    .row:last-child{margin-bottom:0}
    .label{font-size:0.95rem;color:#e2e8f0}
    .sublabel{font-size:0.75rem;color:#64748b;margin-top:2px}
    .toggle{position:relative;width:52px;height:28px;flex-shrink:0}
    .toggle input{opacity:0;width:0;height:0}
    .toggle .sl{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;
      background:#334155;border-radius:28px;transition:0.3s}
    .toggle .sl:before{content:'';position:absolute;height:22px;width:22px;left:3px;bottom:3px;
      background:#fff;border-radius:50%;transition:0.3s}
    .toggle input:checked+.sl{background:var(--accent)}
    .toggle input:checked+.sl:before{transform:translateX(24px)}
    .slider-wrap{display:flex;align-items:center;gap:12px;width:100%}
    .slider-wrap input[type=range]{flex:1;height:6px;-webkit-appearance:none;background:#334155;border-radius:3px;outline:none}
    .slider-wrap input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;border-radius:50%;
      background:var(--accent);cursor:pointer;box-shadow:0 0 10px rgba(0,217,255,0.5)}
    .slider-val{min-width:36px;text-align:right;font-weight:bold;font-size:0.95rem}
    .color-section{display:none;margin-top:15px}
    .color-section.visible{display:block}
    .color-row{display:flex;align-items:center;gap:12px;margin-bottom:10px}
    .color-row label{min-width:20px;font-weight:bold;font-size:0.9rem}
    .color-row.red label{color:#ff4444}
    .color-row.green label{color:#44ff44}
    .color-row.blue label{color:#4444ff}
    .color-row input[type=range]{flex:1;height:6px;-webkit-appearance:none;border-radius:3px;outline:none}
    .color-row.red input{background:linear-gradient(90deg,#000,#f00)}
    .color-row.green input{background:linear-gradient(90deg,#000,#0f0)}
    .color-row.blue input{background:linear-gradient(90deg,#000,#00f)}
    .color-row input::-webkit-slider-thumb{-webkit-appearance:none;width:18px;height:18px;border-radius:50%;
      background:#fff;cursor:pointer;border:2px solid #888}
    .color-row .val{min-width:30px;text-align:right;font-size:0.85rem}
    .preview-row{display:flex;align-items:center;gap:15px;margin-top:10px;padding-top:12px;border-top:1px solid rgba(255,255,255,0.08)}
    .preview-label{font-size:0.85rem;color:#94a3b8}
    .preview-dot{width:40px;height:40px;border-radius:50%;border:2px solid rgba(255,255,255,0.2);
      transition:background 0.3s,box-shadow 0.3s}
    .theme-info{font-size:0.8rem;color:#64748b;margin-top:6px;font-style:italic}
    .hex-row{display:flex;align-items:center;gap:10px;margin-bottom:10px}
    .hex-row label{font-size:0.85rem;color:#94a3b8;min-width:35px}
    .hex-input{background:#1e293b;border:1px solid rgba(255,255,255,0.15);border-radius:8px;
      color:#fff;padding:6px 10px;font-size:0.9rem;font-family:monospace;width:100px;text-transform:uppercase}
    .hex-input:focus{outline:none;border-color:var(--accent)}
    .status{text-align:center;padding:8px;border-radius:8px;margin-top:10px;font-size:0.85rem;
      opacity:0;transition:opacity 0.3s}
    .status.show{opacity:1}
    .status.ok{color:#4ade80}
    .status.err{color:#f87171}
  </style>
</head>
<body>
  <div class="container">
    <a href="/settings" class="back-link">&larr; Settings</a>
    <div class="header">
      <h1>LED RGB</h1>
      <p>Anello WS2812 - 12 LED</p>
    </div>

    <div class="card">
      <h3>Generale</h3>
      <div class="row">
        <div>
          <div class="label">LED RGB</div>
          <div class="sublabel">Accendi o spegni l'anello LED</div>
        </div>
        <label class="toggle">
          <input type="checkbox" id="enabled">
          <span class="sl"></span>
        </label>
      </div>
      <div class="row" style="flex-direction:column;align-items:stretch">
        <div style="display:flex;justify-content:space-between;margin-bottom:8px">
          <div class="label">Luminosita</div>
          <span class="slider-val" id="brightVal">80</span>
        </div>
        <div class="slider-wrap">
          <input type="range" id="brightness" min="0" max="255" value="80">
        </div>
      </div>
      <div class="row" style="flex-direction:column;align-items:stretch">
        <div style="display:flex;justify-content:space-between;margin-bottom:8px">
          <div>
            <div class="label">Audio Reactive</div>
            <div class="sublabel">LED reagiscono alla musica</div>
          </div>
        </div>
        <select id="audioReactive" style="width:100%;padding:10px;border-radius:8px;border:1px solid rgba(255,255,255,0.15);background:#1e293b;color:#fff;font-size:0.9rem">
          <option value="0">Disattivato</option>
          <option value="1">LED accesi: pulsano con la musica</option>
          <option value="2">LED spenti: si accendono con la musica</option>
        </select>
      </div>
    </div>

    <div class="card">
      <h3>Colore</h3>
      <div class="row">
        <div>
          <div class="label">Colore personalizzato</div>
          <div class="sublabel">Ignora il colore del tema attivo</div>
        </div>
        <label class="toggle">
          <input type="checkbox" id="override">
          <span class="sl"></span>
        </label>
      </div>

      <div class="color-section" id="colorSection">
        <div class="hex-row">
          <label>HEX</label>
          <input type="text" class="hex-input" id="hexInput" placeholder="#FF00FF" maxlength="7">
        </div>
        <div class="color-row red">
          <label>R</label>
          <input type="range" id="sliderR" min="0" max="255" value="255">
          <span class="val" id="valR">255</span>
        </div>
        <div class="color-row green">
          <label>G</label>
          <input type="range" id="sliderG" min="0" max="255" value="255">
          <span class="val" id="valG">255</span>
        </div>
        <div class="color-row blue">
          <label>B</label>
          <input type="range" id="sliderB" min="0" max="255" value="255">
          <span class="val" id="valB">255</span>
        </div>
      </div>

      <div class="preview-row">
        <div class="preview-label">Colore attuale:</div>
        <div class="preview-dot" id="previewDot"></div>
        <div>
          <div id="previewRGB" style="font-size:0.85rem;font-family:monospace"></div>
          <div class="theme-info" id="themeInfo">Segue il tema attivo</div>
        </div>
      </div>
    </div>

    <div class="status" id="status"></div>
  </div>

  <script>
    var saveTimer = null;
    var themeR = 255, themeG = 200, themeB = 100;
    var loading = true;

    function $(id){ return document.getElementById(id); }

    function updatePreview(){
      var dot = $('previewDot');
      var txt = $('previewRGB');
      if(!$('enabled').checked){
        dot.style.background = '#333';
        dot.style.boxShadow = 'none';
        txt.textContent = 'OFF';
        return;
      }
      var r, g, b;
      if($('override').checked){
        r = parseInt($('sliderR').value);
        g = parseInt($('sliderG').value);
        b = parseInt($('sliderB').value);
      } else {
        r = themeR; g = themeG; b = themeB;
      }
      var c = 'rgb('+r+','+g+','+b+')';
      dot.style.background = c;
      dot.style.boxShadow = '0 0 15px '+c+', 0 0 30px '+c;
      txt.textContent = 'R:'+r+' G:'+g+' B:'+b;
    }

    function slidersToHex(){
      var r = parseInt($('sliderR').value);
      var g = parseInt($('sliderG').value);
      var b = parseInt($('sliderB').value);
      $('valR').textContent = r;
      $('valG').textContent = g;
      $('valB').textContent = b;
      $('hexInput').value = '#' +
        ('0'+r.toString(16)).slice(-2) +
        ('0'+g.toString(16)).slice(-2) +
        ('0'+b.toString(16)).slice(-2);
    }

    function hexToSliders(){
      var hex = $('hexInput').value.replace('#','');
      if(hex.length !== 6) return;
      var r = parseInt(hex.substr(0,2),16);
      var g = parseInt(hex.substr(2,2),16);
      var b = parseInt(hex.substr(4,2),16);
      if(isNaN(r)||isNaN(g)||isNaN(b)) return;
      $('sliderR').value = r;
      $('sliderG').value = g;
      $('sliderB').value = b;
      $('valR').textContent = r;
      $('valG').textContent = g;
      $('valB').textContent = b;
      updatePreview();
      schedSave();
    }

    function toggleOverride(){
      var show = $('override').checked;
      $('colorSection').className = 'color-section' + (show ? ' visible' : '');
      $('themeInfo').textContent = show ? 'Colore personalizzato' : 'Segue il tema attivo';
      updatePreview();
    }

    function showStatus(txt, ok){
      var el = $('status');
      el.textContent = txt;
      el.className = 'status show ' + (ok ? 'ok' : 'err');
      setTimeout(function(){ el.className = 'status'; }, 2000);
    }

    function schedSave(){
      if(loading) return;
      if(saveTimer) clearTimeout(saveTimer);
      saveTimer = setTimeout(doSave, 300);
    }

    function doSave(){
      var url = '/ledrgb/set?enabled='+($('enabled').checked?1:0)
        +'&brightness='+$('brightness').value
        +'&override='+($('override').checked?1:0)
        +'&r='+$('sliderR').value
        +'&g='+$('sliderG').value
        +'&b='+$('sliderB').value
        +'&audioReactive='+$('audioReactive').value
        +'&_t='+Date.now();
      fetch(url)
        .then(function(r){ return r.text(); })
        .then(function(d){ showStatus('Salvato', true); })
        .catch(function(e){ showStatus('Errore', false); });
    }

    function loadStatus(){
      fetch('/ledrgb/status?_t='+Date.now())
        .then(function(r){
          if(!r.ok) throw new Error('HTTP '+r.status);
          return r.json();
        })
        .then(function(d){
          $('enabled').checked = d.enabled;
          $('brightness').value = d.brightness;
          $('brightVal').textContent = d.brightness;
          $('override').checked = d.override;
          $('sliderR').value = d.r;
          $('sliderG').value = d.g;
          $('sliderB').value = d.b;
          $('valR').textContent = d.r;
          $('valG').textContent = d.g;
          $('valB').textContent = d.b;
          $('audioReactive').value = d.audioReactive;
          themeR = d.themeR;
          themeG = d.themeG;
          themeB = d.themeB;
          slidersToHex();
          toggleOverride();
          updatePreview();
          loading = false;
          setupEvents();
        })
        .catch(function(e){
          console.error('Load error:', e);
          loading = false;
          setupEvents();
        });
    }

    function setupEvents(){
      // Toggle e checkbox: salva su change
      $('enabled').addEventListener('change', function(){ updatePreview(); schedSave(); });
      $('audioReactive').addEventListener('change', function(){ schedSave(); });  // select
      $('override').addEventListener('change', function(){ toggleOverride(); schedSave(); });

      // Brightness slider
      $('brightness').addEventListener('input', function(){
        $('brightVal').textContent = this.value;
      });
      $('brightness').addEventListener('change', function(){ schedSave(); });

      // Color sliders
      ['sliderR','sliderG','sliderB'].forEach(function(id){
        $(id).addEventListener('input', function(){
          slidersToHex();
          updatePreview();
        });
        $(id).addEventListener('change', function(){ schedSave(); });
      });

      // Hex input
      $('hexInput').addEventListener('input', function(){ hexToSliders(); });
    }

    loadStatus();
  </script>
</body>
</html>
)rawliteral";

// ===== Setup webserver =====
void setup_ledrgb_webserver(AsyncWebServer* server) {

  // IMPORTANTE: registrare endpoint specifici PRIMA di quello generico!
  // Altrimenti /ledrgb cattura anche /ledrgb/status e /ledrgb/set

  // Status JSON
  server->on("/ledrgb/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    uint8_t tR = 255, tG = 200, tB = 100;
    getLedColorForMode((DisplayMode)currentMode, tR, tG, tB);

    String json = "{";
    json += "\"enabled\":" + String(ledRgbEnabled ? "true" : "false") + ",";
    json += "\"brightness\":" + String(ledRgbBrightness) + ",";
    json += "\"override\":" + String(ledRgbOverride ? "true" : "false") + ",";
    json += "\"r\":" + String(ledRgbOverrideR) + ",";
    json += "\"g\":" + String(ledRgbOverrideG) + ",";
    json += "\"b\":" + String(ledRgbOverrideB) + ",";
    json += "\"audioReactive\":" + String(ledAudioReactive) + ",";
    json += "\"themeR\":" + String(tR) + ",";
    json += "\"themeG\":" + String(tG) + ",";
    json += "\"themeB\":" + String(tB);
    json += "}";

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    request->send(response);
  });

  // Salva impostazioni
  server->on("/ledrgb/set", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("enabled")) {
      ledRgbEnabled = (request->getParam("enabled")->value() == "1");
    }
    if (request->hasParam("brightness")) {
      ledRgbBrightness = request->getParam("brightness")->value().toInt();
    }
    if (request->hasParam("override")) {
      ledRgbOverride = (request->getParam("override")->value() == "1");
    }
    if (request->hasParam("r")) {
      ledRgbOverrideR = request->getParam("r")->value().toInt();
    }
    if (request->hasParam("g")) {
      ledRgbOverrideG = request->getParam("g")->value().toInt();
    }
    if (request->hasParam("b")) {
      ledRgbOverrideB = request->getParam("b")->value().toInt();
    }
    if (request->hasParam("audioReactive")) {
      ledAudioReactive = request->getParam("audioReactive")->value().toInt();
      if (ledAudioReactive > 2) ledAudioReactive = 0;
    }

    saveLedRgbSettings();

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    request->send(response);
    Serial.printf("[LED RGB] Salvato: enabled=%d, bright=%d, override=%d, R=%d G=%d B=%d, audio=%d\n",
                  ledRgbEnabled, ledRgbBrightness, ledRgbOverride, ledRgbOverrideR, ledRgbOverrideG, ledRgbOverrideB, ledAudioReactive);
  });

  // Pagina HTML (registrata PER ULTIMA)
  server->on("/ledrgb", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", LED_RGB_HTML);
  });

  Serial.println("[LED RGB] Webserver: /ledrgb/status, /ledrgb/set, /ledrgb");
}

#endif // EFFECT_LED_RGB
