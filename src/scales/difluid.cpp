// difluid.cpp
#include "difluid.h"
#include "remote_scales_plugin_registry.h"

/*
Handle protocol according to the spec found at
https://github.com/DiFluid/difluid-sdk-demo/blob/master/docs/protocolMicrobalance.md
*/

const NimBLEUUID serviceUUID("00DD");
const NimBLEUUID weightCharacteristicUUID("AA01");

//-----------------------------------------------------------------------------------/
//---------------------------        PUBLIC       -----------------------------------/
//-----------------------------------------------------------------------------------/
DifluidScales::DifluidScales(const DiscoveredDevice& device) : RemoteScales(device) {}

bool DifluidScales::connect() {
    if (RemoteScales::clientIsConnected()) {
        RemoteScales::log("Already connected\n");
        return true;
    }

    //RemoteScales::log("Connecting to %s[%s]\n", getDeviceName().c_str(), getDeviceAddress().c_str());
    if (!clientConnect()) {
        RemoteScales::clientCleanup();
        return false;
    }

    if (!performConnectionHandshake()) {
        RemoteScales::clientCleanup();
        return false;
    }

    return true;
}

void DifluidScales::disconnect() {
    RemoteScales::clientCleanup();
}

bool DifluidScales::isConnected() {
    return RemoteScales::clientIsConnected();
}

void DifluidScales::update() {
    if (markedForReconnection) {
        RemoteScales::log("Marked for disconnection. Will attempt to reconnect.\n");
        RemoteScales::clientCleanup();
        connect();
        markedForReconnection = false;
    } else {
        sendHeartbeat();
        RemoteScales::log("Heartbeat sent.\n");
    }
}

// Tare function
bool DifluidScales::tare() {
    if (!isConnected()) return false;
    RemoteScales::log("Tare command sent.\n");
    uint8_t tareCommand[] = {0xdf, 0xdf, 0x03, 0x02, 0x01, 0x01, 0xc5};
    weightCharacteristic->writeValue(tareCommand, sizeof(tareCommand), true);
    return true;
}

//-----------------------------------------------------------------------------------/
//---------------------------       PRIVATE       -----------------------------------/
//-----------------------------------------------------------------------------------/
void DifluidScales::notifyCallback(
    NimBLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify
) {
    // Use the log function from RemoteScales
    RemoteScales::log("Notification received:\n");
    for (size_t i = 0; i < length; i++) {
        RemoteScales::log("%02X ", pData[i]);
    }
    RemoteScales::log("\n");

    // Parse the weight from the notification data
    if (length >= 19 && pData[3] == 0) {
        int32_t weight = (pData[5] << 24) | (pData[6] << 16) | (pData[7] << 8) | pData[8];
        weight /= 10;  // Divide by 10 to convert to grams
        RemoteScales::log("Weight: %d\n", weight);
        setWeight(static_cast<float>(weight));
    } else if (pData[0] == 0x02 && pData[1] == 0x00) {
        // Handle heartbeat acknowledgment if necessary
        RemoteScales::log("Heartbeat acknowledged.\n");
    } else {
        RemoteScales::log("Unknown notification received.\n");
    }
}

bool DifluidScales::performConnectionHandshake() {
    RemoteScales::log("Performing handshake\n");

    service = clientGetService(serviceUUID);
    if (service != nullptr) {
        RemoteScales::log("Service found.\n");
    } else {
        RemoteScales::log("Service not found.\n");
        return false;
    }

    weightCharacteristic = service->getCharacteristic(weightCharacteristicUUID);
    if (weightCharacteristic != nullptr) {
        RemoteScales::log("Characteristic found!\n");

        // Subscribe to notifications
        weightCharacteristic->subscribe(true, [this](NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
            notifyCallback(characteristic, data, length, isNotify);
        });

        // Set the scale unit to grams
        setUnitToGram();

        // Enable auto notifications
        enableAutoNotifications();

        // Send a tare command after the connection
        tare();

        // Initialize heartbeat timer
        lastHeartbeat = millis();
    } else {
        RemoteScales::log("Characteristic not found.\n");
        return false;
    }

    return true;
}

void DifluidScales::setUnitToGram() {
    uint8_t unitToGramCommand[] = {0xdf, 0xdf, 0x01, 0x04, 0x01, 0x00, 0xc4};
    weightCharacteristic->writeValue(unitToGramCommand, sizeof(unitToGramCommand), true);
    RemoteScales::log("Set unit to grams.\n");
}

void DifluidScales::enableAutoNotifications() {
    uint8_t enableNotificationsCommand[] = {0xdf, 0xdf, 0x01, 0x00, 0x01, 0x01, 0xc1};
    weightCharacteristic->writeValue(enableNotificationsCommand, sizeof(enableNotificationsCommand), true);
    RemoteScales::log("Enabled auto notifications.\n");
}

void DifluidScales::sendHeartbeat() {
    if (!isConnected()) {
        return;
    }

    uint32_t now = millis();
    if (now - lastHeartbeat < 2000) {
        return;
    }

    uint8_t heartbeatCommand[] = {0xdf, 0xdf, 0x02, 0x00, 0x00, 0x00, 0xc2};
    weightCharacteristic->writeValue(heartbeatCommand, sizeof(heartbeatCommand), true);
    lastHeartbeat = now;
}
