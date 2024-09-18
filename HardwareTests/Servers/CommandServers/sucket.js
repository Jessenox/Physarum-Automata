const http = require('http');
const net = require('net');
const fs = require('fs');
const socketPath = '/tmp/unixSocket10'; // UNIX socket path

// Remove the socket file if it already exists
if (fs.existsSync(socketPath)) {
    fs.unlinkSync(socketPath);
}

// Variable to store the C++ client socket
let cppClientSocket = null;

// Create the UNIX socket server to communicate with the C++ program
const unixSocketServer = net.createServer((socket) => {
    console.log('Connected to C++ program.');

    // Store the client socket to use for sending commands later
    cppClientSocket = socket;

    socket.on('error', (err) => {
        console.error('UNIX socket error:', err.message);
    });

    socket.on('close', () => {
        console.log('C++ program disconnected.');
        cppClientSocket = null;
    });
});

// Start listening on the UNIX socket
unixSocketServer.listen(socketPath, () => {
    console.log('UNIX socket server listening on', socketPath);
});

// Create the second HTTP server to receive commands and relay them to the UNIX socket
const relayServer = http.createServer((req, res) => {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    if (req.method === 'POST' && req.url === '/relay-command') {
        let body = '';

        req.on('data', chunk => {
            body += chunk;
        });

        req.on('end', () => {
            const { command } = JSON.parse(body);
            console.log('Received command:', command);

            // Check if the C++ client is connected
            if (cppClientSocket) {
                // Send the command to the C++ client via the stored socket
                cppClientSocket.write(command);
                console.log('Command sent to C++ program:', command);
            } else {
                console.error('No C++ client connected to the UNIX socket.');
            }

            // Respond to the first HTTP server
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ status: 'Command received', command: command }));
        });
    } else {
        res.writeHead(404, { 'Content-Type': 'text/plain' });
        res.end('Not Found');
    }
});

// Start the second HTTP server on port 4001
relayServer.listen(25565, () => {
    console.log('Relay HTTP server listening on port 4001');
});
