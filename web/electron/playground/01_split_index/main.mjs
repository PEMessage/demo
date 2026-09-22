import { app, BrowserWindow } from 'electron';
import path from 'node:path';

app.whenReady().then(() => {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  });

  win.loadFile(path.join(import.meta.dirname, 'index.html')).then(() => {
    // Observation only, not a new concept: print the preferences this
    // renderer actually got, and open its own console.
    console.log(
      'win.webContents.getLastWebPreferences() =',
      win.webContents.getLastWebPreferences()
    );
    win.webContents.openDevTools();
  });
});
