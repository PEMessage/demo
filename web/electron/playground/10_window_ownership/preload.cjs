const { contextBridge, ipcRenderer } = require('electron');

// 09: this preload is now shared by two different pages, so it can no
// longer assume any particular element exists.
window.addEventListener('DOMContentLoaded', () => {
  const el = document.getElementById('preload-version');
  if (el) el.textContent = process.versions.electron;
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

  openWindow() {
    ipcRenderer.send('window:open');
  },
  openSettings() {
    ipcRenderer.send('window:open-settings');
  },

  onTick(callback) {
    const listener = (event, n) => callback(n);
    ipcRenderer.on('main:tick', listener);
    return () => ipcRenderer.removeListener('main:tick', listener);
  }
});
