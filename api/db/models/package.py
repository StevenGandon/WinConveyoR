from sqlalchemy import Column, DateTime, ForeignKey, Integer, String, Text, func
from api.db.base import Base

class Package(Base):
    __tablename__ = "packages"

    id = Column(Integer, primary_key=True, index=True)
    name = Column(String(100), nullable=False, index=True)
    version = Column(String(50), nullable=False)
    description = Column(Text)
    arch = Column(String(50), nullable=False)
    machine = Column(String(50), nullable=False)
    owner_id = Column(Integer, ForeignKey("users.id", ondelete="CASCADE"), nullable=False)
    filename = Column(String(255), nullable=False)
    artifact_path = Column(String(512), nullable=False)
    checksum = Column(String(64), nullable=False)
    size = Column(Integer, nullable=False)
    created_at = Column(DateTime(timezone=True), server_default=func.now(), nullable=False)
