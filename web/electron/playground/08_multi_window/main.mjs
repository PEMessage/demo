import { app, BrowserWindow, ipcMain } from 'electron';
import path from 'node:path';

// Still app-owned. Every window shows the same counter.
let tick = 0;

// 08: a window is only a window. No app-wide recurring work in here.
function createWindow() {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    show: false,
    webPreferences: {
      preload: path.join(import.meta.dirname, 'preload.cjs')
    }
  });

  win.loadFile(path.join(import.meta.dirname, 'index.html'));

  win.once('ready-to-show', () => win.show());
  win.on('closed', () => console.log('[main] window closed'));

  return win;
}

app.whenReady().then(() => {
  ipcMain.on('page:say', (event, message) => {
    console.log('[main] page said:', message);
  });

  ipcMain.on('page:quit', () => app.quit());

  ipcMain.handle('app:user-data', () => app.getPath('userData'));

  // A window can ask for another window.
  ipcMain.on('window:open', () => createWindow());

  // 08: one app-wide clock, broadcast to the whole set of windows.
  setInterval(() => {
    for (const win of BrowserWindow.getAllWindows()) {
      if (win.isDestroyed()) continue;
      win.webContents.send('main:tick', ++tick);
    }
  }, 1000);

  createWindow();
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow();
});
