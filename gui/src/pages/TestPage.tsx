declare global {
  interface Window {
    ffiAPI: {
      ffiAdd: (a: number, b: number) => Promise<{success: boolean, result?: number, error?: string}>;
      ffiSub: (a: number, b: number) => Promise<{success: boolean, result?: number, error?: string}>;
      getPlatformInfo: () => Promise<{platform: string, arch: string}>;
    }
  }
}

import React, { useState, useEffect } from "react";

const TestPage: React.FC = () => {
  const [result, setResult] = useState<number | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [platformInfo, setPlatformInfo] = useState<{platform: string, arch: string} | null>(null);
  const [loading, setLoading] = useState<boolean>(false);
  
  const [addValue1, setAddValue1] = useState<string>("0");
  const [addValue2, setAddValue2] = useState<string>("0");
  const [subValue1, setSubValue1] = useState<string>("0");
  const [subValue2, setSubValue2] = useState<string>("0");

  useEffect(() => {
    const fetchPlatformInfo = async () => {
      try {
        const info = await window.ffiAPI.getPlatformInfo();
        setPlatformInfo(info);
      } catch (err) {
        console.error('Erreur lors de la récupération des infos de plateforme:', err);
      }
    };

    fetchPlatformInfo();
  }, []);

  const testAddition = async () => {
    const a = parseInt(addValue1);
    const b = parseInt(addValue2);
    
    if (isNaN(a) || isNaN(b)) {
      setError("Veuillez entrer des nombres valides pour l'addition");
      return;
    }

    try {
      setLoading(true);
      setError(null);
      
      const response = await window.ffiAPI.ffiAdd(a, b);
      
      if (response.success) {
        setResult(response.result!);
        console.log(`Résultat de add(${a}, ${b}):`, response.result);
      } else {
        setError(response.error || 'Erreur inconnue');
      }

    } catch (err) {
      console.error('Erreur lors du chargement de la bibliothèque:', err);
      setError(err instanceof Error ? err.message : 'Erreur inconnue');
    } finally {
      setLoading(false);
    }
  };

  const testSubtraction = async () => {
    const a = parseInt(subValue1);
    const b = parseInt(subValue2);
    
    if (isNaN(a) || isNaN(b)) {
      setError("Veuillez entrer des nombres valides pour la soustraction");
      return;
    }

    try {
      setLoading(true);
      setError(null);
      
      const response = await window.ffiAPI.ffiSub(a, b);
      
      if (response.success) {
        setResult(response.result!);
        console.log(`Résultat de sub(${a}, ${b}):`, response.result);
      } else {
        setError(response.error || 'Erreur inconnue');
      }

    } catch (err) {
      console.error('Erreur lors du chargement de la bibliothèque:', err);
      setError(err instanceof Error ? err.message : 'Erreur inconnue');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="p-6 max-w-4xl">
      <div className="space-y-6">
        <div>
          <h1 className="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-4">
            Test Lib
          </h1>
        </div>
        <div className="bg-white dark:bg-gray-800 rounded-lg shadow-sm border border-gray-200 dark:border-gray-700 p-6">
          <h2 className="text-lg font-semibold text-gray-900 dark:text-gray-100 mb-4">
            Test Add
          </h2>
          <div className="space-y-4">
            <div className="grid grid-cols-1 md:grid-cols-3 gap-4 items-end">
              <div>
                <input
                  id="add-value1"
                  type="number"
                  value={addValue1}
                  onChange={(e) => setAddValue1(e.target.value)}
                  className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md shadow-sm focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-blue-500 dark:bg-gray-700 dark:text-white"
                />
              </div>
              <div>
                <input
                  id="add-value2"
                  type="number"
                  value={addValue2}
                  onChange={(e) => setAddValue2(e.target.value)}
                  className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md shadow-sm focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-blue-500 dark:bg-gray-700 dark:text-white"
                />
              </div>
              <div>
                <button
                  onClick={testAddition}
                  disabled={loading}
                  className="w-full px-4 py-2 bg-blue-600 text-white rounded-md hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-blue-500 disabled:opacity-50 disabled:cursor-not-allowed"
                >
                  Add
                </button>
              </div>
            </div>
          </div>
        </div>
        <div className="bg-white dark:bg-gray-800 rounded-lg shadow-sm border border-gray-200 dark:border-gray-700 p-6">
          <h2 className="text-lg font-semibold text-gray-900 dark:text-gray-100 mb-4">
            Test Sub
          </h2>
          <div className="space-y-4">
            <div className="grid grid-cols-1 md:grid-cols-3 gap-4 items-end">
              <div>
                <input
                  id="sub-value1"
                  type="number"
                  value={subValue1}
                  onChange={(e) => setSubValue1(e.target.value)}
                  className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md shadow-sm focus:outline-none focus:ring-2 focus:ring-green-500 focus:border-green-500 dark:bg-gray-700 dark:text-white"
                />
              </div>
              <div>
                <input
                  id="sub-value2"
                  type="number"
                  value={subValue2}
                  onChange={(e) => setSubValue2(e.target.value)}
                  className="w-full px-3 py-2 border border-gray-300 dark:border-gray-600 rounded-md shadow-sm focus:outline-none focus:ring-2 focus:ring-green-500 focus:border-green-500 dark:bg-gray-700 dark:text-white"
                />
              </div>
              <div>
                <button
                  onClick={testSubtraction}
                  disabled={loading}
                  className="w-full px-4 py-2 bg-green-600 text-white rounded-md hover:bg-green-700 focus:outline-none focus:ring-2 focus:ring-green-500 disabled:opacity-50 disabled:cursor-not-allowed"
                >
                  Sub
                </button>
              </div>
            </div>
          </div>
        </div>
        {(result !== null || error) && (
          <div className="bg-white dark:bg-gray-800 rounded-lg shadow-sm border border-gray-200 dark:border-gray-700 p-6">
            <div className="flex justify-between items-center mb-4">
              <h2 className="text-lg font-semibold text-gray-900 dark:text-gray-100">
                Result
              </h2>
            </div>
            {result !== null && (
              <div className="p-4 bg-green-50 dark:bg-green-900/20 border border-green-200 dark:border-green-800 rounded-md">
                <h3 className="text-sm font-medium text-green-800 dark:text-green-200 mb-1">
                  Result
                </h3>
                <p className="text-green-700 dark:text-green-300 font-mono text-2xl font-bold">
                  {result}
                </p>
              </div>
            )}
            {error && (
              <div className="p-4 bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-800 rounded-md">
                <h3 className="text-sm font-medium text-red-800 dark:text-red-200 mb-1">
                  Error
                </h3>
                <p className="text-red-700 dark:text-red-300 font-mono text-sm">
                  {error}
                </p>
              </div>
            )}
          </div>
        )}
        <div className="bg-white dark:bg-gray-800 rounded-lg shadow-sm border border-gray-200 dark:border-gray-700 p-6">
          <h2 className="text-lg font-semibold text-gray-900 dark:text-gray-100 mb-4">
            Infos
          </h2>
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4 text-sm">
            <div>
              <span className="font-medium text-gray-700 dark:text-gray-300">Plateforme:</span>
              <span className="ml-2 text-gray-600 dark:text-gray-400">
                {platformInfo?.platform || 'Chargement...'}
              </span>
            </div>
            <div>
              <span className="font-medium text-gray-700 dark:text-gray-300">Architecture:</span>
              <span className="ml-2 text-gray-600 dark:text-gray-400">
                {platformInfo?.arch || 'Chargement...'}
              </span>
            </div>
            <div>
              <span className="font-medium text-gray-700 dark:text-gray-300">Fichier bibliothèque:</span>
              <span className="ml-2 text-gray-600 dark:text-gray-400 font-mono">
                {platformInfo?.platform === 'win32' ? 'testlib.dll' : 'testlib.so'}
              </span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default TestPage;