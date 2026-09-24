import { app, BrowserWindow, ipcMain } from 'electron';
import path from 'node:path';

let tick = 0;

// 09: main remembers the settings window by its role. A role is a fact
// main owns; a title or URL is just a guess about the content.
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

// 09: "open settings" means "make settings visible", not "create a window".
function openSettings() {
  if (settingsWin) {
    if (settingsWin.isMinimized()) settingsWin.restore();
    settingsWin.focus();
    return settingsWin;
  }

  settingsWin = new BrowserWindow({
    width: 460,
    height: 320,
    show: false,
    webPreferences: {
      preload: path.join(import.meta.dirname, 'preload.cjs')
    }
  });

  settingsWin.loadFile(path.join(import.meta.dirname, 'settings.html'));
  settingsWin.once('ready-to-show', () => settingsWin.show());

  // The reference must die with the window, or the next open focuses a ghost.
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
  ipcMain.on('window:open-settings', () => openSettings());

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
