import json
import os
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError

_DEFAULT_API_URL = "http://localhost:8010/api/v1"
_TOKEN_PATH = os.path.expanduser("~/.config/wcr/token")

def _api_url() -> str:
    return os.environ.get("WCR_API_URL", _DEFAULT_API_URL)

_FIELD_HINTS = {
    "email": "not a valid email (e.g. user@example.com)",
    "password": "must be at least 8 characters",
    "username": "must be between 3 and 50 characters",
}

def _format_error(detail) -> str:
    if isinstance(detail, str):
        return detail
    if isinstance(detail, list):
        parts = []
        for err in detail:
            field = err.get("loc", [])[-1] if err.get("loc") else "?"
            parts.append(f"{field}: {_FIELD_HINTS.get(field, err.get('msg', 'invalid'))}")
        return "\n".join(parts)
    return str(detail)

def _post(endpoint: str, body: dict, token: str | None = None) -> dict:
    url = f"{_api_url()}{endpoint}"
    data = json.dumps(body).encode()
    headers = {"Content-Type": "application/json"}
    if token:
        headers["Authorization"] = f"Bearer {token}"

    req = Request(url, data=data, headers=headers, method="POST")
    try:
        with urlopen(req) as resp:
            return json.loads(resp.read())
    except HTTPError as e:
        detail = json.loads(e.read()).get("detail", e.reason)
        raise RuntimeError(_format_error(detail))
    except URLError as e:
        raise RuntimeError(f"cannot reach API ({e.reason})")

def _get(endpoint: str, token: str | None = None) -> dict:
    url = f"{_api_url()}{endpoint}"
    headers = {}
    if token:
        headers["Authorization"] = f"Bearer {token}"

    req = Request(url, headers=headers, method="GET")
    try:
        with urlopen(req) as resp:
            return json.loads(resp.read())
    except HTTPError as e:
        detail = json.loads(e.read()).get("detail", e.reason)
        raise RuntimeError(_format_error(detail))
    except URLError as e:
        raise RuntimeError(f"cannot reach API ({e.reason})")

def save_token(token: str) -> None:
    os.makedirs(os.path.dirname(_TOKEN_PATH), exist_ok=True)
    with open(_TOKEN_PATH, "w") as f:
        f.write(token)

def load_token() -> str | None:
    try:
        with open(_TOKEN_PATH, "r") as f:
            return f.read().strip()
    except FileNotFoundError:
        return None

def register(username: str, email: str, password: str, full_name: str | None = None) -> dict:
    body = {"username": username, "email": email, "password": password}
    if full_name:
        body["full_name"] = full_name
    return _post("/auth/register", body)

def login(email: str, password: str) -> str:
    resp = _post("/auth/login", {"email": email, "password": password})
    token = resp["access_token"]
    save_token(token)
    return token

def get_me(token: str) -> dict:
    return _get("/users/me", token)
