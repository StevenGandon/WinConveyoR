import React from 'react';
import { Minus, Square, X } from 'lucide-react';

declare global {
  interface Window {
    electronAPI: {
      minimizeWindow: () => void;
      maximizeWindow: () => void;
      closeWindow: () => void;
    }
  }
}

const TitleBar: React.FC = () => {
  return (
    <div className="h-8 bg-gray-900 flex items-center justify-between select-none drag">
      <div className="px-4 text-gray-400 text-sm">WinConveyoR</div>
      
      <div className="flex h-full no-drag">
        <button
          onClick={() => window.electronAPI.minimizeWindow()}
          className="h-full px-4 text-gray-400 hover:bg-gray-800 flex items-center justify-center focus:outline-none"
          aria-label="Minimize"
        >
          <Minus size={16} />
        </button>
        
        <button
          onClick={() => window.electronAPI.maximizeWindow()}
          className="h-full px-4 text-gray-400 hover:bg-gray-800 flex items-center justify-center focus:outline-none"
          aria-label="Maximize"
        >
          <Square size={14} />
        </button>
        
        <button
          onClick={() => window.electronAPI.closeWindow()}
          className="h-full px-4 text-gray-400 hover:bg-red-600 flex items-center justify-center focus:outline-none"
          aria-label="Close"
        >
          <X size={16} />
        </button>
      </div>
    </div>
  );
};

export default TitleBar;