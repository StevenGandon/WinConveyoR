from fastapi import APIRouter, Depends, HTTPException, status, Request
from sqlalchemy.orm import Session
from api.src.v1.deps import get_db, get_current_user, limiter
from api.db.repository import Repository
from api.db.models.user import User
from api.schemas.user import UserIn, UserOut

router = APIRouter()

@router.get("/me", response_model=UserOut)
def read_current_user(current_user: User = Depends(get_current_user)):
    return current_user

@router.get("/{user_id}", response_model=UserOut)
def read_user(user_id: int, db: Session = Depends(get_db)):
    repo = Repository[User](db, User)
    obj = repo.get(user_id)
    if not obj:
        raise HTTPException(status_code=404, detail="User not found")
    return obj

@router.put("/{user_id}", response_model=UserOut)
def update_user(
    user_id: int,
    user: UserIn,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user),
):
    if current_user.id != user_id:
        raise HTTPException(status_code=403, detail="Cannot modify another user")
    repo = Repository[User](db, User)
    db_obj = repo.get(user_id)
    if not db_obj:
        raise HTTPException(status_code=404, detail="User not found")
    return repo.update(db_obj, user.model_dump())

@router.delete("/{user_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_user(
    user_id: int,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user),
):
    if current_user.id != user_id:
        raise HTTPException(status_code=403, detail="Cannot delete another user")
    repo = Repository[User](db, User)
    try:
        repo.delete(user_id)
    except ValueError:
        raise HTTPException(status_code=404, detail="User not found")
