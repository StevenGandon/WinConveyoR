import hashlib
import json
import uuid
from pathlib import Path
import yaml
from fastapi import (
    APIRouter, Depends, File, HTTPException, Request, UploadFile, status,
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
_ARCHIVE_SUFFIXES = (".zip", ".tar.gz", ".tgz")
_JSON_SUFFIXES = (".json",)          # built package config
_YAML_SUFFIXES = (".yml", ".yaml")   # unbuilt package config


def _package_or_404(db: Session, package_id: int) -> Package:
    pkg = db.get(Package, package_id)
    if pkg is None:
        raise HTTPException(status_code=404, detail="Package not found")
    return pkg


def _kind_from_config(filename: str) -> str:
    lowered = filename.lower()
    if lowered.endswith(_JSON_SUFFIXES):
        return "built"
    if lowered.endswith(_YAML_SUFFIXES):
        return "unbuilt"
    raise HTTPException(
        status_code=400,
        detail="Config must be package.json (built) or package.yml (unbuilt)",
    )


def _require(data: dict, key: str, source: str):
    if key not in data or data[key] in (None, ""):
        raise HTTPException(status_code=400, detail=f"Missing '{key}' in {source}")
    return data[key]


def _parse_config(kind: str, raw: bytes) -> dict:
    """Extract package metadata from the uploaded config (json or yaml)."""
    if kind == "built":
        try:
            data = json.loads(raw)
        except json.JSONDecodeError as e:
            raise HTTPException(status_code=400, detail=f"Invalid package.json: {e}")
        if not isinstance(data, dict):
            raise HTTPException(status_code=400, detail="package.json must be an object")
        return {
            "name": _require(data, "package", "package.json"),
            "version": _require(data, "version", "package.json"),
            "arch": _require(data, "architecture", "package.json"),
            "machine": _require(data, "machine", "package.json"),
            "description": data.get("description"),
            "depends": list(data.get("depends") or []),
        }

    # unbuilt -> yaml wizard config
    try:
        data = yaml.safe_load(raw)
    except yaml.YAMLError as e:
        raise HTTPException(status_code=400, detail=f"Invalid package.yml: {e}")
    if not isinstance(data, dict) or "wizard" not in data:
        raise HTTPException(status_code=400, detail="package.yml must contain a 'wizard' section")
    meta = data["wizard"].get("metadata") if isinstance(data["wizard"], dict) else None
    if not isinstance(meta, dict):
        raise HTTPException(status_code=400, detail="Missing 'wizard.metadata' in package.yml")
    return {
        "name": _require(meta, "name", "package.yml metadata"),
        "version": _require(meta, "version", "package.yml metadata"),
        "arch": _require(meta, "architecture", "package.yml metadata"),
        "machine": _require(meta, "machine", "package.yml metadata"),
        "description": meta.get("description"),
        "depends": list(meta.get("deps") or []),
    }


def _store_file(upload: UploadFile, prefix: str) -> tuple[Path, str, int]:
    """Stream an upload to STORAGE_DIR, returning (path, sha256_hex, size)."""
    settings.STORAGE_DIR.mkdir(parents=True, exist_ok=True)
    stored = settings.STORAGE_DIR / f"{prefix}_{Path(upload.filename or 'file').name}"
    sha256 = hashlib.sha256()
    size = 0
    with stored.open("wb") as out:
        while chunk := upload.file.read(_CHUNK_SIZE):
            sha256.update(chunk)
            size += len(chunk)
            out.write(chunk)
    return stored, sha256.hexdigest(), size


@router.post("", response_model=PackageOut, status_code=status.HTTP_201_CREATED)
@limiter.limit("10/minute")
def upload_package(
    request: Request,
    archive: UploadFile = File(...),
    config: UploadFile = File(...),
    db: Session = Depends(get_db),
    current_user: User = Depends(get_current_user),
):
    if not (archive.filename or "").lower().endswith(_ARCHIVE_SUFFIXES):
        allowed = ", ".join(_ARCHIVE_SUFFIXES)
        raise HTTPException(status_code=400, detail=f"Archive must be one of: {allowed}")

    kind = _kind_from_config(config.filename or "")
    meta = _parse_config(kind, config.file.read())

    prefix = uuid.uuid4().hex
    archive_path, checksum, size = _store_file(archive, prefix)
    if size == 0:
        archive_path.unlink(missing_ok=True)
        raise HTTPException(status_code=400, detail="Empty archive")
    config_path, _, _ = _store_file(config, prefix)

    pkg = Package(
        name=meta["name"],
        version=meta["version"],
        description=meta["description"],
        arch=meta["arch"],
        machine=meta["machine"],
        depends=meta["depends"],
        kind=kind,
        owner_id=current_user.id,
        archive_filename=Path(archive.filename or "archive").name,
        archive_path=str(archive_path),
        config_filename=Path(config.filename or "config").name,
        config_path=str(config_path),
        checksum=checksum,
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
    if not Path(pkg.archive_path).is_file():
        raise HTTPException(status_code=410, detail="Artifact no longer available")
    return FileResponse(
        pkg.archive_path,
        filename=pkg.archive_filename,
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
    Path(pkg.archive_path).unlink(missing_ok=True)
    Path(pkg.config_path).unlink(missing_ok=True)
    db.delete(pkg)
    db.commit()
