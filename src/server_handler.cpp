// ============================================================================
// server_handler.cpp - a tiny web server running on the robot itself.
// It serves the phone UI (the HTML/JS/CSS files stored in LittleFS flash)
// and answers API calls like /emotion or /look by invoking callbacks that
// main.cpp registered (setEmotionCallback, setLookCallback) - so the web
// page can change the robot's face over Wi-Fi.
// ============================================================================
#include "server_handler.h"
#include <LittleFS.h>

ServerHandler::ServerHandler(int port) : _server(port) {}

void ServerHandler::setEmotionCallback(std::function<void(String)> callback) {
    _onEmotion = callback;
}

void ServerHandler::setLookCallback(std::function<void(int, int)> callback) {
    _onLook = callback;
}

void ServerHandler::setTextCallback(std::function<void(String, bool)> callback) {
    _onText = callback;
}

void ServerHandler::setBitmapCallback(std::function<void(String)> callback) {
    _onBitmap = callback;
}

void ServerHandler::setToneCallback(std::function<void(int, int)> callback) {
    _onTone = callback;
}

void ServerHandler::setup() {
    // Mount LittleFS
    if (!LittleFS.begin(true)) { // Pass true to format if failed, though we prefer upload
        Serial.println("LittleFS Mount Failed");
        return;
    }
    
    Serial.println("LittleFS Mounted");
    // Debug: List files
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while(file){
        Serial.print("FILE: ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
    
    setupRoutes();
}

void ServerHandler::begin() {
    _server.begin();
    Serial.println("Server started");
}

void ServerHandler::setupRoutes() {
    // Serve static files
    _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // API: Set Emotion
    _server.on("/api/eyes/emotion", HTTP_POST, [this](AsyncWebServerRequest *request) {}, NULL, 
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, (const char*)data);
            String emotion = doc["emotion"].as<String>();
            if (_onEmotion) _onEmotion(emotion);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // API: Set Look Direction
    _server.on("/api/eyes/look", HTTP_POST, [this](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, (const char*)data);
            int x = doc["x"];
            int y = doc["y"];
            if (_onLook) _onLook(x, y);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // API: Display Text
    _server.on("/api/display/text", HTTP_POST, [this](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(2048);
            deserializeJson(doc, (const char*)data);
            String text = doc["text"].as<String>();
            bool persistent = doc["persistent"] | false; 
            if (_onText) _onText(text, persistent);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // API: Display Image (Bitmap String)
    _server.on("/api/display/image", HTTP_POST, [this](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
             // Handle Chunked Data
             if (index == 0) {
                 _bodyBuffer = "";
             }
             
             // Append chunk
             for(size_t i=0; i<len; i++) {
                 _bodyBuffer += (char)data[i];
             }
             
             // Check if complete
             if (index + len == total) {
                 DynamicJsonDocument doc(20000); // 8192 chars + JSON overhead ~ 9-10KB
                 DeserializationError error = deserializeJson(doc, _bodyBuffer);
                 
                 if (!error) {
                     String bitmap = doc["bitmap"].as<String>();
                     if (_onBitmap) _onBitmap(bitmap);
                     request->send(200, "application/json", "{\"status\":\"ok\"}");
                 } else {
                     Serial.print("JSON Error: "); Serial.println(error.c_str());
                     request->send(400, "application/json", "{\"status\":\"error\", \"msg\":\"JSON Parse Fail\"}");
                 }
                 _bodyBuffer = ""; // Clear memory
             }
    });

    // API: Play Tone
    _server.on("/api/sound/tone", HTTP_POST, [this](AsyncWebServerRequest *request) {}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, (const char*)data);
            int freq = doc["freq"];
            int duration = doc["duration"];
            if (_onTone) _onTone(freq, duration);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    // CORS
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    // 404 Handler - helpful for debugging if file is missing
    _server.onNotFound([](AsyncWebServerRequest *request){
        Serial.printf("NOT_FOUND: %s\n", request->url().c_str());
        request->send(404, "text/plain", "404: Not Found. Did you upload the filesystem? Check Serial output.");
    });
}