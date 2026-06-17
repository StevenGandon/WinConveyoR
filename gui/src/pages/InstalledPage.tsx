import { usePackages } from '../context/PackageContext';
import Header from '../components/layout/Header';

const InstalledPage: React.FC = () => {
  const { installedPackages } = usePackages();

  return (
    <>
      <Header title="Installed Packages" />
      <div className="p-6">
        {installedPackages.length === 0 ? (
          <p className="text-gray-500 dark:text-gray-400">No packages installed yet.</p>
        ) : (
          <ul className="space-y-2">
            {installedPackages.map(pkg => (
              <li key={pkg.name} className="flex items-center justify-between p-3 bg-white dark:bg-gray-800 rounded-lg border border-gray-200 dark:border-gray-700">
                <div>
                  <span className="font-medium text-gray-900 dark:text-gray-100">{pkg.name}</span>
                  <span className="ml-2 text-sm text-gray-500 dark:text-gray-400">v{pkg.version}</span>
                </div>
                {pkg.isUpdatable && (
                  <span className="text-xs bg-orange-100 text-orange-700 dark:bg-orange-900 dark:text-orange-300 px-2 py-1 rounded">
                    Update available
                  </span>
                )}
              </li>
            ))}
          </ul>
        )}
      </div>
    </>
  );
};

export default InstalledPage;
