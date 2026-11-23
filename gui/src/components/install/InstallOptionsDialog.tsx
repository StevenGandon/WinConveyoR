import { useEffect, useMemo, useState } from 'react';
import { Download, FileCode, Settings2, X } from 'lucide-react';
import Card from '../ui/Card';
import Button from '../ui/Button';
import Input from '../ui/Input';
import Badge from '../ui/Badge';
import type { InstallOptions, PackageOption } from '../../context/CliContext';

interface InstallOptionsDialogProps {
  packageName: string;
  options: PackageOption[];
  onCancel: () => void;
  onConfirm: (opts: InstallOptions) => void;
  // fields of the package (no procedure) or of the user procedure replacing it (F30)
  onLoadOptions: (procedure?: string) => Promise<PackageOption[] | null>;
}

const MODE_PATTERN = /^[0-7]{1,4}$/;

// same rules as the lib (wizard_options.c), checked before launching the install
function validate(option: PackageOption, value: string): string | undefined {
  if (option.type === 'mode' && !MODE_PATTERN.test(value)) return 'Octal permissions expected (e.g. 0755)';
  if ((option.type === 'path' || option.type === 'user') && value.trim() === '') return 'Required';
  return undefined;
}

const defaultsOf = (options: PackageOption[]) => Object.fromEntries(options.map(o => [o.name, o.default]));

const InstallOptionsDialog: React.FC<InstallOptionsDialogProps> = ({
  packageName, options: packageOptions, onCancel, onConfirm, onLoadOptions,
}) => {
  const [prefix, setPrefix] = useState('');
  const [procedure, setProcedure] = useState('');
  const [loading, setLoading] = useState(false);
  const [options, setOptions] = useState<PackageOption[]>(packageOptions);
  const [values, setValues] = useState<Record<string, string>>(() => defaultsOf(packageOptions));

  // a procedure declares its own fields: reload them, back to the package ones on clear
  const changeProcedure = async (path: string) => {
    setLoading(true);
    const loaded = await onLoadOptions(path || undefined);
    setLoading(false);
    if (loaded === null) return;
    setProcedure(path);
    setOptions(loaded);
    setValues(defaultsOf(loaded));
  };

  const browseProcedure = async () => {
    const path = await window.electronAPI?.pickProcedure();
    if (path) await changeProcedure(path);
  };

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => { if (e.key === 'Escape') onCancel(); };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [onCancel]);

  const errors = useMemo(
    () => Object.fromEntries(options.map(o => [o.name, validate(o, values[o.name] ?? '')])),
    [options, values]
  );
  const hasErrors = Object.values(errors).some(Boolean);

  const handleConfirm = () => {
    // only send what the user changed: untouched defaults are re-resolved
    // by the lib against the chosen prefix
    const changed = Object.fromEntries(
      options.filter(o => values[o.name] !== o.default).map(o => [o.name, values[o.name]])
    );
    onConfirm({ prefix: prefix.trim() || undefined, values: changed, procedure: procedure || undefined });
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 p-4" onClick={onCancel}>
      <div className="w-full max-w-lg" onClick={(e) => e.stopPropagation()}>
        <Card className="p-5 space-y-4">
          <div className="flex items-center space-x-2">
            <Settings2 size={18} className="text-wc-accent dark:text-wc-accent-bright" />
            <h2 className="text-base font-semibold text-wc-fg dark:text-wc-fg-dark">
              Install {packageName}
            </h2>
          </div>

          <Input
            fullWidth
            label="Install prefix"
            placeholder="Default (~/.local)"
            value={prefix}
            onChange={(e) => setPrefix(e.target.value)}
          />

          <div>
            <span className="block text-sm font-medium text-wc-fg dark:text-wc-fg-dark mb-1">Custom procedure</span>
            <div className="flex items-center space-x-2">
              <Input
                fullWidth
                readOnly
                placeholder="Package procedure"
                value={procedure}
                leftIcon={<FileCode size={14} />}
              />
              {procedure && (
                <Button variant="ghost" size="sm" onClick={() => changeProcedure('')} disabled={loading} aria-label="Use package procedure">
                  <X size={14} />
                </Button>
              )}
              <Button variant="outline" size="sm" onClick={browseProcedure} disabled={loading}>
                Browse
              </Button>
            </div>
            <p className="mt-1 text-xs text-wc-muted dark:text-wc-muted-dark">
              Optional .yaml or .WIZARD replacing the package install steps.
            </p>
          </div>

          {options.length === 0 && (
            <p className="text-sm text-wc-muted dark:text-wc-muted-dark">
              {loading ? 'Loading fields...' : 'No fields to fill.'}
            </p>
          )}

          <div className="space-y-3 max-h-[50vh] overflow-y-auto pr-1">
            {options.map(option => (
              <div key={option.name}>
                <div className="flex items-center justify-between mb-1">
                  <span className="text-sm font-medium text-wc-fg dark:text-wc-fg-dark">{option.prompt}</span>
                  <Badge variant="neutral" size="sm">{option.type}</Badge>
                </div>
                <Input
                  fullWidth
                  value={values[option.name] ?? ''}
                  error={errors[option.name]}
                  onChange={(e) => setValues(prev => ({ ...prev, [option.name]: e.target.value }))}
                />
              </div>
            ))}
          </div>

          <p className="text-xs text-wc-muted dark:text-wc-muted-dark">
            Defaults follow the install prefix unless you edit them.
          </p>

          <div className="flex justify-end space-x-2">
            <Button variant="outline" size="sm" onClick={onCancel}>Cancel</Button>
            <Button
              variant="primary"
              size="sm"
              leftIcon={<Download size={14} />}
              onClick={handleConfirm}
              disabled={hasErrors || loading}
            >
              Install
            </Button>
          </div>
        </Card>
      </div>
    </div>
  );
};

export default InstallOptionsDialog;
