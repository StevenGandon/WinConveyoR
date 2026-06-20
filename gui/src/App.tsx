import { useState, useEffect } from 'react';
import { AuthProvider, useAuth } from './context/AuthContext';
import { SettingsProvider } from './context/SettingsContext';
import { PackageProvider } from './context/PackageContext';
import { CliProvider } from './context/CliContext';
import TitleBar from './components/layout/TitleBar';
import Sidebar from './components/layout/Sidebar';
import HomePage from './pages/HomePage';
import DiscoverPage from './pages/DiscoverPage';
import InstalledPage from './pages/InstalledPage';
import UpdatesPage from './pages/UpdatesPage';
import SettingsPage from './pages/SettingsPage';
import OutputPage from './pages/OutputPage';
import LoginPage from './pages/LoginPage';
import RegisterPage from './pages/RegisterPage';

function AuthGate() {
  const { token, isLoading } = useAuth();
  const [authPage, setAuthPage] = useState<'login' | 'register'>('login');

  useEffect(() => {
    if (!token) setAuthPage('login');
  }, [token]);

  if (token === 'anonymous') {
    return <MainApp />;
  }

  if (isLoading) {
    return (
      <div className="flex flex-col h-screen bg-gray-50 dark:bg-gray-950">
        <TitleBar />
        <div className="flex items-center justify-center flex-1">
          <p className="text-gray-500 dark:text-gray-400">Loading...</p>
        </div>
      </div>
    );
  }

  if (!token) {
    const page = authPage === 'login'
      ? <LoginPage onSwitchToRegister={() => setAuthPage('register')} />
      : <RegisterPage onSwitchToLogin={() => setAuthPage('login')} />;
    return (
      <div className="flex flex-col h-screen bg-gray-50 dark:bg-gray-950">
        <TitleBar />
        <div className="flex-1 flex items-center justify-center overflow-hidden">{page}</div>
      </div>
    );
  }

  return <MainApp />;
}

function MainApp() {
  const [activePage, setActivePage] = useState('home');

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.altKey && !isNaN(Number(e.key))) {
        e.preventDefault();
        const pages = ['home', 'discover', 'installed', 'updates', 'output', 'settings'];
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
      case 'output':
        return <OutputPage />;
      case 'settings':
        return <SettingsPage />;
      default:
        return <HomePage />;
    }
  };

  return (
    <PackageProvider>
      <CliProvider>
      <div className="flex flex-col h-screen bg-gray-50 dark:bg-gray-950 text-gray-900 dark:text-gray-100">
        <TitleBar />
        <div className="flex flex-1 overflow-hidden">
          <Sidebar activePage={activePage} onNavigate={setActivePage} />
          <main className="flex-1 flex flex-col overflow-auto">
            {renderPage()}
          </main>
        </div>
      </div>
      </CliProvider>
    </PackageProvider>
  );
}

function App() {
  return (
    <SettingsProvider>
      <AuthProvider>
        <AuthGate />
      </AuthProvider>
    </SettingsProvider>
  );
}

export default App;