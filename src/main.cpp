#include "remote_scales_plugin_registry.h"
#include "scales/difluid.h"
#include "remote_scales.h"  // For DiscoveredDevice

// Global variable for scales
DifluidScales* devices = nullptr;
RemoteScalesScanner scanner;

// Function to scan and initialize the scales
void setup() {
	Serial.begin(115200);
	// Initialize BLE
	BLEDevice::init("");
	Serial.println("BLE initialized");

	// Apply the DifluidScales plugin
	DifluidScalesPlugin::apply();

	// Initialize and start asynchronous scanning
	scanner.initializeAsyncScan();
	Serial.println("Started scanning for devices.");

	// Wait for a device to be discovered
	while (scanner.getDiscoveredScales().empty()) {
		delay(100);
	}

	// Get the first discovered device
	std::vector<DiscoveredDevice> discoveredDevices = scanner.getDiscoveredScales();
	DiscoveredDevice device = discoveredDevices.front();

	// Initialize the scales with the discovered device
	devices = new DifluidScales(device);
    devices->connect();
	Serial.println("Scales initialized successfully.");
}

// Loop function (can contain logic to handle updates, further processing, etc.)
void loop() {
	// Update the scales
	if (devices != nullptr) {
		devices->update();
	}
	delay(1000);
}