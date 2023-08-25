clear();
Protocol.CAN.Receiver(); // Start the receiver

// Define the Signal List, Frame List, Receive List, and Wait Time
var SignalList = [
    { id: 0x10  }, // HazardSignal
    { id: 0x11  }, // RightSignal
    { id: 0x12  }, // LeftSignal
    { id: 0x13  }, // FrontSignal
    { id: 0x14  }, // BackSignal
    { id: 0x15  }, // horn
    { id: 0x02  }, // cruizeCtrlSpeed
    { id: 0x09  }, // solarCellOutputEffPercent
    { id: 0x02F4  }, // Battery Status (BATT_ST)
    { id: 0x04F4  }, // Cell Voltage (CELL_VOLT)
    { id: 0x05F4  }, // Cell Temeprature (CELL_TEMP)
    { id: 0x07F4  }, // Fault Information (ALM_INFO)
    // ... Add more signals here
];

// Define the Frame List with ID and Wait Time
var FrameList = [
    { id: 0x08850225  }, // Frame 0
    { id: 0x08850245  }, // Frame 0
    { id: 0x08850265  }, // Frame 0
    { id: 0x08850285  }, // Frame 0
    { id: 0x08950225  }, // Frame 1
    { id: 0x08950245  }, // Frame 1
    { id: 0x08950265  }, // Frame 1
    { id: 0x08950285  }, // Frame 1
    { id: 0x08A50225  }, // Frame 2
    { id: 0x08A50245  }, // Frame 2
    { id: 0x08A50265  }, // Frame 2
    { id: 0x08A50285  }, // Frame 2
    // ... Add more frames here
];

// Define the Receive List with IDs of expected responses
var ReceiveList = [
    0x08F89540, // Request Command for Rear Left
    0x08F91540, // Request Command for Rear Right
    0x08F99540, // Request Command for Front Left
    0x08FA9540  // Request Command for Front Right
];
const WAIT_TIME_MS = 1000; // Wait time between sending messages

// Set CAN communication configuration (TX and RX pins, communication rate)
Protocol.CAN.Configure({
    tx: 1, // Set the TX pin (e.g., channel 1)
    rx: 2, // Set the RX pin (e.g., channel 2)
    rate: 250000 // Set the communication rate to 250 kHz
});

/* // Simulate messages with arbitration and receive functionality
function simulateAndReceive() {
    while (true) {
        // Send a message from the Signal List
        var currentIndex = Math.floor(Math.random() * SignalList.length);
        var signal = SignalList[currentIndex];
        var randomData = generateRandomData(); // Generate random data
        Protocol.CAN.Send(signal.id, false, 0, false, randomData.length, randomData);
        wait(WAIT_TIME_MS);

        // Receive and process messages
        var receivedMessage = Protocol.CAN.Receive();

        if (receivedMessage.length >= 8) {
            var receivedID = receivedMessage[1];
            var receivedData = receivedMessage.slice(8);

            if (ReceiveList.includes(receivedID)) {
                var requestWheel = getWheelForRequest(receivedID);
                var requestedFrames = getRequestedFramesFromData(receivedData);

                if (requestWheel !== null && requestedFrames.length > 0) {
                    for (var frameNumber of requestedFrames) {
                        sendFrameDataForWheelAndFrame(requestWheel, frameNumber);
                    }
                }
            }
        }
    }
}



// Get the requested frames from data
function getRequestedFramesFromData(data) {
    var requestedFrames = [];
    var bitmask = data[0];

    for (var i = 0; i < 3; i++) {
        if ((bitmask >> i) & 1) {
            requestedFrames.push(i);
        }
    }

    return requestedFrames;
}

// Get the wheel associated with a request
function getWheelForRequest(requestID) {
    var index = ReceiveList.indexOf(requestID);

    if (index !== -1) {
        var requestWheel = SignalList[index].destination;
        return requestWheel;
    }

    return null;
}

// Send data for the requested frame for a specific wheel
function sendFrameDataForWheelAndFrame(requestWheel, frameNumber) {
    var frame = FrameList[frameNumber];
    if (frame.wheel === requestWheel) {
        var randomData = generateRandomData(); // Generate random data
        Protocol.CAN.Send(frame.id, false, 0, false, randomData.length, randomData);
        wait(frame.wait);
    }
}



// Simulate arbitration and receive in parallel
ParallelThread(simulateAndReceive);*/

// Generate random data within specified range
function generateRandomData() {
    var data = [];
    for (var i = 0; i < 8; i++) {
        var randomValue = Math.floor(Math.random() * 256); // Random value between 0 and 255
        data.push(randomValue);
    }
    return data;
}

/ Simulate sending messages for each signal
function simulateSending() {
    while (true) {
        for (var signal of SignalList) {
            var randomData = generateRandomData(); // Generate random data
            Protocol.CAN.Send(signal.id, false, 0, false, randomData.length, randomData);
            wait(WAIT_TIME_MS);
        }
    }
}

// Start simulating sending messages
simulateSending();