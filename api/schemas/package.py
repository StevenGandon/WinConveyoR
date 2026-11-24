import datetime as _dt
from typing import List, Optional
from pydantic import BaseModel

class PackageOut(BaseModel):
    id: int
    name: str
    version: str
    description: Optional[str]
    arch: str
    machine: str
    depends: List[str]
    kind: str
    owner_id: int
    archive_filename: str
    config_filename: str
    checksum: str
    size: int
    created_at: _dt.datetime

    model_config = {"from_attributes": True}
