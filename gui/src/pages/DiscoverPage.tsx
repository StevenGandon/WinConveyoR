import { useState, useMemo, useEffect } from 'react';
import { Search, Package as PackageIcon, Download, Info } from 'lucide-react';
import { useCli } from '../context/CliContext';
import { usePackages } from '../context/PackageContext';
import Header from '../components/layout/Header';
import Card from '../components/ui/Card';
import Button from '../components/ui/Button';
import Input from '../components/ui/Input';
import Badge from '../components/ui/Badge';

const DiscoverPage: React.FC = () => {
  const { busy, runInstall, runInfo } = useCli();
  const { installedPackages, availablePackages, discoverQuery, setDiscoverQuery, refreshInstalled, loadAvailable } = usePackages();
  const [infoVariants, setInfoVariants] = useState<{ version: string; arch: string; machine: string }[] | null>(null);
  const [infoTarget, setInfoTarget] = useState('');

  useEffect(() => {
    if (availablePackages.length === 0) {
      loadAvailable();
    }
  }, []);

  const filtered = useMemo(() => {
    if (!discoverQuery.trim()) return availablePackages;
    const q = discoverQuery.toLowerCase();
    return availablePackages.filter(pkg => pkg.name.toLowerCase().includes(q));
  }, [availablePackages, discoverQuery]);

  const handleInstall = async (name: string) => {
    const result = await runInstall(name);
    if (result.code === 0) {
      await refreshInstalled();
    }
  };

  const handleInfo = async (name: string) => {
    setInfoTarget(name);
    const output = await runInfo(name);
    try {
      const match = output.match(/\[.*\]/s);
      if (match) {
        setInfoVariants(JSON.parse(match[0]));
        return;
      }
    } catch { /* ignore */ }
    setInfoVariants([]);
  };

  const isInstalled = (name: string) => installedPackages.some(p => p.name === name);

  return (
    <>
      <Header title="Discover Packages" />
      <div className="p-6 space-y-4">
        <Input
          fullWidth
          placeholder="Filter packages..."
          value={discoverQuery}
          onChange={(e) => setDiscoverQuery(e.target.value)}
          disabled={busy}
        />

        {infoVariants && (
          <Card className="p-4">
            <div className="flex items-center justify-between mb-3">
              <div className="flex items-center space-x-2">
                <PackageIcon size={16} className="text-wc-accent dark:text-wc-accent-bright" />
                <h3 className="text-sm font-semibold text-wc-fg dark:text-wc-fg-dark">
                  {infoTarget}
                </h3>
                <Badge variant="info" size="sm">
                  {infoVariants.length} variant{infoVariants.length !== 1 ? 's' : ''}
                </Badge>
              </div>
              <Button variant="ghost" size="sm" onClick={() => setInfoVariants(null)}>
                Close
              </Button>
            </div>
            {infoVariants.length === 0 ? (
              <p className="text-sm text-wc-muted dark:text-wc-muted-dark">No variants found.</p>
            ) : (
              <div className="overflow-x-auto">
                <table className="w-full text-sm">
                  <thead>
                    <tr className="border-b border-wc-border dark:border-wc-border-dark">
                      <th className="text-left py-2 pr-4 text-xs font-medium text-wc-muted dark:text-wc-muted-dark uppercase tracking-wider">Version</th>
                      <th className="text-left py-2 pr-4 text-xs font-medium text-wc-muted dark:text-wc-muted-dark uppercase tracking-wider">Architecture</th>
                      <th className="text-left py-2 text-xs font-medium text-wc-muted dark:text-wc-muted-dark uppercase tracking-wider">Machine</th>
                    </tr>
                  </thead>
                  <tbody>
                    {infoVariants.map((v, i) => (
                      <tr key={i} className="border-b border-wc-border/50 dark:border-wc-border-dark/50 last:border-0">
                        <td className="py-2 pr-4">
                          <Badge variant="default" size="sm">{v.version}</Badge>
                        </td>
                        <td className="py-2 pr-4 text-wc-fg dark:text-wc-fg-dark">{v.arch}</td>
                        <td className="py-2 text-wc-fg dark:text-wc-fg-dark">{v.machine}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            )}
          </Card>
        )}

        {availablePackages.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-16 text-center">
            <div className="rounded-full bg-wc-accent-soft dark:bg-wc-accent-soft-dark p-4 mb-4">
              <Search size={32} className="text-wc-accent dark:text-wc-accent-bright" />
            </div>
            <h3 className="text-lg font-medium text-wc-fg dark:text-wc-fg-dark">
              No packages available
            </h3>
            <p className="mt-1 text-sm text-wc-muted dark:text-wc-muted-dark max-w-sm">
              Click <span className="font-mono text-xs bg-wc-surface dark:bg-wc-surface-dark px-1.5 py-0.5 rounded">Update</span> on the Home page first to sync sources.
            </p>
          </div>
        ) : (
          <>
            <p className="text-sm text-wc-muted dark:text-wc-muted-dark">
              {filtered.length} package{filtered.length !== 1 ? 's' : ''}
              {discoverQuery.trim() ? ` matching "${discoverQuery}"` : ' available'}
            </p>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {filtered.map(pkg => (
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
                        {isInstalled(pkg.name) && (
                          <Badge variant="success" size="sm">Installed</Badge>
                        )}
                      </div>
                      <p className="text-xs text-wc-muted dark:text-wc-muted-dark mt-0.5">
                        v{pkg.version}
                      </p>
                    </div>
                  </div>
                  <div className="mt-auto px-4 pb-4 flex justify-end space-x-2">
                    <Button
                      variant="ghost"
                      size="sm"
                      leftIcon={<Info size={14} />}
                      onClick={() => handleInfo(pkg.name)}
                      disabled={busy}
                    >
                      Info
                    </Button>
                    {!isInstalled(pkg.name) && (
                      <Button
                        variant="primary"
                        size="sm"
                        leftIcon={<Download size={14} />}
                        onClick={() => handleInstall(pkg.name)}
                        disabled={busy}
                      >
                        Install
                      </Button>
                    )}
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

export default DiscoverPage;
