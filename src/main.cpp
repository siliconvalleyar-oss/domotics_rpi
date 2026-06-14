#include "config.h"
#include "logger.h"
#include "mqtt_handler.h"
#include "gpio_controller.h"
#include "ws2812_controller.h"

#include <iostream>
#include <string>
#include <csignal>
#include <atomic>
#include <sstream>
#include <algorithm>

#define MODULE "MAIN"

std::atomic<bool> g_running(true);
MqttHandler* g_mqtt = nullptr;
GpioController* g_gpio = nullptr;
WS2812Controller* g_ws2812 = nullptr;

void signalHandler(int sig) {
    (void)sig;
    Logger::info(MODULE, "Signal received, shutting down...");
    g_running = false;
}

void processMessage(const std::string& topic, const std::string& payload);

void publishStatus(const std::string& deviceType, const std::string& deviceId, bool isOn, double value) {
    if (!g_mqtt) return;

    std::ostringstream statusTopic;
    statusTopic << "domotics/" << deviceType << "/status";

    std::ostringstream payload;
    payload << "{\"deviceId\":\"" << deviceId
            << "\",\"isOn\":" << (isOn ? "true" : "false")
            << ",\"value\":" << value << "}";

    g_mqtt->publish(statusTopic.str(), payload.str());
    Logger::debug(MODULE, "Published status: " + statusTopic.str() + " -> " + payload.str());
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Logger::init("logs", "domotics_rpi.log");

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    Logger::info(MODULE, "=== Domotics RPi Controller ===");
    Logger::info(MODULE, "MQTT Broker: " + MQTT_HOST + ":" + std::to_string(MQTT_PORT));

    GpioController gpio;
    g_gpio = &gpio;

    if (!gpio.initialize()) {
        Logger::warn(MODULE, "GPIO init failed, continuing without GPIO");
    } else {
        for (const auto& [deviceId, gpioPin] : GPIO_PIN_MAP) {
            gpio.registerPin(deviceId, gpioPin);
        }
    }

    WS2812Controller ws2812(WS2812_PIN, WS2812_LED_COUNT);
    g_ws2812 = &ws2812;

    if (!ws2812.initialize()) {
        Logger::warn(MODULE, "WS2812 init failed, continuing without RGB");
    }

    MqttHandler mqtt(MQTT_HOST, MQTT_PORT, MQTT_CLIENT_ID);
    g_mqtt = &mqtt;

    mqtt.setMessageCallback(processMessage);

    if (!mqtt.connect(MQTT_KEEPALIVE)) {
        Logger::warn(MODULE, "MQTT connection failed, will retry in background");
    }

    mqtt.loopStart();

    for (const auto& topic : SUBSCRIPTION_TOPICS) {
        mqtt.subscribe(topic);
    }

    // Publish initial state for all known devices (sync on connect)
    for (const auto& [deviceId, gpioPin] : GPIO_PIN_MAP) {
        (void)gpioPin;
        auto typeIt = DEVICE_TYPE_MAP.find(deviceId);
        if (typeIt != DEVICE_TYPE_MAP.end()) {
            publishStatus(typeIt->second, deviceId, false, 0.0);
        }
    }

    Logger::info(MODULE, "Ready. Waiting for commands...");

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Logger::info(MODULE, "Cleaning up...");
    ws2812.turnOff();
    mqtt.loopStop();
    mqtt.disconnect();
    gpio.cleanup();

    Logger::shutdown();
    return 0;
}

