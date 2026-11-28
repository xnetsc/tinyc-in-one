/* The suite's local test server: an HTTP echo service and a
 * WebSocket echo service on one 127.0.0.1 port, plus static
 * file service for the runners' unit files. No dependencies:
 * the WebSocket side is a minimal RFC 6455 frame codec. */
import { createHash } from 'node:crypto';
import fs from 'node:fs';
import http from 'node:http';
import path from 'node:path';

export function startEchoServer({ serveRoot, headers } = {}) {
  const server = http.createServer((req, res) => {
    if (headers) {
      for (const [k, v] of Object.entries(headers)) res.setHeader(k, v);
    }
    const url = req.url ?? '/';
    if (url === '/get' || url === '/echo') {
      let body = '';
      req.on('data', (c) => { body += c; });
      req.on('end', () => {
        res.writeHead(200, { 'content-type': 'application/json', 'x-test': 'yes' });
        res.end(JSON.stringify({
          method: req.method,
          url,
          body,
          ua: req.headers['user-agent'] ?? null,
          ct: req.headers['content-type'] ?? null,
        }));
      });
      return;
    }
    if (url === '/redir') {
      res.writeHead(302, { location: '/get' });
      res.end();
      return;
    }
    // Static service for unit files (any path; resolved by
    // basename under serveRoot or the sysroot lib directory).
    if (serveRoot) {
      const base = path.basename(url);
      for (const root of serveRoot) {
        const file = path.join(root, base);
        if (fs.existsSync(file) && fs.statSync(file).isFile()) {
          const type = base.endsWith('.html') ? 'text/html'
            : base.endsWith('.json') ? 'application/json'
            : /\.(mjs|js)$/.test(base) ? 'text/javascript'
            : 'application/octet-stream';
          res.writeHead(200, { 'content-type': type });
          res.end(fs.readFileSync(file));
          return;
        }
      }
    }
    res.writeHead(404);
    res.end('not found');
  });

  server.on('upgrade', (req, socket) => {
    const key = req.headers['sec-websocket-key'];
    if (!key) { socket.destroy(); return; }
    const accept = createHash('sha1')
      .update(key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11')
      .digest('base64');
    socket.write('HTTP/1.1 101 Switching Protocols\r\n' +
      'Upgrade: websocket\r\nConnection: Upgrade\r\n' +
      `Sec-WebSocket-Accept: ${accept}\r\n\r\n`);
    let buf = Buffer.alloc(0);
    socket.on('data', (chunk) => {
      buf = Buffer.concat([buf, chunk]);
      for (;;) {
        if (buf.length < 2) return;
        const op = buf[0] & 0x0f;
        const masked = buf[1] & 0x80;
        let len = buf[1] & 0x7f;
        let off = 2;
        if (len === 126) {
          if (buf.length < 4) return;
          len = buf.readUInt16BE(2); off = 4;
        } else if (len === 127) {
          if (buf.length < 10) return;
          len = Number(buf.readBigUInt64BE(2)); off = 10;
        }
        const maskOff = off;
        off += masked ? 4 : 0;
        if (buf.length < off + len) return;
        const payload = Buffer.from(buf.subarray(off, off + len));
        if (masked) {
          const mask = buf.subarray(maskOff, maskOff + 4);
          for (let i = 0; i < payload.length; i++) payload[i] ^= mask[i & 3];
        }
        buf = buf.subarray(off + len);
        if (op === 8) { socket.end(); return; }
        if (op === 9) { socket.write(Buffer.from([0x8a, 0])); continue; }
        if (op === 10) continue;
        const head = [0x80 | op];
        if (payload.length < 126) head.push(payload.length);
        else if (payload.length < 65536) {
          head.push(126, payload.length >> 8, payload.length & 255);
        } else {
          head.push(127);
          const lenBuf = Buffer.alloc(8);
          lenBuf.writeBigUInt64BE(BigInt(payload.length));
          socket.write(Buffer.concat([Buffer.from(head), lenBuf, payload]));
          continue;
        }
        socket.write(Buffer.concat([Buffer.from(head), payload]));
      }
    });
    socket.on('error', () => {});
  });

  return new Promise((resolve) => {
    server.listen(0, '127.0.0.1', () => {
      const port = server.address().port;
      resolve({
        port,
        httpBase: `http://127.0.0.1:${port}`,
        wsBase: `ws://127.0.0.1:${port}`,
        close: () => server.close(),
      });
    });
  });
}
