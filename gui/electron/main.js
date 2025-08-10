import { app, BrowserWindow, ipcMain } from 'electron';
import path, { dirname } from 'path';
import { fileURLToPath } from 'url';
import isDev from 'electron-is-dev';
import { load, DataType, open, close } from 'ffi-rs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

let mainWindow;
let isLibraryOpen = false;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    minWidth: 940,
    minHeight: 600,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js')
    },
    titleBarStyle: 'hidden',
    frame: false,
    show: false
  });

  const startURL = isDev
  ? 'http://localhost:5173'
  : `file://${path.join(__dirname, '../dist/index.html')}`;

  mainWindow.loadURL(startURL);

  mainWindow.once('ready-to-show', () => {
    mainWindow.show();
  });

  if (isDev) {
    mainWindow.webContents.openDevTools();
  }

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (mainWindow === null) {
    createWindow();
  }
});

ipcMain.on('minimize-window', () => {
  mainWindow?.minimize();
});

ipcMain.on('maximize-window', () => {
  if (mainWindow?.isMaximized()) {
    mainWindow.unmaximize();
  } else {
    mainWindow?.maximize();
  }
});

ipcMain.on('close-window', () => {
  mainWindow?.close();
});

ipcMain.handle('get-platform-info', async () => {
  return {
    platform: process.platform,
    arch: process.arch
  };
});

async function ensureLibraryOpen() {
  if (!isLibraryOpen) {
    const isWindows = process.platform === 'win32';
    const dynamicLib = isWindows 
      ? path.join(__dirname, '..', 'src', 'lib', 'testlib.dll')
      : path.join(__dirname, '..', 'src', 'lib', 'testlib.so');

    console.log('Opening library:', dynamicLib);
    
    try {
      open({
        library: 'testlib',
        path: dynamicLib
      });
      isLibraryOpen = true;
      console.log('Library opened successfully');
    } catch (error) {
      console.error('Error opening library:', error.message);
      throw new Error(`Failed to open library: ${error.message}`);
    }
  }
}

ipcMain.handle('ffi-add', async (event, a, b) => {
  try {
    await ensureLibraryOpen();
    
    console.log('Load parameters:', {
      library: 'testlib',
      funcName: 'add',
      retType: DataType.I32,
      paramsType: [DataType.I32, DataType.I32],
      paramsValue: [a, b]
    });

    const result = load({
      library: 'testlib',
      funcName: 'add',
      retType: DataType.I32,
      paramsType: [DataType.I32, DataType.I32],
      paramsValue: [a, b]
    });

    console.log('FFI add result:', result);
    return { success: true, result };
  } catch (error) {
    console.error('Erreur FFI add:', error);
    return { success: false, error: error.message };
  }
});

ipcMain.handle('ffi-sub', async (event, a, b) => {
  try {
    await ensureLibraryOpen();
    
    console.log('Load parameters:', {
      library: 'testlib',
      funcName: 'sub',
      retType: DataType.I32,
      paramsType: [DataType.I32, DataType.I32],
      paramsValue: [a, b]
    });

    const result = load({
      library: 'testlib',
      funcName: 'sub',
      retType: DataType.I32,
      paramsType: [DataType.I32, DataType.I32],
      paramsValue: [a, b]
    });

    console.log('FFI sub result:', result);
    return { success: true, result };
  } catch (error) {
    console.error('Erreur FFI sub:', error);
    return { success: false, error: error.message };
  }
});

app.on('before-quit', () => {
  if (isLibraryOpen) {
    try {
      close('testlib');
      console.log('Library closed successfully');
    } catch (error) {
      console.error('Error closing library:', error);
    }
  }
});
