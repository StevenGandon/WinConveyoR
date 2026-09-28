import hashlib
import uuid
from pathlib import Path
from fastapi import (
    APIRouter, Depends, File, Form, HTTPException, Request, UploadFile, status,
)
from fastapi.responses import FileResponse
from sqlalchemy.orm import Session
from api.src.v1.deps import get_db, get_current_user, limiter
from api.db.models.package import Package
from api.db.models.user import User
from api.schemas.package import PackageOut
from api.core.config import get_settings

router = APIRouter()
settings = get_settings()

_CHUNK_SIZE = 1024 * 1024  # 1 MiB streaming chunks
# Accepted artifact extensions (.wcr is the PoC test format, .tar.gz the target).
_ALLOWED_SUFFIXES = (".tar.gz", ".tgz", ".wcr")


def _package_or_404(db: Session, package_id: int) -> Package:
    pkg = db.get(Package, package_id)
    if pkg is None:
        raise HTTPException(status_code=404, detail="Package not found")
    return pkg


def _validate_suffix(filename: str) -> None:
    lowered = filename.lower()
    if not lowered.endswith(_ALLOWED_SUFFIXES):
        allowed = ", ".join(_ALLOWED_SUFFIXES)
        raise HTTPException(
            status_code=400,
            detail=f"Unsupported artifact type (allowed: {allowed})",
        )


@router.post("", response_model=PackageOut, status_code=status.HTTP_201_CREATED)
@limiter.limit("10/minute")
def upload_package(
    request: Request,
    name: str = Form(..., min_length=1, max_length=100),
    version: str = Form(..., min_length=1, max_length=50),
    arch: str = Form(..., min_length=1, max_length=50),
    machine: str = Form(..., min_length=1, max_length=50),
    description: str | None = Form(None),
    artifact: UploadFile = File(...),
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user),
):
    _validate_suffix(artifact.filename or "")

    settings.STORAGE_DIR.mkdir(parents=True, exist_ok=True)
    stored_name = f"{uuid.uuid4().hex}_{Path(artifact.filename or 'artifact').name}"
    dest = settings.STORAGE_DIR / stored_name

    sha256 = hashlib.sha256()
    size = 0
    with dest.open("wb") as out:
        while chunk := artifact.file.read(_CHUNK_SIZE):
            sha256.update(chunk)
            size += len(chunk)
            out.write(chunk)

    if size == 0:
        dest.unlink(missing_ok=True)
        raise HTTPException(status_code=400, detail="Empty artifact")

    pkg = Package(
        name=name,
        version=version,
        description=description,
        arch=arch,
        machine=machine,
        owner_id=current_user.id,
        filename=Path(artifact.filename or stored_name).name,
        artifact_path=str(dest),
        checksum=sha256.hexdigest(),
        size=size,
    )
    db.add(pkg)
    db.commit()
    db.refresh(pkg)
    return pkg


@router.get("", response_model=list[PackageOut])
@limiter.limit("60/minute")
def list_packages(
    request: Request,
    q: str | None = None,
    db: Session = Depends(get_db),
):
    query = db.query(Package)
    if q:
        term = f"%{q}%"
        query = query.filter(
            Package.name.ilike(term) | Package.description.ilike(term)
        )
    return query.order_by(Package.created_at.desc()).all()


@router.get("/{package_id}", response_model=PackageOut)
@limiter.limit("60/minute")
def get_package(request: Request, package_id: int, db: Session = Depends(get_db)):
    return _package_or_404(db, package_id)


@router.get("/{package_id}/download")
@limiter.limit("60/minute")
def download_package(request: Request, package_id: int, db: Session = Depends(get_db)):
    pkg = _package_or_404(db, package_id)
    if not Path(pkg.artifact_path).is_file():
        raise HTTPException(status_code=410, detail="Artifact no longer available")
    return FileResponse(
        pkg.artifact_path,
        filename=pkg.filename,
        media_type="application/octet-stream",
    )


@router.delete("/{package_id}", status_code=status.HTTP_204_NO_CONTENT)
@limiter.limit("20/minute")
def delete_package(
    request: Request,
    package_id: int,
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user),
):
    pkg = _package_or_404(db, package_id)
    if pkg.owner_id != current_user.id:
        raise HTTPException(status_code=403, detail="Not the package owner")
    Path(pkg.artifact_path).unlink(missing_ok=True)
    db.delete(pkg)
    db.commit()
