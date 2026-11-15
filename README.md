# Willary Wildlife Detector
**AI-Powered Human-Wildlife Conflict Deterrent System**
Willary Robotics | info@willaryrobotics.com

---

## Phase 1 — ESP32-CAM + GPT-4o Vision Detection

### Project Structure
```
willary-wildlife-detector/
├── esp32-cam/                   ← PlatformIO project (framework = espidf)
│   ├── platformio.ini           ← board, platform, upload/monitor settings
│   ├── CMakeLists.txt           ← ESP-IDF project file (used by PlatformIO)
│   ├── sdkconfig.defaults       ← PSRAM, flash size, CPU freq
│   └── main/
│       ├── CMakeLists.txt
│       ├── idf_component.yml    ← pulls in espressif/esp32-camera
│       ├── app_config.h         ← EDIT: WiFi, backend URL, pins
│       ├── wildlife_detector.c  ← app_main + trigger loop
│       ├── camera.c / camera_pins.h
│       ├── wifi_sta.c
│       └── http_upload.c
├── backend/
│   ├── server.py                ← Run on your laptop
│   ├── requirements.txt
│   └── test_send_image.py       ← Test without ESP32-CAM
└── README.md
```

---

## STEP 1 — Set up the Python Backend

### Install dependencies
```bash
cd backend
pip install -r requirements.txt
```

### Set your OpenAI API key
```bash
# Windows
set OPENAI_API_KEY=sk-your-key-here

# Linux / Mac
export OPENAI_API_KEY=sk-your-key-here
```

### Find your laptop's local IP address
```bash
# Windows
ipconfig

# Linux
ip addr show
```
Note the IP — e.g. `192.168.1.100`. You need this for the ESP32-CAM.

### Run the backend server
```bash
python server.py
```
Server starts on port 5000. Keep this terminal open.

---

## STEP 2 — Test Backend Without ESP32-CAM

With the server running, open a second terminal:

```bash
# Test with a webcam photo
python test_send_image.py

# Test with an image file
python test_send_image.py elephant.jpg
```

You should see the detection result printed in both terminals.

---

## STEP 3 — Flash the ESP32-CAM (PlatformIO + ESP-IDF)

### Prerequisites
- **VS Code + PlatformIO IDE extension** (or the `pio` CLI: `pip install platformio`).
  PlatformIO downloads the ESP32 toolchain and ESP-IDF itself, so you don't
  need a separate ESP-IDF install.
- ESP32-CAM wired to a USB-serial adapter: `5V`/`3V3`, `GND`, `U0T→RX`, `U0R→TX`,
  and `IO0 → GND` while flashing (boot mode)

> **⚠ No spaces in the project path.** PlatformIO's ESP-IDF builder fails with
> `Detected a whitespace character in project paths`. Clone into a path like
> `E:\projects\willary-wildlife-detector`, or map the folder to a drive letter:
> `subst W: "E:\projects external\willary-wildlife-detector"` and open/build
> from `W:\esp32-cam` (`subst` mappings are lost on reboot).

### Open the project
Open `willary-detector.code-workspace`. The **ESP32-CAM Firmware (PlatformIO)**
folder holds `platformio.ini`, so PlatformIO detects it automatically.

### Configure
Edit `esp32-cam/main/app_config.h`:
```c
#define WIFI_SSID     "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define BACKEND_URL   "http://192.168.1.100:5000/detect"
//                              ↑ your laptop's IP
```
To pin the serial port, uncomment `upload_port` / `monitor_port` in
`esp32-cam/platformio.ini` (e.g. `COM5` or `/dev/ttyUSB0`).

### Build, flash, monitor
From the PlatformIO toolbar use **Build → Upload → Monitor**, or from a terminal:
```bash
cd esp32-cam
pio run                          # first build downloads ESP-IDF + esp32-camera (needs internet)
pio run -t upload                # with IO0 bridged to GND, press RESET first
pio device monitor               # 115200 baud, Ctrl+C to exit
pio run -t menuconfig            # optional: ESP-IDF menuconfig
```
After uploading, remove the `IO0–GND` jumper and press **RESET**. The monitor
prints the WiFi IP, then `ready — waiting for trigger...`.

> PlatformIO stores the generated config as `esp32-cam/sdkconfig.esp32cam`.
> If you change `sdkconfig.defaults`, delete that file (and `.pio/`) so it is
> regenerated.

---

## HOW IT WORKS

```
[Animal appears]
      ↓
[PIR sensor triggers / button pressed]
      ↓
[ESP32-CAM fires flash LED + captures JPEG]
      ↓
[JPEG sent via HTTP POST to laptop backend]
      ↓
[GPT-4o Vision analyses image]
      ↓
[Returns: species, threat level, action, deterrent sound]
      ↓
[Result logged + printed to Serial Monitor]
```

---

## Detection Response Format

```json
{
  "animal_detected": true,
  "species": "elephant",
  "classification": "elephant",
  "is_threat": true,
  "confidence": "high",
  "action": "PLAY_DETERRENT",
  "deterrent_sound": "bee_swarm",
  "description": "Plastic toy elephant figurine, grey, facing left",
  "timestamp": "20250908_143022",
  "model": "gpt-4o-vision"
}
```

---

## Next Phases
- **Phase 2:** Add DFPlayer Mini — play bee_swarm.mp3 when elephant detected
- **Phase 3:** Add SIM800L — send SMS alert with species and timestamp
- **Phase 4:** Add servo radar (HC-SR04 on SG90) for direction detection
- **Phase 5:** Solar power integration
