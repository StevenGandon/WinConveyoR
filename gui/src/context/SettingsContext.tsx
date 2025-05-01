import React, { createContext, useContext, useState, useEffect } from 'react';
import { AppSettings, ThemeMode, ContrastMode } from '../types';

interface SettingsContextType {
  settings: AppSettings;
  updateSettings: (settings: Partial<AppSettings>) => void;
  toggleTheme: () => void;
  toggleContrast: () => void;
}

const defaultSettings: AppSettings = {
  theme: 'system',
  contrast: 'normal',
  fontSize: 16,
  reduceMotion: false,
  enableKeyboardShortcuts: true,
};

const SettingsContext = createContext<SettingsContextType | undefined>(undefined);

export const SettingsProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [settings, setSettings] = useState<AppSettings>(defaultSettings);

  useEffect(() => {
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    const theme = settings.theme === 'system' ? (prefersDark ? 'dark' : 'light') : settings.theme;
    
    document.documentElement.classList.remove('light', 'dark');
    document.documentElement.classList.add(theme);

    if (settings.contrast === 'high') {
      document.documentElement.classList.add('high-contrast');
    } else {
      document.documentElement.classList.remove('high-contrast');
    }

    if (settings.reduceMotion) {
      document.documentElement.classList.add('reduce-motion');
    } else {
      document.documentElement.classList.remove('reduce-motion');
    }

    document.documentElement.style.fontSize = `${settings.fontSize}px`;
  }, [settings]);

  const updateSettings = (newSettings: Partial<AppSettings>) => {
    setSettings(prev => ({ ...prev, ...newSettings }));
  };

  const toggleTheme = () => {
    setSettings(prev => ({
      ...prev,
      theme: prev.theme === 'light' ? 'dark' : prev.theme === 'dark' ? 'system' : 'light',
    }));
  };

  const toggleContrast = () => {
    setSettings(prev => ({
      ...prev,
      contrast: prev.contrast === 'normal' ? 'high' : 'normal',
    }));
  };

  return (
    <SettingsContext.Provider value={{ settings, updateSettings, toggleTheme, toggleContrast }}>
      {children}
    </SettingsContext.Provider>
  );
};

export const useSettings = (): SettingsContextType => {
  const context = useContext(SettingsContext);
  if (context === undefined) {
    throw new Error('useSettings must be used within a SettingsProvider');
  }
  return context;
};