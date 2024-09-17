const http = require('http');

const unixSocketServerHost = 'localhost';
const unixSocketServerPort = 4001; // Port of the UNIX socket server

// Create an HTTP server
const server = http.createServer((req, res) => {
    // Set CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*'); // Allow requests from any origin
    res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS'); // Allow POST and OPTIONS methods
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type'); // Allow Content-Type header

    if (req.method === 'OPTIONS') {
        // If it's an OPTIONS request, end the response here
        res.writeHead(204);
        res.end();
        return;
    }

    if (req.method === 'POST' && req.url === '/send-command') {
        let body = '';

        req.on('data', chunk => {
            body += chunk;
        });

        req.on('end', () => {
            const { command } = JSON.parse(body);

            // Send the command to the UNIX socket server via HTTP
            const options = {
                hostname: unixSocketServerHost,
                port: unixSocketServerPort,
                path: '/relay-command',
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Content-Length': Buffer.byteLength(body)
                }
            };

            const forwardReq = http.request(options, (forwardRes) => {
                let responseBody = '';
                forwardRes.on('data', chunk => {
                    responseBody += chunk;
                });

                forwardRes.on('end', () => {
                    console.log('Response from UNIX socket server:', responseBody);
                });
            });

            forwardReq.on('error', (error) => {
                console.error('Error sending to UNIX socket server:', error.message);
            });

            // Send the data
            forwardReq.write(body);
            forwardReq.end();

            // Respond to the client
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({ status: 'Command forwarded', command: command }));
        });
    } else {
        res.writeHead(404, { 'Content-Type': 'text/plain' });
        res.end('Not Found');
    }
});

// Start the HTTP server on port 3000
server.listen(3000, () => {
    console.log('HTTP server listening on port 3000');
});
