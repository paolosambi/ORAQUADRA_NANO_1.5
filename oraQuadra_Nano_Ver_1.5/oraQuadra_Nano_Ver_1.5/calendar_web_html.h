// ================== CALENDAR WEB PAGE HTML ==================
// Pagina calendario con griglia mensile e gestione eventi
// Griglia CSS 7 colonne (Lun-Dom) x 6 righe
// CRUD eventi locali + sincronizzazione Google Calendar
// Accesso: http://<IP_ESP32>:8080/calendar

const char CALENDAR_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>OraQuadra - Calendario</title>
  <style>
    *{margin:0;padding:0;box-sizing:border-box}
    @keyframes gradient{0%,100%{background-position:0% 50%}50%{background-position:100% 50%}}
    @keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}
    :root{--glow-color:#00d9ff;--bg-dark:#0a0a1a;--bg-card:rgba(20,20,40,0.8);--accent:#00d9ff;--accent-rgb:0,217,255;--green:#4CAF50;--red:#e94560;--orange:#ff9500}
    body{font-family:'Segoe UI',system-ui,sans-serif;background:var(--bg-dark);min-height:100vh;color:#fff;overflow-x:hidden}
    .bg-animation{position:fixed;top:0;left:0;width:100%;height:100%;z-index:-1;background:linear-gradient(125deg,#0a0a1a,#1a1a3a,#0a1a2a,#1a0a2a);background-size:400% 400%;animation:gradient 15s ease infinite}
    .container{max-width:900px;margin:0 auto;padding:15px}
    .header{text-align:center;padding:15px 10px}
    .logo{font-size:1.8rem;font-weight:900;letter-spacing:-1px;background:linear-gradient(90deg,#00d9ff,#ff00ff,#00ff00);background-size:300% auto;-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;animation:gradient 3s linear infinite}
    .back-link{display:inline-block;color:var(--accent);text-decoration:none;font-size:0.85rem;margin-bottom:10px;opacity:0.7;transition:opacity 0.3s}
    .back-link:hover{opacity:1}

    /* Month Navigation */
    .month-nav{display:flex;align-items:center;justify-content:center;gap:20px;margin:15px 0}
    .month-nav button{background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.15);color:#fff;width:44px;height:44px;border-radius:50%;font-size:1.2rem;cursor:pointer;transition:all 0.3s;display:flex;align-items:center;justify-content:center}
    .month-nav button:hover{background:rgba(0,217,255,0.2);border-color:var(--accent)}
    .month-title{font-size:1.4rem;font-weight:700;min-width:220px;text-align:center;letter-spacing:1px}
    .month-nav .today-btn{width:auto;padding:0 16px;border-radius:20px;font-size:0.8rem;letter-spacing:1px}

    /* Calendar Grid */
    .cal-grid{background:var(--bg-card);border:1px solid rgba(255,255,255,0.05);border-radius:15px;padding:12px;margin-bottom:20px}
    .cal-weekdays{display:grid;grid-template-columns:repeat(7,1fr);gap:4px;margin-bottom:6px}
    .cal-weekday{text-align:center;font-size:0.75rem;font-weight:600;color:#64748b;text-transform:uppercase;letter-spacing:1px;padding:6px 0}
    .cal-days{display:grid;grid-template-columns:repeat(7,1fr);gap:4px}
    .cal-day{aspect-ratio:1;background:rgba(255,255,255,0.02);border:1px solid rgba(255,255,255,0.05);border-radius:10px;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:1px;cursor:pointer;transition:all 0.2s;position:relative;min-height:52px}
    .cal-day:hover{background:rgba(0,217,255,0.1);border-color:rgba(0,217,255,0.3)}
    .cal-day.empty{background:transparent;border-color:transparent;cursor:default}
    .cal-day.empty:hover{background:transparent}
    .cal-day.today{border-color:var(--accent);background:rgba(0,217,255,0.08);box-shadow:0 0 12px rgba(0,217,255,0.2)}
    .cal-day.selected{border-color:var(--accent);background:rgba(0,217,255,0.15)}
    .cal-day .day-num{font-size:1rem;font-weight:600;color:#ccc}
    .cal-day.today .day-num{color:var(--accent)}
    .cal-day.other-month .day-num{color:#333}
    .cal-day .badge{min-width:20px;height:20px;border-radius:10px;font-size:0.7rem;font-weight:700;display:flex;align-items:center;justify-content:center;padding:0 5px;margin-top:2px}
    .cal-day .badge.local{background:var(--green);color:#fff}
    .cal-day .badge.google{background:#4285F4;color:#fff}
    .cal-day .badge.mixed{background:linear-gradient(135deg,var(--green),#4285F4);color:#fff}

    /* Day Detail Panel */
    .day-panel{background:var(--bg-card);border:1px solid rgba(255,255,255,0.05);border-radius:15px;padding:16px;margin-bottom:20px;display:none}
    .day-panel.active{display:block}
    .day-panel-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px}
    .day-panel-title{font-size:1.1rem;font-weight:700}
    .day-panel-close{background:none;border:none;color:#64748b;font-size:1.5rem;cursor:pointer;padding:4px 8px;transition:color 0.3s}
    .day-panel-close:hover{color:#fff}

    .event-list{max-height:300px;overflow-y:auto}
    .event-item{display:flex;align-items:center;gap:10px;padding:10px;background:rgba(255,255,255,0.03);border-radius:8px;margin-bottom:6px;transition:background 0.2s}
    .event-item:hover{background:rgba(255,255,255,0.06)}
    .event-dot{width:8px;height:8px;border-radius:50%;flex-shrink:0}
    .event-dot.local{background:var(--green)}
    .event-dot.google{background:#4285F4}
    .event-dot.synced{background:linear-gradient(135deg,var(--green),#4285F4)}
    .event-time{font-size:0.85rem;color:var(--accent);white-space:nowrap;min-width:90px}
    .event-title{font-size:0.9rem;flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
    .event-actions{display:flex;gap:4px}
    .event-actions button{background:none;border:none;color:#64748b;cursor:pointer;font-size:0.85rem;padding:4px 6px;border-radius:4px;transition:all 0.2s}
    .event-actions button:hover{color:#fff;background:rgba(255,255,255,0.1)}
    .event-actions button.del:hover{color:var(--red);background:rgba(233,69,96,0.15)}
    .no-events{text-align:center;color:#475569;padding:20px;font-size:0.9rem}

    /* Add Event Form */
    .add-form{background:rgba(255,255,255,0.03);border:1px solid rgba(255,255,255,0.08);border-radius:10px;padding:14px;margin-top:12px}
    .add-form h4{font-size:0.85rem;color:var(--accent);margin-bottom:10px;font-weight:600;letter-spacing:1px;text-transform:uppercase}
    .form-row{display:flex;gap:8px;margin-bottom:8px;flex-wrap:wrap}
    .form-row input{flex:1;min-width:100px;background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);border-radius:8px;padding:8px 12px;color:#fff;font-size:0.85rem;outline:none;transition:border-color 0.3s}
    .form-row input:focus{border-color:var(--accent)}
    .form-row input::placeholder{color:#475569}
    .btn{padding:8px 18px;border:none;border-radius:8px;font-size:0.85rem;font-weight:600;cursor:pointer;transition:all 0.3s;letter-spacing:0.5px}
    .btn-primary{background:linear-gradient(135deg,var(--accent),#0088cc);color:#fff}
    .btn-primary:hover{box-shadow:0 0 15px rgba(0,217,255,0.4);transform:translateY(-1px)}
    .btn-secondary{background:rgba(255,255,255,0.08);color:#ccc}
    .btn-secondary:hover{background:rgba(255,255,255,0.15)}
    .btn-danger{background:rgba(233,69,96,0.15);color:var(--red)}
    .btn-danger:hover{background:rgba(233,69,96,0.3)}
    .btn-sync{background:linear-gradient(135deg,#4285F4,#34A853);color:#fff}
    .btn-sync:hover{box-shadow:0 0 15px rgba(66,133,244,0.4);transform:translateY(-1px)}

    /* Sync Bar */
    .sync-bar{display:flex;align-items:center;justify-content:space-between;gap:10px;background:var(--bg-card);border:1px solid rgba(255,255,255,0.05);border-radius:12px;padding:10px 16px;margin-bottom:15px;flex-wrap:wrap}
    .sync-info{font-size:0.8rem;color:#64748b}
    .sync-info span{color:#ccc}
    .sync-actions{display:flex;gap:8px}

    /* Toast */
    .toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%) translateY(100px);background:rgba(20,20,40,0.95);border:1px solid rgba(0,217,255,0.3);color:#fff;padding:12px 24px;border-radius:10px;font-size:0.85rem;z-index:1000;transition:transform 0.3s ease;backdrop-filter:blur(10px)}
    .toast.show{transform:translateX(-50%) translateY(0)}
    .toast.error{border-color:rgba(233,69,96,0.5)}

    /* Edit Modal */
    .modal-overlay{display:none;position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,0.7);z-index:100;align-items:center;justify-content:center;backdrop-filter:blur(5px)}
    .modal-overlay.active{display:flex}
    .modal{background:linear-gradient(145deg,#1a1a2e,#0f0f1a);border:1px solid rgba(255,255,255,0.1);border-radius:15px;padding:24px;width:90%;max-width:400px}
    .modal h3{font-size:1rem;margin-bottom:16px;color:var(--accent)}
    .modal .form-row{margin-bottom:10px}
    .modal .form-row label{display:block;font-size:0.75rem;color:#64748b;margin-bottom:4px;text-transform:uppercase;letter-spacing:1px}
    .modal .form-row input{width:100%}
    .modal-actions{display:flex;gap:8px;justify-content:flex-end;margin-top:16px}

    .footer{text-align:center;padding:20px;color:#475569;font-size:0.8rem;border-top:1px solid rgba(255,255,255,0.05);margin-top:20px}
    .footer a{color:var(--accent);text-decoration:none}
  </style>
</head>
<body>
  <div class="bg-animation"></div>
  <div class="container">
    <div class="header">
      <a href="/" class="back-link">&larr; Home</a>
      <div class="logo">CALENDARIO</div>
    </div>

    <div class="sync-bar">
      <div class="sync-info">
        Locale: <span id="localCount">0</span> &bull;
        Google: <span id="googleCount">0</span> &bull;
        Ultimo sync: <span id="lastSync">--</span>
      </div>
      <div class="sync-actions">
        <button class="btn btn-sync" onclick="syncGoogle()" id="syncBtn">&#x21bb; Sync Google</button>
      </div>
    </div>

    <div class="month-nav">
      <button onclick="changeMonth(-1)" title="Mese precedente">&lsaquo;</button>
      <div class="month-title" id="monthTitle">--</div>
      <button onclick="changeMonth(1)" title="Mese successivo">&rsaquo;</button>
      <button class="today-btn" onclick="goToday()">OGGI</button>
    </div>

    <div class="cal-grid">
      <div class="cal-weekdays">
        <div class="cal-weekday">Lun</div>
        <div class="cal-weekday">Mar</div>
        <div class="cal-weekday">Mer</div>
        <div class="cal-weekday">Gio</div>
        <div class="cal-weekday">Ven</div>
        <div class="cal-weekday">Sab</div>
        <div class="cal-weekday">Dom</div>
      </div>
      <div class="cal-days" id="calDays"></div>
    </div>

    <div class="day-panel" id="dayPanel">
      <div class="day-panel-header">
        <div class="day-panel-title" id="dayPanelTitle">--</div>
        <button class="day-panel-close" onclick="closeDayPanel()">&times;</button>
      </div>
      <div class="event-list" id="eventList"></div>
      <div class="add-form">
        <h4>Nuovo Evento</h4>
        <div class="form-row">
          <input type="text" id="newTitle" placeholder="Titolo evento" maxlength="40">
        </div>
        <div class="form-row">
          <input type="time" id="newStart" value="09:00">
          <input type="time" id="newEnd" value="10:00">
        </div>
        <div class="form-row" style="justify-content:flex-end">
          <button class="btn btn-primary" onclick="addEvent()">+ Aggiungi</button>
        </div>
      </div>
    </div>

    <div class="footer">
      <p>OraQuadra Nano v1.5 &bull; <a href="/">Home</a> &bull; <a href="/settings">Settings</a></p>
    </div>
  </div>

  <!-- Edit Modal -->
  <div class="modal-overlay" id="editModal">
    <div class="modal">
      <h3>Modifica Evento</h3>
      <input type="hidden" id="editId">
      <div class="form-row">
        <label>Titolo</label>
        <input type="text" id="editTitle" maxlength="40">
      </div>
      <div class="form-row">
        <label>Data (GG/MM/AAAA)</label>
        <input type="text" id="editDate" placeholder="DD/MM/YYYY">
      </div>
      <div class="form-row">
        <label>Ora Inizio</label>
        <input type="time" id="editStart">
      </div>
      <div class="form-row">
        <label>Ora Fine</label>
        <input type="time" id="editEnd">
      </div>
      <div class="modal-actions">
        <button class="btn btn-secondary" onclick="closeEditModal()">Annulla</button>
        <button class="btn btn-primary" onclick="saveEdit()">Salva</button>
      </div>
    </div>
  </div>

  <div class="toast" id="toast"></div>

  <script>
    const MONTHS = ['Gennaio','Febbraio','Marzo','Aprile','Maggio','Giugno','Luglio','Agosto','Settembre','Ottobre','Novembre','Dicembre'];
    const DAYS_SHORT = ['Dom','Lun','Mar','Mer','Gio','Ven','Sab'];

    let viewYear, viewMonth; // 0-indexed month
    let selectedDay = 0;
    let eventsCache = [];
    let autoRefreshTimer = null;
    let lastDataHash = '';

    function init() {
      const now = new Date();
      viewYear = now.getFullYear();
      viewMonth = now.getMonth();
      renderCalendar();
      startAutoRefresh();
    }

    // Auto-refresh ogni 10 secondi per sync rapida
    function startAutoRefresh() {
      if (autoRefreshTimer) clearInterval(autoRefreshTimer);
      autoRefreshTimer = setInterval(async () => {
        // Non aggiornare se il modal edit e' aperto
        if (document.getElementById('editModal').classList.contains('active')) return;
        await renderCalendar();
        if (selectedDay) openDayPanel(selectedDay);
      }, 10000);
    }

    // Poll fino a sync completata (max 15s)
    async function waitForSync(maxWait) {
      maxWait = maxWait || 15000;
      const start = Date.now();
      while (Date.now() - start < maxWait) {
        await new Promise(function(r) { setTimeout(r, 1500); });
        try {
          const r = await fetch('/calendar/syncstatus?_=' + Date.now(), {cache:'no-store'});
          const d = await r.json();
          if (!d.syncing) {
            await renderCalendar();
            if (selectedDay) openDayPanel(selectedDay);
            return true;
          }
        } catch(e) {}
      }
      // Timeout - refresh comunque
      await renderCalendar();
      if (selectedDay) openDayPanel(selectedDay);
      return false;
    }

    function changeMonth(delta) {
      viewMonth += delta;
      if (viewMonth < 0) { viewMonth = 11; viewYear--; }
      if (viewMonth > 11) { viewMonth = 0; viewYear++; }
      selectedDay = 0;
      closeDayPanel();
      renderCalendar();
    }

    function goToday() {
      const now = new Date();
      viewYear = now.getFullYear();
      viewMonth = now.getMonth();
      selectedDay = now.getDate();
      renderCalendar();
      openDayPanel(selectedDay);
    }

    async function renderCalendar() {
      document.getElementById('monthTitle').textContent = MONTHS[viewMonth] + ' ' + viewYear;

      // Fetch events for this month
      const mm = String(viewMonth + 1).padStart(2, '0');
      try {
        const r = await fetch('/calendar/list?month=' + mm + '&year=' + viewYear + '&_=' + Date.now(), {cache:'no-store'});
        const data = await r.json();
        eventsCache = data.events || [];
        document.getElementById('localCount').textContent = data.localCount || 0;
        document.getElementById('googleCount').textContent = data.googleCount || 0;
        if (data.lastSync) document.getElementById('lastSync').textContent = data.lastSync;
      } catch(e) {
        eventsCache = [];
      }

      // Build grid
      const container = document.getElementById('calDays');
      container.innerHTML = '';

      const firstDay = new Date(viewYear, viewMonth, 1);
      let startDow = firstDay.getDay(); // 0=Sun
      startDow = (startDow === 0) ? 6 : startDow - 1; // Convert to Mon=0

      const daysInMonth = new Date(viewYear, viewMonth + 1, 0).getDate();
      const today = new Date();
      const isCurrentMonth = (viewYear === today.getFullYear() && viewMonth === today.getMonth());

      // Empty cells before first day
      for (let i = 0; i < startDow; i++) {
        const cell = document.createElement('div');
        cell.className = 'cal-day empty';
        container.appendChild(cell);
      }

      // Day cells
      for (let d = 1; d <= daysInMonth; d++) {
        const cell = document.createElement('div');
        let cls = 'cal-day';
        if (isCurrentMonth && d === today.getDate()) cls += ' today';
        if (d === selectedDay) cls += ' selected';
        cell.className = cls;

        const numEl = document.createElement('div');
        numEl.className = 'day-num';
        numEl.textContent = d;
        cell.appendChild(numEl);

        // Count events for this day
        const dayStr = String(d).padStart(2, '0') + '/' + String(viewMonth + 1).padStart(2, '0') + '/' + viewYear;
        const dayEvents = eventsCache.filter(e => e.date === dayStr);
        if (dayEvents.length > 0) {
          const badge = document.createElement('div');
          badge.textContent = dayEvents.length;
          const hasLocal = dayEvents.some(e => e.source === 0 || e.source === 2);
          const hasGoogle = dayEvents.some(e => e.source === 1);
          badge.className = 'badge ' + (hasLocal && hasGoogle ? 'mixed' : hasLocal ? 'local' : 'google');
          cell.appendChild(badge);
        }

        const day = d;
        cell.onclick = () => { selectedDay = day; highlightSelected(); openDayPanel(day); };
        container.appendChild(cell);
      }

      // Fill remaining cells
      const totalCells = startDow + daysInMonth;
      const remaining = (totalCells % 7 === 0) ? 0 : 7 - (totalCells % 7);
      for (let i = 0; i < remaining; i++) {
        const cell = document.createElement('div');
        cell.className = 'cal-day empty';
        container.appendChild(cell);
      }
    }

    function highlightSelected() {
      document.querySelectorAll('.cal-day').forEach(c => c.classList.remove('selected'));
      const days = document.querySelectorAll('.cal-day:not(.empty)');
      days.forEach(c => {
        const num = c.querySelector('.day-num');
        if (num && parseInt(num.textContent) === selectedDay) c.classList.add('selected');
      });
    }

    function openDayPanel(day) {
      selectedDay = day;
      const panel = document.getElementById('dayPanel');
      panel.classList.add('active');

      const dayStr = String(day).padStart(2, '0') + '/' + String(viewMonth + 1).padStart(2, '0') + '/' + viewYear;
      const dow = new Date(viewYear, viewMonth, day).getDay();
      document.getElementById('dayPanelTitle').textContent = DAYS_SHORT[dow] + ' ' + dayStr;

      const dayEvents = eventsCache.filter(e => e.date === dayStr);
      const list = document.getElementById('eventList');

      if (dayEvents.length === 0) {
        list.innerHTML = '<div class="no-events">Nessun evento per questo giorno</div>';
      } else {
        dayEvents.sort((a, b) => (a.start || '').localeCompare(b.start || ''));
        list.innerHTML = dayEvents.map(e => {
          const dotCls = e.source === 1 ? 'google' : e.source === 2 ? 'synced' : 'local';
          const timeStr = (e.start || '--:--') + (e.end ? ' - ' + e.end : '');
          const isLocal = e.source === 0 || e.source === 2;
          const actions = isLocal ?
            `<div class="event-actions">
              <button onclick="editEvent(${e.id})" title="Modifica">&#9998;</button>
              <button class="del" onclick="deleteEvent(${e.id})" title="Elimina">&#10005;</button>
              ${e.source === 0 ? '<button onclick="pushEvent(' + e.id + ')" title="Push a Google">&#9729;</button>' : ''}
            </div>` : '';
          return `<div class="event-item">
            <div class="event-dot ${dotCls}"></div>
            <div class="event-time">${timeStr}</div>
            <div class="event-title">${escHtml(e.title)}</div>
            ${actions}
          </div>`;
        }).join('');
      }
    }

    function closeDayPanel() {
      document.getElementById('dayPanel').classList.remove('active');
    }

    function escHtml(s) {
      const d = document.createElement('div');
      d.textContent = s;
      return d.innerHTML;
    }

    function toDash(s) { return s.replace(/[\/\:]/g, '-'); }

    async function addEvent() {
      const title = document.getElementById('newTitle').value.trim();
      const start = document.getElementById('newStart').value;
      const end = document.getElementById('newEnd').value;
      if (!title) { showToast('Inserisci un titolo', true); return; }
      if (!selectedDay) return;

      const dateStr = String(selectedDay).padStart(2, '0') + '-' + String(viewMonth + 1).padStart(2, '0') + '-' + viewYear;
      try {
        const url = '/calendar/add?title=' + encodeURIComponent(title) + '&date=' + dateStr + '&start=' + toDash(start) + '&end=' + toDash(end) + '&_=' + Date.now();
        const r = await fetch(url, {cache:'no-store'});
        const txt = await r.text();
        let d;
        try { d = JSON.parse(txt); } catch(pe) { showToast('Errore risposta server', true); return; }
        if (d.ok) {
          showToast('Evento aggiunto - sincronizzazione Google...');
          document.getElementById('newTitle').value = '';
          await renderCalendar();
          openDayPanel(selectedDay);
          // Attendi sync con Google (push + refresh)
          await waitForSync(15000);
          showToast('Evento sincronizzato!');
        } else {
          showToast(d.error || 'Errore', true);
        }
      } catch(e) { showToast('Errore rete: ' + e.message, true); }
    }

    async function deleteEvent(id) {
      if (!confirm('Eliminare questo evento?')) return;
      try {
        const r = await fetch('/calendar/delete?id=' + id + '&_=' + Date.now(), {cache:'no-store'});
        const txt = await r.text();
        let d;
        try { d = JSON.parse(txt); } catch(pe) { showToast('Errore risposta server', true); return; }
        if (d.ok) {
          showToast('Evento eliminato - aggiornamento...');
          await renderCalendar();
          openDayPanel(selectedDay);
          await waitForSync(10000);
          showToast('Calendario aggiornato');
        } else {
          showToast(d.error || 'Errore', true);
        }
      } catch(e) { showToast('Errore rete: ' + e.message, true); }
    }

    function editEvent(id) {
      const ev = eventsCache.find(e => e.id === id);
      if (!ev) return;
      document.getElementById('editId').value = id;
      document.getElementById('editTitle').value = ev.title;
      document.getElementById('editDate').value = ev.date;
      document.getElementById('editStart').value = ev.start || '';
      document.getElementById('editEnd').value = ev.end || '';
      document.getElementById('editModal').classList.add('active');
    }

    function closeEditModal() {
      document.getElementById('editModal').classList.remove('active');
    }

    async function saveEdit() {
      const id = document.getElementById('editId').value;
      const title = document.getElementById('editTitle').value.trim();
      const date = document.getElementById('editDate').value.trim();
      const start = document.getElementById('editStart').value;
      const end = document.getElementById('editEnd').value;
      if (!title || !date) { showToast('Compila tutti i campi', true); return; }

      try {
        const url = '/calendar/edit?id=' + id + '&title=' + encodeURIComponent(title) + '&date=' + toDash(date) + '&start=' + toDash(start) + '&end=' + toDash(end) + '&_=' + Date.now();
        const r = await fetch(url, {cache:'no-store'});
        const txt = await r.text();
        let d;
        try { d = JSON.parse(txt); } catch(pe) { showToast('Errore risposta server', true); return; }
        if (d.ok) {
          showToast('Evento modificato - aggiornamento...');
          closeEditModal();
          await renderCalendar();
          openDayPanel(selectedDay);
          await waitForSync(10000);
          showToast('Calendario aggiornato');
        } else {
          showToast(d.error || 'Errore', true);
        }
      } catch(e) { showToast('Errore rete: ' + e.message, true); }
    }

    async function pushEvent(id) {
      showToast('Invio a Google Calendar...');
      try {
        const r = await fetch('/calendar/push?id=' + id + '&_=' + Date.now(), {cache:'no-store'});
        const txt = await r.text();
        let d;
        try { d = JSON.parse(txt); } catch(pe) { showToast('Errore risposta server', true); return; }
        if (d.ok) {
          showToast('Push in corso...');
          const ok = await waitForSync(15000);
          showToast(ok ? 'Evento inviato a Google!' : 'Evento inviato (verifica sync)');
        } else {
          showToast(d.error || 'Errore push', true);
        }
      } catch(e) { showToast('Errore rete: ' + e.message, true); }
    }

    async function syncGoogle() {
      const btn = document.getElementById('syncBtn');
      btn.disabled = true;
      btn.textContent = 'Sincronizzazione...';
      try {
        const r = await fetch('/calendar/sync?_=' + Date.now(), {cache:'no-store'});
        const d = await r.json();
        if (d.ok) {
          showToast('Sync in corso...');
          const ok = await waitForSync(15000);
          showToast(ok ? 'Calendario aggiornato!' : 'Sync completata');
        } else {
          showToast(d.error || 'Errore sync', true);
        }
      } catch(e) { showToast('Errore rete: ' + e.message, true); }
      btn.disabled = false;
      btn.innerHTML = '&#x21bb; Sync Google';
    }

    function showToast(msg, isError) {
      const t = document.getElementById('toast');
      t.textContent = msg;
      t.className = 'toast show' + (isError ? ' error' : '');
      setTimeout(() => { t.className = 'toast'; }, 3000);
    }

    init();
  </script>
</body>
</html>
)rawliteral";
