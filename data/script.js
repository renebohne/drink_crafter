const PUMP_COUNT = 8;
const CALIBRATION_RUN_DURATION_MS = 10000;
let currentData = { pumps: [], recipes: [], relayStates: [], systemState: 'IDLE', totalRunCount: 0 };
let progressModal;

function updateProgress() {
    const progressModalEl = document.getElementById('progressModal');
    const isModalVisible = progressModalEl.classList.contains('show');

    if (currentData.systemState === 'IDLE') {
        if (isModalVisible) {
            progressModalEl.blur(); 
            progressModal.hide();
        }
        return;
    }

    if (!currentData.progress) return;

    if (!isModalVisible) {
        progressModal.show();
    }
    
    const { taskName, currentStep, totalSteps, stepStartTime, stepDuration, currentTime } = currentData.progress;
    
    document.getElementById('progressTaskName').textContent = taskName;
    document.getElementById('progressStepLabel').textContent = `Step ${currentStep} of ${totalSteps}`;
    
    const elapsedTime = currentTime - stepStartTime;
    const progressPercent = Math.min(100, (elapsedTime / stepDuration) * 100);
    
    const progressBar = document.getElementById('progress-bar');
    progressBar.style.width = `${progressPercent}%`;
    progressBar.textContent = `${Math.round(progressPercent)}%`;
}

