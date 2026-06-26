import React, { useState } from "react";
import { useAuth } from "../context/AuthContext";
import { register, login } from "../services/api";
import Button from "../components/ui/Button";
import Input from "../components/ui/Input";
import Card, { CardHeader, CardTitle, CardContent } from "../components/ui/Card";
import { Package as PackageIcon } from "lucide-react";

interface RegisterPageProps {
  onSwitchToLogin: () => void;
}

const RegisterPage: React.FC<RegisterPageProps> = ({ onSwitchToLogin }) => {
  const { setToken } = useAuth();
  const [username, setUsername] = useState("");
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [confirm, setConfirm] = useState("");
  const [fullName, setFullName] = useState("");
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError("");

    if (password !== confirm) {
      setError("Passwords do not match");
      return;
    }

    setLoading(true);

    try {
      await register(username, email, password, fullName || undefined);
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
          <CardTitle className="text-center">Create your account</CardTitle>
        </CardHeader>
        <CardContent>
          <form onSubmit={handleSubmit} className="space-y-4">
            <Input
              fullWidth
              label="Username"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              placeholder="john"
              required
            />
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
              placeholder="Min. 8 characters"
              required
            />
            <Input
              fullWidth
              label="Confirm password"
              type="password"
              value={confirm}
              onChange={(e) => setConfirm(e.target.value)}
              placeholder="********"
              required
            />
            <Input
              fullWidth
              label="Full name (optional)"
              value={fullName}
              onChange={(e) => setFullName(e.target.value)}
              placeholder="John Doe"
            />

            {error && (
              <p className="text-sm text-wc-danger dark:text-wc-danger-dark whitespace-pre-line">{error}</p>
            )}

            <Button variant="primary" className="w-full" disabled={loading}>
              {loading ? "Creating account..." : "Create account"}
            </Button>
          </form>

          <p className="mt-4 text-center text-sm text-wc-muted dark:text-wc-muted-dark">
            Already have an account?{" "}
            <button
              onClick={onSwitchToLogin}
              className="text-wc-accent dark:text-wc-accent-bright hover:underline font-medium"
            >
              Sign in
            </button>
          </p>
        </CardContent>
      </Card>
    </div>
  );
};

export default RegisterPage;
