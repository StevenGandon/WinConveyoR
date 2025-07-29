from typing import Generator
from sqlalchemy.orm import Session
from api.db.session import SessionLocal
from api.core.rate_limiter import limiter

def get_db() -> Generator[Session, None, None]:
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()
