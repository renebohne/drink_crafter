#ifndef CONFIG_H
#define CONFIG_H

// --- WiFi Configuration ---
// These are declared here but defined in main.cpp to prevent multiple definitions.
extern const char* AP_SSID;
extern const char* AP_PASSWORD;

// --- Pump & Relay Configuration ---
// These values are now defined by the build flags in platformio.ini
// This allows for multiple hardware profiles.
#ifndef PUMP_COUNT
  #define PUMP_COUNT 8 // Default fallback
#endif

#ifndef RELAY_PINS
  #define RELAY_PINS {13, 12, 14, 27, 26, 25, 33, 32} // Default fallback
#endif

// This is declared here but defined in main.cpp
extern const int relayPins[PUMP_COUNT];
const int CALIBRATION_RUN_DURATION_MS = 10000; // 10 seconds for calibration run

// --- Recipe Configuration ---
const int MAX_RECIPES = 10;
const int MAX_STEPS_PER_RECIPE = 10;

// --- Storage Configuration ---
#define PREFERENCES_NAMESPACE "pump-config"
#define TOTAL_RUNS_KEY "totalRuns"

#endif // CONFIG_H
