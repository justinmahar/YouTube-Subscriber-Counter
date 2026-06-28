// Library Imports
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Update.h>
#include <math.h>

// File imports
#include "FontSubs.h"

// Hardware config
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
//#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW
#define MAX_DEVICES 4
#define CS_PIN 5

#define AP_SSID_PREFIX "YouTubeCounter-Setup-"
#define WOKWI_GUEST_SSID "Wokwi-GUEST"
#define DNS_PORT 53
#define WIFI_TIMEOUT_MS 15000
#define WOKWI_SETUP_TIMEOUT_MS 8000

MD_Parola Display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
WiFiClientSecure client;
WebServer server(80);
DNSServer dnsServer;
Preferences prefs;

const unsigned long DISPLAY_UPDATE_MS = 1000;
const unsigned long STAT_CYCLE_MS = 5000;
const uint8_t STAT_RIGHT_PADDING_COLUMNS = 1;
const unsigned int DEFAULT_REFRESH_MINUTES = 5;
const unsigned int MAX_REFRESH_MINUTES = 1440;
const double SECONDS_PER_28_DAYS = 28.0 * 24.0 * 60.0 * 60.0;
const int INTERVAL_PATTERN_LENGTH = 48;

const uint8_t STAT_SUBSCRIBERS = 1 << 0;
const uint8_t STAT_VIEWS = 1 << 1;
const uint8_t STAT_WATCH_HOURS = 1 << 2;
const uint8_t STAT_ALL = STAT_SUBSCRIBERS | STAT_VIEWS | STAT_WATCH_HOURS;

const int STAT_INDEX_SUBSCRIBERS = 0;
const int STAT_INDEX_VIEWS = 1;
const int STAT_INDEX_WATCH_HOURS = 2;
const int STAT_COUNT = 3;

const uint8_t STAT_MASKS[STAT_COUNT] = { STAT_SUBSCRIBERS, STAT_VIEWS, STAT_WATCH_HOURS };

unsigned long api_lasttime = 0;
unsigned long display_lasttime = 0;
unsigned long cycle_lasttime = 0;
unsigned long stats_fetched_at = 0;
double stat_baseline_values[STAT_COUNT] = { 0, 0, 0 };
double stat_increase_per_28_days[STAT_COUNT] = { 0, 0, 0 };
double stat_baseline_started_at_unix = 0;
double stats_as_of_unix = 0;
int current_stat_index = STAT_INDEX_SUBSCRIBERS;
bool statsLoaded = false;
StaticJsonDocument<1536> doc;

String saved_ssid, saved_pass, saved_endpoint;
uint8_t saved_stats = STAT_SUBSCRIBERS;
unsigned int saved_refresh_minutes = DEFAULT_REFRESH_MINUTES;
bool configMode = false;
bool captivePortalActive = false;

String stat_format(double value, int statIndex);
String getSetupApSsid();
String html_escape(String value);
bool fetchStats();
void showProjectedStat();
void renderRightAlignedStat(const char *text);
double getProjectedStatValue(int statIndex, double currentUnixTimestamp);
double getProjectedWholeStatValue(double baselineStartedAtUnix, double startingValue, double increasePer28Days, double currentUnixTimestamp);
double getProjectedFractionalStatValue(double baselineStartedAtUnix, double startingValue, double increasePer28Days, double currentUnixTimestamp);

