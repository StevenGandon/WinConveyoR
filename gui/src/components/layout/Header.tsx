import React, { useState } from 'react';
import { Search, Moon, Sun, Monitor, ZoomIn, ZoomOut, BellRing } from 'lucide-react';
import Input from '../ui/Input';
import { useSettings } from '../../context/SettingsContext';
import { usePackages } from '../../context/PackageContext';

interface HeaderProps {
  title: string;
}

const Header: React.FC<HeaderProps> = ({ title }) => {
  const { settings, toggleTheme, updateSettings } = useSettings();
  const { setSearchQuery, searchQuery } = usePackages();
  const [localSearch, setLocalSearch] = useState(searchQuery);

  const handleSearch = (e: React.FormEvent) => {
    e.preventDefault();
    setSearchQuery(localSearch);
  };

  const getThemeIcon = () => {
    switch (settings.theme) {
      case 'light':
        return <Sun size={20} />;
      case 'dark':
        return <Moon size={20} />;
      default:
        return <Monitor size={20} />;
    }
  };

  return (
    <header className="h-16 bg-white dark:bg-gray-900 border-b border-gray-200 dark:border-gray-800 flex items-center justify-between px-4">
      <h2 className="text-xl font-semibold text-gray-800 dark:text-gray-100">{title}</h2>
      
      <div className="flex items-center space-x-4">
        <form onSubmit={handleSearch} className="relative">
          <Input
            type="search"
            placeholder="Search packages..."
            value={localSearch}
            onChange={(e) => setLocalSearch(e.target.value)}
            leftIcon={<Search size={18} />}
            className="w-64"
            aria-label="Search packages"
          />
        </form>
        
        <div className="flex items-center space-x-2">
          <button
            onClick={() => updateSettings({ fontSize: Math.max(12, settings.fontSize - 1) })}
            className="p-2 rounded-full hover:bg-gray-100 dark:hover:bg-gray-800 text-gray-700 dark:text-gray-300 focus:outline-none focus:ring-2 focus:ring-blue-500"
            title="Decrease font size"
            aria-label="Decrease font size"
          >
            <ZoomOut size={20} />
          </button>
          
          <button
            onClick={() => updateSettings({ fontSize: Math.min(20, settings.fontSize + 1) })}
            className="p-2 rounded-full hover:bg-gray-100 dark:hover:bg-gray-800 text-gray-700 dark:text-gray-300 focus:outline-none focus:ring-2 focus:ring-blue-500"
            title="Increase font size"
            aria-label="Increase font size"
          >
            <ZoomIn size={20} />
          </button>
          
          <button
            onClick={toggleTheme}
            className="p-2 rounded-full hover:bg-gray-100 dark:hover:bg-gray-800 text-gray-700 dark:text-gray-300 focus:outline-none focus:ring-2 focus:ring-blue-500"
            title={`Current theme: ${settings.theme}. Click to change.`}
            aria-label={`Current theme: ${settings.theme}. Click to change.`}
          >
            {getThemeIcon()}
          </button>
          
          <button
            className="p-2 rounded-full hover:bg-gray-100 dark:hover:bg-gray-800 text-gray-700 dark:text-gray-300 focus:outline-none focus:ring-2 focus:ring-blue-500 relative"
            title="Notifications"
            aria-label="Notifications"
          >
            <BellRing size={20} />
            <span className="absolute top-0 right-0 block h-2.5 w-2.5 rounded-full bg-red-500 ring-2 ring-white dark:ring-gray-900"></span>
          </button>
        </div>
      </div>
    </header>
  );
};

export default Header;