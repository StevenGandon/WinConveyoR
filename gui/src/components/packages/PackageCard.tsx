import React from 'react';
import { Download, RefreshCw, Trash2 } from 'lucide-react';
import Card from '../ui/Card';
import Badge from '../ui/Badge';
import Button from '../ui/Button';
import { Package } from '../../types';
import { usePackages } from '../../context/PackageContext';

interface PackageCardProps {
  pkg: Package;
}

const PackageCard: React.FC<PackageCardProps> = ({ pkg }) => {
  const { installPackage, uninstallPackage, updatePackage, selectPackage } = usePackages();

  return (
    <Card 
      hoverable 
      className="h-full flex flex-col transition-transform hover:translate-y-[-2px]"
      onClick={() => selectPackage(pkg)}
    >
      <div className="p-4 flex items-start space-x-3">
        <div className="flex-shrink-0 w-10 h-10 bg-gray-100 dark:bg-gray-800 rounded-md flex items-center justify-center overflow-hidden">
          <img 
            src={pkg.iconUrl} 
            alt={`${pkg.name} logo`} 
            className="w-6 h-6 object-contain"
            onError={(e) => {
              (e.target as HTMLImageElement).src = 'https://cdn.jsdelivr.net/gh/simple-icons/simple-icons/icons/windows.svg';
            }}
          />
        </div>

        <div className="flex-1 min-w-0">
          <div className="flex items-center justify-between">
            <h3 className="text-base font-medium text-gray-900 dark:text-gray-100 truncate">{pkg.name}</h3>
            {pkg.isInstalled && (
              <Badge variant={pkg.isUpdatable ? 'warning' : 'success'} size="sm">
                {pkg.isUpdatable ? 'Update' : 'Installed'}
              </Badge>
            )}
          </div>
          
          <p className="text-xs text-gray-500 dark:text-gray-400 mt-0.5">
            v{pkg.version} • {pkg.publisher}
          </p>
          
          <p className="text-sm text-gray-600 dark:text-gray-300 mt-2 line-clamp-2">
            {pkg.description}
          </p>
        </div>
      </div>

      <div className="mt-auto p-3 pt-0 flex justify-between items-center">
        <div className="text-xs text-gray-500 dark:text-gray-400">
          {pkg.size} • {pkg.downloadCount.toLocaleString()} downloads
        </div>

        <div className="flex space-x-2">
          {!pkg.isInstalled ? (
            <Button
              variant="primary"
              size="sm"
              leftIcon={<Download size={14} />}
              onClick={(e) => {
                e.stopPropagation();
                installPackage(pkg.id);
              }}
              aria-label={`Install ${pkg.name}`}
            >
              Install
            </Button>
          ) : pkg.isUpdatable ? (
            <Button
              variant="secondary"
              size="sm"
              leftIcon={<RefreshCw size={14} />}
              onClick={(e) => {
                e.stopPropagation();
                updatePackage(pkg.id);
              }}
              aria-label={`Update ${pkg.name}`}
            >
              Update
            </Button>
          ) : (
            <Button
              variant="ghost"
              size="sm"
              leftIcon={<Trash2 size={14} />}
              onClick={(e) => {
                e.stopPropagation();
                uninstallPackage(pkg.id);
              }}
              aria-label={`Uninstall ${pkg.name}`}
            >
              Uninstall
            </Button>
          )}
        </div>
      </div>
    </Card>
  );
};

export default PackageCard;