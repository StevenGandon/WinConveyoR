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

export const PackageProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [packages, setPackages] = useState<Package[]>([]);

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
