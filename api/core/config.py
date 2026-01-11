from pathlib import Path
from functools import lru_cache
from pydantic_settings import BaseSettings

BASE_DIR = Path(__file__).resolve().parent.parent

class Settings(BaseSettings):
    API_ROOT: str = "/api"
    PROJECT_NAME: str = "WinConveyoR API"
    DATABASE_URL: str = "postgresql+psycopg2://postgres:postgres@localhost:5432/abstract_api"
    RATE_LIMIT: str = "200/minute"

    class Config:
        env_file = BASE_DIR / ".env"
        env_file_encoding = "utf-8"
        case_sensitive = False

@lru_cache
def get_settings() -> "Settings":
    return Settings()
