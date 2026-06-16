// Library Imports
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <string>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Update.h>

// File imports
#include "FontSubs.h"

// Hardware config
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
//#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW
#define MAX_DEVICES 4
#define CS_PIN 5

#define AP_SSID "YouTubeCounter-Setup"
#define DNS_PORT 53
#define WIFI_TIMEOUT_MS 15000

MD_Parola Display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
WiFiClientSecure client;
WebServer server(80);
DNSServer dnsServer;
Preferences prefs;

unsigned long api_mtbs = 10000;
unsigned long api_lasttime;
long subs = 0;
long views = 0;
String apiUrl = "";
StaticJsonDocument<1000> doc;
bool showSubs = false;

String saved_ssid, saved_pass, saved_channel, saved_apikey;
bool configMode = false;

String num_format(long num);

// ─── Config portal HTML ───────────────────────────────────────────────────────
const char CONFIG_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>YouTube Counter Setup</title>
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
  input[type=text],input[type=password]{width:100%;background:#1c1c1c;border:1px solid #2e2e2e;border-radius:8px;padding:9px 12px;font-size:13px;color:#e8e8e8;outline:none}
  input[type=text]:focus,input[type=password]:focus{border-color:#22c55e}
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
    <div class="logo"><div class="dot">&#9654;</div><div><h1>YouTube Stats Counter</h1><div class="sub">Device setup</div></div></div>
  </div>
  <div class="body">
    <div class="status"><div class="dot-green"></div>Connected &mdash; IP_PLACEHOLDER</div>
    <div class="section">Wi-Fi</div>
    <button class="btn-scan" id="scan-btn" onclick="scanNetworks()"><span>&#8635;</span> Scan for networks</button>
    <div class="net-list" id="net-list"></div>
    <div><label>Network name (SSID)</label><input type="text" id="ssid" placeholder="Leave blank to keep saved" value="SSID_PLACEHOLDER"></div>
    <div><label>Password <span style="color:#555;font-weight:400;text-transform:none">(leave blank to keep saved)</span></label><div class="pw-wrap"><input type="password" id="pw" placeholder="&bull;&bull;&bull;&bull;&bull;&bull;&bull;&bull;"><button class="eye-btn" type="button" onclick="toggleEye('pw','eye-pw')"><span id="eye-pw">&#128065;</span></button></div></div>
    <div class="divider"></div>
    <div class="section">YouTube API</div>
    <div>
      <label>Channel ID</label>
      <input type="text" id="channel" placeholder="UCxxxxxxxxxxxxxxxxxx" value="CHANNEL_PLACEHOLDER">
      <div class="hint">YouTube Studio &rarr; Settings &rarr; Channel &rarr; Advanced</div>
    </div>
    <div>
      <label>API key <span style="color:#555;font-weight:400;text-transform:none">(leave blank to keep saved)</span></label>
      <div class="pw-wrap"><input type="password" id="apikey" placeholder="AIzaSy..."><button class="eye-btn" type="button" onclick="toggleEye('apikey','eye-api')"><span id="eye-api">&#128065;</span></button></div>
      <div class="hint">Google Cloud Console &rarr; Credentials</div>
    </div>
    <div id="msg" class="msg"></div>
    <button class="btn" onclick="save()">Save &amp; connect</button>
    <div class="divider"></div>
    <div class="section">Firmware update</div>
    <div>
      <label>Firmware .bin file</label>
      <input type="file" id="binfile" accept=".bin">
      <div class="hint">Arduino IDE &rarr; Sketch &rarr; Export compiled binary</div>
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
  var c=document.getElementById('channel').value.trim();
  var k=document.getElementById('apikey').value.trim();
  var msg=document.getElementById('msg');
  if(!c){msg.className='msg err';msg.textContent='Channel ID is required.';return;}
  msg.className='msg ok';msg.textContent='Saving\u2026 device will reboot and connect.';
  fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p)+'&channel='+encodeURIComponent(c)+'&apikey='+encodeURIComponent(k)})
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
  saved_ssid    = prefs.getString("ssid",    "");
  saved_pass    = prefs.getString("pass",    "");
  saved_channel = prefs.getString("channel", "");
  saved_apikey  = prefs.getString("apikey",  "");
  prefs.end();
}

