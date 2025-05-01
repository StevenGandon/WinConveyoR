import React, { useState, useEffect } from 'react';
import { SettingsProvider } from './context/SettingsContext';
import { PackageProvider } from './context/PackageContext';
import TitleBar from './components/layout/TitleBar';
import Sidebar from './components/layout/Sidebar';
import HomePage from './pages/HomePage';
import DiscoverPage from './pages/DiscoverPage';
import InstalledPage from './pages/InstalledPage';
import UpdatesPage from './pages/UpdatesPage';
import SettingsPage from './pages/SettingsPage';

function App() {
  const [activePage, setActivePage] = useState('home');
  
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.altKey && !isNaN(Number(e.key))) {
        e.preventDefault();
        const pages = ['home', 'discover', 'installed', 'updates', 'settings'];
        const index = Number(e.key) - 1;
        if (index >= 0 && index < pages.length) {
          setActivePage(pages[index]);
        }
      }
    };
    
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, []);

  const renderPage = () => {
    switch (activePage) {
      case 'home':
        return <HomePage />;
      case 'discover':
        return <DiscoverPage />;
      case 'installed':
        return <InstalledPage />;
      case 'updates':
        return <UpdatesPage />;
      case 'settings':
        return <SettingsPage />;
      default:
        return <HomePage />;
    }
  };

  return (
    <SettingsProvider>
      <PackageProvider>
        <div className="flex flex-col h-screen bg-gray-50 dark:bg-gray-950 text-gray-900 dark:text-gray-100">
          <TitleBar />
          <div className="flex flex-1 overflow-hidden">
            <Sidebar activePage={activePage} onNavigate={setActivePage} />
            <main className="flex-1 flex flex-col overflow-hidden">
              {renderPage()}
            </main>
          </div>
        </div>
      </PackageProvider>
    </SettingsProvider>
  );
}

export default App;