document.getElementById("currentTime").innerText = new Date().toLocaleTimeString();

function fetchStatus() {
    fetch('/get-status')
        .then(response => response.json())
        .then(data => {
            console.log("JSON-Daten:", data);
            document.getElementById('wifiStatus').textContent = "RSSI: " + data.rssi;
            document.getElementById('modbusStatus').textContent = data.modbusStatus;
            // ... für andere Datenpunkte, sobald Sie sie haben ...
        })
        .catch(error => {
            console.error("Failed to fetch or parse JSON:", error);
        });
}

function updateFreeSpace() {
    fetch('/get-free-space')
        .then(response => response.json())
        .then(data => {
            document.getElementById('freeSpace').textContent = data.freeSpace;
        });
}

function loadContent(url) {
    fetch(url)
        .then(response => response.text())
        .then(data => {
            document.getElementById('contentContainer').innerHTML = data;
            if (url.includes('modbus-settings.html')) {
                loadModbusSettings();
            } else if (url.includes('wlan-settings.html')) {
                loadWLANSettings();
            }
        })
        .catch(error => {
            console.error("Fehler beim Laden des Inhalts:", error);
        });
}

document.addEventListener('DOMContentLoaded', function() {
    loadContent('/wlan-settings.html'); // oder jede andere Standardseite, die Sie anzeigen möchten
    updateFreeSpace();
    fetchStatus();
    setInterval(fetchStatus, 10000); // Regelmäßige Updates alle 10 Sekunden
});

async function httpRequest(url, method = 'GET', data = null) {
    const options = {
        method: method,
        headers: {}
    };

    if (method === 'POST' && data) {
        if (data instanceof FormData) {
            options.body = data;
        } else {
            options.headers['Content-Type'] = 'application/json;charset=UTF-8';
            options.body = JSON.stringify(data);
        }
    }

    const response = await fetch(url, options);
    if (response.ok) {
        return await response.json();
    } else {
        throw new Error(`Status: ${response.status}, Text: ${response.statusText}`);
    }
}
