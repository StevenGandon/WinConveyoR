const { app, BrowserWindow } = require('electron');
const { equal } = require('assert');
const { load, DataType, open, close, arrayConstructor, define } = require('ffi-rs');

const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  });

  win.loadFile('index.html');
}

app.whenReady().then(() => {
  createWindow();
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') app.quit()
});