#include "web_server.h"
#include "config.h"
#include "pump_control.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <SPIFFS.h>

WebServer server(80);

// --- New Logging Function ---
// This function prints the details of an incoming HTTP request to the Serial monitor.
void logRequest() {
    String method = (server.method() == HTTP_GET) ? "GET" : "POST";
    if (server.method() == HTTP_PUT) {
        method = "PUT";
    }
    Serial.println("HTTP Request: " + method + " " + server.uri());
}

// Helper function to get the content type based on the file extension
String getContentType(String filename) {
    if (server.hasArg("download")) return "application/octet-stream";
    else if (filename.endsWith(".htm")) return "text/html";
    else if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/x-pdf";
    else if (filename.endsWith(".zip")) return "application/x-zip";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

// This function will be called when a client requests a file that is not explicitly handled
bool handleFileRead(String path) {
    if (path.endsWith("/")) path += "index.html"; // If a directory is requested, try to serve index.html
    String contentType = getContentType(path);
    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false; // If the file doesn't exist, return false
}


// --- Route Handler Prototypes ---
void handleSetSettings(); void handleSetVolumes();
void handleStartDosing(); void handleStop(); void handleStatus();
void handleRunRecipe(); void handleCalibratePump();
void handlePumpControl(); void handleResetCounters();
void handleRecipes();

void setupWebServer() {
    // SPIFFS is now initialized in main.cpp

    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.print("AP IP address: "); Serial.println(WiFi.softAPIP());

    // --- Explicit Handlers for Web Assets ---
    server.on("/", HTTP_GET, []() {
        logRequest();
        handleFileRead("/");
    });
    server.on("/script.js", HTTP_GET, []() {
        logRequest();
        handleFileRead("/script.js");
    });
    server.on("/bootstrap.min.css", HTTP_GET, []() {
        logRequest();
        handleFileRead("/bootstrap.min.css");
    });
    server.on("/bootstrap.bundle.min.js", HTTP_GET, []() {
        logRequest();
        handleFileRead("/bootstrap.bundle.min.js");
    });
     server.on("/favicon.ico", HTTP_GET, [](){
        logRequest();
        server.send(404, "text/plain", "Not found");
    });


    // API and action routes
    server.on("/recipes", HTTP_GET, handleRecipes);
    server.on("/recipes", HTTP_PUT, handleRecipes);
    server.on("/settings", HTTP_POST, handleSetSettings);
    server.on("/volumes", HTTP_POST, handleSetVolumes);
    server.on("/start_dosing", HTTP_GET, handleStartDosing);
    server.on("/stop", HTTP_GET, handleStop);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/reset_counters", HTTP_POST, handleResetCounters);

    // The onNotFound handler is now a true fallback for any other request.
    server.onNotFound([]() {
        logRequest();
        String uri = server.uri();
        // Check if the request is for pump control
        if (uri.startsWith("/pump/")) {
            handlePumpControl();
        }
        // Check if the request is to run a recipe
        else if (uri.startsWith("/run_recipe/")) {
            handleRunRecipe();
        }
        // Check if the request is to calibrate a pump
        else if (uri.startsWith("/calibrate/")) {
            handleCalibratePump();
        }
        // Check if it's a file to be served from SPIFFS
        else if (!handleFileRead(uri)) {
            // If not a pump command and not a file, send 404
            server.send(404, "text/plain", "404: Not Found");
        }
    });

    server.begin();
    Serial.println("HTTP server started");
}

void handleWebServerClient() { server.handleClient(); }

// --- Route Handler Implementations ---

void handleRecipes() {
    logRequest();
    if (server.method() == HTTP_GET) {
        handleFileRead("/recipes.json");
    } else if (server.method() == HTTP_PUT) {
        File file = SPIFFS.open("/recipes.json", "w");
        if (!file) {
            server.send(500, "text/plain", "Error opening recipes.json for writing");
            return;
        }
        file.print(server.arg("plain"));
        file.close();
        
        // Clear old recipes from permanent storage before rebooting
        clearAllRecipesFromPreferences();

        server.send(200, "text/plain", "Recipes updated. Rebooting to apply changes.");
        vTaskDelay(200 / portTICK_PERIOD_MS); // Short delay to ensure response is sent
        ESP.restart();
    }
}

void handlePumpControl() {
    String uri = server.uri(); // e.g., "/pump/0/1"
    
    // Find the position of the slashes to extract the numbers
    int firstSlash = uri.indexOf('/', 6); // Find slash after "/pump/"
    
    if (firstSlash != -1) {
        String pumpIndexStr = uri.substring(6, firstSlash);
        String stateStr = uri.substring(firstSlash + 1);
        int pumpIndex = pumpIndexStr.toInt();
        int state = stateStr.toInt();
        
        setPumpState(pumpIndex, state);
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void handleRunRecipe() {
    String uri = server.uri(); // e.g., "/run_recipe/0"
    String indexStr = uri.substring(12); // Get substring after "/run_recipe/"
    int recipeIndex = indexStr.toInt();
    startRecipe(recipeIndex);
    server.send(200, "text/plain", "OK");
}

void handleCalibratePump() {
    String uri = server.uri(); // e.g., "/calibrate/0"
    String indexStr = uri.substring(11); // Get substring after "/calibrate/"
    int pumpIndex = indexStr.toInt();
    calibratePump(pumpIndex);
    server.send(200, "text/plain", "OK");
}

void handleStartDosing() { logRequest(); startDosingSequence(); server.send(200, "text/plain", "OK"); }
void handleStop() { logRequest(); stopCurrentSequence(); server.send(200, "text/plain", "OK"); }
void handleResetCounters() { logRequest(); resetAllCounters(); server.send(200, "text/plain", "OK"); }

void handleSetSettings() {
    logRequest();
    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "text/plain", "JSON fail"); return; }
    JsonArray pumpsArray = doc["pumps"];
    Pump newPumps[PUMP_COUNT];
    for (int i = 0; i < PUMP_COUNT; i++) {
        newPumps[i].name = pumpsArray[i]["name"].as<String>();
        newPumps[i].calibration = pumpsArray[i]["calibration"].as<unsigned long>();
        newPumps[i].assignedRelay = pumpsArray[i]["assignedRelay"].as<int>();
    }
    updatePumpSettings(newPumps);
    server.send(200, "text/plain", "OK");
}

void handleSetVolumes() {
    logRequest();
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, server.arg("plain"))) { server.send(400, "text/plain", "JSON fail"); return; }
    JsonArray volArray = doc["volumes"];
    unsigned int newVolumes[PUMP_COUNT];
    for (int i = 0; i < PUMP_COUNT; i++) newVolumes[i] = volArray[i].as<unsigned int>();
    setVolumes(newVolumes);
    server.send(200, "text/plain", "OK");
}

