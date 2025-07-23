#include "pump_control.h"
#include "config.h"
#include <Preferences.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// --- Global State Variables ---
Pump pumps[PUMP_COUNT];
Recipe recipes[MAX_RECIPES];
unsigned int recipeRunCounts[MAX_RECIPES] = {0}; // New array for counters
int relayStates[PUMP_COUNT] = {LOW};
unsigned long totalRunCount = 0;

SystemState systemState = IDLE;

// For sequence execution
unsigned long stepStartTime = 0;
unsigned long longestStepTime = 0;
int currentRecipeIndex = -1;
int currentStepIndex = -1;
int calibratingPumpIndex = -1;

Preferences preferences;

// --- Local (Static) Function Declarations ---
// These are only used within this file.
static void savePumpSettings();
static void saveRecipe(int recipeIndex, const Recipe& recipe);
static void loadRecipes();
static void loadPumpSettings();
static void loadCounters();

// --- Helper Functions ---
int findPumpIndexByName(String name) {
    for (int i = 0; i < PUMP_COUNT; i++) {
        if (pumps[i].name.equalsIgnoreCase(name)) {
            return i;
        }
    }
    return -1; // Not found
}

// --- First Boot Setup ---
void initializeFirstBootSettings() {
    Serial.println("Performing first-time setup...");

    // 1. Set default pump names and settings
    Serial.println("Step 1: Setting default pump names...");
    String defaultNames[] = {"Vodka", "Tequila", "Rum", "Orange Juice", "Pineapple Juice", "Grenadine", "Coconut Milk", "Lime Juice"};
    for (int i = 0; i < PUMP_COUNT; i++) {
        if (i < 8) {
            pumps[i].name = defaultNames[i];
        } else {
            pumps[i].name = "Pump " + String(i + 1);
        }
        pumps[i].assignedRelay = i;
        pumps[i].calibration = 1000; // Default 1 sec/ml
        pumps[i].volume = 0;
    }
    savePumpSettings();
    Serial.println("Default pump names have been set and saved.");

    // 2. Load recipes from recipes.json in SPIFFS
    Serial.println("Step 2: Loading recipes from recipes.json...");
    if (SPIFFS.exists("/recipes.json")) {
        Serial.println("recipes.json found. Parsing...");
        File file = SPIFFS.open("/recipes.json", "r");
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            return;
        }

        JsonArray recipeArray = doc.as<JsonArray>();
        int recipeIdx = 0;
        for (JsonObject recipeObj : recipeArray) {
            if (recipeIdx >= MAX_RECIPES) break;

            Recipe newRecipe;
            newRecipe.name = recipeObj["name"].as<String>();
            JsonArray stepsArray = recipeObj["steps"];
            newRecipe.stepCount = 0;
            Serial.println(" - Found recipe: " + newRecipe.name);

            for (JsonObject stepObj : stepsArray) {
                if (newRecipe.stepCount >= MAX_STEPS_PER_RECIPE) break;
                
                RecipeStep& currentStep = newRecipe.steps[newRecipe.stepCount];
                currentStep.actionCount = 0;
                JsonArray actionsArray = stepObj["actions"];

                for (JsonObject actionObj : actionsArray) {
                    if (currentStep.actionCount >= PUMP_COUNT) break;
                    
                    String pumpName = actionObj["pump"];
                    int pumpIndex = findPumpIndexByName(pumpName);

                    if (pumpIndex != -1) {
                        currentStep.actions[currentStep.actionCount].pumpIndex = pumpIndex;
                        currentStep.actions[currentStep.actionCount].volume = actionObj["volume"];
                        currentStep.actionCount++;
                    } else {
                        Serial.println("   - Warning: Pump '" + pumpName + "' not found for recipe '" + newRecipe.name + "'. Skipping action.");
                    }
                }
                if (currentStep.actionCount > 0) {
                    newRecipe.stepCount++;
                }
            }
            if (newRecipe.stepCount > 0) {
                saveRecipe(recipeIdx, newRecipe);
                recipeIdx++;
            }
        }
    } else {
        Serial.println("Warning: /recipes.json not found in SPIFFS.");
    }

    // 3. Finalize setup by resetting counters
    Serial.println("Step 3: Finalizing setup and resetting counters...");
    resetAllCounters();
    Serial.println("First-time setup complete.");
}


// --- Pump Setup & Settings ---

void setupPumps() {
    for (int i = 0; i < PUMP_COUNT; i++) {
        pinMode(relayPins[i], OUTPUT);
        digitalWrite(relayPins[i], LOW);
    }

    preferences.begin(PREFERENCES_NAMESPACE, true);
    bool recipesExistInPrefs = preferences.isKey("r_json_0");
    preferences.end();
    
    if (!recipesExistInPrefs) {
        initializeFirstBootSettings();
    }

    loadPumpSettings();
    loadRecipes();
    loadCounters();
    Serial.println("Pumps & Recipes initialized.");
}

