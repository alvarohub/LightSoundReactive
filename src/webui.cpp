#include "webui.h"

#include <WiFi.h>
#include <stdarg.h>
#include <stdio.h>

namespace {

const char kPage[] PROGMEM = R"PAGE(<!doctype html>
<html lang=en>
<head>
<meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>Sound Light</title>
<style>
:root{color-scheme:dark}
*{box-sizing:border-box}
body{margin:0 auto;padding:14px;max-width:520px;background:#0f1115;color:#e6edf3;font:16px/1.45 system-ui,-apple-system,sans-serif}
h1{margin:0 0 12px;font-size:1.15rem;font-weight:600}
.card{background:#171b21;border:1px solid #262c36;border-radius:12px;padding:12px 14px;margin-bottom:10px}
.seg{display:flex;gap:8px}
/* The channel buttons are tabs: the thick ring marks which one you are editing,
   the fill marks whether that channel is powered on (green) or off (red). */
.seg button{flex:1;padding:11px 8px;font-size:.95rem;border-radius:9px;border:2px solid #3a424d;background:#21262e;color:#8b949e;font-weight:600}
.seg button.on{background:#17301e;border-color:#2ea043;color:#7ee787}
.seg button.off{background:#2b1d1d;border-color:#6b2b2b;color:#ff9d95}
.seg button.sel{outline:3px solid #58a6ff;outline-offset:2px}
.row{display:flex;justify-content:space-between;align-items:baseline;margin:8px 0 2px;font-size:.9rem;color:#9da7b3}
.row b{color:#7ee787;font-variant-numeric:tabular-nums}
.hint{color:#8b949e;font-size:.76rem;margin:2px 0 4px}
input[type=range]{-webkit-appearance:none;appearance:none;width:100%;height:26px;background:transparent;margin:0}
input[type=range]::-webkit-slider-runnable-track{height:6px;border-radius:3px;background:#2b323c}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:22px;height:22px;margin-top:-8px;border-radius:50%;background:#3fb950}
input[type=range]::-moz-range-track{height:6px;border-radius:3px;background:#2b323c}
input[type=range]::-moz-range-thumb{width:22px;height:22px;border:0;border-radius:50%;background:#3fb950}
.bar{height:9px;border-radius:5px;background:#2b323c;overflow:hidden;margin-top:6px}
.bar i{display:block;height:100%;width:0;background:linear-gradient(90deg,#3fb950,#d29922,#f85149)}
.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:12px}
.stats div{background:#21262e;border-radius:9px;padding:8px 4px;text-align:center}
.stats span{display:block;font-size:.68rem;letter-spacing:.04em;color:#8b949e;text-transform:uppercase}
.stats b{font-size:.95rem;font-variant-numeric:tabular-nums}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.btn{padding:11px;border-radius:9px;border:1px solid #2b323c;background:#21262e;color:#e6edf3;font-size:.95rem}
.btn:disabled{opacity:.35}
.btn.alt{background:#16261a;border-color:#2ea043}
.btn.wide{width:100%;margin-top:8px}
.btn.warn{background:#2a1a1a;border-color:#6b2b2b;color:#ff7b72}
#pmsg{color:#8b949e;font-size:.78rem;font-weight:400}
.sw{position:relative;display:inline-block;width:52px;height:28px;flex:none}
.sw input{opacity:0;width:0;height:0;position:absolute}
.sw i{position:absolute;inset:0;background:#2b323c;border-radius:14px;transition:background .15s;cursor:pointer}
.sw i:before{content:"";position:absolute;width:22px;height:22px;left:3px;top:3px;border-radius:50%;background:#8b949e;transition:transform .15s,background .15s}
.sw input:checked + i{background:#238636}
.sw input:checked + i:before{transform:translateX(24px);background:#fff}
.ptitle{margin:0 0 10px;padding-left:9px;border-left:4px solid #58a6ff;font-weight:600;color:#f0f6fc}
.ptitle small{display:block;font-weight:400;font-size:.76rem;color:#8b949e;margin-top:2px}
</style>
</head>
<body>
<h1>Sound Light</h1>
<div class=card><div class=seg id=pick></div></div>
<div class=card id=params></div>
<div class=card>
<div class=row><span>Microphone gain</span><b id=gainDbv>-</b></div>
<input type=range id=gainDb min=0 max=18 step=6>
<div class=row><span>Beat sensitivity</span><b id=beatSensitivityPctv>-</b></div>
<input type=range id=beatSensitivityPct min=0 max=100 step=5>
</div>
<div class=card>
<div id=meters></div>
<div class=row><span>Beat</span><b id=bpmv>-</b></div>
<div class=bar><i id=beatbar style="background:#d29922"></i></div>
<div class=stats>
<div><span>Level</span><b id=sLvl>0%</b></div>
<div><span>dBFS</span><b id=sDb>-</b></div>
<div><span>Peak</span><b id=sPk>-</b></div>
</div>
</div>
<div class=card>
<div class=row><span>Presets</span><b id=pmsg></b></div>
<div class=grid>
<button id=sp1 class=btn>Save 1</button>
<button id=lp1 class="btn alt">Load 1</button>
<button id=sp2 class=btn>Save 2</button>
<button id=lp2 class="btn alt">Load 2</button>
</div>
<button id=rst class="btn warn wide">Factory reset</button>
</div>
<script>
var SPEC=[
{k:'centerHz',g:'reactive',n:'Center frequency',min:65,max:7600,st:5,u:'Hz'},
{k:'widthHz',g:'reactive',n:'Band width',min:40,max:16000,st:10,u:'Hz'},
{k:'attackMs',g:'reactive',n:'Attack (0 = instant)',min:0,max:1000,st:5,u:'ms'},
{k:'releaseMs',g:'reactive',n:'Release / decay',min:0,max:2000,st:5,u:'ms'},
{k:'gatePct',g:'reactive',n:'Noise gate',min:0,max:50,st:1,u:'%'},
{k:'echoCount',g:'reactive',n:'Echo repeats (0 = off)',min:0,max:10,st:1,u:''},
{k:'echoSpacingMs',g:'reactive',n:'Echo spacing',min:20,max:500,st:5,u:'ms'},
{k:'echoFeedbackPct',g:'reactive',n:'Echo decay each repeat',min:0,max:90,st:5,u:'%'},
{k:'beatBoostPct',g:'reactive',n:'Beat flash strength',min:0,max:100,st:5,u:'%'},
{k:'glowPeriodMs',g:'glow',n:'Period',min:100,max:5000,st:10,u:'ms'},
{k:'glowMinPct',g:'glow',n:'Dim level',min:0,max:100,st:1,u:'%'},
{k:'glowMaxPct',g:'glow',n:'Bright level',min:0,max:100,st:1,u:'%'}
];
var sel=0, st=null, seeded=false, built=false;
var NAMES=['R','G','B'], COLS=['#f85149','#3fb950','#58a6ff'];
function byId(id){return document.getElementById(id);}
function nameOf(i){return NAMES[i]||('L'+(i+1));}
function colOf(i){return COLS[i]||'#8b949e';}
function buildLights(n){
var p='', m='';
for(var i=0;i<n;i++){
p+='<button id=sel'+i+'>'+nameOf(i)+'</button>';
m+='<div class=row><span>'+nameOf(i)+' out</span><b id=p'+i+'>0%</b></div>'
 +'<div class=bar><i id=b'+i+' style="background:'+colOf(i)+'"></i></div>';
}
byId('pick').innerHTML=p;
byId('meters').innerHTML=m;
for(var j=0;j<n;j++)byId('sel'+j).onclick=(function(k){return function(){sel=k;fill();live();};})(j);
}
function build(){
var box=byId('params');
box.innerHTML='<div class=ptitle id=ptitle></div>'
+'<div class=row><span>Power</span><label class=sw><input type=checkbox id=pwr><i></i></label></div>'
+'<div class=row><span>Mode</span><b></b></div>'
+'<div class=seg><button id=bm0>Reactive</button><button id=bm1>Glow</button></div>'
+'<div class=row><span>Beat detection</span><b></b></div>'
+'<div class=seg><button id=bt0>Off</button><button id=bt1>Flash</button><button id=bt2>Sync glow</button></div>'
+'<div class="hint" id=beathint></div>';
SPEC.forEach(function(s){
box.insertAdjacentHTML('beforeend','<div class="row g-'+s.g+' r-'+s.k+'"><span>'+s.n+'</span><b id="'+s.k+'v">-</b></div><input class="g-'+s.g+' r-'+s.k+'" type=range id="'+s.k+'" min='+s.min+' max='+s.max+' step='+s.st+'>');
if(s.k==='widthHz')box.insertAdjacentHTML('beforeend','<div class="hint g-reactive" id=win></div>');
var i=byId(s.k);
i.addEventListener('input',labels);
i.addEventListener('change',function(){send(sel,s.k,i.value);});
});
byId('pwr').addEventListener('change',function(){send(sel,'enabled',byId('pwr').checked?1:0);});
byId('bm0').onclick=function(){send(sel,'mode',0);};
byId('bm1').onclick=function(){send(sel,'mode',1);};
byId('bt0').onclick=function(){send(sel,'beatMode',0);};
byId('bt1').onclick=function(){send(sel,'beatMode',1);};
byId('bt2').onclick=function(){send(sel,'beatMode',2);};
}
function labels(){
SPEC.forEach(function(s){byId(s.k+'v').textContent=byId(s.k).value+s.u;});
var c=+byId('centerHz').value, w=+byId('widthHz').value;
var lo=Math.max(65,Math.round(c-w/2)), hi=Math.min(7600,Math.round(c+w/2));
byId('win').textContent='listens to '+lo+' - '+hi+' Hz';
}
function fill(){
if(!st)return;
var L=st.leds[sel];
SPEC.forEach(function(s){byId(s.k).value=L[s.k];});
byId('pwr').checked=!!L.enabled;
// Same convention as the channel tabs: the ring marks the choice.
byId('bm0').className=L.mode?'':'sel';
byId('bm1').className=L.mode?'sel':'';
var b=(L.beatMode||0);
byId('bt0').className=b===0?'sel':'';
byId('bt1').className=b===1?'sel':'';
byId('bt2').className=b===2?'sel':'';
// The boost only matters in flash mode; sync glow only in glow mode.
var boost=document.querySelectorAll('.r-beatBoostPct');
for(i=0;i<boost.length;i++)boost[i].style.display=b===1?'':'none';
var hint=byId('beathint');
if(hint)hint.textContent=b===0?'off':(b===1
  ?'each beat adds a flash on top of the level'
  :'glow period follows the tempo and peaks on the beat');
// The panel header names the channel being edited, so the tab ring is never the
// only clue.
byId('ptitle').style.borderLeftColor=colOf(sel);
byId('ptitle').innerHTML='Channel '+nameOf(sel)+'<small>'
+(L.enabled?'powered on':'powered off')+' &middot; '
+(L.mode?'glow':'reactive')+' &middot; '+L.centerHz+' Hz</small>';
var i,g=document.querySelectorAll('.g-reactive'),h=document.querySelectorAll('.g-glow');
for(i=0;i<g.length;i++)g[i].style.display=L.mode?'none':'';
for(i=0;i<h.length;i++)h[i].style.display=L.mode?'':'none';
labels();
}
function fillGain(){
if(!st)return;
byId('gainDb').value=st.gainDb;
byId('gainDbv').textContent=st.gainDb+'dB';
byId('beatSensitivityPct').value=st.beatSensitivityPct;
byId('beatSensitivityPctv').textContent=st.beatSensitivityPct+'%';
}
function live(){
if(!st)return;
for(var i=0;i<st.leds.length;i++){
var on=!!st.leds[i].enabled;
byId('b'+i).style.width=st.leds[i].brightPct+'%';
byId('p'+i).textContent=on?st.leds[i].brightPct+'%':'off';
// Ring = which tab you are editing; fill = powered on or off.
byId('sel'+i).textContent=nameOf(i);
byId('sel'+i).className=(sel===i?'sel ':'')+(on?'on':'off');
byId('sel'+i).title=nameOf(i)+(on?' (on)':' (off)');
}
byId('sLvl').textContent=st.levelPct+'%';
byId('sDb').textContent=st.dbfs;
byId('sPk').textContent=st.peakHz+' Hz';
byId('bpmv').textContent=st.bpm>0?(st.bpm+' BPM'):'--';
byId('beatbar').style.width=st.beatPct+'%';
byId('lp1').disabled=!st.hasPreset0;
byId('lp2').disabled=!st.hasPreset1;
}
function apply(s){
st=s;
if(!built){built=true;buildLights(s.leds.length);}
if(!seeded){seeded=true;fill();fillGain();}
live();
}
function send(i,k,v){
fetch('/api/params?led='+i+'&'+k+'='+v,{method:'POST'}).then(function(r){return r.json();}).then(function(s){st=s;fill();live();});
}
function sendGain(v){
fetch('/api/params?gainDb='+v,{method:'POST'}).then(function(r){return r.json();}).then(function(s){st=s;fillGain();live();});
}
function sendSens(v){
fetch('/api/params?beatSensitivityPct='+v,{method:'POST'}).then(function(r){return r.json();}).then(function(s){st=s;fillGain();live();});
}
function post(u){
return fetch(u,{method:'POST'}).then(function(r){return r.json();}).then(function(s){st=s;fill();fillGain();live();return s;});
}
var msgTimer=0;
function msg(t){
byId('pmsg').textContent=t;
clearTimeout(msgTimer);
msgTimer=setTimeout(function(){byId('pmsg').textContent='';},2500);
}
byId('gainDb').addEventListener('input',function(){byId('gainDbv').textContent=byId('gainDb').value+'dB';});
byId('gainDb').addEventListener('change',function(){sendGain(byId('gainDb').value);});
byId('beatSensitivityPct').addEventListener('input',function(){byId('beatSensitivityPctv').textContent=byId('beatSensitivityPct').value+'%';});
byId('beatSensitivityPct').addEventListener('change',function(){sendSens(byId('beatSensitivityPct').value);});
byId('sp1').onclick=function(){post('/api/preset?slot=1&action=save').then(function(){msg('Saved to 1');});};
byId('sp2').onclick=function(){post('/api/preset?slot=2&action=save').then(function(){msg('Saved to 2');});};
byId('lp1').onclick=function(){post('/api/preset?slot=1&action=load').then(function(){msg('Loaded 1');});};
byId('lp2').onclick=function(){post('/api/preset?slot=2&action=load').then(function(){msg('Loaded 2');});};
byId('rst').onclick=function(){
if(confirm('Restore the default parameters?')){post('/api/reset').then(function(){msg('Defaults restored');});}
};
function poll(){
fetch('/api/state',{cache:'no-store'}).then(function(r){return r.json();}).then(apply).catch(function(){});
}
build();fillGain();poll();setInterval(poll,400);
</script>
</body>
</html>)PAGE";

void appendf(char *buf, size_t cap, size_t &n, const char *fmt, ...) {
  if (n >= cap - 1) {
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  const int written = vsnprintf(buf + n, cap - n, fmt, ap);
  va_end(ap);
  if (written > 0) {
    n += (size_t)written;
    if (n > cap - 1) {
      n = cap - 1;
    }
  }
}

} // namespace

void WebUi::setup(Settings *settings, SettingsStore *store) {
  settings_ = settings;
  store_ = store;
}

bool WebUi::begin() { return setEnabled(true); }

bool WebUi::setEnabled(bool on) {
  if (on == enabled_) {
    return true;
  }

  if (!on) {
    server_.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    enabled_ = false;
    Serial.println("[web] access point off");
    return true;
  }

  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(kSsid, kPassword)) {
    Serial.println("[web] softAP failed");
    return false;
  }

  bindRoutes();
  server_.begin();
  enabled_ = true;
  Serial.printf("[web] \"%s\" / \"%s\" -> http://%s\n", kSsid, kPassword, ip().c_str());
  return true;
}

void WebUi::bindRoutes() {
  if (routesBound_) {
    return;
  }
  routesBound_ = true;

  server_.on("/", [this] { handleRoot(); });
  server_.on("/api/state", [this] { handleState(); });
  server_.on("/api/params", HTTP_POST, [this] { handleParams(); });
  server_.on("/api/preset", HTTP_POST, [this] { handlePreset(); });
  server_.on("/api/reset", HTTP_POST, [this] { handleReset(); });
  server_.onNotFound([this] {
    if (server_.uri() == "/favicon.ico") {
      server_.send(204, "text/plain", "");
    } else {
      server_.send(404, "text/plain", "not found");
    }
  });
}

String WebUi::ip() const { return enabled_ ? WiFi.softAPIP().toString() : String("0.0.0.0"); }

String WebUi::stateJson() const {
  // Sized for kLedCount lights with maximum-width values; a short buffer would
  // silently truncate the JSON and break the page.
  char buf[kStateJsonMaxBytes];
  size_t n = 0;
  buf[0] = '\0';

  appendf(buf, sizeof(buf), n, "{\"gainDb\":%u,\"leds\":[", (unsigned)settings_->gainDb);

  for (size_t i = 0; i < kLedCount; i++) {
    const LedSettings &l = settings_->led[i];
    appendf(buf, sizeof(buf), n,
            "%s{\"mode\":%u,\"enabled\":%u,\"centerHz\":%u,\"widthHz\":%u,"
            "\"attackMs\":%u,\"releaseMs\":%u,\"gatePct\":%u,\"glowPeriodMs\":%u,"
            "\"glowMinPct\":%u,\"glowMaxPct\":%u,\"echoCount\":%u,"
            "\"echoSpacingMs\":%u,\"echoFeedbackPct\":%u,\"beatMode\":%u,"
            "\"beatBoostPct\":%u,\"brightPct\":%u,\"duty\":%u}",
            i == 0 ? "" : ",", (unsigned)l.mode, (unsigned)l.enabled,
            (unsigned)l.centerHz, (unsigned)l.widthHz, (unsigned)l.attackMs,
            (unsigned)l.releaseMs, (unsigned)l.gatePct, (unsigned)l.glowPeriodMs,
            (unsigned)l.glowMinPct, (unsigned)l.glowMaxPct, (unsigned)l.echoCount,
            (unsigned)l.echoSpacingMs, (unsigned)l.echoFeedbackPct,
            (unsigned)l.beatMode, (unsigned)l.beatBoostPct,
            (unsigned)tele_.brightPct[i], (unsigned)tele_.duty[i]);
  }

  appendf(buf, sizeof(buf), n,
          "],\"gainDb\":%u,\"beatSensitivityPct\":%u,\"levelPct\":%u,\"dbfs\":%d,"
          "\"peakHz\":%u,\"bpm\":%u,\"beatPct\":%u,\"hasPreset0\":%u,"
          "\"hasPreset1\":%u}",
          (unsigned)settings_->gainDb, (unsigned)settings_->beatSensitivityPct,
          (unsigned)tele_.levelPct, (int)tele_.dbfs, (unsigned)tele_.peakHz,
          (unsigned)tele_.bpm, (unsigned)tele_.beatPct,
          store_ && store_->hasPreset(0) ? 1u : 0u,
          store_ && store_->hasPreset(1) ? 1u : 0u);

  return String(buf);
}

void WebUi::handleRoot() {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send_P(200, "text/html", kPage);
}

void WebUi::handleState() {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json", stateJson());
}

void WebUi::applyArg(const char *name, uint8_t *dst) {
  if (!server_.hasArg(name)) {
    return;
  }
  *dst = (uint8_t)constrain(server_.arg(name).toInt(), 0L, 255L);
}

void WebUi::applyArg(const char *name, uint16_t *dst) {
  if (!server_.hasArg(name)) {
    return;
  }
  *dst = (uint16_t)constrain(server_.arg(name).toInt(), 0L, 65535L);
}

void WebUi::handleParams() {
  // Light parameters are addressed with ?led=<index>; gain is global.
  const int index = server_.hasArg("led") ? server_.arg("led").toInt() : -1;
  if (index >= 0 && index < (int)kLedCount) {
    LedSettings &l = settings_->led[index];
    applyArg("mode", &l.mode);
    applyArg("enabled", &l.enabled);
    applyArg("centerHz", &l.centerHz);
    applyArg("widthHz", &l.widthHz);
    applyArg("attackMs", &l.attackMs);
    applyArg("releaseMs", &l.releaseMs);
    applyArg("gatePct", &l.gatePct);
    applyArg("echoCount", &l.echoCount);
    applyArg("echoSpacingMs", &l.echoSpacingMs);
    applyArg("echoFeedbackPct", &l.echoFeedbackPct);
    applyArg("glowPeriodMs", &l.glowPeriodMs);
    applyArg("glowMinPct", &l.glowMinPct);
    applyArg("glowMaxPct", &l.glowMaxPct);
    applyArg("beatMode", &l.beatMode);
    applyArg("beatBoostPct", &l.beatBoostPct);
  }
  applyArg("gainDb", &settings_->gainDb);
  applyArg("beatSensitivityPct", &settings_->beatSensitivityPct);
  clampSettings(*settings_);
  changed_ = true;

  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json", stateJson());
}

void WebUi::handlePreset() {
  const int slot = server_.hasArg("slot") ? server_.arg("slot").toInt() : 0;
  const String action = server_.hasArg("action") ? server_.arg("action") : String();

  // The API is 1-based so the numbers match the buttons on the page.
  const uint8_t index = (slot >= 1 && slot <= SettingsStore::kPresetCount)
                            ? (uint8_t)(slot - 1)
                            : 0xFF;

  bool ok = false;
  if (index != 0xFF && store_ != nullptr) {
    if (action == "save") {
      ok = store_->savePreset(index, *settings_);
    } else if (action == "load") {
      Settings loaded;
      if (store_->loadPreset(index, loaded)) {
        *settings_ = loaded;
        changed_ = true; // the live settings were replaced, so persist them
        ok = true;
      }
    }
  }

  Serial.printf("[web] preset %d %s -> %s\n", slot, action.c_str(), ok ? "ok" : "failed");

  server_.sendHeader("Cache-Control", "no-store");
  server_.send(ok ? 200 : 400, "application/json", stateJson());
}

void WebUi::handleReset() {
  // Presets are deliberately left alone: a reset of the running parameters
  // should not silently destroy the two saved slots.
  *settings_ = defaultSettings();
  changed_ = true;

  Serial.println("[web] factory reset");

  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json", stateJson());
}
