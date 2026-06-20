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
          <Terminal size={18} className="text-gray-500 dark:text-gray-400" />
          <p className="text-sm text-gray-500 dark:text-gray-400">
            {busy ? 'Running...' : log ? 'Last command output' : 'No output yet'}
          </p>
        </div>
        <pre
          ref={logRef}
          className="flex-1 bg-gray-950 text-green-400 text-sm font-mono p-4 rounded-lg overflow-auto whitespace-pre-wrap min-h-[200px]"
        >
          {log || 'Waiting for a command...'}
        </pre>
      </div>
    </>
  );
};

export default OutputPage;
