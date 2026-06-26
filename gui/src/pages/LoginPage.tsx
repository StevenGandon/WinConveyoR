import React, { useState } from "react";
import { useAuth } from "../context/AuthContext";
import { login } from "../services/api";
import Button from "../components/ui/Button";
import Input from "../components/ui/Input";
import Card, { CardHeader, CardTitle, CardContent } from "../components/ui/Card";
import { Package as PackageIcon } from "lucide-react";

interface LoginPageProps {
  onSwitchToRegister: () => void;
}

const LoginPage: React.FC<LoginPageProps> = ({ onSwitchToRegister }) => {
  const { setToken } = useAuth();
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError("");
    setLoading(true);

    try {
      const res = await login(email, password);
      setToken(res.access_token);
    } catch (err: any) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="flex items-center justify-center w-full h-full">
      <Card className="w-full max-w-md mx-4">
        <CardHeader>
          <div className="flex items-center justify-center mb-4">
            <PackageIcon className="h-10 w-10 text-wc-accent dark:text-wc-accent-bright" />
          </div>
          <CardTitle className="text-center">Sign in to WinConveyoR</CardTitle>
        </CardHeader>
        <CardContent>
          <form onSubmit={handleSubmit} className="space-y-4">
            <Input
              fullWidth
              label="Email"
              type="email"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              placeholder="you@example.com"
              required
            />
            <Input
              fullWidth
              label="Password"
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              placeholder="********"
              required
            />

            {error && (
              <p className="text-sm text-wc-danger dark:text-wc-danger-dark whitespace-pre-line">{error}</p>
            )}

            <Button variant="primary" className="w-full" disabled={loading}>
              {loading ? "Signing in..." : "Sign in"}
            </Button>
          </form>

          <div className="relative my-4">
            <div className="absolute inset-0 flex items-center">
              <div className="w-full border-t border-wc-border dark:border-wc-border-dark" />
            </div>
            <div className="relative flex justify-center text-xs uppercase">
              <span className="bg-wc-card dark:bg-wc-card-dark px-2 text-wc-muted dark:text-wc-muted-dark">or</span>
            </div>
          </div>

          <Button
            variant="secondary"
            className="w-full"
            onClick={() => setToken('anonymous')}
          >
            Continue without account
          </Button>

          <p className="mt-4 text-center text-sm text-wc-muted dark:text-wc-muted-dark">
            Don't have an account?{" "}
            <button
              onClick={onSwitchToRegister}
              className="text-wc-accent dark:text-wc-accent-bright hover:underline font-medium"
            >
              Create one
            </button>
          </p>
        </CardContent>
      </Card>
    </div>
  );
};

export default LoginPage;
