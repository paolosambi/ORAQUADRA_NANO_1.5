// ============================================================================
// arcade_web_html.h - Web interface for Arcade mode
// Features: Game toggle ON/OFF, ROM Manager (SD upload/delete)
// ============================================================================

#ifndef ARCADE_WEB_HTML_H
#define ARCADE_WEB_HTML_H

const char ARCADE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Arcade - oraQuadra Nano</title>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/jszip/3.10.1/jszip.min.js"></script>
  <style>
    :root { --bg: #0a0a0a; --card: #1a1a2e; --accent: #FFD700; --text: #e0e0e0; --dim: #888; --green: #4CAF50; --red: #dc3545; }
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: 'Segoe UI', system-ui, sans-serif; background: var(--bg); color: var(--text); min-height: 100vh; padding: 16px; }
    .header { text-align: center; padding: 20px 0; }
    .header h1 { color: var(--accent); font-size: 28px; letter-spacing: 3px; }
    .header p { color: var(--dim); margin-top: 4px; }
    .nav { display: flex; gap: 8px; justify-content: center; margin-bottom: 20px; flex-wrap: wrap; }
    .nav a { color: var(--accent); text-decoration: none; padding: 8px 16px; border: 1px solid #333; border-radius: 20px; font-size: 13px; transition: all 0.2s; }
    .nav a:hover { background: #222; border-color: var(--accent); }

    /* Tabs */
    .tabs { display: flex; gap: 0; margin-bottom: 20px; border-bottom: 2px solid #333; }
    .tab { padding: 12px 24px; cursor: pointer; color: var(--dim); font-weight: 600; border-bottom: 2px solid transparent; margin-bottom: -2px; transition: all 0.2s; }
    .tab:hover { color: var(--text); }
    .tab.active { color: var(--accent); border-bottom-color: var(--accent); }
    .tab-content { display: none; }
    .tab-content.active { display: block; }

    /* Status card */
    .status-card { background: var(--card); border-radius: 12px; padding: 20px; margin-bottom: 16px; border: 1px solid #333; }
    .status-card h3 { color: var(--accent); margin-bottom: 12px; font-size: 16px; }
    .status-row { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #222; }
    .status-row:last-child { border-bottom: none; }
    .status-label { color: var(--dim); }
    .status-value { color: var(--text); font-weight: 600; }

    /* Game grid */
    .game-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(200px, 1fr)); gap: 12px; margin-top: 16px; }
    .game-card { background: var(--card); border-radius: 10px; padding: 16px; border: 1px solid #333; transition: all 0.2s; text-align: center; position: relative; }
    .game-card:hover { border-color: var(--accent); transform: translateY(-2px); }
    .game-card.active { border-color: var(--accent); background: #1a1a3e; }
    .game-card.disabled { opacity: 0.5; filter: grayscale(70%); }
    .game-card .icon { font-size: 36px; margin-bottom: 8px; }
    .game-card .name { font-size: 14px; font-weight: 600; color: var(--text); }
    .game-card .type { font-size: 11px; color: var(--dim); margin-top: 4px; }
    .game-card .card-actions { display: flex; gap: 8px; justify-content: center; margin-top: 12px; align-items: center; }

    /* Toggle switch */
    .toggle { position: relative; width: 44px; height: 24px; flex-shrink: 0; }
    .toggle input { opacity: 0; width: 0; height: 0; }
    .toggle .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background: #555; border-radius: 24px; transition: 0.3s; }
    .toggle .slider:before { content: ''; position: absolute; height: 18px; width: 18px; left: 3px; bottom: 3px; background: white; border-radius: 50%; transition: 0.3s; }
    .toggle input:checked + .slider { background: var(--green); }
    .toggle input:checked + .slider:before { transform: translateX(20px); }

    /* Buttons */
    .btn { display: inline-block; padding: 8px 16px; border-radius: 8px; border: none; font-size: 13px; cursor: pointer; transition: all 0.2s; font-weight: 600; }
    .btn-sm { padding: 5px 12px; font-size: 12px; }
    .btn-primary { background: var(--accent); color: #000; }
    .btn-primary:hover { background: #FFC000; }
    .btn-danger { background: var(--red); color: white; }
    .btn-danger:hover { background: #c82333; }
    .btn-secondary { background: #444; color: var(--text); }
    .btn-secondary:hover { background: #555; }
    .actions { text-align: center; margin-top: 20px; display: flex; gap: 12px; justify-content: center; }

    /* ROM Manager */
    .sd-bar { background: #222; border-radius: 8px; height: 24px; overflow: hidden; margin: 12px 0; }
    .sd-bar-fill { background: linear-gradient(90deg, var(--green), var(--accent)); height: 100%; border-radius: 8px; transition: width 0.5s; }
    .sd-info { display: flex; justify-content: space-between; font-size: 12px; color: var(--dim); margin-bottom: 4px; }
    .rom-folder { background: var(--card); border-radius: 10px; padding: 16px; margin-bottom: 12px; border: 1px solid #333; }
    .rom-folder-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; cursor: pointer; }
    .rom-folder-header h4 { color: var(--accent); font-size: 15px; }
    .rom-file { display: flex; justify-content: space-between; align-items: center; padding: 6px 8px; border-bottom: 1px solid #1a1a1a; font-size: 13px; }
    .rom-file:last-child { border-bottom: none; }
    .rom-file .fname { color: var(--text); }
    .rom-file .fsize { color: var(--dim); font-size: 12px; }

    /* Upload area */
    .upload-area { border: 2px dashed #444; border-radius: 12px; padding: 30px; text-align: center; margin: 16px 0; transition: all 0.3s; cursor: pointer; }
    .upload-area:hover, .upload-area.dragover { border-color: var(--accent); background: rgba(255,215,0,0.05); }
    .upload-area p { color: var(--dim); margin-top: 8px; }
    .upload-row { display: flex; gap: 12px; align-items: center; margin-bottom: 16px; flex-wrap: wrap; }
    .upload-row select, .upload-row input { padding: 8px 12px; border-radius: 8px; border: 1px solid #444; background: #1a1a1a; color: var(--text); font-size: 13px; }
    .upload-row select { min-width: 160px; }
    .progress-bar { width: 100%; height: 6px; background: #333; border-radius: 3px; overflow: hidden; margin-top: 8px; display: none; }
    .progress-bar-fill { height: 100%; background: var(--accent); border-radius: 3px; transition: width 0.3s; }

    /* Dialog */
    .dialog-overlay { position: fixed; top: 0; left: 0; right: 0; bottom: 0; background: rgba(0,0,0,0.7); display: none; align-items: center; justify-content: center; z-index: 100; }
    .dialog-overlay.show { display: flex; }
    .dialog { background: var(--card); border: 1px solid #555; border-radius: 12px; padding: 24px; max-width: 400px; width: 90%; text-align: center; }
    .dialog h3 { margin-bottom: 12px; color: var(--accent); }
    .dialog p { margin-bottom: 20px; color: var(--dim); font-size: 14px; word-break: break-all; }
    .dialog .btn { margin: 0 6px; }
  </style>
</head>
<body>
  <div class="header">
    <h1>ARCADE</h1>
    <p>Classic Arcade Game Emulator</p>
  </div>

  <div style="background:#1a1a2e;border:1px solid #444;border-radius:10px;padding:14px 18px;margin:0 auto 16px;max-width:640px;font-size:12px;color:#aaa;line-height:1.5;text-align:center">
    <span style="color:#FFD700;font-weight:600">&#9888; Copyright Notice</span><br>
    Questo emulatore arcade e' un software gratuito e open source basato sul progetto <b>Galagino</b>.<br>
    Le ROM dei giochi <b>non sono incluse</b> per motivi di copyright. I diritti di tutti i giochi originali appartengono ai rispettivi titolari (Namco, Nintendo, Konami, Capcom e altri).<br>
    L'utente e' responsabile di fornire le proprie ROM nel rispetto delle leggi vigenti.
  </div>

  <div class="nav">
    <a href="/">Home</a>
    <a href="/settings">Settings</a>
  </div>

  <!-- Tab Switcher -->
  <div class="tabs">
    <div class="tab active" onclick="switchTab('games')">Giochi</div>
    <div class="tab" onclick="switchTab('roms')">ROM Manager</div>
  </div>

  <!-- TAB 1: GIOCHI -->
  <div id="tab-games" class="tab-content active">
    <div class="status-card">
      <h3>Status</h3>
      <div class="status-row">
        <span class="status-label">Stato</span>
        <span class="status-value" id="status">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Gioco attivo</span>
        <span class="status-value" id="currentGame">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Frame count</span>
        <span class="status-value" id="frames">--</span>
      </div>
      <div class="status-row">
        <span class="status-label">Giochi disponibili</span>
        <span class="status-value" id="gameCount">--</span>
      </div>
    </div>

    <div class="status-card">
      <h3>Giochi Disponibili</h3>
      <div class="game-grid" id="gameGrid"></div>
    </div>

    <div class="actions">
      <button class="btn btn-danger" onclick="stopGame()">Stop Gioco</button>
    </div>
  </div>

  <!-- TAB 2: ROM MANAGER -->
  <div id="tab-roms" class="tab-content">
    <div class="status-card">
      <h3>Spazio SD Card</h3>
      <div class="sd-info">
        <span id="sdUsed">--</span>
        <span id="sdFree">--</span>
      </div>
      <div class="sd-bar">
        <div class="sd-bar-fill" id="sdBarFill" style="width:0%"></div>
      </div>
      <div style="text-align:center;font-size:12px;color:var(--dim)" id="sdTotal">Totale: --</div>
    </div>

    <div class="status-card">
      <h3>Upload ROM</h3>
      <div class="upload-row">
        <select id="uploadFolder">
          <option value="PACMAN">PACMAN</option>
          <option value="GALAGA">GALAGA</option>
          <option value="DKONG">DKONG</option>
          <option value="FROGGER">FROGGER</option>
          <option value="DIGDUG">DIGDUG</option>
          <option value="1942">1942</option>
          <option value="EYES">EYES</option>
          <option value="MRTNT">MRTNT</option>
          <option value="LIZWIZ">LIZWIZ</option>
          <option value="THEGLOB">THEGLOB</option>
          <option value="CRUSH">CRUSH</option>
          <option value="ANTEATER">ANTEATER</option>
          <option value="LADYBUG">LADYBUG</option>
          <option value="XEVIOUS">XEVIOUS</option>
          <option value="BOMBJACK">BOMBJACK</option>
          <option value="GYRUSS">GYRUSS</option>
        </select>
        <input type="text" id="newFolder" placeholder="o nuova cartella..." style="flex:1;min-width:120px">
      </div>
      <div class="upload-area" id="dropZone" onclick="document.getElementById('fileInput').click()">
        <div style="font-size:36px">&#128193;</div>
        <p>Trascina file .bin o .zip MAME qui, o clicca per selezionare</p>
        <input type="file" id="fileInput" accept=".bin,.zip" multiple style="display:none">
      </div>
      <div class="progress-bar" id="uploadProgress">
        <div class="progress-bar-fill" id="uploadProgressFill" style="width:0%"></div>
      </div>
      <div id="uploadStatus" style="text-align:center;font-size:13px;color:var(--dim);margin-top:8px"></div>
    </div>

    <div class="status-card">
      <h3>File ROM su SD</h3>
      <div id="romTree">
        <p style="color:var(--dim);text-align:center">Caricamento...</p>
      </div>
      <div style="text-align:center;margin-top:12px">
        <button class="btn btn-secondary btn-sm" onclick="loadRoms()">&#128260; Aggiorna</button>
      </div>
    </div>
  </div>

  <!-- Delete confirmation dialog -->
  <div class="dialog-overlay" id="deleteDialog">
    <div class="dialog">
      <h3>Conferma Eliminazione</h3>
      <p id="deleteMsg">Eliminare questo file?</p>
      <button class="btn btn-danger" onclick="confirmDelete()">Elimina</button>
      <button class="btn btn-secondary" onclick="closeDialog()">Annulla</button>
    </div>
  </div>

  <script>
    const gameIcons = {
      'PAC-MAN': '\u{1F47B}', 'GALAGA': '\u{1F680}', 'DONKEY KONG': '\u{1F435}',
      'FROGGER': '\u{1F438}', 'DIG DUG': '\u{26CF}', '1942': '\u{2708}',
      'EYES': '\u{1F440}', 'MR. TNT': '\u{1F4A3}', 'LIZ WIZ': '\u{1F98E}',
      'THE GLOB': '\u{1F7E2}', 'CRUSH ROLLER': '\u{1F3B3}', 'ANTEATER': '\u{1F41C}',
      'LADY BUG': '\u{1F41E}', 'XEVIOUS': '\u{2708}', 'BOMB JACK': '\u{1F4A3}',
      'GYRUSS': '\u{1F680}'
    };

    let deletePath = '';

    // Tab switching
    function switchTab(tab) {
      document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
      document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
      document.querySelector('.tab:nth-child(' + (tab === 'games' ? '1' : '2') + ')').classList.add('active');
      document.getElementById('tab-' + tab).classList.add('active');
      if (tab === 'roms') loadRoms();
    }

    // Fetch arcade status
    async function fetchStatus() {
      try {
        const r = await fetch('/arcade/status');
        const d = await r.json();
        document.getElementById('status').textContent = d.inMenu ? 'Menu' : 'In gioco';
        document.getElementById('currentGame').textContent = d.gameName || '--';
        document.getElementById('frames').textContent = d.frames || '0';
        document.getElementById('gameCount').textContent = d.gameCount || '0';

        const grid = document.getElementById('gameGrid');
        grid.innerHTML = '';
        if (d.games) {
          d.games.forEach((g, i) => {
            const card = document.createElement('div');
            card.className = 'game-card' + (d.selectedGame === i ? ' active' : '') + (!g.enabled ? ' disabled' : '');
            card.innerHTML =
              '<div class="icon">' + (gameIcons[g.name] || '\u{1F3AE}') + '</div>' +
              '<div class="name">' + g.name + '</div>' +
              '<div class="type">Tipo: ' + g.type + '</div>' +
              '<div class="card-actions">' +
                '<label class="toggle"><input type="checkbox"' + (g.enabled ? ' checked' : '') +
                  ' onchange="toggleGame(' + i + ',this.checked)"><span class="slider"></span></label>' +
                '<button class="btn btn-primary btn-sm" onclick="startGame(' + i + ')"' +
                  (!g.enabled ? ' disabled style="opacity:0.4;cursor:not-allowed"' : '') +
                '>Play</button>' +
              '</div>';
            grid.appendChild(card);
          });
        }
      } catch(e) {
        console.error('Fetch error:', e);
      }
    }

    // Toggle game enabled
    async function toggleGame(idx, enabled) {
      try {
        await fetch('/arcade/togglegame?idx=' + idx + '&enabled=' + (enabled ? 'true' : 'false'), { method: 'POST' });
        setTimeout(fetchStatus, 300);
      } catch(e) { console.error(e); }
    }

    // Start game
    async function startGame(index) {
      try {
        const r = await fetch('/arcade/start?game=' + index, { method: 'POST' });
        const d = await r.json();
        if (d.error) alert(d.error);
        setTimeout(fetchStatus, 500);
      } catch(e) { console.error(e); }
    }

    // Stop game
    async function stopGame() {
      try {
        await fetch('/arcade/stop', { method: 'POST' });
        setTimeout(fetchStatus, 500);
      } catch(e) { console.error(e); }
    }

    // ===== ROM Manager =====

    function formatSize(kb) {
      if (kb >= 1024) return (kb / 1024).toFixed(1) + ' MB';
      return kb + ' KB';
    }

    async function loadRoms() {
      try {
        const r = await fetch('/arcade/roms/list');
        const d = await r.json();

        // SD space
        const pct = d.sdTotal > 0 ? ((d.sdUsed / d.sdTotal) * 100).toFixed(1) : 0;
        document.getElementById('sdUsed').textContent = 'Usato: ' + formatSize(d.sdUsed);
        document.getElementById('sdFree').textContent = 'Libero: ' + formatSize(d.sdFree);
        document.getElementById('sdTotal').textContent = 'Totale: ' + formatSize(d.sdTotal);
        document.getElementById('sdBarFill').style.width = pct + '%';

        // Sync dropdown with server folders (add any new ones found on SD)
        const sel = document.getElementById('uploadFolder');
        const existing = new Set();
        for (let i = 0; i < sel.options.length; i++) existing.add(sel.options[i].value);
        if (d.folders) {
          d.folders.forEach(f => {
            if (!existing.has(f.name)) {
              const opt = document.createElement('option');
              opt.value = f.name;
              opt.textContent = f.name;
              sel.appendChild(opt);
            }
          });
        }

        // ROM tree
        const tree = document.getElementById('romTree');
        if (!d.folders || d.folders.length === 0) {
          tree.innerHTML = '<p style="color:var(--dim);text-align:center">Nessuna cartella ROM trovata su SD</p>';
          return;
        }

        let html = '';
        d.folders.forEach(f => {
          html += '<div class="rom-folder">';
          html += '<div class="rom-folder-header">';
          html += '<h4>&#128193; ' + f.name + ' (' + f.files.length + ' file)</h4>';
          html += '<button class="btn btn-danger btn-sm" onclick="askDelete(\'' + f.name + '\',true)">&#128465; Cartella</button>';
          html += '</div>';
          f.files.forEach(file => {
            html += '<div class="rom-file">';
            html += '<span class="fname">' + file.name + '</span>';
            html += '<span class="fsize">' + formatSize(Math.ceil(file.size / 1024)) + '</span>';
            html += '<button class="btn btn-danger btn-sm" onclick="askDelete(\'' + f.name + '/' + file.name + '\',false)" style="margin-left:8px">&#128465;</button>';
            html += '</div>';
          });
          html += '</div>';
        });
        tree.innerHTML = html;
      } catch(e) {
        console.error('ROM list error:', e);
        document.getElementById('romTree').innerHTML = '<p style="color:var(--red)">Errore caricamento lista ROM</p>';
      }
    }

    // ===== MAME ROM Chip Mapping =====
    // MAME chip name mapping. _folder overrides the upload folder (for variant ROM sets)
    const MAME_ROM_MAP = {
      PACMAN:      { rom1: ['pacman.6e','pacman.6f','pacman.6h','pacman.6j'] },
      GALAGA:      { rom1: ['gg1_1b.3p','gg1_2b.3m','gg1_3.2m','gg1_4b.2l'], rom2: ['gg1_5b.3f'], rom3: ['gg1_7b.2c'] },
      DKONG:       { rom1: ['c_5et_g.bin','c_5ct_g.bin','c_5bt_g.bin','c_5at_g.bin'], rom2: ['s_3i_b.bin','s_3j_b.bin'] },
      FROGGER:     { rom1: ['frogger.26','frogger.27','frsm3.7'], rom2: ['frogger.608','frogger.609','frogger.610'] },
      DIGDUG:      { rom1: ['dd1a.1','dd1a.2','dd1a.3','dd1a.4'], rom2: ['dd1a.5','dd1a.6'], rom3: ['dd1.7'] },
      DIGDUG_R1:   { _folder:'DIGDUG', rom1: ['dd1.1','dd1.2','dd1.3','dd1.4'], rom2: ['dd1.5','dd1.6'], rom3: ['dd1.7'] },
      DIGDUG_AT:   { _folder:'DIGDUG', rom1: ['136007.101','136007.102','136007.103','136007.104'], rom2: ['136007.105','136007.106'], rom3: ['136007.107'] },
      '1942':      { rom1: ['srb-03.m3','srb-04.m4'], rom2: ['sr-01.c11'], rom1_b0: ['srb-05.m5'], rom1_b1: ['srb-06.m6'], rom1_b2: ['srb-07.m7'] },
      EYES:        { rom1: ['d7.bin','e7.bin','f7.bin','h7.bin'] },
      MRTNT:       { rom1: ['tnt.1','tnt.2','tnt.3','tnt.4'] },
      LIZWIZ:      { rom1: ['6e.cpu','6f.cpu','6h.cpu','6j.cpu'] },
      THEGLOB:     { rom1: ['glob.u2','glob.u3'] },
      CRUSH:       { rom1: ['crushkrl.6e','crushkrl.6f','crushkrl.6h','crushkrl.6j'] },
      ANTEATER:    { rom1: ['ra1-low.bin','ra1-high.bin','ra2-low.bin','ra2-high.bin'], rom2: ['ra4-low.bin','ra4-high.bin'] },
      LADYBUG:     { rom1: ['lb1a.cpu','lb2a.cpu','lb3a.cpu','l4.h4','l5.j4','lb6a.cpu'] },
      XEVIOUS:     { rom1: ['xvi_1.3p','xvi_2.3m','xvi_3.2m','xvi_4.2l'], rom2: ['xvi_5.3f','xvi_6.3j'], rom3: ['xvi_7.2c'] },
      BOMBJACK:    { rom1: ['09_j01b.bin','10_l01b.bin','11_m01b.bin','12_n01b.bin','13.1r'] },
      GYRUSS:      { rom1: ['gyrussk.1','gyrussk.2','gyrussk.3'], rom2: ['gyrussk.9'], rom3: ['gyrussk.1a','gyrussk.2a'] }
    };

    // Detect which game a set of filenames belongs to
    function detectMameGame(zipFileNames) {
      const lower = zipFileNames.map(n => n.toLowerCase());
      let bestGame = null, bestCount = 0, bestTotal = 0;
      for (const [game, roms] of Object.entries(MAME_ROM_MAP)) {
        let totalChips = 0, foundChips = 0;
        for (const [key, chips] of Object.entries(roms)) {
          if (key === '_folder') continue;  // skip folder override
          for (const chip of chips) {
            totalChips++;
            if (lower.includes(chip.toLowerCase())) foundChips++;
          }
        }
        if (foundChips > bestCount || (foundChips === bestCount && totalChips > bestTotal)) {
          bestCount = foundChips;
          bestTotal = totalChips;
          bestGame = game;
        }
      }
      return (bestCount > 0) ? bestGame : null;
    }

    // Upload a single Blob as a named file to the given folder
    function uploadBlob(blob, fileName, folder, statusEl, fill, bar) {
      return new Promise((resolve, reject) => {
        bar.style.display = 'block';
        fill.style.width = '0%';
        statusEl.textContent = 'Upload: ' + fileName + ' -> ' + folder + '/...';
        const formData = new FormData();
        formData.append('file', blob, fileName);
        const xhr = new XMLHttpRequest();
        xhr.open('POST', '/arcade/roms/upload?folder=' + encodeURIComponent(folder));
        xhr.upload.onprogress = e => {
          if (e.lengthComputable) fill.style.width = ((e.loaded / e.total) * 100).toFixed(0) + '%';
        };
        xhr.onload = () => {
          if (xhr.status === 200) {
            try {
              const d = JSON.parse(xhr.responseText);
              if (d.success) { statusEl.textContent = fileName + ' caricato in ' + folder + '!'; }
              else { statusEl.textContent = 'Errore: ' + (d.error || 'upload fallito'); }
            } catch(ex) { statusEl.textContent = 'Errore risposta server'; }
          } else { statusEl.textContent = 'Errore HTTP ' + xhr.status; }
          resolve();
        };
        xhr.onerror = () => { statusEl.textContent = 'Errore di rete'; resolve(); };
        xhr.send(formData);
      });
    }

    // Convert a MAME ZIP: decompress, detect game, concatenate chips -> romN.bin, upload
    async function convertMameZip(file) {
      const bar = document.getElementById('uploadProgress');
      const fill = document.getElementById('uploadProgressFill');
      const statusEl = document.getElementById('uploadStatus');

      bar.style.display = 'block';
      fill.style.width = '0%';
      statusEl.textContent = 'Apertura ZIP: ' + file.name + '...';

      let zip;
      try {
        zip = await JSZip.loadAsync(file);
      } catch(e) {
        statusEl.textContent = 'Errore: impossibile aprire lo ZIP - ' + e.message;
        return;
      }

      // List all files in the ZIP (flatten paths)
      const zipFiles = {};
      zip.forEach((path, entry) => {
        if (!entry.dir) {
          const baseName = path.includes('/') ? path.split('/').pop() : path;
          zipFiles[baseName.toLowerCase()] = entry;
        }
      });
      const zipFileNames = Object.keys(zipFiles);

      statusEl.textContent = 'Rilevamento gioco (' + zipFileNames.length + ' file nello ZIP)...';

      // Detect game
      const detectedGame = detectMameGame(zipFileNames);
      if (!detectedGame) {
        statusEl.textContent = 'Errore: gioco non riconosciuto. Chip trovati: ' + zipFileNames.join(', ');
        setTimeout(() => { bar.style.display = 'none'; }, 3000);
        return;
      }

      const romDef = MAME_ROM_MAP[detectedGame];
      const uploadFolder = romDef._folder || detectedGame;  // Use _folder override if present

      // Auto-select the folder dropdown (use canonical folder name)
      const folderSelect = document.getElementById('uploadFolder');
      for (let i = 0; i < folderSelect.options.length; i++) {
        if (folderSelect.options[i].value === uploadFolder) {
          folderSelect.selectedIndex = i;
          break;
        }
      }
      document.getElementById('newFolder').value = '';

      statusEl.textContent = 'Gioco rilevato: ' + uploadFolder + ' - conversione chip...';

      // For each romN entry, concatenate chips in order (skip _folder metadata)
      const romEntries = Object.entries(romDef).filter(([k]) => k !== '_folder').sort((a, b) => a[0].localeCompare(b[0]));
      let uploadCount = 0;

      for (const [romName, chips] of romEntries) {
        // Build the output filename: rom1->rom1.bin, rom1_b0->rom1_b0.bin, etc
        const outFileName = romName + '.bin';

        // Collect chip data in order
        const parts = [];
        let missing = [];
        for (const chip of chips) {
          const entry = zipFiles[chip.toLowerCase()];
          if (!entry) {
            missing.push(chip);
          } else {
            parts.push(entry);
          }
        }

        if (missing.length > 0) {
          statusEl.textContent = 'Attenzione: chip mancanti per ' + outFileName + ': ' + missing.join(', ');
          await new Promise(r => setTimeout(r, 1500));
          if (parts.length === 0) continue;
        }

        // Read and concatenate all chip data
        statusEl.textContent = 'Concatenazione ' + chips.length + ' chip -> ' + outFileName + '...';
        const buffers = [];
        let totalSize = 0;
        for (const entry of parts) {
          const data = await entry.async('uint8array');
          buffers.push(data);
          totalSize += data.length;
        }

        // Merge into single Uint8Array
        const merged = new Uint8Array(totalSize);
        let offset = 0;
        for (const buf of buffers) {
          merged.set(buf, offset);
          offset += buf.length;
        }

        const blob = new Blob([merged], { type: 'application/octet-stream' });
        statusEl.textContent = outFileName + ' (' + (totalSize / 1024).toFixed(1) + ' KB) - upload...';

        await uploadBlob(blob, outFileName, uploadFolder, statusEl, fill, bar);
        uploadCount++;
      }

      statusEl.textContent = uploadFolder + ': ' + uploadCount + ' file ROM convertiti e caricati!';
      setTimeout(() => { bar.style.display = 'none'; }, 3000);
      loadRoms();
      refreshGameList();
    }

    // Upload
    const dropZone = document.getElementById('dropZone');
    const fileInput = document.getElementById('fileInput');

    dropZone.addEventListener('dragover', e => { e.preventDefault(); dropZone.classList.add('dragover'); });
    dropZone.addEventListener('dragleave', () => dropZone.classList.remove('dragover'));
    dropZone.addEventListener('drop', e => {
      e.preventDefault();
      dropZone.classList.remove('dragover');
      if (e.dataTransfer.files.length) uploadFiles(e.dataTransfer.files);
    });
    fileInput.addEventListener('change', () => { if (fileInput.files.length) uploadFiles(fileInput.files); });

    async function uploadFiles(files) {
      const bar = document.getElementById('uploadProgress');
      const fill = document.getElementById('uploadProgressFill');
      const statusEl = document.getElementById('uploadStatus');

      for (let i = 0; i < files.length; i++) {
        const file = files[i];
        const name = file.name.toLowerCase();

        // ZIP file -> MAME conversion
        if (name.endsWith('.zip')) {
          await convertMameZip(file);
          continue;
        }

        // BIN file -> direct upload
        if (!name.endsWith('.bin')) {
          statusEl.textContent = 'Errore: ' + file.name + ' - formato non supportato (usa .bin o .zip)';
          continue;
        }

        const folder = document.getElementById('newFolder').value.trim() || document.getElementById('uploadFolder').value;
        await uploadBlob(file, file.name, folder, statusEl, fill, bar);
      }

      setTimeout(() => { bar.style.display = 'none'; }, 2000);
      loadRoms();
      refreshGameList();
      fileInput.value = '';
    }

    // Delete
    function askDelete(path, isFolder) {
      deletePath = path;
      document.getElementById('deleteMsg').textContent = isFolder
        ? 'Eliminare la cartella "' + path + '" e tutti i file contenuti?'
        : 'Eliminare il file "' + path + '"?';
      document.getElementById('deleteDialog').classList.add('show');
    }

    function closeDialog() {
      document.getElementById('deleteDialog').classList.remove('show');
    }

    async function confirmDelete() {
      closeDialog();
      try {
        await fetch('/arcade/roms/delete?path=' + encodeURIComponent(deletePath), { method: 'POST' });
        loadRoms();
        refreshGameList();
      } catch(e) { console.error(e); }
    }

    // Refresh game list on device + web after ROM changes
    async function refreshGameList() {
      try {
        await fetch('/arcade/roms/refresh', { method: 'POST' });
        setTimeout(fetchStatus, 500);
      } catch(e) { console.error(e); }
    }

    // Init
    fetchStatus();
    setInterval(fetchStatus, 3000);
  </script>
</body>
</html>
)rawliteral";

#endif // ARCADE_WEB_HTML_H
