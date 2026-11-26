// Node sync-tier test driver: runs Pyodide inside a
// worker_threads worker (node_worker.mjs), where tinyc_web takes
// its synchronous tier, and reports the shared case list's result.
//
// Usage: node run_node.mjs
import { Worker } from 'node:worker_threads';

const worker = new Worker(new URL('./node_worker.mjs', import.meta.url));
const timer = setTimeout(() => {
  console.error('NODE SYNC TIER: TIMEOUT');
  process.exit(2);
}, 300000);

worker.on('message', (msg) => {
  clearTimeout(timer);
  if (msg.done && msg.failed.length === 0) {
    console.log('NODE SYNC TIER: PASS');
    process.exit(0);
  }
  console.log('NODE SYNC TIER: FAIL', JSON.stringify(msg));
  process.exit(1);
});
worker.on('error', (err) => {
  clearTimeout(timer);
  console.error('NODE SYNC TIER WORKER ERROR:', err);
  process.exit(1);
});
