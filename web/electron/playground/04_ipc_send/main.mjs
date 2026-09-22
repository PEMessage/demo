import { app, BrowserWindow, ipcMain } from 'electron';
import path from 'node:path';

app.whenReady().then(() => {
  // Listeners are registered before the window exists, so nothing can race them.
  ipcMain.on('page:say', (event, message) => {
    console.log('[main] page said:', message);
  });

  ipcMain.on('page:quit', () => {
    app.quit();
  });

  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(import.meta.dirname, 'preload.cjs')
    }
  });

  win.loadFile(path.join(import.meta.dirname, 'index.html'));
});
