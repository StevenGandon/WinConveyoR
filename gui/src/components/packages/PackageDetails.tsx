import React from 'react';
import { X, Download, RefreshCw, Trash2, Star, Calendar, Clock, HardDrive, ExternalLink } from 'lucide-react';
import { Package } from '../../types';
import Button from '../ui/Button';
import Badge from '../ui/Badge';
import { usePackages } from '../../context/PackageContext';

interface PackageDetailsProps {
  pkg: Package;
  onClose: () => void;
}

const PackageDetails: React.FC<PackageDetailsProps> = ({ pkg, onClose }) => {
  const { installPackage, uninstallPackage, updatePackage } = usePackages();

  const formatDate = (dateString: string) => {
    const date = new Date(dateString);
    return new Intl.DateTimeFormat('en-US', {
      year: 'numeric',
      month: 'long',
      day: 'numeric',
    }).format(date);
  };

  const renderRatingStars = (rating: number) => {
    const fullStars = Math.floor(rating);
    const hasHalfStar = rating % 1 >= 0.5;
    
    return (
      <div className="flex">
        {[...Array(5)].map((_, i) => (
          <Star
            key={i}
            size={16}
            className={`${
              i < fullStars
                ? 'text-yellow-400 fill-yellow-400'
                : i === fullStars && hasHalfStar
                ? 'text-yellow-400 fill-yellow-400 half-star'
                : 'text-gray-300 dark:text-gray-600'
            }`}
          />
        ))}
        <span className="ml-1 text-sm text-gray-600 dark:text-gray-300">{rating.toFixed(1)}</span>
      </div>
    );
  };

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center p-4 z-50">
      <div className="bg-white dark:bg-gray-900 rounded-lg shadow-xl max-w-2xl w-full max-h-[85vh] flex flex-col animate-fade-in">
        <div className="flex items-center justify-between p-4 border-b border-gray-200 dark:border-gray-800">
          <div className="flex items-center">
            <div className="w-10 h-10 bg-gray-100 dark:bg-gray-800 rounded-md flex items-center justify-center mr-3">
              <img 
                src={pkg.iconUrl} 
                alt={`${pkg.name} logo`} 
                className="w-6 h-6 object-contain"
                onError={(e) => {
                  (e.target as HTMLImageElement).src = 'https://cdn.jsdelivr.net/gh/simple-icons/simple-icons/icons/windows.svg';
                }}
              />
            </div>
            <div>
              <h2 className="text-xl font-semibold text-gray-900 dark:text-gray-100">{pkg.name}</h2>
              <p className="text-sm text-gray-500 dark:text-gray-400">v{pkg.version} • {pkg.publisher}</p>
            </div>
          </div>
          
          <button
            onClick={onClose}
            className="p-2 rounded-full hover:bg-gray-100 dark:hover:bg-gray-800 text-gray-500 focus:outline-none"
            aria-label="Close details"
          >
            <X size={20} />
          </button>
        </div>
        
        <div className="flex-1 overflow-y-auto p-4">
          <div className="flex flex-wrap gap-2 mb-4">
            <Badge variant={pkg.isInstalled ? (pkg.isUpdatable ? 'warning' : 'success') : 'default'}>
              {pkg.isInstalled ? (pkg.isUpdatable ? 'Update Available' : 'Installed') : 'Not Installed'}
            </Badge>
            <Badge variant="info">{pkg.category}</Badge>
          </div>
          
          <p className="text-gray-700 dark:text-gray-300 mb-6">
            {pkg.description}
          </p>
          
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4 mb-6">
            <div className="flex items-center space-x-2 text-sm text-gray-600 dark:text-gray-400">
              <HardDrive size={16} />
              <span>Size: {pkg.size}</span>
            </div>
            <div className="flex items-center space-x-2 text-sm text-gray-600 dark:text-gray-400">
              <Calendar size={16} />
              <span>Last Updated: {formatDate(pkg.lastUpdated)}</span>
            </div>
            <div className="flex items-center space-x-2 text-sm text-gray-600 dark:text-gray-400">
              <Download size={16} />
              <span>{pkg.downloadCount.toLocaleString()} downloads</span>
            </div>
            {pkg.installDate && (
              <div className="flex items-center space-x-2 text-sm text-gray-600 dark:text-gray-400">
                <Clock size={16} />
                <span>Installed on: {formatDate(pkg.installDate)}</span>
              </div>
            )}
          </div>
          
          <div className="mb-6">
            <h3 className="text-lg font-medium text-gray-900 dark:text-gray-100 mb-2">Rating</h3>
            {renderRatingStars(pkg.rating)}
          </div>
          
          <div className="mb-6">
            <h3 className="text-lg font-medium text-gray-900 dark:text-gray-100 mb-2">Details</h3>
            <div className="prose prose-sm dark:prose-invert max-w-none">
              <p>
                Developed by {pkg.publisher}, {pkg.name} is a powerful {pkg.category.toLowerCase()} tool 
                designed for Windows. With a strong user rating of {pkg.rating}/5 and over {pkg.downloadCount.toLocaleString()} downloads,
                it's a popular choice for users looking for reliable software in this category.
              </p>
              <p>
                The latest version ({pkg.version}) was released on {formatDate(pkg.lastUpdated)}.
                {pkg.isUpdatable && ' An update is available for your currently installed version.'}
              </p>
            </div>
          </div>
        </div>
        
        <div className="p-4 border-t border-gray-200 dark:border-gray-800 flex justify-between items-center">
          <a 
            href="#" 
            className="text-blue-600 dark:text-blue-400 flex items-center hover:underline text-sm"
            aria-label="Visit publisher website"
          >
            <ExternalLink size={16} className="mr-1" />
            Visit publisher website
          </a>
          
          <div className="flex space-x-3">
            {!pkg.isInstalled ? (
              <Button
                variant="primary"
                leftIcon={<Download size={18} />}
                onClick={() => installPackage(pkg.id)}
              >
                Install
              </Button>
            ) : pkg.isUpdatable ? (
              <Button
                variant="secondary"
                leftIcon={<RefreshCw size={18} />}
                onClick={() => updatePackage(pkg.id)}
              >
                Update
              </Button>
            ) : (
              <Button
                variant="danger"
                leftIcon={<Trash2 size={18} />}
                onClick={() => {
                  uninstallPackage(pkg.id);
                  onClose();
                }}
              >
                Uninstall
              </Button>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};

export default PackageDetails;