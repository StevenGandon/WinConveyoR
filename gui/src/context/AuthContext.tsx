import React, { createContext, useContext, useState, useEffect } from "react";
import { getMe, UserOut } from "../services/api";

interface AuthContextType {
  token: string | null;
  user: UserOut | null;
  isLoading: boolean;
  setToken: (token: string | null) => void;
  logout: () => void;
}

const AuthContext = createContext<AuthContextType | undefined>(undefined);

export const AuthProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [token, setTokenState] = useState<string | null>(() => localStorage.getItem("wcr_token"));
  const [user, setUser] = useState<UserOut | null>(null);
  const [isLoading, setIsLoading] = useState(!!localStorage.getItem("wcr_token"));

  const setToken = (t: string | null) => {
    setTokenState(t);
    if (t) {
      localStorage.setItem("wcr_token", t);
    } else {
      localStorage.removeItem("wcr_token");
    }
  };

  const logout = () => {
    setToken(null);
    setUser(null);
  };

  useEffect(() => {
    if (!token) {
      setUser(null);
      setIsLoading(false);
      return;
    }

    setIsLoading(true);
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 5000);
    getMe(token, controller.signal)
      .then(setUser)
      .catch(() => logout())
      .finally(() => { clearTimeout(timer); setIsLoading(false); });
    return () => { controller.abort(); clearTimeout(timer); };
  }, [token]);

  return (
    <AuthContext.Provider value={{ token, user, isLoading, setToken, logout }}>
      {children}
    </AuthContext.Provider>
  );
};

export const useAuth = (): AuthContextType => {
  const ctx = useContext(AuthContext);
  if (!ctx) throw new Error("useAuth must be used within AuthProvider");
  return ctx;
};
