const { contextBridge, ipcRenderer } = require('electron');

window.addEventListener('DOMContentLoaded', () => {
  document.getElementById('preload-version').textContent =
    process.versions.electron;
});

contextBridge.exposeInMainWorld('native', {
  // 03: answered entirely inside the renderer.
  getInfo() {
    return {
      electron: process.versions.electron,
      node: process.versions.node
    };
  },

  // 04: one-way, no answer expected.
  sayHi(message) {
    ipcRenderer.send('page:say', message);
  },
  quit() {
    ipcRenderer.send('page:quit');
  },

  // 05: one-way out, but with a way back in.
  getUserData() {
    return ipcRenderer.invoke('app:user-data');
  }
});
