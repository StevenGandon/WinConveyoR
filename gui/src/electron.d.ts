interface CliResult {
  code: number;
  output: string;
}

interface InstallOptions {
  prefix?: string;
  values?: Record<string, string>;
}

interface ElectronAPI {
  minimizeWindow: () => void;
  maximizeWindow: () => void;
  closeWindow: () => void;
  runUpdate: () => Promise<CliResult>;
  runInstall: (packageName: string, opts?: InstallOptions) => Promise<CliResult>;
  runOptions: (packageName: string) => Promise<CliResult>;
  runUninstall: (packageName: string) => Promise<CliResult>;
  listInstalled: () => Promise<CliResult>;
  runSearch: (query?: string) => Promise<CliResult>;
  runInfo: (packageSpec: string) => Promise<CliResult>;
  runCheck: (packageSpec: string) => Promise<CliResult>;
  runUpgrade: (packageName?: string) => Promise<CliResult>;
  onCliOutput: (callback: (text: string) => void) => () => void;
}

declare global {
  interface Window {
    electronAPI?: ElectronAPI;
  }
}

export {};
