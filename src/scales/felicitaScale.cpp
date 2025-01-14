#include "felicitaScale.h"
#include <cstring>

// Initialize UUID constants
const NimBLEUUID FelicitaScale::DATA_SERVICE_UUID("FFE0");
const NimBLEUUID FelicitaScale::DATA_CHARACTERISTIC_UUID("FFE1");

FelicitaScale::FelicitaScale(const DiscoveredDevice& device) : RemoteScales(device) {}

bool FelicitaScale::connect() {
    if (isConnected()) {
        log("Already connected.\n");
        return true;
    }

    if (!clientConnect()) {
        clientCleanup();
        return false;
    }

    if (!performConnectionHandshake()) {
        clientCleanup();
        return false;
    }

    return true;
}

void FelicitaScale::disconnect() {
    clientCleanup();
}

bool FelicitaScale::isConnected() {
    return clientIsConnected();
}

void FelicitaScale::update() {
    if (markedForReconnection) {
        log("Reconnecting...\n");
        clientCleanup();
        connect();
        markedForReconnection = false;
    } else {
        sendHeartbeat();
    }
}

bool FelicitaScale::tare() {
    if (!isConnected()) return false;
    log("Tare command sent.\n");
    uint8_t tareCommand[] = {CMD_TARE};
    dataCharacteristic->writeValue(tareCommand, sizeof(tareCommand), true);
    return true;
}

bool FelicitaScale::performConnectionHandshake() {
    log("Performing handshake...\n");

    service = clientGetService(DATA_SERVICE_UUID);
    if (!service) {
        log("Service not found.\n");
        return false;
    }

    dataCharacteristic = service->getCharacteristic(DATA_CHARACTERISTIC_UUID);
    if (!dataCharacteristic) {
        log("Characteristic not found.\n");
        return false;
    }

    if (dataCharacteristic->canNotify()) {
        dataCharacteristic->subscribe(true, [this](NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
            notifyCallback(characteristic, data, length, isNotify);
        });
    } else {
        log("Notifications not supported.\n");
        return false;
    }

    return true;
}

void FelicitaScale::sendHeartbeat() {
    if (!isConnected()) {
        markedForReconnection = true;
        return;
    }

    uint8_t heartbeatCommand[] = {0x00}; // Example: Replace with valid heartbeat command if needed
    dataCharacteristic->writeValue(heartbeatCommand, sizeof(heartbeatCommand), true);
}

void FelicitaScale::notifyCallback(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
    log("Notification received.\n");
    if (length < 18) {
        log("Malformed data.\n");
        return;
    }
    parseStatusUpdate(data, length);
}

void FelicitaScale::parseStatusUpdate(const uint8_t* data, size_t length) {
    float weight = static_cast<float>(parseWeight(data)) / 100.0f;
    setWeight(weight);
    log("Weight updated: %.2f g\n", weight);
}

int32_t FelicitaScale::parseWeight(const uint8_t* data) {
    return (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
}

uint8_t FelicitaScale::calculateChecksum(const uint8_t* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length - 1; ++i) {
        checksum += data[i];
    }
    return checksum;
}
