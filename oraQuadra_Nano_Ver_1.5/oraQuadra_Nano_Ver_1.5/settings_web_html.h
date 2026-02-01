#ifndef SETTINGS_WEB_HTML_H
#define SETTINGS_WEB_HTML_H

// HTML per pagina web SETTINGS - Configurazione completa OraQuadra Nano
// Tutte le impostazioni organizzate per tipologia in sezioni collassabili
// Include attivazione/disattivazione delle singole modalità

const char SETTINGS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>OraQuadra Nano - Impostazioni</title>
<style>
:root {
  --primary: #6366f1;
  --primary-dark: #4f46e5;
  --success: #22c55e;
  --warning: #f59e0b;
  --danger: #ef4444;
  --bg: #0f172a;
  --card: #1e293b;
  --card-hover: #334155;
  --text: #f1f5f9;
  --text-muted: #94a3b8;
  --border: #334155;
}
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  background: var(--bg);
  color: var(--text);
  min-height: 100vh;
  padding: 10px;
  font-size: 14px;
}
.container { max-width: 800px; margin: 0 auto; }
header {
  text-align: center;
  padding: 15px 10px;
  background: linear-gradient(135deg, var(--primary), #8b5cf6);
  border-radius: 12px;
  margin-bottom: 15px;
}
header h1 { font-size: 1.4rem; margin-bottom: 5px; }
header p { font-size: 0.85rem; opacity: 0.9; }

.section {
  background: var(--card);
  border-radius: 10px;
  margin-bottom: 10px;
  overflow: hidden;
  border: 1px solid var(--border);
}
.section-header {
  display: flex;
  align-items: center;
  padding: 12px 15px;
  cursor: pointer;
  background: var(--card-hover);
  transition: background 0.2s;
}
.section-header:hover { background: #3b4a5f; }
.section-icon {
  width: 32px;
  height: 32px;
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
  margin-right: 12px;
  font-size: 16px;
}
.section-title { flex: 1; font-weight: 600; font-size: 0.95rem; }
.section-arrow {
  transition: transform 0.3s;
  font-size: 12px;
  color: var(--text-muted);
}
.section.open .section-arrow { transform: rotate(180deg); }
.section-content {
  max-height: 0;
  overflow: hidden;
  transition: max-height 0.3s ease-out;
}
.section.open .section-content { max-height: 3000px; }
.section-inner { padding: 15px; }

.setting-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 10px 0;
  border-bottom: 1px solid var(--border);
}
.setting-row:last-child { border-bottom: none; }
.setting-info { flex: 1; margin-right: 15px; }
.setting-label { font-weight: 500; font-size: 0.9rem; }
.setting-desc { font-size: 0.75rem; color: var(--text-muted); margin-top: 2px; }

.toggle {
  position: relative;
  width: 48px;
  height: 26px;
  flex-shrink: 0;
}
.toggle input { opacity: 0; width: 0; height: 0; }
.toggle-slider {
  position: absolute;
  cursor: pointer;
  inset: 0;
  background: #475569;
  border-radius: 26px;
  transition: 0.3s;
}
.toggle-slider:before {
  content: '';
  position: absolute;
  height: 20px;
  width: 20px;
  left: 3px;
  bottom: 3px;
  background: white;
  border-radius: 50%;
  transition: 0.3s;
}
.toggle input:checked + .toggle-slider { background: var(--success); }
.toggle input:checked + .toggle-slider:before { transform: translateX(22px); }

.range-group { display: flex; align-items: center; gap: 10px; }
.range-value {
  min-width: 45px;
  text-align: center;
  background: var(--bg);
  padding: 4px 8px;
  border-radius: 6px;
  font-weight: 600;
  font-size: 0.85rem;
}
input[type="range"] {
  -webkit-appearance: none;
  width: 100px;
  height: 6px;
  background: #475569;
  border-radius: 3px;
  outline: none;
}
input[type="range"]::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 18px;
  height: 18px;
  background: var(--primary);
  border-radius: 50%;
  cursor: pointer;
}

select {
  background: var(--bg);
  color: var(--text);
  border: 1px solid var(--border);
  padding: 8px 12px;
  border-radius: 6px;
  font-size: 0.85rem;
  cursor: pointer;
  min-width: 120px;
}

input[type="color"] {
  width: 40px;
  height: 30px;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  background: transparent;
}

input[type="number"] {
  background: var(--bg);
  color: var(--text);
  border: 1px solid var(--border);
  padding: 6px 10px;
  border-radius: 6px;
  width: 70px;
  text-align: center;
  font-size: 0.9rem;
}

.time-group { display: flex; align-items: center; gap: 8px; }
.time-group span { color: var(--text-muted); }

.btn {
  padding: 10px 20px;
  border: none;
  border-radius: 8px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s;
  font-size: 0.9rem;
}
.btn-primary { background: var(--primary); color: white; }
.btn-primary:hover { background: var(--primary-dark); }
.btn-danger { background: var(--danger); color: white; }
.btn-danger:hover { background: #dc2626; }
.btn-success { background: var(--success); color: white; }
.btn-success:hover { background: #16a34a; }
.btn-small { padding: 6px 12px; font-size: 0.8rem; }

.status-bar {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  background: var(--card);
  padding: 10px 15px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  border-top: 1px solid var(--border);
  z-index: 100;
}
.status-text { font-size: 0.85rem; }
.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  display: inline-block;
  margin-right: 8px;
}
.status-dot.online { background: var(--success); }
.status-dot.offline { background: var(--danger); }
.status-dot.saving { background: var(--warning); animation: pulse 1s infinite; }
@keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }

