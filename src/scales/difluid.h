
#pragma once
#include "remote_scales.h"
#include "remote_scales_plugin_registry.h"
// Extended DifluidMessageType Enum
enum class DifluidMessageType : uint8_t {
    SYSTEM = 0x0A,       // System messages like status or control
    WEIGHT = 0x0B,       // Weight measurement messages
    SENSOR = 0x0C,       // Custom message type for sensor data
    CALIBRATION = 0x0D, // Custom message type for calibration
};

class DifluidScales : public RemoteScales {
public:
    DifluidScales(const DiscoveredDevice& device);
    
    // Override functions to manage connection lifecycle
    void update() override;
    bool connect() override;
    void disconnect() override;
    bool isConnected() override;
    bool tare() override;

private:
    std::string weightUnits;
    float time;
    uint8_t battery;
    uint32_t lastHeartbeat = 0;
    bool markedForReconnection = false;

    // BLE characteristics and services
    NimBLERemoteService* service = nullptr;
    NimBLERemoteCharacteristic* weightCharacteristic = nullptr;
    NimBLERemoteCharacteristic* commandCharacteristic = nullptr;

    std::vector<uint8_t> dataBuffer;

    // Private methods to handle protocol-specific operations
    bool performConnectionHandshake();
    void subscribeToNotifications();
    void sendMessage(DifluidMessageType msgType, const uint8_t* payload, size_t length, bool waitResponse = false);
    void sendEvent(const uint8_t* payload, size_t length);
    void sendHeartbeat();
    void sendNotificationRequest();
    void sendId();
    
    // Callback for handling BLE notifications
    void notifyCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    
    // Decode and handle the incoming notification
    bool decodeAndHandleNotification();
};

// Plugin class for Difluid Scales, registers with the RemoteScalesPluginRegistry
class DifluidScalesPlugin {
public:
    static void apply() {
        RemoteScalesPlugin plugin = RemoteScalesPlugin{
            .id = "plugin-difluid",
            .handles = [](const DiscoveredDevice& device) { return DifluidScalesPlugin::handles(device); },
            .initialise = [](const DiscoveredDevice& device) -> std::unique_ptr<RemoteScales> { return std::make_unique<DifluidScales>(device); },
        };
        RemoteScalesPluginRegistry::getInstance()->registerPlugin(plugin);
    }

private:
    // Determines whether this plugin can handle the given device
    static bool handles(const DiscoveredDevice& device) {
        const std::string& deviceName = device.getName();
        return !deviceName.empty() && (deviceName.find("Mb") == 0);
    }
};
