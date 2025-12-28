#ifndef SERVER_HANDLER_H
#define SERVER_HANDLER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

class ServerHandler {
public:
    ServerHandler(int port = 80);
    void setup();
    void begin();
    
    // Callbacks provided by main to handle hardware actions
    void setEmotionCallback(std::function<void(String)> callback);
    void setLookCallback(std::function<void(int, int)> callback);
    void setTextCallback(std::function<void(String, bool)> callback);
    void setBitmapCallback(std::function<void(String)> callback);
    void setToneCallback(std::function<void(int, int)> callback);
    
private:
    AsyncWebServer _server;
    
    std::function<void(String)> _onEmotion;
    std::function<void(int, int)> _onLook;
    std::function<void(String, bool)> _onText;
    std::function<void(String)> _onBitmap;
    std::function<void(int, int)> _onTone;
    
    String _bodyBuffer;

    void setupRoutes();
};

#endif
