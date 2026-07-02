import { useState } from 'react';
import { Package as PackageIcon, Download, RefreshCw } from 'lucide-react';
import { usePackages } from '../context/PackageContext';
import { useCli } from '../context/CliContext';
import Card, { CardHeader, CardTitle, CardContent } from '../components/ui/Card';
import Button from '../components/ui/Button';
import Input from '../components/ui/Input';

const HomePage: React.FC = () => {
  const { installedPackages, updatablePackages, refreshInstalled } = usePackages();
  const { busy, runUpdate, runInstall } = useCli();
  const [installName, setInstallName] = useState('');

  const handleInstall = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!installName.trim()) return;
    const pkg = installName.trim();
    const result = await runInstall(pkg);
    if (result.code === 0) {
      await refreshInstalled();
    }
    setInstallName('');
  };

  return (
    <div className="p-6 space-y-6">
      <Card>
        <CardHeader>
          <CardTitle>Welcome to WinConveyoR</CardTitle>
        </CardHeader>
        <CardContent>
          <p className="text-wc-muted dark:text-wc-muted-dark">
            Manage your Windows packages from one place.
          </p>
        </CardContent>
      </Card>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        <Card>
          <CardContent className="flex items-center space-x-3 p-4">
            <PackageIcon className="text-wc-accent" size={24} />
            <div>
              <p className="text-sm text-wc-muted dark:text-wc-muted-dark">Installed</p>
              <p className="text-2xl font-semibold">{installedPackages.length}</p>
            </div>
          </CardContent>
        </Card>
        <Card>
          <CardContent className="flex items-center space-x-3 p-4">
            <RefreshCw className="text-wc-accent" size={24} />
            <div>
              <p className="text-sm text-wc-muted dark:text-wc-muted-dark">Updates</p>
              <p className="text-2xl font-semibold">{updatablePackages.length}</p>
            </div>
          </CardContent>
        </Card>
        <Card>
          <CardContent className="flex items-center space-x-3 p-4">
            <Download className={busy ? 'text-wc-accent' : 'text-wc-success dark:text-wc-success-dark'} size={24} />
            <div>
              <p className="text-sm text-wc-muted dark:text-wc-muted-dark">Status</p>
              <div className="flex items-center space-x-2">
                <span className={`inline-block w-2 h-2 rounded-full ${busy ? 'bg-wc-accent animate-pulse' : 'bg-wc-success dark:bg-wc-success-dark'}`} />
                <p className="text-2xl font-semibold">{busy ? '...' : 'OK'}</p>
              </div>
            </div>
          </CardContent>
        </Card>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        <Card>
          <CardHeader>
            <CardTitle>Update sources</CardTitle>
          </CardHeader>
          <CardContent>
            <Button
              variant="primary"
              leftIcon={<RefreshCw size={18} />}
              onClick={runUpdate}
              disabled={busy}
              isLoading={busy}
            >
              Update
            </Button>
          </CardContent>
        </Card>

        <Card>
          <CardHeader>
            <CardTitle>Install package</CardTitle>
          </CardHeader>
          <CardContent>
            <form onSubmit={handleInstall} className="flex space-x-2">
              <Input
                fullWidth
                placeholder="Package name (e.g. gcc)"
                value={installName}
                onChange={(e) => setInstallName(e.target.value)}
                disabled={busy}
              />
              <Button
                variant="primary"
                leftIcon={<Download size={18} />}
                disabled={busy || !installName.trim()}
                isLoading={busy}
              >
                Install
              </Button>
            </form>
          </CardContent>
        </Card>
      </div>

    </div>
  );
};

export default HomePage;
