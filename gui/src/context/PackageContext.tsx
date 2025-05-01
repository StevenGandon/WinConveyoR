import React, { createContext, useContext, useState, useEffect } from 'react';
import { Package, Category } from '../types';
import { mockPackages, mockCategories } from '../data/mockData';

interface PackageContextType {
  packages: Package[];
  categories: Category[];
  installedPackages: Package[];
  updatablePackages: Package[];
  selectedPackage: Package | null;
  searchQuery: string;
  selectedCategory: string;
  isLoading: boolean;
  installPackage: (id: string) => void;
  uninstallPackage: (id: string) => void;
  updatePackage: (id: string) => void;
  selectPackage: (pkg: Package | null) => void;
  setSearchQuery: (query: string) => void;
  setSelectedCategory: (category: string) => void;
}

const PackageContext = createContext<PackageContextType | undefined>(undefined);

export const PackageProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [packages, setPackages] = useState<Package[]>([]);
  const [categories, setCategories] = useState<Category[]>([]);
  const [selectedPackage, setSelectedPackage] = useState<Package | null>(null);
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedCategory, setSelectedCategory] = useState('all');
  const [isLoading, setIsLoading] = useState(true);

  useEffect(() => {
    const loadData = async () => {
      await new Promise(resolve => setTimeout(resolve, 1000));
      setPackages(mockPackages);
      setCategories(mockCategories);
      setIsLoading(false);
    };
    
    loadData();
  }, []);

  const installedPackages = packages.filter(pkg => pkg.isInstalled);
  const updatablePackages = packages.filter(pkg => pkg.isInstalled && pkg.isUpdatable);

  const installPackage = (id: string) => {
    setPackages(prev =>
      prev.map(pkg => 
        pkg.id === id 
          ? { ...pkg, isInstalled: true, installDate: new Date().toISOString() } 
          : pkg
      )
    );
  };

  const uninstallPackage = (id: string) => {
    setPackages(prev =>
      prev.map(pkg => 
        pkg.id === id 
          ? { ...pkg, isInstalled: false, installDate: undefined } 
          : pkg
      )
    );
  };

  const updatePackage = (id: string) => {
    setPackages(prev =>
      prev.map(pkg => 
        pkg.id === id 
          ? { ...pkg, isUpdatable: false, lastUpdated: new Date().toISOString() } 
          : pkg
      )
    );
  };

  const selectPackage = (pkg: Package | null) => {
    setSelectedPackage(pkg);
  };

  return (
    <PackageContext.Provider
      value={{
        packages,
        categories,
        installedPackages,
        updatablePackages,
        selectedPackage,
        searchQuery,
        selectedCategory,
        isLoading,
        installPackage,
        uninstallPackage,
        updatePackage,
        selectPackage,
        setSearchQuery,
        setSelectedCategory
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