import React from 'react';
import { RefreshCw } from 'lucide-react';
import { usePackages } from '../context/PackageContext';
import Header from '../components/layout/Header';
import PackageGrid from '../components/packages/PackageGrid';
import PackageDetails from '../components/packages/PackageDetails';
import Button from '../components/ui/Button';

const UpdatesPage: React.FC = () => {
  const { updatablePackages, selectedPackage, selectPackage, isLoading } = usePackages();

  const handleUpdateAll = () => {
    // TODO : Update all packages
    console.log('Updating all packages');
  };

  return (
    <>
      <Header title="Available Updates" />
      
      <div className="p-6">
        <div className="flex justify-between items-center mb-4">
          <div>
            <h2 className="text-lg font-medium text-gray-900 dark:text-gray-100">
              Updates available
            </h2>
            <p className="text-sm text-gray-500 dark:text-gray-400">
              {updatablePackages.length} package{updatablePackages.length !== 1 ? 's' : ''} can be updated
            </p>
          </div>
          
          {updatablePackages.length > 0 && (
            <Button
              variant="primary"
              leftIcon={<RefreshCw size={18} />}
              onClick={handleUpdateAll}
            >
              Update all
            </Button>
          )}
        </div>
        
        <PackageGrid 
          packages={updatablePackages} 
          isLoading={isLoading}
          emptyMessage="All packages are up to date"
        />
      </div>
      
      {selectedPackage && (
        <PackageDetails pkg={selectedPackage} onClose={() => selectPackage(null)} />
      )}
    </>
  );
};

export default UpdatesPage;