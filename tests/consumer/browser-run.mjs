// Browser consumer test driver: serves a staging directory over
// HTTP and drives browser-test.html in headless Chromium (Playwright).
//
//   node browser-run.mjs <staging-dir>
//
// The staging dir holds index.html (browser-test.html) and pkg/ (the
// built browser package). Exit code 0 iff the page reports all checks
// passed. Requires the playwright npm package and its Chromium build
// (CI: npm install playwright && npx playwright install chromium).
import http from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, normalize } from 'node:path';
import { chromium } from 'playwright';

const root = process.argv[2];
if (!root) {
  console.error('usage: node browser-run.mjs <staging-dir>');
  process.exit(2);
}

const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.mjs': 'text/javascript',
  '.js': 'text/javascript',
  '.wasm': 'application/wasm',
  '.json': 'application/json',
};

const server = http.createServer(async (req, res) => {
  try {
    let path = decodeURIComponent(new URL(req.url, 'http://x').pathname);
    if (path === '/') path = '/index.html';
    const file = normalize(join(root, path));
    if (!file.startsWith(normalize(root))) throw new Error('bad path');
    const data = await readFile(file);
    res.writeHead(200, { 'content-type': MIME[extname(file)] || 'application/octet-stream' });
    res.end(data);
  } catch {
    res.writeHead(404);
    res.end('not found');
  }
});
await new Promise((r) => server.listen(0, '127.0.0.1', r));
const url = `http://127.0.0.1:${server.address().port}/index.html`;

let exitCode = 1;
try {
  const browser = await chromium.launch();
  const page = await browser.newPage();
  page.on('console', (m) => console.log('[page]', m.text()));
  page.on('pageerror', (e) => console.log('[pageerror]', e.message));
  await page.goto(url);
  await page.waitForFunction(() => window.__TINYC_DONE__ === true, null, { timeout: 180000 });
  const log = await page.evaluate(() => window.__TINYC_LOG__);
  const okFlag = await page.evaluate(() => window.__TINYC_OK__);
  for (const line of log) console.log(line);
  console.log(okFlag ? 'BROWSER CONSUMER TEST: PASS' : 'BROWSER CONSUMER TEST: FAIL');
  exitCode = okFlag ? 0 : 1;
  await browser.close();
} finally {
  server.close();
}
process.exit(exitCode);
