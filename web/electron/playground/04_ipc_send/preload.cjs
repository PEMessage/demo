const { contextBridge, ipcRenderer } = require('electron');

window.addEventListener('DOMContentLoaded', () => {
  document.getElementById('preload-version').textContent =
    process.versions.electron;
});

contextBridge.exposeInMainWorld('native', {
  // 03: answered entirely inside the renderer, main never hears about it.
  getInfo() {
    return {
      electron: process.versions.electron,
      node: process.versions.node
    };
  },

  // 04: one-way messages. They leave this process and are not coming back.
  sayHi(message) {
    ipcRenderer.send('page:say', message);
  },
  quit() {
    ipcRenderer.send('page:quit');
  }
});
