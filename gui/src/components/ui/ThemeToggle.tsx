import React from 'react';
import { Sun, Moon, Monitor } from 'lucide-react';
import { useSettings } from '../../context/SettingsContext';

const icons = { light: Sun, dark: Moon, system: Monitor };

const ThemeToggle: React.FC<{ className?: string }> = ({ className = '' }) => {
  const { settings, toggleTheme } = useSettings();
  const Icon = icons[settings.theme];

  return (
    <button
      onClick={toggleTheme}
      title={`Theme: ${settings.theme}`}
      aria-label="Toggle theme"
      className={`p-2 rounded-full text-wc-muted dark:text-wc-muted-dark hover:bg-wc-surface dark:hover:bg-wc-surface-dark hover:text-wc-fg dark:hover:text-wc-fg-dark transition-colors focus:outline-none ${className}`}
    >
      <Icon size={20} />
    </button>
  );
};

export default ThemeToggle;
