function saveSettings(event) {
    event.preventDefault(); // Verhindert das Standardverhalten des Formulars

    let formData = new FormData(document.getElementById('modbus-settings-form'));

    fetch('/save-modbus-settings', {
        method: 'POST',
        body: formData
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            alert('Einstellungen erfolgreich gespeichert!');
        } else {
            alert('Fehler beim Speichern der Einstellungen. ' + (data.message || ''));
        }
    })
    .catch(error => {
        console.error("Es gab einen Fehler beim Senden der Daten:", error);
        alert('Es gab einen Fehler beim Senden der Daten.');
    });
    }

    function loadModbusSettings() {
        fetch('/get-modbus-settings')
        .then(response => response.json())
        .then(data => {
            if(data) {
                document.getElementById('deviceAddress').value = data.deviceAddress || '';
                document.getElementById('baudrate').value = data.baudrate || '4800';
                document.getElementById('parity').value = data.parity || 'n';
                document.getElementById('stopbits').value = data.stopbits || '1';
            }
        })
        .catch(error => {
            console.error("Error fetching modbus settings:", error);
        });
    }
    
    
    
    function loadWLANSettings() {
        fetch('/get-wlan-settings')
        .then(response => response.json())
        .then(data => {
            if (data) {
                document.getElementById('ssid').value = data.ssid || '';
                document.getElementById('password').value = data.password || '';
            }
        })
        .catch(error => {
            console.error("Error fetching WLAN settings:", error);
        });
    }
        
    // Aufrufen der Funktionen loadModbusSettings und loadWLANSettings nach dem Laden der Seite
    window.onload = function() {
        loadModbusSettings();
        loadWLANSettings();
    }

    var isScanning = false; // Globale Variable, um zu überprüfen, ob ein Scanvorgang läuft

$(document).ready(function() {
    $('#startManual').click(function() {
        if(!isScanning) {
            isScanning = true;
            startManualScan();
        }
    });

    $('#stopManual').click(function() {
        isScanning = false;
    });

    $('#startAuto').click(function() {
        if(!isScanning) {
            isScanning = true;
            startAutoScan();
        }
    });

    $('#stopAuto').click(function() {
        isScanning = false;
    });
});

function startManualScan() {
    if(isScanning) {
        var startregister = $('#startregister').val();
        var length = $('#length').val();
        var func = $('#function').val();

        // Senden Sie die Daten an den Server
        $.post("/manualscan", { startregister: startregister, length: length, function: func }, function(data) {
            $('#manualResults').html(data); // Setzt die Antwort des Servers als Inhalt von manualResults
            setTimeout(startManualScan, 1000); // Ruft die Funktion nach 1 Sekunde erneut auf, wenn isScanning noch true ist
        });
    }
}

function startAutoScan() {
    if(isScanning) {
        // Senden Sie die Anforderung an den Server
        $.get("/autoscan", function(data) {
            $('#autoResults').html(data); // Setzt die Antwort des Servers als Inhalt von autoResults
            setTimeout(startAutoScan, 1000); // Ruft die Funktion nach 1 Sekunde erneut auf, wenn isScanning noch true ist
        });
    }
}

document.addEventListener('DOMContentLoaded', function() {
    // Abrufen des Status
    fetchStatus();

    // Event-Listener für das Formular hinzufügen
    let form = document.getElementById('modbus-settings-form');
    if (form) {
        form.addEventListener('submit', saveSettings);
    }
});

async function fetchFiles() {
    const response = await fetch('/list-dir');
    const files = await response.json();
    const fileList = document.getElementById('file-list');
    fileList.innerHTML = '';
    files.forEach(file => {
        const li = document.createElement('li');
        li.className = 'list-group-item d-flex justify-content-between align-items-center';
        li.innerHTML = `${file.name} <span class="badge badge-primary badge-pill">${file.size} bytes</span> <a href="/download?path=${file.name}" target="_blank" class="btn btn-sm btn-success">Download</a>`;
        fileList.appendChild(li);
    });
}

function uploadFile() {
    const fileInput = document.getElementById('upload-file');
    const file = fileInput.files[0];
    const formData = new FormData();
    formData.append('file', file);
    fetch('/upload', {
        method: 'POST',
        body: formData
    }).then(() => {
        alert('Datei hochgeladen!');
        fetchFiles();
    });
}

fetchFiles();
