'use strict';
/*
 * The JS part of the hybrid test unit (libhybrid.tinyc.js).
 * js_add bumps the shared counter in the wasm side's memory
 * (reached through the wasm export counter_addr) on every
 * call, so the suite can watch data cross between the kinds
 * in both directions.
 */
function counterAddr(ctx) {
  return ctx.callExport('counter_addr');
}

module.exports = {
  __exports: [
    { name: 'js_add', sig: { params: ['i32'], results: ['i32'] } },
    { name: 'js_calls_wasm', sig: { params: ['i32'], results: ['i32'] } },
    { name: 'js_read_counter', sig: { params: [], results: ['i32'] } },
    { name: 'js_invoke_cb', sig: { params: ['i32', 'i32'], results: ['i32'] } },
  ],
  js_add: (ctx, x) => {
    const view = new DataView(ctx.memory().buffer);
    const p = counterAddr(ctx);
    view.setInt32(p, view.getInt32(p, true) + 1, true);
    return x + 100;
  },
  js_calls_wasm: (ctx, x) => ctx.callExport('wasm_double', x) + 1,
  js_read_counter: (ctx) =>
    new DataView(ctx.memory().buffer).getInt32(counterAddr(ctx), true),
  js_invoke_cb: (ctx, cb, x) => ctx.callFunction(cb, x),
};
