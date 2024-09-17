#include <NimBLEDevice.h>
#include "scales/difluid.h"

// Global instance for DifluidScales
DifluidScales *scales = nullptr;
NimBLEScan *pScan = nullptr;

// BLE scan result callback
class MyAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override {
        Serial.print("Found BLE device: ");
        Serial.println(advertisedDevice->toString().c_str());

        // Create the DiscoveredDevice instance
        DiscoveredDevice device(advertisedDevice);

        // Directly attempt to connect to the Difluid device
        Serial.println("Attempting to connect to Difluid device...");
        scales = new DifluidScales(device);

        if (scales->connect()) {
            Serial.println("Successfully connected to Difluid scales.");
        } else {
            Serial.println("Failed to connect to Difluid scales.");
            delete scales;
            scales = nullptr;
        }
    }
};

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    Serial.println("Starting Remote Scales...");

    // Initialize NimBLE device
    NimBLEDevice::init("RemoteScales");

    // Set scan parameters
    pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pScan->setActiveScan(true);

    // Start scanning for devices
    Serial.println("Starting BLE scan...");
    pScan->start(30);
}

void loop() {
    // Continuously update the scales if initialized
    if (scales != nullptr) {
        scales->update();
    }

    delay(1000);  // Adjust delay as needed
}
