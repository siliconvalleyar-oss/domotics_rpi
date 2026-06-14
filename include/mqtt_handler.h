#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <mosquitto.h>
#include <string>
#include <functional>
#include <atomic>
#include <thread>

using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;

class MqttHandler {
public:
    MqttHandler(const std::string& host, int port, const std::string& clientId);
    ~MqttHandler();

    bool connect(int keepalive = 60);
    void disconnect();
    bool isConnected() const;

    bool subscribe(const std::string& topic, int qos = 1);
    bool publish(const std::string& topic, const std::string& payload, int qos = 1, bool retain = false);

    void setMessageCallback(MessageCallback cb);

    void loopStart();
    void loopStop();

    static void onMessageCallback(struct mosquitto* mosq, void* obj,
                                   const struct mosquitto_message* msg);
    static void onConnectCallback(struct mosquitto* mosq, void* obj, int rc);
    static void onDisconnectCallback(struct mosquitto* mosq, void* obj, int rc);

private:
    void reconnectLoop();

    struct mosquitto* mosq_;
    std::string host_;
    int port_;
    std::string clientId_;
    int keepalive_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    MessageCallback messageCallback_;
    std::thread reconnectThread_;
};

#endif // MQTT_HANDLER_H
