import React from 'react';
import { usePackages } from '../../context/PackageContext';
import { useAuth } from '../../context/AuthContext';
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
      className={`w-full flex items-center px-3 py-2 rounded-md text-left text-sm font-medium transition-colors focus:outline-none focus:ring-2 focus:ring-blue-500 ${
        isActive
          ? 'bg-blue-100 text-blue-900 dark:bg-blue-900 dark:text-blue-100'
          : 'text-gray-600 hover:bg-gray-100 dark:text-gray-300 dark:hover:bg-gray-800'
      }`}
      onClick={onClick}
      aria-current={isActive ? 'page' : undefined}
    >
      <span className="flex items-center justify-center w-5 h-5 mr-3">{icon}</span>
      <span className="flex-1">{label}</span>
      {count !== undefined && count > 0 && (
        <span className="ml-auto bg-gray-200 dark:bg-gray-700 text-xs font-semibold px-2 py-0.5 rounded-full">
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

  return (
    <div className="w-64 h-full bg-white dark:bg-gray-900 border-r border-gray-200 dark:border-gray-800 flex flex-col">
      <div className="p-4 flex items-center border-b border-gray-200 dark:border-gray-800">
        <PackageIcon className="h-6 w-6 text-blue-600 dark:text-blue-400 mr-2" />
        <h1 className="text-xl font-bold text-gray-900 dark:text-gray-100">WinConveyoR</h1>
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
          label="Updates"
          isActive={activePage === 'updates'}
          count={updatablePackages.length}
          onClick={() => onNavigate('updates')}
        />
        <SidebarItem
          icon={<Terminal size={18} />}
          label="Output"
          isActive={activePage === 'output'}
          onClick={() => onNavigate('output')}
        />
      </div>

      <div className="p-3 border-t border-gray-200 dark:border-gray-800 space-y-1">
        <SidebarItem
          icon={<Settings size={18} />}
          label="Settings"
          isActive={activePage === 'settings'}
          onClick={() => onNavigate('settings')}
        />
        {user && (
          <div className="flex items-center justify-between px-3 py-2 mt-2 rounded-md bg-gray-50 dark:bg-gray-800">
            <div className="flex items-center min-w-0">
              <User size={16} className="text-gray-500 dark:text-gray-400 mr-2 flex-shrink-0" />
              <span className="text-sm text-gray-700 dark:text-gray-300 truncate">{user.username}</span>
            </div>
            <button
              onClick={logout}
              className="ml-2 p-1 rounded hover:bg-gray-200 dark:hover:bg-gray-700 text-gray-500 dark:text-gray-400 flex-shrink-0"
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
