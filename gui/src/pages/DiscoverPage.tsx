import React, { useState, useEffect } from 'react';
import { usePackages } from '../context/PackageContext';
import Header from '../components/layout/Header';
import PackageGrid from '../components/packages/PackageGrid';
import PackageDetails from '../components/packages/PackageDetails';
import { Package } from '../types';

const DiscoverPage: React.FC = () => {
  const { packages, searchQuery, selectedCategory, selectedPackage, selectPackage, isLoading } = usePackages();
  const [filteredPackages, setFilteredPackages] = useState<Package[]>([]);

  useEffect(() => {
    // Filter packages based on search query and selected category
    let filtered = [...packages];
    
    if (searchQuery) {
      const query = searchQuery.toLowerCase();
      filtered = filtered.filter(pkg => 
        pkg.name.toLowerCase().includes(query) || 
        pkg.description.toLowerCase().includes(query) ||
        pkg.publisher.toLowerCase().includes(query)
      );
    }
    
    if (selectedCategory && selectedCategory !== 'all') {
      filtered = filtered.filter(pkg => pkg.category === selectedCategory);
    }
    
    setFilteredPackages(filtered);
  }, [packages, searchQuery, selectedCategory]);

  return (
    <>
      <Header title="Discover Packages" />
      
      <div className="p-6">
        {searchQuery && (
          <div className="mb-4">
            <h2 className="text-lg font-medium text-gray-900 dark:text-gray-100">
              Search results for: <span className="font-semibold">"{searchQuery}"</span>
            </h2>
            <p className="text-sm text-gray-500 dark:text-gray-400">
              Found {filteredPackages.length} package{filteredPackages.length !== 1 ? 's' : ''}
            </p>
          </div>
        )}
        
        {selectedCategory && selectedCategory !== 'all' && (
          <div className="mb-4">
            <h2 className="text-lg font-medium text-gray-900 dark:text-gray-100">
              Category: <span className="font-semibold">{selectedCategory}</span>
            </h2>
          </div>
        )}
        
        <PackageGrid 
          packages={filteredPackages} 
          isLoading={isLoading}
          emptyMessage={searchQuery ? `No results found for "${searchQuery}"` : "No packages found in this category"}
        />
      </div>
      
      {selectedPackage && (
        <PackageDetails pkg={selectedPackage} onClose={() => selectPackage(null)} />
      )}
    </>
  );
};

export default DiscoverPage;