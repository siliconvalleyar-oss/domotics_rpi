#!/usr/bin/env python3
"""
Domotics RPi Controller - Python alternative

Controla GPIO y tiras WS2812 recibiendo comandos MQTT desde
la app Flutter domotics_app.

Dependencias:
  pip install paho-mqtt rpi_ws281x RPi.GPIO

Uso:
  python domotics_rpi.py
"""

import json
import signal
import sys
import time
import logging
from typing import Optional

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("ERROR: paho-mqtt not installed. Run: pip install paho-mqtt")
    sys.exit(1)

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
MQTT_HOST = "test.mosquitto.org"
MQTT_PORT = 1883
MQTT_CLIENT_ID = "domotics_rpi_py"
MQTT_KEEPALIVE = 60

# GPIO pin mapping (deviceId -> BCM pin number)
GPIO_PIN_MAP = {
    "light_1": 17,
    "light_2": 27,
    "light_3": 22,
    "lock_1": 23,
    "fan_1": 24,
}

# WS2812 config
WS2812_PIN = 18
WS2812_LED_COUNT = 60

# Topics
TOPICS = [
    "domotics/light",
    "domotics/temperature",
    "domotics/fan",
    "domotics/lock",
    "domotics/curtain",
    "domotics/energy",
    "domotics/rgb",
]

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("domotics")


# ---------------------------------------------------------------------------
# GPIO Controller (using RPi.GPIO or mock)
# ---------------------------------------------------------------------------
class GpioController:
    def __init__(self):
        self.gpio_available = False
        self._gpio = None
        self._pins = {}

    def initialize(self):
        try:
            import RPi.GPIO as GPIO
            GPIO.setmode(GPIO.BCM)
            GPIO.setwarnings(False)
            self._gpio = GPIO
            self.gpio_available = True
            log.info("GPIO initialized (RPi.GPIO)")
        except (ImportError, RuntimeError):
            log.warning("RPi.GPIO not available, using mock GPIO")
            self.gpio_available = False

    def register_pin(self, device_id: str, gpio_pin: int):
        self._pins[device_id] = gpio_pin
        if self.gpio_available:
            self._gpio.setup(gpio_pin, self._gpio.OUT, initial=self._gpio.LOW)
        log.info(f"Registered {device_id} -> GPIO{gpio_pin}")

    def set_pin(self, device_id: str, value: bool):
        if device_id not in self._pins:
            log.error(f"Unknown device: {device_id}")
            return
        gpio_pin = self._pins[device_id]
        if self.gpio_available:
            self._gpio.output(gpio_pin, self._gpio.HIGH if value else self._gpio.LOW)
        log.info(f"GPIO {device_id} (pin {gpio_pin}) -> {'ON' if value else 'OFF'}")

    def cleanup(self):
        if self.gpio_available:
            self._gpio.cleanup()
        log.info("GPIO cleaned up")


# ---------------------------------------------------------------------------
# WS2812 Controller (using rpi_ws281x or mock)
# ---------------------------------------------------------------------------
class WS2812Controller:
    def __init__(self, pin: int, led_count: int):
        self.pin = pin
        self.led_count = led_count
        self._strip = None
        self.ws_available = False

    def initialize(self):
        try:
            from rpi_ws281x import PixelStrip, ws
            self._strip = PixelStrip(
                self.led_count, self.pin,
                freq_hz=800000, dma=10, invert=False,
                brightness=128, channel=0, strip_type=ws.WS2811_STRIP_GRB
            )
            self._strip.begin()
            self.ws_available = True
            log.info(f"WS2812 initialized: {self.led_count} LEDs on GPIO{self.pin}")
        except (ImportError, RuntimeError):
            log.warning("rpi_ws281x not available, using mock WS2812")

    def set_all(self, r: int, g: int, b: int):
        r = max(0, min(255, r))
        g = max(0, min(255, g))
        b = max(0, min(255, b))
        if self.ws_available:
            for i in range(self.led_count):
                self._strip.setPixelColorRGB(i, r, g, b)
            self._strip.show()
        log.info(f"WS2812 set all LEDs -> RGB({r},{g},{b})")

    def turn_off(self):
        self.set_all(0, 0, 0)

    def cleanup(self):
        self.turn_off()
        log.info("WS2812 cleaned up")


# ---------------------------------------------------------------------------
# Global instances
# ---------------------------------------------------------------------------
gpio_ctrl = GpioController()
ws2812_ctrl = WS2812Controller(WS2812_PIN, WS2812_LED_COUNT)
mqtt_client: Optional[mqtt.Client] = None
running = True


