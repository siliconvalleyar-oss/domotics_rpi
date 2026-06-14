#ifndef WS2812_CONTROLLER_H
#define WS2812_CONTROLLER_H

#include <cstdint>
#include <vector>
#include <string>

class WS2812Controller {
public:
    WS2812Controller(int pin, int ledCount);
    ~WS2812Controller();

    bool initialize();
    bool setPixel(int index, uint8_t r, uint8_t g, uint8_t b);
    bool setAll(uint8_t r, uint8_t g, uint8_t b);
    bool setBrightness(uint8_t brightness);
    bool render();
    void clear();
    bool turnOff();

    bool isInitialized() const { return initialized_; }
    int ledCount() const { return ledCount_; }

    // Parse "{\"r\":255,\"g\":0,\"b\":0}" format
    bool setFromJson(const std::string& json);

private:
    int pin_;
    int ledCount_;
    uint8_t brightness_;
    bool initialized_;
    std::vector<uint8_t> buffer_;
};

#endif // WS2812_CONTROLLER_H
