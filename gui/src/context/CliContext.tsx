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

  return (
    <CliContext.Provider value={{ log, busy, runUpdate, runInstall, runUninstall }}>
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
