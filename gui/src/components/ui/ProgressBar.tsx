import React from 'react';

interface ProgressBarProps {
  value: number;
  max: number;
  label?: string;
  showValue?: boolean;
  size?: 'sm' | 'md' | 'lg';
  variant?: 'default' | 'success' | 'warning' | 'error';
  className?: string;
}

const ProgressBar: React.FC<ProgressBarProps> = ({
  value,
  max,
  label,
  showValue = false,
  size = 'md',
  variant = 'default',
  className = '',
}) => {
  const percentage = Math.min(Math.max(0, (value / max) * 100), 100);
  
  const baseClasses = 'w-full bg-wc-surface dark:bg-wc-surface-dark rounded-full overflow-hidden';
  
  const sizeClasses = {
    sm: 'h-1',
    md: 'h-2',
    lg: 'h-3',
  };
  
  const variantClasses = {
    default: 'bg-wc-accent',
    success: 'bg-wc-success dark:bg-wc-success-dark',
    warning: 'bg-wc-accent dark:bg-wc-accent-bright',
    error: 'bg-wc-danger dark:bg-wc-danger-dark',
  };
  
  return (
    <div className={className}>
      {(label || showValue) && (
        <div className="flex justify-between items-center mb-1">
          {label && (
            <span className="text-sm font-medium text-wc-fg dark:text-wc-fg-dark">
              {label}
            </span>
          )}
          {showValue && (
            <span className="text-sm font-medium text-wc-fg dark:text-wc-fg-dark">
              {value}/{max} ({percentage.toFixed(0)}%)
            </span>
          )}
        </div>
      )}
      <div className={`${baseClasses} ${sizeClasses[size]}`} role="progressbar" aria-valuenow={value} aria-valuemin={0} aria-valuemax={max}>
        <div 
          className={`${variantClasses[variant]} h-full transition-all duration-300 ease-in-out`}
          style={{ width: `${percentage}%` }}
        />
      </div>
    </div>
  );
};

export default ProgressBar;