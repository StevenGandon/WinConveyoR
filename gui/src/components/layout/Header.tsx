import React from 'react';
import { Moon, Sun, Monitor } from 'lucide-react';
import { useSettings } from '../../context/SettingsContext';

interface HeaderProps {
  title: string;
}

const Header: React.FC<HeaderProps> = ({ title }) => {
  const { settings, toggleTheme } = useSettings();

  const getThemeIcon = () => {
    switch (settings.theme) {
      case 'light': return <Sun size={20} />;
      case 'dark': return <Moon size={20} />;
      default: return <Monitor size={20} />;
    }
  };

  return (
    <header className="h-16 bg-white dark:bg-gray-900 border-b border-gray-200 dark:border-gray-800 flex items-center justify-between px-4">
      <h2 className="text-xl font-semibold text-gray-800 dark:text-gray-100">{title}</h2>

      <div className="flex items-center space-x-2">
        <button
          onClick={toggleTheme}
          className="p-2 rounded-full hover:bg-gray-100 dark:hover:bg-gray-800 text-gray-700 dark:text-gray-300 focus:outline-none focus:ring-2 focus:ring-blue-500"
          title={`Theme: ${settings.theme}`}
        >
          {getThemeIcon()}
        </button>
      </div>
    </header>
  );
};

export default Header;
