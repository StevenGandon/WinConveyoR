// Lightweight, client-side activity log backed by localStorage.
// Records real user actions (installs, updates, ...) so the Home page can
// surface a genuine "Recent activity" feed instead of fabricated data.

const STORAGE_KEY = 'wcr_activity';
const MAX_ENTRIES = 50;
const ACTIVITY_EVENT = 'wcr-activity';

export type ActivityKind = 'installed' | 'updated' | 'uninstalled' | 'source_added' | 'sources_synced';

export interface ActivityEntry {
  id: string;
  kind: ActivityKind;
  name: string;
  detail?: string;
  ts: number;
}

export function getActivity(limit = MAX_ENTRIES): ActivityEntry[] {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return [];
    const list: ActivityEntry[] = JSON.parse(raw);
    return list.slice(0, limit);
  } catch {
    return [];
  }
}

export function logActivity(kind: ActivityKind, name: string, detail?: string): void {
  try {
    const entry: ActivityEntry = {
      id: `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`,
      kind,
      name,
      detail,
      ts: Date.now(),
    };
    const next = [entry, ...getActivity()].slice(0, MAX_ENTRIES);
    localStorage.setItem(STORAGE_KEY, JSON.stringify(next));
    window.dispatchEvent(new Event(ACTIVITY_EVENT));
  } catch {
    /* ignore storage errors (private mode, quota, ...) */
  }
}

export function onActivityChange(listener: () => void): () => void {
  window.addEventListener(ACTIVITY_EVENT, listener);
  return () => window.removeEventListener(ACTIVITY_EVENT, listener);
}
