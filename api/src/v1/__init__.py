from fastapi import APIRouter
from api.src.v1.endpoints import users, auth, packages

router = APIRouter()
router.include_router(auth.router, prefix="/auth", tags=["auth"])
router.include_router(users.router, prefix="/users", tags=["users"])
router.include_router(packages.router, prefix="/packages", tags=["packages"])
