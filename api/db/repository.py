from typing import Generic, List, Optional, Type, TypeVar
from sqlalchemy.orm import Session
from api.db.base import Base

T = TypeVar("T", bound=Base)

class Repository(Generic[T]):
    def __init__(self, db: Session, model: Type[T]):
        self.db = db
        self.model = model

    def get(self, obj_id: int) -> Optional[T]:
        return self.db.get(self.model, obj_id)

    def list(self, skip: int = 0, limit: int = 100) -> List[T]:
        return self.db.query(self.model).offset(skip).limit(limit).all()

    def create(self, obj_in: dict) -> T:
        obj = self.model(**obj_in)
        self.db.add(obj)
        self.db.commit()
        self.db.refresh(obj)
        return obj

    def update(self, db_obj: T, obj_in: dict) -> T:
        for field, value in obj_in.items():
            setattr(db_obj, field, value)
        self.db.commit()
        self.db.refresh(db_obj)
        return db_obj

    def delete(self, obj_id: int) -> None:
        obj = self.get(obj_id)
        if obj is None:
            raise ValueError("Object not found")
        self.db.delete(obj)
        self.db.commit()
