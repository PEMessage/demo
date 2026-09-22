const { contextBridge } = require('electron');

// The preload can still push directly into the DOM, as in 02.
window.addEventListener('DOMContentLoaded', () => {
  document.getElementById('preload-version').textContent =
    process.versions.electron;
});

// 03: the page can now pull, but only through what we explicitly list here.
contextBridge.exposeInMainWorld('native', {
  getInfo() {
    return {
      electron: process.versions.electron,
      node: process.versions.node
    };
  }
});
