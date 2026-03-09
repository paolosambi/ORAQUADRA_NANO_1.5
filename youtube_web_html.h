// ================== YOUTUBE STATS HTML ==================
// Pagina web per visualizzare statistiche canale YouTube
// Accesso: http://<IP_ESP32>:8080/youtube

const char YOUTUBE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>YouTube Stats - OraQuadra Nano</title>
  <style>
    *{margin:0;padding:0;box-sizing:border-box}
    :root{--yt-red:#FF0000;--yt-dark:#0f0f0f;--yt-card:#1a1a2e;--yt-text:#fff;--yt-dim:#aaa}
    body{font-family:'Segoe UI',system-ui,sans-serif;background:var(--yt-dark);color:var(--yt-text);min-height:100vh}
    .container{max-width:600px;margin:0 auto;padding:20px}
    .header{text-align:center;padding:20px 0}
    .header .logo{display:inline-block;background:var(--yt-red);border-radius:12px;padding:12px 20px;margin-bottom:15px}
    .header .logo svg{width:48px;height:34px;fill:#fff}
    .header h1{font-size:1.6rem;margin-bottom:4px}
    .header .sub{color:var(--yt-dim);font-size:0.85rem}
    .channel-name{text-align:center;font-size:1.3rem;font-weight:700;margin:15px 0 25px;color:var(--yt-text)}
    .stats-grid{display:grid;grid-template-columns:1fr;gap:14px;margin-bottom:25px}
    .stat-card{background:var(--yt-card);border-radius:16px;padding:22px;text-align:center;border:1px solid rgba(255,255,255,0.06);transition:transform 0.2s}
    .stat-card:hover{transform:translateY(-3px)}
    .stat-card .value{font-size:2.4rem;font-weight:800;margin-bottom:4px}
    .stat-card .label{color:var(--yt-dim);font-size:0.85rem;text-transform:uppercase;letter-spacing:1px}
    .stat-card.subscribers .value{color:var(--yt-red)}
    .stat-card.views .value{color:#fff}
    .stat-card.videos .value{color:#FF8C00}
    .info-card{background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);border-radius:14px;padding:20px;margin-bottom:20px}
    .info-card h3{font-size:1rem;margin-bottom:10px;color:#FFD700}
    .info-card p,.info-card ol{color:var(--yt-dim);font-size:0.82rem;line-height:1.7}
    .info-card ol{padding-left:20px}
    .info-card code{background:rgba(255,255,255,0.1);padding:2px 6px;border-radius:4px;font-size:0.8rem}
    .error-msg{text-align:center;color:#ff6b6b;padding:15px;background:rgba(255,0,0,0.1);border-radius:10px;margin-bottom:15px;display:none}
    .status-bar{text-align:center;color:var(--yt-dim);font-size:0.75rem;margin-top:15px}
    .back-link{display:inline-block;margin-top:20px;color:#00d9ff;text-decoration:none;font-size:0.9rem}
    .back-link:hover{text-decoration:underline}
    .loading{text-align:center;padding:40px;color:var(--yt-dim)}
    .dot-pulse{display:inline-block;animation:pulse 1.5s infinite}
    @keyframes pulse{0%,100%{opacity:1}50%{opacity:0.3}}
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <div class="logo">
        <svg viewBox="0 0 68 48"><path d="M66.5 7.7c-.8-2.9-3-5.2-5.9-6C55.3 0 34 0 34 0S12.7 0 7.4 1.7c-2.9.8-5.2 3-5.9 6C0 13 0 24 0 24s0 11 1.5 16.3c.8 2.9 3 5.2 5.9 6C12.7 48 34 48 34 48s21.3 0 26.6-1.7c2.9-.8 5.2-3 5.9-6C68 35 68 24 68 24s0-11-1.5-16.3z" fill="#FF0000"/><path d="M27 34V14l18 10-18 10z" fill="#fff"/></svg>
      </div>
      <h1>YouTube Stats</h1>
      <div class="sub">Statistiche canale in tempo reale</div>
    </div>

    <div class="error-msg" id="errorMsg"></div>

    <div class="channel-name" id="channelName"><span class="dot-pulse">Caricamento...</span></div>

    <div class="stats-grid">
      <div class="stat-card subscribers">
        <div class="value" id="subs">--</div>
        <div class="label">Iscritti</div>
      </div>
      <div class="stat-card views">
        <div class="value" id="views">--</div>
        <div class="label">Visualizzazioni totali</div>
      </div>
      <div class="stat-card videos">
        <div class="value" id="videos">--</div>
        <div class="label">Video pubblicati</div>
      </div>
    </div>

    <div class="info-card" id="setupInfo">
      <h3>Come configurare</h3>
      <p>Per visualizzare le statistiche, serve una API Key YouTube Data v3:</p>
      <ol>
        <li>Vai su <code>console.cloud.google.com</code></li>
        <li>Crea un nuovo progetto (o seleziona uno esistente)</li>
        <li>Abilita "YouTube Data API v3" nella Libreria API</li>
        <li>Vai su Credenziali &rarr; Crea credenziali &rarr; Chiave API</li>
        <li>Copia la chiave e inseriscila nel file <code>39_YOUTUBE.ino</code></li>
        <li>Imposta anche il <code>YOUTUBE_CHANNEL_ID</code> del canale desiderato</li>
      </ol>
      <p style="margin-top:10px">Quota gratuita: 10.000 unita/giorno. Ogni richiesta usa 1 unita (fetch ogni 5 min = 288/giorno).</p>
    </div>

    <div class="status-bar">
      Ultimo aggiornamento: <span id="lastUpdate">--</span>
      <br><a href="/" class="back-link">&larr; Torna alla Home</a>
    </div>
  </div>

  <script>
    function fmtNum(n){
      if(n===null||n===undefined||n==='--') return '--';
      n=parseInt(n);
      if(isNaN(n)) return '--';
      if(n>=1000000) return (n/1000000).toFixed(1)+'M';
      if(n>=1000) return (n/1000).toFixed(1)+'K';
      return n.toLocaleString('it-IT');
    }

    async function loadStatus(){
      try{
        const r=await fetch('/youtube/status');
        const d=await r.json();
        if(d.error && d.error!==''){
          document.getElementById('errorMsg').textContent=d.error;
          document.getElementById('errorMsg').style.display='block';
          document.getElementById('setupInfo').style.display='block';
        } else {
          document.getElementById('errorMsg').style.display='none';
          if(d.subscribers>0) document.getElementById('setupInfo').style.display='none';
        }
        document.getElementById('channelName').textContent=d.channelName||'Canale non configurato';
        document.getElementById('subs').textContent=fmtNum(d.subscribers);
        document.getElementById('views').textContent=fmtNum(d.views);
        document.getElementById('videos').textContent=fmtNum(d.videos);
        document.getElementById('lastUpdate').textContent=d.lastUpdate||'--';
      }catch(e){
        console.error(e);
        document.getElementById('errorMsg').textContent='Errore connessione al dispositivo';
        document.getElementById('errorMsg').style.display='block';
      }
    }

    loadStatus();
    setInterval(loadStatus,60000);
  </script>
</body>
</html>
)rawliteral";
