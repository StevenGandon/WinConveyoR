import { useRef, useEffect } from 'react';
import { Terminal } from 'lucide-react';
import { useCli } from '../context/CliContext';
import Header from '../components/layout/Header';

const OutputPage: React.FC = () => {
  const { log, busy } = useCli();
  const logRef = useRef<HTMLPreElement>(null);

  useEffect(() => {
    if (logRef.current) logRef.current.scrollTop = logRef.current.scrollHeight;
  }, [log]);

  return (
    <>
      <Header title="Output" />
      <div className="p-6 flex flex-col flex-1 min-h-0">
        <div className="flex items-center space-x-2 mb-4">
          <Terminal size={18} className="text-wc-muted dark:text-wc-muted-dark" />
          <p className="text-sm text-wc-muted dark:text-wc-muted-dark">
            {busy ? 'Running...' : log ? 'Last command output' : 'No output yet'}
          </p>
        </div>
        <pre
          ref={logRef}
          className="flex-1 bg-wc-card text-wc-success text-sm font-mono p-4 rounded-lg overflow-auto whitespace-pre-wrap min-h-[200px] dark:bg-wc-card-dark dark:text-wc-success-dark border border-wc-border dark:border-wc-border-dark"
        >
          {log || 'Waiting for a command...'}
        </pre>
      </div>
    </>
  );
};

export default OutputPage;
