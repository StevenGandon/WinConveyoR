const API_URL = "http://localhost:8010/api/v1";

async function request<T>(endpoint: string, options: RequestInit = {}): Promise<T> {
  const res = await fetch(`${API_URL}${endpoint}`, {
    headers: { "Content-Type": "application/json", ...options.headers },
    ...options,
  });

  const body = await res.json();

  if (!res.ok) {
    const detail = body.detail;
    if (Array.isArray(detail)) {
      const messages = detail.map((e: any) => {
        const field = e.loc?.[e.loc.length - 1] ?? "?";
        return `${field}: ${e.msg}`;
      });
      throw new Error(messages.join("\n"));
    }
    throw new Error(typeof detail === "string" ? detail : res.statusText);
  }

  return body as T;
}

export interface UserOut {
  id: number;
  username: string;
  email: string;
  full_name: string | null;
  created_at: string;
}

export interface TokenOut {
  access_token: string;
  token_type: string;
}

export function register(username: string, email: string, password: string, full_name?: string) {
  return request<UserOut>("/auth/register", {
    method: "POST",
    body: JSON.stringify({ username, email, password, full_name: full_name || null }),
  });
}

export function login(email: string, password: string) {
  return request<TokenOut>("/auth/login", {
    method: "POST",
    body: JSON.stringify({ email, password }),
  });
}

export function getMe(token: string, signal?: AbortSignal) {
  return request<UserOut>("/users/me", {
    headers: { Authorization: `Bearer ${token}` },
    signal,
  });
}
