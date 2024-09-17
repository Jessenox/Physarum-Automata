'use strict';

const net = require('net');
const fs = require('fs');
const WebSocket = require('./node_modules/ws');

const socketPath = '/tmp/unixSocket8';
const wssUrl = 'ws://18.224.22.196:8093';  // WebSocket server URL

// Remove the socket file if it already exists
if (fs.existsSync(socketPath)) {
    fs.unlinkSync(socketPath);
}

// Create the UNIX socket server
const unixSocketServer = net.createServer();

// WebSocket client
let ws;
function connectWebSocket() {
    ws = new WebSocket(wssUrl);

    ws.on('open', () => {
        console.log('Connected to WebSocket server:', wssUrl);
    });

    ws.on('close', () => {
        console.log('WebSocket connection closed. Reconnecting...');
        setTimeout(connectWebSocket, 1000);  // Attempt to reconnect after 1 second
    });

    ws.on('error', (error) => {
        console.error('WebSocket error:', error.message);
    });
}

// Connect to the WebSocket server
connectWebSocket();

unixSocketServer.listen(socketPath, () => {
    console.log('Server is now listening on', socketPath);
});

// Handle incoming connections
unixSocketServer.on('connection', (socket) => {
    console.log('Got connection!');

    // Receive data from the C++ client
    socket.on('data', (data) => {
        const message = data.toString();
        console.log('Received data:', message);

        // Forward the JSON-like data to the WebSocket server
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(message, (error) => {
                if (error) {
                    console.error('Failed to send data over WebSocket:', error.message);
                }
            });
        } else {
            console.log('WebSocket is not open. Data not sent.');
        }
    });

    // Handle socket end event
    socket.on('end', () => {
        console.log('Client disconnected');
    });
});

// Clean up the socket file on server exit
unixSocketServer.on('close', () => {
    if (fs.existsSync(socketPath)) {
        fs.unlinkSync(socketPath);
    }
});
