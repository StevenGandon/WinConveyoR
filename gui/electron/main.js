import { app, BrowserWindow, ipcMain, dialog, session } from 'electron';
import path, { dirname } from 'path';
import { fileURLToPath } from 'url';
import { spawn } from 'child_process';
import fs from 'fs';
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
    icon: path.join(__dirname, '..', 'build', 'icon.png'),
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

// bash -c receives one string: quote every argument (paths, --set values...)
function shellQuote(arg) {
  return `'${String(arg).replace(/'/g, `'\\''`)}'`;
}

// C:\foo\bar -> /mnt/c/foo/bar (the CLI runs inside WSL on Windows)
function toWslPath(p) {
  return p.replace(/^([A-Z]):\\/i, (_, d) => `/mnt/${d.toLowerCase()}/`).replace(/\\/g, '/');
}

// a file picked in the GUI, as the CLI sees it
function toCliPath(p) {
  return process.platform === 'win32' ? toWslPath(p) : p;
}

// install options from the GUI form -> CLI parameters
function installArgs(packageName, opts) {
  const args = ['install', packageName];
  if (opts?.prefix) args.push(`--prefix=${opts.prefix}`);
  if (opts?.procedure) args.push(`--procedure=${toCliPath(opts.procedure)}`);
  const values = Object.entries(opts?.values ?? {}).map(([name, value]) => `${name}=${value}`);
  if (values.length > 0) args.push(`--set=${values.join(',')}`);
  return args;
}

// fields of the package, or of the user procedure replacing it (F30)
function optionsArgs(packageName, procedure) {
  const args = ['options', packageName];
  if (procedure) args.push(`--procedure=${toCliPath(procedure)}`);
  return args;
}

function runCli(args) {
  return new Promise((resolve) => {
    let child;

    if (process.platform === 'win32') {
      const wslRoot = toWslPath(PROJECT_ROOT);
      const wslCli = `${wslRoot}/cli/main.py`;
      const wslLib = ['.', `${wslRoot}/cli/build`, `${wslRoot}/lib/libwconr/build`, `${wslRoot}/lib/libwconr`].join(':');
      child = spawn('wsl', ['bash', '-c', `LD_LIBRARY_PATH="${wslLib}" python3 "${wslCli}" --no-ansi ${args.map(shellQuote).join(' ')}`], {
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

app.setName('WinConveyoR');
if (process.platform === 'win32') {
  app.setAppUserModelId('com.winconveyor.app');
}

// <Downloads>/name, or name (1).ext, name (2).ext... if it already exists
function uniqueDownloadPath(filename) {
  const dir = app.getPath('downloads');
  const ext = path.extname(filename);
  const base = path.basename(filename, ext);
  let candidate = path.join(dir, filename);

  for (let i = 1; fs.existsSync(candidate); i++) {
    candidate = path.join(dir, `${base} (${i})${ext}`);
  }
  return candidate;
}

// F28: files downloaded from the hub (<a download>) -> notify the page when done
function watchDownloads() {
  session.defaultSession.on('will-download', (_event, item) => {
    // save straight into Downloads: the "save as" dialog can open hidden behind the
    // frameless window (WSLg), leaving the download pending forever
    item.setSavePath(uniqueDownloadPath(item.getFilename()));
    console.log(`[download] ${item.getURL()} -> ${item.getSavePath()}`);
    item.once('done', (_e, state) => {
      console.log(`[download] ${item.getFilename()}: ${state}`);
      mainWindow?.webContents.send('download-done', {
        filename: item.getFilename(),
        path: item.getSavePath(),
        state,
      });
    });
  });
}

app.whenReady().then(() => {
  watchDownloads();
  createWindow();
});

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
ipcMain.handle('cli-install', (_event, packageName, opts) => runCli(installArgs(packageName, opts)));
ipcMain.handle('cli-options', (_event, packageName, procedure) => runCli(optionsArgs(packageName, procedure)));
ipcMain.handle('pick-procedure', async () => {
  const result = await dialog.showOpenDialog(mainWindow, {
    title: 'Choose an install procedure',
    properties: ['openFile'],
    filters: [
      { name: 'Install procedure', extensions: ['yaml', 'yml', 'WIZARD'] },
      { name: 'All files', extensions: ['*'] },
    ],
  });
  return result.canceled ? null : result.filePaths[0];
});
ipcMain.handle('cli-uninstall', (_event, packageName) => runCli(['uninstall', packageName]));
ipcMain.handle('cli-list-installed', () => runCli(['list']));
ipcMain.handle('cli-search', (_event, query) => runCli(query ? ['search', query] : ['search']));
ipcMain.handle('cli-info', (_event, packageSpec) => runCli(['info', packageSpec]));
ipcMain.handle('cli-check', (_event, packageSpec) => runCli(['check', packageSpec]));
ipcMain.handle('cli-upgrade', (_event, packageName) => runCli(packageName ? ['upgrade', packageName] : ['upgrade']));
