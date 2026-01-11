import datetime as _dt
from typing import Optional
from pydantic import BaseModel, Field

class UserBase(BaseModel):
    username: str = Field(..., example="john")
    full_name: Optional[str] = Field(None, example="John Doe")

class UserIn(UserBase):
    pass

class UserOut(UserBase):
    id: int
    created_at: _dt.datetime

    class Config:
        orm_mode = True
