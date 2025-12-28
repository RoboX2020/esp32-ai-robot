#include "eyes.h"

// Screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Eye configuration
#define EYE_RADIUS 18
#define PUPIL_RADIUS 8
#define EYE_SPACING 34 // Distance from center to center of each eye

RobotEyes::RobotEyes(Adafruit_SSD1306* display) {
    _display = display;
    _lastUpdate = 0;
    _nextBlink = 2000;
    _isBlinking = false;
    _currentEmotion = "neutral";
    _targetX = 0;
    _targetY = 0;
    _currentX = 0;
    _currentY = 0;
}

void RobotEyes::setup() {
    // Initial setup if needed
}

void RobotEyes::update() {
    _display->clearDisplay();
    
    updateBlink();
    updateMovement();
    
    // Calculate eye positions based on gaze
    int centerLeftX = SCREEN_WIDTH / 2 - EYE_SPACING / 2;
    int centerRightX = SCREEN_WIDTH / 2 + EYE_SPACING / 2;
    int centerY = SCREEN_HEIGHT / 2;

    int lookOffsetX = _currentX;
    int lookOffsetY = _currentY;

    // Draw Left Eye
    drawEye(centerLeftX + lookOffsetX, centerY + lookOffsetY, EYE_RADIUS, true);
    
    // Draw Right Eye
    drawEye(centerRightX + lookOffsetX, centerY + lookOffsetY, EYE_RADIUS, false);

    _display->display();
}

void RobotEyes::updateMovement() {
    // Smooth movement towards target
    if (_currentX < _targetX) _currentX++;
    if (_currentX > _targetX) _currentX--;
    if (_currentY < _targetY) _currentY++;
    if (_currentY > _targetY) _currentY--;

    // Random saccades if neutral
    if (millis() > _lastUpdate + 2000) {
        _lastUpdate = millis();
        if (random(0, 10) > 7) {
            lookAt(random(-10, 10), random(-10, 10));
        } else {
            lookAt(0, 0);
        }
    }
}

void RobotEyes::updateBlink() {
    if (!_isBlinking && millis() > _nextBlink) {
        _isBlinking = true;
        _blinkStartTime = millis();
    }

    if (_isBlinking && millis() - _blinkStartTime > 150) { // Blink duration
        _isBlinking = false;
        _nextBlink = millis() + random(2000, 6000);
    }
}

void RobotEyes::lookAt(int x, int y) {
    _targetX = x;
    _targetY = y;
}

void RobotEyes::setEmotion(String emotion) {
    _currentEmotion = emotion;
}

void RobotEyes::drawEye(int x, int y, int r, bool isLeft) {
    if (_isBlinking) {
        // Draw a line for closed eye
        _display->drawFastHLine(x - r, y, r * 2, SSD1306_WHITE);
        return;
    }

    if (_currentEmotion == "happy") {
        // Happy: Inverted U (Arc)
        int h = r / 2;
        _display->fillCircle(x, y, r, SSD1306_WHITE);
        _display->fillCircle(x, y + h, r, SSD1306_BLACK); // cutout bottom
        return;
    }
    
    if (_currentEmotion == "sad") {
        // Sad: U shape (fill circle, cutout top)
        int h = r / 2;
        _display->fillCircle(x, y, r, SSD1306_WHITE);
        _display->fillCircle(x, y - h, r, SSD1306_BLACK); // cutout top
        return;
    }

    if (_currentEmotion == "angry") {
        // Angry: Top cutoff line slanted? 
        // Simple: Top half flat cut.
        _display->fillCircle(x, y, r, SSD1306_WHITE);
        _display->fillTriangle(x-r, y-r, x+r, y-r, x, y, SSD1306_BLACK);
        _display->fillCircle(x, y, PUPIL_RADIUS, SSD1306_BLACK);
        return;
    }

    if (_currentEmotion == "spiderman") {
        // Spider-Man Mask Eye: Teardrop / Triangle shape
        // We simulate this with 3 triangles or a custom polygon logic if GFX supported it well.
        // Let's draw a white filled triangle + circle combo or just a large filled triangle.
        
        // Dynamic Squinting?
        int squint = 0;
        if (millis() % 2000 < 1000) squint = 5; // Breathe effect
        
        // Draw main white shape (Triangle-ish)
        // Center x,y. 
        if (isLeft) {
            _display->fillTriangle(x + r, y - r + squint, x - r, y, x + r, y + r - squint, SSD1306_WHITE);
        } else {
            _display->fillTriangle(x - r, y - r + squint, x + r, y, x - r, y + r - squint, SSD1306_WHITE);
        }
        // No pupil for spidey graphics usually
        return;
    }

    // Normal Eye (Neutral / Default)
    _display->fillCircle(x, y, r, SSD1306_WHITE);
    _display->fillCircle(x, y, PUPIL_RADIUS, SSD1306_BLACK);
    _display->fillCircle(x - 4, y - 4, 3, SSD1306_WHITE); // Glint
}
