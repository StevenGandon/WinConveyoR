import { Search } from 'lucide-react';
import Header from '../components/layout/Header';

const DiscoverPage: React.FC = () => {
  return (
    <>
      <Header title="Discover Packages" />
      <div className="p-6">
        <div className="flex flex-col items-center justify-center py-16 text-center">
          <div className="rounded-full bg-purple-100 dark:bg-purple-900 p-4 mb-4">
            <Search size={32} className="text-purple-500 dark:text-purple-400" />
          </div>
          <h3 className="text-lg font-medium text-gray-900 dark:text-gray-100">
            Discover packages
          </h3>
          <p className="mt-1 text-sm text-gray-500 dark:text-gray-400 max-w-sm">
            Package discovery will be available once connected to a WCR source.
            Use <span className="font-mono text-xs bg-gray-100 dark:bg-gray-800 px-1.5 py-0.5 rounded">Update</span> on the Home page first.
          </p>
        </div>
      </div>
    </>
  );
};

export default DiscoverPage;
