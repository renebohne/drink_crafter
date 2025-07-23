#include <Arduino.h>
#include "config.h"
#include "pump_control.h"
#include "web_server.h"
#include <SPIFFS.h> // Include SPIFFS library here

// --- Global Variable Definitions ---
// These variables are declared as 'extern' in config.h and defined here.
const char* AP_SSID = "Drink-Crafter";
const char* AP_PASSWORD = "12345678";
const int relayPins[PUMP_COUNT] = RELAY_PINS;


void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  // Initialize SPIFFS first, as other modules depend on it.
  if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
  }

  // Initialize pumps and load all settings from flash
  setupPumps();

  // Initialize the web server
  setupWebServer();
}

void loop() {
  // Handle web client requests
  handleWebServerClient();

  // Update pump states based on current system state
  updateExecution();

  // Yield to other tasks (like the web server) using the FreeRTOS vTaskDelay.
  // This is the correct way to prevent the UI from becoming unresponsive.
  vTaskDelay(1); 
}
