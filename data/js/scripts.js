document.getElementById("currentTime").innerText = new Date().toLocaleTimeString();

let currentPath = '/logger/'; // speichert den aktuellen Pfad
let ws; // Globale Variable für die WebSocket-Verbindung
let requestId = 0;
const pendingRequests = {};


function createListItem(content, className = 'list-group-item d-flex justify-content-between align-items-center') {
    const li = document.createElement('li');
    li.className = className;
    li.innerHTML = content;
    return li;
}

function fetchFiles(path = '/logger/') {
    console.log("fetchFiles() aufgerufen.");
    currentPath = path;

    // Anstatt einen HTTP-Request zu senden, senden wir eine WebSocket-Nachricht
    ws.send(JSON.stringify({ action: 'fetchFiles', path: path }));
}

async function deleteFile(filePath) {
    const confirmation = confirm("Möchten Sie diese Datei wirklich löschen?");
    if (confirmation) {
        // Anstatt einen HTTP-Request zu senden, senden wir eine WebSocket-Nachricht
        ws.send(JSON.stringify({ action: 'deleteFile', path: filePath }));
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

    const reader = new FileReader();
    reader.onload = function(event) {
        const fileData = event.target.result;
        const message = {
            action: 'uploadFile',
            fileName: file.name,
            fileType: file.type,
            data: fileData
        };
        ws.send(JSON.stringify(message));
    };
    reader.onerror = function(error) {
        console.error("Fehler beim Lesen der Datei:", error);
    };
    reader.readAsDataURL(file);
}

async function httpRequest(url, method = 'GET', data = null, responseType = 'json') {
    // Anfrage über WebSocket senden
    const response = await wsRequest('httpRequest', {
        url: url,
        method: method,
        data: data,
        responseType: responseType
    });

    if (response.success) {
        return response.data;
    } else {
        throw new Error(response.error);
    }
}


async function loadSettings(endpoint, fields) {
    try {
        const data = await wsRequest('loadSettings', { endpoint: endpoint });
        fields.forEach(field => {
            if (document.getElementById(field)) {
                document.getElementById(field).value = data[field] || '';
            }
        });
    } catch (error) {
        handleError(error, `Error fetching settings from ${endpoint}:`);
    }
}


function addFormEventListener(formId, actionType) {
    const form = document.getElementById(formId);
    if (form) {
        form.addEventListener('submit', function(event) {
            event.preventDefault();
            let formData = new FormData(this);
            const dataObj = {};
            formData.forEach((value, key) => {
                dataObj[key] = value;
            });

            // Senden Sie die Daten über WebSocket anstatt über HTTP
            ws.send(JSON.stringify({
                action: actionType,
                data: dataObj
            }));
        });

        // Hier fügen wir den click-Event-Listener hinzu
        form.addEventListener('click', function(event) {
            if (event.target.closest('a') && event.target.closest('li')) {
                const path = event.target.getAttribute('data-path');
                if (path) {
                    // Anstatt einen HTTP-Request zu senden, senden wir eine WebSocket-Nachricht
                    ws.send(JSON.stringify({ action: 'fetchFiles', path: path }));
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
    const dataObj = {};
    formData.forEach((value, key) => {
        dataObj[key] = value;
    });

    // Senden Sie die Daten über WebSocket anstatt über HTTP
    ws.send(JSON.stringify({
        action: 'saveModbusSettings',
        data: dataObj
    }));
}


let isScanning = false;

function startScan(type) {
    if (!isScanning) return;

    let action, resultElement;
    const data = {};

    if (type === "manual") {
        action = "manualScan";
        resultElement = document.getElementById('manualResults');
        data.startregister = document.getElementById('startregister').value;
        data.length = document.getElementById('length').value;
        data.function = document.getElementById('function').value;
    } else if (type === "auto") {
        action = "autoScan";
        resultElement = document.getElementById('autoResults');
    }

    // Senden Sie die Daten über WebSocket
    ws.send(JSON.stringify({
        action: action,
        data: data
    }));

    // Der Server sollte eine Antwort mit den Scan-Ergebnissen senden, die von ws.onmessage verarbeitet wird
    ws.onmessage = function(event) {
        const response = JSON.parse(event.data);
        if (response.action === action + "Response") {
            resultElement.innerHTML = response.data;
            if (isScanning) {
                setTimeout(() => startScan(type), 1000);
            }
        }
    };
}


    async function loadContent(url) {
    try {
        // Anfrage über WebSocket senden
        const response = await wsRequest('loadContent', { url: url });
        document.getElementById('contentContainer').innerHTML = response.data;

        if (url.includes('wlan-settings.html')) {
            loadSettings('/get-wlan-settings', ['ssid', 'password']);
        } else if (url.includes('modbus-settings.html')) {
            loadSettings('/get-modbus-settings', ['deviceAddress', 'baudrate', 'parity', 'stopbits']);
            document.getElementById('modbus-settings-form').addEventListener('submit', saveSettings);
        } else if (url.includes('file-management.html')) {
            fetchFiles(path = '/logger/'); // Hier rufen Sie die fetchFiles Funktion auf
        } else if (url.includes('modbus-scanner.html')) {
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
    } catch (error) {
        console.error("Fehler beim Laden des Inhalts:", error);
    }
    }
    
    function setupWebSocket() {
    // WebSocket-Verbindung herstellen
    ws = new WebSocket('ws://' + location.host + '/ws');

    ws.onopen = function() {
        console.log('WebSocket-Verbindung hergestellt.');
        // Hier können Sie eine Nachricht an den Server senden, z.B.:
        // ws.send('Hallo Server!');
    };

    function wsRequest(action, data = null) {
    return new Promise((resolve, reject) => {
        requestId++;
        const message = {
            id: requestId,
            action: action,
            data: data
        };
        pendingRequests[requestId] = { resolve, reject };
        ws.send(JSON.stringify(message));
    });
    }

    ws.onmessage = function(event) {
    console.log('WebSocket-Nachricht empfangen:', event.data);
    const response = JSON.parse(event.data);

    // Überprüfen Sie, ob es sich um eine Antwort auf eine wsRequest-Anfrage handelt
    if (pendingRequests[response.id]) {
        const { resolve, reject } = pendingRequests[response.id];
        if (response.success) {
            resolve(response.data);
        } else {
            reject(new Error(response.error));
        }
        delete pendingRequests[response.id];
    } else {
        // Behandeln Sie spezifische Aktionen
        switch (response.action) {
            case 'updateFiles':
                updateFileList(response.files);
                break;
            case 'deleteFileResponse':
                if (response.success) {
                    alert('Datei erfolgreich gelöscht!');
                    fetchFiles(currentPath);
                } else {
                    alert('Fehler beim Löschen der Datei.');
                    console.error("Fehler beim Löschen der Datei:", response.error);
                }
                break;
            case 'uploadFileResponse':
                if (response.success) {
                    console.log("Datei erfolgreich hochgeladen.");
                    alert('Datei hochgeladen!');
                    fetchFiles();
                } else {
                    console.error("Fehler beim Hochladen der Datei:", response.error);
                    alert('Fehler beim Hochladen der Datei.');
                }
                break;
            // ... andere Aktionen können hier hinzugefügt werden ...
        }
    }
};

    
    ws.onclose = function() {
        console.log('WebSocket-Verbindung geschlossen.');
        // Optional: Sie können versuchen, die Verbindung automatisch wiederherzustellen
        setTimeout(setupWebSocket, 5000);
    };

    ws.onerror = function(error) {
        console.error('WebSocket-Fehler:', error);
    };
    }

    function updateFileList(files) {
        console.log("Empfangene Dateien:", files);
        
        const fileList = document.getElementById('file-list');
        fileList.innerHTML = '';
        
        if (currentPath !== '/logger/') {
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
                content = `<span>📁 <a href="#" onclick="fetchFiles('${currentPath}${file.name}/')">${file.name}</a></span>`;
            } else {
                content = `
                    📄 ${file.name}
                    <div class="d-flex align-items-center">
                        <span class="badge badge-primary badge-pill">${file.size} bytes</span>
                        <a href="/download?path=${currentPath}${file.name}" target="_blank" class="btn btn-sm btn-success ml-3">Download</a>
                        <button onclick="deleteFile('${currentPath}${file.name}')" class="btn btn-sm btn-danger ml-3">Löschen</button>
                    </div>
                `;
            }
            fileList.appendChild(createListItem(content));
        });
    }

    document.addEventListener('DOMContentLoaded', async function() {
    setupWebSocket();
    await loadContent('/wlan-settings.html');
    await fetchData('/get-free-space', 'freeSpace', 'freeSpace');
    await fetchData('/get-status', 'wifiStatus', 'rssi');
    await fetchData('/get-status', 'modbusStatus', 'modbusStatus');
    setInterval(() => fetchData('/get-status', 'wifiStatus', 'rssi'), 10000);
    addFormEventListener('wlan-settings-form', '/set-wlan');
    addFormEventListener('modbus-settings-form', '/save-modbus-settings');
});

