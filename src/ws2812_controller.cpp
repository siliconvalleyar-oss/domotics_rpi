#include "ws2812_controller.h"
#include "logger.h"
#include <cstdlib>

/*
 * On a real Raspberry Pi with rpi_ws281x library linked:
 *   #include <ws2811.h>
 *   ws2811_t ledstring = ...;
 *   ws2811_init(&ledstring);
 *   ws2811_render(&ledstring);
 *
 * Compile with: -lws2811
 */

WS2812Controller::WS2812Controller(int pin, int ledCount)
    : pin_(pin)
    , ledCount_(ledCount)
    , brightness_(128)
    , initialized_(false)
    , buffer_(ledCount * 3, 0)
{
}

WS2812Controller::~WS2812Controller() {
    clear();
    render();
    Logger::info("WS2812", "Destroyed");
}

bool WS2812Controller::initialize() {
    if (ledCount_ <= 0 || ledCount_ > 1000) {
        Logger::error("WS2812", "Invalid LED count: " + std::to_string(ledCount_));
        return false;
    }

    buffer_.assign(ledCount_ * 3, 0);
    initialized_ = true;
    Logger::info("WS2812", "Initialized: " + std::to_string(ledCount_)
                 + " LEDs on GPIO" + std::to_string(pin_)
                 + ", brightness=" + std::to_string(brightness_));
    return true;
}

bool WS2812Controller::setPixel(int index, uint8_t r, uint8_t g, uint8_t b) {
    if (!initialized_ || index < 0 || index >= ledCount_) return false;

    uint8_t br = (r * brightness_) / 255;
    uint8_t bg = (g * brightness_) / 255;
    uint8_t bb = (b * brightness_) / 255;

    // WS2812 uses GRB format
    buffer_[index * 3 + 0] = bg;
    buffer_[index * 3 + 1] = br;
    buffer_[index * 3 + 2] = bb;

    return true;
}

bool WS2812Controller::setAll(uint8_t r, uint8_t g, uint8_t b) {
    if (!initialized_) return false;
    for (int i = 0; i < ledCount_; ++i) {
        setPixel(i, r, g, b);
    }
    return true;
}

bool WS2812Controller::setBrightness(uint8_t brightness) {
    brightness_ = brightness;
    return true;
}

bool WS2812Controller::render() {
    if (!initialized_) return false;
    Logger::debug("WS2812", "Render: " + std::to_string(ledCount_) + " LEDs updated");
    return true;
}

void WS2812Controller::clear() {
    buffer_.assign(ledCount_ * 3, 0);
}

bool WS2812Controller::turnOff() {
    clear();
    return render();
}

bool WS2812Controller::setFromJson(const std::string& json) {
    int r = 0, g = 0, b = 0;

    auto findInt = [&](const std::string& key) -> int {
        auto pos = json.find(key);
        if (pos == std::string::npos) return -999;
        pos = json.find(':', pos);
        if (pos == std::string::npos) return -999;
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        if (pos >= json.size()) return -999;
        bool neg = false;
        if (json[pos] == '-') { neg = true; pos++; }
        int val = 0;
        while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') {
            val = val * 10 + (json[pos] - '0');
            pos++;
        }
        return neg ? -val : val;
    };

    r = findInt("\"r\"");
    g = findInt("\"g\"");
    b = findInt("\"b\"");

    if (r == -999 || g == -999 || b == -999) {
        Logger::error("WS2812", "Invalid JSON: " + json);
        return false;
    }

    r = (r < 0) ? 0 : (r > 255) ? 255 : r;
    g = (g < 0) ? 0 : (g > 255) ? 255 : g;
    b = (b < 0) ? 0 : (b > 255) ? 255 : b;

    setAll(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
    return render();
}
