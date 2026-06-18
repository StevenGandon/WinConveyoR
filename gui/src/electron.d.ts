interface CliResult {
  code: number;
  output: string;
}

interface ElectronAPI {
  minimizeWindow: () => void;
  maximizeWindow: () => void;
  closeWindow: () => void;
  runUpdate: () => Promise<CliResult>;
  runInstall: (packageName: string) => Promise<CliResult>;
  runUninstall: (packageName: string) => Promise<CliResult>;
  onCliOutput: (callback: (text: string) => void) => () => void;
}

declare global {
  interface Window {
    electronAPI?: ElectronAPI;
  }
}

export {};
