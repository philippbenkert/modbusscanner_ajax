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
                document.getElementById('deviceAddress').value = data.deviceAddress || '1';
                document.getElementById('baudrate').value = data.baudrate || '19200';
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
        // URL des aktuellen Fensters abrufen
        const currentURL = window.location.href;
    
        // Überprüfen Sie, ob die aktuelle URL 'modbus-settings.html' oder 'modbus-scanner.html' enthält
        if (currentURL.indexOf('modbus-settings.html') !== -1 || currentURL.indexOf('modbus-scanner.html') !== -1) {
            loadModbusSettings();
        }
    
        // Überprüfen Sie, ob die aktuelle URL 'wlan-settings.html' enthält
        if (currentURL.indexOf('wlan-settings.html') !== -1) {
            loadWLANSettings();
        }
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





function httpRequest(url, method = 'GET', data = null) {
    return new Promise((resolve, reject) => {
        const xhr = new XMLHttpRequest();
        xhr.open(method, url, true);
        xhr.onload = function() {
            if (this.status >= 200 && this.status < 300) {
                resolve(JSON.parse(this.responseText));
            } else {
                reject(new Error(`Status: ${this.status}, Text: ${this.statusText}`));
            }
        };
        xhr.onerror = function() {
            reject(new Error('Netzwerkfehler'));
        };
        if (method === 'POST' && data) {
            if (data instanceof FormData) {
                xhr.send(data);
            } else {
                xhr.setRequestHeader('Content-Type', 'application/json;charset=UTF-8');
                xhr.send(JSON.stringify(data));
            }
        } else {
            xhr.send();
        }
    });
}

let currentPath = '/'; // speichert den aktuellen Pfad

async function fetchFiles(path = '/') {
    console.log("fetchFiles() aufgerufen.");
    currentPath = path; // Aktualisieren des aktuellen Pfads

    try {
        const files = await httpRequest(`/list-dir?path=${path}`);
        console.log("Empfangene Dateien:", files);
        
        const fileList = document.getElementById('file-list');
        fileList.innerHTML = '';
        
        if (path !== '/') {
            // Hinzufügen einer Option zum Zurückgehen
            const li = document.createElement('li');
            li.className = 'list-group-item d-flex justify-content-between align-items-center';
            li.innerHTML = `🔙 <a href="#" onclick="fetchFiles('/')">Zurück zum Hauptordner</a>`;
            fileList.appendChild(li);
        }
        
        // Sortieren: Erst Ordner, dann Dateien. Beide alphanumerisch.
        const sortedFiles = files.sort((a, b) => {
            if (a.type === 'directory' && b.type !== 'directory') return -1;
            if (a.type !== 'directory' && b.type === 'directory') return 1;
            return a.name.localeCompare(b.name); // Alphanumerische Sortierung
        });

        sortedFiles.forEach(file => {
            const li = document.createElement('li');
            li.className = 'list-group-item d-flex justify-content-between align-items-center';
            
            if (file.type === 'directory') {
                li.innerHTML = `<span>📁 <a href="#" onclick="fetchFiles('${path}${file.name}/')">${file.name}</a></span>`;
            } else {
                li.innerHTML = `📄 ${file.name} <span class="badge badge-primary badge-pill">${file.size} bytes</span> <a href="/download?path=${path}${file.name}" target="_blank" class="btn btn-sm btn-success">Download</a>`;
            }
            
            fileList.appendChild(li);
        });
    } catch (error) {
        console.error("Fehler beim Abrufen der Dateiliste:", error);
    }
}



function uploadFile() {
    console.log("uploadFile() aufgerufen.");
    const fileInput = document.getElementById('upload-file');
    const file = fileInput.files[0];
    if (!file) {
        console.warn("Keine Datei zum Hochladen ausgewählt.");
        return;
    }
    const formData = new FormData();
    formData.append('file', file);
    httpRequest('/upload', 'POST', formData)
    .then(() => {
        console.log("Datei erfolgreich hochgeladen.");
        alert('Datei hochgeladen!');
        fetchFiles();
    })
    .catch(error => {
        console.error("Fehler beim Hochladen der Datei:", error);
    });
}

fetchFiles();

document.addEventListener('DOMContentLoaded', function() {
    // Abrufen des Status
    fetchStatus();

    // Event-Listener für das Formular hinzufügen
    let form = document.getElementById('modbus-settings-form');
    if (form) {
        form.addEventListener('submit', saveSettings);
    }
});
