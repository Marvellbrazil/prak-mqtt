import { createServer } from 'http';
import path from 'path';
import { fileURLToPath } from 'url';
import express, { json } from 'express';
import { WebSocketServer, WebSocket } from 'ws';
import { startMqttSubscriber } from './subscriber.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const server = createServer(app);
const wss = new WebSocketServer({ server });

app.use(json());
app.use(express.static(path.join(__dirname, 'public')));

let latestData = {};

function broadcast(data) {
  const payload = JSON.stringify(data);
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(payload);
    }
  });
}

wss.on('connection', (ws) => {
  if (Object.keys(latestData).length > 0) {
    ws.send(JSON.stringify({ event: 'initial_state', data: latestData }));
  }
});

startMqttSubscriber((topic, payload) => {
  latestData = {
    ...payload,
    received_at: new Date().toISOString()
  };

  broadcast({
    event: 'telemetry_update',
    topic: topic,
    data: latestData
  });
});

app.get('/api/telemetry/latest', (req, res) => {
  if (!latestData.device_id) {
    return res.status(404).json({ status: 'error', message: 'No data yet' });
  }
  res.json({ status: 'success', data: latestData });
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`Server & WebSocket running on port ${PORT}`);
});