void handleStatus() {
    logRequest();
    StaticJsonDocument<4096> doc;
    doc["systemState"] = (systemState == IDLE) ? "IDLE" : (systemState == DOSING_MANUAL ? "DOSING_MANUAL" : (systemState == CALIBRATING ? "CALIBRATING" : "EXECUTING_RECIPE"));
    doc["totalRunCount"] = totalRunCount;
    
    // Add execution progress info
    if (systemState != IDLE) {
        JsonObject progress = doc.createNestedObject("progress");
        if (systemState == EXECUTING_RECIPE) {
            progress["taskName"] = recipes[currentRecipeIndex].name;
            progress["currentStep"] = currentStepIndex + 1;
            progress["totalSteps"] = recipes[currentRecipeIndex].stepCount;
        } else if (systemState == DOSING_MANUAL) {
            progress["taskName"] = "Manual Dosing";
            progress["currentStep"] = 1;
            progress["totalSteps"] = 1;
        } else if (systemState == CALIBRATING) {
            progress["taskName"] = "Calibrating: " + pumps[calibratingPumpIndex].name;
            progress["currentStep"] = 1;
            progress["totalSteps"] = 1;
        }
        progress["stepStartTime"] = stepStartTime;
        progress["stepDuration"] = longestStepTime;
        progress["currentTime"] = millis();
    }

    JsonArray states = doc.createNestedArray("relayStates");
    for (int i = 0; i < PUMP_COUNT; i++) states.add(relayStates[i]);
    JsonArray pumpsArray = doc.createNestedArray("pumps");
    for (int i = 0; i < PUMP_COUNT; i++) {
        JsonObject p = pumpsArray.createNestedObject();
        p["name"] = pumps[i].name;
        p["assignedRelay"] = pumps[i].assignedRelay;
        p["calibration"] = pumps[i].calibration;
    }
    JsonArray recipesArray = doc.createNestedArray("recipes");
    for (int i = 0; i < MAX_RECIPES; i++) {
        if (recipes[i].name != "") {
            JsonObject r = recipesArray.createNestedObject();
            r["name"] = recipes[i].name;
            r["stepCount"] = recipes[i].stepCount;
            r["runCount"] = recipeRunCounts[i];
            JsonArray steps = r.createNestedArray("steps");
            for(int j=0; j<recipes[i].stepCount; j++) {
                JsonObject s = steps.createNestedObject();
                JsonArray actions = s.createNestedArray("actions");
                for(int k=0; k<recipes[i].steps[j].actionCount; k++) {
                    JsonObject a = actions.createNestedObject();
                    a["pumpIndex"] = recipes[i].steps[j].actions[k].pumpIndex;
                    a["volume"] = recipes[i].steps[j].actions[k].volume;
                }
            }
        }
    }
    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
}
