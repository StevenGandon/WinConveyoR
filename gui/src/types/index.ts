export interface Package {
  name: string;
  version: string;
  description?: string;
  isInstalled: boolean;
  isUpdatable: boolean;
  availableVersion?: string;
}

export type ThemeMode = 'light' | 'dark' | 'system';
export type ContrastMode = 'normal' | 'high';

export interface AppSettings {
  theme: ThemeMode;
  contrast: ContrastMode;
  fontSize: number;
  reduceMotion: boolean;
  enableKeyboardShortcuts: boolean;
  showOutputPage: boolean;
}
