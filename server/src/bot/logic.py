from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession
from fastapi import status
from src.bot.exceptions import BotException
from passlib.context import CryptContext
from datetime import datetime

from src.sensors.models import User, System


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


async def _change_user_status(chat_id: int, db: AsyncSession):
    try:
        result = await db.execute(select(User).filter(User.chat_id == chat_id, User.deleted_at == None))
        user = result.scalar_one_or_none()

        if not user:
            raise BotException("Пользователь не найден.")

        user.isAdmin = True  
        await db.commit()
    except Exception as e:
        await db.rollback()
        raise

async def _remove_admin_rights(target_chat_id: int, requester_chat_id: int, db: AsyncSession):
    try:
        requester = await db.execute(
            select(User).where(User.chat_id == requester_chat_id, User.isAdmin == True, User.deleted_at == None)
        )
        requester = requester.scalar_one_or_none()
        if not requester:
            raise BotException("У вас нет прав для выполнения этой команды.")

        target = await db.execute(
            select(User).where(User.chat_id == target_chat_id, User.deleted_at == None)
        )
        target = target.scalar_one_or_none()
        if not target:
            raise BotException("Пользователь не найден.")

        target.isAdmin = False

        await db.commit()
    except Exception:
        await db.rollback()
        raise

async def _set_wanna_entrance_open(requester_chat_id: int, db: AsyncSession):
    try:
        user = await db.execute(
            select(User).where(User.chat_id == requester_chat_id, User.deleted_at == None)
        )
        user = user.scalar_one_or_none()
        if not user:
            raise BotException("Вы не зарегистрированы в системе.")

        result = await db.execute(select(System).where(System.id == 1))
        system = result.scalar_one_or_none()
        if not system:
            raise BotException("Системная запись не найдена.")

        system.isWannaEntranceOpen = 1
        await db.commit()
    except Exception as e:
        await db.rollback()
        raise


async def _set_wanna_exit_open(requester_chat_id: int, db: AsyncSession):
    try:
        user = await db.execute(
            select(User).where(User.chat_id == requester_chat_id, User.deleted_at == None)
        )
        user = user.scalar_one_or_none()
        if not user:
            raise BotException("Вы не зарегистрированы в системе.")

        result = await db.execute(select(System).where(System.id == 1))
        system = result.scalar_one_or_none()
        if not system:
            raise BotException("Системная запись не найдена.")

        system.isWannaExitOpen = 1
        await db.commit()
    except Exception as e:
        await db.rollback()
        raise
