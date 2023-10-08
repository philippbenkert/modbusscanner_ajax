

function getCurrentBrowserTime() {
    const now = new Date();
    const timeString = now.getHours() + ":" + now.getMinutes() + ":" + now.getSeconds();
    return timeString;
}

function updateStatus() {
    console.log("updateStatus() aufgerufen");
    // ... (bereits vorhandener Code) ...
    const currentBrowserTime = getCurrentBrowserTime();
    console.log("Aktuelle Browserzeit:", currentBrowserTime);
    document.querySelector("#browserTime").textContent = "Browser Zeit: " + currentBrowserTime;
}

document.getElementById("currentTime").innerText = new Date().toLocaleTimeString();

console.log("Script gestartet!");

function fetchStatus() {
    console.log("fetchStatus() aufgerufen");
    fetch('/get-status')
        .then(response => {
            console.log("Antwort erhalten:", response);
            return response.json();
        })
        .then(data => {
            console.log("JSON-Daten:", data);
            document.getElementById('currentTime').textContent = data.currentTime;
            document.getElementById('wifiStatus').textContent = data.wifiStrength;
            // ... für andere Datenpunkte, sobald Sie sie haben ...
        })
        .catch(error => {
            console.error('Fehler beim Abrufen des Status:', error);
        });
}

// Annahme, dass eine getCurrentBrowserTime-Funktion existiert
function getCurrentBrowserTime() {
    console.log("getCurrentBrowserTime() aufgerufen");
    return new Date().toLocaleTimeString();
}

function loadContent(url) {
    const iframe = document.getElementById("embeddedContent");
    iframe.src = url;
}
