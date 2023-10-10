document.getElementById("currentTime").innerText = new Date().toLocaleTimeString();

function fetchStatusUsingXHR() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/get-status', true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    
    xhr.onload = function() {
        if (xhr.status >= 200 && xhr.status < 400) {
            try {
                var correctedResponse = "{" + xhr.responseText.slice(0, -1) + "}";  // Removing the trailing comma and wrapping in curly braces
                var data = JSON.parse(correctedResponse);
                
                console.log("JSON-Daten:", data);
                document.getElementById('wifiStatus').textContent = "RSSI: " + data.rssi;
                document.getElementById('modbusStatus').textContent = data.modbusStatus;
                // ... für andere Datenpunkte, sobald Sie sie haben ...
            } catch (e) {
                console.error("Failed to parse JSON:", e, "Response:", xhr.responseText);
            }
        } else {
            console.error('Server returned an error:', xhr.status);
        }
    };

    xhr.onerror = function() {
        console.error('Connection error');
    };

    xhr.send();
}


// Rufen Sie die Funktion wie gewohnt auf


function adjustIframeHeight() {
    var iframe = document.getElementById("embeddedContent");
    if(iframe) {
        iframe.style.height = iframe.contentWindow.document.body.scrollHeight + 'px';
    }
}

function loadContent(url) {
    const iframe = document.getElementById("embeddedContent");
    iframe.src = url;
}

window.addEventListener('DOMContentLoaded', (event) => {
    fetchStatusUsingXHR();
             // Initialer Aufruf
    setInterval(fetchStatusUsingXHR, 10000); // Regelmäßige Updates alle 10 Sekunden
});
