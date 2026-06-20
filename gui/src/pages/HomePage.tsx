import { useState } from 'react';
import { Package as PackageIcon, Download, RefreshCw } from 'lucide-react';
import { usePackages } from '../context/PackageContext';
import { useCli } from '../context/CliContext';
import Card, { CardHeader, CardTitle, CardContent } from '../components/ui/Card';
import Button from '../components/ui/Button';
import Input from '../components/ui/Input';

const HomePage: React.FC = () => {
  const { installedPackages, updatablePackages, addInstalled } = usePackages();
  const { busy, runUpdate, runInstall } = useCli();
  const [installName, setInstallName] = useState('');

  const handleInstall = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!installName.trim()) return;
    const pkg = installName.trim();
    const result = await runInstall(pkg);
    if (result.code === 0) {
      addInstalled(pkg, result.version);
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
          <p className="text-gray-600 dark:text-gray-300">
            Manage your Windows packages from one place.
          </p>
        </CardContent>
      </Card>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        <Card>
          <CardContent className="flex items-center space-x-3 p-4">
            <PackageIcon className="text-blue-500" size={24} />
            <div>
              <p className="text-sm text-gray-500 dark:text-gray-400">Installed</p>
              <p className="text-2xl font-semibold">{installedPackages.length}</p>
            </div>
          </CardContent>
        </Card>
        <Card>
          <CardContent className="flex items-center space-x-3 p-4">
            <RefreshCw className="text-orange-500" size={24} />
            <div>
              <p className="text-sm text-gray-500 dark:text-gray-400">Updates</p>
              <p className="text-2xl font-semibold">{updatablePackages.length}</p>
            </div>
          </CardContent>
        </Card>
        <Card>
          <CardContent className="flex items-center space-x-3 p-4">
            <Download className={busy ? 'text-orange-500' : 'text-green-500'} size={24} />
            <div>
              <p className="text-sm text-gray-500 dark:text-gray-400">Status</p>
              <div className="flex items-center space-x-2">
                <span className={`inline-block w-2 h-2 rounded-full ${busy ? 'bg-orange-500 animate-pulse' : 'bg-green-500'}`} />
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
