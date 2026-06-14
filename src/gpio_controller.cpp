#include "gpio_controller.h"
#include "logger.h"
#include <cstring>

GpioController::GpioController()
    : chip_(nullptr)
    , initialized_(false)
{
}

GpioController::~GpioController() {
    cleanup();
}

bool GpioController::initialize() {
    chip_ = gpiod_chip_open_by_number(0);
    if (!chip_) {
        Logger::error("GPIO", "Failed to open gpiochip0: " + std::string(strerror(errno)));
        return false;
    }
    initialized_ = true;
    Logger::info("GPIO", "Initialized");
    return true;
}

bool GpioController::registerPin(const std::string& deviceId, unsigned int gpioPin) {
    if (!chip_) return false;

    struct gpiod_line* line = gpiod_chip_get_line(chip_, gpioPin);
    if (!line) {
        Logger::error("GPIO", "Failed to get line " + std::to_string(gpioPin) + ": "
                      + std::string(strerror(errno)));
        return false;
    }

    int ret = gpiod_line_request_output(line, "domotics_rpi", 0);
    if (ret != 0) {
        Logger::error("GPIO", "Failed to set line " + std::to_string(gpioPin)
                      + " as output: " + std::string(strerror(errno)));
        gpiod_line_release(line);
        return false;
    }

    pinMap_[deviceId] = gpioPin;
    pinState_[deviceId] = false;
    gpiod_line_release(line);
    Logger::info("GPIO", "Registered " + deviceId + " -> GPIO" + std::to_string(gpioPin));
    return true;
}

bool GpioController::setPin(const std::string& deviceId, bool value) {
    if (!chip_) return false;

    auto it = pinMap_.find(deviceId);
    if (it == pinMap_.end()) {
        Logger::error("GPIO", "Unknown device: " + deviceId);
        return false;
    }

    struct gpiod_line* line = gpiod_chip_get_line(chip_, it->second);
    if (!line) {
        Logger::error("GPIO", "Failed to get line " + std::to_string(it->second));
        return false;
    }

    int ret = gpiod_line_request_output(line, "domotics_rpi", value ? 1 : 0);
    if (ret != 0) {
        Logger::error("GPIO", "Failed to set " + deviceId + " = " + (value ? "1" : "0"));
        gpiod_line_release(line);
        return false;
    }

    gpiod_line_set_value(line, value ? 1 : 0);
    pinState_[deviceId] = value;
    gpiod_line_release(line);

    Logger::info("GPIO", deviceId + " -> " + (value ? "ON" : "OFF"));
    return true;
}

bool GpioController::getPin(const std::string& deviceId) const {
    auto it = pinState_.find(deviceId);
    if (it == pinState_.end()) return false;
    return it->second;
}

void GpioController::cleanup() {
    if (chip_) {
        gpiod_chip_close(chip_);
        chip_ = nullptr;
    }
    pinMap_.clear();
    pinState_.clear();
    initialized_ = false;
    Logger::info("GPIO", "Cleaned up");
}
