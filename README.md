# ESP32 AI Robot 🤖✨

A retro-futuristic AI robot powered by **ESP32-C3 Super Mini**, **OpenAI GPT-4o**, and **WebSockets**. Features a 128x64 OLED display for eyes/text/animations, a piezo buzzer for sound, and a completely locally-hosted web controller with Speech-to-Text.

## Features 🚀

- **👀 Tiny Eyes Animation**: Expressive eyes that blink, look around, and react to emotions (Happy, Sad, Angry, Spider-Man).
- **🗣️ Voice Conversation**: Talk to the robot via the web interface.
- **🎮 Generative Games**: Ask the AI to *"Create a Pong game"* or *"Make a Flappy Bird game"*, and it will generate the code and stream the game to your OLED!
- **🎹 Generative Music**: Ask for *"Mario Theme"* or *"Imperial March"* to hear 8-bit tunes.
- **💬 Modes**:
  - **Normal**: Eyes + Animations/Games.
  - **Chat Mode**: Text-only display (Q&A) for reading long responses.
  - **Caption Mode**: Rapid Serial Visual Presentation (RSVP) - stream text one word at a time.
- **📸 Image Upload**: Upload images to display on the OLED.

## Hardware Required 🛠️

- **ESP32-C3 Super Mini** (or similar ESP32 board).
- **0.96" SSD1306 OLED Display** (I2C).
- **Active/Passive Buzzer**.
- **Wiring**:
    - **OLED SDA** -> GPIO 5
    - **OLED SCL** -> GPIO 6
    - **Buzzer** -> GPIO 3
    - **VCC/GND** accordingly.

## Installation 📦

### 1. Firmware (PlatformIO)
1.  Install **VS Code** and **PlatformIO** extension.
2.  Clone this repository.
3.  Open `src/main.cpp` and edit:
    ```cpp
    const char* SSID = "YOUR_WIFI_SSID";
    const char* PASSWORD = "YOUR_WIFI_PASSWORD";
    ```
4.  Connect ESP32 via USB.
5.  **Build** and **Upload** firmware.

### 2. Filesystem (Web App)
1.  In PlatformIO, run task: **`PlatformIO: Upload Filesystem Image`**.
    - This uploads the `data/` folder (HTML/JS/CSS) to the ESP32's LittleFS.

### 3. Setup
1.  Check Serial Monitor for the IP Address (e.g., `192.168.1.50`).
2.  Open that IP in your Browser (Phone or PC).
3.  Click **CFG** (Config) button.
4.  Enter your **OpenAI API Key** and click **SAVE**.
5.  Start talking!

## Usage 🕹️

- **Talk**: Click **MIC** and speak.
- **Games**: Say *"Create a bouncing ball game"*.
- **Music**: Say *"Play a happy tune"*.
- **Modes**: Use the toggles to switch between Normal, Chat, and Caption modes.

## Troubleshooting ⚠️

- **Microphone Error?**: Browsers block microphones on non-HTTPS sites.
    - **Fix**: Go to `chrome://flags/#unsafely-treat-insecure-origin-as-secure`, enable it, add your ESP32 IP (`http://192.168.x.x`), and relaunch Chrome.
