import React from 'react';
import { Minus, Square, X } from 'lucide-react';
import logo from '../../assets/logo.png';

const TitleBar: React.FC = () => {
  return (
    <div className="h-12 bg-wc-sidebar dark:bg-wc-sidebar-dark flex items-center justify-between select-none drag border-b border-wc-border dark:border-wc-border-dark">
      {/* Brand — aligned with the sidebar width */}
      <div className="flex items-center gap-2 px-4 w-64">
        <img src={logo} alt="WinConveyoR" className="h-6 w-6" />
        <span className="text-lg font-bold text-wc-fg dark:text-wc-fg-dark">
          WinConveyo<span className="text-wc-accent dark:text-wc-accent-bright">R</span>
        </span>
      </div>

      <div className="flex h-full no-drag">
        <button
          onClick={() => window.electronAPI?.minimizeWindow()}
          className="h-full px-4 text-wc-muted dark:text-wc-muted-dark hover:bg-wc-surface dark:hover:bg-wc-surface-dark flex items-center justify-center focus:outline-none"
          aria-label="Minimize"
        >
          <Minus size={16} />
        </button>

        <button
          onClick={() => window.electronAPI?.maximizeWindow()}
          className="h-full px-4 text-wc-muted dark:text-wc-muted-dark hover:bg-wc-surface dark:hover:bg-wc-surface-dark flex items-center justify-center focus:outline-none"
          aria-label="Maximize"
        >
          <Square size={14} />
        </button>

        <button
          onClick={() => window.electronAPI?.closeWindow()}
          className="h-full px-4 text-wc-muted dark:text-wc-muted-dark hover:bg-wc-danger hover:text-white dark:hover:bg-wc-danger dark:hover:text-white flex items-center justify-center focus:outline-none"
          aria-label="Close"
        >
          <X size={16} />
        </button>
      </div>
    </div>
  );
};

export default TitleBar;
