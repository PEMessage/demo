import { app, BrowserWindow } from 'electron';

app.whenReady().then(() => {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  });

  const html = `
    <!doctype html>
    <html>
      <body>
        <h1>Hello Electron (ESM)</h1>
        <button onclick="alert('clicked')">Click Me</button>
      </body>
    </html>
  `;

  win.loadURL('data:text/html;charset=utf-8,' + encodeURIComponent(html));
});

