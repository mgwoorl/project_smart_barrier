from fastapi import APIRouter, Depends, status, Response
from sqlalchemy.ext.asyncio import AsyncSession
from starlette.responses import JSONResponse
import logging
from src.database import get_db_session

from src.users import UserRegisterRequestModel, _register_user

from src.users.exceptions import UserException

user_router = APIRouter(prefix=f"/users")
logger = logging.getLogger(__name__)


@user_router.post("/register")
async def register_device(user: UserRegisterRequestModel, db: AsyncSession = Depends(get_db_session)):
    try:
        await _register_user(user, db)
        return JSONResponse(content={}, status_code=status.HTTP_201_CREATED)
    except UserException as exception:
        return JSONResponse(content={"message": exception.detail}, status_code=exception.status_code)
    except Exception as exception:
        logger.exception(f"Server Exception: {exception}")
        return JSONResponse(content={}, status_code=status.HTTP_500_INTERNAL_SERVER_ERROR)
    