// ─── Config portal HTML ───────────────────────────────────────────────────────
const char CONFIG_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Stats Counter Setup</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:#111;color:#e8e8e8;font-family:system-ui,sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:1rem}
  .card{background:#1a1a1a;border:1px solid #2a2a2a;border-radius:14px;width:100%;max-width:360px;overflow:hidden}
  .header{background:#0d0d0d;padding:18px 22px;border-bottom:1px solid #222}
  .logo{display:flex;align-items:center;gap:10px}
  .dot{width:28px;height:28px;background:#22c55e;border-radius:6px;display:flex;align-items:center;justify-content:center;font-size:16px;font-weight:700;color:#0a2e18}
  h1{font-size:15px;font-weight:600;color:#f0f0f0}
  .sub{font-size:12px;color:#555;margin-top:3px}
  .body{padding:20px 22px 24px;display:flex;flex-direction:column;gap:13px}
  .status{background:#0a1a10;border:1px solid #1a3a20;border-radius:8px;padding:8px 12px;font-size:12px;color:#4ade80;display:flex;align-items:center;gap:8px}
  .dot-green{width:7px;height:7px;border-radius:50%;background:#22c55e;flex-shrink:0}
  .section{font-size:11px;font-weight:600;color:#555;text-transform:uppercase;letter-spacing:.05em}
  label{font-size:11px;font-weight:600;color:#777;text-transform:uppercase;letter-spacing:.04em;display:block;margin-bottom:4px}
  input[type=text],input[type=password],input[type=url],input[type=number]{width:100%;background:#1c1c1c;border:1px solid #2e2e2e;border-radius:8px;padding:9px 12px;font-size:13px;color:#e8e8e8;outline:none}
  input[type=text]:focus,input[type=password]:focus,input[type=url]:focus,input[type=number]:focus{border-color:#22c55e}
  .checks{display:flex;flex-direction:column;gap:7px}
  .check{display:flex;align-items:center;gap:8px;background:#1c1c1c;border:1px solid #2e2e2e;border-radius:8px;padding:9px 12px;font-size:13px;color:#e8e8e8}
  .check input{accent-color:#22c55e}
  .check span{font-size:13px;color:#e8e8e8;text-transform:none;letter-spacing:0}
  input::placeholder{color:#444}
  .hint{font-size:11px;color:#555;margin-top:3px}
  .divider{height:1px;background:#222}
  .btn{width:100%;background:#22c55e;color:#0a2e18;border:none;border-radius:8px;padding:11px;font-size:14px;font-weight:600;cursor:pointer;margin-top:4px}
  .btn:hover{background:#1daa50}
  .btn-ota{width:100%;background:#1c1c1c;color:#e8e8e8;border:1px solid #2e2e2e;border-radius:8px;padding:11px;font-size:14px;font-weight:600;cursor:pointer;margin-top:4px}
  .btn-ota:hover{background:#252525}
  .msg{font-size:13px;text-align:center;padding:8px;border-radius:8px;display:none}
  .msg.ok{background:#0a1a10;color:#4ade80;border:1px solid #1a3a20;display:block}
  .msg.err{background:#1a0a0a;color:#f87171;border:1px solid #3a1a1a;display:block}
  .msg.info{background:#0a0e1a;color:#60a5fa;border:1px solid #1a2a3a;display:block}
  .progress-wrap{background:#1c1c1c;border-radius:8px;height:8px;overflow:hidden;display:none}
  .progress-wrap.show{display:block}
  .progress-bar{height:100%;width:0%;background:#22c55e;border-radius:8px;transition:width .2s}
  input[type=file]{width:100%;background:#1c1c1c;border:1px solid #2e2e2e;border-radius:8px;padding:8px 12px;font-size:13px;color:#888;outline:none;cursor:pointer}
  input[type=file]::file-selector-button{background:#22c55e;color:#0a2e18;border:none;border-radius:6px;padding:4px 10px;font-size:12px;font-weight:600;cursor:pointer;margin-right:10px}
  .btn-scan{width:100%;background:#1c1c1c;color:#e8e8e8;border:1px solid #2e2e2e;border-radius:8px;padding:9px;font-size:13px;font-weight:600;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:6px}
  .btn-scan:hover{background:#252525}
  .btn-scan.spinning span{display:inline-block;animation:spin .8s linear infinite}
  @keyframes spin{to{transform:rotate(360deg)}}
  .net-list{display:flex;flex-direction:column;gap:6px;display:none}
  .net-list.show{display:flex}
  .net-item{display:flex;align-items:center;justify-content:space-between;background:#1c1c1c;border:1px solid #2e2e2e;border-radius:8px;padding:9px 12px;cursor:pointer;font-size:13px;transition:border-color .15s}
  .net-item:hover{border-color:#22c55e}
  .net-item.active{border-color:#22c55e;background:#0a1a10}
  .net-name{color:#e8e8e8;font-weight:500}
  .net-meta{display:flex;align-items:center;gap:8px}
  .net-lock{font-size:11px;color:#555}
  .net-bars{display:flex;align-items:flex-end;gap:2px;height:14px}
  .net-bars span{width:3px;background:#555;border-radius:1px}
  .net-bars span.lit{background:#22c55e}
  .pw-wrap{position:relative}
  .pw-wrap input{padding-right:38px}
  .eye-btn{position:absolute;right:10px;top:50%;transform:translateY(-50%);background:none;border:none;cursor:pointer;color:#555;font-size:16px;line-height:1;padding:0}
  .eye-btn:hover{color:#aaa}
</style></head><body>
<div class="card">
  <div class="header">
    <div class="logo"><div class="dot">&#9654;</div><div><h1>Stats Counter</h1><div class="sub">Device setup</div></div></div>
  </div>
  <div class="body">
    <div class="status"><div class="dot-green"></div>Connected &mdash; IP_PLACEHOLDER</div>
    <div class="section">Wi-Fi</div>
    <button class="btn-scan" id="scan-btn" onclick="scanNetworks()"><span>&#8635;</span> Scan for networks</button>
    <div class="net-list" id="net-list"></div>
    <div><label>Network name (SSID)</label><input type="text" id="ssid" placeholder="Leave blank to keep saved" value="SSID_PLACEHOLDER"></div>
    <div><label>Password <span style="color:#555;font-weight:400;text-transform:none">(leave blank to keep saved)</span></label><div class="pw-wrap"><input type="password" id="pw" placeholder="&bull;&bull;&bull;&bull;&bull;&bull;&bull;&bull;"><button class="eye-btn" type="button" onclick="toggleEye('pw','eye-pw')"><span id="eye-pw">&#128065;</span></button></div></div>
    <div class="divider"></div>
    <div class="section">Stats API</div>
    <div>
      <label>Stats API endpoint</label>
      <input type="url" id="endpoint" placeholder="https://your-domain.example/api/stats" value="ENDPOINT_PLACEHOLDER">
      <div class="hint">Must return the documented stats JSON shape.</div>
    </div>
    <div>
      <label>Stats to show</label>
      <div class="checks">
        <label class="check"><input type="checkbox" id="stat-subs" SUBS_CHECKED><span>Subscribers</span></label>
        <label class="check"><input type="checkbox" id="stat-views" VIEWS_CHECKED><span>Views</span></label>
        <label class="check"><input type="checkbox" id="stat-hours" HOURS_CHECKED><span>Watch hours</span></label>
      </div>
      <div class="hint">Multiple selections cycle on the matrix.</div>
    </div>
    <div>
      <label>Refresh rate (minutes)</label>
      <input type="number" id="refresh" min="1" max="1440" step="1" value="REFRESH_PLACEHOLDER">
      <div class="hint">The display projects values every second between API refreshes.</div>
    </div>
    <div id="msg" class="msg"></div>
    <button class="btn" onclick="save()">Save &amp; connect</button>
    <div class="divider"></div>
    <div class="section">Firmware update</div>
    <div>
      <label>Firmware .bin file</label>
      <input type="file" id="binfile" accept=".bin">
      <div class="hint">PlatformIO build output: .pio/build/esp32dev/firmware.bin</div>
    </div>
    <div class="progress-wrap" id="prog-wrap"><div class="progress-bar" id="prog-bar"></div></div>
    <div id="ota-msg" class="msg"></div>
    <button class="btn-ota" onclick="uploadFirmware()">Upload firmware</button>
  </div>
</div>
<script>
function barsHtml(rssi){
  var lvl=rssi>-55?4:rssi>-65?3:rssi>-75?2:1;
  var h='<div class="net-bars">';
  var heights=[4,7,10,14];
  for(var i=0;i<4;i++){h+='<span style="height:'+heights[i]+'px"'+(i<lvl?' class="lit"':'')+' ></span>';}
  return h+'</div>';
}
function scanNetworks(){
  var btn=document.getElementById('scan-btn');
  var list=document.getElementById('net-list');
  btn.classList.add('spinning');
  btn.disabled=true;
  list.className='net-list';
  list.innerHTML='';
  fetch('/scan').then(r=>r.json()).then(nets=>{
    btn.classList.remove('spinning');
    btn.disabled=false;
    if(!nets.length){list.innerHTML='<div style="font-size:12px;color:#555;text-align:center">No networks found</div>';list.className='net-list show';return;}
    list.innerHTML=nets.map(function(n){
      return '<div class="net-item" onclick="pickNet(this,\''+n.ssid.replace(/'/g,"\\'")+'\')">'
        +'<span class="net-name">'+n.ssid+'</span>'
        +'<span class="net-meta">'+barsHtml(n.rssi)+(n.secure?'<span class="net-lock">&#128274;</span>':'')+'</span>'
        +'</div>';
    }).join('');
    list.className='net-list show';
  }).catch(function(){
    btn.classList.remove('spinning');
    btn.disabled=false;
  });
}
function pickNet(el,ssid){
  document.querySelectorAll('.net-item').forEach(function(x){x.classList.remove('active');});
  el.classList.add('active');
  document.getElementById('ssid').value=ssid;
  document.getElementById('pw').focus();
}
function toggleEye(inputId,iconId){
  var inp=document.getElementById(inputId);
  var ico=document.getElementById(iconId);
  if(inp.type==='password'){inp.type='text';ico.textContent='\uD83D\uDE48';}
  else{inp.type='password';ico.textContent='\uD83D\uDC41';}
}
function save(){
  var s=document.getElementById('ssid').value.trim();
  var p=document.getElementById('pw').value;
  var e=document.getElementById('endpoint').value.trim();
  var r=parseInt(document.getElementById('refresh').value,10);
  var stats=0;
  if(document.getElementById('stat-subs').checked)stats|=1;
  if(document.getElementById('stat-views').checked)stats|=2;
  if(document.getElementById('stat-hours').checked)stats|=4;
  var msg=document.getElementById('msg');
  if(!e){msg.className='msg err';msg.textContent='Stats API endpoint is required.';return;}
  if(!/^https?:\/\//i.test(e)){msg.className='msg err';msg.textContent='Endpoint must start with http:// or https://';return;}
  if(!stats){msg.className='msg err';msg.textContent='Choose at least one stat to show.';return;}
  if(!r||r<1){msg.className='msg err';msg.textContent='Refresh rate must be at least 1 minute.';return;}
  msg.className='msg ok';msg.textContent='Saving\u2026 device will reboot and connect.';
  fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p)+'&endpoint='+encodeURIComponent(e)+'&stats='+stats+'&refresh='+r})
  .then(r=>r.text()).then(t=>{msg.textContent=t;});
}
function uploadFirmware(){
  var file=document.getElementById('binfile').files[0];
  var omsg=document.getElementById('ota-msg');
  var bar=document.getElementById('prog-bar');
  var wrap=document.getElementById('prog-wrap');
  if(!file){omsg.className='msg err';omsg.textContent='Select a .bin file first.';return;}
  if(!file.name.endsWith('.bin')){omsg.className='msg err';omsg.textContent='File must be a .bin';return;}
  var xhr=new XMLHttpRequest();
  xhr.open('POST','/update',true);
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable){
      var pct=Math.round(e.loaded/e.total*100);
      wrap.className='progress-wrap show';
      bar.style.width=pct+'%';
      omsg.className='msg info';
      omsg.textContent='Uploading\u2026 '+pct+'%';
    }
  };
  xhr.onload=function(){
    if(xhr.status===200){
      omsg.className='msg ok';
      omsg.textContent='Update complete! Rebooting\u2026';
      bar.style.width='100%';
    } else {
      omsg.className='msg err';
      omsg.textContent='Update failed: '+xhr.responseText;
    }
  };
  xhr.onerror=function(){omsg.className='msg err';omsg.textContent='Upload error. Check connection.';};
  var fd=new FormData();
  fd.append('firmware',file,file.name);
  xhr.send(fd);
}
</script>
</body></html>
)rawhtml";

// ─── Helpers ──────────────────────────────────────────────────────────────────
void loadPrefs() {
  prefs.begin("ytcounter", true);
  saved_ssid            = prefs.getString("ssid", "");
  saved_pass            = prefs.getString("pass", "");
  saved_endpoint        = prefs.getString("endpoint", "");
  saved_stats           = prefs.getUChar("stats", STAT_SUBSCRIBERS) & STAT_ALL;
  saved_refresh_minutes = prefs.getUInt("refresh", DEFAULT_REFRESH_MINUTES);
  prefs.end();

  if (saved_stats == 0) saved_stats = STAT_SUBSCRIBERS;
  if (saved_refresh_minutes < 1) saved_refresh_minutes = DEFAULT_REFRESH_MINUTES;
  if (saved_refresh_minutes > MAX_REFRESH_MINUTES) saved_refresh_minutes = MAX_REFRESH_MINUTES;

  Serial.print("Config loaded: ssid=");
  Serial.print(saved_ssid.length() ? saved_ssid : "(empty)");
  Serial.print(", endpoint=");
  Serial.print(saved_endpoint.length() ? "set" : "empty");
  Serial.print(", stats=");
  Serial.println(saved_stats);
}

void savePrefs(String ssid, String pass, String endpoint, uint8_t stats, unsigned int refreshMinutes) {
  prefs.begin("ytcounter", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putString("endpoint", endpoint);
  prefs.putUChar("stats", stats);
  prefs.putUInt("refresh", refreshMinutes);
  prefs.end();
}

String html_escape(String value) {
  value.replace("&", "&amp;");
  value.replace("\"", "&quot;");
  value.replace("'", "&#39;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  return value;
}

String buildPage() {
  String page = String(CONFIG_HTML);
  String ip = configMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  page.replace("IP_PLACEHOLDER", ip);
  page.replace("SSID_PLACEHOLDER", html_escape(saved_ssid));
  page.replace("ENDPOINT_PLACEHOLDER", html_escape(saved_endpoint));
  page.replace("SUBS_CHECKED", (saved_stats & STAT_SUBSCRIBERS) ? "checked" : "");
  page.replace("VIEWS_CHECKED", (saved_stats & STAT_VIEWS) ? "checked" : "");
  page.replace("HOURS_CHECKED", (saved_stats & STAT_WATCH_HOURS) ? "checked" : "");
  page.replace("REFRESH_PLACEHOLDER", String(saved_refresh_minutes));
  return page;
}

// ─── Server handlers ──────────────────────────────────────────────────────────
void handleRoot() {
  server.send(200, "text/html", buildPage());
}

void handleSave() {
  if (!server.hasArg("endpoint")) {
    server.send(400, "text/plain", "Stats API endpoint is required.");
    return;
  }

  String endpoint = server.arg("endpoint");
  endpoint.trim();
  if (endpoint.length() == 0) {
    server.send(400, "text/plain", "Stats API endpoint is required.");
    return;
  }
  if (!endpoint.startsWith("http://") && !endpoint.startsWith("https://")) {
    server.send(400, "text/plain", "Endpoint must start with http:// or https://");
    return;
  }

  uint8_t stats = server.arg("stats").toInt() & STAT_ALL;
  if (stats == 0) {
    server.send(400, "text/plain", "Choose at least one stat to show.");
    return;
  }

  int refreshMinutes = server.arg("refresh").toInt();
  if (refreshMinutes < 1) refreshMinutes = 1;
  if (refreshMinutes > MAX_REFRESH_MINUTES) refreshMinutes = MAX_REFRESH_MINUTES;

  String new_ssid = server.arg("ssid");
  new_ssid.trim();
  if (new_ssid.length() == 0) {
    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == WOKWI_GUEST_SSID) {
      new_ssid = WOKWI_GUEST_SSID;
    } else {
      new_ssid = saved_ssid;
    }
  }
  String new_pass = server.arg("pass");
  if (new_pass.length() == 0) new_pass = saved_pass;
  savePrefs(new_ssid, new_pass, endpoint, stats, refreshMinutes);
  server.send(200, "text/plain", "Saved! Rebooting now...");
  delay(1500);
  ESP.restart();
}

void handleUpdate() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("OTA start: %s\n", upload.filename.c_str());
    Display.print("OTA...");
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("OTA success: %u bytes\n", upload.totalSize);
      Display.print("Rebooting");
    } else {
      Update.printError(Serial);
      Display.print("OTA fail");
    }
  }
}

void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\","
            "\"rssi\":" + String(WiFi.RSSI(i)) + ","
            "\"secure\":" + (WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
  }
  json += "]";
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

void handleUpdateFinish() {
  if (Update.hasError()) {
    server.send(500, "text/plain", Update.errorString());
  } else {
    server.send(200, "text/plain", "OK");
    delay(1000);
    ESP.restart();
  }
}

void registerRoutes() {
  server.on("/",       handleRoot);
  server.on("/scan",   HTTP_GET,  handleScan);
  server.on("/save",   HTTP_POST, handleSave);
  server.on("/update", HTTP_POST, handleUpdateFinish, handleUpdate);
  server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
}

bool isStatSelected(int index) {
  return (saved_stats & STAT_MASKS[index]) != 0;
}

int selectedStatsCount() {
  int count = 0;
  for (int i = 0; i < STAT_COUNT; i++) {
    if (isStatSelected(i)) count++;
  }
  return count;
}

int firstSelectedStatIndex() {
  for (int i = 0; i < STAT_COUNT; i++) {
    if (isStatSelected(i)) return i;
  }
  return STAT_INDEX_SUBSCRIBERS;
}

int nextSelectedStatIndex(int currentIndex) {
  for (int step = 1; step <= STAT_COUNT; step++) {
    int index = (currentIndex + step) % STAT_COUNT;
    if (isStatSelected(index)) return index;
  }
  return firstSelectedStatIndex();
}

void ensureCurrentStatSelected() {
  if (!isStatSelected(current_stat_index)) {
    current_stat_index = firstSelectedStatIndex();
  }
}

double interval_weights[INTERVAL_PATTERN_LENGTH];
bool interval_weights_initialized = false;

double getDeterministicNoise(uint32_t incrementNumber) {
  uint32_t hash = (incrementNumber ^ 0x9e3779b9UL) * 0x85ebca6bUL;
  hash ^= hash >> 13;
  hash *= 0xc2b2ae35UL;
  hash ^= hash >> 16;

  return (double)hash / 4294967296.0;
}

double getIntervalWeight(int patternIndex) {
  double noise = getDeterministicNoise((uint32_t)patternIndex + 1);

  if (noise < 0.22) {
    return 0.08 + (noise / 0.22) * 0.17;
  }

  if (noise < 0.42) {
    return 0.34 + ((noise - 0.22) / 0.2) * 0.36;
  }

  if (noise < 0.82) {
    return 0.9 + ((noise - 0.42) / 0.4) * 0.45;
  }

  return 1.8 + ((noise - 0.82) / 0.18) * 1.6;
}

void ensureIntervalWeightsInitialized() {
  if (interval_weights_initialized) return;

  double rawWeights[INTERVAL_PATTERN_LENGTH];
  double rawWeightTotal = 0;

  for (int i = 0; i < INTERVAL_PATTERN_LENGTH; i++) {
    rawWeights[i] = getIntervalWeight(i);
    rawWeightTotal += rawWeights[i];
  }

  for (int i = 0; i < INTERVAL_PATTERN_LENGTH; i++) {
    interval_weights[i] = (rawWeights[i] * INTERVAL_PATTERN_LENGTH) / rawWeightTotal;
  }

  interval_weights_initialized = true;
}

uint64_t getJitteredIncrementCount(double elapsedSeconds, double secondsPerIncrement) {
  ensureIntervalWeightsInitialized();

  double cycleSeconds = secondsPerIncrement * INTERVAL_PATTERN_LENGTH;
  uint64_t completedCycles = (uint64_t)floor(elapsedSeconds / cycleSeconds);
  uint64_t incrementCount = completedCycles * INTERVAL_PATTERN_LENGTH;
  double remainingSeconds = elapsedSeconds - (completedCycles * cycleSeconds);

  for (int i = 0; i < INTERVAL_PATTERN_LENGTH; i++) {
    double intervalSeconds = interval_weights[i] * secondsPerIncrement;

    if (remainingSeconds < intervalSeconds) {
      break;
    }

    remainingSeconds -= intervalSeconds;
    incrementCount += 1;
  }

  return incrementCount;
}

double getProjectedWholeStatValue(double baselineStartedAtUnix, double startingValue, double increasePer28Days, double currentUnixTimestamp) {
  double elapsedSeconds = max(0.0, currentUnixTimestamp - baselineStartedAtUnix);
  double secondsPerIncrement = SECONDS_PER_28_DAYS / increasePer28Days;

  if (!isfinite(secondsPerIncrement) || secondsPerIncrement <= 0) {
    return startingValue;
  }

  return startingValue + getJitteredIncrementCount(elapsedSeconds, secondsPerIncrement);
}

double getProjectedFractionalStatValue(double baselineStartedAtUnix, double startingValue, double increasePer28Days, double currentUnixTimestamp) {
  double elapsedSeconds = max(0.0, currentUnixTimestamp - baselineStartedAtUnix);
  double increasePerSecond = increasePer28Days / SECONDS_PER_28_DAYS;

  if (!isfinite(increasePerSecond) || increasePerSecond <= 0) {
    return startingValue;
  }

  return startingValue + elapsedSeconds * increasePerSecond;
}

double getProjectedStatValue(int statIndex, double currentUnixTimestamp) {
  if (statIndex == STAT_INDEX_WATCH_HOURS) {
    return getProjectedFractionalStatValue(
      stat_baseline_started_at_unix,
      stat_baseline_values[statIndex],
      stat_increase_per_28_days[statIndex],
      currentUnixTimestamp
    );
  }

  return getProjectedWholeStatValue(
    stat_baseline_started_at_unix,
    stat_baseline_values[statIndex],
    stat_increase_per_28_days[statIndex],
    currentUnixTimestamp
  );
}

bool fetchStats() {
  HTTPClient http;
  bool beginOk = false;

  Serial.print("Fetching stats: ");
  Serial.println(saved_endpoint);

  if (saved_endpoint.startsWith("https://")) {
    client.setInsecure();
    beginOk = http.begin(client, saved_endpoint);
  } else {
    beginOk = http.begin(saved_endpoint);
  }

  if (!beginOk) {
    Serial.println("HTTP begin failed.");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("Stats endpoint returned HTTP ");
    Serial.println(httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  DeserializationError error = deserializeJson(doc, payload);
  http.end();

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

  JsonObject baseline = doc["baseline"].as<JsonObject>();
  JsonObject metrics28Days = doc["metrics28Days"].as<JsonObject>();
  JsonObject adjusted = doc["adjusted"].as<JsonObject>();
  if (baseline.isNull() || metrics28Days.isNull() || adjusted.isNull()) {
    Serial.println("Stats response missing baseline, metrics28Days, or adjusted.");
    return false;
  }

  stat_baseline_started_at_unix = baseline["startedAtUnix"] | 0.0;
  stats_as_of_unix = adjusted["asOfUnix"] | 0.0;

  if (stat_baseline_started_at_unix <= 0 || stats_as_of_unix <= 0) {
    Serial.println("Stats response missing valid Unix timestamps.");
    return false;
  }

  stat_baseline_values[STAT_INDEX_SUBSCRIBERS] = baseline["subscribers"] | 0.0;
  stat_baseline_values[STAT_INDEX_VIEWS] = baseline["totalViews"] | 0.0;
  stat_baseline_values[STAT_INDEX_WATCH_HOURS] = baseline["watchHours"] | 0.0;
  stat_increase_per_28_days[STAT_INDEX_SUBSCRIBERS] = metrics28Days["subscribers"] | 0.0;
  stat_increase_per_28_days[STAT_INDEX_VIEWS] = metrics28Days["views"] | 0.0;
  stat_increase_per_28_days[STAT_INDEX_WATCH_HOURS] = metrics28Days["watchHours"] | 0.0;

  stats_fetched_at = millis();
  statsLoaded = true;
  ensureCurrentStatSelected();

  Serial.println("Stats updated.");
  return true;
}

static char statDisplayBuffer[20];
static String lastDisplayedValue = "";
static bool statDisplayScrolling = false;

void renderRightAlignedStat(const char *text) {
  MD_MAX72XX *matrix = Display.getGraphicObject();
  uint8_t charSpacing = Display.getCharSpacing();
  uint16_t col = STAT_RIGHT_PADDING_COLUMNS;
  uint8_t glyph[8];

  matrix->update(MD_MAX72XX::OFF);
  matrix->clear();

  for (int i = (int)strlen(text) - 1; i >= 0; i--) {
    uint8_t glyphWidth = matrix->getChar(text[i], sizeof(glyph), glyph);
    if (glyphWidth == 0) continue;

    matrix->setChar(col + glyphWidth - 1, text[i]);
    col += glyphWidth + charSpacing;
  }

  matrix->update(MD_MAX72XX::ON);
}

void showProjectedStat() {
  if (!statsLoaded) return;

  ensureCurrentStatSelected();
  double currentUnixTimestamp = stats_as_of_unix + ((millis() - stats_fetched_at) / 1000.0);
  double projected = getProjectedStatValue(current_stat_index, currentUnixTimestamp);

  String formatted = stat_format(projected, current_stat_index);
  if (formatted == lastDisplayedValue) return;
  lastDisplayedValue = formatted;

  formatted.toCharArray(statDisplayBuffer, sizeof(statDisplayBuffer));
  Serial.println(formatted);

  uint16_t startCol, endCol;
  Display.getDisplayExtent(startCol, endCol);
  uint16_t displayWidth = endCol - startCol + 1;
  uint16_t textWidth = Display.getTextColumns(statDisplayBuffer);

  if (textWidth + STAT_RIGHT_PADDING_COLUMNS <= displayWidth) {
    statDisplayScrolling = false;
    renderRightAlignedStat(statDisplayBuffer);
  } else {
    statDisplayScrolling = true;
    Display.displayScroll(statDisplayBuffer, PA_RIGHT, PA_SCROLL_LEFT, 80);
  }
}

bool startWokwiConfigPortal() {
  Display.print(" Wokwi...");
  Serial.println("Trying Wokwi-GUEST for setup portal...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WOKWI_GUEST_SSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WOKWI_SETUP_TIMEOUT_MS) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wokwi-GUEST not found.");
    return false;
  }

  configMode = true;
  registerRoutes();
  server.begin();

  Serial.println("Wokwi setup portal ready.");
  Serial.println("Open http://localhost:8180 in your browser.");
  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP());

  Display.print("Setup");
  return true;
}

String getSetupApSsid() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
  return String(AP_SSID_PREFIX) + suffix;
}

void startConfigPortal() {
  configMode = true;
  Display.print("Setup AP");

  WiFi.mode(WIFI_AP);
  String setupApSsid = getSetupApSsid();
  WiFi.softAP(setupApSsid.c_str());
  delay(500);

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  captivePortalActive = true;
  registerRoutes();
  server.begin();

  Serial.println("Config portal started.");
  Serial.print("AP SSID: ");
  Serial.println(setupApSsid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  Display.displayScroll(setupApSsid.c_str(), PA_LEFT, PA_SCROLL_LEFT, 80);
  while (!Display.displayAnimate()) { delay(10); }
  delay(2000);
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  Display.begin();
  Display.setIntensity(0);
  Display.setFont(fontSubs);
  Display.setTextAlignment(PA_CENTER);

  loadPrefs();

  bool hasCredentials = (saved_ssid.length() > 0 &&
                         saved_endpoint.length() > 0 &&
                         (saved_stats & STAT_ALL) != 0);

  if (hasCredentials) {
    Display.print(" WiFi...");
    Serial.print("Connecting to WiFi: ");
    Serial.println(saved_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
      Serial.print(".");
      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("Connected! IP: ");
      Serial.println(WiFi.localIP());

      // Scroll IP across matrix then pause on last frame for 2 seconds
      String ip = WiFi.localIP().toString();
      Display.displayScroll(ip.c_str(), PA_LEFT, PA_SCROLL_LEFT, 80);
      while (!Display.displayAnimate()) { delay(10); }
      delay(2000);

      registerRoutes();
      server.begin();

      Display.displayClear();
      Display.print("fetching");
      delay(250);

      client.setInsecure();
    } else {
      Serial.println("WiFi failed. Starting config portal.");
      if (!startWokwiConfigPortal()) {
        startConfigPortal();
      }
    }
  } else {
    Serial.println("No credentials. Starting config portal.");
    if (!startWokwiConfigPortal()) {
      startConfigPortal();
    }
  }
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  if (configMode) {
    if (captivePortalActive) {
      dnsServer.processNextRequest();
    }
    server.handleClient();
    return;
  }

  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) {
    unsigned long now = millis();
    unsigned long refreshInterval = (unsigned long)saved_refresh_minutes * 60000UL;

    if (!statsLoaded || api_lasttime == 0 || now - api_lasttime >= refreshInterval) {
      if (fetchStats()) {
        showProjectedStat();
        display_lasttime = now;
        cycle_lasttime = now;
      } else if (!statsLoaded) {
        Display.print("API err");
      }
      api_lasttime = now;
    }

    if (statsLoaded && statDisplayScrolling) {
      Display.displayAnimate();
    }

    if (statsLoaded) {
      if (selectedStatsCount() > 1 && now - cycle_lasttime >= STAT_CYCLE_MS) {
        current_stat_index = nextSelectedStatIndex(current_stat_index);
        lastDisplayedValue = "";
        showProjectedStat();
        display_lasttime = now;
        cycle_lasttime = now;
      } else if (now - display_lasttime >= DISPLAY_UPDATE_MS) {
        showProjectedStat();
        display_lasttime = now;
      }
    }
  }
}

String stat_format(double value, int statIndex) {
  if (statIndex == STAT_INDEX_WATCH_HOURS) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%.1f", value);
    return String(buf);
  }
  return String((long)(value + 0.5));
}
