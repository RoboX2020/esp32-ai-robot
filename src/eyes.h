#ifndef EYES_H
#define EYES_H

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

class RobotEyes {
public:
    RobotEyes(Adafruit_SSD1306* display);
    void setup();
    void update();
    void setEmotion(String emotion); // "neutral", "happy", "cockeyed", etc.
    void lookAt(int x, int y);

private:
    Adafruit_SSD1306* _display;
    unsigned long _lastUpdate;
    unsigned long _nextBlink;
    bool _isBlinking;
    unsigned long _blinkStartTime;
    
    // Eye state
    int _leftEyeX, _leftEyeY;
    int _rightEyeX, _rightEyeY;
    int _targetX, _targetY; // Where we want to look (relative to screen center 0,0)
    int _currentX, _currentY;
    
    String _currentEmotion;

    void drawEye(int x, int y, int size, bool isLeft);
    void updateMovement();
    void updateBlink();
};

#endif
