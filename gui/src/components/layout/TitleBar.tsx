import React from 'react';
import { Minus, Square, X } from 'lucide-react';

const TitleBar: React.FC = () => {
  return (
    <div className="h-8 bg-wc-accent-soft dark:bg-wc-bg-dark flex items-center justify-between select-none drag">
      <div className="px-4 text-wc-accent-deep dark:text-wc-muted-dark text-sm">WinConveyoR</div>

      <div className="flex h-full no-drag">
        <button
          onClick={() => window.electronAPI?.minimizeWindow()}
          className="h-full px-4 text-wc-accent-deep dark:text-wc-muted-dark hover:bg-wc-surface dark:hover:bg-wc-surface-dark flex items-center justify-center focus:outline-none"
          aria-label="Minimize"
        >
          <Minus size={16} />
        </button>

        <button
          onClick={() => window.electronAPI?.maximizeWindow()}
          className="h-full px-4 text-wc-accent-deep dark:text-wc-muted-dark hover:bg-wc-surface dark:hover:bg-wc-surface-dark flex items-center justify-center focus:outline-none"
          aria-label="Maximize"
        >
          <Square size={14} />
        </button>

        <button
          onClick={() => window.electronAPI?.closeWindow()}
          className="h-full px-4 text-wc-accent-deep dark:text-wc-muted-dark hover:bg-wc-danger dark:hover:bg-wc-danger flex items-center justify-center focus:outline-none"
          aria-label="Close"
        >
          <X size={16} />
        </button>
      </div>
    </div>
  );
};

export default TitleBar;