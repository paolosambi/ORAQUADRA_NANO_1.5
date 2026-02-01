#ifndef BTTF_WEB_HTML_H
#define BTTF_WEB_HTML_H

// HTML per pagina web BTTF - in file separato per evitare bug preprocessore Arduino IDE 1.8.19

const char BTTF_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>BTTF Time Circuits</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&display=swap" rel="stylesheet">
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }

body {
  font-family: 'Orbitron', 'Courier New', monospace;
  background: linear-gradient(135deg, #0a0a0a 0%, #1a1a2e 50%, #0a0a0a 100%);
  min-height: 100vh;
  padding: 20px;
  position: relative;
  overflow-x: hidden;
}

body::before {
  content: '';
  position: fixed;
  top: 0; left: 0;
  width: 100%; height: 100%;
  background:
    repeating-linear-gradient(0deg, rgba(0,255,255,0.03) 0px, transparent 1px, transparent 2px, rgba(0,255,255,0.03) 3px),
    repeating-linear-gradient(90deg, rgba(255,0,0,0.03) 0px, transparent 1px, transparent 2px, rgba(255,0,0,0.03) 3px);
  pointer-events: none;
  z-index: 0;
}

.container {
  max-width: 900px;
  margin: 0 auto;
  position: relative;
  z-index: 1;
}

.settings-btn {
  position: fixed;
  top: 15px;
  left: 15px;
  background: rgba(255,100,0,0.3);
  color: #ffaa00;
  border: 2px solid #ffaa00;
  padding: 10px 18px;
  border-radius: 8px;
  cursor: pointer;
  font-size: 0.9rem;
  font-family: 'Orbitron', monospace;
  text-decoration: none;
  transition: all 0.3s;
  z-index: 100;
  text-shadow: 0 0 10px rgba(255,170,0,0.8);
  box-shadow: 0 0 15px rgba(255,100,0,0.3);
}

.settings-btn:hover {
  background: rgba(255,100,0,0.5);
  box-shadow: 0 0 25px rgba(255,100,0,0.6);
}

h1 {
  text-align: center;
  font-size: clamp(24px, 5vw, 48px);
  font-weight: 900;
  margin-bottom: 40px;
  background: linear-gradient(45deg, #ff0000, #ff6600, #ffff00);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  text-shadow: 0 0 30px rgba(255,0,0,0.5);
  animation: glow 2s ease-in-out infinite alternate;
  letter-spacing: 4px;
}

@keyframes glow {
  from { filter: drop-shadow(0 0 20px rgba(255,0,0,0.8)); }
  to { filter: drop-shadow(0 0 40px rgba(255,100,0,1)); }
}

.panel {
  background: rgba(10,10,10,0.8);
  border: 3px solid;
  border-radius: 15px;
  padding: 25px;
  margin: 30px 0;
  position: relative;
  backdrop-filter: blur(10px);
  transition: all 0.3s ease;
  box-shadow: 0 8px 32px rgba(0,0,0,0.5);
}

.panel::before {
  content: '';
  position: absolute;
  top: -3px; left: -3px; right: -3px; bottom: -3px;
  background: inherit;
  border-radius: 15px;
  filter: blur(15px);
  opacity: 0.5;
  z-index: -1;
}

.panel:hover {
  transform: translateY(-5px);
  box-shadow: 0 12px 48px rgba(0,0,0,0.7);
}

.panel-red {
  border-color: #ff0000;
  box-shadow: 0 0 30px rgba(255,0,0,0.3), inset 0 0 20px rgba(255,0,0,0.1);
}

.panel-green {
  border-color: #00ff00;
  box-shadow: 0 0 30px rgba(0,255,0,0.3), inset 0 0 20px rgba(0,255,0,0.1);
}

.panel-amber {
  border-color: #ffaa00;
  box-shadow: 0 0 30px rgba(255,170,0,0.3), inset 0 0 20px rgba(255,170,0,0.1);
}

.panel h2 {
  font-size: clamp(18px, 3vw, 28px);
  font-weight: 700;
  margin-bottom: 20px;
  padding-bottom: 15px;
  border-bottom: 2px solid;
  text-transform: uppercase;
  letter-spacing: 3px;
  display: flex;
  align-items: center;
  gap: 10px;
}

.panel-red h2 {
  color: #ff0000;
  border-color: #ff0000;
  text-shadow: 0 0 10px rgba(255,0,0,0.8);
}

.panel-green h2 {
  color: #00ff00;
  border-color: #00ff00;
  text-shadow: 0 0 10px rgba(0,255,0,0.8);
}

.panel-amber h2 {
  color: #ffaa00;
  border-color: #ffaa00;
  text-shadow: 0 0 10px rgba(255,170,0,0.8);
}

.time-display {
  background: #000;
  border: 2px solid;
  border-radius: 10px;
  padding: 20px;
  margin: 15px 0;
  font-size: clamp(24px, 4vw, 36px);
  font-weight: 700;
  text-align: center;
  letter-spacing: 8px;
  font-family: 'Courier New', monospace;
  box-shadow: inset 0 0 20px rgba(0,0,0,0.8);
}

.panel-red .time-display {
  border-color: #ff0000;
  color: #ff0000;
  text-shadow: 0 0 20px rgba(255,0,0,1);
}

.panel-green .time-display {
  border-color: #00ff00;
  color: #00ff00;
  text-shadow: 0 0 20px rgba(0,255,0,1);
  animation: pulse 2s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.8; }
}

.panel-amber .time-display {
  border-color: #ffaa00;
  color: #ffaa00;
  text-shadow: 0 0 20px rgba(255,170,0,1);
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 15px;
  margin: 15px 0;
}

.form-row {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

label {
  color: #00ddff;
  font-size: 12px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 1px;
}

input, select {
  background: #000;
  color: #00ff00;
  border: 2px solid #00ff00;
  border-radius: 8px;
  padding: 12px;
  font-family: 'Orbitron', monospace;
  font-size: 16px;
  font-weight: 700;
  transition: all 0.3s ease;
  box-shadow: inset 0 0 10px rgba(0,0,0,0.5);
}

input:focus, select:focus {
  outline: none;
  border-color: #00ffff;
  box-shadow: 0 0 15px rgba(0,255,255,0.5), inset 0 0 10px rgba(0,0,0,0.5);
}

input[readonly] {
  cursor: not-allowed;
  opacity: 0.8;
}

.alarm-toggle {
  display: flex;
  align-items: center;
  gap: 15px;
  margin-top: 20px;
  padding: 15px;
  background: rgba(255,255,0,0.1);
  border: 2px dashed #ffff00;
  border-radius: 10px;
  transition: all 0.3s ease;
}

.alarm-toggle:hover {
  background: rgba(255,255,0,0.2);
  border-color: #ffff00;
  box-shadow: 0 0 20px rgba(255,255,0,0.3);
}

.alarm-toggle label {
  flex: 1;
  color: #ffff00;
  font-size: 14px;
  cursor: pointer;
}

input[type="checkbox"] {
  width: 24px;
  height: 24px;
  cursor: pointer;
  accent-color: #ffff00;
}

.status {
  color: #ffff00;
  font-weight: 700;
  text-align: center;
  margin: 20px 0;
  padding: 15px;
  background: rgba(255,255,0,0.1);
  border: 2px solid #ffff00;
  border-radius: 10px;
  font-size: 16px;
  letter-spacing: 2px;
  box-shadow: 0 0 20px rgba(255,255,0,0.3);
  transition: all 0.3s ease;
}

.status.success {
  color: #00ff00;
  border-color: #00ff00;
  background: rgba(0,255,0,0.1);
  animation: flash 0.5s ease;
}

.status.error {
  color: #ff0000;
  border-color: #ff0000;
  background: rgba(255,0,0,0.1);
  animation: shake 0.5s ease;
}

@keyframes flash {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.5; }
}

@keyframes shake {
  0%, 100% { transform: translateX(0); }
  25% { transform: translateX(-10px); }
  75% { transform: translateX(10px); }
}

@media (max-width: 600px) {
  .form-grid {
    grid-template-columns: 1fr;
  }
  h1 { font-size: 24px; }
  .panel { padding: 15px; }
}
</style>
</head>
<body>
<a href="/" class="settings-btn">&larr; Home</a>
<div class="container">
<h1>BTTF TIME CIRCUITS</h1>

<div class="panel panel-red">
  <h2>DESTINATION TIME</h2>
  <div class="time-display" id="dest_display">-- --- ---- --:-- --</div>
  <div class="form-grid">
    <div class="form-row">
      <label>Month</label>
      <input type="number" id="dest_month" min="1" max="12" value="10">
    </div>
    <div class="form-row">
      <label>Day</label>
      <input type="number" id="dest_day" min="1" max="31" value="26">
    </div>
    <div class="form-row">
      <label>Year</label>
      <input type="number" id="dest_year" min="1900" max="2100" value="1985">
    </div>
    <div class="form-row">
      <label>Hour (1-12)</label>
      <input type="number" id="dest_hour" min="1" max="12" value="1">
    </div>
    <div class="form-row">
      <label>Minute</label>
      <input type="number" id="dest_minute" min="0" max="59" value="20">
    </div>
    <div class="form-row">
      <label>AM/PM</label>
      <select id="dest_ampm">
        <option value="AM">AM</option>
        <option value="PM">PM</option>
      </select>
    </div>
  </div>
  <div class="alarm-toggle">
    <label for="dest_alarm">Attiva sveglia</label>
    <input type="checkbox" id="dest_alarm">
  </div>
  <div class="alarm-toggle">
    <label for="dest_daily">Ripeti ogni giorno (solo ora)</label>
    <input type="checkbox" id="dest_daily">
  </div>
</div>

<div class="panel panel-green">
  <h2>PRESENT TIME</h2>
  <div class="time-display" id="pres_display">-- --- ---- --:--:-- --</div>
</div>

<div class="panel panel-amber">
  <h2>LAST TIME DEPARTED</h2>
  <div class="time-display" id="last_display">-- --- ---- --:-- --</div>
  <div class="form-grid">
    <div class="form-row">
      <label>Month</label>
      <input type="number" id="last_month" min="1" max="12" value="11">
    </div>
    <div class="form-row">
      <label>Day</label>
      <input type="number" id="last_day" min="1" max="31" value="5">
    </div>
    <div class="form-row">
      <label>Year</label>
      <input type="number" id="last_year" min="1900" max="2100" value="1955">
    </div>
    <div class="form-row">
      <label>Hour (1-12)</label>
      <input type="number" id="last_hour" min="1" max="12" value="6">
    </div>
    <div class="form-row">
      <label>Minute</label>
      <input type="number" id="last_minute" min="0" max="59" value="0">
    </div>
    <div class="form-row">
      <label>AM/PM</label>
      <select id="last_ampm">
        <option value="AM">AM</option>
        <option value="PM">PM</option>
      </select>
    </div>
  </div>
  <div class="alarm-toggle">
    <label for="last_alarm">Attiva sveglia</label>
    <input type="checkbox" id="last_alarm">
  </div>
  <div class="alarm-toggle">
    <label for="last_daily">Ripeti ogni giorno (solo ora)</label>
    <input type="checkbox" id="last_daily">
  </div>
</div>

<div class="status" id="status">Auto-save attivo</div>

</div>

<script>
const monthNames = ['', 'JAN', 'FEB', 'MAR', 'APR', 'MAY', 'JUN', 'JUL', 'AUG', 'SEP', 'OCT', 'NOV', 'DEC'];

function updateDisplay(prefix, month, day, year, hour, minute, second, ampmOverride) {
  second = second || null;
  ampmOverride = ampmOverride || null;
  const monthName = monthNames[month] || '---';
  const dayStr = String(day).padStart(2, '0');
  const yearStr = String(year);
  const hourStr = String(hour).padStart(2, '0');
  const minStr = String(minute).padStart(2, '0');

  let timeStr = hourStr + ':' + minStr;
  if (second !== null) {
    const secStr = String(second).padStart(2, '0');
    timeStr += ':' + secStr;
  }

  let ampm = ampmOverride;
  if (!ampm) {
    const ampmElement = document.getElementById(prefix + '_ampm');
    if (ampmElement) {
      ampm = ampmElement.value || ampmElement.textContent || 'AM';
    } else {
      ampm = 'AM';
    }
  }

  const display = monthName + ' ' + dayStr + ' ' + yearStr + ' ' + timeStr + ' ' + ampm;
  const displayElement = document.getElementById(prefix + '_display');
  if (displayElement) {
    displayElement.textContent = display;
  }
}

function setupInputListeners(prefix) {
  ['month', 'day', 'year', 'hour', 'minute', 'ampm'].forEach(function(field) {
    const element = document.getElementById(prefix + '_' + field);
    if (element) {
      element.addEventListener('change', function() {
        updateDisplayFromInputs(prefix);
        saveConfig();
      });
      element.addEventListener('input', function() {
        updateDisplayFromInputs(prefix);
      });
    }
  });
}

function setupAlarmCheckboxListeners() {
  ['dest_alarm', 'dest_daily', 'last_alarm', 'last_daily'].forEach(function(id) {
    const checkbox = document.getElementById(id);
    if (checkbox) {
      checkbox.addEventListener('change', function() {
        console.log('Checkbox ' + id + ' changed to: ' + checkbox.checked);
        saveConfig();
      });
    }
  });
}

function updateDisplayFromInputs(prefix) {
  const month = parseInt(document.getElementById(prefix + '_month').value) || 1;
  const day = parseInt(document.getElementById(prefix + '_day').value) || 1;
  const year = parseInt(document.getElementById(prefix + '_year').value) || 2000;
  const hour = parseInt(document.getElementById(prefix + '_hour').value) || 12;
  const minute = parseInt(document.getElementById(prefix + '_minute').value) || 0;
  updateDisplay(prefix, month, day, year, hour, minute);
}

function loadConfig() {
  fetch('/bttf/config')
    .then(function(resp) { return resp.json(); })
    .then(function(data) {
      document.getElementById('dest_month').value = data.destination.month;
      document.getElementById('dest_day').value = data.destination.day;
      document.getElementById('dest_year').value = data.destination.year;
      document.getElementById('dest_hour').value = data.destination.hour;
      document.getElementById('dest_minute').value = data.destination.minute;
      document.getElementById('dest_ampm').value = data.destination.ampm;
      document.getElementById('dest_alarm').checked = data.destination.alarmEnabled;
      document.getElementById('dest_daily').checked = data.destination.dailyRepeat || false;
      updateDisplay('dest', data.destination.month, data.destination.day, data.destination.year,
                    data.destination.hour, data.destination.minute);

      document.getElementById('last_month').value = data.lastDeparted.month;
      document.getElementById('last_day').value = data.lastDeparted.day;
      document.getElementById('last_year').value = data.lastDeparted.year;
      document.getElementById('last_hour').value = data.lastDeparted.hour;
      document.getElementById('last_minute').value = data.lastDeparted.minute;
      document.getElementById('last_ampm').value = data.lastDeparted.ampm;
      document.getElementById('last_alarm').checked = data.lastDeparted.alarmEnabled;
      document.getElementById('last_daily').checked = data.lastDeparted.dailyRepeat || false;
      updateDisplay('last', data.lastDeparted.month, data.lastDeparted.day, data.lastDeparted.year,
                    data.lastDeparted.hour, data.lastDeparted.minute);

      const statusEl = document.getElementById('status');
      statusEl.textContent = 'Configurazione caricata';
      statusEl.className = 'status success';
      setTimeout(function() { statusEl.className = 'status'; }, 2000);
    })
    .catch(function(err) {
      const statusEl = document.getElementById('status');
      statusEl.textContent = 'Errore caricamento';
      statusEl.className = 'status error';
    });
}

function saveConfig() {
  console.log('=== SAVE CONFIG CALLED ===');

  const data = {
    dest_month: document.getElementById('dest_month').value,
    dest_day: document.getElementById('dest_day').value,
    dest_year: document.getElementById('dest_year').value,
    dest_hour: document.getElementById('dest_hour').value,
    dest_minute: document.getElementById('dest_minute').value,
    dest_ampm: document.getElementById('dest_ampm').value,
    dest_alarm: document.getElementById('dest_alarm').checked ? 1 : 0,
    dest_daily: document.getElementById('dest_daily').checked ? 1 : 0,
    last_month: document.getElementById('last_month').value,
    last_day: document.getElementById('last_day').value,
    last_year: document.getElementById('last_year').value,
    last_hour: document.getElementById('last_hour').value,
    last_minute: document.getElementById('last_minute').value,
    last_ampm: document.getElementById('last_ampm').value,
    last_alarm: document.getElementById('last_alarm').checked ? 1 : 0,
    last_daily: document.getElementById('last_daily').checked ? 1 : 0
  };

  console.log('Data to save:', data);

  const params = new URLSearchParams(data);
  const url = '/bttf/save?' + params;
  console.log('Request URL:', url);

  const statusEl = document.getElementById('status');

  statusEl.textContent = 'Salvataggio in corso...';
  statusEl.className = 'status';

  console.log('Sending fetch request...');
  fetch(url)
    .then(function(resp) {
      console.log('Response received:', resp.status, resp.statusText);
      if (resp.ok) {
        return resp.text();
      } else {
        throw new Error('Response not OK: ' + resp.status);
      }
    })
    .then(function(responseText) {
      console.log('Response text:', responseText);

      statusEl.textContent = 'Salvato automaticamente';
      statusEl.className = 'status success';

      updateDisplayFromInputs('dest');
      updateDisplayFromInputs('last');

      setTimeout(function() {
        statusEl.textContent = 'Auto-save attivo';
        statusEl.className = 'status';
      }, 2000);
    })
    .catch(function(err) {
      console.error('Fetch error:', err);
      statusEl.textContent = 'Errore connessione: ' + err.message;
      statusEl.className = 'status error';
    });

  console.log('=== SAVE CONFIG COMPLETED ===');
}

function updatePresentTime() {
  fetch('/bttf/presenttime')
    .then(function(resp) { return resp.json(); })
    .then(function(data) {
      updateDisplay('pres', data.month, data.day, data.year,
                    data.hour, data.minute, data.second, data.ampm);
    })
    .catch(function(err) {
      console.error('Errore aggiornamento present time:', err);
    });
}

let bttfIntervalId = null;

function stopBttfRefresh() {
  if (bttfIntervalId) {
    clearInterval(bttfIntervalId);
    bttfIntervalId = null;
  }
}

window.addEventListener('beforeunload', stopBttfRefresh);
window.addEventListener('pagehide', stopBttfRefresh);

setupInputListeners('dest');
setupInputListeners('last');
setupAlarmCheckboxListeners();
loadConfig();
updatePresentTime();
bttfIntervalId = setInterval(updatePresentTime, 1000);
</script>
</body>
</html>
)rawliteral";

#endif
