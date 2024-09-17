const WebSocket = require('./node_modules/ws');

// Create a WebSocket server listening on port 8080
const wss = new WebSocket.Server({ port: 8080 });

wss.on('connection', (ws) => {
    console.log('New client connected.');

    // Handle incoming messages
    ws.on('message', (message) => {
        try {
            // Log the received message
            console.log('Received message:', message);

            // Parse the JSON-like data
            const data = JSON.parse(message);

            if (data.points && Array.isArray(data.points)) {
                console.log('Broadcasting data to clients:', JSON.stringify(data)); // Log the data being sent

                // Broadcast the message to all connected WebSocket clients
                wss.clients.forEach((client) => {
                    if (client.readyState === WebSocket.OPEN) {
                        client.send(JSON.stringify(data)); // Make sure to send a valid JSON string
                    }
                });
            } else {
                console.log('Invalid data format received:', message);
            }
        } catch (error) {
            console.error('Error parsing JSON:', error);
        }
    });

    ws.on('close', () => {
        console.log('Client disconnected.');
    });

    ws.on('error', (error) => {
        console.error('WebSocket error:', error.message);
    });
});

console.log('WebSocket server is listening on ws://localhost:8080');
