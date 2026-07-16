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
    <header className="p-4 bg-wc-card dark:bg-wc-card-dark border-b border-wc-border dark:border-wc-border-dark flex items-center justify-between">
      <h2 className="text-xl font-semibold text-wc-fg dark:text-wc-fg-dark leading-6">{title}</h2>

      <button
        onClick={toggleTheme}
        className="p-1 rounded-full hover:bg-wc-surface dark:hover:bg-wc-surface-dark text-wc-muted dark:text-wc-muted-dark focus:outline-none"
        title={`Theme: ${settings.theme}`}
      >
        {getThemeIcon()}
      </button>
    </header>
  );
};

export default Header;
