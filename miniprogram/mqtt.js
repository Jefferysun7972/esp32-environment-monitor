const MQTT_BROKER = 'wss://f4319339.ala.cn-hangzhou.emqxsl.cn:8084/mqtt';
const MQTT_USER = 'jerrysun';
const MQTT_PASS = 'renyi1004';

let socketTask = null;
let pingTimer = null;
let clientId = '';

function randomId() {
  return 'wx_' + Math.random().toString(16).substr(2, 10);
}

function encodeUTF8(str) {
  const buf = [];
  for (let i = 0; i < str.length; i++) {
    const code = str.charCodeAt(i);
    if (code < 0x80) {
      buf.push(code);
    } else if (code < 0x800) {
      buf.push(0xc0 | (code >> 6), 0x80 | (code & 0x3f));
    } else {
      buf.push(0xe0 | (code >> 12), 0x80 | ((code >> 6) & 0x3f), 0x80 | (code & 0x3f));
    }
  }
  return buf;
}

function encodeLength(len) {
  const buf = [];
  do {
    let byte = len & 0x7f;
    len >>= 7;
    if (len > 0) byte |= 0x80;
    buf.push(byte);
  } while (len > 0);
  return buf;
}

function buildConnectPacket() {
  const protocol = encodeUTF8('MQTT');
  const protocolLevel = [4];
  const connectFlags = [0x02 | 0x40 | 0x80]; // clean session, password, username
  const keepAlive = [0x00, 0x3c]; // 60s

  const clientIdBytes = encodeUTF8(clientId);
  const usernameBytes = encodeUTF8(MQTT_USER);
  const passwordBytes = encodeUTF8(MQTT_PASS);

  const payload = [].concat(
    protocol, [0x00, 0x04], // protocol name + length
    protocolLevel,
    connectFlags,
    keepAlive,
    [clientIdBytes.length >> 8, clientIdBytes.length & 0xff], clientIdBytes,
    [usernameBytes.length >> 8, usernameBytes.length & 0xff], usernameBytes,
    [passwordBytes.length >> 8, passwordBytes.length & 0xff], passwordBytes
  );

  const header = [0x10]; // CONNECT
  const lenBytes = encodeLength(payload.length);
  return new Uint8Array([].concat(header, lenBytes, payload)).buffer;
}

function buildSubscribePacket(topics) {
  const payloadParts = [];
  topics.forEach(t => {
    const tBytes = encodeUTF8(t);
    payloadParts.push([tBytes.length >> 8, tBytes.length & 0xff]);
    payloadParts.push(tBytes);
    payloadParts.push([0x00]); // QoS 0
  });
  const payload = [].concat(...payloadParts);
  const packetId = [0x00, 0x01];

  const header = [0x82]; // SUBSCRIBE
  const lenBytes = encodeLength(2 + payload.length);
  return new Uint8Array([].concat(header, lenBytes, packetId, payload)).buffer;
}

function buildPingReqPacket() {
  return new Uint8Array([0xc0, 0x00]).buffer;
}

function parsePublishPacket(buffer) {
  const view = new DataView(buffer);
  let pos = 0;

  const header = view.getUint8(pos++);
  const type = (header >> 4) & 0x0f;

  if (type !== 3) return null; // not PUBLISH

  // read remaining length
  let multiplier = 1;
  let remainingLen = 0;
  let byte;
  do {
    byte = view.getUint8(pos++);
    remainingLen += (byte & 0x7f) * multiplier;
    multiplier *= 128;
  } while ((byte & 0x80) !== 0);

  const topicLen = view.getUint16(pos);
  pos += 2;
  const topic = String.fromCharCode.apply(null, new Uint8Array(buffer, pos, topicLen));
  pos += topicLen;

  const payload = String.fromCharCode.apply(null, new Uint8Array(buffer, pos, remainingLen - topicLen - 2));

  return { topic, payload };
}

function startPing() {
  stopPing();
  pingTimer = setInterval(() => {
    if (socketTask) {
      socketTask.send({ data: buildPingReqPacket() });
    }
  }, 30000);
}

function stopPing() {
  if (pingTimer) {
    clearInterval(pingTimer);
    pingTimer = null;
  }
}

function connect(callbacks) {
  clientId = randomId();
  console.log('MQTT connecting...', clientId);

  socketTask = wx.connectSocket({
    url: MQTT_BROKER,
    success: () => console.log('socket connect success'),
    fail: (err) => console.error('socket connect fail:', err)
  });

  socketTask.onOpen(() => {
    console.log('WebSocket opened, sending CONNECT');
    socketTask.send({ data: buildConnectPacket() });
  });

  socketTask.onMessage((res) => {
    const buffer = res.data;
    if (!(buffer instanceof ArrayBuffer)) return;

    const view = new DataView(buffer);
    const header = view.getUint8(0);
    const type = (header >> 4) & 0x0f;

    if (type === 2) {
      // CONNACK
      console.log('MQTT connected (CONNACK)');
      socketTask.send({ data: buildSubscribePacket(['sensor/am2020dy', 'sensor/sen68']) });
      startPing();
      if (callbacks.onConnected) callbacks.onConnected();
    } else if (type === 3) {
      // PUBLISH
      const parsed = parsePublishPacket(buffer);
      if (parsed && callbacks.onMessage) {
        callbacks.onMessage(parsed.topic, parsed.payload);
      }
    }
  });

  socketTask.onClose(() => {
    console.log('WebSocket closed, reconnecting in 5s');
    stopPing();
    if (callbacks.onDisconnected) callbacks.onDisconnected();
    setTimeout(() => connect(callbacks), 5000);
  });

  socketTask.onError((err) => {
    console.error('WebSocket error:', err);
  });
}

module.exports = { connect };