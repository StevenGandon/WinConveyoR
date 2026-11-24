from sqlalchemy import Column, DateTime, ForeignKey, Integer, String, Text, JSON, func
from api.db.base import Base

# A shared package = an archive (package.zip / .tar.gz) + a config descriptor:
#   - package.json  -> "built"   package
#   - package.yml   -> "unbuilt" package
# Metadata (name/version/arch/machine/description/depends) is parsed from the
# config at upload time, not entered by hand.

class Package(Base):
    __tablename__ = "packages"

    id = Column(Integer, primary_key=True, index=True)
    name = Column(String(100), nullable=False, index=True)
    version = Column(String(50), nullable=False)
    description = Column(Text)
    arch = Column(String(50), nullable=False)
    machine = Column(String(50), nullable=False)
    depends = Column(JSON, nullable=False, default=list)
    kind = Column(String(10), nullable=False)  # "built" | "unbuilt"
    owner_id = Column(Integer, ForeignKey("users.id", ondelete="CASCADE"), nullable=False)
    archive_filename = Column(String(255), nullable=False)
    archive_path = Column(String(512), nullable=False)
    config_filename = Column(String(255), nullable=False)
    config_path = Column(String(512), nullable=False)
    checksum = Column(String(64), nullable=False)  # sha256 of the archive
    size = Column(Integer, nullable=False)          # archive size in bytes
    created_at = Column(DateTime(timezone=True), server_default=func.now(), nullable=False)
