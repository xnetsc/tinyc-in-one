// Entry point for the classic-script bundle (tinyc-web.js), built by
// build-web-bundle.mjs. The bundle re-exports the whole public API of
// index.mjs; the IIFE output exposes it as the global `TinycWeb`, so a
// page can use the package with a plain <script> tag and no module
// loader. The per-target glue (tcc-<target>.mjs) is still loaded with
// a dynamic import() at runtime, and the wasm files stay next to the
// bundle — see the "Asset base URL" section in index.mjs for how
// their URLs are resolved.
export * from './index.mjs';
export { default } from './index.mjs';
