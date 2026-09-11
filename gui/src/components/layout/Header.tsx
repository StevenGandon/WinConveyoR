import React from 'react';

interface HeaderProps {
  title: string;
}

const Header: React.FC<HeaderProps> = ({ title }) => {
  return (
    <header className="p-4 bg-wc-card dark:bg-wc-card-dark border-b border-wc-border dark:border-wc-border-dark flex items-center justify-between">
      <h2 className="text-xl font-semibold text-wc-fg dark:text-wc-fg-dark leading-6">{title}</h2>
    </header>
  );
};

export default Header;
