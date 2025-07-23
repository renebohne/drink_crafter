#ifndef PUMP_CONTROL_H
#define PUMP_CONTROL_H

#include <Arduino.h>
#include "config.h"

// --- Data Structures ---

struct Pump {
    String name;
    int assignedRelay;
    unsigned long calibration; // ms per ml
    unsigned int volume;       // ml to dose for a single run
};

struct RecipeStepAction {
    int pumpIndex;
    unsigned int volume; // ml
};

struct RecipeStep {
    RecipeStepAction actions[PUMP_COUNT];
    int actionCount;
};

struct Recipe {
    String name;
    RecipeStep steps[MAX_STEPS_PER_RECIPE];
    int stepCount;
};

// --- Function Declarations ---

// Pump Setup & Settings
void setupPumps();
void updatePumpSettings(const Pump newPumps[]);

// Direct Pump & Volume Control
void setPumpState(int pumpIndex, int state);
void setVolumes(const unsigned int newVolumes[]);
void startDosingSequence();
void stopCurrentSequence();
void calibratePump(int pumpIndex);

// Recipe Management
void resetAllCounters();

// Recipe Execution
void startRecipe(int recipeIndex);
void updateExecution();


// --- Extern Variable Declarations ---
extern Pump pumps[PUMP_COUNT];
extern Recipe recipes[MAX_RECIPES];
extern unsigned int recipeRunCounts[MAX_RECIPES]; // New array for counters
extern int relayStates[PUMP_COUNT];
extern unsigned long totalRunCount;

// State Machine
enum SystemState { IDLE, DOSING_MANUAL, EXECUTING_RECIPE };
extern SystemState systemState;

// Execution state variables needed for status reporting
extern int currentRecipeIndex;
extern int currentStepIndex;
extern unsigned long stepStartTime;
extern unsigned long longestStepTime;


#endif // PUMP_CONTROL_H
