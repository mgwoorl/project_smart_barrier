from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession
from fastapi import status
from src.bot.exceptions import BotException
from passlib.context import CryptContext
from datetime import datetime

from src.users.models import User


async def _add_new_user(user_chat_id: str, chat_id: int, db: AsyncSession):
    try:
        admin = await db.execute(select(User).filter(User.chat_id == chat_id, User.isAdmin == True, User.deleted_at == None))
        admin = admin.scalar_one_or_none()

        if not admin:
            raise BotException(detail="У вас нет доступа")
        

        user = await db.execute(select(User).filter(User.chat_id == user_chat_id, User.deleted_at == None))
        user = user.scalar_one_or_none()

        if user:
            raise BotException(detail="Такой пользователь уже зарегистрирован")
        
        deleted_user = await db.execute(select(User).filter(User.chat_id == user_chat_id, User.deleted_at != None))
        deleted_user = deleted_user.scalar_one_or_none()

        if deleted_user:
            deleted_user.deleted_at = None
            await db.commit()
            return
        

        db_user = User(
            chat_id = user_chat_id,
            isAdmin = False,
        )

        db.add(db_user)
        await db.commit()

    except Exception as e:
        await db.rollback()
        raise