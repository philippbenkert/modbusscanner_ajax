document.getElementById("currentTime").innerText = new Date().toLocaleTimeString();

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

async function loadContent(url) {
    try {
        const data = await httpRequest(url, 'GET', null, 'text');
        document.getElementById('contentContainer').innerHTML = data;
        if (url.includes('modbus-settings.html')) {
            // loadModbusSettings(); // Wenn es eine solche Funktion gibt
        } else if (url.includes('wlan-settings.html')) {
            // loadWLANSettings(); // Wenn es eine solche Funktion gibt
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
});

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
