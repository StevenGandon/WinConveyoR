import { useState, useEffect, useCallback, useRef } from 'react';
import { Search, Package as PackageIcon, Download, Upload, X, AlertCircle, HardDrive } from 'lucide-react';
import { toast } from 'sonner';
import { searchPackages, uploadPackage, downloadUrl, PackageOut } from '../services/api';
import { useAuth } from '../context/AuthContext';
import { useCli } from '../context/CliContext';
import { logActivity } from '../services/activity';
import Header from '../components/layout/Header';
import Card from '../components/ui/Card';
import Button from '../components/ui/Button';
import Input from '../components/ui/Input';
import Badge from '../components/ui/Badge';

const DEBOUNCE_MS = 300;

function formatSize(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
}

const CommunityHubPage: React.FC = () => {
  const { token, user } = useAuth();
  const { busy, runInstall } = useCli();
  const isAuthed = !!token && token !== 'anonymous';

  const [query, setQuery] = useState('');
  const [packages, setPackages] = useState<PackageOut[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [selected, setSelected] = useState<PackageOut | null>(null);
  const [showUpload, setShowUpload] = useState(false);

  // F29 — debounced search against the hub API, with loading/empty/error states.
  const fetchPackages = useCallback((q: string, signal: AbortSignal) => {
    setLoading(true);
    setError(null);
    searchPackages(q, signal)
      .then((list) => setPackages(list))
      .catch((e) => {
        if (signal.aborted) return;
        setError(e instanceof Error ? e.message : 'Failed to reach the hub');
      })
      .finally(() => {
        if (!signal.aborted) setLoading(false);
      });
  }, []);

  useEffect(() => {
    const controller = new AbortController();
    const timer = setTimeout(() => fetchPackages(query, controller.signal), DEBOUNCE_MS);
    return () => { clearTimeout(timer); controller.abort(); };
  }, [query, fetchPackages]);

  const refresh = useCallback(() => {
    const controller = new AbortController();
    fetchPackages(query, controller.signal);
  }, [query, fetchPackages]);

  // F28 — install a hub package through the existing CLI installer path.
  const handleInstall = async (pkg: PackageOut) => {
    const result = await runInstall(pkg.name);
    if (result.code === 0) {
      logActivity('installed', pkg.name, result.version ?? pkg.version);
    }
  };

  return (
    <>
      <Header title="Community Hub" />
      <div className="p-6 space-y-4">
        <div className="flex items-center gap-3">
          <div className="flex-1">
            <Input
              fullWidth
              leftIcon={<Search size={16} />}
              placeholder="Search community packages..."
              value={query}
              onChange={(e) => setQuery(e.target.value)}
            />
          </div>
          <Button
            variant="primary"
            leftIcon={<Upload size={16} />}
            onClick={() => setShowUpload(true)}
          >
            Create
          </Button>
        </div>

        {error ? (
          <div className="flex flex-col items-center justify-center py-16 text-center">
            <div className="rounded-full bg-wc-danger-soft dark:bg-wc-danger-soft-dark p-4 mb-4">
              <AlertCircle size={32} className="text-wc-danger dark:text-wc-danger-dark" />
            </div>
            <h3 className="text-lg font-medium text-wc-fg dark:text-wc-fg-dark">
              Couldn't load the hub
            </h3>
            <p className="mt-1 text-sm text-wc-muted dark:text-wc-muted-dark max-w-sm whitespace-pre-line">
              {error}
            </p>
            <Button variant="outline" size="sm" className="mt-4" onClick={refresh}>
              Retry
            </Button>
          </div>
        ) : loading ? (
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            {[0, 1, 2, 3, 4, 5].map((i) => (
              <Card key={i} className="flex flex-col">
                <div className="p-4 flex items-start space-x-3">
                  <div className="flex-shrink-0 w-10 h-10 rounded-lg bg-wc-surface dark:bg-wc-surface-dark animate-pulse" />
                  <div className="flex-1 min-w-0 space-y-2">
                    <div className="h-4 w-2/3 rounded bg-wc-surface dark:bg-wc-surface-dark animate-pulse" />
                    <div className="h-3 w-1/3 rounded bg-wc-surface dark:bg-wc-surface-dark animate-pulse" />
                  </div>
                </div>
              </Card>
            ))}
          </div>
        ) : packages.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-16 text-center">
            <div className="rounded-full bg-wc-accent-soft dark:bg-wc-accent-soft-dark p-4 mb-4">
              <Search size={32} className="text-wc-accent dark:text-wc-accent-bright" />
            </div>
            <h3 className="text-lg font-medium text-wc-fg dark:text-wc-fg-dark">
              {query.trim() ? 'No matching packages' : 'No packages published yet'}
            </h3>
            <p className="mt-1 text-sm text-wc-muted dark:text-wc-muted-dark max-w-sm">
              {query.trim()
                ? `Nothing matches "${query.trim()}".`
                : 'Be the first to publish one with Create.'}
            </p>
          </div>
        ) : (
          <>
            <p className="text-sm text-wc-muted dark:text-wc-muted-dark">
              {packages.length} package{packages.length !== 1 ? 's' : ''}
              {query.trim() ? ` matching "${query.trim()}"` : ' published'}
            </p>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {packages.map((pkg) => (
                <Card
                  key={pkg.id}
                  hoverable
                  onClick={() => setSelected(pkg)}
                  className="flex flex-col"
                >
                  <div className="p-4 flex items-start space-x-3">
                    <div className="flex-shrink-0 w-10 h-10 bg-wc-accent-soft dark:bg-wc-accent-soft-dark rounded-lg flex items-center justify-center">
                      <PackageIcon size={20} className="text-wc-accent dark:text-wc-accent-bright" />
                    </div>
                    <div className="flex-1 min-w-0">
                      <div className="flex items-center justify-between gap-2">
                        <h3 className="text-base font-semibold text-wc-fg dark:text-wc-fg-dark truncate">
                          {pkg.name}
                        </h3>
                        <Badge variant="neutral" size="sm">v{pkg.version}</Badge>
                      </div>
                      <p className="text-xs text-wc-muted dark:text-wc-muted-dark mt-0.5 truncate">
                        {pkg.description || `${pkg.arch} · ${pkg.machine}`}
                      </p>
                    </div>
                  </div>
                </Card>
              ))}
            </div>
          </>
        )}
      </div>

      {selected && (
        <PackageDetail
          pkg={selected}
          busy={busy}
          onClose={() => setSelected(null)}
          onInstall={() => handleInstall(selected)}
        />
      )}

      {showUpload && (
        <UploadDialog
          isAuthed={isAuthed}
          username={user?.username ?? null}
          token={token}
          onClose={() => setShowUpload(false)}
          onUploaded={() => {
            setShowUpload(false);
            refresh();
          }}
        />
      )}
    </>
  );
};

