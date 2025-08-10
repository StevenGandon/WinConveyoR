const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('electronAPI', {
  minimizeWindow: () => ipcRenderer.send('minimize-window'),
  maximizeWindow: () => ipcRenderer.send('maximize-window'),
  closeWindow: () => ipcRenderer.send('close-window'),
});

contextBridge.exposeInMainWorld('ffiAPI', {
  ffiAdd: (a, b) => ipcRenderer.invoke('ffi-add', a, b),
  ffiSub: (a, b) => ipcRenderer.invoke('ffi-sub', a, b),
  getPlatformInfo: () => ipcRenderer.invoke('get-platform-info')
});