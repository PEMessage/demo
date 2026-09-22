import { app, BrowserWindow, ipcMain } from 'electron';
import path from 'node:path';

// Main owns the truth. The counter lives here, not in any window, so it
// survives a window being closed and re-created.
let tick = 0;

function createWindow() {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    show: false, // 07: visible is not the same as ready.
    webPreferences: {
      preload: path.join(import.meta.dirname, 'preload.cjs')
    }
  });

  win.loadFile(path.join(import.meta.dirname, 'index.html'));

  // "Can it receive yet?" is an event, not a guess.
  win.once('ready-to-show', () => {
    win.show();
    console.log('[main] window ready');
  });

  const timer = setInterval(() => {
    if (win.isDestroyed()) return;
    win.webContents.send('main:tick', ++tick);
  }, 1000);

  // Every window that starts something must stop it when it dies.
  win.on('closed', () => {
    clearInterval(timer);
    console.log('[main] window closed');
  });
}

app.whenReady().then(() => {
  // IPC handlers are app-scoped: registered once, never per window.
  ipcMain.on('page:say', (event, message) => {
    console.log('[main] page said:', message);
  });

  ipcMain.on('page:quit', () => app.quit());

  ipcMain.handle('app:user-data', () => app.getPath('userData'));

  createWindow();
});

// The app's life is not the window's life.
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow();
});
