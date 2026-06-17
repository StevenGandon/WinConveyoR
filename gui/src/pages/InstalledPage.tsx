import { Package as PackageIcon, RefreshCw } from 'lucide-react';
import { usePackages } from '../context/PackageContext';
import Header from '../components/layout/Header';
import Card from '../components/ui/Card';
import Badge from '../components/ui/Badge';
import Button from '../components/ui/Button';

const InstalledPage: React.FC = () => {
  const { installedPackages } = usePackages();

  return (
    <>
      <Header title="Installed Packages" />
      <div className="p-6">
        {installedPackages.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-16 text-center">
            <div className="rounded-full bg-gray-100 dark:bg-gray-800 p-4 mb-4">
              <PackageIcon size={32} className="text-gray-400 dark:text-gray-500" />
            </div>
            <h3 className="text-lg font-medium text-gray-900 dark:text-gray-100">No packages installed</h3>
            <p className="mt-1 text-sm text-gray-500 dark:text-gray-400">
              Use the Home page to install your first package.
            </p>
          </div>
        ) : (
          <>
            <p className="text-sm text-gray-500 dark:text-gray-400 mb-4">
              {installedPackages.length} package{installedPackages.length !== 1 ? 's' : ''} installed
            </p>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {installedPackages.map(pkg => (
                <Card key={pkg.name} className="flex flex-col">
                  <div className="p-4 flex items-start space-x-3">
                    <div className="flex-shrink-0 w-10 h-10 bg-blue-100 dark:bg-blue-900 rounded-lg flex items-center justify-center">
                      <PackageIcon size={20} className="text-blue-600 dark:text-blue-400" />
                    </div>
                    <div className="flex-1 min-w-0">
                      <div className="flex items-center justify-between">
                        <h3 className="text-base font-semibold text-gray-900 dark:text-gray-100 truncate">
                          {pkg.name}
                        </h3>
                        <Badge
                          variant={pkg.isUpdatable ? 'warning' : 'success'}
                          size="sm"
                        >
                          {pkg.isUpdatable ? 'Update' : 'Installed'}
                        </Badge>
                      </div>
                      <p className="text-xs text-gray-500 dark:text-gray-400 mt-0.5">
                        v{pkg.version}
                      </p>
                      {pkg.description && (
                        <p className="text-sm text-gray-600 dark:text-gray-300 mt-2 line-clamp-2">
                          {pkg.description}
                        </p>
                      )}
                    </div>
                  </div>
                  {pkg.isUpdatable && (
                    <div className="mt-auto px-4 pb-4 flex justify-end">
                      <Button variant="secondary" size="sm" leftIcon={<RefreshCw size={14} />}>
                        Update
                      </Button>
                    </div>
                  )}
                </Card>
              ))}
            </div>
          </>
        )}
      </div>
    </>
  );
};

export default InstalledPage;
