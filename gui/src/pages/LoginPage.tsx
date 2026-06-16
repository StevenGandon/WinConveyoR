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
    <div className="flex items-center justify-center min-h-screen bg-gray-50 dark:bg-gray-950">
      <Card className="w-full max-w-md mx-4">
        <CardHeader>
          <div className="flex items-center justify-center mb-4">
            <PackageIcon className="h-10 w-10 text-blue-600 dark:text-blue-400" />
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
              <p className="text-sm text-red-600 dark:text-red-400 whitespace-pre-line">{error}</p>
            )}

            <Button variant="primary" className="w-full" disabled={loading}>
              {loading ? "Signing in..." : "Sign in"}
            </Button>
          </form>

          <p className="mt-4 text-center text-sm text-gray-600 dark:text-gray-400">
            Don't have an account?{" "}
            <button
              onClick={onSwitchToRegister}
              className="text-blue-600 dark:text-blue-400 hover:underline font-medium"
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
