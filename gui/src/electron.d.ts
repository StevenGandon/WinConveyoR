interface CliResult {
  code: number;
  output: string;
}

interface InstallOptions {
  prefix?: string;
  values?: Record<string, string>;
  procedure?: string;
}

interface DownloadInfo {
  filename: string;
  path: string;
  state: 'completed' | 'cancelled' | 'interrupted';
}

interface ElectronAPI {
  minimizeWindow: () => void;
  maximizeWindow: () => void;
  closeWindow: () => void;
  runUpdate: () => Promise<CliResult>;
  runInstall: (packageName: string, opts?: InstallOptions) => Promise<CliResult>;
  runOptions: (packageName: string, procedure?: string) => Promise<CliResult>;
  pickProcedure: () => Promise<string | null>;
  runUninstall: (packageName: string) => Promise<CliResult>;
  listInstalled: () => Promise<CliResult>;
  runSearch: (query?: string) => Promise<CliResult>;
  runInfo: (packageSpec: string) => Promise<CliResult>;
  runCheck: (packageSpec: string) => Promise<CliResult>;
  runUpgrade: (packageName?: string) => Promise<CliResult>;
  onCliOutput: (callback: (text: string) => void) => () => void;
  onDownloadDone: (callback: (info: DownloadInfo) => void) => () => void;
}

declare global {
  interface Window {
    electronAPI?: ElectronAPI;
  }
}

export {};
