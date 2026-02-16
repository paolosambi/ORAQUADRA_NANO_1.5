// ================== OTA UPDATE PAGE HTML ==================
// Pagina per aggiornamento firmware e LittleFS via browser
// Accesso: http://<IP_ESP32>:8080/update

const char OTA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>OTA Update - OraQuadra Nano</title>
  <style>
    :root{
      --ota-bg:#0f0f0f;
      --ota-card:#1a1a2e;
      --ota-accent:#00d9ff;
      --ota-text:#fff;
      --ota-dim:#aaa;
      --ota-success:#4CAF50;
      --ota-error:#e94560;
      --ota-border:rgba(255,255,255,0.06)
    }
    *{margin:0;padding:0;box-sizing:border-box}
    body{font-family:'Segoe UI',system-ui,sans-serif;background:var(--ota-bg);color:var(--ota-text);min-height:100vh;padding:20px}
    .container{max-width:600px;margin:0 auto}
    .header{text-align:center;margin-bottom:30px}
    .header h1{font-size:1.8rem;margin-bottom:5px;background:linear-gradient(90deg,var(--ota-accent),#ff00ff);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
    .header p{color:var(--ota-dim);font-size:0.85rem}
    a.back-link{display:inline-block;margin-bottom:20px;color:var(--ota-accent);text-decoration:none;font-size:0.9rem}
    a.back-link:hover{text-decoration:underline}

    .upload-section{background:var(--ota-card);border:1px solid var(--ota-border);border-radius:15px;padding:25px;margin-bottom:20px;position:relative;overflow:hidden}
    .upload-section::before{content:'';position:absolute;top:0;left:0;right:0;height:3px;background:var(--section-color,var(--ota-accent))}
    .upload-section h2{font-size:1.1rem;margin-bottom:5px;display:flex;align-items:center;gap:10px}
    .upload-section h2 .icon{font-size:1.5rem}
    .upload-section .desc{color:var(--ota-dim);font-size:0.8rem;margin-bottom:18px;line-height:1.5}
    .section-firmware{--section-color:#00d9ff}
    .section-littlefs{--section-color:#ff9500}

    .file-input-wrap{position:relative;margin-bottom:15px}
    .file-input-wrap input[type="file"]{width:100%;padding:12px;background:rgba(255,255,255,0.05);border:2px dashed rgba(255,255,255,0.15);border-radius:10px;color:var(--ota-text);font-size:0.9rem;cursor:pointer;transition:border-color 0.3s}
    .file-input-wrap input[type="file"]:hover{border-color:var(--section-color,var(--ota-accent))}
    .file-input-wrap input[type="file"]::file-selector-button{background:linear-gradient(135deg,var(--section-color,var(--ota-accent)),rgba(0,0,0,0.3));color:#fff;border:none;padding:8px 16px;border-radius:6px;cursor:pointer;margin-right:12px;font-size:0.85rem}

    .btn-upload{width:100%;padding:14px;border:none;border-radius:10px;font-size:1rem;font-weight:600;cursor:pointer;transition:all 0.3s;color:#fff;letter-spacing:0.5px}
    .btn-upload:disabled{opacity:0.4;cursor:not-allowed}
    .btn-firmware{background:linear-gradient(135deg,#0088cc,#00bbff)}
    .btn-firmware:not(:disabled):hover{box-shadow:0 0 25px rgba(0,187,255,0.4);transform:translateY(-1px)}
    .btn-littlefs{background:linear-gradient(135deg,#cc7700,#ff9500)}
    .btn-littlefs:not(:disabled):hover{box-shadow:0 0 25px rgba(255,149,0,0.4);transform:translateY(-1px)}

    .progress-wrap{display:none;margin-top:15px}
    .progress-bar{width:100%;height:28px;background:rgba(255,255,255,0.08);border-radius:14px;overflow:hidden;position:relative}
    .progress-fill{height:100%;border-radius:14px;transition:width 0.2s;width:0%}
    .progress-fill.firmware{background:linear-gradient(90deg,#0088cc,#00bbff)}
    .progress-fill.littlefs{background:linear-gradient(90deg,#cc7700,#ff9500)}
    .progress-text{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);font-size:0.8rem;font-weight:600;text-shadow:0 1px 3px rgba(0,0,0,0.5)}

    .status-msg{margin-top:12px;padding:10px 15px;border-radius:8px;font-size:0.85rem;display:none}
    .status-msg.success{display:block;background:rgba(76,175,80,0.15);border:1px solid rgba(76,175,80,0.3);color:var(--ota-success)}
    .status-msg.error{display:block;background:rgba(233,69,96,0.15);border:1px solid rgba(233,69,96,0.3);color:var(--ota-error)}
    .status-msg.info{display:block;background:rgba(0,217,255,0.1);border:1px solid rgba(0,217,255,0.2);color:var(--ota-accent)}

    .warning-box{background:rgba(255,149,0,0.1);border:1px solid rgba(255,149,0,0.25);border-radius:10px;padding:15px;margin-bottom:20px;font-size:0.8rem;color:#ff9500;line-height:1.6}
    .warning-box strong{display:block;margin-bottom:5px;font-size:0.9rem}

    .guide-section{background:var(--ota-card);border:1px solid var(--ota-border);border-radius:15px;padding:25px;margin-bottom:20px}
    .guide-section h2{font-size:1.1rem;margin-bottom:15px;display:flex;align-items:center;gap:10px}
    .guide-section h2 .icon{font-size:1.5rem}
    .guide-step{display:flex;gap:12px;margin-bottom:14px;align-items:flex-start}
    .guide-step:last-child{margin-bottom:0}
    .step-num{min-width:28px;height:28px;background:linear-gradient(135deg,var(--ota-accent),#0088cc);border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:0.8rem;font-weight:700;flex-shrink:0;margin-top:1px}
    .step-text{font-size:0.85rem;color:var(--ota-dim);line-height:1.5}
    .step-text strong{color:var(--ota-text)}
    .step-text code{background:rgba(255,255,255,0.08);padding:2px 6px;border-radius:4px;font-size:0.8rem;color:var(--ota-accent)}
    .guide-divider{border:none;border-top:1px solid var(--ota-border);margin:18px 0}
    .guide-sub{font-size:0.9rem;font-weight:600;color:var(--ota-text);margin-bottom:12px;padding-left:2px}

    .footer{text-align:center;padding:20px;color:#475569;font-size:0.75rem;margin-top:20px}
  </style>
</head>
<body>
  <div class="container">
    <a href="/" class="back-link">&larr; Torna alla Home</a>
    <div class="header">
      <h1>Aggiornamento OTA</h1>
      <p>Aggiorna firmware e filesystem via Wi-Fi</p>
    </div>

    <div class="warning-box">
      <strong>&#9888; Attenzione</strong>
      Non spegnere il dispositivo durante l'aggiornamento.<br>
      Firmware: usa il file <code>.bin</code> generato da Arduino IDE (Sketch &rarr; Export Compiled Binary).<br>
      LittleFS: usa il file <code>.bin</code> generato dal tool LittleFS Data Upload.
    </div>

    <!-- Firmware Upload -->
    <div class="upload-section section-firmware">
      <h2><span class="icon">&#128187;</span> Firmware</h2>
      <p class="desc">Aggiorna il programma principale del dispositivo (.bin)</p>
      <div class="file-input-wrap">
        <input type="file" id="fwFile" accept=".bin" onchange="fileSelected('fw')">
      </div>
      <button class="btn-upload btn-firmware" id="fwBtn" onclick="startUpload('fw')" disabled>Upload Firmware</button>
      <div class="progress-wrap" id="fwProgress">
        <div class="progress-bar">
          <div class="progress-fill firmware" id="fwFill"></div>
          <span class="progress-text" id="fwPercent">0%</span>
        </div>
      </div>
      <div class="status-msg" id="fwStatus"></div>
    </div>

    <!-- LittleFS Upload -->
    <div class="upload-section section-littlefs">
      <h2><span class="icon">&#128193;</span> LittleFS Filesystem</h2>
      <p class="desc">Aggiorna i file di sistema: audio, configurazione, pagine web (.bin)</p>
      <div class="warning-box" style="margin-top:0;margin-bottom:15px;border-color:rgba(233,69,96,0.35);background:rgba(233,69,96,0.1);color:#e94560">
        <strong>&#9888; Sovrascrittura completa</strong>
        L'upload LittleFS sostituisce <b>l'intera partizione</b> filesystem.<br>
        I dati salvati a runtime (API key, eventi calendario, configurazioni) verranno <b>cancellati</b>.<br>
        Dopo l'aggiornamento dovrai riconfigurare le impostazioni dalla pagina <b>/settings</b>.
      </div>
      <div class="file-input-wrap">
        <input type="file" id="fsFile" accept=".bin" onchange="fileSelected('fs')">
      </div>
      <button class="btn-upload btn-littlefs" id="fsBtn" onclick="startUpload('fs')" disabled>Upload LittleFS</button>
      <div class="progress-wrap" id="fsProgress">
        <div class="progress-bar">
          <div class="progress-fill littlefs" id="fsFill"></div>
          <span class="progress-text" id="fsPercent">0%</span>
        </div>
      </div>
      <div class="status-msg" id="fsStatus"></div>
    </div>

    <!-- Guida Compilazione -->
    <div class="guide-section">
      <h2><span class="icon">&#128214;</span> Come generare i file .bin</h2>

      <div class="guide-sub">Firmware (.bin)</div>
      <div class="guide-step">
        <div class="step-num">1</div>
        <div class="step-text">Apri <strong>Arduino IDE</strong> e carica il progetto <code>oraQuadra_Nano_Ver_1.5.ino</code></div>
      </div>
      <div class="guide-step">
        <div class="step-num">2</div>
        <div class="step-text">Vai su <strong>Sketch &rarr; Export Compiled Binary</strong> (oppure <code>Ctrl+Alt+S</code>)</div>
      </div>
      <div class="guide-step">
        <div class="step-num">3</div>
        <div class="step-text">Il file <code>.bin</code> viene salvato nella cartella <strong>build/</strong> del progetto</div>
      </div>

      <hr class="guide-divider">

      <div class="guide-sub">LittleFS (.bin)</div>
      <div class="guide-step">
        <div class="step-num">1</div>
        <div class="step-text">Installa il plugin <strong>ESP32 LittleFS Data Upload</strong> in Arduino IDE</div>
      </div>
      <div class="guide-step">
        <div class="step-num">2</div>
        <div class="step-text">Inserisci i file (audio, config) nella cartella <code>data/</code> del progetto</div>
      </div>
      <div class="guide-step">
        <div class="step-num">3</div>
        <div class="step-text">Vai su <strong>Tools &rarr; ESP32 Sketch Data Upload</strong></div>
      </div>
      <div class="guide-step">
        <div class="step-num">4</div>
        <div class="step-text">Per trovare il file <code>.bin</code> generato: attiva <strong>File &rarr; Preferences &rarr; Show verbose output</strong> e cerca il percorso nella console</div>
      </div>
    </div>

    <div class="footer">
      OraQuadra Nano v1.5 &bull; OTA Update
    </div>
  </div>

  <script>
    function fileSelected(type){
      const input=document.getElementById(type==='fw'?'fwFile':'fsFile');
      const btn=document.getElementById(type==='fw'?'fwBtn':'fsBtn');
      btn.disabled=!input.files.length;
    }

    function startUpload(type){
      const input=document.getElementById(type==='fw'?'fwFile':'fsFile');
      const btn=document.getElementById(type==='fw'?'fwBtn':'fsBtn');
      const progressWrap=document.getElementById(type==='fw'?'fwProgress':'fsProgress');
      const fill=document.getElementById(type==='fw'?'fwFill':'fsFill');
      const pctText=document.getElementById(type==='fw'?'fwPercent':'fsPercent');
      const statusEl=document.getElementById(type==='fw'?'fwStatus':'fsStatus');

      if(!input.files.length) return;
      const file=input.files[0];
      const url=type==='fw'?'/update/firmware':'/update/littlefs';

      // Reset UI
      btn.disabled=true;
      progressWrap.style.display='block';
      fill.style.width='0%';
      pctText.textContent='0%';
      statusEl.className='status-msg';
      statusEl.style.display='none';

      const xhr=new XMLHttpRequest();
      xhr.open('POST',url,true);

      xhr.upload.addEventListener('progress',function(e){
        if(e.lengthComputable){
          const pct=Math.round((e.loaded/e.total)*100);
          fill.style.width=pct+'%';
          pctText.textContent=pct+'%';
        }
      });

      xhr.addEventListener('load',function(){
        if(xhr.status===200){
          fill.style.width='100%';
          pctText.textContent='100%';
          statusEl.className='status-msg success';
          statusEl.textContent='Aggiornamento completato! Il dispositivo si sta riavviando...';
          statusEl.style.display='block';
          setTimeout(function(){
            statusEl.className='status-msg info';
            statusEl.textContent='Riconnessione in corso...';
            setTimeout(function(){location.href='/';},10000);
          },3000);
        }else{
          statusEl.className='status-msg error';
          statusEl.textContent='Errore: '+xhr.responseText;
          statusEl.style.display='block';
          btn.disabled=false;
        }
      });

      xhr.addEventListener('error',function(){
        statusEl.className='status-msg error';
        statusEl.textContent='Errore di connessione. Verificare che il dispositivo sia raggiungibile.';
        statusEl.style.display='block';
        btn.disabled=false;
      });

      const formData=new FormData();
      formData.append('file',file,file.name);
      xhr.send(formData);
    }
  </script>
</body>
</html>
)rawliteral";
