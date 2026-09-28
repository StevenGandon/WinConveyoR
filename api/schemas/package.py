import datetime as _dt
from typing import Optional
from pydantic import BaseModel, Field

class PackageCreate(BaseModel):
    name: str = Field(..., min_length=1, max_length=100, example="hello")
    version: str = Field(..., min_length=1, max_length=50, example="1.0.0")
    description: Optional[str] = Field(None, example="A friendly greeting tool")
    arch: str = Field(..., min_length=1, max_length=50, example="x86_64")
    machine: str = Field(..., min_length=1, max_length=50, example="linux")

class PackageOut(BaseModel):
    id: int
    name: str
    version: str
    description: Optional[str]
    arch: str
    machine: str
    owner_id: int
    filename: str
    checksum: str
    size: int
    created_at: _dt.datetime

    model_config = {"from_attributes": True}
