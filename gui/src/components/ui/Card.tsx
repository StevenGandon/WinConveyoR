import React from 'react';

interface CardProps {
  children: React.ReactNode;
  className?: string;
  onClick?: () => void;
  hoverable?: boolean;
}

const Card: React.FC<CardProps> = ({ 
  children, 
  className = '', 
  onClick,
  hoverable = false
}) => {
  const baseClasses = 'bg-wc-card dark:bg-wc-card-dark rounded-lg shadow-sm border border-wc-border dark:border-wc-border-dark overflow-hidden transition-all duration-200';
  const hoverClasses = hoverable ? 'hover:shadow-md hover:border-wc-border-strong dark:hover:border-wc-border-strong-dark cursor-pointer' : '';
  
  return (
    <div 
      className={`${baseClasses} ${hoverClasses} ${className}`}
      onClick={onClick}
      role={onClick ? 'button' : undefined}
      tabIndex={onClick ? 0 : undefined}
    >
      {children}
    </div>
  );
};

export const CardHeader: React.FC<{ className?: string; children: React.ReactNode }> = ({ 
  className = '', 
  children 
}) => {
  return (
    <div className={`p-4 border-b border-wc-border dark:border-wc-border-dark ${className}`}>
      {children}
    </div>
  );
};

export const CardTitle: React.FC<{ className?: string; children: React.ReactNode }> = ({ 
  className = '', 
  children 
}) => {
  return (
    <h3 className={`text-lg font-semibold text-wc-fg dark:text-wc-fg-dark ${className}`}>
      {children}
    </h3>
  );
};

export const CardContent: React.FC<{ className?: string; children: React.ReactNode }> = ({ 
  className = '', 
  children 
}) => {
  return (
    <div className={`p-4 ${className}`}>
      {children}
    </div>
  );
};

export const CardFooter: React.FC<{ className?: string; children: React.ReactNode }> = ({ 
  className = '', 
  children 
}) => {
  return (
    <div className={`p-4 border-t border-wc-border dark:border-wc-border-dark ${className}`}>
      {children}
    </div>
  );
};

export default Card;