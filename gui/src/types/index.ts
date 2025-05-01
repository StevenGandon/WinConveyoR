export interface Package {
  id: string;
  name: string;
  version: string;
  description: string;
  category: string;
  publisher: string;
  downloadCount: number;
  rating: number;
  size: string;
  installDate?: string;
  isInstalled: boolean;
  isUpdatable: boolean;
  lastUpdated: string;
  iconUrl: string;
}

export interface Category {
  id: string;
  name: string;
  count: number;
}

export type ThemeMode = 'light' | 'dark' | 'system';
export type ContrastMode = 'normal' | 'high';

export interface AppSettings {
  theme: ThemeMode;
  contrast: ContrastMode;
  fontSize: number;
  reduceMotion: boolean;
  enableKeyboardShortcuts: boolean;
}