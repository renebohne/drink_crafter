#include <Arduino.h>
#include "config.h"
#include "pump_control.h"
#include "web_server.h"
#include <LittleFS.h> // Use LittleFS library for all platforms

// --- Global Variable Definitions ---
// These variables are declared as 'extern' in config.h and defined here.
const char* AP_SSID = "Drink-Crafter";
const char* AP_PASSWORD = "12345678";
const int relayPins[PUMP_COUNT] = RELAY_PINS;


void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");

  // Initialize LittleFS
  if(!LittleFS.begin(true)){
      Serial.println("An Error has occurred while mounting LittleFS");
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
  vTaskDelay(1); 
}
