import mqtt from 'mqtt';

const HIVEMQ_HOST = 'fc8997de959045f2b0e09a13b4521491.s1.eu.hivemq.cloud';
const HIVEMQ_PORT = 8883;
const TOPIC = 'iot/esp32/telemetry';

export function startMqttSubscriber(onMessageCallback) {
  const brokerUrl = `mqtts://${HIVEMQ_HOST}:${HIVEMQ_PORT}`;
  
  const options = {
    username: 'esp32_client',
    password: 'secret123',
    rejectUnauthorized: true,
    reconnectPeriod: 3000,
  };

  const client = mqtt.connect(brokerUrl, options);

  client.on('connect', () => {
    client.subscribe(TOPIC, { qos: 1 });
  });

  client.on('message', (topic, message) => {
    try {
      const data = JSON.parse(message.toString());
      onMessageCallback(topic, data);
    } catch (err) {
      console.error(err.message);
    }
  });

  client.on('error', (err) => {
    console.error(err.message);
  });
}
