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

export interface PackageOut {
  id: number;
  name: string;
  version: string;
  description: string | null;
  arch: string;
  machine: string;
  owner_id: number;
  filename: string;
  checksum: string;
  size: number;
  created_at: string;
}

export function searchPackages(q?: string, signal?: AbortSignal) {
  const qs = q?.trim() ? `?q=${encodeURIComponent(q.trim())}` : "";
  return request<PackageOut[]>(`/packages${qs}`, { signal });
}

export function getPackage(id: number, signal?: AbortSignal) {
  return request<PackageOut>(`/packages/${id}`, { signal });
}

// Multipart upload: build the FormData in the caller and let the browser set
// the multipart boundary — do NOT force Content-Type here (request() would set
// application/json and break the parsing server-side).
export async function uploadPackage(form: FormData, token: string): Promise<PackageOut> {
  const res = await fetch(`${API_URL}/packages`, {
    method: "POST",
    headers: { Authorization: `Bearer ${token}` },
    body: form,
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
  return body as PackageOut;
}

export function downloadUrl(id: number): string {
  return `${API_URL}/packages/${id}/download`;
}

export async function deletePackage(id: number, token: string): Promise<void> {
  const res = await fetch(`${API_URL}/packages/${id}`, {
    method: "DELETE",
    headers: { Authorization: `Bearer ${token}` },
  });
  if (!res.ok) {
    let detail: string | undefined;
    try {
      detail = (await res.json()).detail;
    } catch { /* 204 / empty body */ }
    throw new Error(typeof detail === "string" ? detail : res.statusText);
  }
}
