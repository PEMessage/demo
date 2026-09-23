import { app, BrowserWindow, ipcMain } from 'electron';
import path from 'node:path';

let tick = 0;
let settingsWin = null;

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

// 10: a window can belong to another window.
function openSettings(parent) {
  if (settingsWin) {
    if (settingsWin.isMinimized()) settingsWin.restore();
    settingsWin.focus();
    return settingsWin;
  }

  settingsWin = new BrowserWindow({
    width: 460,
    height: 320,
    parent: parent ?? undefined, // ownership: stays above it, dies with it
    // modal: true would be the stronger form: block the parent entirely.
    show: false,
    webPreferences: {
      preload: path.join(import.meta.dirname, 'preload.cjs')
    }
  });

  settingsWin.loadFile(path.join(import.meta.dirname, 'settings.html'));
  settingsWin.once('ready-to-show', () => settingsWin.show());

  settingsWin.on('closed', () => {
    settingsWin = null;
    console.log('[main] settings closed');
  });

  return settingsWin;
}

app.whenReady().then(() => {
  ipcMain.on('page:say', (event, message) => {
    console.log('[main] page said:', message);
  });

  ipcMain.on('page:quit', () => app.quit());

  ipcMain.handle('app:user-data', () => app.getPath('userData'));

  ipcMain.on('window:open', () => createWindow());

  // 10: finally use the sender we have been handed since 04: who asked?
  ipcMain.on('window:open-settings', (event) => {
    const parent = BrowserWindow.fromWebContents(event.sender);
    if (parent) openSettings(parent);
  });

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
