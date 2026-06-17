import Header from '../components/layout/Header';

const DiscoverPage: React.FC = () => {
  return (
    <>
      <Header title="Discover Packages" />
      <div className="p-6">
        <p className="text-gray-500 dark:text-gray-400">
          Package discovery will be available once connected to a WCR source.
        </p>
      </div>
    </>
  );
};

export default DiscoverPage;
