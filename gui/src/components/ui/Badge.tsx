import React from 'react';

interface BadgeProps {
  variant?: 'default' | 'success' | 'warning' | 'error' | 'info';
  size?: 'sm' | 'md' | 'lg';
  children: React.ReactNode;
  className?: string;
}

const Badge: React.FC<BadgeProps> = ({ 
  variant = 'default', 
  size = 'md', 
  children, 
  className = '' 
}) => {
  const baseClasses = 'inline-flex items-center justify-center font-medium rounded-full transition-colors';
  
  const variantClasses = {
    default: 'bg-wc-surface text-wc-fg dark:bg-wc-surface-dark dark:text-wc-fg-dark',
    success: 'bg-wc-success-soft text-wc-success dark:bg-wc-success-soft-dark dark:text-wc-success-dark',
    warning: 'bg-wc-accent-soft text-wc-accent-deep dark:bg-wc-accent-soft-dark dark:text-wc-accent-bright',
    error: 'bg-wc-danger-soft text-wc-danger dark:bg-wc-danger-soft-dark dark:text-wc-danger-dark',
    info: 'bg-wc-accent-soft text-wc-accent dark:bg-wc-accent-soft-dark dark:text-wc-accent-bright'
  };
  
  const sizeClasses = {
    sm: 'text-xs px-2 py-0.5',
    md: 'text-sm px-2.5 py-0.5',
    lg: 'text-base px-3 py-1'
  };

  const classes = `${baseClasses} ${variantClasses[variant]} ${sizeClasses[size]} ${className}`;
  
  return (
    <span className={classes} role="status">
      {children}
    </span>
  );
};

export default Badge;