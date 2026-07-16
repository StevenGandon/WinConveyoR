import React, { createContext, useContext, useState, useEffect, useCallback } from 'react';
import { Package } from '../types';

interface AvailablePackage {
  name: string;
  version: string;
}

interface PackageContextType {
  packages: Package[];
  installedPackages: Package[];
  updatablePackages: Package[];
  availablePackages: AvailablePackage[];
  discoverQuery: string;
  setDiscoverQuery: (q: string) => void;
  isLoading: boolean;
  addInstalled: (name: string, version?: string) => void;
  removeInstalled: (name: string) => void;
  refreshInstalled: () => Promise<void>;
  loadAvailable: () => Promise<void>;
}

const PackageContext = createContext<PackageContextType | undefined>(undefined);

export const PackageProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [packages, setPackages] = useState<Package[]>([]);
  const [availablePackages, setAvailablePackages] = useState<AvailablePackage[]>([]);
  const [discoverQuery, setDiscoverQuery] = useState('');
  const [isLoading, setIsLoading] = useState(true);

  const refreshInstalled = useCallback(async () => {
    if (!window.electronAPI?.listInstalled) {
      setIsLoading(false);
      return;
    }
    const result = await window.electronAPI.listInstalled();
    if (result.code === 0 && result.output.trim()) {
      try {
        const match = result.output.match(/\[.*\]/s);
        if (match) {
          const list: { name: string; version: string }[] = JSON.parse(match[0]);
          setPackages(list.map(p => ({
            name: p.name,
            version: p.version,
            isInstalled: true,
            isUpdatable: false,
          })));
        }
      } catch { /* ignore parse errors */ }
    }
    setIsLoading(false);
  }, []);

  const loadAvailable = useCallback(async () => {
    if (!window.electronAPI?.runSearch) return;
    const result = await window.electronAPI.runSearch();
    if (result.code === 0 && result.output.trim()) {
      try {
        const match = result.output.match(/\[.*\]/s);
        if (match) {
          setAvailablePackages(JSON.parse(match[0]));
        }
      } catch { /* ignore */ }
    }
  }, []);

  useEffect(() => {
    refreshInstalled();
  }, [refreshInstalled]);

  const installedPackages = packages.filter(pkg => pkg.isInstalled);
  const updatablePackages = packages.filter(pkg => pkg.isInstalled && pkg.isUpdatable);

  const addInstalled = (name: string, version?: string) => {
    setPackages(prev => {
      if (prev.some(p => p.name === name)) return prev;
      return [...prev, { name, version: version ?? 'unknown', isInstalled: true, isUpdatable: false }];
    });
  };

  const removeInstalled = (name: string) => {
    setPackages(prev => prev.filter(p => p.name !== name));
  };

  return (
    <PackageContext.Provider
      value={{
        packages, installedPackages, updatablePackages,
        availablePackages, discoverQuery, setDiscoverQuery,
        isLoading, addInstalled, removeInstalled, refreshInstalled, loadAvailable,
      }}
    >
      {children}
    </PackageContext.Provider>
  );
};

export const usePackages = (): PackageContextType => {
  const context = useContext(PackageContext);
  if (context === undefined) {
    throw new Error('usePackages must be used within a PackageProvider');
  }
  return context;
};
