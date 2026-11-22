import React, { InputHTMLAttributes } from 'react';

interface InputProps extends InputHTMLAttributes<HTMLInputElement> {
  label?: string;
  error?: string;
  leftIcon?: React.ReactNode;
  rightIcon?: React.ReactNode;
  fullWidth?: boolean;
}

const Input: React.FC<InputProps> = ({
  label,
  error,
  leftIcon,
  rightIcon,
  fullWidth = false,
  className = '',
  id,
  ...props
}) => {
  const inputId = id || `input-${Math.random().toString(36).substring(2, 9)}`;
  
  const baseInputClasses = 'block bg-wc-card dark:bg-wc-card-dark text-wc-fg dark:text-wc-fg-dark placeholder:text-wc-muted dark:placeholder:text-wc-muted-dark rounded-md border border-wc-border dark:border-wc-border-dark focus:outline-none focus:border-wc-accent transition-colors';
  const errorInputClasses = error ? 'border-wc-danger focus:border-wc-danger' : '';
  const paddingClasses = leftIcon ? 'pl-10' : 'pl-4';
  const rightPaddingClasses = rightIcon ? 'pr-10' : 'pr-4';
  const widthClass = fullWidth ? 'w-full' : '';
  
  const inputClasses = `${baseInputClasses} ${errorInputClasses} ${paddingClasses} ${rightPaddingClasses} py-2 ${widthClass} ${className}`;
  
  return (
    <div className={fullWidth ? 'w-full' : ''}>
      {label && (
        <label 
          htmlFor={inputId} 
          className="block text-sm font-medium text-wc-fg dark:text-wc-fg-dark mb-1"
        >
          {label}
        </label>
      )}
      <div className="relative">
        {leftIcon && (
          <div className="absolute inset-y-0 left-0 pl-3 flex items-center pointer-events-none text-wc-muted dark:text-wc-muted-dark">
            {leftIcon}
          </div>
        )}
        <input
          id={inputId}
          className={inputClasses}
          aria-invalid={error ? 'true' : 'false'}
          aria-describedby={error ? `${inputId}-error` : undefined}
          {...props}
        />
        {rightIcon && (
          <div className="absolute inset-y-0 right-0 pr-3 flex items-center pointer-events-none text-wc-muted dark:text-wc-muted-dark">
            {rightIcon}
          </div>
        )}
      </div>
      {error && (
        <p id={`${inputId}-error`} className="mt-1 text-sm text-wc-danger dark:text-wc-danger-dark" role="alert">
          {error}
        </p>
      )}
    </div>
  );
};

export default Input;