// ================== SCROLLTEXT WEB PAGE ==================
// Pagina HTML PROGMEM per configurazione testo scorrevole
// Auto-save immediato + attivazione automatica del modo

#ifndef SCROLLTEXT_WEB_HTML_H
#define SCROLLTEXT_WEB_HTML_H

const char SCROLLTEXT_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Testo Scorrevole - OraQuadra</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#0a0a0a;color:#e0e0e0;min-height:100vh}
.container{max-width:600px;margin:0 auto;padding:16px}
.header{text-align:center;padding:16px 0 8px}
.header .icon{font-size:40px}
.header h1{font-size:20px;color:#7CFC00;margin:6px 0 0}
.back{display:inline-block;color:#7CFC00;text-decoration:none;padding:6px 0;font-size:13px;opacity:0.8}
.back:hover{opacity:1}

.toast{position:fixed;top:16px;left:50%;transform:translateX(-50%);background:#7CFC00;color:#000;padding:8px 20px;border-radius:8px;font-size:13px;font-weight:600;z-index:100;opacity:0;transition:opacity .3s;pointer-events:none}
.toast.show{opacity:1}
.toast.err{background:#ff3333;color:#fff}

.msg-card{background:#141414;border-radius:10px;padding:12px;margin:8px 0;border:1px solid #222}
.msg-card.disabled{opacity:0.4}
.msg-top{display:flex;align-items:center;gap:8px;margin-bottom:8px}
.msg-num{font-size:13px;font-weight:bold;color:#7CFC00;min-width:44px}
.msg-top .toggle{position:relative;width:36px;height:20px;cursor:pointer;flex-shrink:0}
.msg-top .toggle input{opacity:0;width:0;height:0}
.msg-top .toggle .sl{position:absolute;top:0;left:0;right:0;bottom:0;background:#333;border-radius:10px;transition:.3s}
.msg-top .toggle input:checked+.sl{background:#7CFC00}
.msg-top .toggle .sl:before{content:'';position:absolute;width:16px;height:16px;left:2px;bottom:2px;background:#fff;border-radius:50%;transition:.3s}
.msg-top .toggle input:checked+.sl:before{transform:translateX(16px)}
.btn-x{background:#ff3333;color:#fff;border:none;border-radius:5px;width:26px;height:26px;font-size:13px;cursor:pointer;flex-shrink:0}
.btn-x:hover{background:#cc0000}

textarea{width:100%;background:#111;border:1px solid #333;border-radius:6px;padding:8px;color:#fff;font-size:15px;resize:none;height:44px;font-family:inherit}
textarea:focus{outline:none;border-color:#7CFC00}

.opts{display:flex;gap:6px;margin-top:8px;flex-wrap:wrap;align-items:center}
select{background:#111;border:1px solid #333;border-radius:5px;padding:4px 6px;color:#fff;font-size:12px;max-width:140px}
select:focus{outline:none;border-color:#7CFC00}
input[type=color]{width:34px;height:28px;border:none;border-radius:5px;cursor:pointer;background:transparent;padding:0}
input[type=range]{width:70px;accent-color:#7CFC00}
.spd-v{font-size:11px;color:#7CFC00;min-width:16px;text-align:center}

.dir-row{display:flex;gap:3px;align-items:center}
.db{width:28px;height:28px;border-radius:5px;border:1px solid #333;background:#111;color:#888;font-size:13px;cursor:pointer;display:flex;align-items:center;justify-content:center;padding:0}
.db.on{background:#7CFC00;color:#000;border-color:#7CFC00}

.fx-row{display:flex;gap:10px;margin-top:6px}
.fx{display:flex;align-items:center;gap:4px;cursor:pointer;font-size:12px;color:#aaa}
.fx input{accent-color:#7CFC00;width:14px;height:14px}

.glob{background:#141414;border-radius:10px;padding:10px 12px;margin:8px 0;border:1px solid #222;display:flex;gap:10px;align-items:center;flex-wrap:wrap}
.glob label{font-size:12px;color:#888}
.glob input[type=number]{width:55px;background:#111;border:1px solid #333;border-radius:5px;padding:4px 6px;color:#fff;font-size:13px}
.glob input[type=color]{width:34px;height:28px;border:none;border-radius:5px;cursor:pointer;background:transparent}

.gtog{display:flex;align-items:center;gap:6px;font-size:12px;color:#888}
.gtog .toggle{position:relative;width:36px;height:20px;cursor:pointer;flex-shrink:0}
.gtog .toggle input{opacity:0;width:0;height:0}
.gtog .toggle .sl{position:absolute;top:0;left:0;right:0;bottom:0;background:#333;border-radius:10px;transition:.3s}
.gtog .toggle input:checked+.sl{background:#7CFC00}
.gtog .toggle .sl:before{content:'';position:absolute;width:16px;height:16px;left:2px;bottom:2px;background:#fff;border-radius:50%;transition:.3s}
.gtog .toggle input:checked+.sl:before{transform:translateX(16px)}
.btn-add{display:block;width:100%;padding:10px;background:#1a1a1a;border:2px dashed #333;border-radius:10px;color:#7CFC00;font-size:14px;cursor:pointer;text-align:center;margin:8px 0}
.btn-add:hover{border-color:#7CFC00;background:#111}
</style>
</head>
<body>
<div class="container">
  <a href="/" class="back">&larr; Home</a>
  <div class="header">
    <div class="icon">&#128172;</div>
    <h1>Testo Scorrevole</h1>
  </div>

  <div class="glob">
    <label>Rot.</label><input type="number" id="gRot" min="3" max="300" value="10" onchange="autoSave()">
    <label>sec</label>
    <label style="margin-left:auto">Sfondo</label><input type="color" id="gBg" value="#000000" onchange="autoSave()">
  </div>
  <div class="glob">
    <span class="gtog"><label class="toggle"><input type="checkbox" id="gTime" checked onchange="autoSave()"><span class="sl"></span></label>Ora</span>
    <span class="gtog"><label class="toggle"><input type="checkbox" id="gDate" checked onchange="autoSave()"><span class="sl"></span></label>Data</span>
  </div>

  <div id="msgList"></div>
  <button class="btn-add" id="btnAdd" onclick="addMsg()">+ Messaggio</button>
</div>
<div class="toast" id="toast"></div>

<script>
const FONTS=[
'\u25A0 Pixel (scalabile!)','Sans Bold 14','Sans Bold 18','Sans Bold 24',
'Sans 18','Sans 24','Serif 18','Serif 24',
'Mono 18','Mono 24','Fat 20','Fat 30',
'Display 22','Display 32','Retro 22',
'Fat 35','Display 38','Display 42','Display 54'
];
const DA=['\u2190','\u2192'];
let msgs=[];
let saveTimer=null;
let modeActivated=false;

function toast(t,err){
  const e=document.getElementById('toast');
  e.textContent=t;e.className='toast'+(err?' err':'')+' show';
  setTimeout(()=>e.className='toast',1500);
}

function h2r(h){const m=h.match(/^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i);return m?{r:+('0x'+m[1]),g:+('0x'+m[2]),b:+('0x'+m[3])}:{r:255,g:255,b:255}}
function r2h(r,g,b){return '#'+[r,g,b].map(x=>x.toString(16).padStart(2,'0')).join('')}
function esc(s){return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;')}

function render(){
  const L=document.getElementById('msgList');L.innerHTML='';
  msgs.forEach((m,i)=>{
    const c=document.createElement('div');
    c.className='msg-card'+(m.on?'':' disabled');
    c.innerHTML=`
<div class="msg-top">
  <span class="msg-num">#${i+1}</span>
  <label class="toggle"><input type="checkbox" ${m.on?'checked':''} onchange="tog(${i},this.checked)"><span class="sl"></span></label>
  <div style="flex:1"></div>
  <button class="btn-x" onclick="del(${i})">&#10005;</button>
</div>
<textarea maxlength="200" oninput="msgs[${i}].text=this.value;autoSave()">${esc(m.text)}</textarea>
<div class="opts">
  <select onchange="msgs[${i}].font=+this.value;autoSave();render()">${FONTS.map((f,fi)=>`<option value="${fi}"${fi===m.font?' selected':''}>${f}</option>`).join('')}</select>
  <span class="spd-v" style="color:#aaa">x</span><input type="range" min="1" max="20" value="${m.sc}" oninput="msgs[${i}].sc=+this.value;this.nextElementSibling.textContent=this.value+'x';autoSave()"><span class="spd-v">${m.sc}x</span>
  <input type="color" value="${r2h(m.r,m.g,m.b)}" onchange="let c=h2r(this.value);msgs[${i}].r=c.r;msgs[${i}].g=c.g;msgs[${i}].b=c.b;autoSave()">
</div>
<div class="opts">
  <span style="font-size:11px;color:#666">Vel</span><input type="range" min="1" max="10" value="${m.speed}" oninput="msgs[${i}].speed=+this.value;this.nextElementSibling.textContent=this.value;autoSave()"><span class="spd-v">${m.speed}</span>
  <div class="dir-row">${DA.map((a,d)=>`<button class="db${m.dir===d?' on':''}" onclick="msgs[${i}].dir=${d};autoSave();render()">${a}</button>`).join('')}</div>
</div>
<div class="fx-row">
  <label class="fx"><input type="checkbox" ${m.fx&1?'checked':''} onchange="msgs[${i}].fx=(msgs[${i}].fx&~1)|(this.checked?1:0);autoSave()">Rainbow</label>
  <label class="fx"><input type="checkbox" ${m.fx&2?'checked':''} onchange="msgs[${i}].fx=(msgs[${i}].fx&~2)|(this.checked?2:0);autoSave()">Fade</label>
</div>`;
    L.appendChild(c);
  });
  document.getElementById('btnAdd').style.display=msgs.length>=10?'none':'block';
}

function tog(i,v){msgs[i].on=v;render();autoSave()}
function del(i){msgs.splice(i,1);render();autoSave()}
function addMsg(){
  if(msgs.length>=10)return;
  msgs.push({text:'',font:0,sc:6,r:0,g:255,b:0,speed:5,dir:0,fx:0,on:true});
  render();
  // Focus sulla textarea appena creata
  setTimeout(()=>{const ta=document.querySelectorAll('textarea');if(ta.length)ta[ta.length-1].focus()},50);
}

function autoSave(){
  if(saveTimer)clearTimeout(saveTimer);
  saveTimer=setTimeout(doSave,600);
}

async function doSave(){
  const rot=parseInt(document.getElementById('gRot').value)||10;
  const bg=h2r(document.getElementById('gBg').value);
  const sTime=document.getElementById('gTime').checked?1:0;
  const sDate=document.getElementById('gDate').checked?1:0;
  try{
    await fetch(`/scrolltext/global?count=${msgs.length}&rotation=${rot*1000}&bgR=${bg.r}&bgG=${bg.g}&bgB=${bg.b}&showTime=${sTime}&showDate=${sDate}`);
    for(let i=0;i<msgs.length;i++){
      const m=msgs[i];
      await fetch('/scrolltext/set?idx='+i+'&text='+encodeURIComponent(m.text)+'&font='+m.font+'&sc='+(m.sc||1)+'&r='+m.r+'&g='+m.g+'&b='+m.b+'&speed='+m.speed+'&dir='+m.dir+'&fx='+m.fx+'&on='+(m.on?1:0));
    }
    // Attiva modo automaticamente alla prima modifica
    if(!modeActivated){
      await fetch('/settings/setmode?mode=32');
      modeActivated=true;
    }
    toast('Salvato');
  }catch(e){toast('Errore!',true)}
}

async function load(){
  try{
    const r=await fetch('/scrolltext/status');const d=await r.json();
    document.getElementById('gRot').value=Math.round((d.rotation||10000)/1000);
    document.getElementById('gBg').value=r2h(d.bgR||0,d.bgG||0,d.bgB||0);
    document.getElementById('gTime').checked=d.showTime!==undefined?!!d.showTime:true;
    document.getElementById('gDate').checked=d.showDate!==undefined?!!d.showDate:true;
    msgs=[];
    if(d.msgs)d.msgs.forEach(m=>msgs.push({text:m.text||'',font:m.font||0,sc:m.sc||6,r:m.r||255,g:m.g||255,b:m.b||255,speed:m.speed||5,dir:m.dir||0,fx:m.fx||0,on:m.on?true:false}));
    if(!msgs.length)msgs.push({text:'',font:0,sc:6,r:0,g:255,b:0,speed:5,dir:0,fx:0,on:true});
    render();
  }catch(e){
    msgs=[{text:'',font:0,sc:6,r:0,g:255,b:0,speed:5,dir:0,fx:0,on:true}];
    render();
  }
  // Controlla se gia in mode scrolltext
  try{const r=await fetch('/settings/status');const d=await r.json();if(d.mode===29)modeActivated=true;}catch(e){}
}
load();
</script>
</body>
</html>
)rawliteral";

#endif // SCROLLTEXT_WEB_HTML_H
