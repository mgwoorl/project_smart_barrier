from fastapi import status
from sqlalchemy import select, and_
from sqlalchemy.ext.asyncio import AsyncSession

from src.users import UserRegisterRequestModel

from passlib.context import CryptContext
from src.users.exceptions import UserException
from src.users.models import User

pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")


def get_password_hash(password: str) -> str:
    return pwd_context.hash(password)


async def get_user_by_chat_id(chat_id: str, db: AsyncSession):
    result = await db.execute(select(User).filter(User.chat_id == chat_id))
    return result.scalar_one_or_none()


async def _register_user(user: UserRegisterRequestModel, db: AsyncSession):
    found_user = await get_user_by_chat_id(user.chat_id, db)

    if found_user is not None:
        raise UserException(status.HTTP_400_BAD_REQUEST, "The User is already registered")

    db_user = User(
        name = user.name,
        chat_id = user.chat_id,
        password = get_password_hash(user.password)
    )

    db.add(db_user)

    await db.commit()
    await db.refresh(db_user)