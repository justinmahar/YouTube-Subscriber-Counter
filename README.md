# YouTube Stats Counter

An ESP32-based display that shows your YouTube subscriber and view counts on a MAX7219 LED matrix. No IDE required after the first flash — configure everything through a built-in web portal and push firmware updates over Wi-Fi.

<div align="center">
  <img src="https://raw.githubusercontent.com/ThePrintingPilot/YouTube-Subscriber-Counter/refs/heads/main/Images/Thumbnail2.png" width="560" alt="YouTube Subscriber Counter V2.0">
</div>

---

## ⚡ [Install via browser](https://theprintingpilot.github.io/YouTube-Subscriber-Counter/) — no IDE needed

---

## Features

- Alternates between subscriber count and view count on a 4-module MAX7219 matrix
- **Captive portal setup** — on first boot the ESP32 broadcasts a hotspot; connect and configure everything from your phone or browser, no code changes needed
- **Persistent config** — credentials saved to NVS flash, survive reboots and firmware updates
- **Wi-Fi scanner** — scan nearby networks and tap to auto-fill the SSID field
- **IP display on boot** — scrolls your local IP across the matrix for 5 seconds so you always know where to reach the config page
- **In-browser reconfigure** — visit the device IP any time to change Wi-Fi or YouTube settings; leave any field blank to keep the saved value
- **OTA firmware updates** — upload a compiled `.bin` straight from the config page, no USB cable needed after first flash

---

## Hardware

### Electronics

| Component | Link |
|---|---|
| ESP32 Dev Board | [AliExpress](https://s.click.aliexpress.com/e/_opLIWvk) |
| MAX7219 Dot Matrix (4 modules) | [AliExpress](https://s.click.aliexpress.com/e/_oo3TdS6) |

### Hardware

| Component | Link |
|---|---|
| M3 Thread Inserts | [AliExpress](https://s.click.aliexpress.com/e/_c2Iun0o1) |
| M3x8 Screws | [AliExpress](https://s.click.aliexpress.com/e/_oogbRPM) |
| Acrylic Sheet | [AliExpress](https://s.click.aliexpress.com/e/_oorPPai) |
| 6x3mm Magnets | [AliExpress](https://s.click.aliexpress.com/e/_c3qP6N2t) |

### Battery / Wireless Version (optional)

| Component | Link |
|---|---|
| Battery | [AliExpress](https://s.click.aliexpress.com/e/_opFpjhg) |
| BMS | [AliExpress](https://s.click.aliexpress.com/e/_c4sosls1) |
| On/Off Switch | [AliExpress](https://s.click.aliexpress.com/e/_oDqU0l8) |

> Links are affiliate links — they help support the project at no extra cost to you.

<div align="center">
  <img src="https://github.com/ThePrintingPilot/YouTube-Subscriber-Counter/blob/main/Images/ExplodedView.gif?raw=true" width="650" alt="YouTube Subscriber Counter V2.0 Exploded View">
</div>

---


## Download the 3D Files


[![Printables](https://img.shields.io/badge/Printables-FA6831?style=for-the-badge&logoColor=white)](https://www.printables.com/model/1756251-youtube-subscriber-v20)
[![MakerWorld](https://img.shields.io/badge/MakerWorld-000000?style=for-the-badge&logoColor=white)](https://makerworld.com/en/models/2941691-youtube-subscriber-v2-0#profileId-3294669)


---

### Wiring

Battery Version:

<div align="center">
  <img src="https://raw.githubusercontent.com/ThePrintingPilot/YouTube-Subscriber-Counter/refs/heads/main/Images/Circuit%20Battery.png" width="650" alt="Wiring - Battery Version">
</div>

Regular Version:

<div align="center">
  <img src="https://raw.githubusercontent.com/ThePrintingPilot/YouTube-Subscriber-Counter/refs/heads/main/Images/Circuit%20Regular.png" width="650" alt="Wiring - Regular Version">
</div>

---

## First-Time Setup

### 1. Get a YouTube API key

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a project → Enable **YouTube Data API v3**
3. Go to Credentials → Create API key → copy it

### 2. Find your Channel ID

1. Open [YouTube Studio](https://studio.youtube.com/)
2. Settings → Channel → Advanced settings
3. Copy the Channel ID (starts with `UC...`)

### 3. Flash the firmware

**Option A — Browser installer (recommended)**

No IDE needed. Click the link below, connect your ESP32 via USB, and hit Install:

➡ **[Install via browser](https://theprintingpilot.github.io/YouTube-Subscriber-Counter/)**

<div align="center">
  <img src="https://raw.githubusercontent.com/ThePrintingPilot/YouTube-Subscriber-Counter/refs/heads/main/Images/webinstaller.png" width="250" alt="Config Page">
</div>

Requires Chrome or Edge on desktop.

**Option B — Arduino IDE**

1. Open `YouTube_Sub_Follower_Counter.ino` in Arduino IDE
2. Select your ESP32 board and port
3. Upload



### 4. Configure via the portal

1. Power on the device — the matrix shows `WiFi:YT`
2. On your phone or laptop, connect to the Wi-Fi network **`YouTubeCounter-Setup`**
3. A browser page opens automatically (or navigate to `192.168.4.1`)
4. Tap **Scan for networks**, pick your Wi-Fi, enter your password
5. Enter your YouTube Channel ID and API key
6. Hit **Save & connect** — the device reboots and starts fetching stats


<div align="center">
  <img src="https://raw.githubusercontent.com/ThePrintingPilot/YouTube-Subscriber-Counter/refs/heads/main/Images/webconfig.png" width="250" alt="Config Page">
</div>

---

## After Setup

On every boot the device connects to your saved Wi-Fi and scrolls the assigned IP address across the matrix. Open that IP in any browser on the same network to:

- Change Wi-Fi network or password
- Update YouTube Channel ID or API key
- Upload new firmware (`.bin`) without a USB cable

Leave any field blank when saving and the device keeps the previously stored value — you only need to fill in what you want to change.

---

## OTA Firmware Updates (device already flashed)

1. In Arduino IDE: **Sketch → Export Compiled Binary** — saves a `.bin` file next to your sketch
2. Open the device IP in your browser
3. Scroll to **Firmware update**, pick the main firmware `.bin`, click **Upload firmware**
4. The matrix shows `OTA...` then `Rebooting` — done


---

## Credits

Number formatting code by [The Swedish Maker](https://www.youtube.com/@TheSwedishMaker).

---

## License

MIT
