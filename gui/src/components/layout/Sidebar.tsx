import React from 'react';
import { usePackages } from '../../context/PackageContext';
import { useAuth } from '../../context/AuthContext';
import { useSettings } from '../../context/SettingsContext';
import { Package as PackageIcon, Download, RefreshCw, Settings, Home, Search, LogOut, User, Terminal } from 'lucide-react';

interface SidebarItemProps {
  icon: React.ReactNode;
  label: string;
  isActive: boolean;
  count?: number;
  onClick: () => void;
}

const SidebarItem: React.FC<SidebarItemProps> = ({ icon, label, isActive, count, onClick }) => {
  return (
    <button
      className={`w-full flex items-center px-3 py-2 rounded-md text-left text-sm font-medium transition-colors focus:outline-none ${
        isActive
          ? 'bg-wc-accent-soft text-wc-accent-deep dark:bg-wc-accent-soft-dark dark:text-wc-accent-bright'
          : 'text-wc-fg hover:bg-wc-surface dark:text-wc-fg-dark dark:hover:bg-wc-surface-dark'
      }`}
      onClick={onClick}
      aria-current={isActive ? 'page' : undefined}
    >
      <span className="flex items-center justify-center w-5 h-5 mr-3">{icon}</span>
      <span className="flex-1">{label}</span>
      {count !== undefined && count > 0 && (
        <span className={`ml-auto text-xs font-semibold px-2 py-0.5 rounded-full transition-shadow duration-200 ${
          isActive
            ? 'bg-wc-accent-deep text-white dark:bg-wc-accent-bright dark:text-wc-fg'
            : 'bg-wc-accent-soft text-wc-accent-deep dark:bg-wc-accent-soft-dark dark:text-wc-accent-bright'
        }`}>
          {count}
        </span>
      )}
    </button>
  );
};

interface SidebarProps {
  activePage: string;
  onNavigate: (page: string) => void;
}

const Sidebar: React.FC<SidebarProps> = ({ activePage, onNavigate }) => {
  const { installedPackages, updatablePackages } = usePackages();
  const { user, logout } = useAuth();
  const { settings } = useSettings();

  return (
    <div className="w-64 h-full bg-wc-sidebar dark:bg-wc-sidebar-dark border-r border-wc-border dark:border-wc-border-dark flex flex-col">
      <div className="p-4 flex items-center border-b border-wc-border dark:border-wc-border-dark">
        <PackageIcon className="h-6 w-6 text-wc-accent dark:text-wc-accent-bright mr-2" />
        <h1 className="text-xl font-bold text-wc-fg dark:text-wc-fg-dark">WinConveyo<span className="text-wc-accent dark:text-wc-accent-bright">R</span></h1>
      </div>

      <div className="flex-1 overflow-y-auto p-3 space-y-1">
        <SidebarItem
          icon={<Home size={18} />}
          label="Home"
          isActive={activePage === 'home'}
          onClick={() => onNavigate('home')}
        />
        <SidebarItem
          icon={<Search size={18} />}
          label="Discover"
          isActive={activePage === 'discover'}
          onClick={() => onNavigate('discover')}
        />
        <SidebarItem
          icon={<Download size={18} />}
          label="Installed"
          isActive={activePage === 'installed'}
          count={installedPackages.length}
          onClick={() => onNavigate('installed')}
        />
        <SidebarItem
          icon={<RefreshCw size={18} />}
          label="Upgrades"
          isActive={activePage === 'upgrades'}
          count={updatablePackages.length}
          onClick={() => onNavigate('upgrades')}
        />
        {settings.showOutputPage && (
          <SidebarItem
            icon={<Terminal size={18} />}
            label="Output"
            isActive={activePage === 'output'}
            onClick={() => onNavigate('output')}
          />
        )}
      </div>

      <div className="p-3 border-t border-wc-border dark:border-wc-border-dark space-y-1">
        <SidebarItem
          icon={<Settings size={18} />}
          label="Settings"
          isActive={activePage === 'settings'}
          onClick={() => onNavigate('settings')}
        />
        {user && (
          <div className="flex items-center justify-between px-3 py-2 mt-2 rounded-md bg-wc-bg dark:bg-wc-card-dark">
            <div className="flex items-center min-w-0">
              <User size={16} className="text-wc-muted dark:text-wc-muted-dark mr-2 flex-shrink-0" />
              <span className="text-sm text-wc-fg dark:text-wc-fg-dark truncate">{user.username}</span>
            </div>
            <button
              onClick={logout}
              className="ml-2 p-1 rounded hover:bg-wc-surface dark:hover:bg-wc-surface-dark text-wc-muted dark:text-wc-muted-dark flex-shrink-0"
              title="Sign out"
            >
              <LogOut size={16} />
            </button>
          </div>
        )}
      </div>
    </div>
  );
};

export default Sidebar;
