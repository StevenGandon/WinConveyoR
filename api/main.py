from fastapi import FastAPI
from slowapi import _rate_limit_exceeded_handler
from api.core.config import get_settings
from api.core.rate_limiter import limiter, rate_limit_handler
from api.src import api_router
from api.db.base import Base 
from api.db.session import engine

settings = get_settings()

app = FastAPI(title=settings.PROJECT_NAME, version="0.1.0")
app.state.limiter = limiter
app.add_exception_handler(429, rate_limit_handler or _rate_limit_exceeded_handler)
app.include_router(api_router, prefix=settings.API_ROOT)

@app.on_event("startup")
def _create_tables() -> None:
    Base.metadata.create_all(bind=engine)