function render() {
    const pumpControls = document.getElementById('pump-controls');
    const recipeList = document.getElementById('recipe-list');
    const settingsInputs = document.getElementById('settings-inputs');
    const calibrationControls = document.getElementById('calibration-controls');
    const statsList = document.getElementById('recipe-stats');
    const volInputs = document.getElementById('volume-inputs');
    
    // --- Build UI sections only once on first render ---
    if (pumpControls.children.length === 0) {
        let pumpControlsHtml = '';
        let settingsHtml = '';
        let calHtml = '';
        let volHtml = '';

        currentData.pumps.forEach((pump, i) => {
            pumpControlsHtml += `
                <div class="form-check form-switch mb-2">
                    <input class="form-check-input" type="checkbox" role="switch" id="pump-toggle-${i}" onchange="togglePump(${i})">
                    <label class="form-check-label" for="pump-toggle-${i}">${pump.name}</label>
                </div>`;
            
            settingsHtml += `
                <div class="row align-items-center mb-2 border-bottom pb-2">
                    <div class="col-12 col-md-4 mb-2 mb-md-0"><input type="text" class="form-control form-control-sm" id="name-${i}" value="${pump.name}" placeholder="Pump Name"></div>
                    <div class="col-6 col-md-4"><div class="input-group input-group-sm"><input type="number" class="form-control form-control-sm" id="cal-${i}" value="${pump.calibration}" min="0"><span class="input-group-text">ms/ml</span></div></div>
                    <div class="col-6 col-md-4"><select class="form-select form-select-sm" id="relay-${i}">${currentData.pumps.map((p, j) => `<option value="${j}" ${j === pump.assignedRelay ? 'selected' : ''}>Relay ${j}</option>`).join('')}</select></div>
                </div>`;

            calHtml += `
                <div class="input-group input-group-sm mb-2">
                    <button class="btn btn-secondary" type="button" onclick="calibratePump(${i})">${pump.name}</button>
                    <input type="number" class="form-control" placeholder="Measured ml" oninput="calculateCalibration(${i}, this.value)">
                    <span class="input-group-text">ml</span>
                </div>`;
            
            volHtml += `<div class="input-group input-group-sm mb-2"><span class="input-group-text">${pump.name}</span><input type="number" class="form-control form-control-sm" id="vol-${i}" value="0" min="0"><span class="input-group-text">ml</span></div>`;
        });
        
        pumpControls.innerHTML = pumpControlsHtml;
        settingsInputs.innerHTML = settingsHtml;
        calibrationControls.innerHTML = calHtml;
        volInputs.innerHTML = volHtml;
    }

    // --- Update values on every render (respecting focus) ---
    let anyPumpOn = false;
    recipeList.innerHTML = '';
    statsList.innerHTML = '';

    currentData.pumps.forEach((pump, i) => {
        const relayState = currentData.relayStates[pump.assignedRelay] || 0;
        if (relayState === 1) anyPumpOn = true;

        const toggle = document.getElementById(`pump-toggle-${i}`);
        if (document.activeElement !== toggle) {
            toggle.checked = relayState === 1;
        }

        const nameInput = document.getElementById(`name-${i}`);
        if (document.activeElement !== nameInput) nameInput.value = pump.name;

        const calInput = document.getElementById(`cal-${i}`);
        if (document.activeElement !== calInput) calInput.value = pump.calibration;

        const relaySelect = document.getElementById(`relay-${i}`);
        if (document.activeElement !== relaySelect) relaySelect.value = pump.assignedRelay;
    });

    document.getElementById('all-pumps-toggle').checked = anyPumpOn;

    currentData.recipes.forEach((recipe, i) => {
        if (recipe.name) {
            // --- New logic to get ingredients ---
            const ingredients = new Set();
            recipe.steps.forEach(step => {
                step.actions.forEach(action => {
                    if (currentData.pumps[action.pumpIndex]) {
                        const pumpName = currentData.pumps[action.pumpIndex].name;
                        ingredients.add(pumpName);
                    }
                });
            });
            const ingredientsString = Array.from(ingredients).join(', ');

            // Render recipe list as large buttons with ingredients
            recipeList.innerHTML += `
                <div class="col-6 col-md-4 p-2">
                    <button class="btn btn-primary w-100 recipe-btn d-flex flex-column justify-content-center" onclick="runRecipe(${i})">
                        <span>${recipe.name}</span>
                        <small class="ingredient-list">${ingredientsString}</small>
                    </button>
                </div>`;
            
            statsList.innerHTML += `<li class="list-group-item d-flex justify-content-between align-items-center">${recipe.name}<span class="badge bg-secondary">${recipe.runCount}</span></li>`;
        }
    });

    document.getElementById('total-runs').textContent = currentData.totalRunCount;
    const statusText = document.getElementById('status-text');
    const statusIndicator = document.getElementById('status-indicator');
    if (currentData.systemState === 'DOSING_MANUAL') {
        statusIndicator.className = 'bg-info'; statusText.textContent = 'Manual Dosing';
    } else if (currentData.systemState === 'EXECUTING_RECIPE') {
        statusIndicator.className = 'bg-success'; statusText.textContent = 'Recipe Running';
    } else if (currentData.systemState === 'CALIBRATING') {
        statusIndicator.className = 'bg-warning'; statusText.textContent = 'Calibrating';
    } else {
        statusIndicator.className = 'bg-secondary'; statusText.textContent = 'Idle';
    }

    updateProgress();
}


async function fetchStatus() {
    try {
        const response = await fetch('/status');
        if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
        currentData = await response.json();
        render();
    } catch (error) {
        console.error("Failed to fetch status:", error);
        document.getElementById('status-text').textContent = 'Connection Error';
        document.getElementById('status-indicator').className = 'bg-danger';
        const progressModalEl = document.getElementById('progressModal');
        if (progressModalEl.classList.contains('show')) {
            progressModal.hide();
        }
    }
}

