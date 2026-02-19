// ================== PONG GAME HTML ==================
// Pagina istruzioni e stato del gioco PONG Dual Display
// Accessibile su /pong

const char PONG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>PONG - OraQuadra Nano</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{--bg:#0a0a0a;--card:#141414;--accent:#00cc00;--text:#fff;--dim:#999;--border:rgba(255,255,255,0.08)}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);min-height:100vh;padding:16px}
.container{max-width:600px;margin:0 auto}
.header{text-align:center;padding:24px 0 16px}
.header h1{font-size:2.2em;font-weight:800;letter-spacing:2px;color:var(--accent)}
.header .subtitle{color:var(--dim);font-size:.95em;margin-top:4px}
.back{display:inline-block;color:var(--accent);text-decoration:none;font-size:.9em;margin-bottom:12px;opacity:.8}
.back:hover{opacity:1}
.card{background:var(--card);border:1px solid var(--border);border-radius:14px;padding:20px;margin-bottom:16px}
.card h2{font-size:1.1em;margin-bottom:12px;color:var(--accent);display:flex;align-items:center;gap:8px}
.card h2 .icon{font-size:1.3em}
.card p,.card li{color:var(--dim);font-size:.92em;line-height:1.6}
.card ul{padding-left:20px;margin-top:8px}
.card li{margin-bottom:6px}
.card li strong{color:var(--text)}
.field{display:flex;justify-content:center;align-items:center;margin:20px 0;position:relative;height:200px;background:#000;border-radius:10px;border:1px solid var(--border);overflow:hidden}
.field .line{position:absolute;left:50%;top:0;width:2px;height:100%;background:repeating-linear-gradient(to bottom,transparent 0,transparent 8px,#333 8px,#333 16px)}
.field .paddle{position:absolute;width:6px;height:40px;background:var(--text);border-radius:3px}
.field .paddle.left{left:12px;top:50%;transform:translateY(-50%)}
.field .paddle.right{right:12px;top:40%;transform:translateY(-50%)}
.field .ball{position:absolute;width:8px;height:8px;background:var(--text);border-radius:50%;left:52%;top:45%;animation:pongBall 3s ease-in-out infinite alternate}
.field .score{position:absolute;top:10px;font-size:1.4em;font-weight:700;color:rgba(255,255,255,.3)}
.field .score.l{left:25%}
.field .score.r{right:25%}
@keyframes pongBall{0%{left:15%;top:30%}25%{left:48%;top:65%}50%{left:80%;top:25%}75%{left:48%;top:70%}100%{left:15%;top:45%}}
.status{text-align:center;padding:12px;border-radius:10px;font-weight:600;font-size:.95em}
.status.ok{background:rgba(0,204,0,.12);color:#00cc00;border:1px solid rgba(0,204,0,.2)}
.status.warn{background:rgba(255,165,0,.12);color:#ffa500;border:1px solid rgba(255,165,0,.2)}
.status.off{background:rgba(255,0,0,.12);color:#ff4444;border:1px solid rgba(255,0,0,.2)}
.btn{display:inline-block;padding:10px 24px;background:var(--accent);color:#000;border:none;border-radius:8px;font-weight:700;font-size:.95em;text-decoration:none;cursor:pointer;text-align:center;transition:opacity .2s}
.btn:hover{opacity:.85}
.btn-row{display:flex;gap:10px;justify-content:center;margin-top:16px;flex-wrap:wrap}
.controls{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:12px}
.ctrl{text-align:center;padding:14px;background:rgba(255,255,255,.04);border-radius:10px;border:1px solid var(--border)}
.ctrl .label{font-size:.8em;color:var(--dim);margin-bottom:4px}
.ctrl .value{font-size:1.1em;font-weight:700;color:var(--text)}
.sep{border:none;border-top:1px solid var(--border);margin:16px 0}
</style>
</head>
<body>
<div class="container">
  <a href="/" class="back">&larr; Home</a>
  <div class="header">
    <h1>&#127955; PONG</h1>
    <div class="subtitle">Classic Arcade - Dual Display</div>
  </div>

  <!-- Campo animato -->
  <div class="field">
    <div class="line"></div>
    <div class="paddle left"></div>
    <div class="paddle right"></div>
    <div class="ball"></div>
    <div class="score l">0</div>
    <div class="score r">0</div>
  </div>

  <!-- Status -->
  <div class="card">
    <h2><span class="icon">&#128225;</span> Stato</h2>
    <div id="statusBox" class="status warn">Caricamento...</div>
    <div class="controls">
      <div class="ctrl"><div class="label">Modalit&agrave;</div><div class="value" id="curMode">--</div></div>
      <div class="ctrl"><div class="label">Multi-Display</div><div class="value" id="dualStatus">--</div></div>
      <div class="ctrl"><div class="label">Punteggio</div><div class="value" id="score">- : -</div></div>
      <div class="ctrl"><div class="label">Ruolo</div><div class="value" id="role">--</div></div>
    </div>
    <div class="btn-row">
      <a href="/dualdisplay" class="btn">Configura Multi-Display</a>
    </div>
  </div>

  <!-- Istruzioni -->
  <div class="card">
    <h2><span class="icon">&#127918;</span> Come giocare</h2>
    <p>PONG richiede <strong>2 pannelli OraQuadra</strong> collegati via ESP-NOW in griglia 2&times;1 (960&times;480 pixel virtuali).</p>
    <hr class="sep">
    <ul>
      <li><strong>Pannello sinistro (Master)</strong>: tocca lo schermo per muovere la racchetta sinistra su/gi&ugrave;</li>
      <li><strong>Pannello destro (Slave)</strong>: tocca lo schermo per muovere la racchetta destra su/gi&ugrave;</li>
      <li>La pallina rimbalza sulle racchette e sui bordi superiore/inferiore</li>
      <li>Primo giocatore a <strong>5 punti</strong> vince la partita</li>
      <li>Dopo il Game Over, tocca lo schermo per ricominciare</li>
    </ul>
  </div>

  <!-- Setup -->
  <div class="card">
    <h2><span class="icon">&#128295;</span> Configurazione</h2>
    <p>Per attivare il gioco:</p>
    <ul>
      <li>Vai su <a href="/dualdisplay" style="color:var(--accent)">Multi-Display</a> e configura <strong>griglia 2&times;1</strong></li>
      <li><strong>Device 1</strong>: Ruolo = Master, Posizione = (0, 0)</li>
      <li><strong>Device 2</strong>: Ruolo = Slave, Posizione = (1, 0)</li>
      <li>Esegui la scansione e associa i pannelli</li>
      <li>Seleziona la modalit&agrave; <strong>PONG</strong> (modo 31)</li>
    </ul>
  </div>

  <!-- Comandi touch -->
  <div class="card">
    <h2><span class="icon">&#128400;</span> Comandi touch</h2>
    <ul>
      <li><strong>Tocca e trascina</strong>: muovi la racchetta (segue il dito in Y)</li>
      <li><strong>Angolo alto-sinistra</strong> (Master): cambia modalit&agrave;</li>
      <li><strong>Tocco durante Game Over</strong>: nuova partita</li>
    </ul>
  </div>

  <!-- Dettagli tecnici -->
  <div class="card">
    <h2><span class="icon">&#9881;</span> Dettagli tecnici</h2>
    <ul>
      <li>Canvas virtuale: 960&times;480 px (2 pannelli da 480&times;480)</li>
      <li>Frame rate: ~30 fps sincronizzati via ESP-NOW</li>
      <li>Racchette: 12&times;80 px, pallina: 12&times;12 px</li>
      <li>Il Master calcola tutta la fisica e invia lo stato allo Slave</li>
      <li>Il touch dello Slave viene inoltrato al Master via ESP-NOW</li>
      <li>Latenza touch: ~1-2 ms (ESP-NOW diretto)</li>
    </ul>
  </div>
</div>

<script>
async function loadStatus(){
  try{
    const r = await fetch('/pong/status');
    const d = await r.json();
    const sb = document.getElementById('statusBox');
    const isPong = d.mode === 28;
    const isDual = d.dualActive;

    document.getElementById('curMode').textContent = isPong ? 'PONG' : d.modeName || ('Modo ' + d.mode);
    document.getElementById('dualStatus').textContent = isDual ? (d.role === 1 ? 'Master' : 'Slave') : 'Disattivo';
    document.getElementById('role').textContent = d.role === 1 ? 'Master' : d.role === 2 ? 'Slave' : 'Standalone';
    document.getElementById('score').textContent = d.scoreL + ' : ' + d.scoreR;

    document.querySelector('.score.l').textContent = d.scoreL;
    document.querySelector('.score.r').textContent = d.scoreR;

    if (isPong && isDual) {
      sb.className = 'status ok';
      sb.textContent = 'Gioco PONG attivo' + (d.gameOver ? ' - GAME OVER' : '');
    } else if (isDual && !isPong) {
      sb.className = 'status warn';
      sb.textContent = 'Multi-Display attivo - seleziona modo PONG (31)';
    } else if (!isDual) {
      sb.className = 'status off';
      sb.textContent = 'Multi-Display non attivo - configura 2 pannelli';
    }
  } catch(e) {
    document.getElementById('statusBox').className = 'status off';
    document.getElementById('statusBox').textContent = 'Errore connessione';
  }
}
loadStatus();
setInterval(loadStatus, 3000);
</script>
</body>
</html>
)rawliteral";
