document.getElementById("currentTime").innerText = new Date().toLocaleTimeString();

let currentPath = '/logger/'; // speichert den aktuellen Pfad

function createListItem(content, className = 'list-group-item d-flex justify-content-between align-items-center') {
    const li = document.createElement('li');
    li.className = className;
    li.innerHTML = content;
    return li;
}

async function fetchFiles(path = '/logger/') {
    console.log("fetchFiles() aufgerufen.");
    currentPath = path;

    try {
        const files = await httpRequest(`/list-dir?path=${path}`);
        console.log("Empfangene Dateien:", files);
        
        const fileList = document.getElementById('file-list');
        fileList.innerHTML = '';
        
        if (path !== '/logger/') {
            const backItem = createListItem(`🔙 <a href="#" onclick="fetchFiles('/logger/')">Zurück zum Hauptordner</a>`);
            fileList.appendChild(backItem);
        }
        
        const sortedFiles = files.sort((a, b) => {
            if (a.type === 'directory' && b.type !== 'directory') return -1;
            if (a.type !== 'directory' && b.type === 'directory') return 1;
            return a.name.localeCompare(b.name);
        });

        sortedFiles.forEach(file => {
            let content;
            if (file.type === 'directory') {
                content = `<span>📁 <a href="#" onclick="fetchFiles('${path}${file.name}/')">${file.name}</a></span>`;
            } else {
                content = `
                    📄 ${file.name}
                    <div class="d-flex align-items-center">
                        <span class="badge badge-primary badge-pill">${file.size} bytes</span>
                        <a href="/download?path=${path}${file.name}" target="_blank" class="btn btn-sm btn-success ml-3">Download</a>
                        <button onclick="deleteFile('${path}${file.name}')" class="btn btn-sm btn-danger ml-3">Löschen</button>
                    </div>
                `;
            }
            fileList.appendChild(createListItem(content));
        });
        
    } catch (error) {
        console.error("Fehler beim Abrufen der Dateiliste:", error);
    }
}

async function deleteFile(filePath) {
    const confirmation = confirm("Möchten Sie diese Datei wirklich löschen?");
    if (confirmation) {
        try {
            const response = await httpRequest(`/delete?path=${filePath}`, 'DELETE');
            if (response.success) {
                alert('Datei erfolgreich gelöscht!');
                fetchFiles(currentPath);
            } else {
                alert('Fehler beim Löschen der Datei.');
            }
        } catch (error) {
            console.error("Fehler beim Löschen der Datei:", error);
        }
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
    fetch('/upload', {
        method: 'POST',
        body: formData
    })
    .then(response => {
        if (!response.ok) {
            throw new Error(`Status: ${response.status}, Text: ${response.statusText}`);
        }
        console.log("Datei erfolgreich hochgeladen.");
        alert('Datei hochgeladen!');
        fetchFiles();
    })
    .catch(error => {
        console.error("Fehler beim Hochladen der Datei:", error);
    });
}

async function httpRequest(url, method = 'GET', data = null, responseType = 'json') {
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
        if (responseType === 'json') {
            return await response.json();
        } else if (responseType === 'text') {
            return await response.text();
        }
    } else {
        throw new Error(`Status: ${response.status}, Text: ${response.statusText}`);
    }
}

function loadSettings(endpoint, fields) {
    fetch(endpoint)
    .then(response => response.json())
    .then(data => {
        fields.forEach(field => {
            if (document.getElementById(field)) {
                document.getElementById(field).value = data[field] || '';
            }
        });
    })
    .catch(error => {
        handleError(error, `Error fetching settings from ${endpoint}:`);
    });
}

