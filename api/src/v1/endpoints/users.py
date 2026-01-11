from fastapi import APIRouter, Depends, HTTPException, status, Request
from sqlalchemy.orm import Session
from api.src.v1.deps import get_db, limiter
from api.db.repository import Repository
from api.db.models.user import User
from api.schemas.user import UserIn, UserOut

router = APIRouter()

@router.post("", response_model=UserOut)
@limiter.limit("10/second")
def create_user(request: Request, user: UserIn, db: Session = Depends(get_db)):
    repo = Repository[User](db, User)
    return repo.create(user.model_dump())

@router.get("/{user_id}", response_model=UserOut)
def read_user(user_id: int, db: Session = Depends(get_db)):
    repo = Repository[User](db, User)
    obj = repo.get(user_id)
    if not obj:
        raise HTTPException(status_code=404, detail="User not found")
    return obj

@router.put("/{user_id}", response_model=UserOut)
def update_user(user_id: int, user: UserIn, db: Session = Depends(get_db)):
    repo = Repository[User](db, User)
    db_obj = repo.get(user_id)
    if not db_obj:
        raise HTTPException(status_code=404, detail="User not found")
    return repo.update(db_obj, user.dict())

@router.delete("/{user_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_user(user_id: int, db: Session = Depends(get_db)):
    repo = Repository[User](db, User)
    try:
        repo.delete(user_id)
    except ValueError:
        raise HTTPException(status_code=404, detail="User not found")
