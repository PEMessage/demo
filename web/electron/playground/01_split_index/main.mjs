import { app, BrowserWindow } from 'electron';
import path from 'node:path';

app.whenReady().then(() => {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  });

  win.loadFile(path.join(import.meta.dirname, 'index.html'));
});
