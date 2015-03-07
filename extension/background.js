var powered = false;
var discovering = false;
var lights = {};

/**
 *  Determines the light number from its name and returns it
 */
function getLightNum(name) {
    if (name.indexOf("Light") != 0) {
        //Not a light
        return undefined;
    }
    return parseInt(name.substr(-2));
}

/**
 *  Populates the lights dictionary from the current devices
 */
function findLights() {
    chrome.bluetooth.getDevices(function(devices) {
        for (var i = 0; i < devices.length; i++) {
            updateDevice(devices[i]);
        }
    });
}

/**
 *  Creates a socket connection to a light
 */
function connectLight(num) {
    var uuid = "1105";
    chrome.bluetoothSocket.create(function(createInfo) {
        lights[num].socketId = createInfo.socketId;
        chrome.bluetoothSocket.connect(createInfo.socketId,
            lights[num].device.address, uuid, onConnectedCallback);
    });
}

/** 
 *  Updates or adds a device to the lights dictionary
 */
function updateDevice(device) {
    var num = getLightNum(device.name);
    if (num != undefined) {
        lights[num] = {device: undefined};
        lights[num].device = device;
        console.log("Found light", num, "at address", device.address);
        connectLight(num);
    }
}

/** 
 *  Removes a device from the lights dictionary
 */
function removeDevice(device) {
    var num = getLightNum(device.name);
    if (num != undefined) {
        delete lights[num];
        console.log("Removed light", num);
    }
}

//Event listeners for device notifications
chrome.bluetooth.onDeviceAdded.addListener(updateDevice);
chrome.bluetooth.onDeviceChanged.addListener(updateDevice);
chrome.bluetooth.onDeviceRemoved.addListener(removeDevice);

chrome.bluetooth.onAdapterStateChanged.addListener(updateAdapterState);
chrome.bluetooth.getAdapterState(updateAdapterState);

/**
 *  Updates the adapter state and discovers devices if possible
 */
function updateAdapterState(adapter) {
    if (adapter.powered != powered) {
        powered = adapter.powered;
        if (powered) {
            console.log("Adapter radio is on");
            findLights();

            // // Now begin the discovery process.
            // if (!discovering) {
            //     chrome.bluetooth.startDiscovery(function() {
            //         if (chrome.runtime.lastError) {
            //             console.error("startDiscovery failed: " + chrome.runtime.lastError.message);
            //         } else {
            //             discovering = true;
            //             // Stop discovery after 30 seconds.
            //             setTimeout(function() {
            //                 chrome.bluetooth.stopDiscovery(function() {
            //                     discovering = false;
            //                 });
            //             }, 30000);
            //         }
            //     });
            // }
        } else {
            console.log("Adapter radio is off");
        }
    }
}

//Socket connection
function onConnectedCallback() {
    if (chrome.runtime.lastError) {
        console.error("Connection failed: " + chrome.runtime.lastError.message);
    } else {
        // Profile implementation here.
        setRGB(num, 255, 0, 0);
        console.log("Connected to a light");
    }
}

/**
 *  Sets a light to an RGB value
 *  @param num      The light number to change
 *  @param red      The new red value of the light
 *  @param green    The new green value of the light
 *  @param blue     The new blue value of the light
 */
function setRGB(num, red, green, blue) {
    var buffer = new ArrayBuffer(8);
    var byteView = new Uint8Array(buffer);
    var checksum = (red + green + blue) % 256;
    byteView = ['S', 'I', 'G', 'M', red, green, blue, checksum];

    chrome.bluetoothSocket.send(lights[num].socketId, buffer, function(bytes_sent) {
        if (chrome.runtime.lastError) {
            console.error("Send failed: " + chrome.runtime.lastError.message);
        } else {
            console.log("Sent " + bytes_sent + " bytes");
        }
    });
}
