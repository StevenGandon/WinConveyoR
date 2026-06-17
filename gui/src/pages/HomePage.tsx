import { useState, useEffect, useRef } from 'react';
import { Package as PackageIcon, Download, RefreshCw, Terminal } from 'lucide-react';
import { usePackages } from '../context/PackageContext';
import Card, { CardHeader, CardTitle, CardContent } from '../components/ui/Card';
import Button from '../components/ui/Button';
import Input from '../components/ui/Input';

const HomePage: React.FC = () => {
  const { installedPackages, updatablePackages, addInstalled } = usePackages();
  const [installName, setInstallName] = useState('');
  const [log, setLog] = useState('');
  const [busy, setBusy] = useState(false);
  const logRef = useRef<HTMLPreElement>(null);

  useEffect(() => {
    const cleanup = window.electronAPI?.onCliOutput((text) => {
      setLog(prev => prev + text);
    });
    return () => cleanup?.();
  }, []);

  useEffect(() => {
    if (logRef.current) logRef.current.scrollTop = logRef.current.scrollHeight;
  }, [log]);

  const handleUpdate = async () => {
    if (!window.electronAPI) return;
    setBusy(true);
    setLog('');
    const result = await window.electronAPI.runUpdate();
    if (result.code !== 0) setLog(prev => prev + `\nExited with code ${result.code}`);
    setBusy(false);
  };

  const handleInstall = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!window.electronAPI || !installName.trim()) return;
    setBusy(true);
    setLog('');
    const pkg = installName.trim();
    const result = await window.electronAPI.runInstall(pkg);
    if (result.code === 0) {
      const resolved = result.output.match(/install: resolving '([^']+)'/g);
      if (resolved) {
        resolved.forEach(m => {
          const name = m.match(/'([^']+)'/)?.[1];
          if (name) addInstalled(name);
        });
      }
    } else {
      setLog(prev => prev + `\nExited with code ${result.code}`);
    }
    setBusy(false);
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
              onClick={handleUpdate}
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

      {log && (
        <Card>
          <CardHeader>
            <div className="flex items-center space-x-2">
              <Terminal size={18} className="text-gray-500" />
              <CardTitle>Output</CardTitle>
            </div>
          </CardHeader>
          <CardContent>
            <pre
              ref={logRef}
              className="bg-gray-950 text-green-400 text-sm font-mono p-4 rounded-lg max-h-64 overflow-auto whitespace-pre-wrap"
            >
              {log}
            </pre>
          </CardContent>
        </Card>
      )}
    </div>
  );
};

export default HomePage;
