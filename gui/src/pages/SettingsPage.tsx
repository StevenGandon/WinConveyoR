import React from 'react';
import { Moon, Sun, Monitor, ZoomIn, ZoomOut, Sliders, Eye, Keyboard } from 'lucide-react';
import { useSettings } from '../context/SettingsContext';
import Header from '../components/layout/Header';
import Card, { CardHeader, CardTitle, CardContent } from '../components/ui/Card';
import Button from '../components/ui/Button';

const SettingsPage: React.FC = () => {
  const { settings, updateSettings } = useSettings();

  const themeButtons = [
    { value: 'light', label: 'Light', icon: <Sun size={18} /> },
    { value: 'dark', label: 'Dark', icon: <Moon size={18} /> },
    { value: 'system', label: 'System', icon: <Monitor size={18} /> },
  ];

  const contrastButtons = [
    { value: 'normal', label: 'Normal contrast' },
    { value: 'high', label: 'High contrast' },
  ];

  return (
    <>
      <Header title="Settings" />
      
      <div className="p-6 max-w-4xl">
        <div className="space-y-6">
          <Card>
            <CardHeader>
              <CardTitle>Appearance</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="space-y-6">
                <div>
                  <h3 className="text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Theme</h3>
                  <div className="flex flex-wrap gap-3">
                    {themeButtons.map(({ value, label, icon }) => (
                      <Button
                        key={value}
                        variant={settings.theme === value ? 'primary' : 'outline'}
                        leftIcon={icon}
                        onClick={() => updateSettings({ theme: value as 'light' | 'dark' | 'system' })}
                        aria-pressed={settings.theme === value}
                      >
                        {label}
                      </Button>
                    ))}
                  </div>
                </div>
                
                <div>
                  <h3 className="text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Contrast</h3>
                  <div className="flex flex-wrap gap-3">
                    {contrastButtons.map(({ value, label }) => (
                      <Button
                        key={value}
                        variant={settings.contrast === value ? 'primary' : 'outline'}
                        onClick={() => updateSettings({ contrast: value as 'normal' | 'high' })}
                        aria-pressed={settings.contrast === value}
                      >
                        {label}
                      </Button>
                    ))}
                  </div>
                </div>
                
                <div>
                  <h3 className="text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Font Size</h3>
                  <div className="flex items-center space-x-4">
                    <Button
                      variant="outline"
                      leftIcon={<ZoomOut size={18} />}
                      onClick={() => updateSettings({ fontSize: Math.max(12, settings.fontSize - 1) })}
                      aria-label="Decrease font size"
                      disabled={settings.fontSize <= 12}
                    >
                      Smaller
                    </Button>
                    <span className="text-gray-700 dark:text-gray-300 font-medium">
                      {settings.fontSize}px
                    </span>
                    <Button
                      variant="outline"
                      leftIcon={<ZoomIn size={18} />}
                      onClick={() => updateSettings({ fontSize: Math.min(20, settings.fontSize + 1) })}
                      aria-label="Increase font size"
                      disabled={settings.fontSize >= 20}
                    >
                      Larger
                    </Button>
                  </div>
                </div>
              </div>
            </CardContent>
          </Card>
          
          <Card>
            <CardHeader>
              <CardTitle>Accessibility</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="space-y-6">
                <div className="flex items-center justify-between">
                  <div className="flex items-center space-x-2">
                    <Eye size={18} className="text-gray-700 dark:text-gray-300" />
                    <span className="text-gray-700 dark:text-gray-300">Reduce motion</span>
                  </div>
                  <label className="inline-flex items-center cursor-pointer">
                    <input
                      type="checkbox"
                      className="sr-only peer"
                      checked={settings.reduceMotion}
                      onChange={(e) => updateSettings({ reduceMotion: e.target.checked })}
                    />
                    <div className="relative w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 dark:peer-focus:ring-blue-800 rounded-full peer dark:bg-gray-700 peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all dark:border-gray-600 peer-checked:bg-blue-600"></div>
                  </label>
                </div>
                
                <div className="flex items-center justify-between">
                  <div className="flex items-center space-x-2">
                    <Keyboard size={18} className="text-gray-700 dark:text-gray-300" />
                    <span className="text-gray-700 dark:text-gray-300">Enable keyboard shortcuts</span>
                  </div>
                  <label className="inline-flex items-center cursor-pointer">
                    <input
                      type="checkbox"
                      className="sr-only peer"
                      checked={settings.enableKeyboardShortcuts}
                      onChange={(e) => updateSettings({ enableKeyboardShortcuts: e.target.checked })}
                    />
                    <div className="relative w-11 h-6 bg-gray-200 peer-focus:outline-none peer-focus:ring-4 peer-focus:ring-blue-300 dark:peer-focus:ring-blue-800 rounded-full peer dark:bg-gray-700 peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all dark:border-gray-600 peer-checked:bg-blue-600"></div>
                  </label>
                </div>
              </div>
            </CardContent>
          </Card>
          
          <Card>
            <CardHeader>
              <CardTitle>About</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="space-y-2">
                <div className="flex items-center space-x-2">
                  <Sliders size={18} className="text-gray-700 dark:text-gray-300" />
                  <span className="text-gray-700 dark:text-gray-300">WinConveyoR</span>
                </div>
                <p className="text-gray-600 dark:text-gray-400">Version 1.0.0</p>
                <p className="text-gray-600 dark:text-gray-400 mt-2">
                  WinConveyoR is a modern package manager for Windows, designed with accessibility in mind.
                  It provides an easy way to discover, install, update, and manage software packages.
                </p>
              </div>
            </CardContent>
          </Card>
        </div>
      </div>
    </>
  );
};

export default SettingsPage;