void savePrefs(String ssid, String pass, String channel, String apikey) {
  prefs.begin("ytcounter", false);
  prefs.putString("ssid",    ssid);
  prefs.putString("pass",    pass);
  prefs.putString("channel", channel);
  prefs.putString("apikey",  apikey);
  prefs.end();
}

String buildPage() {
  String page = String(CONFIG_HTML);
  String ip = configMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  page.replace("IP_PLACEHOLDER",      ip);
  page.replace("SSID_PLACEHOLDER",    saved_ssid);
  page.replace("CHANNEL_PLACEHOLDER", saved_channel);
  return page;
}

// ─── Server handlers ──────────────────────────────────────────────────────────
void handleRoot() {
  server.send(200, "text/html", buildPage());
}

void handleSave() {
  if (!server.hasArg("channel")) {
    server.send(400, "text/plain", "Channel ID is required.");
    return;
  }
  String new_ssid = server.arg("ssid");
  if (new_ssid.length() == 0) new_ssid = saved_ssid;
  String new_pass = server.arg("pass");
  if (new_pass.length() == 0) new_pass = saved_pass;
  String new_apikey = server.arg("apikey");
  if (new_apikey.length() == 0) new_apikey = saved_apikey;
  savePrefs(new_ssid, new_pass, server.arg("channel"), new_apikey);
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

void startConfigPortal() {
  configMode = true;
  Display.print("Setup AP");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  delay(500);

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  registerRoutes();
  server.begin();

  Serial.println("Config portal started.");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  Display.print("WiFi:YT");
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
                         saved_channel.length() > 0 &&
                         saved_apikey.length() > 0);

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
      startConfigPortal();
    }
  } else {
    Serial.println("No credentials. Starting config portal.");
    startConfigPortal();
  }
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  if (configMode) {
    dnsServer.processNextRequest();
    server.handleClient();
    return;
  }

  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) {
    if (millis() - api_lasttime > api_mtbs) {
      HTTPClient http;

      std::string channelID(saved_channel.c_str());
      std::string apiKey(saved_apikey.c_str());

      apiUrl = ("https://www.googleapis.com/youtube/v3/channels?part=statistics&id=" +
                channelID + "&key=" + apiKey).c_str();

      http.begin(apiUrl);
      int httpCode = http.GET();

      if (httpCode > 0) {
        String payload = http.getString();

        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }

        Serial.print(payload);
        if (showSubs) {
          subs = doc["items"][0]["statistics"]["subscriberCount"];
          String subsCount = num_format(subs);
          Serial.println(subsCount);
          Display.print("*" + subsCount);
        } else {
          views = doc["items"][0]["statistics"]["viewCount"];
          String viewsCount = num_format(views);
          Serial.println(viewsCount);
          Display.print("*" + viewsCount);
        }

        showSubs = !showSubs;
      }
      api_lasttime = millis();
    }
  }
}

// ─── Number formatter (original, untouched) ──────────────────────────────────
// Code from The Swedish Maker
// https://www.youtube.com/@TheSwedishMaker
String num_format(long num) {
  String num_s;
  long num_original = num;
  if (num > 99999 && num <= 999999) {
    num = num / 1000;
    long fraction = num_original % 1000;
    String num_fraction = String(fraction);
    String decimal = num_fraction.substring(0, 1);
    num_s = String(num) + "." + decimal + "K";
  } else if (num > 999999) {
    num = num / 1000000;
    long fraction = num_original % 1000000;
    String num_fraction = String(fraction);
    String decimal = num_fraction.substring(0, 1);
    if (num_original < 100000000) {
      num_s = " " + String(num) + "." + decimal + "M";
    } else {
      num_s = String(num) + "." + decimal + "M";
    }
  } else {
    int num_l = String(num).length();
    char num_f[15];
    int blankDigits = 6 - num_l;
    for (int i = 0; i < blankDigits; i++) {
      num_f[i] = ' ';
    }
    num_f[blankDigits] = '\0';
    num_s = num_f + String(num);
  }
  return num_s;
}
