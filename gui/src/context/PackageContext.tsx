import React, { createContext, useContext, useState } from 'react';
import { Package } from '../types';

interface PackageContextType {
  packages: Package[];
  installedPackages: Package[];
  updatablePackages: Package[];
  isLoading: boolean;
  setPackages: (pkgs: Package[]) => void;
}

const PackageContext = createContext<PackageContextType | undefined>(undefined);

export const PackageProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [packages, setPackages] = useState<Package[]>([]);
  const [isLoading] = useState(false);

  const installedPackages = packages.filter(pkg => pkg.isInstalled);
  const updatablePackages = packages.filter(pkg => pkg.isInstalled && pkg.isUpdatable);

  return (
    <PackageContext.Provider
      value={{ packages, installedPackages, updatablePackages, isLoading, setPackages }}
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
