import { Package as PackageIcon, RefreshCw, CheckCircle } from 'lucide-react';
import { usePackages } from '../context/PackageContext';
import { useCli } from '../context/CliContext';
import Header from '../components/layout/Header';
import Card from '../components/ui/Card';
import Button from '../components/ui/Button';

const UpgradesPage: React.FC = () => {
  const { updatablePackages, refreshInstalled } = usePackages();
  const { busy, runUpgrade } = useCli();

  const handleUpgradeAll = async () => {
    const code = await runUpgrade();
    if (code === 0) await refreshInstalled();
  };

  const handleUpgradeOne = async (name: string) => {
    const code = await runUpgrade(name);
    if (code === 0) await refreshInstalled();
  };

  return (
    <>
      <Header title="Available Upgrades" />
      <div className="p-6">
        {updatablePackages.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-16 text-center">
            <div className="rounded-full bg-wc-success-soft dark:bg-wc-success-soft-dark p-4 mb-4">
              <CheckCircle size={32} className="text-wc-success dark:text-wc-success-dark" />
            </div>
            <h3 className="text-lg font-medium text-wc-fg dark:text-wc-fg-dark">All up to date</h3>
            <p className="mt-1 text-sm text-wc-muted dark:text-wc-muted-dark">
              All your packages are on the latest version.
            </p>
          </div>
        ) : (
          <>
            <div className="flex items-center justify-between mb-4">
              <p className="text-sm text-wc-muted dark:text-wc-muted-dark">
                {updatablePackages.length} upgrade{updatablePackages.length !== 1 ? 's' : ''} available
              </p>
              <Button
                variant="primary"
                size="sm"
                leftIcon={<RefreshCw size={16} />}
                onClick={handleUpgradeAll}
                disabled={busy}
                isLoading={busy}
              >
                Upgrade all
              </Button>
            </div>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {updatablePackages.map(pkg => (
                <Card key={pkg.name} className="flex flex-col">
                  <div className="p-4 flex items-start space-x-3">
                    <div className="flex-shrink-0 w-10 h-10 bg-wc-accent-soft dark:bg-wc-accent-soft-dark rounded-lg flex items-center justify-center">
                      <PackageIcon size={20} className="text-wc-accent dark:text-wc-accent-bright" />
                    </div>
                    <div className="flex-1 min-w-0">
                      <h3 className="text-base font-semibold text-wc-fg dark:text-wc-fg-dark truncate">
                        {pkg.name}
                      </h3>
                      <p className="text-xs text-wc-muted dark:text-wc-muted-dark mt-0.5">
                        v{pkg.version}
                      </p>
                    </div>
                  </div>
                  <div className="mt-auto px-4 pb-4 flex justify-end">
                    <Button
                      variant="secondary"
                      size="sm"
                      leftIcon={<RefreshCw size={14} />}
                      onClick={() => handleUpgradeOne(pkg.name)}
                      disabled={busy}
                    >
                      Upgrade
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

export default UpgradesPage;
