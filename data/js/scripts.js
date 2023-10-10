document.getElementById("currentTime").innerText = new Date().toLocaleTimeString();

async function fetchStatus() {
    try {
        console.log("fetchStatus() aufgerufen");
        
        const response = await fetch('/get-status');
        
        if (!response.ok) {
            throw new Error(`HTTP error! Status: ${response.status}`);
        }

        const data = await response.json();
        
        console.log("JSON-Daten:", data);
        document.getElementById('wifiStatus').textContent = "RSSI: " + data.rssi;
        document.getElementById('modbusStatus').textContent = data.modbusStatus;

        // ... für andere Datenpunkte, sobald Sie sie haben ...

    } catch (error) {
        console.error('Fehler beim Abrufen des Status:', error);
    }
}



function loadContent(url) {
    const iframe = document.getElementById("embeddedContent");
    iframe.src = url;
}

window.addEventListener('DOMContentLoaded', (event) => {
    fetchStatus();             // Initialer Aufruf
    setInterval(fetchStatus, 10000); // Regelmäßige Updates alle 10 Sekunden
});
