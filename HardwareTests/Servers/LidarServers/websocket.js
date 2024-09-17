const WebSocket = require('./node_modules/ws');

// Create a WebSocket server listening on port 8080
const wss = new WebSocket.Server({ port: 8093 });

wss.on('connection', (ws) => {
    console.log('New client connected.');

    // Send a welcome message to the client
    ws.send('Welcome! You are connected to the WebSocket server.');

    // Handle incoming messages from clients (optional if clients send data)
    ws.on('message', (message) => {
        try {
            // Parse the incoming message (if sent from another server or client)
            const data = JSON.parse(message);

            if (data.points && Array.isArray(data.points)) {
                console.log('Received LiDAR points:', data.points);

                // Broadcast the message to all connected WebSocket clients
                wss.clients.forEach((client) => {
                    if (client.readyState === WebSocket.OPEN) {
                        client.send(message);  // Forward the message to each client
                    }
                });
            } else {
                console.log('Invalid data format received:', message);
            }
        } catch (error) {
            console.error('Error parsing JSON:', error);
        }
    });

    // Handle client disconnection
    ws.on('close', () => {
        console.log('Client disconnected.');
    });

    // Handle WebSocket errors
    ws.on('error', (error) => {
        console.error('WebSocket error:', error.message);
    });
    
});

console.log('WebSocket server is listening on ws://localhost:8080');