static void savePumpSettings() {
    preferences.begin(PREFERENCES_NAMESPACE, false);
    for (int i = 0; i < PUMP_COUNT; i++) {
        preferences.putString(("p_name_" + String(i)).c_str(), pumps[i].name);
        preferences.putInt(("p_relay_" + String(i)).c_str(), pumps[i].assignedRelay);
        preferences.putULong(("p_cal_" + String(i)).c_str(), pumps[i].calibration);
    }
    preferences.end();
    Serial.println("Pump settings saved.");
}

static void loadPumpSettings() {
    preferences.begin(PREFERENCES_NAMESPACE, true);
    for (int i = 0; i < PUMP_COUNT; i++) {
        pumps[i].name = preferences.getString(("p_name_" + String(i)).c_str(), "Pump " + String(i + 1));
        pumps[i].assignedRelay = preferences.getInt(("p_relay_" + String(i)).c_str(), i);
        pumps[i].calibration = preferences.getULong(("p_cal_" + String(i)).c_str(), 1000);
        pumps[i].volume = 0;
    }
    preferences.end();
}

void updatePumpSettings(const Pump newPumps[]) {
    for (int i = 0; i < PUMP_COUNT; i++) {
        pumps[i].name = newPumps[i].name;
        pumps[i].assignedRelay = newPumps[i].assignedRelay;
        pumps[i].calibration = newPumps[i].calibration;
    }
    savePumpSettings();
}

// --- Direct Pump & Volume Control ---

void setPumpState(int pumpIndex, int state) {
    if (pumpIndex < 0 || pumpIndex >= PUMP_COUNT) return;
    int relayIndex = pumps[pumpIndex].assignedRelay;
    if (relayIndex >= 0 && relayIndex < PUMP_COUNT) {
        digitalWrite(relayPins[relayIndex], state);
        relayStates[relayIndex] = state;
    }
}

void calibratePump(int pumpIndex) {
    if (systemState != IDLE) return;
    if (pumpIndex < 0 || pumpIndex >= PUMP_COUNT) return;

    Serial.println("Running calibration for pump: " + pumps[pumpIndex].name);
    systemState = CALIBRATING;
    calibratingPumpIndex = pumpIndex;
    longestStepTime = CALIBRATION_RUN_DURATION_MS;
    stepStartTime = millis();
    setPumpState(pumpIndex, HIGH);
}

void setVolumes(const unsigned int newVolumes[]) {
    for (int i = 0; i < PUMP_COUNT; i++) {
        pumps[i].volume = newVolumes[i];
    }
    Serial.println("Volumes updated.");
}

void startDosingSequence() {
    if (systemState != IDLE) return;
    Serial.println("Starting manual dosing sequence...");
    systemState = DOSING_MANUAL;
    currentStepIndex = 0;
    longestStepTime = 0;

    for (int i = 0; i < PUMP_COUNT; i++) {
        unsigned long runTime = (unsigned long)pumps[i].volume * pumps[i].calibration;
        if (runTime > 0) {
            setPumpState(i, HIGH);
            if (runTime > longestStepTime) {
                longestStepTime = runTime;
            }
        }
    }
    stepStartTime = millis();
}

// --- Recipe Management ---

static void loadRecipes() {
    preferences.begin(PREFERENCES_NAMESPACE, true);
    for (int i = 0; i < MAX_RECIPES; i++) {
        String recipeKey = "r_json_" + String(i);
        String json = preferences.getString(recipeKey.c_str(), "");
        if (json.length() > 0) {
            StaticJsonDocument<1024> doc;
            deserializeJson(doc, json);
            recipes[i].name = doc["name"].as<String>();
            recipes[i].stepCount = doc["stepCount"];
            JsonArray steps = doc["steps"];
            for (int j = 0; j < recipes[i].stepCount; j++) {
                JsonArray actions = steps[j]["actions"];
                recipes[i].steps[j].actionCount = actions.size();
                for (int k = 0; k < recipes[i].steps[j].actionCount; k++) {
                    recipes[i].steps[j].actions[k].pumpIndex = actions[k]["pump"];
                    recipes[i].steps[j].actions[k].volume = actions[k]["vol"];
                }
            }
        } else {
            recipes[i].name = "";
            recipes[i].stepCount = 0;
        }
    }
    preferences.end();
}

static void saveRecipe(int recipeIndex, const Recipe& recipe) {
    if (recipeIndex < 0 || recipeIndex >= MAX_RECIPES) return;
    recipes[recipeIndex] = recipe;
    
    StaticJsonDocument<1024> doc;
    doc["name"] = recipe.name;
    doc["stepCount"] = recipe.stepCount;
    JsonArray steps = doc.createNestedArray("steps");
    for (int i = 0; i < recipe.stepCount; i++) {
        JsonObject stepObj = steps.createNestedObject();
        JsonArray actions = stepObj.createNestedArray("actions");
        for (int j = 0; j < recipe.steps[i].actionCount; j++) {
            JsonObject actionObj = actions.createNestedObject();
            actionObj["pump"] = recipe.steps[i].actions[j].pumpIndex;
            actionObj["vol"] = recipe.steps[i].actions[j].volume;
        }
    }
    
    String json;
    serializeJson(doc, json);
    
    preferences.begin(PREFERENCES_NAMESPACE, false);
    preferences.putString(("r_json_" + String(recipeIndex)).c_str(), json);
    preferences.end();
    Serial.println("Recipe " + String(recipeIndex) + " saved.");
}