.icon-display { background: linear-gradient(135deg, #f59e0b, #d97706); }
.icon-time { background: linear-gradient(135deg, #22c55e, #16a34a); }
.icon-audio { background: linear-gradient(135deg, #ec4899, #db2777); }
.icon-effects { background: linear-gradient(135deg, #8b5cf6, #7c3aed); }
.icon-modes { background: linear-gradient(135deg, #14b8a6, #0d9488); }
.icon-sensors { background: linear-gradient(135deg, #06b6d4, #0891b2); }
.icon-network { background: linear-gradient(135deg, #3b82f6, #2563eb); }
.icon-system { background: linear-gradient(135deg, #64748b, #475569); }

/* Griglia modalità */
.modes-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(140px, 1fr));
  gap: 10px;
  margin-top: 10px;
}
.mode-card {
  background: var(--bg);
  border: 2px solid var(--border);
  border-radius: 10px;
  padding: 10px;
  text-align: center;
  transition: all 0.2s;
  position: relative;
  cursor: pointer;
}
.mode-card:hover { transform: scale(1.02); }
.mode-card.enabled { border-color: var(--success); }
.mode-card.active {
  border-color: var(--warning);
  background: rgba(245, 158, 11, 0.15);
  box-shadow: 0 0 20px rgba(245, 158, 11, 0.5);
}
.mode-card.disabled { opacity: 0.4; cursor: not-allowed; }
.mode-card.disabled:hover { transform: none; }
.mode-icon { font-size: 28px; margin-bottom: 4px; }
.mode-name { font-size: 0.8rem; font-weight: 600; margin-bottom: 6px; }
.mode-toggle { margin-top: 4px; transform: scale(0.85); }
.mode-badge {
  position: absolute;
  top: -8px;
  left: 50%;
  transform: translateX(-50%);
  background: var(--warning);
  color: black;
  font-size: 0.65rem;
  font-weight: 700;
  padding: 2px 8px;
  border-radius: 10px;
}
.mode-status {
  font-size: 0.65rem;
  color: var(--text-muted);
  margin-top: 4px;
}

.info-card {
  background: var(--bg);
  border-radius: 8px;
  padding: 10px;
  margin-bottom: 10px;
}
.info-row {
  display: flex;
  justify-content: space-between;
  padding: 5px 0;
  font-size: 0.85rem;
}
.info-label { color: var(--text-muted); }
.info-value { font-weight: 600; }

.nav-links {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
  justify-content: center;
  padding: 10px;
}
.nav-link {
  color: var(--primary);
  text-decoration: none;
  padding: 8px 15px;
  background: var(--card);
  border-radius: 8px;
  font-size: 0.85rem;
  transition: background 0.2s;
}
.nav-link:hover { background: var(--card-hover); }

.actions {
  display: flex;
  gap: 10px;
  padding: 15px;
  justify-content: center;
  margin-bottom: 60px;
}

/* Status Summary Grid con LED */
.status-summary-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));
  gap: 10px;
}
.status-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 10px 12px;
  background: var(--bg);
  border-radius: 8px;
  border: 1px solid var(--border);
  transition: all 0.2s ease;
}
.status-item-clickable {
  cursor: pointer;
}
.status-item-clickable:hover {
  background: var(--card-hover);
  border-color: var(--primary);
  transform: translateY(-1px);
}
.status-item-clickable:active {
  transform: translateY(0);
}
.status-item-info { flex: 1; min-width: 0; }
.status-item-label {
  font-size: 0.7rem;
  color: var(--text-muted);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}
.status-item-value {
  font-size: 0.85rem;
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

/* LED Indicator */
.led-indicator {
  width: 14px;
  height: 14px;
  border-radius: 50%;
  background: #475569;
  box-shadow: inset 0 1px 3px rgba(0,0,0,0.4);
  flex-shrink: 0;
  transition: all 0.3s ease;
}
.led-indicator.led-on {
  background: #22c55e;
  box-shadow: 0 0 8px #22c55e, 0 0 12px #22c55e, inset 0 -2px 4px rgba(0,0,0,0.2);
}
.led-indicator.led-off {
  background: #374151;
  box-shadow: inset 0 1px 3px rgba(0,0,0,0.4);
}
.led-indicator.led-warning {
  background: #f59e0b;
  box-shadow: 0 0 8px #f59e0b, 0 0 12px #f59e0b, inset 0 -2px 4px rgba(0,0,0,0.2);
}
.led-indicator.led-error {
  background: #ef4444;
  box-shadow: 0 0 8px #ef4444, 0 0 12px #ef4444, inset 0 -2px 4px rgba(0,0,0,0.2);
}
.led-indicator.led-blue {
  background: #3b82f6;
  box-shadow: 0 0 8px #3b82f6, 0 0 12px #3b82f6, inset 0 -2px 4px rgba(0,0,0,0.2);
}
.led-indicator.led-cyan {
  background: #06b6d4;
  box-shadow: 0 0 8px #06b6d4, 0 0 12px #06b6d4, inset 0 -2px 4px rgba(0,0,0,0.2);
}
.led-indicator.led-purple {
  background: #8b5cf6;
  box-shadow: 0 0 8px #8b5cf6, 0 0 12px #8b5cf6, inset 0 -2px 4px rgba(0,0,0,0.2);
}
.led-indicator.led-yellow {
  background: #eab308;
  box-shadow: 0 0 8px #eab308, 0 0 12px #eab308, inset 0 -2px 4px rgba(0,0,0,0.2);
}
.led-indicator.led-blink {
  animation: led-blink 1s ease-in-out infinite;
}
@keyframes led-blink {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.4; }
}

@media (max-width: 480px) {
  .setting-row { flex-wrap: wrap; }
  .setting-info { width: 100%; margin-bottom: 8px; }
  input[type="range"] { width: 80px; }
  .modes-grid { grid-template-columns: repeat(2, 1fr); }
  .status-summary-grid { grid-template-columns: repeat(2, 1fr); }
}
</style>
</head>
<body>
<div class="container">
  <header>
    <h1>OraQuadra Nano</h1>
    <p>Pannello Impostazioni Completo</p>
  </header>

  <div class="nav-links">
    <a href="/" class="nav-link" style="background:var(--primary);color:#fff;">&larr; Home</a>
    <a href="/clock" class="nav-link">Orologio Analogico</a>
    <a href="/ledring" class="nav-link">LED Ring</a>
    <a href="/bttf" class="nav-link">BTTF Time Circuits</a>
    <a href="/fluxcap" class="nav-link">⚡ Flux Capacitor</a>
    <a href="#" class="nav-link" id="espcamConfigLink" style="display:none;" target="_blank">📷 ESP-CAM Config</a>
  </div>

  <!-- SEZIONE RIASSUNTO STATO - LED INDICATORI (sempre aperta) -->
  <div class="section open" data-section="status-summary" data-no-collapse="true">
    <div class="section-header" style="cursor: default;">
      <div class="section-icon" style="background: linear-gradient(135deg, #10b981, #059669);">📊</div>
      <div class="section-title">Stato Sistema</div>
      <button onclick="closeAllSections()" style="margin-left: auto; padding: 5px 12px; border: none; border-radius: 6px; background: var(--bg-dark); color: var(--text-muted); font-size: 0.8rem; cursor: pointer;" title="Chiudi tutte le sezioni">▲ Chiudi</button>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <div class="status-summary-grid" id="statusSummaryGrid">
          <!-- Modalità Display Attiva -->
          <div class="status-item status-item-clickable" onclick="goToSection('modes')">
            <div class="led-indicator led-on" id="ledDisplayMode"></div>
            <div class="status-item-info">
              <div class="status-item-label">Modalità Display</div>
              <div class="status-item-value" id="summaryDisplayMode">--</div>
            </div>
          </div>
          <!-- Preset Colore -->
          <div class="status-item status-item-clickable" onclick="goToSection('effects', 'colorPreset')">
            <div class="led-indicator led-on" id="ledPreset" style="background: #ffffff;"></div>
            <div class="status-item-info">
              <div class="status-item-label">Preset / Colore</div>
              <div class="status-item-value" id="summaryPreset">--</div>
            </div>
          </div>
          <!-- Giorno/Notte -->
          <div class="status-item status-item-clickable" onclick="goToSection('time')">
            <div class="led-indicator" id="ledDayNight"></div>
            <div class="status-item-info">
              <div class="status-item-label">Fascia Oraria</div>
              <div class="status-item-value" id="summaryDayNight">--</div>
            </div>
          </div>
          <!-- Auto Night Mode -->
          <div class="status-item status-item-clickable" onclick="goToSection('display', 'autoNightMode')">
            <div class="led-indicator" id="ledAutoNight"></div>
            <div class="status-item-info">
              <div class="status-item-label">Auto Night Mode</div>
              <div class="status-item-value" id="summaryAutoNight">--</div>
            </div>
          </div>
          <!-- Power Save -->
          <div class="status-item status-item-clickable" onclick="goToSection('display', 'powerSave')">
            <div class="led-indicator" id="ledPowerSave"></div>
            <div class="status-item-info">
              <div class="status-item-label">Power Save</div>
              <div class="status-item-value" id="summaryPowerSave">--</div>
            </div>
          </div>
          <!-- Luminosità Display Corrente -->
          <div class="status-item status-item-clickable" onclick="goToSection('display')">
            <div class="led-indicator led-on" id="ledBrightness" style="background: #ffcc00;"></div>
            <div class="status-item-info">
              <div class="status-item-label">Luminosità</div>
              <div class="status-item-value" id="summaryBrightness">--</div>
            </div>
          </div>
          <!-- Presenza Rilevata - visibile solo se radar disponibile -->
          <div class="status-item status-item-clickable" id="summaryPresenceCard" style="display:none;" onclick="goToSection('sensors')">
            <div class="led-indicator" id="ledPresence"></div>
            <div class="status-item-info">
              <div class="status-item-label">Presenza</div>
              <div class="status-item-value" id="summaryPresence">--</div>
            </div>
          </div>
          <!-- Annuncio Orario - visibile solo se audio slave connesso -->
          <div class="status-item status-item-clickable" id="summaryHourlyAnnounceCard" style="display:none;" onclick="goToSection('audio', 'hourlyAnnounce')">
            <div class="led-indicator" id="ledHourlyAnnounce"></div>
            <div class="status-item-info">
              <div class="status-item-label">Annuncio Orario</div>
              <div class="status-item-value" id="summaryHourlyAnnounce">--</div>
            </div>
          </div>
          <!-- Suoni Touch - visibile solo se audio slave connesso -->
          <div class="status-item status-item-clickable" id="summaryTouchSoundsCard" style="display:none;" onclick="goToSection('audio', 'touchSounds')">
            <div class="led-indicator" id="ledTouchSounds"></div>
            <div class="status-item-info">
              <div class="status-item-label">Suoni Touch</div>
              <div class="status-item-value" id="summaryTouchSounds">--</div>
            </div>
          </div>
          <!-- Random Mode -->
          <div class="status-item status-item-clickable" onclick="goToSection('effects', 'randomModeEnabled')">
            <div class="led-indicator" id="ledRandomMode"></div>
            <div class="status-item-info">
              <div class="status-item-label">Modalità Random</div>
              <div class="status-item-value" id="summaryRandomMode">--</div>
            </div>
          </div>
          <!-- Lettera E -->
          <div class="status-item status-item-clickable" onclick="goToSection('effects', 'letterE')">
            <div class="led-indicator" id="ledLetterE"></div>
            <div class="status-item-info">
              <div class="status-item-label">Lettera "E"</div>
              <div class="status-item-value" id="summaryLetterE">--</div>
            </div>
          </div>
          <!-- Sensori - visibili solo se disponibili -->
          <div class="status-item status-item-clickable" id="summaryRadarCard" style="display:none;" onclick="goToSection('sensors')">
            <div class="led-indicator" id="ledRadar"></div>
            <div class="status-item-info">
              <div class="status-item-label">Radar LD2410</div>
              <div class="status-item-value" id="summaryRadar">--</div>
            </div>
          </div>
          <div class="status-item status-item-clickable" id="summaryBME280Card" style="display:none;" onclick="goToSection('sensors')">
            <div class="led-indicator" id="ledBME280"></div>
            <div class="status-item-info">
              <div class="status-item-label">Ambiente</div>
              <div class="status-item-value" id="summaryBME280">--</div>
            </div>
          </div>
          <div class="status-item status-item-clickable" id="summaryAudioSlaveCard" style="display:none;" onclick="goToSection('audio')">
            <div class="led-indicator" id="ledAudioSlave"></div>
            <div class="status-item-info">
              <div class="status-item-label">Audio I2S</div>
              <div class="status-item-value" id="summaryAudioSlave">--</div>
            </div>
          </div>
          <div class="status-item status-item-clickable" id="summaryWeatherCard" style="display:none;" onclick="goToSection('sensors')">
            <div class="led-indicator" id="ledWeather"></div>
            <div class="status-item-info">
              <div class="status-item-label">Meteo Online</div>
              <div class="status-item-value" id="summaryWeather">--</div>
            </div>
          </div>
          <!-- Magnetometro - visibile solo se connesso -->
          <div class="status-item status-item-clickable" id="summaryMagCard" style="display:none;" onclick="goToSection('sensors')">
            <div class="led-indicator" id="ledMagnetometer"></div>
            <div class="status-item-info">
              <div class="status-item-label">Bussola</div>
              <div class="status-item-value" id="summaryMagnetometer">--</div>
            </div>
          </div>
          <div class="status-item status-item-clickable" onclick="goToSection('system')">
            <div class="led-indicator" id="ledSD"></div>
            <div class="status-item-info">
              <div class="status-item-label">SD Card</div>
              <div class="status-item-value" id="summarySD">--</div>
            </div>
          </div>
          <div class="status-item status-item-clickable" onclick="goToSection('network')">
            <div class="led-indicator led-on" id="ledWiFi"></div>
            <div class="status-item-info">
              <div class="status-item-label">WiFi</div>
              <div class="status-item-value" id="summaryWiFi">--</div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- SEZIONE PRESET RAPIDI - Sempre visibile -->
  <div class="section open" data-section="presets" data-no-collapse="true">
    <div class="section-header" style="cursor: default;">
      <div class="section-icon" style="background: linear-gradient(135deg, #f59e0b, #ef4444);">🎨</div>
      <div class="section-title">Preset Colori Rapidi</div>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <div style="margin-bottom: 8px; font-size: 0.8rem; color: var(--text-muted);" id="presetModeLabel2">Colori disponibili per la modalità corrente</div>
        <div id="presetGridMain" style="display: grid; grid-template-columns: repeat(auto-fill, minmax(80px, 1fr)); gap: 8px;"></div>
        <!-- Color picker rapido con anteprima -->
        <div style="margin-top: 12px; display: flex; align-items: center; gap: 12px; padding: 12px; background: var(--bg); border-radius: 8px; border: 2px solid var(--border);" id="colorPickerBox">
          <input type="color" id="quickColorPicker" value="#FFFFFF" style="width: 50px; height: 40px; border: none; border-radius: 8px; cursor: pointer;">
          <div style="flex: 1;">
            <div style="font-weight: 600; font-size: 0.85rem;">Colore Personalizzato</div>
            <div style="font-size: 0.7rem; color: var(--text-muted);" id="colorPickerHint">Trascina per scegliere</div>
          </div>
          <!-- Anteprima colore grande -->
          <div id="colorPreviewBox" style="width: 60px; height: 40px; border-radius: 8px; background: #FFFFFF; box-shadow: 0 0 10px rgba(255,255,255,0.5), inset 0 0 0 2px rgba(255,255,255,0.3); transition: all 0.15s ease;"></div>
          <!-- Indicatore stato invio -->
          <div id="colorSendIndicator" style="width: 12px; height: 12px; border-radius: 50%; background: #444; transition: all 0.2s ease;" title="Stato invio"></div>
        </div>
      </div>
    </div>
  </div>

  <!-- SEZIONE MODALITA' - Sempre visibile -->
  <div class="section open" data-section="modes" data-no-collapse="true">
    <div class="section-header" style="cursor: default;">
      <div class="section-icon icon-modes">🎨</div>
      <div class="section-title">Modalità Display (Attiva/Disattiva)</div>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <p style="color: var(--text-muted); margin-bottom: 15px; font-size: 0.85rem;">
          <b>Clicca su una card</b> per attivare quella modalità sul display.
          Usa il <b>toggle</b> per abilitare/disabilitare (le disabilitate non appariranno con il touch).
        </p>
        <!-- Pulsante Random Mode -->
        <div style="display: flex; align-items: center; justify-content: space-between; padding: 12px 15px; background: var(--bg); border-radius: 10px; margin-bottom: 15px; border: 2px solid var(--border);" id="randomModeBox">
          <div style="display: flex; align-items: center; gap: 12px;">
            <span style="font-size: 1.5rem;">🎲</span>
            <div>
              <div style="font-weight: 600;">Modalità Random</div>
              <div style="font-size: 0.75rem; color: var(--text-muted);">Cambia automaticamente ogni <span id="randomIntervalDisplay">5</span> min</div>
            </div>
          </div>
          <label class="toggle">
            <input type="checkbox" id="randomModeToggle" onchange="toggleRandomMode(this.checked)">
            <span class="toggle-slider"></span>
          </label>
        </div>
        <div class="modes-grid" id="modesGrid">
          <!-- Popolato via JavaScript -->
        </div>
      </div>
    </div>
  </div>

  <!-- SEZIONE DISPLAY -->
  <div class="section" data-section="display">
    <div class="section-header">
      <div class="section-icon icon-display">☀️</div>
      <div class="section-title">Display e Luminosità</div>
      <span class="section-arrow">▼</span>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <!-- Luminosità Radar (visibile solo se radar attivo) -->
        <div class="setting-row" id="radarBrightnessRow" style="display:none;">
          <div class="setting-info">
            <div class="setting-label">Luminosità Radar</div>
            <div class="setting-desc">Luce: <span id="radarLightVal">--</span> → Display: <span id="radarBrightVal">--</span></div>
          </div>
          <div class="range-group">
            <div style="background:linear-gradient(90deg,#333,#fff);height:8px;border-radius:4px;width:100%;position:relative;">
              <div id="radarBrightIndicator" style="position:absolute;width:12px;height:12px;background:#00ff88;border-radius:50%;top:-2px;left:50%;transform:translateX(-50%);box-shadow:0 0 6px #00ff88;"></div>
            </div>
          </div>
        </div>
        <!-- Luminosità Giorno -->
        <div class="setting-row" id="brightnessDayRow">
          <div class="setting-info">
            <div class="setting-label">Luminosità Giorno</div>
            <div class="setting-desc">Luminosità display durante il giorno</div>
          </div>
          <div class="range-group">
            <input type="range" id="brightnessDay" min="10" max="255" value="250">
            <span class="range-value" id="brightnessDayVal">250</span>
          </div>
        </div>
        <!-- Luminosità Notte -->
        <div class="setting-row" id="brightnessNightRow">
          <div class="setting-info">
            <div class="setting-label">Luminosità Notte</div>
            <div class="setting-desc">Luminosità display durante la notte</div>
          </div>
          <div class="range-group">
            <input type="range" id="brightnessNight" min="10" max="255" value="90">
            <span class="range-value" id="brightnessNightVal">90</span>
          </div>
        </div>
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Auto Night Mode</div>
            <div class="setting-desc">Abbassa automaticamente la luminosità di notte</div>
          </div>
          <label class="toggle">
            <input type="checkbox" id="autoNightMode" checked>
            <span class="toggle-slider"></span>
          </label>
        </div>
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Power Save</div>
            <div class="setting-desc">Spegne display senza presenza</div>
          </div>
          <label class="toggle">
            <input type="checkbox" id="powerSave">
            <span class="toggle-slider"></span>
          </label>
        </div>
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Radar Brightness</div>
            <div class="setting-desc">Usa sensore luce radar per luminosità automatica</div>
          </div>
          <label class="toggle">
            <input type="checkbox" id="radarBrightnessControl" checked onchange="updateBrightnessUI()">
            <span class="toggle-slider"></span>
          </label>
        </div>
        <!-- Radar Brightness Min (visibile solo se radar brightness attivo) -->
        <div class="setting-row" id="radarBrightnessMinRow" style="display:none;">
          <div class="setting-info">
            <div class="setting-label">Luminosità Min (buio)</div>
            <div class="setting-desc">Valore minimo quando c'è poca luce</div>
          </div>
          <div class="range-group">
            <input type="range" id="radarBrightnessMin" min="10" max="255" value="90">
            <span class="range-value" id="radarBrightnessMinVal">90</span>
          </div>
        </div>
        <!-- Radar Brightness Max (visibile solo se radar brightness attivo) -->
        <div class="setting-row" id="radarBrightnessMaxRow" style="display:none;">
          <div class="setting-info">
            <div class="setting-label">Luminosità Max (luce)</div>
            <div class="setting-desc">Valore massimo quando c'è molta luce</div>
          </div>
          <div class="range-group">
            <input type="range" id="radarBrightnessMax" min="10" max="255" value="255">
            <span class="range-value" id="radarBrightnessMaxVal">255</span>
          </div>
        </div>
        <!-- Radar Server Remoto -->
        <div class="setting-row" style="margin-top:15px; border-top:1px solid #333; padding-top:15px;">
          <div class="setting-info">
            <div class="setting-label">🌐 Radar Server Remoto</div>
            <div class="setting-desc">Usa radar server esterno</div>
          </div>
          <label class="toggle">
            <input type="checkbox" id="radarServerEnabled" onchange="updateRadarServerUI()">
            <span class="toggle-slider"></span>
          </label>
        </div>
        <div class="setting-row" id="radarServerIPRow" style="display:none;">
          <div class="setting-info">
            <div class="setting-label">IP Radar Server</div>
            <div class="setting-desc">Indirizzo IP del radar server</div>
          </div>
          <div style="display:flex; gap:8px; align-items:center;">
            <input type="text" id="radarServerIP" placeholder="192.168.1.100" style="width:120px; padding:8px; border-radius:5px; border:1px solid #555; background:#222; color:#fff;">
            <button onclick="saveRadarServerConfig()" style="padding:8px 12px; border-radius:5px; border:none; background:#0a0; color:#fff; cursor:pointer;">Connetti</button>
          </div>
        </div>
        <div class="setting-row" id="radarServerStatusRow" style="display:none;">
          <div class="setting-info">
            <div class="setting-label">Stato Connessione</div>
            <div class="setting-desc" id="radarServerStatusDesc">Non connesso</div>
          </div>
          <span id="radarServerStatus" style="padding:5px 10px; border-radius:5px; background:#c00; color:#fff;">OFFLINE</span>
        </div>
      </div>
    </div>
  </div>

  <!-- SEZIONE ORARI -->
  <div class="section" data-section="time">
    <div class="section-header">
      <div class="section-icon icon-time">🕐</div>
      <div class="section-title">Orari Giorno/Notte</div>
      <span class="section-arrow">▼</span>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Inizio Giorno</div>
            <div class="setting-desc">Ora e minuti di inizio modalità giorno</div>
          </div>
          <div class="time-group">
            <input type="number" id="dayStartHour" min="0" max="23" value="8">
            <span>:</span>
            <input type="number" id="dayStartMinute" min="0" max="59" value="0">
          </div>
        </div>
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Inizio Notte</div>
            <div class="setting-desc">Ora e minuti di inizio modalità notte</div>
          </div>
          <div class="time-group">
            <input type="number" id="nightStartHour" min="0" max="23" value="22">
            <span>:</span>
            <input type="number" id="nightStartMinute" min="0" max="59" value="0">
          </div>
        </div>
        <div class="info-card">
          <div class="info-row">
            <span class="info-label">Ora attuale:</span>
            <span class="info-value" id="currentTime">--:--:--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Modalità attuale:</span>
            <span class="info-value" id="currentDayMode">--</span>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- SEZIONE AUDIO -->
  <div class="section" data-section="audio">
    <div class="section-header">
      <div class="section-icon icon-audio">🔊</div>
      <div class="section-title">Audio e Suoni</div>
      <span class="section-arrow">▼</span>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Annuncio Orario</div>
            <div class="setting-desc" id="hourlyAnnounceDesc">Annuncia l'ora ogni ora (tra 8:00 e 20:00)</div>
          </div>
          <label class="toggle">
            <input type="checkbox" id="hourlyAnnounce" checked>
            <span class="toggle-slider"></span>
          </label>
        </div>
        <!-- Voce Google TTS - NASCOSTA -->
        <div class="setting-row" id="ttsAnnounceRow" style="display:none;">
          <div class="setting-info">
            <div class="setting-label">Voce Google TTS</div>
            <div class="setting-desc">Usa sintesi vocale Google invece di file MP3 locali (richiede WiFi)</div>
          </div>
          <label class="toggle">
            <input type="checkbox" id="useTTSAnnounce">
            <span class="toggle-slider"></span>
          </label>
        </div>
        <!-- Tipo Voce TTS - NASCOSTA -->
        <div class="setting-row" id="ttsVoiceRow" style="display:none;">
          <div class="setting-info">
            <div class="setting-label">Tipo Voce TTS</div>
            <div class="setting-desc">Scegli voce femminile o maschile per Google TTS</div>
          </div>
          <select id="ttsVoiceFemale" class="setting-select">
            <option value="1">Femminile</option>
            <option value="0">Maschile</option>
          </select>
        </div>
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Suoni Touch</div>
            <div class="setting-desc">Feedback sonoro al tocco</div>
          </div>
          <label class="toggle">
            <input type="checkbox" id="touchSounds" checked>
            <span class="toggle-slider"></span>
          </label>
        </div>
        <!-- Volume Suoni Touch -->
        <div class="setting-row" id="touchSoundsVolumeRow">
          <div class="setting-info">
            <div class="setting-label">Volume Suoni Touch</div>
            <div class="setting-desc">Regola il volume dei suoni al tocco (0-100)</div>
          </div>
          <div class="range-group">
            <input type="range" id="touchSoundsVolume" min="0" max="100" value="50">
            <span class="range-value" id="touchSoundsVolumeVal">50</span>
          </div>
        </div>
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">VU Meter</div>
            <div class="setting-desc">Visualizza indicatore volume durante audio</div>
          </div>
          <label class="toggle">
            <input type="checkbox" id="vuMeterShow" checked>
            <span class="toggle-slider"></span>
          </label>
        </div>

        <!-- Volume Annunci Orari -->
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Volume Annunci Orari</div>
            <div class="setting-desc">Volume per gli annunci dell'ora (0-100)</div>
          </div>
          <div class="range-group">
            <input type="range" id="announceVolume" min="0" max="100" value="70">
            <span class="range-value" id="announceVolumeVal">70</span>
          </div>
        </div>

        <!-- Separatore Volume Giorno/Notte -->
        <div style="margin-top: 20px; padding-top: 15px; border-top: 1px solid var(--border);">
          <div style="font-weight: 600; margin-bottom: 12px; display: flex; align-items: center; gap: 8px;">
            <span>🌓</span> Volume Giorno/Notte
          </div>

          <!-- Volume Giorno -->
          <div class="setting-row">
            <div class="setting-info">
              <div class="setting-label">🌞 Volume Giorno</div>
              <div class="setting-desc">Volume audio durante il giorno</div>
            </div>
            <div class="range-group">
              <input type="range" id="volumeDay" min="0" max="100" value="80">
              <span class="range-value" id="volumeDayVal">80%</span>
            </div>
          </div>

          <!-- Volume Notte -->
          <div class="setting-row">
            <div class="setting-info">
              <div class="setting-label">🌙 Volume Notte</div>
              <div class="setting-desc">Volume audio durante la notte</div>
            </div>
            <div class="range-group">
              <input type="range" id="volumeNight" min="0" max="100" value="30">
              <span class="range-value" id="volumeNightVal">30%</span>
            </div>
          </div>
        </div>

        <div class="info-card">
          <div class="info-row">
            <span class="info-label">Audio I2S:</span>
            <span class="info-value" id="audioSlaveStatus">--</span>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- SEZIONE EFFETTI -->
  <div class="section" data-section="effects">
    <div class="section-header">
      <div class="section-icon icon-effects">✨</div>
      <div class="section-title">Colori e Effetti</div>
      <span class="section-arrow">▼</span>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <!-- Elementi nascosti per compatibilità JavaScript -->
        <select id="colorPreset" style="display: none;">
          <option value="13">Come Piace a Me</option>
        </select>
        <div id="presetGrid" style="display: none;"></div>
        <div id="presetModeLabel" style="display: none;"></div>
        <div id="colorControlsRow" style="display: none;">
          <input type="color" id="customColor" value="#ffffff">
        </div>
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Lettera "E"</div>
            <div class="setting-desc">Separatore ore/minuti</div>
          </div>
          <select id="letterE">
            <option value="0">Visibile</option>
            <option value="1">Lampeggiante</option>
          </select>
        </div>
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Intervallo Random</div>
            <div class="setting-desc">Minuti tra un cambio e l'altro</div>
          </div>
          <div class="range-group">
            <input type="range" id="randomModeInterval" min="1" max="60" value="5">
            <span class="range-value" id="randomModeIntervalVal">5 min</span>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- SEZIONE SENSORI -->
  <div class="section" data-section="sensors">
    <div class="section-header">
      <div class="section-icon icon-sensors">📡</div>
      <div class="section-title">Sensori</div>
      <span class="section-arrow">▼</span>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <div class="info-card">
          <div class="info-row">
            <span class="info-label">Radar:</span>
            <span class="info-value" id="radarStatus">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Tipo:</span>
            <span class="info-value" id="radarType">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Presenza:</span>
            <span class="info-value" id="presenceStatus">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Luce ambiente:</span>
            <span class="info-value" id="lightLevel">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Luminosità display:</span>
            <span class="info-value" id="displayBrightness">--</span>
          </div>
        </div>
        <!-- Dati aggiuntivi radar esterno - visibile solo se radar esterno connesso -->
        <div class="info-card" id="radarRemoteCard" style="display:none;">
          <div class="info-row">
            <span class="info-label">Radar Server:</span>
            <span class="info-value" id="radarServerIPDisplay">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Temp. sensore:</span>
            <span class="info-value" id="radarRemoteTemp">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Umidità sensore:</span>
            <span class="info-value" id="radarRemoteHum">--</span>
          </div>
        </div>
        <div class="info-card">
          <div class="info-row">
            <span class="info-label">Sensore Interno:</span>
            <span class="info-value" id="bme280Status">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Fonte dati:</span>
            <span class="info-value" id="indoorSensorSource">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Temp. interna:</span>
            <span class="info-value" id="tempIndoor">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Umidità:</span>
            <span class="info-value" id="humIndoor">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Pressione:</span>
            <span class="info-value" id="pressIndoor">--</span>
          </div>
        </div>
        <!-- Calibrazione Sensore - visibile se dati disponibili (BME280 interno o radar) -->
        <div class="info-card" id="bme280CalibCard" style="display:none; border: 1px solid #06b6d4;">
          <div style="font-weight: 600; margin-bottom: 10px; color: #06b6d4;">🔧 Calibrazione Sensore Ambiente</div>
          <div class="setting-row">
            <div class="setting-info">
              <div class="setting-label">Offset Temperatura</div>
              <div class="setting-desc">Aggiungi/sottrai °C (-10.0 a +10.0)</div>
            </div>
            <div class="range-group">
              <input type="number" id="bme280TempOffset" min="-10" max="10" step="0.1" value="0.0" style="width:80px;">
              <span>°C</span>
            </div>
          </div>
          <div class="setting-row">
            <div class="setting-info">
              <div class="setting-label">Offset Umidità</div>
              <div class="setting-desc">Aggiungi/sottrai % (-20.0 a +20.0)</div>
            </div>
            <div class="range-group">
              <input type="number" id="bme280HumOffset" min="-20" max="20" step="0.5" value="0.0" style="width:80px;">
              <span>%</span>
            </div>
          </div>
          <div style="display: flex; gap: 10px; margin-top: 10px; justify-content: center;">
            <button class="btn btn-primary" onclick="saveBME280Calibration()">💾 Salva Calibrazione</button>
            <button class="btn btn-danger btn-small" onclick="resetBME280Calibration()">🔄 Reset</button>
          </div>
          <div id="bme280CalibStatus" style="text-align: center; margin-top: 8px; font-size: 0.8rem; color: var(--text-muted);"></div>
        </div>
        <div class="info-card">
          <div class="info-row">
            <span class="info-label">Meteo esterno:</span>
            <span class="info-value" id="weatherStatus">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Temp. esterna:</span>
            <span class="info-value" id="tempOutdoor">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Umidità esterna:</span>
            <span class="info-value" id="humOutdoor">--</span>
          </div>
        </div>
        <!-- Magnetometro QMC5883P - visibile solo se connesso -->
        <div class="info-card" id="magnetometerCard" style="display:none;">
          <div class="info-row">
            <span class="info-label">Magnetometro:</span>
            <span class="info-value" id="magStatus">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Heading:</span>
            <span class="info-value" id="magHeading">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Direzione:</span>
            <span class="info-value" id="magDirection">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Calibrato:</span>
            <span class="info-value" id="magCalibrated">--</span>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- SEZIONE RETE -->
  <div class="section" data-section="network">
    <div class="section-header">
      <div class="section-icon icon-network">📶</div>
      <div class="section-title">Rete</div>
      <span class="section-arrow">▼</span>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <div class="info-card">
          <div class="info-row">
            <span class="info-label">WiFi:</span>
            <span class="info-value" id="wifiSSID">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">IP:</span>
            <span class="info-value" id="ipAddress">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Segnale:</span>
            <span class="info-value" id="wifiRSSI">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Alexa:</span>
            <span class="info-value">Abilitato</span>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- SEZIONE SISTEMA -->
  <div class="section" data-section="system">
    <div class="section-header">
      <div class="section-icon icon-system">⚙️</div>
      <div class="section-title">Sistema</div>
      <span class="section-arrow">▼</span>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <div class="info-card">
          <div class="info-row">
            <span class="info-label">Firmware:</span>
            <span class="info-value" id="firmwareVersion">V1.5</span>
          </div>
          <div class="info-row">
            <span class="info-label">Uptime:</span>
            <span class="info-value" id="uptime">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">Heap:</span>
            <span class="info-value" id="freeHeap">--</span>
          </div>
          <div class="info-row">
            <span class="info-label">SD Card:</span>
            <span class="info-value" id="sdStatus">--</span>
          </div>
        </div>
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Reset Impostazioni</div>
          </div>
          <button class="btn btn-danger btn-small" onclick="resetSettings()">Reset</button>
        </div>
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Riavvia</div>
          </div>
          <button class="btn btn-danger btn-small" onclick="rebootDevice()">Riavvia</button>
        </div>
      </div>
    </div>
  </div>

  <!-- SEZIONE API KEYS -->
  <div class="section" data-section="apikeys">
    <div class="section-header">
      <div class="section-icon" style="background: linear-gradient(135deg, #f59e0b, #d97706);">🔑</div>
      <div class="section-title">API Keys</div>
      <span class="section-arrow">▼</span>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">🌤️ Open-Meteo (Gratuito)</div>
            <div class="setting-desc">Servizio meteo senza API Key</div>
          </div>
        </div>
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Città Meteo</div>
            <div class="setting-desc">Es: Roma, Milano, Napoli</div>
          </div>
          <input type="text" id="openweatherCity" placeholder="Nome città" style="width:150px; padding:8px; border-radius:6px; border:1px solid var(--border); background:var(--bg); color:var(--text);">
        </div>
        <div class="setting-row" id="apiKeysStatus" style="display:none;">
          <div class="setting-info">
            <div class="setting-label" style="color: var(--success);">✓ API Keys salvate automaticamente</div>
          </div>
        </div>
      </div>
    </div>
  </div>

  <!-- SEZIONE LINGUA -->
  <div class="section" data-section="language">
    <div class="section-header">
      <div class="section-icon" style="background: linear-gradient(135deg, #3b82f6, #8b5cf6);">🌐</div>
      <div class="section-title">Language / Lingua</div>
      <span class="section-arrow">▼</span>
    </div>
    <div class="section-content">
      <div class="section-inner">
        <div class="setting-row">
          <div class="setting-info">
            <div class="setting-label">Display Language</div>
            <div class="setting-desc">Lingua per orologio a parole, meteo, menu</div>
          </div>
          <select id="displayLanguage" onchange="changeLanguage(this.value)">
            <option value="0">🇮🇹 Italiano</option>
            <option value="1">🇬🇧 English</option>
          </select>
        </div>
        <div class="info-card">
          <div class="info-row">
            <span class="info-label">Lingua corrente:</span>
            <span class="info-value" id="currentLanguage">Italiano</span>
          </div>
          <div class="info-row">
            <span class="info-label">Word Clock:</span>
            <span class="info-value" id="wordClockLanguage">IT</span>
          </div>
        </div>
        <p style="color: var(--text-muted); font-size: 0.8rem; margin-top: 10px;">
          <b>Note:</b> Il Word Clock inglese usa formato 12 ore (es. "IT IS QUARTER PAST THREE").
          Il Word Clock italiano usa formato 24 ore (es. "SONO LE QUINDICI E VENTI").
        </p>
      </div>
    </div>
  </div>

  <div class="actions">
    <button class="btn btn-primary" onclick="saveAllSettings()">💾 Salva Tutto</button>
    <button class="btn btn-success" onclick="loadSettings()">🔄 Ricarica</button>
  </div>
</div>

<div class="status-bar">
  <div class="status-text">
    <span class="status-dot online" id="statusDot"></span>
    <span id="statusText">Connesso</span>
  </div>
  <span id="lastSave">--</span>
</div>

<script>
// Definizione modalità
const MODES = [
  { id: 0, name: 'FADE', icon: '🌅', desc: 'Dissolvenza' },
  { id: 1, name: 'SLOW', icon: '🐢', desc: 'Lento' },
  { id: 2, name: 'FAST', icon: '⚡', desc: 'Veloce' },
  { id: 3, name: 'MATRIX', icon: '💚', desc: 'Matrix' },
  { id: 4, name: 'MATRIX2', icon: '💜', desc: 'Matrix 2' },
  { id: 5, name: 'SNAKE', icon: '🐍', desc: 'Serpente' },
  { id: 6, name: 'WATER', icon: '💧', desc: 'Goccia' },
  { id: 7, name: 'MARIO', icon: '🍄', desc: 'Mario' },
  { id: 8, name: 'TRON', icon: '🏍️', desc: 'Tron' },
  { id: 9, name: 'GALAGA', icon: '🚀', desc: 'Galaga' },
  { id: 10, name: 'ANALOG', icon: '🕰️', desc: 'Analogico' },
  { id: 11, name: 'FLIP', icon: '📅', desc: 'Flip Clock' },
  { id: 12, name: 'BTTF', icon: '🔙', desc: 'DeLorean' },
  { id: 13, name: 'LED RING', icon: '💫', desc: 'Cerchio LED' },
  { id: 14, name: 'WEATHER', icon: '🌤️', desc: 'Meteo' },
  { id: 15, name: 'CLOCK', icon: '⏰', desc: 'Orologio' },
  { id: 17, name: 'GALAGA2', icon: '👾', desc: 'Galaga 2' },
  { id: 19, name: 'ESP-CAM', icon: '📷', desc: 'Camera' },
  { id: 20, name: 'FLUX', icon: '⚡', desc: 'Flux Capacitor' },
  { id: 21, name: 'CHRISTMAS', icon: '🎄', desc: 'Natale' },
  { id: 22, name: 'FIRE', icon: '🔥', desc: 'Fuoco Camino' },
  { id: 23, name: 'FIRE TEXT', icon: '🔥', desc: 'Lettere di Fuoco' },
  { id: 24, name: 'MP3 PLAYER', icon: '🎵', desc: 'Lettore MP3' },
  { id: 25, name: 'WEB RADIO', icon: '📻', desc: 'Web Radio' },
  { id: 26, name: 'RADIO ALARM', icon: '⏰', desc: 'Radiosveglia' }
];

let currentMode = 2;
let enabledModes = {};
let activePresetId = 0;      // Preset colore attivo
let rainbowActive = false;   // Se rainbow mode è attivo
let hasWeatherApiKey = false; // Flag per API key meteo configurata

// Inizializza tutte le modalità come abilitate di default
MODES.forEach(m => enabledModes[m.id] = true);

// Render griglia modalità
function renderModes() {
  const grid = document.getElementById('modesGrid');
  grid.innerHTML = '';

  MODES.forEach(mode => {
    // Nascondi modalità WEATHER (14) se API key non configurata
    if (mode.id === 14 && !hasWeatherApiKey) {
      return; // Salta questa modalità
    }

    const isEnabled = enabledModes[mode.id] !== false;
    const isActive = currentMode === mode.id;

    const card = document.createElement('div');
    card.className = 'mode-card' + (isEnabled ? ' enabled' : ' disabled') + (isActive ? ' active' : '');
    card.onclick = (e) => {
      // Non attivare se si clicca sul toggle
      if (e.target.type === 'checkbox' || e.target.classList.contains('toggle-slider')) return;
      if (isEnabled) activateMode(mode.id);
    };
    card.innerHTML = `
      ${isActive ? '<span class="mode-badge">★ ATTIVO</span>' : ''}
      <div class="mode-icon">${mode.icon}</div>
      <div class="mode-name">${mode.name}</div>
      <div class="mode-status">${isActive ? 'In uso' : (isEnabled ? 'Clicca per attivare' : 'Disabilitato')}</div>
      <label class="toggle mode-toggle" onclick="event.stopPropagation()">
        <input type="checkbox" ${isEnabled ? 'checked' : ''} onchange="toggleMode(${mode.id}, this.checked)">
        <span class="toggle-slider"></span>
      </label>
    `;
    grid.appendChild(card);
  });
}

// Toggle abilitazione modalità
function toggleMode(modeId, enabled) {
  enabledModes[modeId] = enabled;
  saveModeSettings();
  renderModes();
  updateEspCamLink();
}

// Variabile globale per URL webcam
var esp32camWebUrl = '';

// Aggiorna visibilità link ESP-CAM
function updateEspCamLink() {
  var camLink = document.getElementById('espcamConfigLink');
  var espCamModeEnabled = enabledModes[19] === true; // 19 = MODE_ESP32CAM
  if (espCamModeEnabled && esp32camWebUrl && esp32camWebUrl.length > 5) {
    camLink.href = esp32camWebUrl;
    camLink.style.display = 'inline-block';
  } else {
    camLink.style.display = 'none';
  }
}

// Attiva modalità
function activateMode(modeId) {
  if (enabledModes[modeId] === false) return;

  setStatus('saving', 'Attivazione...');
  fetch('/settings/setmode?mode=' + modeId)
    .then(r => {
      if (!r.ok) throw new Error('Errore setmode');
      return r.text();
    })
    .then(() => {
      currentMode = modeId;
      renderModes();

      // Carica configurazione aggiornata per questa modalità (con timestamp anti-cache)
      return fetch('/settings/config?t=' + Date.now());
    })
    .then(r => {
      if (!r.ok) throw new Error('Errore config');
      return r.json();
    })
    .then(data => {
      // Aggiorna color picker con colore della modalità
      if (data.customColor) {
        const mainPicker = document.getElementById('customColor');
        const quickPicker = document.getElementById('quickColorPicker');
        if (mainPicker) mainPicker.value = data.customColor;
        if (quickPicker) quickPicker.value = data.customColor;
        // Aggiorna anteprima visiva
        updateColorPreviewGlobal(data.customColor);
      }

      // Aggiorna stato preset per questa modalità (modePreset dal server)
      // 255 = nessun preset, 100 = rainbow, 0-14 = preset ID
      if (data.modePreset !== undefined) {
        if (data.modePreset === 100) {
          // Rainbow attivo per questa modalità
          rainbowActive = true;
          activePresetId = 0;
        } else if (data.modePreset === 255) {
          // Nessun preset, usa colore custom
          rainbowActive = false;
          activePresetId = 13; // Personalizzato
        } else {
          // Preset specifico
          rainbowActive = false;
          activePresetId = data.modePreset;
        }
      } else {
        // Fallback: usa rainbowMode e colorPreset dal server
        rainbowActive = data.rainbowMode === true;
        activePresetId = data.colorPreset || 0;
      }

      // Aggiorna UI
      renderPresets();
      updateColorControlsVisibility();
      updateStatusSummary(data);
      setStatus('online', 'Modalità attivata!');
      setTimeout(() => setStatus('online', 'Connesso'), 2000);
    })
    .catch(err => {
      console.error('activateMode error:', err);
      setStatus('offline', 'Errore');
    });
}

// Salva impostazioni modalità
function saveModeSettings() {
  const enabledList = Object.keys(enabledModes).filter(k => enabledModes[k]).join(',');
  fetch('/settings/savemodes?enabled=' + enabledList)
    .then(() => {
      document.getElementById('lastSave').textContent = 'Salvato: ' + new Date().toLocaleTimeString();
    });
}

// Toggle modalità random on/off
function toggleRandomMode(enabled) {
  setStatus('saving', 'Salvataggio...');
  fetch('/settings/save?randomModeEnabled=' + (enabled ? 1 : 0))
    .then(r => r.text())
    .then(() => {
      // Aggiorna stile box
      const box = document.getElementById('randomModeBox');
      if (enabled) {
        box.style.borderColor = 'var(--warning)';
        box.style.background = 'rgba(245, 158, 11, 0.15)';
      } else {
        box.style.borderColor = 'var(--border)';
        box.style.background = 'var(--bg)';
      }
      // Aggiorna anche il toggle (stesso elemento)
      document.getElementById('randomModeToggle').checked = enabled;
      // Aggiorna visibilità sezione preset (nasconde se random attivo)
      updatePresetSectionVisibility();
      // Aggiorna summary
      updateSummaryFromUI();
      setStatus('online', enabled ? 'Random ON!' : 'Random OFF');
      setTimeout(() => setStatus('online', 'Connesso'), 1500);
    })
    .catch(() => setStatus('offline', 'Errore'));
}

// Toggle Web Radio on/off
function toggleWebRadio(enabled) {
  setStatus('saving', enabled ? 'Avvio radio...' : 'Stop radio...');
  const action = enabled ? 'start' : 'stop';
  fetch('/settings/webradio?action=' + action)
    .then(r => r.json())
    .then(data => {
      updateWebRadioFullUI(data);
      setStatus('online', data.enabled ? 'Radio ON!' : 'Radio OFF');
      setTimeout(() => setStatus('online', 'Connesso'), 1500);
    })
    .catch(() => setStatus('offline', 'Errore radio'));
}

// Imposta volume Web Radio
function setWebRadioVolume(vol) {
  document.getElementById('webRadioVolumeVal').textContent = vol;
  fetch('/settings/webradio?action=volume&value=' + vol)
    .then(r => r.json())
    .catch(() => {});
}

// Aggiorna UI Web Radio dallo stato (versione semplice per polling)
function updateWebRadioUI(data) {
  if (data.webRadioEnabled !== undefined) {
    const box = document.getElementById('webRadioBox');
    const toggle = document.getElementById('webRadioToggle');
    if (toggle) toggle.checked = data.webRadioEnabled;
    if (box) {
      if (data.webRadioEnabled) {
        box.style.borderColor = '#00aaff';
        box.style.background = 'rgba(0, 170, 255, 0.15)';
      } else {
        box.style.borderColor = 'var(--border)';
        box.style.background = 'var(--bg)';
      }
    }
    if (data.webRadioVolume !== undefined) {
      const volSlider = document.getElementById('webRadioVolume');
      const volVal = document.getElementById('webRadioVolumeVal');
      if (volSlider) volSlider.value = data.webRadioVolume;
      if (volVal) volVal.textContent = data.webRadioVolume;
    }
  }
}

// Aggiorna UI Web Radio completa (con lista stazioni)
function updateWebRadioFullUI(data) {
  const box = document.getElementById('webRadioBox');
  const toggle = document.getElementById('webRadioToggle');
  const currentName = document.getElementById('webRadioCurrentName');
  const volSlider = document.getElementById('webRadioVolume');
  const volVal = document.getElementById('webRadioVolumeVal');
  const stationSelect = document.getElementById('webRadioStationSelect');

  // Aggiorna toggle e stile box
  if (toggle) toggle.checked = data.enabled;
  if (box) {
    if (data.enabled) {
      box.style.borderColor = '#00aaff';
      box.style.background = 'rgba(0, 170, 255, 0.15)';
    } else {
      box.style.borderColor = 'var(--border)';
      box.style.background = 'var(--bg)';
    }
  }

  // Aggiorna nome stazione corrente
  if (currentName) {
    currentName.textContent = data.enabled ? data.currentName : 'Radio spenta';
    currentName.style.color = data.enabled ? '#00aaff' : '#666';
  }

  // Aggiorna volume
  if (volSlider) volSlider.value = data.volume;
  if (volVal) volVal.textContent = data.volume;

  // Aggiorna lista stazioni
  if (stationSelect && data.stations) {
    stationSelect.innerHTML = '';
    data.stations.forEach((station, index) => {
      const option = document.createElement('option');
      option.value = index;
      option.textContent = station.name;
      if (index === data.currentIndex) option.selected = true;
      stationSelect.appendChild(option);
    });
  }
}

// Seleziona stazione radio
function selectWebRadioStation(index) {
  setStatus('saving', 'Cambio stazione...');
  fetch('/settings/webradio?action=select&index=' + index)
    .then(r => r.json())
    .then(data => {
      updateWebRadioFullUI(data);
      setStatus('online', 'Stazione: ' + data.currentName);
      setTimeout(() => setStatus('online', 'Connesso'), 2000);
    })
    .catch(() => setStatus('offline', 'Errore selezione'));
}

// Aggiungi nuova stazione radio
function addNewRadioStation() {
  const name = document.getElementById('newRadioName').value.trim();
  const url = document.getElementById('newRadioUrl').value.trim();

  if (!name || !url) {
    alert('Inserisci nome e URL della radio');
    return;
  }

  if (!url.startsWith('http://') && !url.startsWith('https://')) {
    alert('URL non valido. Deve iniziare con http:// o https://');
    return;
  }

  setStatus('saving', 'Aggiunta radio...');
  fetch('/settings/webradio?action=add&name=' + encodeURIComponent(name) + '&url=' + encodeURIComponent(url))
    .then(r => r.json())
    .then(data => {
      updateWebRadioFullUI(data);
      document.getElementById('newRadioName').value = '';
      document.getElementById('newRadioUrl').value = '';
      setStatus('online', 'Radio aggiunta!');
      setTimeout(() => setStatus('online', 'Connesso'), 2000);
    })
    .catch(() => setStatus('offline', 'Errore aggiunta'));
}

// Rimuovi stazione radio selezionata
function removeSelectedRadioStation() {
  const select = document.getElementById('webRadioStationSelect');
  if (!select || select.options.length <= 1) {
    alert('Devi avere almeno una radio nella lista');
    return;
  }

  const index = select.value;
  const name = select.options[select.selectedIndex].textContent;

  if (!confirm('Rimuovere "' + name + '" dalla lista?')) {
    return;
  }

  setStatus('saving', 'Rimozione radio...');
  fetch('/settings/webradio?action=remove&index=' + index)
    .then(r => r.json())
    .then(data => {
      updateWebRadioFullUI(data);
      setStatus('online', 'Radio rimossa!');
      setTimeout(() => setStatus('online', 'Connesso'), 2000);
    })
    .catch(() => setStatus('offline', 'Errore rimozione'));
}

// Carica lista radio all'avvio
function loadWebRadioStations() {
  fetch('/settings/webradio')
    .then(r => r.json())
    .then(data => {
      updateWebRadioFullUI(data);
    })
    .catch(() => {});
}

// Attiva modalità MP3 Player sul display
function activateMP3Player() {
  setStatus('saving', 'Attivazione MP3 Player...');
  fetch('/mp3player/activate')
    .then(r => r.json())
    .then(data => {
      if (data.success) {
        // Evidenzia il box MP3 Player
        const box = document.getElementById('mp3PlayerBox');
        if (box) {
          box.style.borderColor = '#9c27b0';
          box.style.background = 'rgba(156, 39, 176, 0.15)';
        }
        setStatus('online', 'MP3 Player attivato!');
        setTimeout(() => {
          setStatus('online', 'Connesso');
          // Reset stile dopo 2 secondi
          if (box) {
            box.style.borderColor = 'var(--border)';
            box.style.background = 'var(--bg)';
          }
        }, 2000);
      }
    })
    .catch(() => setStatus('offline', 'Errore attivazione'));
}

// Attiva modalità Web Radio sul display
function activateWebRadio() {
  setStatus('saving', 'Attivazione Web Radio...');
  fetch('/webradio/activate')
    .then(r => r.json())
    .then(data => {
      if (data.success) {
        // Evidenzia il box Web Radio
        const box = document.getElementById('webRadioBox');
        if (box) {
          box.style.borderColor = '#0088ff';
          box.style.background = 'rgba(0, 136, 255, 0.15)';
        }
        setStatus('online', 'Web Radio attivata!');
        setTimeout(() => {
          setStatus('online', 'Connesso');
          // Reset stile dopo 2 secondi
          if (box) {
            box.style.borderColor = 'var(--border)';
            box.style.background = 'var(--bg)';
          }
        }, 2000);
      }
    })
    .catch(() => setStatus('offline', 'Errore attivazione'));
}

// Aggiorna stile box random mode
function updateRandomModeBoxStyle(enabled) {
  const box = document.getElementById('randomModeBox');
  if (box) {
    if (enabled) {
      box.style.borderColor = 'var(--warning)';
      box.style.background = 'rgba(245, 158, 11, 0.15)';
    } else {
      box.style.borderColor = 'var(--border)';
      box.style.background = 'var(--bg)';
    }
  }
}

// Salva colore quando cambia
document.addEventListener('DOMContentLoaded', () => {
  const colorPicker = document.getElementById('customColor');
  if (colorPicker) {
    let colorSendTimeout = null;
    // Funzione per inviare il colore al dispositivo
    const sendColor = () => {
      const color = colorPicker.value;
      // Sincronizza con quick color picker
      const quickPicker = document.getElementById('quickColorPicker');
      if (quickPicker) quickPicker.value = color;
      // Aggiorna anteprima visiva
      updateColorPreviewGlobal(color);
      // Imposta preset su Personalizzato (13)
      activePresetId = 13;
      rainbowActive = false;
      renderPresets();
      // Invia colore e preset 13 al dispositivo
      fetch('/settings/save?customColor=' + encodeURIComponent(color) + '&colorPreset=13')
        .then(() => {
          setStatus('online', 'Colore applicato!');
          setTimeout(() => setStatus('online', 'Connesso'), 1000);
        });
    };
    // Evento 'input' - si attiva in tempo reale mentre si trascina nel picker
    colorPicker.addEventListener('input', () => {
      // Aggiorna anteprima immediata
      updateColorPreviewGlobal(colorPicker.value);
      // Debounce: invia max ogni 150ms per non sovraccaricare
      if (colorSendTimeout) clearTimeout(colorSendTimeout);
      colorSendTimeout = setTimeout(sendColor, 150);
    });
    // Evento 'change' - si attiva quando si chiude il picker (conferma finale)
    colorPicker.addEventListener('change', sendColor);
  }

  // Quick Color Picker (sezione Preset Rapidi) con anteprima in tempo reale
  const quickColorPicker = document.getElementById('quickColorPicker');
  if (quickColorPicker) {
    let quickColorTimeout = null;
    const colorSendIndicator = document.getElementById('colorSendIndicator');
    const colorPickerHint = document.getElementById('colorPickerHint');

    // Funzione per mostrare stato invio
    const setColorSendState = (state) => {
      if (!colorSendIndicator || !colorPickerHint) return;
      switch(state) {
        case 'sending':
          colorSendIndicator.style.background = '#FFA500';
          colorSendIndicator.style.boxShadow = '0 0 8px #FFA500';
          colorPickerHint.textContent = 'Invio al display...';
          colorPickerHint.style.color = '#FFA500';
          break;
        case 'success':
          colorSendIndicator.style.background = '#00FF00';
          colorSendIndicator.style.boxShadow = '0 0 8px #00FF00';
          colorPickerHint.textContent = 'Applicato al display!';
          colorPickerHint.style.color = '#00FF00';
          break;
        case 'idle':
          colorSendIndicator.style.background = '#444';
          colorSendIndicator.style.boxShadow = 'none';
          colorPickerHint.textContent = 'Trascina per scegliere';
          colorPickerHint.style.color = 'var(--text-muted)';
          break;
      }
    };

    const sendQuickColor = () => {
      const color = quickColorPicker.value;
      // Sincronizza con color picker principale
      const mainPicker = document.getElementById('customColor');
      if (mainPicker) mainPicker.value = color;
      // Imposta preset su Personalizzato (13)
      activePresetId = 13;
      rainbowActive = false;
      // Aggiorna il pulsante Personalizzato con il nuovo colore
      renderPresets();
      // Mostra stato invio
      setColorSendState('sending');
      // Invia al dispositivo
      fetch('/settings/save?customColor=' + encodeURIComponent(color) + '&colorPreset=13')
        .then(() => {
          setColorSendState('success');
          setStatus('online', 'Colore applicato!');
          setTimeout(() => {
            setColorSendState('idle');
            setStatus('online', 'Connesso');
          }, 1500);
        })
        .catch(() => {
          colorSendIndicator.style.background = '#FF0000';
          colorPickerHint.textContent = 'Errore invio!';
          colorPickerHint.style.color = '#FF0000';
        });
    };

    // Input: aggiorna anteprima in tempo reale + invia con debounce
    quickColorPicker.addEventListener('input', () => {
      const color = quickColorPicker.value;
      // Aggiorna anteprima IMMEDIATA (senza debounce) usando funzione globale
      updateColorPreviewGlobal(color);
      // Invia al dispositivo con debounce
      if (quickColorTimeout) clearTimeout(quickColorTimeout);
      quickColorTimeout = setTimeout(sendQuickColor, 200);
    });

    // Change: invia subito quando si rilascia
    quickColorPicker.addEventListener('change', sendQuickColor);

    // Inizializza anteprima con colore corrente
    updateColorPreviewGlobal(quickColorPicker.value);
  }

  // Quando cambia il preset: invia immediatamente al dispositivo e aggiorna UI
  const presetSelect = document.getElementById('colorPreset');
  if (presetSelect) {
    presetSelect.addEventListener('change', () => {
      const presetVal = presetSelect.value;
      setStatus('saving', 'Applicando preset...');
      // Invia il preset al dispositivo immediatamente
      fetch('/settings/save?colorPreset=' + presetVal)
        .then(r => r.text())
        .then(() => {
          // Aggiorna visibilità color picker
          updateColorControlsVisibility();
          // Ricarica config per ottenere nuova modalità e colore
          fetch('/settings/config')
            .then(r => r.json())
            .then(data => {
              // Aggiorna modalità corrente
              if (data.displayMode !== undefined) {
                currentMode = data.displayMode;
                renderModes();
              }
              // NON aggiornare customColor - deve rimanere indipendente dai preset
              renderPresets();
              // Aggiorna summary
              updateStatusSummary(data);
              setStatus('online', 'Preset applicato!');
              setTimeout(() => setStatus('online', 'Connesso'), 1500);
            });
        })
        .catch(() => {
          setStatus('offline', 'Errore');
        });
    });
  }
});

// Colori default per modalità testuali (orologio a parole)
const MODE_DEFAULT_COLORS = {
  0: '#FFFFFF',  // FADE - Bianco
  1: '#FFFFFF',  // SLOW - Bianco
  2: '#FFFFFF',  // FAST - Bianco
  3: '#00FF00',  // MATRIX - Verde
  4: '#00FF00',  // MATRIX2 - Verde
  5: '#FFFF00',  // SNAKE - Giallo
  6: '#00BFFF',  // WATER - Azzurro
  7: '#FF0000',  // MARIO - Rosso
  8: '#00FFFF',  // TRON - Ciano
  9: '#00FF00',  // GALAGA - Verde
  17: '#00FF00'  // GALAGA2 - Verde
};

// Modalità testuali che supportano colore personalizzato (orologio a parole)
const TEXT_MODES = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 17];

// Modalità che NON supportano preset colori (grafiche/speciali)
const NO_COLOR_MODES = [10, 11, 12, 13, 14, 19, 20, 21, 22, 23, 24, 25];
// 10=ANALOG, 11=FLIP, 12=BTTF, 13=LED RING, 14=WEATHER, 19=ESP-CAM, 20=FLUX, 21=CHRISTMAS, 22=FIRE, 23=FIRE TEXT, 24=MP3 PLAYER

// Verifica se la modalità corrente supporta il colore personalizzato
function isTextMode(modeId) {
  return TEXT_MODES.includes(modeId);
}

// Verifica se la modalità supporta i preset colori
function supportsColorPresets(modeId) {
  return !NO_COLOR_MODES.includes(modeId);
}

// Aggiorna visibilità sezione Preset Colori Rapidi e Preset Custom
function updatePresetSectionVisibility() {
  const presetSection = document.querySelector('[data-section="presets"]');
  const randomToggle = document.getElementById('randomModeToggle');
  const isRandomMode = randomToggle && randomToggle.checked;

  if (presetSection) {
    // Nascondi se: modalità senza colori O random mode attivo
    if (supportsColorPresets(currentMode) && !isRandomMode) {
      presetSection.style.display = 'block';
    } else {
      presetSection.style.display = 'none';
    }
  }
}

// Aggiorna visibilità controlli colore in base alla modalità E al preset
function updateColorControlsVisibility() {
  const colorRow = document.getElementById('colorControlsRow');

  // Aggiorna anche sezione preset
  updatePresetSectionVisibility();

  // Nascondi sempre la riga colore (rimossa dalla sezione Colori ed Effetti)
  if (colorRow) {
    colorRow.style.display = 'none';
  }
}

// Reset colore default per modalità corrente (solo per modalità testuali)
function resetModeColor() {
  if (!isTextMode(currentMode)) {
    setStatus('online', 'Colore non applicabile a questa modalità');
    setTimeout(() => setStatus('online', 'Connesso'), 1500);
    return;
  }

  // Disattiva rainbow se attivo
  // Chiama endpoint per reset colore e ricevi il nuovo colore
  fetch('/settings/resetmodecolor')
    .then(r => r.text())
    .then(newColor => {
      document.getElementById('customColor').value = newColor;
      setStatus('online', 'Colore resettato!');
      setTimeout(() => setStatus('online', 'Connesso'), 1500);
    })
    .catch(() => {
      setStatus('offline', 'Errore reset colore');
    });
}

// Gestione sezioni (escluse quelle con data-no-collapse)
// Solo una sezione può essere aperta alla volta (accordion)
document.querySelectorAll('.section-header').forEach(header => {
  header.addEventListener('click', () => {
    const section = header.parentElement;
    if (section.dataset.noCollapse !== 'true') {
      const isOpen = section.classList.contains('open');
      // Chiudi tutte le altre sezioni (tranne status-summary)
      document.querySelectorAll('.section').forEach(s => {
        if (s.dataset.noCollapse !== 'true') {
          s.classList.remove('open');
        }
      });
      // Se non era aperta, aprila
      if (!isOpen) {
        section.classList.add('open');
      }
    }
  });
});

// Slider real-time
document.querySelectorAll('input[type="range"]').forEach(slider => {
  const valueSpan = document.getElementById(slider.id + 'Val');
  if (valueSpan) {
    slider.addEventListener('input', () => {
      let val = slider.value;
      if (slider.id === 'volumeDay' || slider.id === 'volumeNight') val += '%';
      else if (slider.id === 'randomModeInterval') val += ' min';
      valueSpan.textContent = val;
    });
  }
});

let saveTimeout = null;

// Aggiorna il summary stato in base ai valori UI correnti (dopo modifica locale)
function updateSummaryFromUI() {
  // Auto Night Mode
  const autoNight = document.getElementById('autoNightMode').checked;
  document.getElementById('summaryAutoNight').textContent = autoNight ? 'Attivo' : 'Disattivo';
  document.getElementById('ledAutoNight').className = autoNight ? 'led-indicator led-on' : 'led-indicator led-off';

  // Power Save
  const powerSave = document.getElementById('powerSave').checked;
  document.getElementById('summaryPowerSave').textContent = powerSave ? 'Attivo' : 'Disattivo';
  document.getElementById('ledPowerSave').className = powerSave ? 'led-indicator led-on led-warning' : 'led-indicator led-off';

  // Annuncio Orario
  const hourlyAnn = document.getElementById('hourlyAnnounce').checked;
  document.getElementById('summaryHourlyAnnounce').textContent = hourlyAnn ? 'Attivo' : 'Disattivo';
  document.getElementById('ledHourlyAnnounce').className = hourlyAnn ? 'led-indicator led-on' : 'led-indicator led-off';

  // Suoni Touch
  const touchSnd = document.getElementById('touchSounds').checked;
  document.getElementById('summaryTouchSounds').textContent = touchSnd ? 'Attivo' : 'Disattivo';
  document.getElementById('ledTouchSounds').className = touchSnd ? 'led-indicator led-on' : 'led-indicator led-off';

  // Random Mode
  const randomEnabled = document.getElementById('randomModeToggle').checked;
  const randomInterval = document.getElementById('randomModeInterval').value;
  document.getElementById('summaryRandomMode').textContent = randomEnabled ? (randomInterval + ' min') : 'Disattivo';
  document.getElementById('ledRandomMode').className = randomEnabled ? 'led-indicator led-on led-warning led-blink' : 'led-indicator led-off';
  document.getElementById('randomIntervalDisplay').textContent = randomInterval;
  updateRandomModeBoxStyle(randomEnabled);

  // Lettera E
  const letterE = parseInt(document.getElementById('letterE').value);
  document.getElementById('summaryLetterE').textContent = letterE === 0 ? 'Visibile' : 'Lampeggiante';
  document.getElementById('ledLetterE').className = letterE === 1 ? 'led-indicator led-on led-blink' : 'led-indicator led-on';

  // Preset
  const preset = parseInt(document.getElementById('colorPreset').value);
  document.getElementById('summaryPreset').textContent = PRESET_NAMES[preset] || 'Preset ' + preset;

  // Colore
  const customColor = document.getElementById('customColor').value;
  const ledPreset = document.getElementById('ledPreset');
  ledPreset.style.background = customColor;
  ledPreset.style.boxShadow = '0 0 8px ' + customColor + ', 0 0 12px ' + customColor;
}

function setupAutoSave() {
  // Tutti gli input e select nelle sezioni settings
  // Esclude solo i checkbox per abilitare/disabilitare modalità (che hanno gestione separata)
  const inputs = document.querySelectorAll('.section input, .section select');
  console.log('[AUTOSAVE] Trovati ' + inputs.length + ' input per auto-save');

  inputs.forEach(input => {
    // Salta checkbox di toggle modalità (hanno handler dedicato)
    if (input.closest('.mode-card')) return;

    input.addEventListener('change', () => {
      console.log('[AUTOSAVE] Change su: ' + input.id + ' = ' + (input.type === 'checkbox' ? input.checked : input.value));
      if (saveTimeout) clearTimeout(saveTimeout);
      setStatus('saving', 'Salvataggio...');
      saveTimeout = setTimeout(saveAllSettings, 500);
      // Aggiorna summary immediatamente
      updateSummaryFromUI();
      // Aggiorna descrizione annuncio orario se cambiano orari giorno/notte
      if (input.id === 'dayStartHour' || input.id === 'dayStartMinute' || input.id === 'nightStartHour' || input.id === 'nightStartMinute') {
        updateHourlyAnnounceDesc();
      }
      // Voce TTS nascosta permanentemente - non mostrare mai
      // if (input.id === 'useTTSAnnounce') {
      //   document.getElementById('ttsVoiceRow').style.display = input.checked ? '' : 'none';
      // }
      // Mostra/nascondi slider volume suoni touch quando si attiva/disattiva suoni touch
      if (input.id === 'touchSounds') {
        document.getElementById('touchSoundsVolumeRow').style.display = input.checked ? '' : 'none';
      }
    });
  });
}

function setStatus(status, text) {
  document.getElementById('statusDot').className = 'status-dot ' + status;
  document.getElementById('statusText').textContent = text;
}

// Funzione globale per aggiornare anteprima colore (chiamata quando colore cambia da qualsiasi fonte)
function updateColorPreviewGlobal(color) {
  const colorPreviewBox = document.getElementById('colorPreviewBox');
  const colorPickerBox = document.getElementById('colorPickerBox');
  if (colorPreviewBox) {
    colorPreviewBox.style.background = color;
    colorPreviewBox.style.boxShadow = '0 0 15px ' + color + ', inset 0 0 0 2px rgba(255,255,255,0.3)';
  }
  if (colorPickerBox) {
    colorPickerBox.style.borderColor = color;
  }
}

// Chiudi tutte le sezioni (tranne status-summary)
function closeAllSections() {
  document.querySelectorAll('.section').forEach(s => {
    if (s.dataset.noCollapse !== 'true') {
      s.classList.remove('open');
    }
  });
  // Scrolla in alto alla pagina
  window.scrollTo({ top: 0, behavior: 'smooth' });
}

// Naviga alla sezione e opzionalmente evidenzia un elemento
function goToSection(sectionName, elementId) {
  // Trova la sezione
  const section = document.querySelector('[data-section="' + sectionName + '"]');
  if (!section) return;

  // Chiudi tutte le altre sezioni (accordion) tranne status-summary
  document.querySelectorAll('.section').forEach(s => {
    if (s.dataset.noCollapse !== 'true') {
      s.classList.remove('open');
    }
  });

  // Apri la sezione target
  section.classList.add('open');

  // Scorri alla sezione
  section.scrollIntoView({ behavior: 'smooth', block: 'start' });

  // Se specificato un elementId, evidenzialo
  if (elementId) {
    setTimeout(() => {
      const element = document.getElementById(elementId);
      if (element) {
        // Trova il setting-row contenitore
        let row = element.closest('.setting-row');
        if (row) {
          row.style.transition = 'background 0.3s';
          row.style.background = 'rgba(99, 102, 241, 0.3)';
          row.style.borderRadius = '8px';
          setTimeout(() => {
            row.style.background = '';
          }, 2000);
        }
        // Scorri all'elemento
        element.scrollIntoView({ behavior: 'smooth', block: 'center' });
        // Flash sull'elemento stesso
        element.focus && element.focus();
      }
    }, 400);
  }
}

// Nomi modalità per il riassunto
const MODE_NAMES = {
  0: 'FADE', 1: 'SLOW', 2: 'FAST', 3: 'MATRIX', 4: 'MATRIX2',
  5: 'SNAKE', 6: 'WATER', 7: 'MARIO', 8: 'TRON', 9: 'GALAGA',
  10: 'ANALOG', 11: 'FLIP', 12: 'BTTF', 13: 'LED RING', 14: 'WEATHER',
  15: 'CLOCK', 17: 'GALAGA2', 19: 'ESP-CAM', 20: 'FLUX', 21: 'CHRISTMAS', 22: 'FIRE', 23: 'FIRE TEXT',
  24: 'MP3 PLAYER', 25: 'WEB RADIO', 26: 'RADIO ALARM'
};

// Nomi preset per il riassunto
const PRESET_NAMES = {
  0: 'Random', 1: 'Veloce Acqua', 2: 'Lento Viola', 3: 'Lento Arancione',
  4: 'Fade Rosso', 5: 'Fade Verde', 6: 'Fade Blu', 7: 'Matrix Giallo',
  8: 'Matrix Ciano', 9: 'Matrix Verde', 10: 'Matrix Bianco', 11: 'Snake',
  12: 'Goccia Acqua', 13: 'Personalizzato'
};

// Preset con modalità associate e colori (senza Random, già presente in Modalità Display)
// Il preset Rainbow (id: 14) è disponibile per tutte le modalità testuali
const PRESETS = [
  { id: 1, name: 'Acqua', color: '#00FFFF', modes: [2] },           // FAST
  { id: 2, name: 'Viola', color: '#FF00FF', modes: [1] },           // SLOW
  { id: 3, name: 'Arancione', color: '#FFA500', modes: [1] },       // SLOW
  { id: 4, name: 'Rosso', color: '#FF0000', modes: [0] },           // FADE
  { id: 5, name: 'Verde', color: '#00FF00', modes: [0] },           // FADE
  { id: 6, name: 'Blu', color: '#0000FF', modes: [0] },             // FADE
  { id: 7, name: 'Giallo', color: '#FFFF00', modes: [3] },          // MATRIX
  { id: 8, name: 'Ciano', color: '#00FFFF', modes: [3] },           // MATRIX
  { id: 9, name: 'Verde', color: '#00FF00', modes: [4] },           // MATRIX2
  { id: 10, name: 'Bianco', color: '#FFFFFF', modes: [4] },         // MATRIX2
  { id: 11, name: 'Giallo', color: '#FFFF00', modes: [5] },         // SNAKE
  { id: 12, name: 'Azzurro', color: '#00BFFF', modes: [6] },        // WATER
  { id: 14, name: '🌈 Rainbow', color: 'rainbow', modes: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 17], rainbow: true },  // Tutte le modalità testuali
  { id: 13, name: 'Personalizzato', color: '#FFFFFF', modes: 'all' }
];

// Mappa modalità -> nome per label
const MODE_LABELS = {
  0: 'Fade', 1: 'Slow', 2: 'Fast', 3: 'Matrix', 4: 'Matrix2', 5: 'Snake', 6: 'Water'
};

// Renderizza griglia preset filtrata per modalità corrente
function renderPresets() {
  // Griglia principale (sezione Preset Rapidi in alto)
  const gridMain = document.getElementById('presetGridMain');
  const labelMain = document.getElementById('presetModeLabel2');
  // Griglia secondaria (sezione Effetti)
  const grid = document.getElementById('presetGrid');
  const label = document.getElementById('presetModeLabel');

  // Filtra preset per modalità corrente
  const filtered = PRESETS.filter(p => {
    if (p.modes === 'all') return true;
    return p.modes.includes(currentMode);
  });

  // Aggiorna label
  const modeName = MODE_LABELS[currentMode] || MODE_NAMES[currentMode] || 'questa modalità';
  const labelText = filtered.length <= 1 ? 'Solo colore personalizzato' : 'Colori per ' + modeName;
  if (label) label.textContent = labelText;
  if (labelMain) labelMain.textContent = labelText;

  // Se non ci sono preset specifici, mostra solo Personalizzato
  const presetsToShow = filtered.length > 1 ? filtered : PRESETS.filter(p => p.modes === 'all');

  // Funzione per creare bottone preset
  const createPresetBtn = (preset, compact) => {
    const btn = document.createElement('button');
    btn.className = 'preset-btn';
    const isRainbow = preset.rainbow === true;

    // Determina se questo preset è attivo
    const isActive = (isRainbow && rainbowActive) || (!rainbowActive && preset.id === activePresetId);

    // Per il preset Personalizzato (id 13), usa il colore attuale dal color picker
    let displayColor = preset.color;
    if (preset.id === 13) {
      const customPicker = document.getElementById('customColor') || document.getElementById('quickColorPicker');
      if (customPicker) displayColor = customPicker.value;
    }

    // Stili per Rainbow
    const rainbowGradient = 'linear-gradient(90deg, #ff0000, #ff8000, #ffff00, #00ff00, #00ffff, #0080ff, #8000ff)';
    const rainbowBorder = 'border-image: ' + rainbowGradient + ' 1; border-width: 2px; border-style: solid;';
    const rainbowCircle = 'background: ' + rainbowGradient + ';';

    // Stile base e stile attivo
    const activeBg = 'rgba(76, 175, 80, 0.4)';  // Verde per attivo
    const normalBg = 'rgba(0,0,0,0.3)';
    const activeBorder = isActive ? 'border-width: 3px !important;' : '';
    const boxShadowActive = isActive ? 'box-shadow: 0 0 15px rgba(76, 175, 80, 0.8), 0 0 25px rgba(76, 175, 80, 0.4);' : '';
    const activeBadge = isActive ? '<span style="position: absolute; top: -6px; right: -6px; background: #4CAF50; color: white; font-size: 0.55rem; padding: 2px 5px; border-radius: 8px; font-weight: bold;">ATTIVO</span>' : '';

    if (compact) {
      if (isRainbow) {
        btn.style.cssText = 'position: relative; padding: 8px 6px; ' + rainbowBorder + ' border-radius: 8px; background: ' + (isActive ? activeBg : normalBg) + '; color: white; cursor: pointer; display: flex; flex-direction: column; align-items: center; gap: 3px; transition: all 0.2s; ' + activeBorder + boxShadowActive;
        btn.innerHTML = activeBadge + '<span style="width: 20px; height: 20px; border-radius: 50%; ' + rainbowCircle + '"></span><span style="font-size: 0.7rem;">' + preset.name + '</span>';
      } else {
        btn.style.cssText = 'position: relative; padding: 8px 6px; border: 2px solid ' + displayColor + '; border-radius: 8px; background: ' + (isActive ? activeBg : normalBg) + '; color: white; cursor: pointer; display: flex; flex-direction: column; align-items: center; gap: 3px; transition: all 0.2s; ' + activeBorder + boxShadowActive;
        btn.innerHTML = activeBadge + '<span style="width: 20px; height: 20px; border-radius: 50%; background: ' + displayColor + '; box-shadow: 0 0 6px ' + displayColor + ';"></span><span style="font-size: 0.7rem;">' + preset.name + '</span>';
      }
    } else {
      if (isRainbow) {
        btn.style.cssText = 'position: relative; padding: 10px; ' + rainbowBorder + ' border-radius: 8px; background: ' + (isActive ? activeBg : normalBg) + '; color: white; cursor: pointer; display: flex; flex-direction: column; align-items: center; gap: 4px; transition: all 0.2s; ' + activeBorder + boxShadowActive;
        btn.innerHTML = activeBadge + '<span style="width: 24px; height: 24px; border-radius: 50%; ' + rainbowCircle + '"></span><span style="font-size: 0.8rem;">' + preset.name + '</span>';
      } else {
        btn.style.cssText = 'position: relative; padding: 10px; border: 2px solid ' + displayColor + '; border-radius: 8px; background: ' + (isActive ? activeBg : normalBg) + '; color: white; cursor: pointer; display: flex; flex-direction: column; align-items: center; gap: 4px; transition: all 0.2s; ' + activeBorder + boxShadowActive;
        btn.innerHTML = activeBadge + '<span style="width: 24px; height: 24px; border-radius: 50%; background: ' + displayColor + '; box-shadow: 0 0 8px ' + displayColor + ';"></span><span style="font-size: 0.8rem;">' + preset.name + '</span>';
      }
    }
    btn.onmouseover = () => { if (!isActive) { btn.style.background = 'rgba(255,255,255,0.1)'; } btn.style.transform = 'scale(1.05)'; };
    btn.onmouseout = () => { if (!isActive) { btn.style.background = normalBg; } btn.style.transform = 'scale(1)'; };
    btn.onclick = () => applyPreset(preset.id, isRainbow);
    return btn;
  };

  // Popola griglia principale (compatta)
  if (gridMain) {
    gridMain.innerHTML = '';
    presetsToShow.forEach(preset => {
      gridMain.appendChild(createPresetBtn(preset, true));
    });
  }

  // Popola griglia secondaria
  if (grid) {
    grid.innerHTML = '';
    presetsToShow.forEach(preset => {
      grid.appendChild(createPresetBtn(preset, false));
    });
  }
}

// Applica preset selezionato (viene salvato per la modalità corrente)
function applyPreset(presetId, isRainbow) {
  document.getElementById('colorPreset').value = presetId;
  setStatus('saving', 'Applicando preset...');

  let url;

  // Se è Rainbow, invia SOLO rainbowMode=1 (verrà salvato come preset 100 per questa modalità)
  if (isRainbow) {
    url = '/settings/save?rainbowMode=1';
    rainbowActive = true;
    activePresetId = 0;
  } else {
    // Per altri preset, invia colorPreset e disabilita rainbow
    // Il backend salverà questo preset per la modalità corrente
    url = '/settings/save?colorPreset=' + presetId + '&rainbowMode=0';
    rainbowActive = false;
    activePresetId = presetId;
    // Per preset 13 (Personalizzato) NON inviamo customColor - il backend usa il colore salvato
    // Questo permette di usare il colore impostato dal display via color cycle
  }

  // Aggiorna immediatamente UI per feedback visivo
  renderPresets();

  fetch(url)
    .then(r => r.text())
    .then(() => {
      // Aggiungi timestamp per evitare cache
      fetch('/settings/config?t=' + Date.now())
        .then(r => r.json())
        .then(data => {
          if (data.displayMode !== undefined) {
            currentMode = data.displayMode;
            renderModes();
          }
          // Aggiorna stato da modePreset del server (preset per questa modalità)
          if (data.modePreset !== undefined) {
            if (data.modePreset === 100) {
              rainbowActive = true;
              activePresetId = 0;
            } else if (data.modePreset === 255) {
              rainbowActive = false;
              activePresetId = 13;
            } else {
              rainbowActive = false;
              activePresetId = data.modePreset;
            }
          } else {
            activePresetId = data.colorPreset || 0;
            rainbowActive = data.rainbowMode === true;
          }
          renderPresets();
          updateStatusSummary(data);
          updateColorControlsVisibility();

          // Sincronizza color picker con il colore dal server (importante per preset 13)
          if (data.customColor) {
            const mainPicker = document.getElementById('customColor');
            const quickPicker = document.getElementById('quickColorPicker');
            if (mainPicker) mainPicker.value = data.customColor;
            if (quickPicker) quickPicker.value = data.customColor;
            // Aggiorna anteprima visiva
            updateColorPreviewGlobal(data.customColor);
          }

          const msg = isRainbow ? 'Rainbow salvato per questa modalità!' : 'Preset salvato per questa modalità!';
          setStatus('online', msg);
          setTimeout(() => setStatus('online', 'Connesso'), 1500);
        });
    })
    .catch(() => setStatus('offline', 'Errore'));
}

// Aggiorna UI luminosità in base a Radar Brightness ON/OFF
function updateBrightnessUI() {
  const radarEnabled = document.getElementById('radarBrightnessControl').checked;
  const radarRow = document.getElementById('radarBrightnessRow');
  const radarMinRow = document.getElementById('radarBrightnessMinRow');
  const radarMaxRow = document.getElementById('radarBrightnessMaxRow');
  const dayRow = document.getElementById('brightnessDayRow');
  const nightRow = document.getElementById('brightnessNightRow');
  const autoNightRow = document.getElementById('autoNightMode').closest('.setting-row');

  if (radarEnabled) {
    // Radar attivo: mostra luminosità radar e slider min/max, disabilita controlli manuali
    radarRow.style.display = 'flex';
    radarMinRow.style.display = 'flex';
    radarMaxRow.style.display = 'flex';
    dayRow.style.opacity = '0.4';
    nightRow.style.opacity = '0.4';
    dayRow.style.pointerEvents = 'none';
    nightRow.style.pointerEvents = 'none';
    // Disabilita anche Auto Night Mode (non ha senso con radar)
    autoNightRow.style.opacity = '0.4';
    autoNightRow.style.pointerEvents = 'none';
  } else {
    // Radar disattivo: nascondi radar e min/max, abilita controlli manuali
    radarRow.style.display = 'none';
    radarMinRow.style.display = 'none';
    radarMaxRow.style.display = 'none';
    dayRow.style.opacity = '1';
    nightRow.style.opacity = '1';
    dayRow.style.pointerEvents = 'auto';
    nightRow.style.pointerEvents = 'auto';
    // Abilita Auto Night Mode per luminosità manuale
    autoNightRow.style.opacity = '1';
    autoNightRow.style.pointerEvents = 'auto';
  }
}

// Aggiorna UI Radar Server Remoto
function updateRadarServerUI() {
  const enabled = document.getElementById('radarServerEnabled').checked;
  const ipRow = document.getElementById('radarServerIPRow');
  const statusRow = document.getElementById('radarServerStatusRow');

  if (enabled) {
    ipRow.style.display = 'flex';
    statusRow.style.display = 'flex';
    // Se c'è un IP configurato, connetti subito
    const ip = document.getElementById('radarServerIP').value.trim();
    if (ip && ip.length > 6) {
      // IP valido, salva e connetti
      saveRadarServerConfig();
    } else {
      // Nessun IP, mostra solo stato
      checkRadarServerStatus();
    }
  } else {
    ipRow.style.display = 'none';
    statusRow.style.display = 'none';
    // Salva subito quando si disabilita
    saveRadarServerConfig();
  }
}

// Verifica stato connessione radar server
function checkRadarServerStatus() {
  console.log('[RADAR] Verifica stato connessione...');
  fetch('/radar/status')
    .then(r => r.json())
    .then(data => {
      console.log('[RADAR] Status ricevuto:', data);
      const statusEl = document.getElementById('radarServerStatus');
      const descEl = document.getElementById('radarServerStatusDesc');
      const isConnected = data.remote && data.remote.connected === true;
      console.log('[RADAR] isConnected:', isConnected, 'usingRemote:', data.usingRemote);
      if (isConnected) {
        statusEl.textContent = 'ONLINE';
        statusEl.style.background = '#0a0';
        descEl.textContent = 'Connesso - ' + (data.usingRemote ? 'In uso' : 'Standby');
      } else {
        statusEl.textContent = 'OFFLINE';
        statusEl.style.background = '#c00';
        descEl.textContent = 'Non connesso - Uso radar locale';
      }
      // Aggiorna anche card sensori se connesso
      if (isConnected) {
        refreshSensorData();
      }
    })
    .catch(e => {
      console.error('[RADAR] Errore status:', e);
      document.getElementById('radarServerStatus').textContent = 'ERRORE';
      document.getElementById('radarServerStatus').style.background = '#c00';
    });
}

// Carica configurazione radar server
function loadRadarServerConfig() {
  console.log('[RADAR] Caricamento config...');
  fetch('/radar/config')
    .then(r => r.json())
    .then(data => {
      console.log('[RADAR] Config ricevuta:', data);
      document.getElementById('radarServerEnabled').checked = data.enabled === true;
      document.getElementById('radarServerIP').value = data.serverIP || '';
      // Aggiorna UI senza salvare (prima volta)
      const enabled = data.enabled === true;
      const ipRow = document.getElementById('radarServerIPRow');
      const statusRow = document.getElementById('radarServerStatusRow');
      ipRow.style.display = enabled ? 'flex' : 'none';
      statusRow.style.display = enabled ? 'flex' : 'none';
      if (enabled) checkRadarServerStatus();
    })
    .catch(e => console.log('[RADAR] Errore caricamento:', e));
}

// Salva configurazione radar server
function saveRadarServerConfig() {
  const enabled = document.getElementById('radarServerEnabled').checked ? 1 : 0;
  const ip = document.getElementById('radarServerIP').value.trim();
  console.log('[RADAR] Salvataggio - IP:', ip, 'Enabled:', enabled);

  const url = '/radar/config/set?ip=' + encodeURIComponent(ip) + '&enabled=' + enabled;
  console.log('[RADAR] URL:', url);

  // Mostra stato "Connessione..." mentre attende
  const statusEl = document.getElementById('radarServerStatus');
  const descEl = document.getElementById('radarServerStatusDesc');
  if (statusEl && enabled) {
    statusEl.textContent = '...';
    statusEl.style.background = '#f90';
    if (descEl) descEl.textContent = 'Connessione in corso...';
  }

  // Usa AbortController per timeout (10 secondi per permettere connessione radar)
  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), 10000);

  fetch(url, { signal: controller.signal })
    .then(r => {
      clearTimeout(timeoutId);
      console.log('[RADAR] Response status:', r.status);
      if (!r.ok) throw new Error('HTTP ' + r.status);
      return r.text();
    })
    .then(text => {
      console.log('[RADAR] Response text:', text);
      try {
        return JSON.parse(text);
      } catch(e) {
        console.error('[RADAR] JSON parse error:', e);
        throw new Error('Invalid JSON');
      }
    })
    .then(data => {
      console.log('[RADAR] Risposta salvataggio:', data);
      if (data.status === 'ok') {
        // Aggiorna IMMEDIATAMENTE lo stato dalla risposta
        if (statusEl) {
          if (data.connected) {
            statusEl.textContent = 'ONLINE';
            statusEl.style.background = '#0a0';
            if (descEl) descEl.textContent = 'Connesso - In uso';
          } else if (enabled) {
            statusEl.textContent = 'OFFLINE';
            statusEl.style.background = '#c00';
            if (descEl) descEl.textContent = 'Non connesso - Verificare IP';
          } else {
            statusEl.textContent = 'OFFLINE';
            statusEl.style.background = '#666';
            if (descEl) descEl.textContent = 'Disabilitato';
          }
        }
        // Refresh dati sensori dopo 1s (per sincronizzare tutto)
        setTimeout(refreshSensorData, 1000);
        // Verifica stato connessione dopo 3s (conferma)
        setTimeout(checkRadarServerStatus, 3000);
      }
    })
    .catch(e => {
      clearTimeout(timeoutId);
      console.error('[RADAR] Errore:', e.name, e.message);
      if (statusEl) {
        if (e.name === 'AbortError') {
          statusEl.textContent = 'TIMEOUT';
          if (descEl) descEl.textContent = 'Timeout - Riprovare';
        } else {
          statusEl.textContent = 'ERRORE';
          if (descEl) descEl.textContent = 'Errore: ' + e.message;
        }
        statusEl.style.background = '#c00';
      }
    });
}

// Forza refresh dati sensori (dopo cambio radar interno/esterno)
function refreshSensorData() {
  console.log('[SENSORS] Refresh dati sensori...');
  fetch('/settings/status')
    .then(r => r.json())
    .then(data => {
      // === SEZIONE SENSORI ===
      // Aggiorna tipo radar
      const radarTypeEl = document.getElementById('radarType');
      if (radarTypeEl && data.radarType) {
        radarTypeEl.textContent = data.radarType === 'esterno' ? 'Esterno' : (data.radarType === 'interno' ? 'Interno' : 'N/D');
      }
      // Aggiorna stato radar
      document.getElementById('radarStatus').textContent = data.radarAvailable ? 'OK' : 'N/D';
      // Aggiorna presenza
      document.getElementById('presenceStatus').textContent = data.presenceDetected ? 'Sì' : 'No';
      // Aggiorna luce
      document.getElementById('lightLevel').textContent = data.lightLevel || '--';
      document.getElementById('displayBrightness').textContent = data.displayBrightness || '--';
      // Mostra/nasconde card radar esterno
      const radarRemoteCard = document.getElementById('radarRemoteCard');
      if (radarRemoteCard) {
        if (data.radarServerConnected && data.radarType === 'esterno') {
          radarRemoteCard.style.display = 'block';
          if (data.radarServerIP) {
            document.getElementById('radarServerIPDisplay').textContent = data.radarServerIP;
          }
          if (data.radarRemoteTemp !== undefined && data.radarRemoteTemp > -900) {
            document.getElementById('radarRemoteTemp').textContent = data.radarRemoteTemp.toFixed(1) + ' C';
          }
          if (data.radarRemoteHum !== undefined && data.radarRemoteHum > 0) {
            document.getElementById('radarRemoteHum').textContent = data.radarRemoteHum.toFixed(0) + '%';
          }
        } else {
          radarRemoteCard.style.display = 'none';
        }
      }
      // Aggiorna display luminosità radar
      updateRadarBrightnessDisplay(data);

      // === SUMMARY STATUS (parte alta pagina) ===
      // Card e LED Radar
      const summaryRadarCard = document.getElementById('summaryRadarCard');
      const summaryPresenceCard = document.getElementById('summaryPresenceCard');
      const ledRadar = document.getElementById('ledRadar');
      if (data.radarAvailable) {
        if (summaryRadarCard) summaryRadarCard.style.display = 'flex';
        if (summaryPresenceCard) summaryPresenceCard.style.display = 'flex';
        const summaryRadar = document.getElementById('summaryRadar');
        if (summaryRadar) {
          summaryRadar.textContent = data.radarType === 'esterno' ? 'Esterno' : (data.radarType === 'interno' ? 'Interno' : 'OK');
        }
        if (ledRadar) ledRadar.className = 'led-indicator led-on';
      } else {
        if (summaryRadarCard) summaryRadarCard.style.display = 'none';
        if (summaryPresenceCard) summaryPresenceCard.style.display = 'none';
      }
      // Presenza nel summary
      const summaryPresence = document.getElementById('summaryPresence');
      const ledPresence = document.getElementById('ledPresence');
      if (summaryPresence) {
        summaryPresence.textContent = data.presenceDetected ? 'Rilevata' : 'Assente';
      }
      if (ledPresence) {
        ledPresence.className = data.presenceDetected ? 'led-indicator led-on led-cyan led-blink' : 'led-indicator led-off';
      }
      // Aggiorna anche controlli luminosità radar (abilita/disabilita in base a disponibilità)
      updateRadarControlsAvailability(data.radarAvailable);

      console.log('[SENSORS] Refresh completato - Tipo:', data.radarType, 'Disponibile:', data.radarAvailable);
    })
    .catch(e => console.log('[SENSORS] Errore refresh:', e));
}

// Aggiorna valori luminosità radar dalla risposta JSON
function updateRadarBrightnessDisplay(data) {
  if (data.lightLevel !== undefined) {
    document.getElementById('radarLightVal').textContent = data.lightLevel;
  }
  if (data.displayBrightness !== undefined) {
    document.getElementById('radarBrightVal').textContent = data.displayBrightness;
    // Posiziona indicatore (range configurabile min-max)
    const min = data.radarBrightnessMin || 90;
    const max = data.radarBrightnessMax || 255;
    const range = max - min;
    const percent = range > 0 ? Math.max(0, Math.min(100, ((data.displayBrightness - min) / range) * 100)) : 50;
    document.getElementById('radarBrightIndicator').style.left = percent + '%';
  }
}

// Aggiorna disponibilità controlli radar (disabilita se radar non connesso)
function updateRadarControlsAvailability(radarAvailable) {
  const powerSaveRow = document.getElementById('powerSave').closest('.setting-row');
  const radarBrightnessRow = document.getElementById('radarBrightnessControl').closest('.setting-row');
  const radarMinRow = document.getElementById('radarBrightnessMinRow');
  const radarMaxRow = document.getElementById('radarBrightnessMaxRow');
  const radarDisplayRow = document.getElementById('radarBrightnessRow');
  const dayRow = document.getElementById('brightnessDayRow');
  const nightRow = document.getElementById('brightnessNightRow');
  const autoNightRow = document.getElementById('autoNightMode').closest('.setting-row');

  if (!radarAvailable) {
    // Radar non disponibile: disabilita tutti i controlli radar
    powerSaveRow.style.opacity = '0.4';
    powerSaveRow.style.pointerEvents = 'none';
    radarBrightnessRow.style.opacity = '0.4';
    radarBrightnessRow.style.pointerEvents = 'none';
    radarMinRow.style.display = 'none';
    radarMaxRow.style.display = 'none';
    radarDisplayRow.style.display = 'none';
    // Attiva luminosità manuale (giorno/notte e auto night mode)
    dayRow.style.opacity = '1';
    nightRow.style.opacity = '1';
    dayRow.style.pointerEvents = 'auto';
    nightRow.style.pointerEvents = 'auto';
    autoNightRow.style.opacity = '1';
    autoNightRow.style.pointerEvents = 'auto';
  } else {
    // Radar disponibile: abilita i controlli radar
    powerSaveRow.style.opacity = '1';
    powerSaveRow.style.pointerEvents = 'auto';
    radarBrightnessRow.style.opacity = '1';
    radarBrightnessRow.style.pointerEvents = 'auto';
    // min/max, display row e autoNightMode sono gestiti da updateBrightnessUI
  }
}

// Aggiorna la sezione riassunto stato con LED
function updateStatusSummary(data) {
  // Modalità Display - LED colorato in base alla modalità
  const ledMode = document.getElementById('ledDisplayMode');
  const modeName = MODE_NAMES[data.displayMode] || 'Mode ' + data.displayMode;
  document.getElementById('summaryDisplayMode').textContent = modeName;
  ledMode.className = 'led-indicator led-on led-purple';

  // Preset / Colore - LED con il colore corrente
  const ledPreset = document.getElementById('ledPreset');
  const presetName = PRESET_NAMES[data.colorPreset] || 'Preset ' + data.colorPreset;
  document.getElementById('summaryPreset').textContent = presetName;
  if (data.customColor) {
    ledPreset.style.background = data.customColor;
    ledPreset.style.boxShadow = '0 0 8px ' + data.customColor + ', 0 0 12px ' + data.customColor;
  }
  ledPreset.className = 'led-indicator led-on';

  // Fascia Oraria (Giorno/Notte)
  const ledDayNight = document.getElementById('ledDayNight');
  if (data.currentHour !== undefined) {
    const currentMinutes = data.currentHour * 60 + (data.currentMinute || 0);
    const dayStartMinutes = (data.dayStartHour || 8) * 60 + (data.dayStartMinute || 0);
    const nightStartMinutes = (data.nightStartHour || 22) * 60 + (data.nightStartMinute || 0);
    let isDay;
    if (dayStartMinutes < nightStartMinutes) {
      isDay = (currentMinutes >= dayStartMinutes && currentMinutes < nightStartMinutes);
    } else {
      isDay = (currentMinutes >= dayStartMinutes || currentMinutes < nightStartMinutes);
    }
    document.getElementById('summaryDayNight').textContent = isDay ? 'Giorno' : 'Notte';
    ledDayNight.className = isDay ? 'led-indicator led-on led-yellow' : 'led-indicator led-on led-blue';
  }

  // Auto Night Mode
  const ledAutoNight = document.getElementById('ledAutoNight');
  document.getElementById('summaryAutoNight').textContent = data.autoNightMode ? 'Attivo' : 'Disattivo';
  ledAutoNight.className = data.autoNightMode ? 'led-indicator led-on' : 'led-indicator led-off';

  // Power Save
  const ledPowerSave = document.getElementById('ledPowerSave');
  document.getElementById('summaryPowerSave').textContent = data.powerSave ? 'Attivo' : 'Disattivo';
  ledPowerSave.className = data.powerSave ? 'led-indicator led-on led-warning' : 'led-indicator led-off';

  // Luminosità Display Corrente
  const ledBrightness = document.getElementById('ledBrightness');
  const brightness = data.displayBrightness || data.currentBrightness || data.lastAppliedBrightness || 0;
  const brightnessPercent = Math.round(brightness / 255 * 100);
  document.getElementById('summaryBrightness').textContent = brightness + ' (' + brightnessPercent + '%)';
  // Colore LED in base alla luminosità
  const briHue = Math.round(brightness / 255 * 60); // Da rosso (0) a giallo (60)
  ledBrightness.style.background = 'hsl(' + briHue + ', 100%, 50%)';
  ledBrightness.style.boxShadow = '0 0 8px hsl(' + briHue + ', 100%, 50%)';

  // Presenza
  const ledPresence = document.getElementById('ledPresence');
  document.getElementById('summaryPresence').textContent = data.presenceDetected ? 'Rilevata' : 'Assente';
  ledPresence.className = data.presenceDetected ? 'led-indicator led-on led-cyan led-blink' : 'led-indicator led-off';

  // Annuncio Orario
  const ledHourlyAnnounce = document.getElementById('ledHourlyAnnounce');
  document.getElementById('summaryHourlyAnnounce').textContent = data.hourlyAnnounce ? 'Attivo' : 'Disattivo';
  ledHourlyAnnounce.className = data.hourlyAnnounce ? 'led-indicator led-on' : 'led-indicator led-off';

  // Suoni Touch
  const ledTouchSounds = document.getElementById('ledTouchSounds');
  document.getElementById('summaryTouchSounds').textContent = data.touchSounds ? 'Attivo' : 'Disattivo';
  ledTouchSounds.className = data.touchSounds ? 'led-indicator led-on' : 'led-indicator led-off';

  // Random Mode
  const ledRandomMode = document.getElementById('ledRandomMode');
  let randomText = data.randomModeEnabled ? (data.randomModeInterval + ' min') : 'Disattivo';
  document.getElementById('summaryRandomMode').textContent = randomText;
  ledRandomMode.className = data.randomModeEnabled ? 'led-indicator led-on led-warning led-blink' : 'led-indicator led-off';

  // Lettera E
  const ledLetterE = document.getElementById('ledLetterE');
  const letterEText = data.letterE === 0 ? 'Visibile' : 'Lampeggiante';
  document.getElementById('summaryLetterE').textContent = letterEText;
  ledLetterE.className = data.letterE === 1 ? 'led-indicator led-on led-blink' : 'led-indicator led-on';

  // Sensori - Radar (mostra solo se disponibile)
  const summaryRadarCard = document.getElementById('summaryRadarCard');
  const summaryPresenceCard = document.getElementById('summaryPresenceCard');
  const ledRadar = document.getElementById('ledRadar');
  if (data.radarAvailable) {
    summaryRadarCard.style.display = 'flex';
    summaryPresenceCard.style.display = 'flex';
    // Mostra tipo radar nel summary (interno/esterno)
    const radarTypeText = data.radarType === 'esterno' ? 'Esterno' : (data.radarType === 'interno' ? 'Interno' : 'OK');
    document.getElementById('summaryRadar').textContent = radarTypeText;
    ledRadar.className = 'led-indicator led-on';
  } else {
    summaryRadarCard.style.display = 'none';
    summaryPresenceCard.style.display = 'none';
  }

  // Sensori - Ambiente (BME280 interno o Radar remoto)
  const summaryBME280Card = document.getElementById('summaryBME280Card');
  const ledBME280 = document.getElementById('ledBME280');
  if (data.indoorSensorSource > 0) {
    summaryBME280Card.style.display = 'flex';
    if (data.tempIndoor) {
      document.getElementById('summaryBME280').textContent = data.tempIndoor.toFixed(1) + '°C';
    } else {
      document.getElementById('summaryBME280').textContent = 'OK';
    }
    // LED verde se BME280 interno, giallo se radar remoto
    ledBME280.className = data.indoorSensorSource === 1 ? 'led-indicator led-on' : 'led-indicator led-on led-warning';
  } else {
    summaryBME280Card.style.display = 'none';
  }

  // Audio I2S (mostra solo se connesso, includi Annuncio Orario e Suoni Touch)
  const summaryAudioSlaveCard = document.getElementById('summaryAudioSlaveCard');
  const summaryHourlyAnnounceCard = document.getElementById('summaryHourlyAnnounceCard');
  const summaryTouchSoundsCard = document.getElementById('summaryTouchSoundsCard');
  const ledAudioSlave = document.getElementById('ledAudioSlave');
  if (data.audioSlaveConnected) {
    summaryAudioSlaveCard.style.display = 'flex';
    summaryHourlyAnnounceCard.style.display = 'flex';
    summaryTouchSoundsCard.style.display = 'flex';
    document.getElementById('summaryAudioSlave').textContent = 'OK';
    ledAudioSlave.className = 'led-indicator led-on';
  } else {
    summaryAudioSlaveCard.style.display = 'none';
    summaryHourlyAnnounceCard.style.display = 'none';
    summaryTouchSoundsCard.style.display = 'none';
  }

  // Meteo Online (mostra solo se disponibile)
  const summaryWeatherCard = document.getElementById('summaryWeatherCard');
  const ledWeather = document.getElementById('ledWeather');
  if (data.weatherAvailable) {
    summaryWeatherCard.style.display = 'flex';
    if (data.tempOutdoor > -900) {
      document.getElementById('summaryWeather').textContent = data.tempOutdoor.toFixed(1) + '°C';
    } else {
      document.getElementById('summaryWeather').textContent = 'OK';
    }
    ledWeather.className = 'led-indicator led-on led-cyan';
  } else {
    summaryWeatherCard.style.display = 'none';
  }

  // Magnetometro QMC5883P - mostra solo se connesso
  const summaryMagCard = document.getElementById('summaryMagCard');
  const ledMagnetometer = document.getElementById('ledMagnetometer');
  if (data.magnetometerConnected) {
    summaryMagCard.style.display = 'flex';
    document.getElementById('summaryMagnetometer').textContent = data.magnetometerHeading.toFixed(0) + '° ' + data.magnetometerDirection;
    ledMagnetometer.className = data.magnetometerCalibrated ? 'led-indicator led-on led-purple' : 'led-indicator led-on led-warning';
  } else {
    summaryMagCard.style.display = 'none';
  }

  // SD Card
  const ledSD = document.getElementById('ledSD');
  document.getElementById('summarySD').textContent = data.sdAvailable ? 'OK' : 'N/D';
  ledSD.className = data.sdAvailable ? 'led-indicator led-on' : 'led-indicator led-off';

  // WiFi
  const ledWiFi = document.getElementById('ledWiFi');
  document.getElementById('summaryWiFi').textContent = data.wifiRSSI ? (data.wifiRSSI + ' dBm') : 'OK';
  ledWiFi.className = 'led-indicator led-on led-blue';
}

// Aggiorna la descrizione dell'annuncio orario in base agli orari giorno/notte
function updateHourlyAnnounceDesc() {
  const dayStartH = parseInt(document.getElementById('dayStartHour').value) || 8;
  const dayStartM = parseInt(document.getElementById('dayStartMinute').value) || 0;
  const nightStartH = parseInt(document.getElementById('nightStartHour').value) || 22;
  const nightStartM = parseInt(document.getElementById('nightStartMinute').value) || 0;
  const descEl = document.getElementById('hourlyAnnounceDesc');
  if (descEl) {
    const dayTime = dayStartH + ':' + (dayStartM < 10 ? '0' : '') + dayStartM;
    const nightTime = nightStartH + ':' + (nightStartM < 10 ? '0' : '') + nightStartM;
    descEl.textContent = 'Annuncia l\'ora ogni ora (tra ' + dayTime + ' e ' + nightTime + ')';
  }
}

function loadSettings() {
  setStatus('saving', 'Caricamento...');
  console.log('[LOAD] Caricamento impostazioni...');

  // Aggiungi timestamp per evitare cache del browser
  fetch('/settings/config?t=' + Date.now())
    .then(r => r.json())
    .then(data => {
      console.log('[LOAD] Dati ricevuti:', data);
      console.log('[LOAD] brightnessDay:', data.brightnessDay, 'brightnessNight:', data.brightnessNight);
      // Aggiorna sezione riassunto stato
      updateStatusSummary(data);
      // Display
      document.getElementById('brightnessDay').value = data.brightnessDay || 250;
      document.getElementById('brightnessDayVal').textContent = data.brightnessDay || 250;
      document.getElementById('brightnessNight').value = data.brightnessNight || 90;
      document.getElementById('brightnessNightVal').textContent = data.brightnessNight || 90;
      document.getElementById('autoNightMode').checked = data.autoNightMode !== false;
      document.getElementById('powerSave').checked = data.powerSave === true;
      document.getElementById('radarBrightnessControl').checked = data.radarBrightnessControl !== false;
      document.getElementById('radarBrightnessMin').value = data.radarBrightnessMin || 90;
      document.getElementById('radarBrightnessMinVal').textContent = data.radarBrightnessMin || 90;
      document.getElementById('radarBrightnessMax').value = data.radarBrightnessMax || 255;
      document.getElementById('radarBrightnessMaxVal').textContent = data.radarBrightnessMax || 255;

      // Aggiorna UI luminosità radar
      updateRadarBrightnessDisplay(data);
      updateBrightnessUI();
      // Disabilita controlli radar se radar non disponibile
      updateRadarControlsAvailability(data.radarAvailable);
      // Carica configurazione radar server remoto
      loadRadarServerConfig();

      // Orari
      document.getElementById('dayStartHour').value = data.dayStartHour || 8;
      document.getElementById('dayStartMinute').value = data.dayStartMinute || 0;
      document.getElementById('nightStartHour').value = data.nightStartHour || 22;
      document.getElementById('nightStartMinute').value = data.nightStartMinute || 0;
      // Aggiorna descrizione annuncio orario
      updateHourlyAnnounceDesc();

      // Audio
      document.getElementById('hourlyAnnounce').checked = data.hourlyAnnounce !== false;
      document.getElementById('useTTSAnnounce').checked = data.useTTSAnnounce === true;
      document.getElementById('ttsVoiceFemale').value = data.ttsVoiceFemale !== false ? '1' : '0';
      // Voce TTS nascosta permanentemente - non mostrare mai
      // document.getElementById('ttsVoiceRow').style.display = data.useTTSAnnounce ? '' : 'none';
      document.getElementById('touchSounds').checked = data.touchSounds !== false;
      document.getElementById('touchSoundsVolume').value = data.touchSoundsVolume || 50;
      document.getElementById('touchSoundsVolumeVal').textContent = data.touchSoundsVolume || 50;
      // Mostra/nascondi slider volume in base a touchSounds abilitato
      document.getElementById('touchSoundsVolumeRow').style.display = data.touchSounds !== false ? '' : 'none';
      document.getElementById('vuMeterShow').checked = data.vuMeterShow !== false;

      // Volume annunci orari
      document.getElementById('announceVolume').value = data.announceVolume || 15;
      document.getElementById('announceVolumeVal').textContent = data.announceVolume || 15;

      // Audio giorno/notte
      document.getElementById('volumeDay').value = data.volumeDay || 80;
      document.getElementById('volumeDayVal').textContent = (data.volumeDay || 80) + '%';
      document.getElementById('volumeNight').value = data.volumeNight || 30;
      document.getElementById('volumeNightVal').textContent = (data.volumeNight || 30) + '%';

      // Effetti - usa modePreset per il preset specifico della modalità corrente
      // modePreset: 255 = nessun preset, 100 = rainbow, 0-14 = preset ID
      if (data.modePreset !== undefined) {
        if (data.modePreset === 100) {
          rainbowActive = true;
          activePresetId = 0;
        } else if (data.modePreset === 255) {
          rainbowActive = false;
          activePresetId = 13; // Personalizzato
        } else {
          rainbowActive = false;
          activePresetId = data.modePreset;
        }
      } else {
        // Fallback
        activePresetId = data.colorPreset || 0;
        rainbowActive = data.rainbowMode === true;
      }
      document.getElementById('colorPreset').value = activePresetId;
      if (data.customColor) {
        document.getElementById('customColor').value = data.customColor;
        const quickPicker = document.getElementById('quickColorPicker');
        if (quickPicker) quickPicker.value = data.customColor;
        // Aggiorna anteprima visiva
        updateColorPreviewGlobal(data.customColor);
      }
      document.getElementById('letterE').value = data.letterE || 1;
      document.getElementById('randomModeInterval').value = data.randomModeInterval || 5;
      document.getElementById('randomModeIntervalVal').textContent = (data.randomModeInterval || 5) + ' min';

      // Random Mode toggle nella sezione modalità
      document.getElementById('randomModeToggle').checked = data.randomModeEnabled === true;
      document.getElementById('randomIntervalDisplay').textContent = data.randomModeInterval || 5;
      updateRandomModeBoxStyle(data.randomModeEnabled === true);

      // Web Radio toggle e stato
      updateWebRadioUI(data);

      // Modalità
      currentMode = data.displayMode || 2;
      // Aggiorna flag API key meteo (per nascondere modalità WEATHER se non configurata)
      hasWeatherApiKey = data.hasWeatherApiKey === true;
      if (data.enabledModes) {
        // Reset e carica da server
        MODES.forEach(m => enabledModes[m.id] = false);
        data.enabledModes.forEach(id => enabledModes[id] = true);
      }
      renderModes();
      renderPresets();  // Aggiorna preset filtrati per la modalità corrente
      updateColorControlsVisibility();  // Mostra/nasconde controlli colore in base alla modalità

      // Sensori - Radar
      document.getElementById('radarStatus').textContent = data.radarAvailable ? 'OK' : 'N/D';
      document.getElementById('presenceStatus').textContent = data.presenceDetected ? 'Sì' : 'No';
      document.getElementById('lightLevel').textContent = data.lightLevel || '--';
      document.getElementById('displayBrightness').textContent = data.displayBrightness !== undefined ? data.displayBrightness : '--';
      // Tipo radar (interno/esterno)
      const radarTypeEl = document.getElementById('radarType');
      if (radarTypeEl && data.radarType) {
        radarTypeEl.textContent = data.radarType === 'esterno' ? 'Esterno' : (data.radarType === 'interno' ? 'Interno' : 'N/D');
      }
      // Card radar esterno
      const radarRemoteCard = document.getElementById('radarRemoteCard');
      if (radarRemoteCard) {
        if (data.radarServerConnected && data.radarType === 'esterno') {
          radarRemoteCard.style.display = 'block';
          const radarServerIPEl = document.getElementById('radarServerIPDisplay');
          if (radarServerIPEl && data.radarServerIP) {
            radarServerIPEl.textContent = data.radarServerIP;
          }
          const radarRemoteTempEl = document.getElementById('radarRemoteTemp');
          if (radarRemoteTempEl) {
            radarRemoteTempEl.textContent = (data.radarRemoteTemp !== undefined && data.radarRemoteTemp > -900) ? data.radarRemoteTemp.toFixed(1) + ' C' : '--';
          }
          const radarRemoteHumEl = document.getElementById('radarRemoteHum');
          if (radarRemoteHumEl) {
            radarRemoteHumEl.textContent = (data.radarRemoteHum !== undefined && data.radarRemoteHum > 0) ? data.radarRemoteHum.toFixed(0) + '%' : '--';
          }
        } else {
          radarRemoteCard.style.display = 'none';
        }
      }
      // Sensori - BME280 / Radar remoto
      document.getElementById('bme280Status').textContent = data.bme280Available ? 'BME280 OK' : 'N/D';
      // Mostra fonte dati sensore ambiente
      const sourceNames = ['Nessuna', 'BME280 Interno', 'Radar Remoto'];
      const sourceEl = document.getElementById('indoorSensorSource');
      if (data.indoorSensorSource !== undefined) {
        sourceEl.textContent = sourceNames[data.indoorSensorSource] || 'N/D';
        sourceEl.style.color = data.indoorSensorSource === 1 ? 'var(--success)' : (data.indoorSensorSource === 2 ? 'var(--warning)' : 'var(--text-muted)');
      }
      document.getElementById('tempIndoor').textContent = (data.indoorSensorSource > 0 && data.tempIndoor) ? data.tempIndoor.toFixed(1) + '°C' : '--';
      document.getElementById('humIndoor').textContent = (data.indoorSensorSource > 0 && data.humIndoor !== undefined) ? data.humIndoor.toFixed(0) + '%' : '--';
      document.getElementById('pressIndoor').textContent = (data.indoorSensorSource === 1 && data.pressIndoor) ? data.pressIndoor.toFixed(0) + ' hPa' : '--';
      // Mostra/nasconde card calibrazione (disponibile se c'è una fonte dati)
      const bme280CalibCard = document.getElementById('bme280CalibCard');
      if (data.indoorSensorSource > 0) {
        bme280CalibCard.style.display = 'block';
        // Carica offset solo al primo caricamento (non sovrascrivere se utente sta modificando)
        if (!window.bme280CalibLoaded) {
          document.getElementById('bme280TempOffset').value = data.bme280TempOffset || 0;
          document.getElementById('bme280HumOffset').value = data.bme280HumOffset || 0;
          window.bme280CalibLoaded = true;
        }
      } else {
        bme280CalibCard.style.display = 'none';
      }
      document.getElementById('weatherStatus').textContent = data.weatherAvailable ? 'OK' : 'N/D';
      document.getElementById('tempOutdoor').textContent = (data.weatherAvailable && data.tempOutdoor > -900) ? data.tempOutdoor.toFixed(1) + '°C' : '--';
      document.getElementById('humOutdoor').textContent = (data.weatherAvailable && data.humOutdoor > -900) ? data.humOutdoor.toFixed(0) + '%' : '--';

      // Magnetometro QMC5883P - mostra/nasconde card in base alla connessione
      const magCard = document.getElementById('magnetometerCard');
      if (data.magnetometerConnected) {
        magCard.style.display = 'block';
        document.getElementById('magStatus').textContent = 'QMC5883P';
        document.getElementById('magHeading').textContent = data.magnetometerHeading.toFixed(1) + '°';
        document.getElementById('magDirection').textContent = data.magnetometerDirection || '--';
        document.getElementById('magCalibrated').textContent = data.magnetometerCalibrated ? 'Sì' : 'No';
      } else {
        magCard.style.display = 'none';
      }

      // Rete
      document.getElementById('wifiSSID').textContent = data.wifiSSID || '--';
      document.getElementById('ipAddress').textContent = data.ipAddress || '--';
      document.getElementById('wifiRSSI').textContent = data.wifiRSSI ? data.wifiRSSI + ' dBm' : '--';
      document.getElementById('audioSlaveStatus').textContent = data.audioSlaveConnected ? 'OK' : 'N/D';

      // Sistema
      document.getElementById('uptime').textContent = formatUptime(data.uptime || 0);
      document.getElementById('freeHeap').textContent = data.freeHeap ? (data.freeHeap / 1024).toFixed(1) + ' KB' : '--';
      document.getElementById('sdStatus').textContent = data.sdAvailable ? 'OK' : 'N/D';

      // ESP-CAM - salva URL e aggiorna visibilità link
      if (data.esp32camEnabled && data.esp32camWebUrl) {
        esp32camWebUrl = data.esp32camWebUrl;
      }
      updateEspCamLink();

      // Lingua
      if (data.language !== undefined) {
        document.getElementById('displayLanguage').value = data.language;
        const langNames = ['Italiano', 'English'];
        const langCodes = ['IT', 'EN'];
        document.getElementById('currentLanguage').textContent = langNames[data.language] || 'Italiano';
        document.getElementById('wordClockLanguage').textContent = langCodes[data.language] || 'IT';
      }

      // Ora
      if (data.currentHour !== undefined) {
        const h = String(data.currentHour).padStart(2, '0');
        const m = String(data.currentMinute).padStart(2, '0');
        const s = String(data.currentSecond).padStart(2, '0');
        document.getElementById('currentTime').textContent = h + ':' + m + ':' + s;
        // Calcola se è giorno o notte considerando anche i minuti
        const currentMinutes = data.currentHour * 60 + data.currentMinute;
        const dayStartMinutes = (data.dayStartHour||8) * 60 + (data.dayStartMinute||0);
        const nightStartMinutes = (data.nightStartHour||22) * 60 + (data.nightStartMinute||0);
        let isDay;
        if (dayStartMinutes < nightStartMinutes) {
          isDay = (currentMinutes >= dayStartMinutes && currentMinutes < nightStartMinutes);
        } else {
          isDay = (currentMinutes >= dayStartMinutes || currentMinutes < nightStartMinutes);
        }
        document.getElementById('currentDayMode').textContent = isDay ? '☀️ Giorno' : '🌙 Notte';
      }

      setStatus('online', 'Connesso');
      document.getElementById('lastSave').textContent = 'Aggiornato: ' + new Date().toLocaleTimeString();
    })
    .catch(err => {
      console.error(err);
      setStatus('offline', 'Errore');
    });
}

function saveAllSettings() {
  setStatus('saving', 'Salvataggio...');

  const params = new URLSearchParams({
    brightnessDay: document.getElementById('brightnessDay').value,
    brightnessNight: document.getElementById('brightnessNight').value,
    autoNightMode: document.getElementById('autoNightMode').checked ? 1 : 0,
    powerSave: document.getElementById('powerSave').checked ? 1 : 0,
    radarBrightnessControl: document.getElementById('radarBrightnessControl').checked ? 1 : 0,
    radarBrightnessMin: document.getElementById('radarBrightnessMin').value,
    radarBrightnessMax: document.getElementById('radarBrightnessMax').value,
    dayStartHour: document.getElementById('dayStartHour').value,
    dayStartMinute: document.getElementById('dayStartMinute').value,
    nightStartHour: document.getElementById('nightStartHour').value,
    nightStartMinute: document.getElementById('nightStartMinute').value,
    hourlyAnnounce: document.getElementById('hourlyAnnounce').checked ? 1 : 0,
    useTTSAnnounce: document.getElementById('useTTSAnnounce').checked ? 1 : 0,
    ttsVoiceFemale: parseInt(document.getElementById('ttsVoiceFemale').value),
    touchSounds: document.getElementById('touchSounds').checked ? 1 : 0,
    touchSoundsVolume: document.getElementById('touchSoundsVolume').value,
    vuMeterShow: document.getElementById('vuMeterShow').checked ? 1 : 0,
    announceVolume: document.getElementById('announceVolume').value,
    volumeDay: document.getElementById('volumeDay').value,
    volumeNight: document.getElementById('volumeNight').value,
    colorPreset: document.getElementById('colorPreset').value,
    customColor: document.getElementById('customColor').value,
    letterE: document.getElementById('letterE').value,
    randomModeEnabled: document.getElementById('randomModeToggle').checked ? 1 : 0,
    randomModeInterval: document.getElementById('randomModeInterval').value
  });

  console.log('[SAVE] Invio parametri:', params.toString());

  fetch('/settings/save?' + params)
    .then(r => r.text())
    .then(result => {
      console.log('[SAVE] Risposta server:', result);
      if (result === 'OK') {
        setStatus('online', 'Salvato!');
        document.getElementById('lastSave').textContent = 'Salvato: ' + new Date().toLocaleTimeString();
        // Forza refresh immediato per sincronizzare UI con valori dal server
        refreshStatusNow();
        setTimeout(() => setStatus('online', 'Connesso'), 2000);
      } else {
        console.error('[SAVE] Errore risposta:', result);
        setStatus('offline', 'Errore');
      }
    })
    .catch(err => {
      console.error('[SAVE] Errore fetch:', err);
      setStatus('offline', 'Errore');
    });
}

function resetSettings() {
  if (confirm('Ripristinare tutte le impostazioni?')) {
    fetch('/settings/reset').then(() => {
      alert('Reset completato. Riavvio...');
      setTimeout(() => location.reload(), 3000);
    });
  }
}

function rebootDevice() {
  if (confirm('Riavviare il dispositivo?')) {
    fetch('/settings/reboot').then(() => {
      alert('Riavvio in corso...');
      setTimeout(() => location.reload(), 5000);
    });
  }
}

// Cambio lingua display
function changeLanguage(langValue) {
  setStatus('saving', 'Changing language...');
  fetch('/settings/setlanguage?lang=' + langValue)
    .then(r => r.text())
    .then(result => {
      if (result === 'OK') {
        // Aggiorna UI
        const langNames = ['Italiano', 'English'];
        const langCodes = ['IT', 'EN'];
        document.getElementById('currentLanguage').textContent = langNames[langValue] || 'Italiano';
        document.getElementById('wordClockLanguage').textContent = langCodes[langValue] || 'IT';
        setStatus('online', 'Language changed!');
        setTimeout(() => setStatus('online', 'Connesso'), 2000);
      } else {
        setStatus('offline', 'Error');
      }
    })
    .catch(() => setStatus('offline', 'Error'));
}

let apiKeySaveTimeout = null;

function saveApiKeys() {
  setStatus('saving', 'Salvataggio impostazioni...');
  const cityValue = document.getElementById('openweatherCity').value;
  const params = new URLSearchParams({
    openweatherCity: cityValue
  });
  fetch('/settings/saveapikeys?' + params)
    .then(r => r.text())
    .then(result => {
      if (result === 'OK') {
        setStatus('online', 'Impostazioni salvate!');
        document.getElementById('apiKeysStatus').style.display = 'flex';
        document.getElementById('lastSave').textContent = 'Salvato: ' + new Date().toLocaleTimeString();
        // Aggiorna visibilità modalità Weather in base alla città (Open-Meteo non richiede API key)
        const cityValid = cityValue.length >= 2;
        if (cityValid && !hasWeatherApiKey) {
          hasWeatherApiKey = true;
          renderModes();
        }
        // Ricarica le impostazioni
        loadApiKeys();
        setTimeout(() => {
          setStatus('online', 'Connesso');
          document.getElementById('apiKeysStatus').style.display = 'none';
        }, 3000);
      } else {
        setStatus('offline', 'Errore salvataggio');
      }
    })
    .catch(() => setStatus('offline', 'Errore'));
}

function setupApiKeysAutoSave() {
  const apiKeyInputs = ['openweatherCity'];
  apiKeyInputs.forEach(inputId => {
    const input = document.getElementById(inputId);
    if (input) {
      // Salva su blur (quando l'input perde il focus)
      input.addEventListener('blur', () => {
        if (apiKeySaveTimeout) clearTimeout(apiKeySaveTimeout);
        apiKeySaveTimeout = setTimeout(saveApiKeys, 300);
      });
      // Salva anche dopo 1.5 secondi di inattività durante la digitazione
      input.addEventListener('input', () => {
        if (apiKeySaveTimeout) clearTimeout(apiKeySaveTimeout);
        apiKeySaveTimeout = setTimeout(saveApiKeys, 1500);
      });
    }
  });
}

function loadApiKeys() {
  fetch('/settings/getapikeys')
    .then(r => r.json())
    .then(data => {
      if (data.openweatherCity) document.getElementById('openweatherCity').value = data.openweatherCity;
    })
    .catch(() => console.log('Impostazioni meteo non disponibili'));
}

function formatUptime(s) {
  const d = Math.floor(s / 86400);
  const h = Math.floor((s % 86400) / 3600);
  const m = Math.floor((s % 3600) / 60);
  if (d > 0) return d + 'g ' + h + 'h';
  if (h > 0) return h + 'h ' + m + 'm';
  return m + 'm ' + (s % 60) + 's';
}

// Aggiornamento leggero del summary (solo valori dinamici)
function updateStatusSummaryLight(data) {
  // Modalità Display
  const modeName = MODE_NAMES[data.displayMode] || 'Mode ' + data.displayMode;
  document.getElementById('summaryDisplayMode').textContent = modeName;

  // Preset / Colore - aggiorna sia LED che testo
  if (data.colorPreset !== undefined) {
    const presetName = PRESET_NAMES[data.colorPreset] || 'Preset ' + data.colorPreset;
    document.getElementById('summaryPreset').textContent = presetName;
  }
  if (data.customColor) {
    const ledPreset = document.getElementById('ledPreset');
    ledPreset.style.background = data.customColor;
    ledPreset.style.boxShadow = '0 0 8px ' + data.customColor + ', 0 0 12px ' + data.customColor;
  }

  // Presenza
  const ledPresence = document.getElementById('ledPresence');
  document.getElementById('summaryPresence').textContent = data.presenceDetected ? 'Rilevata' : 'Assente';
  ledPresence.className = data.presenceDetected ? 'led-indicator led-on led-cyan led-blink' : 'led-indicator led-off';

  // Luminosità Display Corrente (aggiornamento dinamico)
  if (data.displayBrightness !== undefined || data.currentBrightness !== undefined || data.lastAppliedBrightness !== undefined) {
    const ledBrightness = document.getElementById('ledBrightness');
    const brightness = data.displayBrightness || data.currentBrightness || data.lastAppliedBrightness || 0;
    const brightnessPercent = Math.round(brightness / 255 * 100);
    document.getElementById('summaryBrightness').textContent = brightness + ' (' + brightnessPercent + '%)';
    const briHue = Math.round(brightness / 255 * 60);
    ledBrightness.style.background = 'hsl(' + briHue + ', 100%, 50%)';
    ledBrightness.style.boxShadow = '0 0 8px hsl(' + briHue + ', 100%, 50%)';
  }

  // Lettera E
  const ledLetterE = document.getElementById('ledLetterE');
  const letterEText = data.letterE === 0 ? 'Visibile' : 'Lampeggiante';
  document.getElementById('summaryLetterE').textContent = letterEText;
  ledLetterE.className = data.letterE === 1 ? 'led-indicator led-on led-blink' : 'led-indicator led-on';

  // Fascia oraria basata sull'ora corrente
  if (data.currentHour !== undefined) {
    const currentMinutes = data.currentHour * 60 + (data.currentMinute || 0);
    // Usa i valori dai campi input
    const dayStartH = parseInt(document.getElementById('dayStartHour').value) || 8;
    const dayStartM = parseInt(document.getElementById('dayStartMinute').value) || 0;
    const nightStartH = parseInt(document.getElementById('nightStartHour').value) || 22;
    const nightStartM = parseInt(document.getElementById('nightStartMinute').value) || 0;
    const dayStartMinutes = dayStartH * 60 + dayStartM;
    const nightStartMinutes = nightStartH * 60 + nightStartM;
    let isDay;
    if (dayStartMinutes < nightStartMinutes) {
      isDay = (currentMinutes >= dayStartMinutes && currentMinutes < nightStartMinutes);
    } else {
      isDay = (currentMinutes >= dayStartMinutes || currentMinutes < nightStartMinutes);
    }
    document.getElementById('summaryDayNight').textContent = isDay ? 'Giorno' : 'Notte';
    document.getElementById('ledDayNight').className = isDay ? 'led-indicator led-on led-yellow' : 'led-indicator led-on led-blue';
  }

  // Sensore ambiente (BME280 interno o Radar remoto) - mostra/nasconde card
  if (data.indoorSensorSource !== undefined) {
    const summaryBME280Card = document.getElementById('summaryBME280Card');
    const ledBME280 = document.getElementById('ledBME280');
    if (data.indoorSensorSource > 0) {
      summaryBME280Card.style.display = 'flex';
      if (data.tempIndoor !== undefined) {
        document.getElementById('summaryBME280').textContent = data.tempIndoor.toFixed(1) + '°C';
      } else {
        document.getElementById('summaryBME280').textContent = 'OK';
      }
      // LED verde se BME280 interno, giallo se radar remoto
      ledBME280.className = data.indoorSensorSource === 1 ? 'led-indicator led-on' : 'led-indicator led-on led-warning';
    } else {
      summaryBME280Card.style.display = 'none';
    }
  } else if (data.tempIndoor !== undefined) {
    document.getElementById('summaryBME280').textContent = data.tempIndoor.toFixed(1) + '°C';
  }

  // Radar LD2410 (mostra/nasconde card)
  if (data.radarAvailable !== undefined) {
    const summaryRadarCard = document.getElementById('summaryRadarCard');
    const summaryPresenceCard = document.getElementById('summaryPresenceCard');
    const ledRadar = document.getElementById('ledRadar');
    if (data.radarAvailable) {
      summaryRadarCard.style.display = 'flex';
      summaryPresenceCard.style.display = 'flex';
      // Mostra tipo radar nel summary (interno/esterno)
      const radarTypeText = data.radarType === 'esterno' ? 'Esterno' : (data.radarType === 'interno' ? 'Interno' : 'OK');
      document.getElementById('summaryRadar').textContent = radarTypeText;
      ledRadar.className = 'led-indicator led-on';
    } else {
      summaryRadarCard.style.display = 'none';
      summaryPresenceCard.style.display = 'none';
    }
  }

  // Meteo Online (mostra/nasconde card)
  if (data.weatherAvailable !== undefined) {
    const summaryWeatherCard = document.getElementById('summaryWeatherCard');
    const ledWeather = document.getElementById('ledWeather');
    if (data.weatherAvailable) {
      summaryWeatherCard.style.display = 'flex';
      if (data.tempOutdoor > -900) {
        document.getElementById('summaryWeather').textContent = data.tempOutdoor.toFixed(1) + '°C';
      } else {
        document.getElementById('summaryWeather').textContent = 'OK';
      }
      ledWeather.className = 'led-indicator led-on led-cyan';
    } else {
      summaryWeatherCard.style.display = 'none';
    }
  }

  // Magnetometro QMC5883P (aggiornamento dinamico)
  if (data.magnetometerConnected !== undefined) {
    const summaryMagCard = document.getElementById('summaryMagCard');
    const ledMagnetometer = document.getElementById('ledMagnetometer');
    if (data.magnetometerConnected) {
      summaryMagCard.style.display = 'flex';
      document.getElementById('summaryMagnetometer').textContent = data.magnetometerHeading.toFixed(0) + '° ' + data.magnetometerDirection;
      ledMagnetometer.className = data.magnetometerCalibrated ? 'led-indicator led-on led-purple' : 'led-indicator led-on led-warning';
    } else {
      summaryMagCard.style.display = 'none';
    }
  }

  // Audio I2S Slave (mostra/nasconde card, includi Annuncio Orario e Suoni Touch)
  if (data.audioSlaveConnected !== undefined) {
    const summaryAudioSlaveCard = document.getElementById('summaryAudioSlaveCard');
    const summaryHourlyAnnounceCard = document.getElementById('summaryHourlyAnnounceCard');
    const summaryTouchSoundsCard = document.getElementById('summaryTouchSoundsCard');
    const ledAudioSlave = document.getElementById('ledAudioSlave');
    if (data.audioSlaveConnected) {
      summaryAudioSlaveCard.style.display = 'flex';
      summaryHourlyAnnounceCard.style.display = 'flex';
      summaryTouchSoundsCard.style.display = 'flex';
      document.getElementById('summaryAudioSlave').textContent = 'OK';
      ledAudioSlave.className = 'led-indicator led-on';
    } else {
      summaryAudioSlaveCard.style.display = 'none';
      summaryHourlyAnnounceCard.style.display = 'none';
      summaryTouchSoundsCard.style.display = 'none';
    }
  }
}

// Funzione per forzare refresh immediato dopo salvataggio
function refreshStatusNow() {
  fetch('/settings/status')
    .then(r => r.json())
    .then(data => {
      // Aggiorna luminosità display corrente
      if (data.displayBrightness !== undefined) {
        document.getElementById('displayBrightness').textContent = data.displayBrightness;
      }
      // Aggiorna slider luminosità (solo se non hanno il focus)
      if (data.brightnessDay !== undefined) {
        const el = document.getElementById('brightnessDay');
        const valEl = document.getElementById('brightnessDayVal');
        if (document.activeElement !== el) {
          el.value = data.brightnessDay;
          if (valEl) valEl.textContent = data.brightnessDay;
        }
      }
      if (data.brightnessNight !== undefined) {
        const el = document.getElementById('brightnessNight');
        const valEl = document.getElementById('brightnessNightVal');
        if (document.activeElement !== el) {
          el.value = data.brightnessNight;
          if (valEl) valEl.textContent = data.brightnessNight;
        }
      }
      // Aggiorna altri parametri modificati
      if (data.autoNightMode !== undefined) {
        document.getElementById('autoNightMode').checked = data.autoNightMode;
      }
      if (data.powerSave !== undefined) {
        document.getElementById('powerSave').checked = data.powerSave;
      }
      // Aggiorna Radar Brightness Control e slider min/max
      if (data.radarBrightnessControl !== undefined) {
        document.getElementById('radarBrightnessControl').checked = data.radarBrightnessControl;
      }
      if (data.radarBrightnessMin !== undefined) {
        const el = document.getElementById('radarBrightnessMin');
        const valEl = document.getElementById('radarBrightnessMinVal');
        if (document.activeElement !== el) {
          el.value = data.radarBrightnessMin;
          if (valEl) valEl.textContent = data.radarBrightnessMin;
        }
      }
      if (data.radarBrightnessMax !== undefined) {
        const el = document.getElementById('radarBrightnessMax');
        const valEl = document.getElementById('radarBrightnessMaxVal');
        if (document.activeElement !== el) {
          el.value = data.radarBrightnessMax;
          if (valEl) valEl.textContent = data.radarBrightnessMax;
        }
      }
      // Aggiorna UI luminosità (mostra/nascondi controlli radar)
      updateBrightnessUI();
      // Aggiorna display luminosità radar
      updateRadarBrightnessDisplay(data);
      // Aggiorna summary stato LED
      updateStatusSummaryLight(data);
      updateSummaryFromUI();
      console.log('[REFRESH] Status aggiornato dopo salvataggio');
    })
    .catch(err => console.error('[REFRESH] Errore:', err));
}

function startAutoRefresh() {
  setInterval(() => {
    fetch('/settings/status')
      .then(r => r.json())
      .then(data => {
        // Ora corrente
        if (data.currentHour !== undefined) {
          const h = String(data.currentHour).padStart(2, '0');
          const m = String(data.currentMinute).padStart(2, '0');
          const s = String(data.currentSecond).padStart(2, '0');
          document.getElementById('currentTime').textContent = h + ':' + m + ':' + s;
        }

        // Info sensori - Sezione Sensori dettagliata
        document.getElementById('presenceStatus').textContent = data.presenceDetected ? 'Sì' : 'No';
        document.getElementById('lightLevel').textContent = data.lightLevel || '--';
        document.getElementById('uptime').textContent = formatUptime(data.uptime || 0);

        // Aggiorna display luminosità radar
        updateRadarBrightnessDisplay(data);

        // Aggiorna dati BME280 nella sezione sensori
        if (data.tempIndoor !== undefined) {
          document.getElementById('tempIndoor').textContent = data.tempIndoor.toFixed(1) + '°C';
        }
        if (data.humIndoor !== undefined) {
          document.getElementById('humIndoor').textContent = data.humIndoor.toFixed(0) + '%';
        }
        if (data.pressIndoor !== undefined) {
          document.getElementById('pressIndoor').textContent = data.pressIndoor.toFixed(0) + ' hPa';
        }

        // Aggiorna dati meteo esterno nella sezione sensori
        if (data.weatherAvailable !== undefined) {
          document.getElementById('weatherStatus').textContent = data.weatherAvailable ? 'OK' : 'N/D';
          if (data.weatherAvailable && data.tempOutdoor > -900) {
            document.getElementById('tempOutdoor').textContent = data.tempOutdoor.toFixed(1) + '°C';
          } else {
            document.getElementById('tempOutdoor').textContent = '--';
          }
          if (data.weatherAvailable && data.humOutdoor > -900) {
            document.getElementById('humOutdoor').textContent = data.humOutdoor.toFixed(0) + '%';
          } else {
            document.getElementById('humOutdoor').textContent = '--';
          }
        }

        // Aggiorna dati magnetometro nella sezione sensori
        const magCard = document.getElementById('magnetometerCard');
        if (data.magnetometerConnected !== undefined) {
          if (data.magnetometerConnected) {
            magCard.style.display = 'block';
            document.getElementById('magStatus').textContent = 'QMC5883P';
            if (data.magnetometerHeading !== undefined) {
              document.getElementById('magHeading').textContent = data.magnetometerHeading.toFixed(1) + '°';
            }
            document.getElementById('magDirection').textContent = data.magnetometerDirection || '--';
            document.getElementById('magCalibrated').textContent = data.magnetometerCalibrated ? 'Sì' : 'No';
          } else {
            magCard.style.display = 'none';
          }
        }

        // Aggiorna stato radar nella sezione sensori
        if (data.radarAvailable !== undefined) {
          document.getElementById('radarStatus').textContent = data.radarAvailable ? 'OK' : 'N/D';
        }
        // Aggiorna tipo radar (interno/esterno)
        if (data.radarType !== undefined) {
          const radarTypeEl = document.getElementById('radarType');
          if (radarTypeEl) {
            radarTypeEl.textContent = data.radarType === 'esterno' ? 'Esterno' : (data.radarType === 'interno' ? 'Interno' : 'N/D');
          }
        }
        // Mostra/nasconde card radar esterno e aggiorna dati
        const radarRemoteCard = document.getElementById('radarRemoteCard');
        if (radarRemoteCard) {
          if (data.radarServerConnected === true && data.radarType === 'esterno') {
            radarRemoteCard.style.display = 'block';
            if (data.radarServerIP) {
              document.getElementById('radarServerIPDisplay').textContent = data.radarServerIP;
            }
            if (data.radarRemoteTemp !== undefined && data.radarRemoteTemp > -900) {
              document.getElementById('radarRemoteTemp').textContent = data.radarRemoteTemp.toFixed(1) + ' C';
            } else {
              document.getElementById('radarRemoteTemp').textContent = '--';
            }
            if (data.radarRemoteHum !== undefined && data.radarRemoteHum > 0) {
              document.getElementById('radarRemoteHum').textContent = data.radarRemoteHum.toFixed(0) + '%';
            } else {
              document.getElementById('radarRemoteHum').textContent = '--';
            }
          } else {
            radarRemoteCard.style.display = 'none';
          }
        }

        // Aggiorna luminosità display corrente
        if (data.displayBrightness !== undefined) {
          document.getElementById('displayBrightness').textContent = data.displayBrightness;
        }

        // Modalità display
        if (data.displayMode !== undefined && data.displayMode !== currentMode) {
          currentMode = data.displayMode;
          renderModes();
          updateColorControlsVisibility();  // Aggiorna visibilità color picker
        }

        // Sincronizza tutti i parametri modificabili dal device
        if (data.colorPreset !== undefined) {
          const el = document.getElementById('colorPreset');
          if (el.value != data.colorPreset) {
            el.value = data.colorPreset;
            updateColorControlsVisibility();  // Aggiorna visibilità color picker
          }
        }
        // Sincronizza customColor dal device (es. quando cambiato da touch display con color cycle)
        // Solo se il color picker non ha il focus (utente non sta modificando)
        if (data.customColor) {
          const colorPicker = document.getElementById('customColor');
          const quickPicker = document.getElementById('quickColorPicker');
          const isPickerActive = document.activeElement === colorPicker || document.activeElement === quickPicker;
          if (!isPickerActive && colorPicker.value.toLowerCase() !== data.customColor.toLowerCase()) {
            colorPicker.value = data.customColor;
            if (quickPicker) quickPicker.value = data.customColor;
            // Aggiorna anteprima visiva in tempo reale
            updateColorPreviewGlobal(data.customColor);
            // Aggiorna anche preset attivo se è cambiato
            if (data.modePreset !== undefined) {
              if (data.modePreset === 13 || data.modePreset === 255) {
                activePresetId = 13;
                rainbowActive = false;
                renderPresets();
              }
            }
          }
        }
        if (data.brightnessDay !== undefined) {
          const el = document.getElementById('brightnessDay');
          const valEl = document.getElementById('brightnessDayVal');
          // Non sovrascrivere se l'utente sta interagendo con lo slider
          if (document.activeElement !== el && el.value != data.brightnessDay) {
            el.value = data.brightnessDay;
            if (valEl) valEl.textContent = data.brightnessDay;
          }
        }
        if (data.brightnessNight !== undefined) {
          const el = document.getElementById('brightnessNight');
          const valEl = document.getElementById('brightnessNightVal');
          // Non sovrascrivere se l'utente sta interagendo con lo slider
          if (document.activeElement !== el && el.value != data.brightnessNight) {
            el.value = data.brightnessNight;
            if (valEl) valEl.textContent = data.brightnessNight;
          }
        }
        if (data.letterE !== undefined) {
          const el = document.getElementById('letterE');
          if (el.value != data.letterE) el.value = data.letterE;
        }

        // Sincronizza checkbox Auto Night Mode e Power Save
        if (data.autoNightMode !== undefined) {
          const el = document.getElementById('autoNightMode');
          if (el.checked !== data.autoNightMode) {
            el.checked = data.autoNightMode;
          }
        }
        if (data.powerSave !== undefined) {
          const el = document.getElementById('powerSave');
          if (el.checked !== data.powerSave) {
            el.checked = data.powerSave;
          }
        }
        // Sincronizza Radar Brightness Control e slider min/max
        if (data.radarBrightnessControl !== undefined) {
          const el = document.getElementById('radarBrightnessControl');
          if (el.checked !== data.radarBrightnessControl) {
            el.checked = data.radarBrightnessControl;
            updateBrightnessUI();  // Aggiorna UI (mostra/nascondi controlli)
          }
        }
        if (data.radarBrightnessMin !== undefined) {
          const el = document.getElementById('radarBrightnessMin');
          const valEl = document.getElementById('radarBrightnessMinVal');
          if (document.activeElement !== el && el.value != data.radarBrightnessMin) {
            el.value = data.radarBrightnessMin;
            if (valEl) valEl.textContent = data.radarBrightnessMin;
          }
        }
        if (data.radarBrightnessMax !== undefined) {
          const el = document.getElementById('radarBrightnessMax');
          const valEl = document.getElementById('radarBrightnessMaxVal');
          if (document.activeElement !== el && el.value != data.radarBrightnessMax) {
            el.value = data.radarBrightnessMax;
            if (valEl) valEl.textContent = data.radarBrightnessMax;
          }
        }
        // Aggiorna display luminosità radar
        updateRadarBrightnessDisplay(data);

        // Aggiorna summary stato LED
        updateStatusSummaryLight(data);

        setStatus('online', 'Connesso');
      })
      .catch(() => setStatus('offline', 'Disconnesso'));
  }, 2000);

  // Check stato radar server ogni 10 secondi (se abilitato)
  setInterval(() => {
    const radarEnabled = document.getElementById('radarServerEnabled');
    if (radarEnabled && radarEnabled.checked) {
      checkRadarServerStatus();
    }
  }, 10000);
}

// ================== CALIBRAZIONE BME280 ==================
function saveBME280Calibration() {
  const tempOffset = parseFloat(document.getElementById('bme280TempOffset').value) || 0;
  const humOffset = parseFloat(document.getElementById('bme280HumOffset').value) || 0;
  const statusEl = document.getElementById('bme280CalibStatus');

  statusEl.textContent = 'Salvataggio...';
  statusEl.style.color = 'var(--warning)';

  fetch('/settings/savebmecalib?tempOffset=' + tempOffset + '&humOffset=' + humOffset + '&t=' + Date.now())
    .then(response => response.json())
    .then(data => {
      if (data.success) {
        statusEl.textContent = '✓ Salvato! Temp: ' + data.tempIndoor + '°C, Hum: ' + data.humIndoor + '%';
        statusEl.style.color = 'var(--success)';
        // Aggiorna i valori visualizzati
        document.getElementById('tempIndoor').textContent = data.tempIndoor + '°C';
        document.getElementById('humIndoor').textContent = data.humIndoor + '%';
        setTimeout(() => { statusEl.textContent = ''; }, 5000);
      } else {
        statusEl.textContent = '✗ Errore salvataggio';
        statusEl.style.color = 'var(--danger)';
      }
    })
    .catch(err => {
      statusEl.textContent = '✗ Errore: ' + err.message;
      statusEl.style.color = 'var(--danger)';
    });
}

function resetBME280Calibration() {
  if (!confirm('Vuoi resettare gli offset di calibrazione a 0?')) return;

  const statusEl = document.getElementById('bme280CalibStatus');
  statusEl.textContent = 'Reset in corso...';
  statusEl.style.color = 'var(--warning)';

  fetch('/settings/resetbmecalib?t=' + Date.now())
    .then(response => response.json())
    .then(data => {
      if (data.success) {
        // Resetta i campi input
        document.getElementById('bme280TempOffset').value = 0;
        document.getElementById('bme280HumOffset').value = 0;
        statusEl.textContent = '✓ Reset completato! Offset a 0';
        statusEl.style.color = 'var(--success)';
        // Aggiorna i valori visualizzati
        document.getElementById('tempIndoor').textContent = data.tempIndoor + '°C';
        document.getElementById('humIndoor').textContent = data.humIndoor + '%';
        setTimeout(() => { statusEl.textContent = ''; }, 5000);
      } else {
        statusEl.textContent = '✗ Errore reset';
        statusEl.style.color = 'var(--danger)';
      }
    })
    .catch(err => {
      statusEl.textContent = '✗ Errore: ' + err.message;
      statusEl.style.color = 'var(--danger)';
    });
}

document.addEventListener('DOMContentLoaded', () => {
  loadSettings();
  loadApiKeys();
  loadWebRadioStations();  // Carica lista stazioni radio
  setupAutoSave();
  setupApiKeysAutoSave();
  startAutoRefresh();
});
</script>
</body>
</html>
)rawliteral";

#endif
