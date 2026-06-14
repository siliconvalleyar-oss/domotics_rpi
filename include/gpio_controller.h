#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#include <gpiod.h>
#include <string>
#include <map>
#include <memory>

struct GpioLineDeleter {
    void operator()(struct gpiod_line* line) const {
        if (line) {
            gpiod_line_release(line);
        }
    }
};

using GpioLinePtr = std::unique_ptr<struct gpiod_line, GpioLineDeleter>;

class GpioController {
public:
    GpioController();
    ~GpioController();

    bool initialize();
    bool registerPin(const std::string& deviceId, unsigned int gpioPin);
    bool setPin(const std::string& deviceId, bool value);
    bool getPin(const std::string& deviceId) const;
    void cleanup();

private:
    struct gpiod_chip* chip_;
    std::map<std::string, unsigned int> pinMap_;
    std::map<std::string, bool> pinState_;
    bool initialized_;
};

#endif // GPIO_CONTROLLER_H
