import React from 'react';
import { usePackages } from '../context/PackageContext';
import Header from '../components/layout/Header';
import PackageGrid from '../components/packages/PackageGrid';
import PackageDetails from '../components/packages/PackageDetails';

const InstalledPage: React.FC = () => {
  const { installedPackages, selectedPackage, selectPackage, isLoading } = usePackages();

  return (
    <>
      <Header title="Installed Packages" />
      
      <div className="p-6">
        <div className="mb-4">
          <h2 className="text-lg font-medium text-gray-900 dark:text-gray-100">
            Your installed packages
          </h2>
          <p className="text-sm text-gray-500 dark:text-gray-400">
            Manage all your currently installed applications
          </p>
        </div>
        
        <PackageGrid 
          packages={installedPackages} 
          isLoading={isLoading}
          emptyMessage="You don't have any packages installed yet"
        />
      </div>
      
      {selectedPackage && (
        <PackageDetails pkg={selectedPackage} onClose={() => selectPackage(null)} />
      )}
    </>
  );
};

export default InstalledPage;