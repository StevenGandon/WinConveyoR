import { app, BrowserWindow, ipcMain } from 'electron';
import path, { dirname } from 'path';
import { fileURLToPath } from 'url';
import { spawn } from 'child_process';
import isDev from 'electron-is-dev';

if (process.platform === 'linux') {
  app.disableHardwareAcceleration();
}

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const PROJECT_ROOT = path.resolve(__dirname, '..', '..');
const CLI_ENTRY = path.join(PROJECT_ROOT, 'cli', 'main.py');
const LIB_PATH = [
  '.',
  path.join(PROJECT_ROOT, 'cli', 'build'),
  path.join(PROJECT_ROOT, 'lib', 'libwconr', 'build'),
  path.join(PROJECT_ROOT, 'lib', 'libwconr'),
].join(':');

let mainWindow;

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
    show: true
  });

  const startURL = isDev
  ? 'http://localhost:5173'
  : `file://${path.join(__dirname, '../dist/index.html')}`;

  mainWindow.loadURL(startURL);


  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

function runCli(args) {
  return new Promise((resolve) => {
    let child;

    if (process.platform === 'win32') {
      const wslRoot = PROJECT_ROOT.replace(/^([A-Z]):\\/i, (_, d) => `/mnt/${d.toLowerCase()}/`).replace(/\\/g, '/');
      const wslCli = `${wslRoot}/cli/main.py`;
      const wslLib = ['.', `${wslRoot}/cli/build`, `${wslRoot}/lib/libwconr/build`, `${wslRoot}/lib/libwconr`].join(':');
      child = spawn('wsl', ['bash', '-c', `LD_LIBRARY_PATH="${wslLib}" python3 "${wslCli}" --no-ansi ${args.join(' ')}`], {
        cwd: PROJECT_ROOT,
      });
    } else {
      const libDirs = ['.', path.join(PROJECT_ROOT, 'cli', 'build'), path.join(PROJECT_ROOT, 'lib', 'libwconr', 'build'), path.join(PROJECT_ROOT, 'lib', 'libwconr')].join(':');
      child = spawn('python3', [CLI_ENTRY, '--no-ansi', ...args], {
        cwd: PROJECT_ROOT,
        env: { ...process.env, LD_LIBRARY_PATH: libDirs },
      });
    }

    let output = '';

    child.stdout.on('data', (data) => {
      const text = data.toString();
      output += text;
      mainWindow?.webContents.send('cli-output', text);
    });

    child.stderr.on('data', (data) => {
      const text = data.toString();
      output += text;
      mainWindow?.webContents.send('cli-output', text);
    });

    child.on('close', (code) => {
      resolve({ code, output });
    });

    child.on('error', (err) => {
      resolve({ code: -1, output: err.message });
    });
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

ipcMain.handle('cli-update', () => runCli(['update']));
ipcMain.handle('cli-install', (_event, packageName) => runCli(['install', packageName]));
ipcMain.handle('cli-uninstall', (_event, packageName) => runCli(['uninstall', packageName]));
ipcMain.handle('cli-list-installed', () => runCli(['list']));
ipcMain.handle('cli-search', (_event, query) => runCli(query ? ['search', query] : ['search']));
ipcMain.handle('cli-info', (_event, packageSpec) => runCli(['info', packageSpec]));
ipcMain.handle('cli-check', (_event, packageSpec) => runCli(['check', packageSpec]));
ipcMain.handle('cli-upgrade', (_event, packageName) => runCli(packageName ? ['upgrade', packageName] : ['upgrade']));
