#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>

// --- Function Declarations ---
void setupWebServer();
void handleWebServerClient();

// --- Route Handler Declarations ---
void handleSetSettings();
void handleSetVolumes();
void handleStartDosing();
void handleStop();
void handleStatus();
void handleRunRecipe();
void handleCalibratePump();
void handlePumpControl();
void handleResetCounters();
void handleRecipes(); // New handler for GET/PUT recipes

// --- Extern WebServer Object ---
extern WebServer server;

#endif // WEB_SERVER_H