// ---- Detail / install + download (F28) -----------------------------------

interface DetailProps {
  pkg: PackageOut;
  busy: boolean;
  onClose: () => void;
  onInstall: () => void;
}

const PackageDetail: React.FC<DetailProps> = ({ pkg, busy, onClose, onInstall }) => (
  <div
    className="fixed inset-0 z-40 flex items-center justify-center bg-black/40 p-4"
    onClick={onClose}
  >
    <Card className="w-full max-w-lg" >
      <div onClick={(e) => e.stopPropagation()}>
        <div className="flex items-start justify-between p-4 border-b border-wc-border dark:border-wc-border-dark">
          <div className="flex items-center space-x-3 min-w-0">
            <div className="flex-shrink-0 w-10 h-10 bg-wc-accent-soft dark:bg-wc-accent-soft-dark rounded-lg flex items-center justify-center">
              <PackageIcon size={20} className="text-wc-accent dark:text-wc-accent-bright" />
            </div>
            <div className="min-w-0">
              <h3 className="text-base font-semibold text-wc-fg dark:text-wc-fg-dark truncate">{pkg.name}</h3>
              <p className="text-xs text-wc-muted dark:text-wc-muted-dark">v{pkg.version}</p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="p-1 rounded hover:bg-wc-surface dark:hover:bg-wc-surface-dark text-wc-muted dark:text-wc-muted-dark"
            aria-label="Close"
          >
            <X size={18} />
          </button>
        </div>

        <div className="p-4 space-y-3">
          {pkg.description && (
            <p className="text-sm text-wc-fg dark:text-wc-fg-dark">{pkg.description}</p>
          )}
          <div className="flex flex-wrap gap-2">
            <Badge variant="info" size="sm">{pkg.arch}</Badge>
            <Badge variant="info" size="sm">{pkg.machine}</Badge>
            <Badge variant="neutral" size="sm">{formatSize(pkg.size)}</Badge>
          </div>
          <div className="text-xs text-wc-muted dark:text-wc-muted-dark break-all">
            <span className="inline-flex items-center gap-1">
              <HardDrive size={12} /> {pkg.filename}
            </span>
            <div className="mt-1 font-mono">sha256: {pkg.checksum}</div>
          </div>
        </div>

        <div className="p-4 border-t border-wc-border dark:border-wc-border-dark flex justify-end gap-2">
          <a href={downloadUrl(pkg.id)} download>
            <Button variant="outline" leftIcon={<Download size={16} />}>Download</Button>
          </a>
          <Button
            variant="primary"
            leftIcon={<Download size={16} />}
            onClick={onInstall}
            disabled={busy}
            isLoading={busy}
          >
            Install
          </Button>
        </div>
      </div>
    </Card>
  </div>
);

