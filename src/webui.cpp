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
/* Right 1/6 of the viewport (16.7vw) is kept empty as a safe thumb-scroll gutter. */
body{margin:0;padding:14px 16.7vw 24px 12px;max-width:640px;background:#0f1115;color:#e6edf3;font:16px/1.45 system-ui,-apple-system,sans-serif;border-right:1px dashed #21262e}
h1{margin:0 0 12px;font-size:1.15rem;font-weight:600}
.card{background:#171b21;border:1px solid #262c36;border-radius:12px;padding:12px 14px;margin-bottom:10px}
.seg{display:flex;gap:8px}
/* The channel buttons are tabs: the thick ring marks which one you are editing,
   the fill marks whether that channel is powered on (green) or off (red). */
.seg button{flex:1;padding:11px 8px;font-size:.95rem;border-radius:9px;border:2px solid #3a424d;background:#21262e;color:#8b949e;font-weight:600}
.seg button:disabled{opacity:.3;cursor:not-allowed}
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
.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:10px}
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
/* Three-disk fingertip drum set */
.drums{display:grid;grid-template-columns:repeat(3,1fr);gap:12px;margin:6px 0 10px;padding:4px 2px}
.drum{position:relative;aspect-ratio:1;width:100%;border-radius:50%;border:3px solid var(--c,#58a6ff);background:radial-gradient(circle at 38% 35%,rgba(255,255,255,.16),rgba(15,17,21,.92) 70%);color:#f0f6fc;font-weight:700;font-size:1.15rem;display:flex;flex-direction:column;align-items:center;justify-content:center;cursor:pointer;user-select:none;-webkit-user-select:none;-webkit-tap-highlight-color:transparent;touch-action:manipulation;box-shadow:0 0 14px -4px var(--c,#58a6ff),inset 0 0 18px rgba(0,0,0,.75);transition:transform 60ms ease,box-shadow 90ms ease,background 90ms ease}
.drum small{font-size:.66rem;font-weight:500;color:#8b949e;margin-top:2px;letter-spacing:.03em;text-transform:uppercase}
.drum.hit{transform:scale(.92);background:radial-gradient(circle at 50% 50%,#fff 0%,var(--c,#58a6ff) 55%,#171b21 100%);box-shadow:0 0 28px 4px var(--c,#58a6ff),inset 0 0 12px #fff;color:#0f1115}
.drum.hit small{color:#0f1115}
.drum.off{opacity:.4;border-style:dashed}
</style>
</head>
<body>
<h1>Sound Light</h1>
<div class=card>
<div class=row><span>Microphone gain</span><b id=gainDbv>-</b></div>
<input type=range id=gainDb min=0 max=18 step=6>
<div class=row><span>Beat sensitivity</span><b id=beatSensitivityPctv>-</b></div>
<input type=range id=beatSensitivityPct min=0 max=100 step=5>
<div class=row><span>Detected beat</span><b id=bpmv>--</b></div>
<div class=bar><i id=beatbar style="background:#d29922"></i></div>
<div class=stats>
<div><span>Level</span><b id=sLvl>0%</b></div>
<div><span>dBFS</span><b id=sDb>-</b></div>
<div><span>Peak</span><b id=sPk>-</b></div>
</div>
</div>
<div class=card>
<div class=row style="margin-top:0"><span>Drum pads (tap with fingertip)</span><b id=drumhint style="color:#8b949e;font-weight:400;font-size:.76rem">triggers pulse + echo</b></div>
<div class=drums id=drums></div>
<div class=row><span>Edit channel</span><b></b></div>
<div class=seg id=pick></div>
<div id=meters style="margin-top:8px"></div>
</div>
<div class=card id=params></div>
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
{k:'beatBoostPct',g:'beat',n:'Beat flash strength',min:0,max:100,st:5,u:'%'},
{k:'centerHz',g:'reactive',n:'Center frequency',min:65,max:7600,st:5,u:'Hz'},
{k:'widthHz',g:'reactive',n:'Band width',min:40,max:16000,st:10,u:'Hz'},
{k:'attackMs',g:'reactive',n:'Attack (0 = instant)',min:0,max:1000,st:5,u:'ms'},
{k:'releaseMs',g:'env',n:'Release / decay',min:0,max:2000,st:5,u:'ms'},
{k:'gatePct',g:'reactive',n:'Noise gate',min:0,max:50,st:1,u:'%'},
{k:'echoCount',g:'env',n:'Echo repeats (0 = off)',min:0,max:10,st:1,u:''},
{k:'echoSpacingMs',g:'env',n:'Echo spacing',min:20,max:500,st:5,u:'ms'},
{k:'echoFeedbackPct',g:'env',n:'Echo decay each repeat',min:0,max:90,st:5,u:'%'},
{k:'glowPeriodMs',g:'glow',n:'Period',min:100,max:5000,st:10,u:'ms'},
{k:'glowMinPct',g:'glow',n:'Dim level',min:0,max:100,st:1,u:'%'},
{k:'glowMaxPct',g:'glow',n:'Bright level',min:0,max:100,st:1,u:'%'}
];
var sel=0, st=null, seeded=false, built=false;
var NAMES=['R','G','B'], COLS=['#f85149','#3fb950','#58a6ff'], MODES=['reactive','glow','manual'];
var holdTimers={};
function byId(id){return document.getElementById(id);}
function nameOf(i){return NAMES[i]||('L'+(i+1));}
function colOf(i){return COLS[i]||'#8b949e';}
function modeName(m){return MODES[m]||'reactive';}
function padDown(i){
var d=byId('drm'+i);
if(d)d.classList.add('hit');
fetch('/api/tap?led='+i+'&down=1',{method:'POST'}).catch(function(){});
clearInterval(holdTimers[i]);
holdTimers[i]=setInterval(function(){
  fetch('/api/tap?led='+i+'&down=1',{method:'POST'}).catch(function(){});
},1000);
}
function padUp(i){
clearInterval(holdTimers[i]);
holdTimers[i]=0;
var d=byId('drm'+i);
if(d&&d.classList.contains('hit')){
  d.classList.remove('hit');
  fetch('/api/tap?led='+i+'&down=0',{method:'POST'}).catch(function(){});
}
}
function buildLights(n){
var p='', m='', d='';
for(var i=0;i<n;i++){
p+='<button id=sel'+i+'>'+nameOf(i)+'</button>';
d+='<button type=button class=drum id=drm'+i+' style="--c:'+colOf(i)+'"><span>'+nameOf(i)+'</span><small id=dsub'+i+'>tap</small></button>';
m+='<div class=row><span>'+nameOf(i)+' out</span><b id=p'+i+'>0%</b></div>'
 +'<div class=bar><i id=b'+i+' style="background:'+colOf(i)+'"></i></div>';
}
byId('pick').innerHTML=p;
byId('drums').innerHTML=d;
byId('meters').innerHTML=m;
for(var j=0;j<n;j++){
byId('sel'+j).onclick=(function(k){return function(){sel=k;fill();live();};})(j);
(function(k){
  var pad=byId('drm'+k);
  pad.addEventListener('pointerdown',function(e){
    e.preventDefault();
    if(pad.setPointerCapture){try{pad.setPointerCapture(e.pointerId);}catch(_){}}
    padDown(k);
  });
  pad.addEventListener('pointerup',function(){padUp(k);});
  pad.addEventListener('pointercancel',function(){padUp(k);});
  pad.addEventListener('lostpointercapture',function(){padUp(k);});
  pad.addEventListener('contextmenu',function(e){e.preventDefault();});
})(j);
}
}
function build(){
var box=byId('params');
box.innerHTML='<div class=ptitle id=ptitle></div>'
+'<div class=row><span>Power</span><label class=sw><input type=checkbox id=pwr><i></i></label></div>'
+'<div class=row><span>Mode</span><b></b></div>'
+'<div class=seg><button id=bm0>Reactive</button><button id=bm1>Glow</button><button id=bm2>Manual</button></div>'
+'<div class=row><span>Beat effect</span><b id=bpmv2>--</b></div>'
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
function setMode(m){
if(m!==1&&st&&st.leds[sel].beatMode===2){
  fetch('/api/params?led='+sel+'&mode='+m+'&beatMode=0',{method:'POST'}).then(function(r){return r.json();}).then(function(s){st=s;fill();live();});
}else{
  send(sel,'mode',m);
}
}
byId('bm0').onclick=function(){setMode(0);};
byId('bm1').onclick=function(){setMode(1);};
byId('bm2').onclick=function(){setMode(2);};
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
function updateBeatHint(){
if(!st)return;
var L=st.leds[sel];
var b=(L.beatMode||0);
var hint=byId('beathint');
if(!hint)return;
if(b===0){
  hint.textContent='Beat detector ignored on this channel.';
}else if(b===1){
  hint.textContent=L.mode===1
    ?'Each beat adds a flash on top of the breathing glow.'
    :(L.mode===2
      ?'Each beat adds an automatic flash alongside your manual drum taps.'
      :'Each beat adds a flash on top of the sound-reactive level.');
}else{
  if(L.mode!==1){
    hint.textContent='Sync glow only applies in Glow mode (switch Mode to Glow, or choose Flash).';
  }else if(st.bpm>0){
    hint.textContent='Glow period locked to '+Math.round(60000/st.bpm)+' ms ('+st.bpm+' BPM), peaking on each beat.';
  }else{
    hint.textContent='Waiting for a steady beat — using manual Period ('+L.glowPeriodMs+' ms) until locked.';
  }
}
}
function fill(){
if(!st)return;
var L=st.leds[sel];
SPEC.forEach(function(s){byId(s.k).value=L[s.k];});
byId('pwr').checked=!!L.enabled;
// Same convention as the channel tabs: the ring marks the choice.
byId('bm0').className=L.mode===0?'sel':'';
byId('bm1').className=L.mode===1?'sel':'';
byId('bm2').className=L.mode===2?'sel':'';
var b=(L.beatMode||0);
byId('bt0').className=b===0?'sel':'';
byId('bt1').className=b===1?'sel':'';
byId('bt2').className=b===2?'sel':'';
// Sync glow only makes sense when the channel is in Glow mode.
byId('bt2').disabled=(L.mode!==1);
// Flash strength applies whenever Flash (b===1) is chosen.
var i,boost=document.querySelectorAll('.r-beatBoostPct');
for(i=0;i<boost.length;i++)boost[i].style.display=b===1?'':'none';
// The panel header names the channel being edited, so the tab ring is never the
// only clue.
byId('ptitle').style.borderLeftColor=colOf(sel);
byId('ptitle').innerHTML='Channel '+nameOf(sel)+'<small>'
+(L.enabled?'powered on':'powered off')+' &middot; '
+modeName(L.mode)
+(L.mode===0?(' &middot; '+L.centerHz+' Hz'):'')+'</small>';
var g=document.querySelectorAll('.g-reactive'),
    e=document.querySelectorAll('.g-env'),
    h=document.querySelectorAll('.g-glow');
for(i=0;i<g.length;i++)g[i].style.display=L.mode===0?'':'none';
for(i=0;i<e.length;i++)e[i].style.display=(L.mode===0||L.mode===2)?'':'none';
for(i=0;i<h.length;i++)h[i].style.display=L.mode===1?'':'none';
labels();
updateBeatHint();
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
var d=byId('drm'+i), dsub=byId('dsub'+i);
if(d){
  d.classList.toggle('off',!on);
}
if(dsub){
  dsub.textContent=on?modeName(st.leds[i].mode):'off';
}
}
byId('sLvl').textContent=st.levelPct+'%';
byId('sDb').textContent=st.dbfs;
byId('sPk').textContent=st.peakHz+' Hz';
var bpmTxt=st.bpm>0?(st.bpm+' BPM'):'--';
byId('bpmv').textContent=bpmTxt;
if(byId('bpmv2'))byId('bpmv2').textContent=bpmTxt;
byId('beatbar').style.width=st.beatPct+'%';
byId('lp1').disabled=!st.hasPreset0;
byId('lp2').disabled=!st.hasPreset1;
updateBeatHint();
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
  server_.on("/api/tap", HTTP_POST, [this] { handleTap(); });
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

void WebUi::handleTap() {
  const int index = server_.hasArg("led") ? server_.arg("led").toInt() : -1;
  if (index >= 0 && index < (int)kLedCount) {
    if (server_.hasArg("down")) {
      const int down = server_.arg("down").toInt();
      pendingHold_[index] = down ? 1 : 0;
    } else {
      pendingTap_[index] = true;
    }
  }
  server_.send(204, "text/plain", "");
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
