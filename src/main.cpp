// ============================================================================
// main.cpp - the robot's brain (ESP32-C3 firmware). This little board has:
//   - an OLED screen that shows animated EYES (eyes.cpp), text, images or
//     the time,
//   - a Wi-Fi web server (server_handler.cpp) so a phone browser can send
//     it things to show or say (the web UI in data/),
//   - a buzzer for beeps.
// Loop: check for web commands, then animate whatever mode the display is
// in. Temporary modes (text/image/time) auto-revert to eyes after 30s.
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "eyes.h"
#include "server_handler.h"

// --- CONFIGURATION ---
// --- CONFIGURATION ---
const char* SSID = "YOUR_WIFI_SSID";
const char* PASSWORD = "YOUR_WIFI_PASSWORD";

// I2C Pins for ESP32-C3 Super Mini (User requested 5 & 6)
#define I2C_SDA 5
#define I2C_SCL 6
#define BUZZER_PIN 3

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
// The driver object for the little OLED screen, reached over the I2C bus (Wire).
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

RobotEyes eyes(&display);
ServerHandler server(80);

// The four things the screen can be showing at any moment.
enum SystemMode {
    MODE_EYES,
    MODE_TEXT,
    MODE_IMAGE,
    MODE_DATETIME
};
SystemMode currentMode = MODE_EYES;
String currentText = "";
bool isPersistent = false; // Add global persistent flag
String currentBitmap = ""; 
// millis() is the board's millisecond stopwatch since power-on; we use it
// to remember when a temporary screen (text/image/time) started.
unsigned long modeStartTime = 0;
// 30,000 ms = 30 seconds: how long a temporary screen stays before the
// eyes come back (unless it was pinned as 'persistent').
const unsigned long AUTO_REVERT_TIME = 30000;

// Date/Time
String currentDate = "";
String currentTime = "";

void setup() {
    Serial.begin(115200);
    // Start the I2C bus (the two-wire link that carries commands to the OLED).
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Init Buzzer
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW); // Off

    // Initialize OLED
    // Wake the OLED. If it doesn't answer, freeze forever (for(;;)) - a robot
    // without its face has nothing useful to do, so stop loudly.
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Splash
    display.setCursor(0, 10);
    display.println("ESP32 AI Robot");
    display.println("Booting...");
    display.display();
    delay(1000);

    // Connect to WiFi
    // Start joining the Wi-Fi network named in the config above.
    WiFi.begin(SSID, PASSWORD);
    int dots = 0;
    // Wait for the connection (half a second per try); give up after ~20s.
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        dots++;
        if(dots > 40) { // 20 seconds timeout
            display.clearDisplay();
            display.setCursor(0,10);
            display.println("WiFi Failed!");
            display.println("Check SSID/Pass");
            display.display();
            delay(5000);
            // Optionally Retry or Continue
            break; 
        }
    }
    
    if(WiFi.status() == WL_CONNECTED) {
        display.clearDisplay();
        display.setCursor(0, 10);
        display.println("Connected!");
        display.print("IP: ");
        display.println(WiFi.localIP());
        display.display();
    } else {
        // If failed, show AP mode or just error
        display.clearDisplay();
        display.setCursor(0, 10);
        display.println("Connection Failed.");
        display.println("Retrying...");
        display.display();
        // Maybe restart or use AP? For now just loop in failure/eyes
    }
    
    // Callbacks
    // These 'callbacks' tell the web server what to do when the phone sends a
    // command: change emotion, look somewhere, show text, show an image, beep.
    server.setEmotionCallback([](String emotion){
        isPersistent = false; // Reset persistence on emotion
        currentMode = MODE_EYES;
        eyes.setEmotion(emotion);
    });
    
    server.setLookCallback([](int x, int y){
        isPersistent = false; // Reset persistence on look
        currentMode = MODE_EYES;
        eyes.lookAt(x, y);
    });
    
    server.setTextCallback([](String text, bool persistent){
        // Text starting with CMD_TIME: is not a message to display - it is the
        // phone pushing the current clock time, so switch to the clock face.
        if(text.startsWith("CMD_TIME:")) {
             // CMD_TIME:12:30 PM|2023-10-25
             int split = text.indexOf('|');
             if(split > 0) {
                 currentTime = text.substring(9, split);
                 currentDate = text.substring(split+1);
                 currentMode = MODE_DATETIME;
                 modeStartTime = millis(); 
                 isPersistent = false;
             }
        } else {
            currentMode = MODE_TEXT;
            currentText = text;
            isPersistent = persistent; // Set global flag
            modeStartTime = millis();
        }
    });
    
    server.setBitmapCallback([](String bitmap){
        isPersistent = false; // Reset persistence
        currentMode = MODE_IMAGE;
        currentBitmap = bitmap;
        modeStartTime = millis();
    });
    
    server.setToneCallback([](int freq, int duration){
        // Simple blocking tone for now, or use non-blocking if needed.
        // For games, short beeps are fine blocking (e.g. 50-100ms).
        if(freq > 0) {
            tone(BUZZER_PIN, freq, duration);
        } else {
            noTone(BUZZER_PIN);
        }
    });

    server.setup();
    server.begin();
    delay(2000);
}

void loop() {
    if (currentMode == MODE_TEXT) {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextWrap(true);
        
        // Pick a font size that fits: short messages get giant letters, long ones small.
        int len = currentText.length();
        // Auto-Calibration
        if (len <= 10) {
            display.setTextSize(3);
            display.setCursor(0, 20); // Center V roughly
        } else if (len <= 40) {
            display.setTextSize(2);
             display.setCursor(0, 0);
        } else {
            display.setTextSize(1);
             display.setCursor(0, 0);
        }
        
        display.println(currentText);
        display.display();
    } 
    else if (currentMode == MODE_IMAGE) {
        display.clearDisplay();
        int len = currentBitmap.length();
        int idx = 0;
        // The image arrives as a string of '0'/'1' characters (128x64 pixels,
        // row by row); walk it and light a pixel for every '1'.
        for(int y=0; y<64; y++) {
            for(int x=0; x<128; x++) {
                if(idx < len) {
                    if(currentBitmap[idx] == '1') display.drawPixel(x, y, SSD1306_WHITE);
                    idx++;
                }
            }
        }
        display.display();
    }
    else if (currentMode == MODE_DATETIME) {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        
        // Draw Border
        display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
        
        // Time
        display.setTextSize(3);
        int16_t x1, y1;
        uint16_t w, h;
        // Measure how wide the text will be, so we can center it on the screen.
        display.getTextBounds(currentTime, 0, 0, &x1, &y1, &w, &h);
        display.setCursor((128 - w) / 2, 10);
        display.println(currentTime);
        
        // Date
        display.setTextSize(1);
        display.getTextBounds(currentDate, 0, 0, &x1, &y1, &w, &h);
        display.setCursor((128 - w) / 2, 45);
        display.println(currentDate);
        
        display.display();
    }
    else {
        // No special screen active? Animate the eyes (blink, wander, emote).
        eyes.update();
    }
    
    // Only revert if NOT persistent
    // If a temporary screen has been up for 30s and wasn't pinned, go back to eyes.
    if (!isPersistent && currentMode != MODE_EYES && millis() - modeStartTime > AUTO_REVERT_TIME) {
        currentMode = MODE_EYES;
    }
}
