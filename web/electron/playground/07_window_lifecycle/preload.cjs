const { contextBridge, ipcRenderer } = require('electron');

window.addEventListener('DOMContentLoaded', () => {
  document.getElementById('preload-version').textContent =
    process.versions.electron;
});

contextBridge.exposeInMainWorld('native', {
  getInfo() {
    return {
      electron: process.versions.electron,
      node: process.versions.node
    };
  },

  sayHi(message) {
    ipcRenderer.send('page:say', message);
  },
  quit() {
    ipcRenderer.send('page:quit');
  },
  getUserData() {
    return ipcRenderer.invoke('app:user-data');
  },

  onTick(callback) {
    const listener = (event, n) => callback(n);
    ipcRenderer.on('main:tick', listener);
    return () => ipcRenderer.removeListener('main:tick', listener);
  }
});
