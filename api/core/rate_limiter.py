from fastapi.responses import PlainTextResponse
from slowapi import Limiter
from slowapi.errors import RateLimitExceeded
from slowapi.util import get_remote_address
from api.core.config import get_settings

settings = get_settings()

limiter = Limiter(key_func=get_remote_address, default_limits=[settings.RATE_LIMIT])

def rate_limit_handler(request, exc: RateLimitExceeded):
    return PlainTextResponse("Too many requests", status_code=429)
