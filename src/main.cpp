// src/main.cpp
#include "remote_scales_plugin_registry.h"
#include "scales/felicitaScale.h"
#include "remote_scales.h"
#include <Arduino.h>

std::unique_ptr<RemoteScales> device = nullptr;
RemoteScalesScanner scanner;

void setup() {
    Serial.begin(115200);
    NimBLEDevice::init("");
    Serial.println("BLE initialized");

    FelicitaScalePlugin::apply();

    // Start scanning
    scanner.initializeAsyncScan();
    Serial.println("Started scanning for devices.");

    // Wait for devices to be discovered
    unsigned long scanStartTime = millis();
    const unsigned long scanTimeout = 10000; // 10 seconds timeout

    while (scanner.getDiscoveredScales().empty()) {
        if (millis() - scanStartTime > scanTimeout) {
            Serial.println("Scan timeout. No devices found.");
            scanner.stopAsyncScan();
            return;
        }
        delay(100);
    }

    // Stop scanning after devices are discovered
    scanner.stopAsyncScan();
    Serial.println("Stopped scanning.");

    // Retrieve the list of discovered devices
    std::vector<DiscoveredDevice> discoveredDevices = scanner.getDiscoveredScales();

    // Iterate through discovered devices to find a compatible one
    bool deviceFound = false;
    for (const auto& dev : discoveredDevices) {
        if (RemoteScalesPluginRegistry::getInstance()->containsPluginForDevice(dev)) {
            device = RemoteScalesFactory::getInstance()->create(dev);
            deviceFound = true;
            Serial.print("Found compatible device: ");
            Serial.println(dev.getName().c_str());
            break;
        }
    }

    if (!deviceFound || !device) {
        Serial.println("No compatible devices found or failed to create scales instance.");
        return;
    }

    // Set up log callback
    device->setLogCallback([](std::string message) {
        Serial.print(message.c_str());
    });

    // Set up weight updated callback
    device->setWeightUpdatedCallback([](float weight) {
        Serial.print("Weight updated: ");
        Serial.print(weight);
        Serial.println(" g");
    });

    // Attempt to connect to the device
    if (device->connect()) {
        Serial.println("Scales connected successfully.");
    } else {
        Serial.println("Failed to connect to scales.");
    }
}

void loop() {
    if (device) {
        device->update();
    }
    delay(1000);
}