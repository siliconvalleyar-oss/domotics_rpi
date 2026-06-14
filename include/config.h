#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>
#include <vector>
#include <cstdint>

// ============================================================
// MQTT Broker Configuration
// ============================================================
const std::string MQTT_HOST = "localhost";
const int MQTT_PORT = 1883;
const std::string MQTT_CLIENT_ID = "domotics_rpi";
const int MQTT_KEEPALIVE = 60;
const int MQTT_RECONNECT_DELAY_SEC = 5;

// ============================================================
// MQTT Topics (must match Flutter app exactly)
// ============================================================
// Command topics (app subscribes, rpi publishes status)
const std::string TOPIC_LIGHT = "domotics/light";
const std::string TOPIC_TEMPERATURE = "domotics/temperature";
const std::string TOPIC_FAN = "domotics/fan";
const std::string TOPIC_LOCK = "domotics/lock";
const std::string TOPIC_CURTAIN = "domotics/curtain";
const std::string TOPIC_ENERGY = "domotics/energy";

// RGB control (extra topic for WS2812 strip)
const std::string TOPIC_RGB = "domotics/rgb";
const std::string TOPIC_RGB_SET = "rgb/set";  // accepts "R,G,B" format

// Status topics (app subscribes to domotics/{type}/status)
const std::string STATUS_LIGHT = "domotics/light/status";
const std::string STATUS_TEMPERATURE = "domotics/temperature/status";
const std::string STATUS_FAN = "domotics/fan/status";
const std::string STATUS_LOCK = "domotics/lock/status";
const std::string STATUS_CURTAIN = "domotics/curtain/status";
const std::string STATUS_ENERGY = "domotics/energy/status";
const std::string STATUS_RGB = "domotics/rgb/status";

// All command topics for subscription
const std::vector<std::string> SUBSCRIPTION_TOPICS = {
    TOPIC_LIGHT, TOPIC_TEMPERATURE, TOPIC_FAN,
    TOPIC_LOCK, TOPIC_CURTAIN, TOPIC_ENERGY,
    TOPIC_RGB,
    TOPIC_RGB_SET
};

// ============================================================
// GPIO Pin Mapping (deviceId -> GPIO pin number)
// ============================================================
const std::map<std::string, unsigned int> GPIO_PIN_MAP = {
    {"light_1", 17},   // Sala - GPIO17
    {"light_2", 27},   // Cocina - GPIO27
    {"light_3", 22},   // Exterior - GPIO22
    {"lock_1",  23},   // Entrada - GPIO23
    {"fan_1",   24},   // Dormitorio - GPIO24
};

// ============================================================
// WS2812 RGB Strip Configuration
// ============================================================
const int WS2812_PIN = 18;          // PWM0 on GPIO18
const int WS2812_LED_COUNT = 60;    // Number of LEDs in the strip
const int WS2812_FREQ_HZ = 800000;  // 800kHz
const int WS2812_DMA_CHANNEL = 10;  // DMA channel 10
const int WS2812_BRIGHTNESS = 128;  // Default brightness (0-255)

// ============================================================
// Device Value Ranges (from Flutter app)
// ============================================================
struct DeviceRange {
    double min;
    double max;
    std::string unit;
};

const std::map<std::string, DeviceRange> DEVICE_RANGES = {
    {"light",      {0,   100, "%"}},
    {"temperature",{16,  30,  "°C"}},
    {"fan",        {0,   5,   ""}},
    {"lock",       {0,   1,   ""}},
    {"curtain",    {0,   100, "%"}},
    {"energy",     {0,   1000,"W"}},
};

// ============================================================
// Device ID to Type mapping
// ============================================================
const std::map<std::string, std::string> DEVICE_TYPE_MAP = {
    {"light_1", "light"},
    {"light_2", "light"},
    {"light_3", "light"},
    {"temp_1",  "temperature"},
    {"fan_1",   "fan"},
    {"lock_1",  "lock"},
    {"curtain_1","curtain"},
    {"energy_1","energy"},
};

#endif // CONFIG_H
