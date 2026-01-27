// ================== HOME PAGE HTML ==================
// Pagina principale con monitor SPETTACOLARE del display
// Griglia 16x16 allineata con word_mappings.h
// Accesso: http://<IP_ESP32>:8080/

const char HOME_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>OraQuadra Nano</title>
  <style>
    *{margin:0;padding:0;box-sizing:border-box}
    @keyframes gradient{0%,100%{background-position:0% 50%}50%{background-position:100% 50%}}
    @keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}
    @keyframes scanline{0%{top:-100%}100%{top:100%}}
    @keyframes flicker{0%,100%{opacity:1}92%{opacity:1}93%{opacity:0.8}94%{opacity:1}97%{opacity:0.9}98%{opacity:1}}
    @keyframes float{0%,100%{transform:translateY(0)}50%{transform:translateY(-10px)}}
    @keyframes matrixFall{0%{transform:translateY(-100%);opacity:1}100%{transform:translateY(1000%);opacity:0}}
    @keyframes fireFlicker{0%,100%{transform:scaleY(1);opacity:0.8}50%{transform:scaleY(1.1);opacity:1}}
    @keyframes snowFall{0%{transform:translateY(-10px) rotate(0deg);opacity:1}100%{transform:translateY(400px) rotate(360deg);opacity:0.3}}
    @keyframes neonPulse{0%,100%{text-shadow:0 0 5px var(--glow-color),0 0 10px var(--glow-color),0 0 20px var(--glow-color),0 0 40px var(--glow-color)}50%{text-shadow:0 0 10px var(--glow-color),0 0 20px var(--glow-color),0 0 40px var(--glow-color),0 0 80px var(--glow-color)}}
    @keyframes borderGlow{0%,100%{box-shadow:0 0 5px var(--glow-color),inset 0 0 5px var(--glow-color)}50%{box-shadow:0 0 20px var(--glow-color),0 0 40px var(--glow-color),inset 0 0 10px var(--glow-color)}}

    :root{--glow-color:#00ff00;--bg-dark:#0a0a1a;--bg-card:rgba(20,20,40,0.8)}

    body{
      font-family:'Segoe UI',system-ui,sans-serif;
      background:var(--bg-dark);
      min-height:100vh;
      color:#fff;
      overflow-x:hidden
    }
    .bg-animation{
      position:fixed;top:0;left:0;width:100%;height:100%;z-index:-1;
      background:linear-gradient(125deg,#0a0a1a,#1a1a3a,#0a1a2a,#1a0a2a);
      background-size:400% 400%;animation:gradient 15s ease infinite
    }
    .stars{position:fixed;top:0;left:0;width:100%;height:100%;z-index:-1;overflow:hidden}
    .star{position:absolute;width:2px;height:2px;background:#fff;border-radius:50%;animation:pulse 2s ease-in-out infinite}

    .container{max-width:1000px;margin:0 auto;padding:15px}
    .header{text-align:center;padding:20px 10px}
    .logo{
      font-size:2.5rem;font-weight:900;letter-spacing:-2px;
      background:linear-gradient(90deg,#00d9ff,#ff00ff,#00ff00,#ffff00,#00d9ff);
      background-size:300% auto;-webkit-background-clip:text;-webkit-text-fill-color:transparent;
      animation:gradient 3s linear infinite;margin-bottom:5px
    }
    .subtitle{color:#64748b;font-size:0.9rem;letter-spacing:3px;text-transform:uppercase}

    .monitor-container{perspective:1000px;margin-bottom:30px}
    .monitor{
      background:linear-gradient(145deg,#1a1a2e,#0f0f1a);border-radius:25px;padding:20px;position:relative;
      transform:rotateX(2deg);
      box-shadow:0 30px 60px rgba(0,0,0,0.5),0 0 0 1px rgba(255,255,255,0.1),inset 0 1px 0 rgba(255,255,255,0.1);
      transition:transform 0.3s ease
    }
    .monitor:hover{transform:rotateX(0deg) scale(1.01)}
    .monitor-frame{background:#000;border-radius:15px;padding:8px;position:relative;overflow:hidden;box-shadow:inset 0 0 50px rgba(0,0,0,0.8)}
    .monitor-frame::before{content:'';position:absolute;top:0;left:0;right:0;bottom:0;background:linear-gradient(180deg,rgba(255,255,255,0.03) 0%,transparent 50%);pointer-events:none;z-index:10}

    .screen{
      background:linear-gradient(180deg,#0a0a0a,#111,#0a0a0a);border-radius:10px;
      aspect-ratio:1;max-width:480px;margin:0 auto;position:relative;overflow:hidden;
      box-shadow:inset 0 0 100px rgba(0,0,0,0.9),0 0 1px rgba(255,255,255,0.1);animation:flicker 10s infinite
    }
    .crt-overlay{position:absolute;top:0;left:0;right:0;bottom:0;pointer-events:none;z-index:5}
    .scanlines{background:repeating-linear-gradient(0deg,transparent,transparent 2px,rgba(0,0,0,0.3) 2px,rgba(0,0,0,0.3) 4px);opacity:0.3}
    .vignette{background:radial-gradient(ellipse at center,transparent 0%,rgba(0,0,0,0.4) 80%,rgba(0,0,0,0.8) 100%)}
    .scanline-moving{position:absolute;width:100%;height:10px;background:linear-gradient(180deg,transparent,rgba(255,255,255,0.05),transparent);animation:scanline 8s linear infinite;opacity:0.5}
    .screen-glare{position:absolute;top:-50%;left:-50%;width:200%;height:200%;background:radial-gradient(ellipse at 30% 30%,rgba(255,255,255,0.1) 0%,transparent 50%);pointer-events:none}

    /* Word Grid 16x16 */
    .word-grid{
      display:grid;grid-template-columns:repeat(16,1fr);gap:1px;padding:8px;height:100%;align-content:center;position:relative;z-index:2
    }
    .word-grid span{
      aspect-ratio:1;display:flex;align-items:center;justify-content:center;
      font-family:'Courier New',monospace;font-size:clamp(8px,2vw,14px);font-weight:bold;
      color:rgba(50,50,50,0.5);border-radius:2px;transition:all 0.5s cubic-bezier(0.4,0,0.2,1);position:relative
    }
    .word-grid span.active{
      color:var(--glow-color);
      text-shadow:0 0 5px var(--glow-color),0 0 10px var(--glow-color),0 0 20px var(--glow-color),0 0 40px var(--glow-color);
      animation:neonPulse 2s ease-in-out infinite;transform:scale(1.05)
    }
    .word-grid span.active::after{
      content:'';position:absolute;top:50%;left:50%;width:150%;height:150%;
      background:radial-gradient(circle,var(--glow-color) 0%,transparent 70%);opacity:0.3;transform:translate(-50%,-50%);filter:blur(5px)
    }

    .matrix-rain{position:absolute;top:0;left:0;right:0;bottom:0;overflow:hidden;z-index:1;opacity:0.3}
    .matrix-drop{position:absolute;color:#00ff00;font-family:monospace;font-size:14px;text-shadow:0 0 10px #00ff00;animation:matrixFall linear infinite;opacity:0.8}

    .fire-container{position:absolute;bottom:0;left:0;right:0;height:100%;display:flex;justify-content:center;align-items:flex-end;overflow:hidden}
    .flame{position:absolute;bottom:-20px;width:60px;height:80px;background:linear-gradient(0deg,#ff6600,#ff9900,#ffcc00,transparent);border-radius:50% 50% 50% 50% / 60% 60% 40% 40%;filter:blur(3px);animation:fireFlicker 0.5s ease-in-out infinite alternate}

    .snow-container{position:absolute;top:0;left:0;right:0;bottom:0;overflow:hidden;pointer-events:none}
    .snowflake{position:absolute;top:-10px;color:#fff;font-size:12px;animation:snowFall linear infinite;opacity:0.8;text-shadow:0 0 5px #fff}

    .bttf-display{padding:20px;height:100%;display:flex;flex-direction:column;justify-content:center;gap:12px;font-family:'Courier New',monospace}
    .bttf-panel{background:linear-gradient(180deg,#1a0000 0%,#2a0000 50%,#1a0000 100%);border:3px solid;border-radius:8px;padding:12px;position:relative;overflow:hidden}
    .bttf-panel::before{content:'';position:absolute;top:0;left:0;right:0;height:50%;background:linear-gradient(180deg,rgba(255,255,255,0.1),transparent);pointer-events:none}
    .bttf-panel.dest{border-color:#ff0000;box-shadow:0 0 20px rgba(255,0,0,0.5),inset 0 0 30px rgba(255,0,0,0.2)}
    .bttf-panel.pres{border-color:#00ff00;box-shadow:0 0 20px rgba(0,255,0,0.5),inset 0 0 30px rgba(0,255,0,0.2)}
    .bttf-panel.last{border-color:#ff8800;box-shadow:0 0 20px rgba(255,136,0,0.5),inset 0 0 30px rgba(255,136,0,0.2)}
    .bttf-label{font-size:0.65rem;color:#666;margin-bottom:5px;letter-spacing:2px}
    .bttf-time{font-size:clamp(14px,4vw,22px);font-weight:bold;letter-spacing:3px;text-shadow:0 0 10px currentColor}
    .bttf-panel.dest .bttf-time{color:#ff0000}
    .bttf-panel.pres .bttf-time{color:#00ff00;animation:neonPulse 1s infinite}
    .bttf-panel.last .bttf-time{color:#ff8800}

    .analog-container{display:flex;align-items:center;justify-content:center;height:100%;padding:20px}
    .analog-clock{width:85%;aspect-ratio:1;border-radius:50%;background:radial-gradient(circle at 30% 30%,#333,#111),conic-gradient(from 0deg,#222,#111,#222,#111,#222,#111,#222,#111,#222,#111,#222,#111);border:6px solid #333;position:relative;box-shadow:0 0 30px rgba(0,0,0,0.8),inset 0 0 30px rgba(0,0,0,0.5),0 0 0 2px rgba(255,255,255,0.1)}
    .clock-markers{position:absolute;top:0;left:0;right:0;bottom:0}
    .clock-marker{position:absolute;width:2px;height:8px;background:#444;left:50%;top:5%;transform-origin:center 450%;border-radius:1px}
    .clock-marker.hour{height:12px;width:3px;background:#666}
    .hand{position:absolute;bottom:50%;left:50%;transform-origin:bottom center;border-radius:4px}
    .hand-hour{width:6px;height:22%;background:linear-gradient(180deg,#fff,#aaa);margin-left:-3px;box-shadow:0 0 10px rgba(255,255,255,0.5)}
    .hand-min{width:4px;height:32%;background:linear-gradient(180deg,#00d9ff,#0088aa);margin-left:-2px;box-shadow:0 0 15px rgba(0,217,255,0.8)}
    .hand-sec{width:2px;height:38%;background:#ff0000;margin-left:-1px;box-shadow:0 0 10px rgba(255,0,0,0.8)}
    .clock-center{position:absolute;width:16px;height:16px;background:radial-gradient(circle,#fff,#888);border-radius:50%;top:50%;left:50%;transform:translate(-50%,-50%);box-shadow:0 0 10px rgba(255,255,255,0.5);z-index:10}

    .flux-container{height:100%;display:flex;align-items:center;justify-content:center;position:relative}
    .flux-y{position:relative;width:70%;height:70%}
    .flux-arm{position:absolute;width:20px;height:45%;background:linear-gradient(90deg,#333,#555,#333);border-radius:10px;left:50%;transform-origin:bottom center}
    .flux-arm:nth-child(1){transform:translateX(-50%) rotate(0deg)}
    .flux-arm:nth-child(2){transform:translateX(-50%) rotate(120deg)}
    .flux-arm:nth-child(3){transform:translateX(-50%) rotate(240deg)}
    .flux-bulb{position:absolute;width:30px;height:30px;background:radial-gradient(circle,#fff,#ffff00,#ff8800);border-radius:50%;top:-15px;left:50%;transform:translateX(-50%);box-shadow:0 0 20px #ffff00,0 0 40px #ff8800,0 0 60px #ff4400;animation:pulse 0.3s ease-in-out infinite}
    .flux-center{position:absolute;width:50px;height:50px;background:radial-gradient(circle,#fff,#aaa);border-radius:50%;top:50%;left:50%;transform:translate(-50%,-50%);box-shadow:0 0 30px rgba(255,255,255,0.5)}
    .flux-glow{position:absolute;top:0;left:0;right:0;bottom:0;background:radial-gradient(circle at center,rgba(255,200,0,0.3),transparent 70%);animation:pulse 0.5s ease-in-out infinite}

    .special-display{height:100%;display:flex;flex-direction:column;align-items:center;justify-content:center;text-align:center;padding:30px;position:relative}
    .special-icon{font-size:5rem;margin-bottom:15px;animation:float 3s ease-in-out infinite;filter:drop-shadow(0 0 20px var(--glow-color))}
    .special-title{font-size:1.5rem;font-weight:bold;margin-bottom:8px;text-shadow:0 0 20px var(--glow-color)}
    .special-desc{color:#64748b;font-size:0.9rem}

    .ledring-display{height:100%;display:flex;align-items:center;justify-content:center}
    .ledring{width:80%;aspect-ratio:1;border-radius:50%;position:relative;background:#111}
    .ledring-track{position:absolute;border-radius:50%;border:8px solid transparent}
    .ledring-hours{top:5%;left:5%;right:5%;bottom:5%;border-color:#ff0000;box-shadow:0 0 15px #ff0000,inset 0 0 15px #ff0000;animation:borderGlow 2s infinite}
    .ledring-mins{top:15%;left:15%;right:15%;bottom:15%;border-color:#00ff00;box-shadow:0 0 15px #00ff00,inset 0 0 15px #00ff00;animation:borderGlow 2s infinite 0.3s}
    .ledring-secs{top:25%;left:25%;right:25%;bottom:25%;border-color:#0088ff;box-shadow:0 0 15px #0088ff,inset 0 0 15px #0088ff;animation:borderGlow 2s infinite 0.6s}
    .ledring-center{position:absolute;top:35%;left:35%;right:35%;bottom:35%;background:radial-gradient(circle,#222,#111);border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:clamp(12px,4vw,20px);font-weight:bold;color:#fff;text-shadow:0 0 10px #fff}

    .status-bar{display:flex;justify-content:center;gap:15px;margin-top:15px;flex-wrap:wrap}
    .status-item{background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);border-radius:12px;padding:10px 20px;text-align:center;backdrop-filter:blur(10px);transition:all 0.3s ease}
    .status-item:hover{background:rgba(255,255,255,0.1);transform:translateY(-2px)}
    .status-value{font-size:1.2rem;font-weight:bold;color:#fff}
    .status-label{font-size:0.7rem;color:#64748b;text-transform:uppercase;letter-spacing:1px;margin-top:3px}
    .color-preview{width:50px;height:25px;border-radius:6px;margin:0 auto;box-shadow:0 0 15px var(--glow-color)}
    .status-item.clickable{cursor:pointer;border:1px solid rgba(0,200,255,0.3)}
    .status-item.clickable:hover{background:rgba(0,200,255,0.2);border-color:rgba(0,200,255,0.6)}
    .random-btn{padding:3px 10px;border-radius:5px;font-size:0.9rem;transition:all 0.3s}
    .random-btn.on{background:linear-gradient(135deg,#00d4ff,#0099cc);color:#fff;box-shadow:0 0 15px rgba(0,212,255,0.5)}
    .random-btn.off{background:rgba(100,100,100,0.3);color:#888}

    .mode-badge{position:absolute;top:15px;right:15px;z-index:20;background:linear-gradient(135deg,rgba(233,69,96,0.3),rgba(233,69,96,0.1));border:1px solid rgba(233,69,96,0.5);color:#e94560;padding:8px 16px;border-radius:25px;font-size:0.85rem;font-weight:600;letter-spacing:1px;backdrop-filter:blur(10px);box-shadow:0 0 20px rgba(233,69,96,0.3)}

    .nav-section{margin-top:30px}
    .section-title{color:#64748b;font-size:0.75rem;text-transform:uppercase;letter-spacing:3px;margin-bottom:15px;padding-left:5px}
    .nav-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(220px,1fr));gap:12px}
    .nav-card{background:var(--bg-card);border:1px solid rgba(255,255,255,0.05);border-radius:15px;padding:18px;text-decoration:none;color:#fff;transition:all 0.4s cubic-bezier(0.4,0,0.2,1);position:relative;overflow:hidden}
    .nav-card::before{content:'';position:absolute;top:0;left:0;right:0;height:3px;background:var(--accent,#00d9ff);transform:scaleX(0);transition:transform 0.3s ease}
    .nav-card:hover::before{transform:scaleX(1)}
    .nav-card:hover{transform:translateY(-5px);box-shadow:0 20px 40px rgba(0,0,0,0.3),0 0 30px rgba(var(--accent-rgb,0,217,255),0.2);border-color:rgba(255,255,255,0.1)}
    .nav-card .icon{font-size:2rem;margin-bottom:10px;display:block}
    .nav-card h3{font-size:1rem;margin-bottom:5px}
    .nav-card p{color:#64748b;font-size:0.8rem;line-height:1.4}
    .nav-card.primary{--accent:#e94560;--accent-rgb:233,69,96}
    .nav-card.clock{--accent:#00d9ff;--accent-rgb:0,217,255}
    .nav-card.bttf{--accent:#ff9500;--accent-rgb:255,149,0}
    .nav-card.led{--accent:#4CAF50;--accent-rgb:76,175,80}
    .nav-card.flux{--accent:#ffeb3b;--accent-rgb:255,235,59}
    .nav-card.video{--accent:#9c27b0;--accent-rgb:156,39,176}
    .nav-card.audio{--accent:#2196F3;--accent-rgb:33,150,243}

    .footer{text-align:center;padding:25px;color:#475569;font-size:0.8rem;margin-top:30px;border-top:1px solid rgba(255,255,255,0.05)}
    .footer a{color:#00d9ff;text-decoration:none}
    .footer a:hover{text-decoration:underline}
  </style>
</head>
<body>
  <div class="bg-animation"></div>
  <div class="stars" id="stars"></div>

  <div class="container">
    <div class="header">
      <div class="logo">ORAQUADRA NANO</div>
      <div class="subtitle">Control Panel</div>
    </div>

    <div class="monitor-container">
      <div class="monitor">
        <div class="mode-badge" id="modeLabel">--</div>
        <div class="monitor-frame">
          <div class="screen" id="screen">
            <div class="word-grid" id="wordGrid"></div>
            <div class="crt-overlay scanlines"></div>
            <div class="crt-overlay vignette"></div>
            <div class="scanline-moving"></div>
            <div class="screen-glare"></div>
          </div>
        </div>
        <div class="status-bar">
          <div class="status-item">
            <div class="status-value" id="dispTime">--:--</div>
            <div class="status-label">Time</div>
          </div>
          <div class="status-item">
            <div class="color-preview" id="dispColor"></div>
            <div class="status-label">Color</div>
          </div>
          <div class="status-item">
            <div class="status-value" id="dispBright">--</div>
            <div class="status-label">Brightness</div>
          </div>
          <div class="status-item" id="tempStatusItem">
            <div class="status-value" id="dispTemp">--.-°</div>
            <div class="status-label">Temp</div>
          </div>
          <div class="status-item clickable" id="randomToggle" onclick="toggleRandom()">
            <div class="status-value random-btn" id="randomStatus">OFF</div>
            <div class="status-label">Random</div>
          </div>
        </div>
      </div>
    </div>

    <div class="nav-section">
      <div class="section-title">Configuration</div>
      <div class="nav-grid">
        <a href="/settings" class="nav-card primary">
          <span class="icon">&#9881;</span>
          <h3>Settings</h3>
          <p>Display modes, colors, brightness</p>
        </a>
        <a href="/clock" class="nav-card clock" id="card-clock" style="display:none">
          <span class="icon">&#128339;</span>
          <h3>Analog Clock</h3>
          <p>Hands colors and skins</p>
        </a>
        <a href="/bttf" class="nav-card bttf" id="card-bttf" style="display:none">
          <span class="icon">&#128663;</span>
          <h3>BTTF Time Circuits</h3>
          <p>Back to the Future display</p>
        </a>
        <a href="/ledring" class="nav-card led" id="card-ledring" style="display:none">
          <span class="icon">&#128308;</span>
          <h3>LED Ring</h3>
          <p>External LED ring colors</p>
        </a>
        <a href="/fluxcap" class="nav-card flux" id="card-flux" style="display:none">
          <span class="icon">&#9889;</span>
          <h3>Flux Capacitor</h3>
          <p>Flux capacitor effect</p>
        </a>
        <a href="/espcam" class="nav-card video" id="card-espcam" style="display:none">
          <span class="icon">&#127909;</span>
          <h3>ESP32-CAM</h3>
          <p>Camera streaming</p>
        </a>
        <a href="/mjpeg" class="nav-card video" id="card-mjpeg" style="display:none">
          <span class="icon">&#127902;</span>
          <h3>MJPEG Stream</h3>
          <p>External video stream</p>
        </a>
        <a href="/btaudio" class="nav-card audio" id="card-btaudio" style="display:none">
          <span class="icon">&#127925;</span>
          <h3>Bluetooth Audio</h3>
          <p>Audio output settings</p>
        </a>
      </div>
    </div>

    <div class="footer">
      <p>OraQuadra Nano v1.5 &bull; <a href="https://github.com/paolosambi/ORAQUADRA_NANO_v1.5" target="_blank">GitHub</a></p>
      <p style="margin-top:8px"><span id="ipAddr">--</span> &bull; Port 8080 &bull; Uptime: <span id="uptime">--</span></p>
    </div>
  </div>

  <script>
    // Stars
    const starsContainer = document.getElementById('stars');
    for(let i=0;i<100;i++){
      const star = document.createElement('div');
      star.className = 'star';
      star.style.left = Math.random()*100+'%';
      star.style.top = Math.random()*100+'%';
      star.style.animationDelay = Math.random()*2+'s';
      star.style.opacity = Math.random()*0.5+0.2;
      starsContainer.appendChild(star);
    }

    // Grid 16x16 da word_mappings.h
    const TFT_L = "SONOULEYOREXZEROVENTITREDICIOTTOECQUATTORDICISEINIUNDICIQUATTROOTNIJVENTUNODIECIIQNSEDICIASSETTEDUDODICIANNOVELFUEIHELPQUARANTAXERCKUVENTITRENTAGRINCINQUANTAUNOSEDICIDODICIOTTODIECIQUATTORDICIQUATTROQUINDICIOARTREDICIASSETTEUNDICIANNOVEOSEICINQUEDUEUMINUTI";

    // Mappatura parole da word_mappings.h
    const WORDS = {
      SONO_LE: [0,1,2,3,5,6,8,9,10],
      E: [116],
      MINUTI: [250,251,252,253,254,255],
      // Ore 0-23 (formato 24h da word_mappings.h)
      ZERO: [12,13,14,15],
      UNA: [57,73,89],
      DUE: [96,112,128],
      TRE: [21,22,23],
      QUATTRO: [56,57,58,59,60,61,62],
      CINQUE_H: [33,49,65,81,97,113],
      SEI: [45,46,47],
      SETTE: [91,92,93,94,95],
      OTTO: [28,29,30,31],
      NOVE: [106,107,108,109],
      DIECI_H: [75,76,77,78,79],
      UNDICI: [50,51,52,53,54,55],
      DODICI: [98,99,100,101,102,103],
      TREDICI: [21,22,23,24,25,26,27],
      QUATTORDICI: [34,35,36,37,38,39,40,41,42,43,44],
      QUINDICI: [34,50,66,82,98,114,130,146],
      SEDICI: [83,84,85,86,87,88],
      DICIASSETTE: [85,86,87,88,89,90,91,92,93,94,95],
      DICIOTTO: [24,25,26,27,28,29,30,31],
      DICIANNOVE: [100,101,102,103,104,105,106,107,108,109],
      VENTI: [16,32,48,64,80],
      VENTUNO: [68,69,70,71,72,73,74],
      VENTIDUE: [16,32,48,64,80,96,112,128],
      VENTITRE: [16,17,18,19,20,21,22,23],
      // Minuti
      MUNO: [157,158,159],
      MDUE: [246,247,248],
      MTRE: [210,211,212],
      MQUATTRO: [192,193,194,195,196,197,198],
      MCINQUE: [240,241,242,243,244,245],
      MSEI: [237,238,239],
      MSETTE: [219,220,221,222,223],
      MOTTO: [172,173,174,175],
      MNOVE: [232,233,234,235],
      MDIECI: [176,177,178,179,180],
      MUNDICI: [224,225,226,227,228,229],
      MDODICI: [166,167,168,169,170,171],
      MVENTI: [133,134,135,136,137],
      MTRENTA: [138,139,140,141,142,143],
      MQUARANTA: [119,120,121,122,123,124,125,126],
      MCINQUANTA: [148,149,150,151,152,153,154,155,156]
    };

    const MODES = ['Fade','Slow','Fast','Matrix','Matrix2','Snake','Water','Mario',
                   'Tron','Galaga','Flux','FlipClock','BTTF','LED Ring','Weather','Radar',
                   'Gemini','Galaga2','MJPEG','ESP32CAM','FluxCap','Christmas','Fire','FireText'];

    let currentMode = 0;
    let currentColor = '#00ff00';

    function initWordGrid(){
      const grid = document.getElementById('wordGrid');
      if(!grid) return;
      grid.innerHTML = '';
      for(let i=0;i<256;i++){
        const span = document.createElement('span');
        span.textContent = TFT_L[i] || ' ';
        span.dataset.idx = i;
        grid.appendChild(span);
      }
    }

    function updateWordClock(h,m){
      const spans = document.querySelectorAll('.word-grid span');
      if(spans.length !== 256) return;
      spans.forEach(s => s.classList.remove('active'));

      // SONO LE
      WORDS.SONO_LE.forEach(i => spans[i]?.classList.add('active'));

      // Ora (formato 24h completo)
      const hourWords = {
        0: WORDS.ZERO, 1: WORDS.UNA, 2: WORDS.DUE, 3: WORDS.TRE,
        4: WORDS.QUATTRO, 5: WORDS.CINQUE_H, 6: WORDS.SEI, 7: WORDS.SETTE,
        8: WORDS.OTTO, 9: WORDS.NOVE, 10: WORDS.DIECI_H, 11: WORDS.UNDICI,
        12: WORDS.DODICI, 13: WORDS.TREDICI, 14: WORDS.QUATTORDICI, 15: WORDS.QUINDICI,
        16: WORDS.SEDICI, 17: WORDS.DICIASSETTE, 18: WORDS.DICIOTTO, 19: WORDS.DICIANNOVE,
        20: WORDS.VENTI, 21: WORDS.VENTUNO, 22: WORDS.VENTIDUE, 23: WORDS.VENTITRE
      };
      if(hourWords[h]) hourWords[h].forEach(i => spans[i]?.classList.add('active'));

      // Minuti
      if(m >= 1 && m <= 59){
        WORDS.E.forEach(i => spans[i]?.classList.add('active'));
        WORDS.MINUTI.forEach(i => spans[i]?.classList.add('active'));

        // Unita
        const units = m % 10;
        const tens = Math.floor(m / 10);

        // Decine
        if(tens === 2) WORDS.MVENTI.forEach(i => spans[i]?.classList.add('active'));
        else if(tens === 3) WORDS.MTRENTA.forEach(i => spans[i]?.classList.add('active'));
        else if(tens === 4) WORDS.MQUARANTA.forEach(i => spans[i]?.classList.add('active'));
        else if(tens === 5) WORDS.MCINQUANTA.forEach(i => spans[i]?.classList.add('active'));

        // Unita o numeri 1-19
        if(m <= 19){
          const minWords = [null,WORDS.MUNO,WORDS.MDUE,WORDS.MTRE,WORDS.MQUATTRO,WORDS.MCINQUE,
            WORDS.MSEI,WORDS.MSETTE,WORDS.MOTTO,WORDS.MNOVE,WORDS.MDIECI,WORDS.MUNDICI,WORDS.MDODICI];
          if(minWords[m]) minWords[m].forEach(i => spans[i]?.classList.add('active'));
        } else if(units > 0){
          const unitWords = [null,WORDS.MUNO,WORDS.MDUE,WORDS.MTRE,WORDS.MQUATTRO,WORDS.MCINQUE,
            WORDS.MSEI,WORDS.MSETTE,WORDS.MOTTO,WORDS.MNOVE];
          if(unitWords[units]) unitWords[units].forEach(i => spans[i]?.classList.add('active'));
        }
      }
    }

    function createMatrixRain(){
      const screen = document.getElementById('screen');
      let rain = screen.querySelector('.matrix-rain');
      if(!rain){
        rain = document.createElement('div');
        rain.className = 'matrix-rain';
        screen.insertBefore(rain, screen.firstChild);
      }
      if(rain.children.length > 0) return;
      for(let i=0;i<20;i++){
        const drop = document.createElement('div');
        drop.className = 'matrix-drop';
        drop.style.left = (i*5+Math.random()*5)+'%';
        drop.style.animationDuration = (2+Math.random()*3)+'s';
        drop.style.animationDelay = Math.random()*2+'s';
        drop.textContent = String.fromCharCode(0x30A0+Math.random()*96);
        rain.appendChild(drop);
      }
    }

    function showBTTF(h,m){
      const screen = document.getElementById('screen');
      const now = new Date();
      const dateStr = now.toLocaleDateString('en-US',{month:'short',day:'2-digit',year:'numeric'}).toUpperCase();
      screen.innerHTML = `
        <div class="bttf-display">
          <div class="bttf-panel dest">
            <div class="bttf-label">DESTINATION TIME</div>
            <div class="bttf-time">OCT 26 1985  01:21 AM</div>
          </div>
          <div class="bttf-panel pres">
            <div class="bttf-label">PRESENT TIME</div>
            <div class="bttf-time">${dateStr}  ${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')} ${h>=12?'PM':'AM'}</div>
          </div>
          <div class="bttf-panel last">
            <div class="bttf-label">LAST TIME DEPARTED</div>
            <div class="bttf-time">NOV 05 1955  06:15 AM</div>
          </div>
        </div>
        <div class="crt-overlay scanlines"></div><div class="crt-overlay vignette"></div>`;
    }

    function showAnalog(h,m,s){
      const screen = document.getElementById('screen');
      const hDeg = (h%12)*30 + m*0.5;
      const mDeg = m*6;
      const sDeg = s*6;
      let markers = '';
      for(let i=0;i<12;i++) markers += '<div class="clock-marker'+(i%3===0?' hour':'')+'" style="transform:rotate('+i*30+'deg)"></div>';
      screen.innerHTML = `
        <div class="analog-container">
          <div class="analog-clock">
            <div class="clock-markers">${markers}</div>
            <div class="hand hand-hour" style="transform:rotate(${hDeg}deg)"></div>
            <div class="hand hand-min" style="transform:rotate(${mDeg}deg)"></div>
            <div class="hand hand-sec" style="transform:rotate(${sDeg}deg)"></div>
            <div class="clock-center"></div>
          </div>
        </div><div class="crt-overlay vignette"></div>`;
    }

    function showLedRing(h,m){
      const screen = document.getElementById('screen');
      screen.innerHTML = `
        <div class="ledring-display">
          <div class="ledring">
            <div class="ledring-track ledring-hours"></div>
            <div class="ledring-track ledring-mins"></div>
            <div class="ledring-track ledring-secs"></div>
            <div class="ledring-center">${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}</div>
          </div>
        </div><div class="crt-overlay vignette"></div>`;
    }

    function showFluxCapacitor(){
      const screen = document.getElementById('screen');
      screen.innerHTML = `
        <div class="flux-container">
          <div class="flux-glow"></div>
          <div class="flux-y">
            <div class="flux-arm"><div class="flux-bulb"></div></div>
            <div class="flux-arm"><div class="flux-bulb"></div></div>
            <div class="flux-arm"><div class="flux-bulb"></div></div>
            <div class="flux-center"></div>
          </div>
        </div><div class="crt-overlay vignette"></div>`;
    }

    function showFire(){
      const screen = document.getElementById('screen');
      screen.innerHTML = `
        <div class="fire-container">
          ${Array(15).fill(0).map((_,i)=>`<div class="flame" style="left:${5+i*6}%;animation-delay:${Math.random()*0.5}s;height:${60+Math.random()*40}px"></div>`).join('')}
        </div><div class="crt-overlay vignette"></div>`;
      screen.style.background = 'linear-gradient(0deg,#ff4400,#220000)';
    }

    function showChristmas(){
      const screen = document.getElementById('screen');
      let snow = '';
      for(let i=0;i<30;i++) snow += `<div class="snowflake" style="left:${Math.random()*100}%;animation-duration:${3+Math.random()*4}s;animation-delay:${Math.random()*3}s">❄</div>`;
      screen.innerHTML = `
        <div class="special-display" style="background:linear-gradient(180deg,#001428,#002244)">
          <div class="snow-container">${snow}</div>
          <div class="special-icon" style="filter:none">🎄</div>
          <div class="special-title" style="color:#ff0000">Merry Christmas!</div>
          <div class="special-desc" style="color:#88ff88">❄ ❄ ❄</div>
        </div><div class="crt-overlay vignette"></div>`;
    }

    function showSpecial(icon, title, desc){
      const screen = document.getElementById('screen');
      screen.innerHTML = `
        <div class="special-display">
          <div class="special-icon">${icon}</div>
          <div class="special-title">${title}</div>
          <div class="special-desc">${desc}</div>
        </div><div class="crt-overlay scanlines"></div><div class="crt-overlay vignette"></div>`;
    }

    function updateDisplay(data){
      currentMode = data.displayMode || 0;
      currentColor = data.customColor || '#00ff00';
      const h = data.currentHour || 0;
      const m = data.currentMinute || 0;
      const s = data.currentSecond || 0;

      document.documentElement.style.setProperty('--glow-color', currentColor);
      document.getElementById('modeLabel').textContent = MODES[currentMode] || 'Mode '+currentMode;
      document.getElementById('dispTime').textContent = String(h).padStart(2,'0')+':'+String(m).padStart(2,'0');
      document.getElementById('dispColor').style.background = currentColor;
      document.getElementById('dispColor').style.boxShadow = '0 0 15px '+currentColor;

      // Brightness: mostra valore assoluto (0-255) come nella pagina settings
      let bright = data.displayBrightness || data.brightnessDay || 0;
      document.getElementById('dispBright').textContent = bright;

      // Temperatura: nascondi se sensore BME280 non disponibile
      const tempItem = document.getElementById('tempStatusItem');
      if(data.bme280Available){
        tempItem.style.display = '';
        document.getElementById('dispTemp').textContent = data.tempIndoor ? data.tempIndoor.toFixed(1)+'°' : '--.-°';
      }else{
        tempItem.style.display = 'none';
      }

      // Random Mode status
      const randomBtn = document.getElementById('randomStatus');
      if(data.randomModeEnabled){
        randomBtn.textContent = 'ON';
        randomBtn.className = 'status-value random-btn on';
      }else{
        randomBtn.textContent = 'OFF';
        randomBtn.className = 'status-value random-btn off';
      }

      const screen = document.getElementById('screen');
      screen.style.background = '';

      // Word Clock (0-10, 17)
      if(currentMode <= 10 || currentMode === 17){
        if(!screen.querySelector('.word-grid')){
          screen.innerHTML = '<div class="word-grid" id="wordGrid"></div><div class="crt-overlay scanlines"></div><div class="crt-overlay vignette"></div><div class="scanline-moving"></div><div class="screen-glare"></div>';
          initWordGrid();
        }
        if(currentMode === 3 || currentMode === 4) createMatrixRain();
        else{const rain = screen.querySelector('.matrix-rain'); if(rain) rain.remove();}
        updateWordClock(h, m);
      }
      else if(currentMode === 11 || currentMode === 15) showAnalog(h, m, s);
      else if(currentMode === 12) showBTTF(h, m);
      else if(currentMode === 13) showLedRing(h, m);
      else if(currentMode === 14) showSpecial('🌤️', 'Weather Station', 'Temp: '+(data.tempIndoor?.toFixed(1)||'--')+'°C');
      else if(currentMode === 18 || currentMode === 19) showSpecial('📹', 'Video Stream', 'Streaming Active');
      else if(currentMode === 20) showFluxCapacitor();
      else if(currentMode === 21) showChristmas();
      else if(currentMode === 22 || currentMode === 23) showFire();
      else showSpecial('⏰', MODES[currentMode]||'Mode '+currentMode, '');
    }

    async function loadStatus(){
      try{
        const r = await fetch('/settings/status');
        const d = await r.json();
        updateDisplay(d);
        document.getElementById('ipAddr').textContent = location.hostname;
        const up = d.uptime || 0;
        document.getElementById('uptime').textContent = Math.floor(up/3600)+'h '+Math.floor((up%3600)/60)+'m';
      }catch(e){console.error(e)}
    }

    async function checkPages(){
      const pages = ['clock','bttf','ledring','fluxcap','espcam','mjpeg','btaudio'];
      for(const p of pages){
        try{
          const r = await fetch('/'+p,{method:'HEAD'});
          if(r.ok) document.getElementById('card-'+(p==='fluxcap'?'flux':p)).style.display = 'block';
        }catch(e){}
      }
    }

    async function toggleRandom(){
      const btn = document.getElementById('randomStatus');
      const isOn = btn.classList.contains('on');
      const newVal = isOn ? 0 : 1;
      try{
        await fetch('/settings/save?randomModeEnabled='+newVal);
        btn.textContent = newVal ? 'ON' : 'OFF';
        btn.className = 'status-value random-btn '+(newVal ? 'on' : 'off');
      }catch(e){console.error(e)}
    }

    initWordGrid();
    loadStatus();
    checkPages();
    setInterval(loadStatus, 1500);
  </script>
</body>
</html>
)rawliteral";
