import { Package as PackageIcon, RefreshCw, CheckCircle } from 'lucide-react';
import { usePackages } from '../context/PackageContext';
import Header from '../components/layout/Header';
import Card from '../components/ui/Card';
import Button from '../components/ui/Button';

const UpdatesPage: React.FC = () => {
  const { updatablePackages } = usePackages();

  return (
    <>
      <Header title="Available Updates" />
      <div className="p-6">
        {updatablePackages.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-16 text-center">
            <div className="rounded-full bg-green-100 dark:bg-green-900 p-4 mb-4">
              <CheckCircle size={32} className="text-green-500 dark:text-green-400" />
            </div>
            <h3 className="text-lg font-medium text-gray-900 dark:text-gray-100">All up to date</h3>
            <p className="mt-1 text-sm text-gray-500 dark:text-gray-400">
              All your packages are on the latest version.
            </p>
          </div>
        ) : (
          <>
            <div className="flex items-center justify-between mb-4">
              <p className="text-sm text-gray-500 dark:text-gray-400">
                {updatablePackages.length} update{updatablePackages.length !== 1 ? 's' : ''} available
              </p>
              <Button variant="primary" size="sm" leftIcon={<RefreshCw size={16} />}>
                Update all
              </Button>
            </div>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {updatablePackages.map(pkg => (
                <Card key={pkg.name} className="flex flex-col">
                  <div className="p-4 flex items-start space-x-3">
                    <div className="flex-shrink-0 w-10 h-10 bg-orange-100 dark:bg-orange-900 rounded-lg flex items-center justify-center">
                      <PackageIcon size={20} className="text-orange-600 dark:text-orange-400" />
                    </div>
                    <div className="flex-1 min-w-0">
                      <h3 className="text-base font-semibold text-gray-900 dark:text-gray-100 truncate">
                        {pkg.name}
                      </h3>
                      <p className="text-xs text-gray-500 dark:text-gray-400 mt-0.5">
                        v{pkg.version}
                      </p>
                    </div>
                  </div>
                  <div className="mt-auto px-4 pb-4 flex justify-end">
                    <Button variant="secondary" size="sm" leftIcon={<RefreshCw size={14} />}>
                      Update
                    </Button>
                  </div>
                </Card>
              ))}
            </div>
          </>
        )}
      </div>
    </>
  );
};

export default UpdatesPage;
