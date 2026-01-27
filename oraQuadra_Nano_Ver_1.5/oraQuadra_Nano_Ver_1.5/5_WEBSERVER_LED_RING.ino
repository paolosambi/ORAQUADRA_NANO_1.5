// ================== WEB SERVER PER CONFIGURAZIONE LED RING ==================
#ifdef EFFECT_LED_RING

// Dichiarazioni extern per variabili definite in altri file
extern bool ledRingFirstDraw;
extern uint8_t ledRingHoursR, ledRingHoursG, ledRingHoursB;
extern uint8_t ledRingMinutesR, ledRingMinutesG, ledRingMinutesB;
extern uint8_t ledRingSecondsR, ledRingSecondsG, ledRingSecondsB;
extern uint8_t ledRingDigitalR, ledRingDigitalG, ledRingDigitalB;
extern bool ledRingHoursRainbow, ledRingMinutesRainbow, ledRingSecondsRainbow;

// HTML compatto per configurazione LED Ring
const char LEDRING_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>LED Ring Config</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:sans-serif;background:#1a1a2e;color:#eee;padding:20px;min-height:100vh}
.container{max-width:600px;margin:0 auto;background:#1e293b;border-radius:15px;padding:20px}
h1{text-align:center;color:#00d9ff;margin-bottom:20px}
.back{display:block;text-align:center;color:#94a3b8;margin-bottom:15px;text-decoration:none}
.section{background:#0f172a;padding:15px;border-radius:10px;margin-bottom:15px}
.section h2{color:#00d9ff;font-size:1rem;margin-bottom:15px;border-bottom:1px solid #334155;padding-bottom:8px}
.row{display:flex;align-items:center;justify-content:space-between;padding:10px 0;border-bottom:1px solid #334155}
.row:last-child{border:none}
.label{font-weight:500}
.controls{display:flex;align-items:center;gap:15px}
input[type="color"]{width:50px;height:35px;border:none;border-radius:6px;cursor:pointer}
.toggle{position:relative;width:44px;height:24px}
.toggle input{opacity:0;width:0;height:0}
.slider{position:absolute;cursor:pointer;inset:0;background:#475569;border-radius:24px;transition:.3s}
.slider:before{content:'';position:absolute;height:18px;width:18px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.3s}
input:checked+.slider{background:linear-gradient(90deg,red,orange,yellow,green,cyan,blue,violet)}
input:checked+.slider:before{transform:translateX(20px)}
.preview{text-align:center;padding:20px}
.ring-preview{width:200px;height:200px;margin:0 auto;position:relative;background:#000;border-radius:50%}
.ring{position:absolute;border-radius:50%;border:3px solid}
.ring-h{width:180px;height:180px;top:10px;left:10px;border-color:#ff6400}
.ring-m{width:150px;height:150px;top:25px;left:25px;border-color:#0096ff}
.ring-s{width:120px;height:120px;top:40px;left:40px;border-color:#00ffff}
.digital{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);font-family:monospace;font-size:1.5rem;color:#00ffc8}
.btn{display:block;width:100%;padding:12px;border:none;border-radius:8px;font-size:1rem;cursor:pointer;margin-top:10px}
.btn-save{background:#22c55e;color:#fff}
.btn-reset{background:#475569;color:#fff}
.msg{text-align:center;padding:10px;margin-top:10px;border-radius:6px;display:none}
.msg.ok{display:block;background:#166534;color:#6aff6a}
.msg.err{display:block;background:#7f1d1d;color:#fca5a5}
</style></head><body>
<div class="container">
<a href="/" class="back">&larr; Home</a>
<h1>LED Ring</h1>
<div class="section">
<h2>Preview</h2>
<div class="preview">
<div class="ring-preview">
<div class="ring ring-h" id="pH"></div>
<div class="ring ring-m" id="pM"></div>
<div class="ring ring-s" id="pS"></div>
<div class="digital" id="pD">12:30</div>
</div>
</div>
</div>
<div class="section">
<h2>Colori</h2>
<div class="row"><span class="label">Ore</span><div class="controls">
<input type="color" id="cH" value="#ff6400">
<label class="toggle"><input type="checkbox" id="rH"><span class="slider"></span></label>
</div></div>
<div class="row"><span class="label">Minuti</span><div class="controls">
<input type="color" id="cM" value="#0096ff">
<label class="toggle"><input type="checkbox" id="rM"><span class="slider"></span></label>
</div></div>
<div class="row"><span class="label">Secondi</span><div class="controls">
<input type="color" id="cS" value="#00ffff">
<label class="toggle"><input type="checkbox" id="rS"><span class="slider"></span></label>
</div></div>
<div class="row"><span class="label">Display</span><div class="controls">
<input type="color" id="cD" value="#00ffc8">
</div></div>
</div>
<button class="btn btn-save" onclick="save()">Salva</button>
<button class="btn btn-reset" onclick="reset()">Default</button>
<div class="msg" id="msg"></div>
</div>
<script>
const $=id=>document.getElementById(id);
function upd(){
$('pH').style.borderColor=$('cH').value;
$('pM').style.borderColor=$('cM').value;
$('pS').style.borderColor=$('cS').value;
$('pD').style.color=$('cD').value;
}
['cH','cM','cS','cD'].forEach(id=>{
$(id).oninput=upd;
$(id).onchange=save;
});
['rH','rM','rS'].forEach(id=>{
$(id).onchange=save;
});
async function load(){
try{
const r=await fetch('/ledring/config');
const c=await r.json();
$('cH').value=c.hoursColor;
$('cM').value=c.minutesColor;
$('cS').value=c.secondsColor;
$('cD').value=c.digitalColor;
$('rH').checked=c.hoursRainbow;
$('rM').checked=c.minutesRainbow;
$('rS').checked=c.secondsRainbow;
upd();
}catch(e){console.error(e);}
}
async function save(){
try{
const c={
hoursColor:$('cH').value,
minutesColor:$('cM').value,
secondsColor:$('cS').value,
digitalColor:$('cD').value,
hoursRainbow:$('rH').checked,
minutesRainbow:$('rM').checked,
secondsRainbow:$('rS').checked
};
const r=await fetch('/ledring/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(c)});
if(r.ok)showMsg('Salvato!','ok');
else showMsg('Errore','err');
}catch(e){showMsg('Errore','err');}
}
async function reset(){
if(!confirm('Ripristinare default?'))return;
try{
const r=await fetch('/ledring/reset',{method:'POST'});
if(r.ok){load();showMsg('Default ripristinati','ok');}
}catch(e){showMsg('Errore','err');}
}
function showMsg(t,c){
const m=$('msg');
m.textContent=t;
m.className='msg '+c;
setTimeout(()=>m.className='msg',2000);
}
load();
setInterval(()=>{
const d=new Date();
$('pD').textContent=d.getHours().toString().padStart(2,'0')+':'+d.getMinutes().toString().padStart(2,'0');
},1000);
</script>
</body></html>
)rawliteral";

// Helper: hex to RGB
void hexToRgbLedRing(const char* hex, uint8_t &r, uint8_t &g, uint8_t &b) {
  if (hex[0] == '#') hex++;
  char rs[3]={hex[0],hex[1],0}, gs[3]={hex[2],hex[3],0}, bs[3]={hex[4],hex[5],0};
  r = strtol(rs, NULL, 16);
  g = strtol(gs, NULL, 16);
  b = strtol(bs, NULL, 16);
}

// Helper: RGB to hex
String rgbToHexLedRing(uint8_t r, uint8_t g, uint8_t b) {
  char hex[8];
  sprintf(hex, "#%02x%02x%02x", r, g, b);
  return String(hex);
}

// Salva config in EEPROM
void saveLedRingConfig() {
  EEPROM.write(EEPROM_LEDRING_HOURS_R_ADDR, ledRingHoursR);
  EEPROM.write(EEPROM_LEDRING_HOURS_G_ADDR, ledRingHoursG);
  EEPROM.write(EEPROM_LEDRING_HOURS_B_ADDR, ledRingHoursB);
  EEPROM.write(EEPROM_LEDRING_MINUTES_R_ADDR, ledRingMinutesR);
  EEPROM.write(EEPROM_LEDRING_MINUTES_G_ADDR, ledRingMinutesG);
  EEPROM.write(EEPROM_LEDRING_MINUTES_B_ADDR, ledRingMinutesB);
  EEPROM.write(EEPROM_LEDRING_SECONDS_R_ADDR, ledRingSecondsR);
  EEPROM.write(EEPROM_LEDRING_SECONDS_G_ADDR, ledRingSecondsG);
  EEPROM.write(EEPROM_LEDRING_SECONDS_B_ADDR, ledRingSecondsB);
  EEPROM.write(EEPROM_LEDRING_DIGITAL_R_ADDR, ledRingDigitalR);
  EEPROM.write(EEPROM_LEDRING_DIGITAL_G_ADDR, ledRingDigitalG);
  EEPROM.write(EEPROM_LEDRING_DIGITAL_B_ADDR, ledRingDigitalB);
  EEPROM.write(EEPROM_LEDRING_HOURS_RAINBOW, ledRingHoursRainbow ? 1 : 0);
  EEPROM.write(EEPROM_LEDRING_MINUTES_RAINBOW, ledRingMinutesRainbow ? 1 : 0);
  EEPROM.write(EEPROM_LEDRING_SECONDS_RAINBOW, ledRingSecondsRainbow ? 1 : 0);
  EEPROM.write(EEPROM_LEDRING_VALID_ADDR, EEPROM_LEDRING_VALID_MARKER);
  EEPROM.commit();
  Serial.println("[LED Ring] Config salvata");
  ledRingFirstDraw = true;
}

// Setup webserver
void setupLedRingWebServer(AsyncWebServer &server) {

  // Pagina principale
  server.on("/ledring", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", LEDRING_HTML);
  });

  // GET config
  server.on("/ledring/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"hoursColor\":\"" + rgbToHexLedRing(ledRingHoursR, ledRingHoursG, ledRingHoursB) + "\",";
    json += "\"minutesColor\":\"" + rgbToHexLedRing(ledRingMinutesR, ledRingMinutesG, ledRingMinutesB) + "\",";
    json += "\"secondsColor\":\"" + rgbToHexLedRing(ledRingSecondsR, ledRingSecondsG, ledRingSecondsB) + "\",";
    json += "\"digitalColor\":\"" + rgbToHexLedRing(ledRingDigitalR, ledRingDigitalG, ledRingDigitalB) + "\",";
    json += "\"hoursRainbow\":" + String(ledRingHoursRainbow ? "true" : "false") + ",";
    json += "\"minutesRainbow\":" + String(ledRingMinutesRainbow ? "true" : "false") + ",";
    json += "\"secondsRainbow\":" + String(ledRingSecondsRainbow ? "true" : "false") + "}";
    request->send(200, "application/json", json);
  });

  // POST save
  server.on("/ledring/save", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body = "";
      for (size_t i = 0; i < len; i++) body += (char)data[i];

      Serial.println("[LED Ring] Ricevuto POST /ledring/save");
      Serial.println("[LED Ring] Body: " + body);

      int idx = body.indexOf("\"hoursColor\":\"");
      if (idx != -1) hexToRgbLedRing(body.substring(idx + 14, idx + 21).c_str(), ledRingHoursR, ledRingHoursG, ledRingHoursB);

      idx = body.indexOf("\"minutesColor\":\"");
      if (idx != -1) hexToRgbLedRing(body.substring(idx + 16, idx + 23).c_str(), ledRingMinutesR, ledRingMinutesG, ledRingMinutesB);

      idx = body.indexOf("\"secondsColor\":\"");
      if (idx != -1) hexToRgbLedRing(body.substring(idx + 16, idx + 23).c_str(), ledRingSecondsR, ledRingSecondsG, ledRingSecondsB);

      idx = body.indexOf("\"digitalColor\":\"");
      if (idx != -1) hexToRgbLedRing(body.substring(idx + 16, idx + 23).c_str(), ledRingDigitalR, ledRingDigitalG, ledRingDigitalB);

      ledRingHoursRainbow = body.indexOf("\"hoursRainbow\":true") != -1;
      ledRingMinutesRainbow = body.indexOf("\"minutesRainbow\":true") != -1;
      ledRingSecondsRainbow = body.indexOf("\"secondsRainbow\":true") != -1;

      Serial.printf("[LED Ring] Parsed: Ore RGB(%d,%d,%d) Rain=%d, Min RGB(%d,%d,%d) Rain=%d, Sec RGB(%d,%d,%d) Rain=%d\n",
                    ledRingHoursR, ledRingHoursG, ledRingHoursB, ledRingHoursRainbow,
                    ledRingMinutesR, ledRingMinutesG, ledRingMinutesB, ledRingMinutesRainbow,
                    ledRingSecondsR, ledRingSecondsG, ledRingSecondsB, ledRingSecondsRainbow);

      saveLedRingConfig();
      request->send(200, "application/json", "{\"ok\":true}");
    }
  );

  // POST reset
  server.on("/ledring/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    ledRingHoursR = 255; ledRingHoursG = 100; ledRingHoursB = 0;
    ledRingMinutesR = 0; ledRingMinutesG = 150; ledRingMinutesB = 255;
    ledRingSecondsR = 0; ledRingSecondsG = 255; ledRingSecondsB = 255;
    ledRingDigitalR = 0; ledRingDigitalG = 255; ledRingDigitalB = 200;
    ledRingHoursRainbow = false;
    ledRingMinutesRainbow = false;
    ledRingSecondsRainbow = false;
    saveLedRingConfig();
    request->send(200, "application/json", "{\"ok\":true}");
  });

  Serial.println("[LED Ring] WebServer /ledring OK");
}

#endif