function addFormEventListener(formId, endpoint) {
    const form = document.getElementById(formId);
    if (form) {
        form.addEventListener('submit', function(event) {
            event.preventDefault();
            let formData = new FormData(this);
            fetch(endpoint, {
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
                console.error(`Es gab einen Fehler beim Senden der Daten an ${endpoint}:`, error);
                alert('Es gab einen Fehler beim Senden der Daten.');
            });
        });

        // Hier fügen wir den click-Event-Listener hinzu
        form.addEventListener('click', function(event) {
            if (event.target.closest('a') && event.target.closest('li')) {
                const path = event.target.getAttribute('data-path');
                if (path) {
                    fetchFiles(path);
                    event.preventDefault();
                }
            }
        });
    }
}


function handleError(error, userMessage) {
    console.error(userMessage, error);
    alert(userMessage);
}

async function fetchData(url, elementId, key) {
    try {
        const data = await httpRequest(url);
        if (elementId && key) {
            document.getElementById(elementId).textContent = key ? data[key] : data;
        }
        return data;
    } catch (error) {
        console.error(`Failed to fetch data from ${url}:`, error);
    }
}

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

    let isScanning = false;

    function startScan(type) {
        if (!isScanning) return;
    
        let url, resultElement;
        if (type === "manual") {
            url = "/manualscan";
            resultElement = document.getElementById('manualResults');
            const startregister = document.getElementById('startregister').value;
            const length = document.getElementById('length').value;
            const func = document.getElementById('function').value;
    
            const data = { startregister, length, function: func };
            fetch(url, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(data)
            })
            .then(response => response.text())
            .then(data => {
                resultElement.innerHTML = data;
                setTimeout(() => startScan(type), 1000);
            });
    
        } else if (type === "auto") {
            url = "/autoscan";
            resultElement = document.getElementById('autoResults');
            fetch(url)
            .then(response => response.text())
            .then(data => {
                resultElement.innerHTML = data;
                setTimeout(() => startScan(type), 1000);
            });
        }
    }

    async function loadContent(url) {
        try {
            const data = await httpRequest(url, 'GET', null, 'text');
            document.getElementById('contentContainer').innerHTML = data;
            
            if (url.includes('wlan-settings.html')) {
                loadSettings('/get-wlan-settings', ['ssid', 'password']);
            } else if (url.includes('modbus-settings.html')) {
                loadSettings('/get-modbus-settings', ['deviceAddress', 'baudrate', 'parity', 'stopbits']);
                document.getElementById('modbus-settings-form').addEventListener('submit', saveSettings);
            } else if (url.includes('file-management.html')) {
                fetchFiles(path = '/logger/'); // Hier rufen Sie die fetchFiles Funktion auf
            } else if (url.includes('modbus-scanner.html')) {
                // Überprüfen, ob ein bestimmtes Element aus modbus-scanner.html im DOM vorhanden ist
                if (document.querySelector('.modbus-scanner-class')) { // Ersetzen Sie '.modbus-scanner-class' durch einen tatsächlichen Selektor von modbus-scanner.html
                    document.addEventListener('click', function(event) {
                        switch (event.target.id) {
                            case 'startManual':
                                isScanning = true;
                                startScan("manual");
                                break;
                            case 'stopManual':
                                isScanning = false;
                                break;
                            case 'startAuto':
                                isScanning = true;
                                startScan("auto");
                                break;
                            case 'stopAuto':
                                isScanning = false;
                                break;
                        }
                    });
                }
            }
            
        } catch (error) {
            console.error("Fehler beim Laden des Inhalts:", error);
        }
    }
    
    document.addEventListener('DOMContentLoaded', async function() {
    await loadContent('/wlan-settings.html');
    await fetchData('/get-free-space', 'freeSpace', 'freeSpace');
    await fetchData('/get-status', 'wifiStatus', 'rssi');
    await fetchData('/get-status', 'modbusStatus', 'modbusStatus');
    setInterval(() => fetchData('/get-status', 'wifiStatus', 'rssi'), 10000);
    addFormEventListener('wlan-settings-form', '/set-wlan');
    addFormEventListener('modbus-settings-form', '/save-modbus-settings');
});

