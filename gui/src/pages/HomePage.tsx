import React from 'react';
import { Download, RefreshCw, ChevronRight } from 'lucide-react';
import { usePackages } from '../context/PackageContext';
import Card, { CardHeader, CardTitle, CardContent } from '../components/ui/Card';
import Button from '../components/ui/Button';
import PackageGrid from '../components/packages/PackageGrid';

const HomePage: React.FC = () => {
  const { packages, updatablePackages, installedPackages, isLoading } = usePackages();
  
  const topPackages = packages
    .sort((a, b) => b.downloadCount - a.downloadCount)
    .slice(0, 6);
    
  const recentlyUpdated = packages
    .sort((a, b) => new Date(b.lastUpdated).getTime() - new Date(a.lastUpdated).getTime())
    .slice(0, 3);

  return (
    <div className="p-6 space-y-6">
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
        <Card className="md:col-span-2">
          <CardHeader>
            <CardTitle>Welcome to WinConveyoR</CardTitle>
          </CardHeader>
          <CardContent>
            <p className="text-gray-600 dark:text-gray-300 mb-4">
              Discover, install, and manage Windows applications from one central location.
              We've designed WinConveyoR with accessibility in mind to provide a seamless experience for all users.
            </p>
            
            <div className="flex flex-wrap gap-3">
              <Button
                variant="primary"
                leftIcon={<Download size={18} />}
              >
                Browse packages
              </Button>
              
              {updatablePackages.length > 0 && (
                <Button
                  variant="secondary"
                  leftIcon={<RefreshCw size={18} />}
                >
                  Update all ({updatablePackages.length})
                </Button>
              )}
            </div>
          </CardContent>
        </Card>
        
        <Card>
          <CardHeader>
            <CardTitle>Quick stats</CardTitle>
          </CardHeader>
          <CardContent>
            <div className="space-y-4">
              <div>
                <p className="text-sm text-gray-500 dark:text-gray-400">Installed packages</p>
                <p className="text-2xl font-semibold text-gray-900 dark:text-gray-100">{installedPackages.length}</p>
              </div>
              
              <div>
                <p className="text-sm text-gray-500 dark:text-gray-400">Updates available</p>
                <p className="text-2xl font-semibold text-gray-900 dark:text-gray-100">{updatablePackages.length}</p>
              </div>
              
              <div>
                <p className="text-sm text-gray-500 dark:text-gray-400">Total package catalog</p>
                <p className="text-2xl font-semibold text-gray-900 dark:text-gray-100">{packages.length}</p>
              </div>
            </div>
          </CardContent>
        </Card>
      </div>
      
      <div>
        <div className="flex items-center justify-between mb-4">
          <h2 className="text-xl font-semibold text-gray-900 dark:text-gray-100">Top Packages</h2>
          <Button
            variant="ghost"
            size="sm"
            rightIcon={<ChevronRight size={16} />}
          >
            View all
          </Button>
        </div>
        
        <PackageGrid packages={topPackages} isLoading={isLoading} />
      </div>
      
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
        <div className="md:col-span-2">
          <div className="flex items-center justify-between mb-4">
            <h2 className="text-xl font-semibold text-gray-900 dark:text-gray-100">Recently updated</h2>
            <Button
              variant="ghost"
              size="sm"
              rightIcon={<ChevronRight size={16} />}
            >
              View all
            </Button>
          </div>
          
          <div className="space-y-4">
            {recentlyUpdated.map(pkg => (
              <Card key={pkg.id} hoverable className="p-4 flex items-center">
                <div className="flex-shrink-0 w-10 h-10 bg-gray-100 dark:bg-gray-800 rounded-md flex items-center justify-center mr-4">
                  <img 
                    src={pkg.iconUrl} 
                    alt={`${pkg.name} logo`} 
                    className="w-6 h-6 object-contain"
                    onError={(e) => {
                      (e.target as HTMLImageElement).src = 'https://cdn.jsdelivr.net/gh/simple-icons/simple-icons/icons/windows.svg';
                    }}
                  />
                </div>
                
                <div className="flex-1 min-w-0">
                  <h3 className="text-base font-medium text-gray-900 dark:text-gray-100 truncate">{pkg.name}</h3>
                  <p className="text-sm text-gray-500 dark:text-gray-400">
                    v{pkg.version} • Updated {new Date(pkg.lastUpdated).toLocaleDateString()}
                  </p>
                </div>
                
                <Button
                  variant={pkg.isInstalled ? (pkg.isUpdatable ? "secondary" : "ghost") : "primary"}
                  size="sm"
                  leftIcon={pkg.isInstalled ? (pkg.isUpdatable ? <RefreshCw size={14} /> : undefined) : <Download size={14} />}
                >
                  {pkg.isInstalled ? (pkg.isUpdatable ? "Update" : "Installed") : "Install"}
                </Button>
              </Card>
            ))}
          </div>
        </div>
        
        <div>
          <div className="flex items-center justify-between mb-4">
            <h2 className="text-xl font-semibold text-gray-900 dark:text-gray-100">Accessibility features</h2>
          </div>
          
          <Card>
            <CardContent className="p-4">
              <ul className="space-y-3 text-gray-600 dark:text-gray-300">
                <li className="flex items-start">
                  <span className="inline-flex items-center justify-center rounded-full bg-blue-100 dark:bg-blue-900 text-blue-600 dark:text-blue-300 p-1 mr-2">
                    <svg className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 13l4 4L19 7" />
                    </svg>
                  </span>
                  <span>Screen reader compatibility</span>
                </li>
                <li className="flex items-start">
                  <span className="inline-flex items-center justify-center rounded-full bg-blue-100 dark:bg-blue-900 text-blue-600 dark:text-blue-300 p-1 mr-2">
                    <svg className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 13l4 4L19 7" />
                    </svg>
                  </span>
                  <span>High contrast mode</span>
                </li>
                <li className="flex items-start">
                  <span className="inline-flex items-center justify-center rounded-full bg-blue-100 dark:bg-blue-900 text-blue-600 dark:text-blue-300 p-1 mr-2">
                    <svg className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 13l4 4L19 7" />
                    </svg>
                  </span>
                  <span>Keyboard navigation support</span>
                </li>
                <li className="flex items-start">
                  <span className="inline-flex items-center justify-center rounded-full bg-blue-100 dark:bg-blue-900 text-blue-600 dark:text-blue-300 p-1 mr-2">
                    <svg className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 13l4 4L19 7" />
                    </svg>
                  </span>
                  <span>Adjustable font sizes</span>
                </li>
                <li className="flex items-start">
                  <span className="inline-flex items-center justify-center rounded-full bg-blue-100 dark:bg-blue-900 text-blue-600 dark:text-blue-300 p-1 mr-2">
                    <svg className="h-4 w-4" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 13l4 4L19 7" />
                    </svg>
                  </span>
                  <span>Reduced motion option</span>
                </li>
              </ul>
            </CardContent>
          </Card>
        </div>
      </div>
    </div>
  );
};

export default HomePage;