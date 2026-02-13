// ================== NEWS FEED HTML ==================
// Pagina web per visualizzare notizie da RSS feeds (ANSA, BBC, Repubblica)
// Accesso: http://<IP_ESP32>:8080/news
// Nessuna API key necessaria

const char NEWS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>News - OraQuadra Nano</title>
  <style>
    *{margin:0;padding:0;box-sizing:border-box}
    :root{--nw-bg:#0f0f0f;--nw-card:#1a1a2e;--nw-accent:#00b4ff;--nw-text:#fff;--nw-dim:#aaa;--nw-border:rgba(255,255,255,0.06)}
    body{font-family:'Segoe UI',system-ui,sans-serif;background:var(--nw-bg);color:var(--nw-text);min-height:100vh}
    .container{max-width:600px;margin:0 auto;padding:20px}
    .header{text-align:center;padding:20px 0}
    .header .logo{display:inline-block;background:var(--nw-accent);border-radius:12px;padding:12px 20px;margin-bottom:15px}
    .header .logo span{font-size:2rem}
    .header h1{font-size:1.6rem;margin-bottom:4px}
    .header .sub{color:var(--nw-dim);font-size:0.85rem}
    .source-bar{display:flex;gap:8px;justify-content:center;margin-bottom:14px}
    .source-pill{background:var(--nw-card);border:2px solid var(--nw-border);border-radius:20px;padding:8px 18px;color:var(--nw-dim);font-size:0.85rem;font-weight:600;cursor:pointer;transition:all 0.2s}
    .source-pill:hover{border-color:var(--nw-accent);color:var(--nw-text)}
    .source-pill.active{background:var(--nw-accent);color:#fff;border-color:var(--nw-accent)}
    .source-pill .flag{font-size:0.7rem;opacity:0.7;margin-left:4px}
    .tabs{display:flex;flex-wrap:wrap;gap:6px;justify-content:center;margin-bottom:20px}
    .tab{background:var(--nw-card);border:1px solid var(--nw-border);border-radius:20px;padding:8px 14px;color:var(--nw-dim);font-size:0.8rem;cursor:pointer;transition:all 0.2s}
    .tab:hover{border-color:var(--nw-accent);color:var(--nw-text)}
    .tab.active{background:var(--nw-accent);color:#fff;border-color:var(--nw-accent)}
    .articles{display:flex;flex-direction:column;gap:12px;margin-bottom:20px}
    .article{background:var(--nw-card);border-radius:12px;padding:16px;border:1px solid var(--nw-border);transition:transform 0.2s}
    .article:hover{transform:translateY(-2px)}
    .article .source{color:var(--nw-accent);font-size:0.75rem;font-weight:600;text-transform:uppercase;letter-spacing:0.5px;margin-bottom:6px}
    .article .title{font-size:1rem;font-weight:700;line-height:1.4;margin-bottom:8px}
    .article .desc{color:var(--nw-dim);font-size:0.85rem;line-height:1.5;margin-bottom:8px}
    .article .article-footer{display:flex;justify-content:space-between;align-items:center}
    .article .time{color:#666;font-size:0.75rem}
    .article .read-more{color:var(--nw-accent);font-size:0.75rem;font-weight:600}
    .article-link{text-decoration:none;color:inherit}
    .status-bar{background:var(--nw-card);border-radius:12px;padding:12px 16px;display:flex;justify-content:space-between;align-items:center;font-size:0.8rem;color:var(--nw-dim);margin-bottom:20px}
    .error-msg{background:#331111;border:1px solid #662222;border-radius:12px;padding:16px;text-align:center;color:#ff6b6b;margin-bottom:20px}
    .loading{text-align:center;padding:40px;color:var(--nw-dim)}
    .loading .spinner{display:inline-block;width:24px;height:24px;border:3px solid var(--nw-card);border-top-color:var(--nw-accent);border-radius:50%;animation:spin 0.8s linear infinite}
    @keyframes spin{to{transform:rotate(360deg)}}
    .nav-back{display:inline-block;color:var(--nw-accent);text-decoration:none;margin-bottom:15px;font-size:0.85rem}
    .nav-back:hover{text-decoration:underline}
    .rss-badge{display:inline-block;background:#f60;color:#fff;font-size:0.65rem;font-weight:700;padding:2px 6px;border-radius:4px;margin-left:6px;vertical-align:middle}
  </style>
</head>
<body>
  <div class="container">
    <a href="/" class="nav-back">&larr; Home</a>
    <div class="header">
      <div class="logo"><span>&#128240;</span></div>
      <h1 id="pageTitle">News ANSA</h1>
      <p class="sub">Feed RSS gratuiti <span class="rss-badge">RSS</span> &mdash; Nessuna API key necessaria</p>
    </div>

    <div class="source-bar" id="sourceBar"></div>

    <div class="tabs" id="catTabs"></div>

    <div class="status-bar">
      <span id="statusText">Caricamento...</span>
      <span id="lastUpdate">--</span>
    </div>

    <div id="errorBox" class="error-msg" style="display:none"></div>
    <div id="articlesBox" class="articles">
      <div class="loading"><div class="spinner"></div><p style="margin-top:10px">Caricamento notizie...</p></div>
    </div>

    <div style="text-align:center;padding:15px 0;color:#555;font-size:0.75rem">
      OraQuadra Nano v1.5 &bull; Feed RSS: ANSA, BBC, Repubblica
    </div>
  </div>

  <script>
    let currentSrc = 0;
    let currentCat = 0;
    let sources = [];
    let cats = [];
    let refreshTimer = null;

    function buildSourceBar(srcList, activeSrc) {
      const bar = document.getElementById('sourceBar');
      bar.innerHTML = '';
      srcList.forEach((s, i) => {
        const pill = document.createElement('div');
        pill.className = 'source-pill' + (i === activeSrc ? ' active' : '');
        pill.innerHTML = s.name + '<span class="flag">' + s.flag + '</span>';
        pill.addEventListener('click', () => {
          currentSrc = i;
          currentCat = 0;
          fetchNews();
        });
        bar.appendChild(pill);
      });
    }

    function buildCatTabs(catList, activeCat) {
      const tabsEl = document.getElementById('catTabs');
      tabsEl.innerHTML = '';
      catList.forEach((name, i) => {
        const tab = document.createElement('div');
        tab.className = 'tab' + (i === activeCat ? ' active' : '');
        tab.textContent = name;
        tab.addEventListener('click', () => {
          currentCat = i;
          fetchNews();
        });
        tabsEl.appendChild(tab);
      });
    }

    function timeAgo(dateStr) {
      if (!dateStr || dateStr === '--') return '';
      try {
        const d = new Date(dateStr);
        const now = new Date();
        const diff = Math.floor((now - d) / 1000);
        if (diff < 60) return 'ora';
        if (diff < 3600) return Math.floor(diff/60) + ' min fa';
        if (diff < 86400) return Math.floor(diff/3600) + ' ore fa';
        return Math.floor(diff/86400) + ' giorni fa';
      } catch(e) { return ''; }
    }

    async function fetchNews() {
      try {
        document.getElementById('articlesBox').innerHTML = '<div class="loading"><div class="spinner"></div><p style="margin-top:10px">Caricamento...</p></div>';

        const r = await fetch('/news/status?src=' + currentSrc + '&cat=' + currentCat);
        const d = await r.json();
        const box = document.getElementById('articlesBox');
        const errBox = document.getElementById('errorBox');

        // Sincronizza stato
        currentSrc = d.source;
        currentCat = d.catIndex;
        sources = d.sources || [];
        cats = d.cats || [];

        // Aggiorna UI
        document.getElementById('pageTitle').textContent = 'News ' + d.sourceName;
        document.getElementById('lastUpdate').textContent = d.lastUpdate ? ('Agg: ' + d.lastUpdate) : '--';

        buildSourceBar(sources, currentSrc);
        buildCatTabs(cats, currentCat);

        if (d.error && d.error.length > 0) {
          errBox.textContent = d.error;
          errBox.style.display = 'block';
        } else {
          errBox.style.display = 'none';
        }

        if (d.articles && d.articles.length > 0) {
          document.getElementById('statusText').textContent =
            d.articles.length + ' notizie - ' + d.sourceName + ' / ' + d.catName;
          let html = '';
          d.articles.forEach(a => {
            if (a.link) {
              html += '<a href="' + esc(a.link) + '" target="_blank" class="article-link">';
            }
            html += '<div class="article">';
            html += '<div class="source">' + esc(a.source || '') + '</div>';
            html += '<div class="title">' + esc(a.title || '') + '</div>';
            if (a.description) html += '<div class="desc">' + esc(a.description) + '</div>';
            html += '<div class="article-footer">';
            if (a.publishedAt) html += '<span class="time">' + timeAgo(a.publishedAt) + '</span>';
            if (a.link) html += '<span class="read-more">Leggi tutto &rarr;</span>';
            html += '</div>';
            html += '</div>';
            if (a.link) html += '</a>';
          });
          box.innerHTML = html;
        } else if (!d.error) {
          box.innerHTML = '<div class="loading"><p>Nessuna notizia disponibile</p></div>';
          document.getElementById('statusText').textContent = 'Nessuna notizia';
        } else {
          box.innerHTML = '';
        }
      } catch(e) {
        document.getElementById('statusText').textContent = 'Errore connessione';
        console.error(e);
      }
    }

    function esc(s) {
      const d = document.createElement('div');
      d.textContent = s;
      return d.innerHTML;
    }

    fetchNews();
    refreshTimer = setInterval(fetchNews, 120000);
  </script>
</body>
</html>
)rawliteral";
