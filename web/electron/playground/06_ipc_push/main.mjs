import { app, BrowserWindow, ipcMain } from 'electron';
import path from 'node:path';

app.whenReady().then(() => {
  ipcMain.on('page:say', (event, message) => {
    console.log('[main] page said:', message);
  });

  ipcMain.on('page:quit', () => {
    app.quit();
  });

  ipcMain.handle('app:user-data', () => app.getPath('userData'));

  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(import.meta.dirname, 'preload.cjs')
    }
  });

  win.loadFile(path.join(import.meta.dirname, 'index.html'));

  // 06: the other direction. Main decides *when* to talk.
  let tick = 0;
  const timer = setInterval(() => {
    if (win.isDestroyed()) return;
    win.webContents.send('main:tick', ++tick);
  }, 1000);

  win.on('closed', () => clearInterval(timer));
});
