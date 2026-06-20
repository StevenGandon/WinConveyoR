import React, { createContext, useContext, useState } from 'react';
import { Package } from '../types';

interface PackageContextType {
  packages: Package[];
  installedPackages: Package[];
  updatablePackages: Package[];
  isLoading: boolean;
  addInstalled: (name: string, version?: string) => void;
  removeInstalled: (name: string) => void;
}

const PackageContext = createContext<PackageContextType | undefined>(undefined);

const STORAGE_KEY = 'wcr_packages';

const loadPackages = (): Package[] => {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    return raw ? JSON.parse(raw) : [];
  } catch {
    return [];
  }
};

const savePackages = (pkgs: Package[]) => {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(pkgs));
};

export const PackageProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [packages, setPackages] = useState<Package[]>(loadPackages);

  const installedPackages = packages.filter(pkg => pkg.isInstalled);
  const updatablePackages = packages.filter(pkg => pkg.isInstalled && pkg.isUpdatable);

  const addInstalled = (name: string, version?: string) => {
    setPackages(prev => {
      if (prev.some(p => p.name === name)) return prev;
      const next = [...prev, { name, version: version ?? 'unknown', isInstalled: true, isUpdatable: false }];
      savePackages(next);
      return next;
    });
  };

  const removeInstalled = (name: string) => {
    setPackages(prev => {
      const next = prev.filter(p => p.name !== name);
      savePackages(next);
      return next;
    });
  };

  return (
    <PackageContext.Provider
      value={{ packages, installedPackages, updatablePackages, isLoading: false, addInstalled, removeInstalled }}
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
