import { useState, useEffect, useCallback } from 'react';
import {
  Package as PackageIcon,
  Download,
  RefreshCw,
  RefreshCcw,
  ArrowDownToLine,
  Check,
  RotateCw,
  Plus,
  Trash2,
} from 'lucide-react';
import { usePackages } from '../context/PackageContext';
import { useCli } from '../context/CliContext';
import { useAuth } from '../context/AuthContext';
import { getActivity, logActivity, onActivityChange, ActivityEntry, ActivityKind } from '../services/activity';
import Card, { CardContent } from '../components/ui/Card';
import Button from '../components/ui/Button';

const GREETING_EVENING_HOUR = 18;
const GREETING_AFTERNOON_HOUR = 12;
const REFRESH_TICK_MS = 60_000;
const RECENT_ACTIVITY_LIMIT = 5;
const LAST_CHECK_KEY = 'wcr_last_check';

function readLastChecked(): number | null {
  try {
    const raw = localStorage.getItem(LAST_CHECK_KEY);
    return raw ? Number(raw) : null;
  } catch {
    return null;
  }
}

function writeLastChecked(ts: number): void {
  try {
    localStorage.setItem(LAST_CHECK_KEY, String(ts));
  } catch {
    /* ignore storage errors */
  }
}

interface HomePageProps {
  onNavigate?: (page: string) => void;
}

function greeting(hour: number): string {
  if (hour >= GREETING_EVENING_HOUR) return 'Good evening';
  if (hour >= GREETING_AFTERNOON_HOUR) return 'Good afternoon';
  return 'Good morning';
}

function relativeTime(ts: number, now: number): string {
  const seconds = Math.max(0, Math.round((now - ts) / 1000));
  if (seconds < 60) return 'just now';
  const minutes = Math.round(seconds / 60);
  if (minutes < 60) return `${minutes} min ago`;
  const hours = Math.round(minutes / 60);
  if (hours < 24) return `${hours} h ago`;
  const days = Math.round(hours / 24);
  return days === 1 ? 'yesterday' : `${days} days ago`;
}

const activityMeta: Record<ActivityKind, { verb: string; icon: React.ReactNode; tone: string }> = {
  installed: {
    verb: 'Installed',
    icon: <Check size={16} />,
    tone: 'bg-wc-success-soft text-wc-success dark:bg-wc-success-soft-dark dark:text-wc-success-dark',
  },
  updated: {
    verb: 'Updated',
    icon: <RotateCw size={16} />,
    tone: 'bg-wc-accent-soft text-wc-accent-deep dark:bg-wc-accent-soft-dark dark:text-wc-accent-bright',
  },
  source_added: {
    verb: 'Added source',
    icon: <Plus size={16} />,
    tone: 'bg-slate-200 text-slate-600 dark:bg-slate-800 dark:text-slate-400',
  },
  uninstalled: {
    verb: 'Uninstalled',
    icon: <Trash2 size={16} />,
    tone: 'bg-wc-danger-soft text-wc-danger dark:bg-wc-danger-soft-dark dark:text-wc-danger-dark',
  },
  sources_synced: {
    verb: 'Checked for updates',
    icon: <RefreshCcw size={16} />,
    tone: 'bg-slate-200 text-slate-600 dark:bg-slate-800 dark:text-slate-400',
  },
};

