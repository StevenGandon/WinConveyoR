import React, { createContext, useContext, useState, useEffect, useCallback } from 'react';

interface InstallResult {
  code: number;
  version?: string;
}

interface CliContextType {
  log: string;
  busy: boolean;
  runUpdate: () => Promise<void>;
  runInstall: (packageName: string) => Promise<InstallResult>;
  runUninstall: (packageName: string) => Promise<number>;
  runSearch: (query?: string) => Promise<{ name: string; version: string }[]>;
  runInfo: (packageSpec: string) => Promise<string>;
  runCheck: (packageSpec: string) => Promise<{ match: number; expected: string; actual: string } | null>;
  runUpgrade: (packageName?: string) => Promise<number>;
}

const CliContext = createContext<CliContextType | undefined>(undefined);

export const CliProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [log, setLog] = useState('');
  const [busy, setBusy] = useState(false);

  useEffect(() => {
    const cleanup = window.electronAPI?.onCliOutput((text) => {
      setLog(prev => prev + text);
    });
    return () => cleanup?.();
  }, []);

  const runUpdate = useCallback(async () => {
    if (!window.electronAPI || busy) return;
    setBusy(true);
    setLog('');
    const result = await window.electronAPI.runUpdate();
    if (result.code !== 0) setLog(prev => prev + `\nExited with code ${result.code}`);
    setBusy(false);
  }, [busy]);

  const runInstall = useCallback(async (packageName: string): Promise<InstallResult> => {
    if (!window.electronAPI || busy) return { code: -1 };
    setBusy(true);
    setLog('');
    const result = await window.electronAPI.runInstall(packageName);
    if (result.code !== 0) setLog(prev => prev + `\nExited with code ${result.code}`);
    setBusy(false);
    const versionMatch = result.output.match(/version=(\S+)/);
    return { code: result.code, version: versionMatch?.[1] };
  }, [busy]);

  const runUninstall = useCallback(async (packageName: string) => {
    if (!window.electronAPI || busy) return -1;
    setBusy(true);
    setLog('');
    const result = await window.electronAPI.runUninstall(packageName);
    if (result.code !== 0) setLog(prev => prev + `\nExited with code ${result.code}`);
    setBusy(false);
    return result.code;
  }, [busy]);

  const runSearch = useCallback(async (query?: string): Promise<{ name: string; version: string }[]> => {
    if (!window.electronAPI || busy) return [];
    setBusy(true);
    setLog('');
    const result = await window.electronAPI.runSearch(query);
    setBusy(false);
    if (result.code !== 0) return [];
    try {
      const match = result.output.match(/\[.*\]/s);
      if (match) return JSON.parse(match[0]);
    } catch { /* ignore */ }
    return [];
  }, [busy]);

  const runInfo = useCallback(async (packageSpec: string): Promise<string> => {
    if (!window.electronAPI || busy) return '';
    setBusy(true);
    setLog('');
    const result = await window.electronAPI.runInfo(packageSpec);
    setBusy(false);
    return result.output;
  }, [busy]);

  const runCheck = useCallback(async (packageSpec: string): Promise<{ match: number; expected: string; actual: string } | null> => {
    if (!window.electronAPI || busy) return null;
    setBusy(true);
    setLog('');
    const result = await window.electronAPI.runCheck(packageSpec);
    setBusy(false);
    const expectedMatch = result.output.match(/Expected:\s*(\S+)/);
    const actualMatch = result.output.match(/Actual:\s*(\S+)/);
    const statusMatch = result.output.match(/Status:\s*(\S+)/);
    if (expectedMatch) {
      return {
        match: statusMatch?.[1] === 'OK' ? 1 : 0,
        expected: expectedMatch[1],
        actual: actualMatch?.[1] ?? '',
      };
    }
    return null;
  }, [busy]);

  const runUpgrade = useCallback(async (packageName?: string): Promise<number> => {
    if (!window.electronAPI || busy) return -1;
    setBusy(true);
    setLog('');
    const result = await window.electronAPI.runUpgrade(packageName);
    if (result.code !== 0) setLog(prev => prev + `\nExited with code ${result.code}`);
    setBusy(false);
    return result.code;
  }, [busy]);

  return (
    <CliContext.Provider value={{ log, busy, runUpdate, runInstall, runUninstall, runSearch, runInfo, runCheck, runUpgrade }}>
      {children}
    </CliContext.Provider>
  );
};

export const useCli = (): CliContextType => {
  const context = useContext(CliContext);
  if (context === undefined) {
    throw new Error('useCli must be used within a CliProvider');
  }
  return context;
};
