#include "mqtt_handler.h"
#include "logger.h"
#include <chrono>
#include <cstring>

MqttHandler::MqttHandler(const std::string& host, int port, const std::string& clientId)
    : mosq_(nullptr)
    , host_(host)
    , port_(port)
    , clientId_(clientId)
    , keepalive_(60)
    , connected_(false)
    , running_(false)
{
    mosquitto_lib_init();
    mosq_ = mosquitto_new(clientId_.c_str(), true, this);
    if (!mosq_) {
        Logger::error("MQTT", "Failed to create mosquitto instance");
        return;
    }

    mosquitto_connect_callback_set(mosq_, onConnectCallback);
    mosquitto_disconnect_callback_set(mosq_, onDisconnectCallback);
    mosquitto_message_callback_set(mosq_, onMessageCallback);
}

MqttHandler::~MqttHandler() {
    loopStop();
    disconnect();
    if (mosq_) {
        mosquitto_destroy(mosq_);
    }
    mosquitto_lib_cleanup();
}

bool MqttHandler::connect(int keepalive) {
    if (!mosq_) return false;
    keepalive_ = keepalive;

    int rc = mosquitto_connect(mosq_, host_.c_str(), port_, keepalive_);
    if (rc != MOSQ_ERR_SUCCESS) {
        Logger::error("MQTT", "Connection failed: " + std::string(mosquitto_strerror(rc)));
        return false;
    }

    connected_ = true;
    Logger::info("MQTT", "Connected to " + host_ + ":" + std::to_string(port_));
    return true;
}

void MqttHandler::disconnect() {
    if (mosq_ && connected_) {
        mosquitto_disconnect(mosq_);
        connected_ = false;
        Logger::info("MQTT", "Disconnected");
    }
}

bool MqttHandler::isConnected() const {
    return connected_.load();
}

bool MqttHandler::subscribe(const std::string& topic, int qos) {
    if (!mosq_ || !connected_) return false;
    int rc = mosquitto_subscribe(mosq_, nullptr, topic.c_str(), qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        Logger::error("MQTT", "Subscribe to " + topic + " failed: "
                      + std::string(mosquitto_strerror(rc)));
        return false;
    }
    Logger::info("MQTT", "Subscribed to " + topic + " (QoS " + std::to_string(qos) + ")");
    return true;
}

bool MqttHandler::publish(const std::string& topic, const std::string& payload,
                           int qos, bool retain) {
    if (!mosq_ || !connected_) return false;
    int rc = mosquitto_publish(mosq_, nullptr, topic.c_str(),
                                payload.size(), payload.c_str(), qos, retain);
    if (rc != MOSQ_ERR_SUCCESS) {
        Logger::error("MQTT", "Publish to " + topic + " failed: "
                      + std::string(mosquitto_strerror(rc)));
        return false;
    }
    Logger::debug("MQTT", "Published " + topic + " -> " + payload);
    return true;
}

void MqttHandler::setMessageCallback(MessageCallback cb) {
    messageCallback_ = cb;
}

void MqttHandler::loopStart() {
    if (!mosq_) return;
    running_ = true;
    int rc = mosquitto_loop_start(mosq_);
    if (rc != MOSQ_ERR_SUCCESS) {
        Logger::error("MQTT", "Loop start failed: " + std::string(mosquitto_strerror(rc)));
        running_ = false;
        return;
    }
    reconnectThread_ = std::thread(&MqttHandler::reconnectLoop, this);
    reconnectThread_.detach();
}

void MqttHandler::loopStop() {
    running_ = false;
    if (mosq_) {
        mosquitto_loop_stop(mosq_, true);
    }
}

void MqttHandler::onMessageCallback(struct mosquitto* mosq, void* obj,
                                     const struct mosquitto_message* msg) {
    (void)mosq;
    MqttHandler* self = static_cast<MqttHandler*>(obj);
    if (!self || !msg || !msg->topic || !msg->payload) return;

    std::string topic(msg->topic);
    std::string payload(static_cast<const char*>(msg->payload), msg->payloadlen);

    if (self->messageCallback_) {
        self->messageCallback_(topic, payload);
    }
}

void MqttHandler::onConnectCallback(struct mosquitto* mosq, void* obj, int rc) {
    (void)mosq;
    MqttHandler* self = static_cast<MqttHandler*>(obj);
    if (!self) return;

    if (rc == MOSQ_ERR_SUCCESS) {
        self->connected_ = true;
        Logger::info("MQTT", "Connected successfully");
    } else {
        Logger::error("MQTT", "Connect failed with code " + std::to_string(rc));
        self->connected_ = false;
    }
}

void MqttHandler::onDisconnectCallback(struct mosquitto* mosq, void* obj, int rc) {
    (void)mosq;
    MqttHandler* self = static_cast<MqttHandler*>(obj);
    if (!self) return;

    self->connected_ = false;
    if (rc != 0) {
        Logger::warn("MQTT", "Unexpected disconnect (rc=" + std::to_string(rc) + ")");
    }
}

void MqttHandler::reconnectLoop() {
    const int delaySec = 5;
    while (running_) {
        if (!connected_ && mosq_) {
            Logger::info("MQTT", "Reconnecting in " + std::to_string(delaySec) + "s...");
            std::this_thread::sleep_for(std::chrono::seconds(delaySec));

            int rc = mosquitto_reconnect(mosq_);
            if (rc == MOSQ_ERR_SUCCESS) {
                connected_ = true;
                Logger::info("MQTT", "Reconnected successfully");
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
