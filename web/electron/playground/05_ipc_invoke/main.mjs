import { app, BrowserWindow, ipcMain } from 'electron';
import path from 'node:path';

app.whenReady().then(() => {
  // 04: fire-and-forget channels.
  ipcMain.on('page:say', (event, message) => {
    console.log('[main] page said:', message);
  });

  ipcMain.on('page:quit', () => {
    app.quit();
  });

  // 05: a channel that answers. One handler per channel, and its return
  // value becomes the resolved value of the caller's promise.
  ipcMain.handle('app:user-data', () => app.getPath('userData'));

  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(import.meta.dirname, 'preload.cjs')
    }
  });

  win.loadFile(path.join(import.meta.dirname, 'index.html'));
});