void processMessage(const std::string& topic, const std::string& payload) {
    Logger::info(MODULE, "Received: " + topic + " -> " + payload);

    if (topic == TOPIC_RGB) {
        if (g_ws2812) {
            g_ws2812->setFromJson(payload);
        }
        return;
    }

    if (topic == TOPIC_RGB_SET) {
        if (g_ws2812) {
            try {
                std::istringstream iss(payload);
                std::string token;
                int rgb[3] = {0, 0, 0};
                int i = 0;
                while (std::getline(iss, token, ',') && i < 3) {
                    rgb[i++] = std::stoi(token);
                }
                uint8_t r = static_cast<uint8_t>(std::clamp(rgb[0], 0, 255));
                uint8_t g = static_cast<uint8_t>(std::clamp(rgb[1], 0, 255));
                uint8_t b = static_cast<uint8_t>(std::clamp(rgb[2], 0, 255));
                g_ws2812->setAll(r, g, b);
                g_ws2812->render();
                Logger::info(MODULE, "RGB set: " + std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b));
            } catch (const std::exception& e) {
                Logger::error(MODULE, "Invalid RGB payload: " + payload + " - " + e.what());
            }
        }
        return;
    }

    std::string deviceType;
    if (topic == TOPIC_LIGHT)            deviceType = "light";
    else if (topic == TOPIC_TEMPERATURE) deviceType = "temperature";
    else if (topic == TOPIC_FAN)         deviceType = "fan";
    else if (topic == TOPIC_LOCK)        deviceType = "lock";
    else if (topic == TOPIC_CURTAIN)     deviceType = "curtain";
    else if (topic == TOPIC_ENERGY)      deviceType = "energy";
    else {
        Logger::warn(MODULE, "Unknown topic: " + topic);
        return;
    }

    auto deviceIdPos = payload.find("\"deviceId\"");
    if (deviceIdPos == std::string::npos) {
        Logger::error(MODULE, "Missing deviceId in payload");
        return;
    }

    auto colonPos = payload.find(':', deviceIdPos);
    if (colonPos == std::string::npos) return;
    auto quote1 = payload.find('"', colonPos);
    if (quote1 == std::string::npos) return;
    auto quote2 = payload.find('"', quote1 + 1);
    if (quote2 == std::string::npos) return;

    std::string deviceId = payload.substr(quote1 + 1, quote2 - quote1 - 1);
    Logger::debug(MODULE, "Device ID: " + deviceId);

    bool isGpioDevice = (GPIO_PIN_MAP.find(deviceId) != GPIO_PIN_MAP.end());
    bool isLight = (deviceType == "light");

    bool hasCommand = (payload.find("\"command\"") != std::string::npos);
    bool hasValue = (payload.find("\"value\"") != std::string::npos);

    bool isOn = false;
    double value = 0.0;

    if (hasCommand) {
        isOn = (payload.find("\"on\"") != std::string::npos);
    }

    if (hasValue) {
        auto valColon = payload.find(':', payload.find("\"value\""));
        if (valColon != std::string::npos) {
            valColon++;
            while (valColon < payload.size() &&
                   (payload[valColon] == ' ' || payload[valColon] == '\t')) {
                valColon++;
            }
            std::string numStr;
            while (valColon < payload.size() &&
                   (isdigit(payload[valColon]) || payload[valColon] == '.' ||
                    payload[valColon] == '-')) {
                numStr += payload[valColon++];
            }
            if (!numStr.empty()) {
                value = std::stod(numStr);
            }
        }
        if (!hasCommand) {
            isOn = (value > 0);
        }
    }

    Logger::info(MODULE, "Action: " + deviceId
                 + " -> " + (isOn ? "ON" : "OFF")
                 + " (value=" + std::to_string(value) + ")");

    if (isGpioDevice && g_gpio) {
        g_gpio->setPin(deviceId, isOn);
    }

    if (isLight && g_ws2812) {
        int pixelIndex = -1;
        if (deviceId == "light_1") pixelIndex = 0;
        else if (deviceId == "light_2") pixelIndex = 1;
        else if (deviceId == "light_3") pixelIndex = 2;

        if (pixelIndex >= 0 && pixelIndex < g_ws2812->ledCount()) {
            if (isOn) {
                uint8_t brightness = static_cast<uint8_t>((value / 100.0) * 255);
                g_ws2812->setPixel(pixelIndex, brightness, brightness, brightness);
            } else {
                g_ws2812->setPixel(pixelIndex, 0, 0, 0);
            }
            g_ws2812->render();
        }
    }

    double statusValue = value;
    if (deviceType == "lock") {
        statusValue = isOn ? 1.0 : 0.0;
    }
    publishStatus(deviceType, deviceId, isOn, statusValue);
}