// --- API Calls ---
async function apiPost(endpoint, body) {
    try {
        const response = await fetch(endpoint, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
        if (!response.ok) throw new Error(await response.text());
        // After a successful post, we fetch the status again to get the latest saved data.
        await fetchStatus();
        return true;
    } catch (e) { console.error(e); alert(`Error: ${e.message}`); return false; }
}
async function apiGet(endpoint) {
    try { 
        await fetch(endpoint); 
        // We don't need to wait for a response for simple GET commands,
        // the next status poll will catch the update.
    } catch (e) { console.error(e); }
}

// --- Control Panel ---
function togglePump(pumpIndex) {
    const toggle = document.getElementById(`pump-toggle-${pumpIndex}`);
    const newState = toggle.checked ? 1 : 0;
    apiGet(`/pump/${pumpIndex}/${newState}`);
}

function toggleAllPumps() {
    const allPumpsToggle = document.getElementById('all-pumps-toggle');
    const newState = allPumpsToggle.checked ? 1 : 0;
    for (let i = 0; i < PUMP_COUNT; i++) {
        fetch(`/pump/${i}/${newState}`);
    }
    setTimeout(fetchStatus, 500);
}

function setVolumes() {
    const volumes = [];
    for (let i = 0; i < PUMP_COUNT; i++) volumes.push(parseInt(document.getElementById(`vol-${i}`).value, 10));
    apiPost('/volumes', { volumes });
}
function startDosing() { apiGet('/start_dosing'); }
function stopSequence() { 
    if (confirm('Are you sure you want to stop the current run?')) {
        apiGet('/stop'); 
    }
}

// --- Settings Panel ---
function saveSettings() {
    const newPumps = [];
    const assignedRelays = new Set();
    for (let i = 0; i < PUMP_COUNT; i++) {
        const assignedRelay = parseInt(document.getElementById(`relay-${i}`).value, 10);
        if (assignedRelays.has(assignedRelay)) { alert(`Error: Relay ${assignedRelay} is assigned to multiple pumps.`); return; }
        assignedRelays.add(assignedRelay);
        newPumps.push({
            name: document.getElementById(`name-${i}`).value,
            calibration: parseInt(document.getElementById(`cal-${i}`).value, 10),
            assignedRelay: assignedRelay
        });
    }
    apiPost('/settings', { pumps: newPumps });
}

function resetCounters() {
    const pin = prompt("To reset all counters, please enter the PIN:", "");
    if (pin === "0509") {
        apiPost('/reset_counters', {});
        alert("All counters have been reset.");
    } else if (pin !== null) {
        alert("Incorrect PIN.");
    }
}

function calibratePump(pumpIndex) {
    apiGet(`/calibrate/${pumpIndex}`);
}

function calculateCalibration(pumpIndex, measuredVolume) {
    const volume = parseFloat(measuredVolume);
    if (isNaN(volume) || volume <= 0) return;

    const msPerMl = Math.round(CALIBRATION_RUN_DURATION_MS / volume);
    const calInput = document.getElementById(`cal-${pumpIndex}`);
    if (calInput) {
        calInput.value = msPerMl;
    }
}

// --- Recipe Management ---
async function downloadRecipes() {
    try {
        const response = await fetch('/recipes');
        const recipes = await response.json();
        const content = JSON.stringify(recipes, null, 2);
        const blob = new Blob([content], { type: 'application/json' });
        const a = document.createElement('a');
        a.href = URL.createObjectURL(blob);
        a.download = 'recipes.json';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
    } catch (e) {
        console.error(e);
        alert('Failed to download recipes.');
    }
}

function uploadRecipes(file) {
    if (!file) return;
    const reader = new FileReader();
    reader.onload = async (e) => {
        try {
            // Basic validation to check if it's a valid JSON
            JSON.parse(e.target.result);
            const response = await fetch('/recipes', {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: e.target.result
            });
            if (!response.ok) throw new Error(await response.text());
            alert('Recipes uploaded successfully! The device will now reboot.');
        } catch (err) {
            console.error(err);
            alert('Upload failed. Please ensure the file is a valid JSON.');
        }
    };
    reader.readAsText(file);
}

// --- Recipe Panel ---
function runRecipe(recipeIndex) { apiGet(`/run_recipe/${recipeIndex}`); }

window.onload = () => {
    progressModal = new bootstrap.Modal(document.getElementById('progressModal'));
    
    const progressModalEl = document.getElementById('progressModal');
    progressModalEl.addEventListener('shown.bs.modal', () => {
        document.body.classList.add('modal-open-noscroll');
    });
    progressModalEl.addEventListener('hidden.bs.modal', () => {
        document.body.classList.remove('modal-open-noscroll');
    });

    fetchStatus();
    setInterval(fetchStatus, 1000);
};
