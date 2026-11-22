import datetime as _dt
from typing import Optional
from pydantic import BaseModel, EmailStr, Field

class UserBase(BaseModel):
    username: str = Field(..., example="john")
    full_name: Optional[str] = Field(None, example="John Doe")

class UserIn(UserBase):
    email: EmailStr = Field(..., example="john@example.com")

class UserOut(UserBase):
    id: int
    email: EmailStr
    created_at: _dt.datetime

    model_config = {"from_attributes": True}

class UserRegister(BaseModel):
    username: str = Field(..., min_length=3, max_length=50, example="john")
    email: EmailStr = Field(..., example="john@example.com")
    password: str = Field(..., min_length=8, example="securepass")
    full_name: Optional[str] = Field(None, example="John Doe")

class UserLogin(BaseModel):
    email: EmailStr = Field(..., example="john@example.com")
    password: str = Field(..., example="securepass")

class TokenOut(BaseModel):
    access_token: str
    token_type: str = "bearer"