const HomePage: React.FC<HomePageProps> = ({ onNavigate }) => {
  const { installedPackages, updatablePackages, refreshInstalled, loadAvailable } = usePackages();
  const { busy, runUpdate, runUpgrade } = useCli();
  const { token, user } = useAuth();

  const [lastChecked, setLastChecked] = useState<number | null>(readLastChecked);
  const [activity, setActivity] = useState<ActivityEntry[]>(() => getActivity());
  const [now, setNow] = useState(() => Date.now());

  // Keep relative timestamps fresh and mirror the activity log across changes.
  useEffect(() => {
    const timer = setInterval(() => setNow(Date.now()), REFRESH_TICK_MS);
    return () => clearInterval(timer);
  }, []);

  useEffect(() => onActivityChange(() => setActivity(getActivity())), []);

  const displayName = token && token !== 'anonymous' && user?.username ? user.username : null;
  const updateCount = updatablePackages.length;

  const handleCheck = useCallback(async () => {
    await runUpdate();
    await loadAvailable();
    await refreshInstalled();
    logActivity('sources_synced', '');
    const ts = Date.now();
    writeLastChecked(ts);
    setLastChecked(ts);
  }, [runUpdate, loadAvailable, refreshInstalled]);

  const handleUpdateAll = useCallback(async () => {
    const targets = [...updatablePackages];
    const code = await runUpgrade();
    if (code === 0) {
      targets.forEach((pkg) => logActivity('updated', pkg.name, pkg.availableVersion));
      await refreshInstalled();
    }
  }, [updatablePackages, runUpgrade, refreshInstalled]);

  const handleUpdateOne = useCallback(async (name: string, availableVersion?: string) => {
    const code = await runUpgrade(name);
    if (code === 0) {
      logActivity('updated', name, availableVersion);
      await refreshInstalled();
    }
  }, [runUpgrade, refreshInstalled]);

  const subtitle = [
    `${updateCount} update${updateCount === 1 ? '' : 's'} ready`,
    lastChecked ? `last check ${relativeTime(lastChecked, now)}` : null,
  ].filter(Boolean).join(' · ');

  return (
    <div className="p-6 space-y-4">
      {/* Header — greeting + primary actions */}
      <div className="flex flex-wrap items-end justify-between gap-4">
        <div>
          <h1 className="text-2xl font-bold text-wc-fg dark:text-wc-fg-dark">
            {greeting(new Date().getHours())}{displayName ? `, ${displayName}` : ''}
          </h1>
          <p className="text-sm text-wc-muted dark:text-wc-muted-dark">{subtitle}</p>
        </div>
        <div className="flex gap-2">
          <Button
            variant="outline"
            size="sm"
            leftIcon={<RefreshCcw size={16} />}
            onClick={handleCheck}
            disabled={busy}
            isLoading={busy}
          >
            Check for updates
          </Button>
          <Button
            variant="primary"
            size="sm"
            leftIcon={<ArrowDownToLine size={16} />}
            onClick={handleUpdateAll}
            disabled={busy || updateCount === 0}
            isLoading={busy}
          >
            Update all
          </Button>
        </div>
      </div>

      {/* Stats row (kept from previous Home) */}
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
              <p className="text-sm text-wc-muted dark:text-wc-muted-dark">Update(s)</p>
              <p className="text-2xl font-semibold">{updateCount}</p>
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

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4 items-start">
        {/* Updates available */}
          <Card>
            <div className="flex items-center justify-between px-4 py-3 border-b border-wc-border dark:border-wc-border-dark">
              <div className="flex items-center gap-2">
                <h3 className="text-lg font-semibold text-wc-fg dark:text-wc-fg-dark">Updates available</h3>
                {updateCount > 0 && (
                  <span className="inline-flex items-center justify-center min-w-6 h-6 px-2 rounded-full text-xs font-semibold bg-wc-accent-soft text-wc-accent-deep dark:bg-wc-accent-soft-dark dark:text-wc-accent-bright">
                    {updateCount}
                  </span>
                )}
              </div>
              {onNavigate && (
                <button
                  onClick={() => onNavigate('upgrades')}
                  className="text-sm font-medium text-wc-accent hover:text-wc-accent-deep dark:hover:text-wc-accent-bright"
                >
                  View all
                </button>
              )}
            </div>
            {updatablePackages.length === 0 ? (
                <p className="p-5 text-sm text-wc-muted dark:text-wc-muted-dark text-center">
                  Everything is up to date.
                </p>
              ) : (
                <ul className="py-1">
                  {updatablePackages.map((pkg) => (
                    <li key={pkg.name} className="flex items-center gap-3 px-4 py-2.5">
                      <span className="flex items-center justify-center w-10 h-10 rounded-md bg-wc-surface dark:bg-wc-surface-dark text-wc-muted dark:text-wc-muted-dark font-semibold uppercase">
                        {pkg.name.charAt(0)}
                      </span>
                      <div className="flex-1 min-w-0">
                        <p className="font-medium truncate">{pkg.name}</p>
                        <p className="text-sm font-mono text-wc-muted dark:text-wc-muted-dark truncate">
                          {pkg.version}{pkg.availableVersion ? ` → ${pkg.availableVersion}` : ''}
                        </p>
                      </div>
                      <Button
                        variant="primary"
                        size="sm"
                        onClick={() => handleUpdateOne(pkg.name, pkg.availableVersion)}
                        disabled={busy}
                      >
                        Update
                      </Button>
                    </li>
                  ))}
                </ul>
              )}
          </Card>

          {/* Recent activity */}
          <Card>
            <div className="px-4 py-3 border-b border-wc-border dark:border-wc-border-dark">
              <h3 className="text-lg font-semibold text-wc-fg dark:text-wc-fg-dark">Recent activity</h3>
            </div>
            {activity.length === 0 ? (
                <p className="p-5 text-sm text-wc-muted dark:text-wc-muted-dark text-center">
                  No recent activity yet.
                </p>
              ) : (
                <ul className="py-1">
                  {activity.slice(0, RECENT_ACTIVITY_LIMIT).map((entry) => {
                    const meta = activityMeta[entry.kind];
                    return (
                      <li key={entry.id} className="flex items-center gap-3 px-4 py-2.5">
                        <span className={`flex items-center justify-center w-8 h-8 shrink-0 rounded-lg ${meta.tone}`}>
                          {meta.icon}
                        </span>
                        <div className="flex-1 min-w-0 leading-tight">
                          <p className="text-sm truncate">
                            <span className="font-medium">{meta.verb}</span>{' '}
                            <span className="text-wc-muted dark:text-wc-muted-dark">{entry.name}</span>
                            {entry.detail && (
                              <span className="font-mono text-wc-muted dark:text-wc-muted-dark"> {entry.detail}</span>
                            )}
                          </p>
                          <p className="text-xs text-wc-muted dark:text-wc-muted-dark">{relativeTime(entry.ts, now)}</p>
                        </div>
                      </li>
                    );
                  })}
                </ul>
              )}
          </Card>
      </div>
    </div>
  );
};

export default HomePage;
