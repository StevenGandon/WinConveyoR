const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
  minimizeWindow: () => ipcRenderer.send('minimize-window'),
  maximizeWindow: () => ipcRenderer.send('maximize-window'),
  closeWindow: () => ipcRenderer.send('close-window'),

  runUpdate: () => ipcRenderer.invoke('cli-update'),
  runInstall: (packageName, opts) => ipcRenderer.invoke('cli-install', packageName, opts),
  runOptions: (packageName, procedure) => ipcRenderer.invoke('cli-options', packageName, procedure),
  pickProcedure: () => ipcRenderer.invoke('pick-procedure'),
  runUninstall: (packageName) => ipcRenderer.invoke('cli-uninstall', packageName),
  listInstalled: () => ipcRenderer.invoke('cli-list-installed'),
  runSearch: (query) => ipcRenderer.invoke('cli-search', query),
  runInfo: (packageSpec) => ipcRenderer.invoke('cli-info', packageSpec),
  runCheck: (packageSpec) => ipcRenderer.invoke('cli-check', packageSpec),
  runUpgrade: (packageName) => ipcRenderer.invoke('cli-upgrade', packageName),

  onDownloadDone: (callback) => {
    const handler = (_event, info) => callback(info);
    ipcRenderer.on('download-done', handler);
    return () => ipcRenderer.removeListener('download-done', handler);
  },
  onCliOutput: (callback) => {
    const handler = (_event, text) => callback(text);
    ipcRenderer.on('cli-output', handler);
    return () => ipcRenderer.removeListener('cli-output', handler);
  },
});
