import { useState } from 'react';
import { Package as PackageIcon, RefreshCw, Trash2, ShieldCheck } from 'lucide-react';
import { usePackages } from '../context/PackageContext';
import { useCli } from '../context/CliContext';
import { logActivity } from '../services/activity';
import Header from '../components/layout/Header';
import Card from '../components/ui/Card';
import Badge from '../components/ui/Badge';
import Button from '../components/ui/Button';

interface CheckResult {
  match: boolean;
  expected: string;
  actual: string;
}

const InstalledPage: React.FC = () => {
  const { installedPackages, refreshInstalled } = usePackages();
  const { busy, runUninstall, runUpgrade, runCheck } = useCli();
  const [checkResults, setCheckResults] = useState<Record<string, CheckResult>>({});

  const handleUninstall = async (name: string) => {
    const code = await runUninstall(name);
    if (code === 0) {
      logActivity('uninstalled', name);
      setCheckResults(prev => {
        const next = { ...prev };
        delete next[name];
        return next;
      });
      await refreshInstalled();
    }
  };

  const handleUpgrade = async (name: string) => {
    const code = await runUpgrade(name);
    if (code === 0) {
      logActivity('updated', name);
      await refreshInstalled();
    }
  };

  const handleCheck = async (name: string) => {
    const result = await runCheck(name);
    if (result) {
      setCheckResults(prev => ({
        ...prev,
        [name]: { match: result.match === 1, expected: result.expected, actual: result.actual },
      }));
    }
  };

  return (
    <>
      <Header title="Installed Packages" />
      <div className="p-6">
        {installedPackages.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-16 text-center">
            <div className="rounded-full bg-wc-surface dark:bg-wc-surface-dark p-4 mb-4">
              <PackageIcon size={32} className="text-wc-muted dark:text-wc-muted-dark" />
            </div>
            <h3 className="text-lg font-medium text-wc-fg dark:text-wc-fg-dark">No packages installed</h3>
            <p className="mt-1 text-sm text-wc-muted dark:text-wc-muted-dark">
              Use the Home page to install your first package.
            </p>
          </div>
        ) : (
          <>
            <p className="text-sm text-wc-muted dark:text-wc-muted-dark mb-4">
              {installedPackages.length} package{installedPackages.length !== 1 ? 's' : ''} installed
            </p>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {installedPackages.map(pkg => {
                const check = checkResults[pkg.name];
                return (
                  <Card key={pkg.name} className="flex flex-col">
                    <div className="p-4 flex items-start space-x-3">
                      <div className="flex-shrink-0 w-10 h-10 bg-wc-accent-soft dark:bg-wc-accent-soft-dark rounded-lg flex items-center justify-center">
                        <PackageIcon size={20} className="text-wc-accent dark:text-wc-accent-bright" />
                      </div>
                      <div className="flex-1 min-w-0">
                        <div className="flex items-center justify-between">
                          <h3 className="text-base font-semibold text-wc-fg dark:text-wc-fg-dark truncate">
                            {pkg.name}
                          </h3>
                          <Badge
                            variant={pkg.isUpdatable ? 'warning' : 'success'}
                            size="sm"
                          >
                            {pkg.isUpdatable ? 'Upgrade' : 'Installed'}
                          </Badge>
                        </div>
                        <p className="text-xs text-wc-muted dark:text-wc-muted-dark mt-0.5">
                          v{pkg.version}
                        </p>
                        {pkg.description && (
                          <p className="text-sm text-wc-muted dark:text-wc-muted-dark mt-2 line-clamp-2">
                            {pkg.description}
                          </p>
                        )}
                        {check && (
                          <div className="mt-2">
                            <Badge variant={check.match ? 'success' : 'danger'} size="sm">
                              {check.match ? 'Hash OK' : 'Hash mismatch'}
                            </Badge>
                            {!check.match && (
                              <p className="text-xs text-wc-muted dark:text-wc-muted-dark mt-1 font-mono break-all">
                                Expected: {check.expected}<br />
                                Actual: {check.actual}
                              </p>
                            )}
                          </div>
                        )}
                      </div>
                    </div>
                    <div className="mt-auto px-4 pb-4 flex justify-end space-x-2">
                      <Button
                        variant="outline"
                        size="sm"
                        leftIcon={<ShieldCheck size={14} />}
                        onClick={() => handleCheck(pkg.name)}
                        disabled={busy}
                      >
                        Check
                      </Button>
                      {pkg.isUpdatable && (
                        <Button
                          variant="secondary"
                          size="sm"
                          leftIcon={<RefreshCw size={14} />}
                          onClick={() => handleUpgrade(pkg.name)}
                          disabled={busy}
                        >
                          Upgrade
                        </Button>
                      )}
                      <Button
                        variant="danger"
                        size="sm"
                        leftIcon={<Trash2 size={14} />}
                        onClick={() => handleUninstall(pkg.name)}
                        disabled={busy}
                      >
                        Uninstall
                      </Button>
                    </div>
                  </Card>
                );
              })}
            </div>
          </>
        )}
      </div>
    </>
  );
};

export default InstalledPage;
