const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
  minimizeWindow: () => ipcRenderer.send('minimize-window'),
  maximizeWindow: () => ipcRenderer.send('maximize-window'),
  closeWindow: () => ipcRenderer.send('close-window'),

  runUpdate: () => ipcRenderer.invoke('cli-update'),
  runInstall: (packageName) => ipcRenderer.invoke('cli-install', packageName),
  runUninstall: (packageName) => ipcRenderer.invoke('cli-uninstall', packageName),
  listInstalled: () => ipcRenderer.invoke('cli-list-installed'),

  onCliOutput: (callback) => {
    const handler = (_event, text) => callback(text);
    ipcRenderer.on('cli-output', handler);
    return () => ipcRenderer.removeListener('cli-output', handler);
  },
});
