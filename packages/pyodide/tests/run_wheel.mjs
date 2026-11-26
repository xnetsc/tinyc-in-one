// Wheel test driver: installs the built tinyc_web wheel into a
// clean Pyodide (in wheel_worker.mjs) and runs the shared case
// list against the installed package. The package under test is
// the wheel file itself; nothing is copied from the source tree.
//
// Usage: node run_wheel.mjs <path-to-wheel>
import path from 'node:path';
import { Worker } from 'node:worker_threads';

const wheel = process.argv[2];
if (!wheel) {
  console.error('usage: node run_wheel.mjs <path-to-wheel>');
  process.exit(2);
}

const worker = new Worker(new URL('./wheel_worker.mjs', import.meta.url), {
  workerData: { wheel: path.resolve(wheel) },
});
const timer = setTimeout(() => {
  console.error('WHEEL TEST: TIMEOUT');
  process.exit(2);
}, 300000);

worker.on('message', (msg) => {
  clearTimeout(timer);
  if (msg.done && msg.failed.length === 0) {
    console.log('WHEEL TEST: PASS');
    process.exit(0);
  }
  console.log('WHEEL TEST: FAIL', JSON.stringify(msg));
  process.exit(1);
});
worker.on('error', (err) => {
  clearTimeout(timer);
  console.error('WHEEL TEST WORKER ERROR:', err);
  process.exit(1);
});
