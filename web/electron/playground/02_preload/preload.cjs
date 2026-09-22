// Sandboxed preload scripts must stay CommonJS. Electron only allows ESM
// preloads when the renderer sandbox is turned off, which we do not want.
window.addEventListener('DOMContentLoaded', () => {
  document.getElementById('preload-version').textContent =
    process.versions.electron;
});