static void loadCounters() {
    preferences.begin(PREFERENCES_NAMESPACE, true);
    totalRunCount = preferences.getULong(TOTAL_RUNS_KEY, 0);
    for (int i = 0; i < MAX_RECIPES; i++) {
        recipeRunCounts[i] = preferences.getUInt(("rc_" + String(i)).c_str(), 0);
    }
    preferences.end();
}

void resetAllCounters() {
    Serial.println("Resetting all run counters.");
    totalRunCount = 0;
    preferences.begin(PREFERENCES_NAMESPACE, false);
    preferences.putULong(TOTAL_RUNS_KEY, 0);
    for (int i = 0; i < MAX_RECIPES; i++) {
        recipeRunCounts[i] = 0;
        preferences.putUInt(("rc_" + String(i)).c_str(), 0);
    }
    preferences.end();
}

void clearAllRecipesFromPreferences() {
    Serial.println("Clearing all recipes from Preferences...");
    preferences.begin(PREFERENCES_NAMESPACE, false);
    for (int i = 0; i < MAX_RECIPES; i++) {
        String recipeKey = "r_json_" + String(i);
        if (preferences.isKey(recipeKey.c_str())) {
            preferences.remove(recipeKey.c_str());
        }
    }
    preferences.end();
    Serial.println("Recipes cleared from Preferences.");
}


// --- Sequence & Recipe Execution ---

void stopCurrentSequence() {
    Serial.println("Halting all operations.");
    systemState = IDLE;
    currentRecipeIndex = -1;
    currentStepIndex = -1;
    calibratingPumpIndex = -1;
    for (int i = 0; i < PUMP_COUNT; i++) {
        setPumpState(i, LOW);
    }
}

void startRecipe(int recipeIndex) {
    if (systemState != IDLE || recipeIndex < 0 || recipeIndex >= MAX_RECIPES || recipes[recipeIndex].stepCount == 0) {
        return;
    }
    Serial.println("Starting recipe: " + recipes[recipeIndex].name);
    
    // Increment counters and save them
    totalRunCount++;
    recipeRunCounts[recipeIndex]++;
    
    preferences.begin(PREFERENCES_NAMESPACE, false);
    preferences.putULong(TOTAL_RUNS_KEY, totalRunCount);
    preferences.putUInt(("rc_" + String(recipeIndex)).c_str(), recipeRunCounts[recipeIndex]);
    preferences.end();

    systemState = EXECUTING_RECIPE;
    currentRecipeIndex = recipeIndex;
    currentStepIndex = -1;
}

void executeNextStep() {
    currentStepIndex++;
    if (currentStepIndex >= recipes[currentRecipeIndex].stepCount) {
        stopCurrentSequence();
        Serial.println("Recipe finished.");
        return;
    }

    Serial.println("Executing step " + String(currentStepIndex + 1));
    RecipeStep& step = recipes[currentRecipeIndex].steps[currentStepIndex];
    longestStepTime = 0;

    for(int i=0; i<PUMP_COUNT; i++) setPumpState(i, LOW);

    for (int i = 0; i < step.actionCount; i++) {
        int pumpIdx = step.actions[i].pumpIndex;
        unsigned int vol = step.actions[i].volume;
        unsigned long runTime = (unsigned long)vol * pumps[pumpIdx].calibration;
        
        if (runTime > 0) {
            setPumpState(pumpIdx, HIGH);
            if (runTime > longestStepTime) {
                longestStepTime = runTime;
            }
        }
    }
    stepStartTime = millis();
}

void updateExecution() {
    if (systemState == IDLE) return;
    
    unsigned long currentTime = millis();

    if (systemState == EXECUTING_RECIPE) {
        if (currentStepIndex == -1 || (currentTime - stepStartTime >= longestStepTime)) {
            executeNextStep();
        }
    } else if (systemState == DOSING_MANUAL) {
        bool allFinished = true;
        for (int i = 0; i < PUMP_COUNT; i++) {
            int relayIdx = pumps[i].assignedRelay;
            if (relayStates[relayIdx] == HIGH) {
                unsigned long runTime = (unsigned long)pumps[i].volume * pumps[i].calibration;
                if (currentTime - stepStartTime >= runTime) {
                    setPumpState(i, LOW);
                } else {
                    allFinished = false;
                }
            }
        }
        if (allFinished) {
            stopCurrentSequence();
            Serial.println("Manual dosing finished.");
        }
    } else if (systemState == CALIBRATING) {
        if (currentTime - stepStartTime >= longestStepTime) {
            stopCurrentSequence();
            Serial.println("Calibration run finished.");
        }
    }
}