# ---------------------------------------------------------------------------
# MQTT callbacks
# ---------------------------------------------------------------------------
def on_connect(client, userdata, flags, rc):
    log.info(f"MQTT connected (rc={rc})")
    for topic in TOPICS:
        client.subscribe(topic, qos=1)
        log.info(f"Subscribed to {topic}")


def on_disconnect(client, userdata, rc):
    log.warning(f"MQTT disconnected (rc={rc}), will auto-reconnect")


def publish_status(device_type: str, device_id: str, is_on: bool, value: float):
    topic = f"domotics/{device_type}/status"
    payload = json.dumps({"deviceId": device_id, "isOn": is_on, "value": value})
    mqtt_client.publish(topic, payload, qos=1)
    log.info(f"Published {topic} -> {payload}")


def on_message(client, userdata, msg):
    topic = msg.topic
    try:
        payload = msg.payload.decode("utf-8")
    except UnicodeDecodeError:
        log.error(f"Failed to decode payload on {topic}")
        return

    log.info(f"Received: {topic} -> {payload}")

    # Parse JSON
    try:
        data = json.loads(payload)
    except json.JSONDecodeError:
        log.error(f"Invalid JSON on {topic}")
        return

    # Handle RGB topic
    if topic == "domotics/rgb":
        r = data.get("r", 0)
        g = data.get("g", 0)
        b = data.get("b", 0)
        ws2812_ctrl.set_all(r, g, b)
        publish_status("rgb", "", True, (r + g + b) / 3)
        return

    # Extract deviceType from topic
    device_type = topic.split("/")[-1]  # e.g., "domotics/light" -> "light"

    device_id = data.get("deviceId", "")
    command = data.get("command", "")
    value = data.get("value", 0)

    is_on = (command == "on") or (value > 0 if "value" in data else False)

    log.info(f"Action: {device_id} -> {'ON' if is_on else 'OFF'} (value={value})")

    # GPIO control
    if device_id in GPIO_PIN_MAP:
        gpio_ctrl.set_pin(device_id, is_on)

    # Map light devices to WS2812 pixels
    if device_type == "light" and device_id in ("light_1", "light_2", "light_3"):
        pixel_map = {"light_1": 0, "light_2": 1, "light_3": 2}
        idx = pixel_map[device_id]
        if is_on:
            brightness = int((value / 100.0) * 255)
            ws2812_ctrl._strip.setPixelColorRGB(idx, brightness, brightness, brightness)
        else:
            ws2812_ctrl._strip.setPixelColorRGB(idx, 0, 0, 0)
        if ws2812_ctrl.ws_available:
            ws2812_ctrl._strip.show()

    # Publish status back to app
    if device_type == "lock":
        publish_status(device_type, device_id, is_on, 1.0 if is_on else 0.0)
    else:
        publish_status(device_type, device_id, is_on, float(value))


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    global mqtt_client, running

    signal.signal(signal.SIGINT, lambda s, f: shutdown())
    signal.signal(signal.SIGTERM, lambda s, f: shutdown())

    log.info("=== Domotics RPi Controller (Python) ===")

    # Initialize hardware
    gpio_ctrl.initialize()
    ws2812_ctrl.initialize()

    for device_id, pin in GPIO_PIN_MAP.items():
        gpio_ctrl.register_pin(device_id, pin)

    # MQTT setup
    mqtt_client = mqtt.Client(
        client_id=MQTT_CLIENT_ID,
        protocol=mqtt.MQTTv311,
    )
    mqtt_client.on_connect = on_connect
    mqtt_client.on_disconnect = on_disconnect
    mqtt_client.on_message = on_message
    mqtt_client.connect_async(MQTT_HOST, MQTT_PORT, MQTT_KEEPALIVE)
    mqtt_client.loop_start()

    log.info(f"Connecting to {MQTT_HOST}:{MQTT_PORT}...")

    try:
        while running:
            time.sleep(0.5)
    except KeyboardInterrupt:
        pass
    finally:
        shutdown()


def shutdown():
    global running, mqtt_client
    if not running:
        return
    running = False
    log.info("Shutting down...")
    ws2812_ctrl.cleanup()
    gpio_ctrl.cleanup()
    if mqtt_client:
        mqtt_client.loop_stop()
        mqtt_client.disconnect()
    log.info("Goodbye!")


if __name__ == "__main__":
    main()
