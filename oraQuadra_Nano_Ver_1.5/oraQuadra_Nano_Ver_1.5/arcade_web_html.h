// ============================================================================
// arcade_web_html.h - Web interface for Arcade mode
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
  <style>
    :root { --bg: #0a0a0a; --card: #1a1a2e; --accent: #FFD700; --text: #e0e0e0; --dim: #888; }
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: 'Segoe UI', system-ui, sans-serif; background: var(--bg); color: var(--text); min-height: 100vh; padding: 16px; }
    .header { text-align: center; padding: 20px 0; }
    .header h1 { color: var(--accent); font-size: 28px; letter-spacing: 3px; }
    .header p { color: var(--dim); margin-top: 4px; }
    .nav { display: flex; gap: 8px; justify-content: center; margin-bottom: 20px; flex-wrap: wrap; }
    .nav a { color: var(--accent); text-decoration: none; padding: 8px 16px; border: 1px solid #333; border-radius: 20px; font-size: 13px; transition: all 0.2s; }
    .nav a:hover { background: #222; border-color: var(--accent); }
    .status-card { background: var(--card); border-radius: 12px; padding: 20px; margin-bottom: 16px; border: 1px solid #333; }
    .status-card h3 { color: var(--accent); margin-bottom: 12px; font-size: 16px; }
    .status-row { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #222; }
    .status-row:last-child { border-bottom: none; }
    .status-label { color: var(--dim); }
    .status-value { color: var(--text); font-weight: 600; }
    .game-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(200px, 1fr)); gap: 12px; margin-top: 16px; }
    .game-card { background: var(--card); border-radius: 10px; padding: 16px; border: 1px solid #333; cursor: pointer; transition: all 0.2s; text-align: center; }
    .game-card:hover { border-color: var(--accent); transform: translateY(-2px); }
    .game-card.active { border-color: var(--accent); background: #1a1a3e; }
    .game-card .icon { font-size: 36px; margin-bottom: 8px; }
    .game-card .name { font-size: 14px; font-weight: 600; color: var(--text); }
    .game-card .type { font-size: 11px; color: var(--dim); margin-top: 4px; }
    .btn { display: inline-block; padding: 10px 24px; border-radius: 8px; border: none; font-size: 14px; cursor: pointer; transition: all 0.2s; font-weight: 600; }
    .btn-primary { background: var(--accent); color: #000; }
    .btn-primary:hover { background: #FFC000; }
    .btn-danger { background: #dc3545; color: white; }
    .btn-danger:hover { background: #c82333; }
    .actions { text-align: center; margin-top: 20px; display: flex; gap: 12px; justify-content: center; }
  </style>
</head>
<body>
  <div class="header">
    <h1>ARCADE</h1>
    <p>Classic Arcade Game Emulator</p>
  </div>

  <div class="nav">
    <a href="/">Home</a>
    <a href="/settings">Settings</a>
  </div>

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

  <script>
    const gameIcons = {
      'PAC-MAN': '\u{1F47B}', 'GALAGA': '\u{1F680}', 'DONKEY KONG': '\u{1F435}',
      'FROGGER': '\u{1F438}', 'DIG DUG': '\u{26CF}', '1942': '\u{2708}',
      'EYES': '\u{1F440}', 'MR. TNT': '\u{1F4A3}', 'LIZ WIZ': '\u{1F98E}',
      'THE GLOB': '\u{1F7E2}', 'CRUSH ROLLER': '\u{1F3B3}', 'ANTEATER': '\u{1F41C}'
    };

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
            card.className = 'game-card' + (d.selectedGame === i ? ' active' : '');
            card.innerHTML = '<div class="icon">' + (gameIcons[g.name] || '\u{1F3AE}') + '</div>' +
              '<div class="name">' + g.name + '</div>' +
              '<div class="type">Tipo: ' + g.type + '</div>';
            card.onclick = () => startGame(i);
            grid.appendChild(card);
          });
        }
      } catch(e) {
        console.error('Fetch error:', e);
      }
    }

    async function startGame(index) {
      try {
        await fetch('/arcade/start?game=' + index, { method: 'POST' });
        setTimeout(fetchStatus, 500);
      } catch(e) { console.error(e); }
    }

    async function stopGame() {
      try {
        await fetch('/arcade/stop', { method: 'POST' });
        setTimeout(fetchStatus, 500);
      } catch(e) { console.error(e); }
    }

    fetchStatus();
    setInterval(fetchStatus, 3000);
  </script>
</body>
</html>
)rawliteral";

#endif // ARCADE_WEB_HTML_H
