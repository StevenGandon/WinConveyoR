import { Search } from 'lucide-react';
import Header from '../components/layout/Header';

const DiscoverPage: React.FC = () => {
  return (
    <>
      <Header title="Discover Packages" />
      <div className="p-6">
        <div className="flex flex-col items-center justify-center py-16 text-center">
          <div className="rounded-full bg-wc-accent-soft dark:bg-wc-accent-soft-dark p-4 mb-4">
            <Search size={32} className="text-wc-accent dark:text-wc-accent-bright" />
          </div>
          <h3 className="text-lg font-medium text-wc-fg dark:text-wc-fg-dark">
            Discover packages
          </h3>
          <p className="mt-1 text-sm text-wc-muted dark:text-wc-muted-dark max-w-sm">
            Package discovery will be available once connected to a WCR source.
            Use <span className="font-mono text-xs bg-wc-surface dark:bg-wc-surface-dark px-1.5 py-0.5 rounded">Update</span> on the Home page first.
          </p>
        </div>
      </div>
    </>
  );
};

export default DiscoverPage;
