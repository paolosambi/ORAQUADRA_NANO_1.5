const char DUALDISPLAY_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
  <title>Multi-Display Config</title>
  <style>
    :root{--bg:#0a0a1a;--bg-card:rgba(15,23,42,0.95);--accent:#00d9ff;--accent2:#e94560;
      --accent-green:#22c55e;--danger:#ef4444;--text:#e2e8f0;--text-dim:#64748b;
      --border:rgba(255,255,255,0.08);--border-accent:rgba(0,217,255,0.3)}
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:'Inter',system-ui,sans-serif;background:var(--bg);color:var(--text);
      min-height:100vh;overflow-x:hidden}
    .bg-grid{position:fixed;inset:0;
      background-image:radial-gradient(rgba(0,217,255,0.03) 1px,transparent 1px);
      background-size:30px 30px;pointer-events:none;z-index:0}
    .container{max-width:640px;margin:0 auto;padding:16px;position:relative;z-index:1}
    .nav-back{display:inline-block;color:var(--accent);text-decoration:none;font-size:0.85rem;
      padding:8px 0;font-weight:500;transition:opacity 0.2s}
    .nav-back:hover{opacity:0.7}
    .header{text-align:center;padding:20px 0 16px}
    .header h1{font-size:1.5rem;font-weight:700;letter-spacing:-0.5px}
    .header h1 span{background:linear-gradient(135deg,#ff6b35,#ff9500);-webkit-background-clip:text;
      -webkit-text-fill-color:transparent;background-clip:text}
    .header .sub{color:var(--text-dim);font-size:0.8rem;margin-top:6px}

    /* === SEZIONI === */
    .section{background:var(--bg-card);border:1px solid var(--border);border-radius:16px;
      margin-bottom:16px;overflow:hidden}
    .section-head{display:flex;align-items:center;gap:10px;padding:16px 18px;font-weight:700;
      font-size:0.95rem;border-bottom:1px solid var(--border)}
    .section-head .icon{width:32px;height:32px;border-radius:9px;display:flex;align-items:center;
      justify-content:center;font-size:1.1rem;flex-shrink:0}
    .section-body{padding:16px 18px}

    /* Sezioni collapsabili */
    .section-head.collapsible{cursor:pointer;user-select:none;transition:background 0.2s}
    .section-head.collapsible:hover{background:rgba(255,255,255,0.03)}
    .section-head .arrow{margin-left:auto;font-size:0.7rem;color:var(--text-dim);transition:transform 0.3s}
    .section.open .section-head .arrow{transform:rotate(180deg)}
    .section-collapse{max-height:0;overflow:hidden;transition:max-height 0.35s ease-out}
    .section.open .section-collapse{max-height:3000px}

    /* === POWER CARD === */
    .power-card{position:relative;transition:border-color 0.4s,box-shadow 0.4s}
    .power-card.active{border-color:rgba(34,197,94,0.4);box-shadow:0 0 30px rgba(34,197,94,0.08)}
    .power-card.inactive{border-color:rgba(255,255,255,0.06)}

    .power-row{display:flex;align-items:center;justify-content:space-between;gap:16px}
    .power-label{font-size:1.1rem;font-weight:700}
    .power-status{font-size:0.8rem;font-weight:600;padding:3px 10px;border-radius:20px;margin-left:8px}
    .power-status.on{background:rgba(34,197,94,0.15);color:var(--accent-green)}
    .power-status.off{background:rgba(255,255,255,0.06);color:var(--text-dim)}

    .toggle{position:relative;width:56px;height:30px;flex-shrink:0}
    .toggle input{opacity:0;width:0;height:0}
    .toggle-slider{position:absolute;cursor:pointer;inset:0;background:#475569;border-radius:30px;transition:0.3s}
    .toggle-slider::before{content:'';position:absolute;height:24px;width:24px;left:3px;bottom:3px;
      background:#fff;border-radius:50%;transition:0.3s}
    .toggle input:checked+.toggle-slider{background:var(--accent-green)}
    .toggle input:checked+.toggle-slider::before{transform:translateX(26px)}

    .mac-box{background:rgba(0,0,0,0.35);border:1px dashed var(--accent);border-radius:10px;
      padding:10px 14px;margin-top:14px;display:flex;align-items:center;justify-content:space-between;
      cursor:pointer;transition:all 0.2s}
    .mac-box:hover{background:rgba(0,217,255,0.06);border-style:solid}
    .mac-box:active{transform:scale(0.98)}
    .mac-box .mac-label{font-size:0.7rem;color:var(--text-dim);text-transform:uppercase;letter-spacing:0.5px}
    .mac-box .mac-addr{font-size:1.05rem;font-weight:700;font-family:'Courier New',monospace;color:var(--accent);
      letter-spacing:1px;margin-top:2px}
    .mac-box .mac-copy{font-size:0.7rem;color:var(--text-dim);background:rgba(255,255,255,0.06);
      padding:4px 10px;border-radius:6px;white-space:nowrap}

    .badge-row{display:flex;flex-wrap:wrap;gap:8px;margin-top:10px}
    .badge{display:inline-flex;align-items:center;gap:5px;background:rgba(0,0,0,0.3);
      border-radius:8px;padding:6px 10px;font-size:0.75rem;border:1px solid var(--border)}
    .badge .b-label{color:var(--text-dim);text-transform:uppercase;letter-spacing:0.4px;font-size:0.65rem}
    .badge .b-value{font-weight:600;color:var(--text)}
    .badge .b-value.accent{color:var(--accent)}
    .badge .b-value.green{color:var(--accent-green)}
    .badge .b-value.orange{color:#f59e0b}

    /* === PRESET / QUICK SETUP === */
    .preset-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
    .preset-card{background:rgba(0,0,0,0.3);border:2px solid var(--border);border-radius:12px;
      padding:14px 10px;cursor:pointer;transition:all 0.25s;text-align:center;position:relative}
    .preset-card:hover{border-color:var(--accent);background:rgba(0,217,255,0.04);transform:translateY(-1px)}
    .preset-card:active{transform:scale(0.97)}
    .preset-card.selected{border-color:var(--accent-green);background:rgba(34,197,94,0.06);
      box-shadow:0 0 16px rgba(34,197,94,0.12)}
    .preset-card .p-title{font-weight:700;font-size:0.85rem;margin-bottom:4px}
    .preset-card .p-sub{font-size:0.7rem;color:var(--text-dim)}
    .preset-card .p-role{font-size:0.65rem;font-weight:600;margin-top:4px;padding:2px 8px;
      border-radius:10px;display:inline-block}
    .preset-card .p-role.master{background:rgba(34,197,94,0.15);color:var(--accent-green)}
    .preset-card .p-role.slave{background:rgba(245,158,11,0.15);color:#f59e0b}

    /* Mini griglia nel preset */
    .mini-grid{display:inline-grid;gap:3px;margin:8px auto 4px;width:fit-content}
    .mini-grid.mg-2x1{grid-template-columns:20px 20px}
    .mini-grid.mg-1x2{grid-template-columns:20px}
    .mini-grid.mg-2x2{grid-template-columns:20px 20px}
    .mini-cell{width:20px;height:20px;border-radius:3px;border:1.5px solid rgba(255,255,255,0.15);
      background:rgba(255,255,255,0.04)}
    .mini-cell.this{border-color:var(--accent-green);background:rgba(34,197,94,0.3)}

    .preset-sep{grid-column:1/-1;text-align:center;font-size:0.72rem;color:var(--text-dim);
      padding:6px 0 2px;font-weight:600;text-transform:uppercase;letter-spacing:1px}

    .btn{display:inline-flex;align-items:center;justify-content:center;gap:6px;padding:10px 20px;
      border-radius:10px;font-size:0.85rem;font-weight:600;font-family:inherit;cursor:pointer;
      border:none;transition:all 0.2s;text-decoration:none}
    .btn:active{transform:scale(0.97)}
    .btn-primary{background:linear-gradient(135deg,var(--accent),#0088cc);color:#000;
      box-shadow:0 4px 15px rgba(0,217,255,0.3)}
    .btn-primary:hover{box-shadow:0 6px 25px rgba(0,217,255,0.4)}
    .btn-secondary{background:rgba(255,255,255,0.08);color:var(--text);border:1px solid var(--border)}
    .btn-secondary:hover{background:rgba(255,255,255,0.12);border-color:var(--text-dim)}
    .btn-danger{background:rgba(239,68,68,0.15);color:var(--danger);border:1px solid rgba(239,68,68,0.3)}
    .btn-danger:hover{background:rgba(239,68,68,0.25)}
    .btn-sm{padding:6px 14px;font-size:0.78rem;border-radius:8px}
    .btn-block{width:100%;margin-top:12px}
    .btn-scan{width:100%;margin-top:14px;padding:12px;font-size:0.9rem}

    /* === GRID STATUS === */
    .grid-container{display:flex;flex-direction:column;align-items:center;gap:16px}
    .grid-visual{display:grid;gap:8px;width:100%;max-width:420px;transition:all 0.3s ease}
    .grid-visual.g1x1{grid-template-columns:1fr}
    .grid-visual.g2x1{grid-template-columns:1fr 1fr}
    .grid-visual.g1x2{grid-template-columns:1fr}
    .grid-visual.g2x2{grid-template-columns:1fr 1fr}

    .grid-cell{aspect-ratio:1;border-radius:12px;padding:14px;display:flex;flex-direction:column;
      align-items:center;justify-content:center;gap:6px;transition:all 0.25s ease;
      border:2px dashed rgba(255,255,255,0.15);background:rgba(0,0,0,0.3);position:relative;min-height:110px}
    .grid-cell.assigned{border:2px solid var(--accent);background:rgba(0,217,255,0.08);
      box-shadow:0 0 20px rgba(0,217,255,0.1)}
    .grid-cell.current{border:2px solid var(--accent-green);background:rgba(34,197,94,0.1);
      box-shadow:0 0 20px rgba(34,197,94,0.15)}
    .grid-cell .pos-label{font-size:0.75rem;color:var(--text-dim);font-weight:600;letter-spacing:1px;text-transform:uppercase}
    .grid-cell .cell-role{font-size:0.85rem;font-weight:700;color:var(--text)}
    .grid-cell .cell-mac{font-size:0.65rem;color:var(--text-dim);font-family:'Courier New',monospace;
      word-break:break-all;text-align:center;line-height:1.3}
    .grid-cell .cell-status{display:flex;align-items:center;gap:4px;font-size:0.7rem}
    .grid-cell .dot{width:6px;height:6px;border-radius:50%;flex-shrink:0}
    .grid-cell .dot.online{background:var(--accent-green);box-shadow:0 0 6px var(--accent-green)}
    .grid-cell .dot.offline{background:var(--danger)}
    .grid-cell .badge-this{position:absolute;top:8px;right:8px;background:var(--accent-green);color:#000;
      font-size:0.6rem;font-weight:800;padding:2px 6px;border-radius:4px;text-transform:uppercase;letter-spacing:0.5px}
    .grid-cell .empty-hint{font-size:0.75rem;color:rgba(255,255,255,0.3);text-align:center;line-height:1.4}

    .stat-row{display:flex;align-items:center;justify-content:space-between;padding:10px 14px;
      background:rgba(0,0,0,0.2);border-radius:8px;margin-top:10px}
    .stat-row .s-label{font-size:0.78rem;color:var(--text-dim)}
    .stat-row .s-value{font-size:0.88rem;font-weight:600}
    .stat-row .s-value.accent{color:var(--accent)}

    @keyframes framePulse{0%,100%{opacity:1}50%{opacity:0.5}}
    .frame-pulse{animation:framePulse 1s ease-in-out infinite}

    /* === ADVANCED === */
    .config-row{display:flex;align-items:center;justify-content:space-between;
      padding:12px 0;border-bottom:1px solid var(--border)}
    .config-row:last-child{border-bottom:none}
    .config-label{font-weight:500;font-size:0.88rem;flex:1}
    .config-desc{font-size:0.72rem;color:var(--text-dim);margin-top:2px}

    select{background:rgba(0,0,0,0.4);color:var(--text);border:1px solid var(--border);border-radius:8px;
      padding:8px 12px;font-size:0.85rem;font-family:inherit;cursor:pointer;min-width:150px;
      transition:border-color 0.2s;-webkit-appearance:none;appearance:none;
      background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' fill='%2394a3b8' viewBox='0 0 16 16'%3E%3Cpath d='M8 11L3 6h10z'/%3E%3C/svg%3E");
      background-repeat:no-repeat;background-position:right 10px center;padding-right:30px}
    select:focus{outline:none;border-color:var(--accent)}

    .peer-list{display:flex;flex-direction:column;gap:10px;margin-top:12px}
    .peer-item{display:flex;align-items:center;gap:12px;background:rgba(0,0,0,0.25);
      border-radius:10px;padding:12px 14px;border:1px solid var(--border);transition:border-color 0.2s}
    .peer-item:hover{border-color:var(--border-accent)}
    .peer-dot{width:10px;height:10px;border-radius:50%;flex-shrink:0}
    .peer-dot.online{background:var(--accent-green);box-shadow:0 0 8px var(--accent-green)}
    .peer-dot.offline{background:var(--danger);box-shadow:0 0 8px rgba(239,68,68,0.5)}
    .peer-info{flex:1;min-width:0}
    .peer-mac{font-size:0.82rem;font-weight:600;font-family:'Courier New',monospace}
    .peer-meta{font-size:0.72rem;color:var(--text-dim);margin-top:2px}
    .peer-actions{display:flex;gap:6px;flex-shrink:0}

    .add-peer-form{display:flex;gap:8px;margin-top:12px;padding-top:12px;border-top:1px solid var(--border)}
    .add-peer-form input{flex:1;background:rgba(0,0,0,0.4);color:var(--text);border:1px solid var(--border);
      border-radius:8px;padding:8px 12px;font-size:0.85rem;font-family:'Courier New',monospace;transition:border-color 0.2s}
    .add-peer-form input:focus{outline:none;border-color:var(--accent)}
    .add-peer-form input::placeholder{color:rgba(255,255,255,0.25)}

    .adv-sub{font-size:0.8rem;color:var(--text-dim);font-weight:600;text-transform:uppercase;
      letter-spacing:0.5px;margin:16px 0 8px;padding-bottom:6px;border-bottom:1px solid var(--border)}
    .adv-sub:first-child{margin-top:0}

    .test-buttons{display:flex;gap:10px;flex-wrap:wrap;margin-top:10px}

    .toast{position:fixed;top:50%;left:50%;transform:translate(-50%,-50%) scale(0.8);opacity:0;
      background:rgba(10,10,20,0.95);border:2px solid var(--border-accent);border-radius:16px;
      padding:18px 28px;font-size:1.1rem;font-weight:600;z-index:9999;
      transition:transform 0.3s ease,opacity 0.3s ease;
      box-shadow:0 10px 60px rgba(0,0,0,0.8);text-align:center;pointer-events:none}
    .toast.show{transform:translate(-50%,-50%) scale(1);opacity:1}
    .toast.success{border-color:var(--accent-green);color:var(--accent-green)}
    .toast.error{border-color:var(--danger);color:var(--danger)}
    @keyframes cardFlash{0%{box-shadow:0 0 0 0 rgba(0,217,255,0.6)}50%{box-shadow:0 0 30px 10px rgba(0,217,255,0.4)}100%{box-shadow:none}}
    .card-flash{animation:cardFlash 1s ease}

    .footer{text-align:center;padding:16px 0;color:rgba(255,255,255,0.25);font-size:0.72rem;margin-top:10px}

    @media(max-width:500px){
      .container{padding:10px}
      .header h1{font-size:1.3rem}
      .preset-grid{grid-template-columns:1fr 1fr}
      .badge-row{gap:6px}
      .badge{padding:5px 8px;font-size:0.7rem}
      .grid-cell{min-height:90px;padding:10px}
      .add-peer-form{flex-direction:column}
      .test-buttons{flex-direction:column}
      .test-buttons .btn{width:100%}
      .config-row{flex-direction:column;align-items:flex-start;gap:8px}
      .config-row select{width:100%}
    }
    ::-webkit-scrollbar{width:6px}
    ::-webkit-scrollbar-track{background:transparent}
    ::-webkit-scrollbar-thumb{background:rgba(255,255,255,0.15);border-radius:3px}
  </style>
</head>
<body>
  <div class="bg-grid"></div>
  <div id="toast" class="toast"></div>

  <div class="container">
    <a href="/" class="nav-back">&larr; Torna alla Home</a>

    <div class="header">
      <h1><span>Multi-Display</span> Config</h1>
      <p class="sub">Pannelli sincronizzati via ESP-NOW &mdash; Griglia fino a 2x2</p>
    </div>

    <!-- ==================== 1. POWER CARD ==================== -->
    <div class="section power-card inactive" id="powerCard">
      <div class="section-head">
        <div class="icon" style="background:rgba(0,217,255,0.15)">&#9889;</div>
        Multi-Display
      </div>
      <div class="section-body">
        <div class="power-row">
          <div style="display:flex;align-items:center">
            <span class="power-label">Attiva</span>
            <span class="power-status off" id="powerBadge">OFF</span>
          </div>
          <label class="toggle">
            <input type="checkbox" id="toggleEnabled" onchange="toggleEnabled(this.checked)">
            <span class="toggle-slider"></span>
          </label>
        </div>
        <div class="mac-box" onclick="copyMac()" title="Tap per copiare il MAC">
          <div>
            <div class="mac-label">Il mio MAC Address</div>
            <div class="mac-addr" id="bMacBig">%%MAC%%</div>
          </div>
          <div class="mac-copy" id="macCopyLabel">&#128203; Copia</div>
        </div>
        <div class="badge-row" id="badgeRow">
          <div class="badge"><span class="b-label">Ruolo</span><span class="b-value green" id="bRole">--</span></div>
          <div class="badge"><span class="b-label">Griglia</span><span class="b-value" id="bGrid">--</span></div>
          <div class="badge"><span class="b-label">Posizione</span><span class="b-value" id="bPos">--</span></div>
          <div class="badge"><span class="b-label">Peer</span><span class="b-value" id="bPeers">0</span></div>
        </div>
      </div>
    </div>

    <!-- ==================== 2. CONFIGURAZIONE RAPIDA ==================== -->
    <div class="section">
      <div class="section-head">
        <div class="icon" style="background:rgba(233,69,96,0.15)">&#128640;</div>
        Configurazione Rapida
      </div>
      <div class="section-body">
        <p style="font-size:0.78rem;color:var(--text-dim);margin-bottom:12px">
          Un click per configurare questo pannello. Seleziona la posizione desiderata.
        </p>
        <div class="preset-grid" id="presetGrid">
          <!-- 2x1 Orizzontale -->
          <div class="preset-sep">2x1 &mdash; Orizzontale</div>
          <div class="preset-card" id="pre_2x1_0_0" onclick="quickSetup(2,1,0,0,1)">
            <div class="mini-grid mg-2x1">
              <div class="mini-cell this"></div><div class="mini-cell"></div>
            </div>
            <div class="p-title">Pannello SINISTRO</div>
            <div class="p-sub">Posizione 0,0</div>
            <div class="p-role master">MASTER</div>
          </div>
          <div class="preset-card" id="pre_2x1_1_0" onclick="quickSetup(2,1,1,0,2)">
            <div class="mini-grid mg-2x1">
              <div class="mini-cell"></div><div class="mini-cell this"></div>
            </div>
            <div class="p-title">Pannello DESTRO</div>
            <div class="p-sub">Posizione 1,0</div>
            <div class="p-role slave">SLAVE</div>
          </div>

          <!-- 1x2 Verticale -->
          <div class="preset-sep">1x2 &mdash; Verticale</div>
          <div class="preset-card" id="pre_1x2_0_0" onclick="quickSetup(1,2,0,0,1)">
            <div class="mini-grid mg-1x2">
              <div class="mini-cell this"></div><div class="mini-cell"></div>
            </div>
            <div class="p-title">Pannello ALTO</div>
            <div class="p-sub">Posizione 0,0</div>
            <div class="p-role master">MASTER</div>
          </div>
          <div class="preset-card" id="pre_1x2_0_1" onclick="quickSetup(1,2,0,1,2)">
            <div class="mini-grid mg-1x2">
              <div class="mini-cell"></div><div class="mini-cell this"></div>
            </div>
            <div class="p-title">Pannello BASSO</div>
            <div class="p-sub">Posizione 0,1</div>
            <div class="p-role slave">SLAVE</div>
          </div>

          <!-- 2x2 Quad -->
          <div class="preset-sep">2x2 &mdash; Quad</div>
          <div class="preset-card" id="pre_2x2_0_0" onclick="quickSetup(2,2,0,0,1)">
            <div class="mini-grid mg-2x2">
              <div class="mini-cell this"></div><div class="mini-cell"></div>
              <div class="mini-cell"></div><div class="mini-cell"></div>
            </div>
            <div class="p-title">ALTO-SX</div>
            <div class="p-sub">Posizione 0,0</div>
            <div class="p-role master">MASTER</div>
          </div>
          <div class="preset-card" id="pre_2x2_1_0" onclick="quickSetup(2,2,1,0,2)">
            <div class="mini-grid mg-2x2">
              <div class="mini-cell"></div><div class="mini-cell this"></div>
              <div class="mini-cell"></div><div class="mini-cell"></div>
            </div>
            <div class="p-title">ALTO-DX</div>
            <div class="p-sub">Posizione 1,0</div>
            <div class="p-role slave">SLAVE</div>
          </div>
          <div class="preset-card" id="pre_2x2_0_1" onclick="quickSetup(2,2,0,1,2)">
            <div class="mini-grid mg-2x2">
              <div class="mini-cell"></div><div class="mini-cell"></div>
              <div class="mini-cell this"></div><div class="mini-cell"></div>
            </div>
            <div class="p-title">BASSO-SX</div>
            <div class="p-sub">Posizione 0,1</div>
            <div class="p-role slave">SLAVE</div>
          </div>
          <div class="preset-card" id="pre_2x2_1_1" onclick="quickSetup(2,2,1,1,2)">
            <div class="mini-grid mg-2x2">
              <div class="mini-cell"></div><div class="mini-cell"></div>
              <div class="mini-cell"></div><div class="mini-cell this"></div>
            </div>
            <div class="p-title">BASSO-DX</div>
            <div class="p-sub">Posizione 1,1</div>
            <div class="p-role slave">SLAVE</div>
          </div>
        </div>

        <button class="btn btn-primary btn-scan" onclick="scanPeers()">
          &#128225; Cerca Pannelli Automaticamente
        </button>
      </div>
    </div>

    <!-- ==================== 3. STATO GRIGLIA (collapsabile, aperta) ==================== -->
    <div class="section open" id="secGrid">
      <div class="section-head collapsible" onclick="toggleSection('secGrid')">
        <div class="icon" style="background:rgba(34,197,94,0.15)">&#9638;</div>
        Stato Griglia
        <span class="arrow">&#9660;</span>
      </div>
      <div class="section-collapse">
        <div class="section-body">
          <div class="grid-container">
            <div id="gridVisual" class="grid-visual g1x1"></div>
          </div>
          <div class="stat-row" style="margin-top:14px">
            <span class="s-label">Peer connessi</span>
            <span class="s-value" id="peerOnline">0 / 0</span>
          </div>
          <div class="stat-row">
            <span class="s-label">Frame Counter</span>
            <span class="s-value accent"><span id="frameCount" class="frame-pulse">0</span></span>
          </div>
          <div class="stat-row">
            <span class="s-label">Risoluzione virtuale</span>
            <span class="s-value accent" id="virtualRes">480 x 480</span>
          </div>
        </div>
      </div>
    </div>

    <!-- ==================== 4. IMPOSTAZIONI AVANZATE (collapsabile, chiusa) ==================== -->
    <div class="section" id="secAdvanced">
      <div class="section-head collapsible" onclick="toggleSection('secAdvanced')">
        <div class="icon" style="background:rgba(245,158,11,0.15)">&#9881;</div>
        Impostazioni Avanzate
        <span class="arrow">&#9660;</span>
      </div>
      <div class="section-collapse">
        <div class="section-body">

          <!-- MAC Address -->
          <div class="adv-sub">Informazioni Dispositivo</div>
          <div class="config-row">
            <div>
              <div class="config-label">MAC Address</div>
              <div class="config-desc">Identificativo unico di questo pannello</div>
            </div>
            <span id="advMac" style="font-family:'Courier New',monospace;font-weight:700;color:var(--accent);font-size:0.9rem;letter-spacing:1px">%%MAC%%</span>
          </div>

          <!-- Config manuale -->
          <div class="adv-sub" style="margin-top:16px">Configurazione Manuale</div>
          <div class="config-row">
            <div>
              <div class="config-label">Layout Griglia</div>
              <div class="config-desc">Numero di pannelli nella configurazione</div>
            </div>
            <select id="selLayout">
              <option value="1x1">1x1 - Singolo</option>
              <option value="2x1">2x1 - Duo Orizzontale</option>
              <option value="1x2">1x2 - Duo Verticale</option>
              <option value="2x2">2x2 - Quad</option>
            </select>
          </div>
          <div class="config-row">
            <div>
              <div class="config-label">Ruolo</div>
              <div class="config-desc">Master sincronizza, Slave riceve</div>
            </div>
            <select id="selRole">
              <option value="0">Standalone</option>
              <option value="1">Master</option>
              <option value="2">Slave</option>
            </select>
          </div>
          <div class="config-row">
            <div>
              <div class="config-label">Posizione X</div>
              <div class="config-desc">Colonna nella griglia (0-1)</div>
            </div>
            <select id="selPosX">
              <option value="0">0</option>
              <option value="1">1</option>
            </select>
          </div>
          <div class="config-row">
            <div>
              <div class="config-label">Posizione Y</div>
              <div class="config-desc">Riga nella griglia (0-1)</div>
            </div>
            <select id="selPosY">
              <option value="0">0</option>
              <option value="1">1</option>
            </select>
          </div>
          <button class="btn btn-primary btn-block" onclick="saveManualConfig()">
            &#128190; Salva Configurazione Manuale
          </button>

          <!-- Gestione Peer -->
          <div class="adv-sub" style="margin-top:24px">Gestione Peer</div>
          <div id="peerList" class="peer-list">
            <div style="text-align:center;color:var(--text-dim);font-size:0.85rem;padding:12px 0">
              Nessun peer connesso
            </div>
          </div>
          <div class="add-peer-form">
            <input type="text" id="inputPeerMac" placeholder="AA:BB:CC:DD:EE:FF" maxlength="17"
                   oninput="formatMacInput(this)">
            <button class="btn btn-primary btn-sm" onclick="addPeer()">Aggiungi</button>
          </div>
          <div style="margin-top:10px">
            <button class="btn btn-secondary" style="width:100%" onclick="scanPeers()">
              &#128225; Scansione Automatica
            </button>
          </div>

          <!-- Diagnostica -->
          <div class="adv-sub" style="margin-top:24px">Test e Diagnostica</div>
          <div class="test-buttons">
            <button class="btn btn-secondary" onclick="testSync()">&#128260; Test Sync</button>
            <button class="btn btn-secondary" onclick="flashPanel()">&#128161; Lampeggia</button>
          </div>

          <!-- Reset -->
          <div class="adv-sub" style="margin-top:24px">Zona Pericolosa</div>
          <button class="btn btn-danger btn-block" onclick="resetAll()">
            &#9888; Reset Completo Multi-Display
          </button>

        </div>
      </div>
    </div>

    <!-- ==================== 5. FOOTER ==================== -->
    <div class="footer">OraQuadra Nano v1.5 &bull; Multi-Display Configuration</div>
  </div>

  <script>
    let S={enabled:%%ENABLED%%,myMAC:'%%MAC%%',role:%%ROLE%%,panelX:%%PANELX%%,panelY:%%PANELY%%,gridW:%%GRIDW%%,gridH:%%GRIDH%%,
      virtualW:%%VIRTUALW%%,virtualH:%%VIRTUALH%%,frameCounter:0,peers:[],discovery:false,peerCount:%%PEERCOUNT%%};
    let refreshId=null;

    document.addEventListener('DOMContentLoaded',()=>{
      updateUI();
      fetchStatus();
      refreshId=setInterval(fetchStatus,2000);
    });

    // === FETCH STATUS ===
    let userToggling=false;
    async function fetchStatus(){
      try{
        const r=await fetch('/dualdisplay/status?t='+Date.now(),{cache:'no-store'});
        if(!r.ok)return;
        const d=await r.json();
        const wasEnabled=S.enabled;
        Object.assign(S,d);
        updateUI();
        // Notifica se lo stato enabled e' cambiato da remoto (push dal master)
        if(S.enabled!==wasEnabled && !userToggling){
          showToast(S.enabled?'Multi-Display ATTIVATO dal master':'Multi-Display DISATTIVATO dal master', S.enabled?'success':'error');
          const pc=document.getElementById('powerCard');
          if(pc){pc.classList.remove('card-flash');void pc.offsetWidth;pc.classList.add('card-flash');}
        }
        userToggling=false;
      }catch(e){console.error(e)}
    }

    const roleNames=['Standalone','Master','Slave'];
    const posLabels=[['TL','TR'],['BL','BR']];

    // === UPDATE UI ===
    function updateUI(){
      // Power card
      const pc=document.getElementById('powerCard');
      const pb=document.getElementById('powerBadge');
      const tog=document.getElementById('toggleEnabled');
      tog.checked=S.enabled;
      if(S.enabled){
        pc.classList.remove('inactive');pc.classList.add('active');
        pb.textContent='ON';pb.className='power-status on';
      }else{
        pc.classList.remove('active');pc.classList.add('inactive');
        pb.textContent='OFF';pb.className='power-status off';
      }

      // MAC box + Badges + Advanced MAC
      document.getElementById('bMacBig').textContent=S.myMAC||'--:--:--:--:--:--';
      const advMacEl=document.getElementById('advMac');
      if(advMacEl) advMacEl.textContent=S.myMAC||'--:--:--:--:--:--';
      const br=document.getElementById('bRole');
      br.textContent=S.roleName||roleNames[S.role]||'?';
      br.className='b-value '+(S.role===1?'green':S.role===2?'orange':'accent');
      document.getElementById('bGrid').textContent=S.gridW+'x'+S.gridH;
      document.getElementById('bPos').textContent=S.panelX+','+S.panelY;
      const onPeers=S.peers?S.peers.filter(p=>p.active).length:0;
      const totalPeers=S.peers?S.peers.length:0;
      document.getElementById('bPeers').textContent=String(totalPeers);

      // Grid status stats
      document.getElementById('peerOnline').textContent=onPeers+' / '+totalPeers;
      document.getElementById('virtualRes').textContent=S.virtualW+' x '+S.virtualH;
      document.getElementById('frameCount').textContent=S.frameCounter.toLocaleString('it-IT');

      // Advanced dropdowns (only if not focused)
      const sl=document.getElementById('selLayout');
      if(document.activeElement!==sl) sl.value=S.gridW+'x'+S.gridH;
      const sr=document.getElementById('selRole');
      if(document.activeElement!==sr) sr.value=String(S.role);
      const sx=document.getElementById('selPosX');
      if(document.activeElement!==sx) sx.value=String(S.panelX);
      const sy=document.getElementById('selPosY');
      if(document.activeElement!==sy) sy.value=String(S.panelY);

      renderGrid();
      renderPeers();
      highlightPreset();
    }

    // === GRID VISUAL ===
    function renderGrid(){
      const c=document.getElementById('gridVisual');
      const gw=S.gridW,gh=S.gridH;
      c.className='grid-visual g'+gw+'x'+gh;
      let h='';
      for(let y=0;y<gh;y++){
        for(let x=0;x<gw;x++){
          const isCur=(x===S.panelX&&y===S.panelY);
          const peer=findPeerAt(x,y);
          let cls='grid-cell';
          if(isCur) cls+=' current';
          else if(peer) cls+=' assigned';
          const lbl=posLabels[y]&&posLabels[y][x]?posLabels[y][x]:'?';
          h+='<div class="'+cls+'">';
          if(isCur) h+='<span class="badge-this">THIS</span>';
          h+='<span class="pos-label">'+lbl+' ('+x+','+y+')</span>';
          if(isCur){
            h+='<span class="cell-role">'+(roleNames[S.role]||'?')+'</span>';
            h+='<span class="cell-mac">'+(S.myMAC||'--')+'</span>';
            h+='<span class="cell-status"><span class="dot online"></span> Online</span>';
          }else if(peer){
            h+='<span class="cell-role">Peer #'+peer.index+'</span>';
            h+='<span class="cell-mac">'+peer.mac+'</span>';
            const ac=peer.active;
            h+='<span class="cell-status"><span class="dot '+(ac?'online':'offline')+'"></span> '+(ac?'Online':'Offline')+'</span>';
          }else{
            h+='<span class="empty-hint">Vuoto</span>';
          }
          h+='</div>';
        }
      }
      c.innerHTML=h;
    }

    function findPeerAt(x,y){
      if(!S.peers)return null;
      for(let i=0;i<S.peers.length;i++){
        if(S.peers[i].peerPanelX===x&&S.peers[i].peerPanelY===y) return S.peers[i];
      }
      return null;
    }

    // === PEERS ===
    function renderPeers(){
      const c=document.getElementById('peerList');
      if(!S.peers||S.peers.length===0){
        c.innerHTML='<div style="text-align:center;color:var(--text-dim);font-size:0.85rem;padding:12px 0">Nessun peer connesso</div>';
        return;
      }
      let h='';
      S.peers.forEach((p,i)=>{
        const ac=p.active;
        h+='<div class="peer-item">';
        h+='<div class="peer-dot '+(ac?'online':'offline')+'"></div>';
        h+='<div class="peer-info">';
        h+='<div class="peer-mac">'+p.mac+'</div>';
        h+='<div class="peer-meta">'+(ac?'Online':'Offline')+' &bull; Pos: '+p.peerPanelX+','+p.peerPanelY+'</div>';
        h+='</div>';
        h+='<div class="peer-actions">';
        h+='<button class="btn btn-danger btn-sm" onclick="removePeer('+p.index+')">Rimuovi</button>';
        h+='</div></div>';
      });
      c.innerHTML=h;
    }

    // === COLLAPSIBLE SECTIONS ===
    function toggleSection(id){
      document.getElementById(id).classList.toggle('open');
    }

    // === QUICK SETUP (core function) ===
    async function quickSetup(gW,gH,pX,pY,role){
      const rName=role===1?'Master':'Slave';
      const fd=new URLSearchParams();
      fd.append('enabled','1');
      fd.append('gridW',String(gW));
      fd.append('gridH',String(gH));
      fd.append('panelX',String(pX));
      fd.append('panelY',String(pY));
      fd.append('role',String(role));
      try{
        const r=await fetch('/dualdisplay/config',{method:'POST',body:fd});
        if(r.ok){
          showToast('Configurato come '+rName+' in griglia '+gW+'x'+gH+' ('+pX+','+pY+')','success');
          // Se slave, auto-scan per trovare il master
          if(role===2){
            setTimeout(async()=>{
              try{
                await fetch('/dualdisplay/scan',{method:'POST'});
              }catch(e){}
            },300);
          }
          setTimeout(fetchStatus,500);
        }else{
          showToast('Errore configurazione','error');
        }
      }catch(e){showToast('Errore connessione','error')}
    }

    // === COPY MAC ===
    function copyMac(){
      const mac=S.myMAC||'';
      if(!mac||mac==='--')return;
      if(navigator.clipboard){
        navigator.clipboard.writeText(mac).then(()=>{
          showToast('MAC copiato: '+mac,'success');
          document.getElementById('macCopyLabel').innerHTML='&#10003; Copiato!';
          setTimeout(()=>{document.getElementById('macCopyLabel').innerHTML='&#128203; Copia'},2000);
        }).catch(()=>fallbackCopy(mac));
      }else{fallbackCopy(mac)}
    }
    function fallbackCopy(text){
      const ta=document.createElement('textarea');ta.value=text;ta.style.position='fixed';ta.style.opacity='0';
      document.body.appendChild(ta);ta.select();document.execCommand('copy');document.body.removeChild(ta);
      showToast('MAC copiato: '+text,'success');
    }

    // === HIGHLIGHT ACTIVE PRESET ===
    function highlightPreset(){
      // Remove all selected
      document.querySelectorAll('.preset-card').forEach(c=>c.classList.remove('selected'));
      if(!S.enabled)return;
      const id='pre_'+S.gridW+'x'+S.gridH+'_'+S.panelX+'_'+S.panelY;
      const el=document.getElementById(id);
      if(el) el.classList.add('selected');
    }

    // === TOGGLE ON/OFF ===
    async function toggleEnabled(val){
      userToggling=true;
      try{
        const fd=new URLSearchParams();
        fd.append('enabled',val?'1':'0');
        const r=await fetch('/dualdisplay/config',{method:'POST',body:fd});
        if(r.ok){showToast(val?'Multi-Display attivato':'Multi-Display disattivato','success');setTimeout(fetchStatus,300)}
        else showToast('Errore','error');
      }catch(e){showToast('Errore connessione','error')}
    }

    // === SAVE MANUAL CONFIG ===
    async function saveManualConfig(){
      const v=document.getElementById('selLayout').value.split('x');
      const role=document.getElementById('selRole').value;
      const pX=document.getElementById('selPosX').value;
      const pY=document.getElementById('selPosY').value;
      const fd=new URLSearchParams();
      fd.append('enabled',document.getElementById('toggleEnabled').checked?'1':'0');
      fd.append('gridW',v[0]);fd.append('gridH',v[1]);
      fd.append('role',role);
      fd.append('panelX',pX);fd.append('panelY',pY);
      try{
        const r=await fetch('/dualdisplay/config',{method:'POST',body:fd});
        if(r.ok){showToast('Configurazione salvata!','success');setTimeout(fetchStatus,300)}
        else showToast('Errore salvataggio','error');
      }catch(e){showToast('Errore connessione','error')}
    }

    // === ADD / REMOVE PEER ===
    async function addPeer(){
      const mac=document.getElementById('inputPeerMac').value.trim().toUpperCase();
      if(!mac||mac.length<17){showToast('MAC non valido (AA:BB:CC:DD:EE:FF)','error');return}
      const fd=new URLSearchParams();fd.append('mac',mac);
      try{
        const r=await fetch('/dualdisplay/addpeer',{method:'POST',body:fd});
        if(r.ok){document.getElementById('inputPeerMac').value='';showToast('Peer aggiunto: '+mac,'success');setTimeout(fetchStatus,300)}
        else{const d=await r.json().catch(()=>({}));showToast(d.error||'Errore','error')}
      }catch(e){showToast('Errore connessione','error')}
    }

    async function removePeer(idx){
      if(!confirm('Rimuovere questo peer?'))return;
      const fd=new URLSearchParams();fd.append('index',String(idx));
      try{
        const r=await fetch('/dualdisplay/removepeer',{method:'POST',body:fd});
        if(r.ok){showToast('Peer rimosso','success');setTimeout(fetchStatus,300)}
        else showToast('Errore rimozione','error');
      }catch(e){showToast('Errore connessione','error')}
    }

    // === SCAN ===
    async function scanPeers(){
      showToast('Scansione in corso...','success');
      try{
        const r=await fetch('/dualdisplay/scan',{method:'POST'});
        if(r.ok){const d=await r.json().catch(()=>({}));
          showToast('Trovati '+String(d.newPeersFound||0)+' nuovi pannelli','success');setTimeout(fetchStatus,500)}
        else showToast('Errore scansione','error');
      }catch(e){showToast('Errore connessione','error')}
    }

    // === TEST / FLASH ===
    async function testSync(){
      try{
        const r=await fetch('/dualdisplay/test',{method:'POST'});
        if(r.ok){const d=await r.json().catch(()=>({}));
          showToast('Test sync inviato a '+(d.sentTo||0)+' peer','success');}
        else showToast('Errore test sync','error');
      }catch(e){showToast('Errore connessione','error')}
    }

    async function flashPanel(){
      try{
        const r=await fetch('/dualdisplay/flash',{method:'POST'});
        if(r.ok) showToast('Lampeggio pannello eseguito!','success');
        else showToast('Errore lampeggio','error');
      }catch(e){showToast('Errore connessione','error')}
    }

    // === RESET ===
    async function resetAll(){
      if(!confirm('Vuoi davvero resettare tutta la configurazione Multi-Display?\n\nVerranno rimossi tutti i peer e disabilitato il sistema.'))return;
      try{
        const r=await fetch('/dualdisplay/reset',{method:'POST'});
        if(r.ok){showToast('Reset completo eseguito','success');setTimeout(fetchStatus,500)}
        else showToast('Errore reset','error');
      }catch(e){showToast('Errore connessione','error')}
    }

    // === MAC INPUT FORMATTER ===
    function formatMacInput(el){
      let v=el.value.replace(/[^0-9A-Fa-f]/g,'').toUpperCase();
      let f='';for(let i=0;i<v.length&&i<12;i++){if(i>0&&i%2===0)f+=':';f+=v[i]}
      el.value=f;
    }

    // === TOAST ===
    let toastTmr=null;
    function showToast(msg,type){
      const t=document.getElementById('toast');t.textContent=msg;
      t.className='toast '+(type||'');void t.offsetWidth;t.classList.add('show');
      clearTimeout(toastTmr);toastTmr=setTimeout(()=>t.classList.remove('show'),3500);
    }
  </script>
</body>
</html>
)rawliteral";