// ---- Upload dialog (F27) --------------------------------------------------

interface UploadProps {
  isAuthed: boolean;
  username: string | null;
  token: string | null;
  onClose: () => void;
  onUploaded: () => void;
}

const UploadDialog: React.FC<UploadProps> = ({ isAuthed, token, onClose, onUploaded }) => {
  const [name, setName] = useState('');
  const [version, setVersion] = useState('');
  const [arch, setArch] = useState('');
  const [machine, setMachine] = useState('');
  const [description, setDescription] = useState('');
  const [file, setFile] = useState<File | null>(null);
  const [submitting, setSubmitting] = useState(false);
  const fileInput = useRef<HTMLInputElement>(null);

  const canSubmit = isAuthed && name.trim() && version.trim() && arch.trim() && machine.trim() && file && !submitting;

  const handleSubmit = async () => {
    if (!canSubmit || !token || !file) return;
    setSubmitting(true);
    const form = new FormData();
    form.append('name', name.trim());
    form.append('version', version.trim());
    form.append('arch', arch.trim());
    form.append('machine', machine.trim());
    if (description.trim()) form.append('description', description.trim());
    form.append('artifact', file);
    try {
      const pkg = await uploadPackage(form, token);
      toast.success(`${pkg.name} v${pkg.version} published`);
      logActivity('source_added', pkg.name, `v${pkg.version}`);
      onUploaded();
    } catch (e) {
      toast.error(e instanceof Error ? e.message : 'Upload failed');
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <div className="fixed inset-0 z-40 flex items-center justify-center bg-black/40 p-4" onClick={onClose}>
      <Card className="w-full max-w-lg">
        <div onClick={(e) => e.stopPropagation()}>
          <div className="flex items-center justify-between p-4 border-b border-wc-border dark:border-wc-border-dark">
            <h3 className="text-base font-semibold text-wc-fg dark:text-wc-fg-dark">Publish a package</h3>
            <button
              onClick={onClose}
              className="p-1 rounded hover:bg-wc-surface dark:hover:bg-wc-surface-dark text-wc-muted dark:text-wc-muted-dark"
              aria-label="Close"
            >
              <X size={18} />
            </button>
          </div>

          {!isAuthed ? (
            <div className="p-6 text-center space-y-2">
              <AlertCircle size={28} className="mx-auto text-wc-accent dark:text-wc-accent-bright" />
              <p className="text-sm text-wc-fg dark:text-wc-fg-dark">You need an account to publish.</p>
              <p className="text-xs text-wc-muted dark:text-wc-muted-dark">
                Sign out of guest mode and log in to share packages with the community.
              </p>
            </div>
          ) : (
            <div className="p-4 space-y-3">
              <div className="grid grid-cols-2 gap-3">
                <Input label="Name" fullWidth value={name} onChange={(e) => setName(e.target.value)} placeholder="hello" />
                <Input label="Version" fullWidth value={version} onChange={(e) => setVersion(e.target.value)} placeholder="1.0.0" />
                <Input label="Architecture" fullWidth value={arch} onChange={(e) => setArch(e.target.value)} placeholder="x86_64" />
                <Input label="Machine" fullWidth value={machine} onChange={(e) => setMachine(e.target.value)} placeholder="linux" />
              </div>
              <Input label="Description" fullWidth value={description} onChange={(e) => setDescription(e.target.value)} placeholder="Optional" />
              <div>
                <label className="block text-sm font-medium text-wc-fg dark:text-wc-fg-dark mb-1">Artifact</label>
                <input
                  ref={fileInput}
                  type="file"
                  className="hidden"
                  onChange={(e) => setFile(e.target.files?.[0] ?? null)}
                />
                <Button variant="outline" fullWidth leftIcon={<Upload size={16} />} onClick={() => fileInput.current?.click()}>
                  {file ? `${file.name} (${formatSize(file.size)})` : 'Choose file...'}
                </Button>
              </div>
            </div>
          )}

          {isAuthed && (
            <div className="p-4 border-t border-wc-border dark:border-wc-border-dark flex justify-end gap-2">
              <Button variant="ghost" onClick={onClose}>Cancel</Button>
              <Button
                variant="primary"
                onClick={handleSubmit}
                disabled={!canSubmit}
                isLoading={submitting}
                leftIcon={<Upload size={16} />}
              >
                Publish
              </Button>
            </div>
          )}
        </div>
      </Card>
    </div>
  );
};

export default CommunityHubPage;
