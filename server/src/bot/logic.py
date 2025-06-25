from sqlalchemy import select, func
from sqlalchemy.ext.asyncio import AsyncSession
from aiogram.types import InputFile
import matplotlib.pyplot as plt
import io
import datetime as dt

from src.bot.exceptions import BotException
from src.sensors.models import User, System, DayStatistic

from server.src import engine

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

async def _get_day_graph(db: AsyncSession, date: dt.date) -> str:
    # Собираем суммарные входы/выходы по часам за день
    res = await db.execute(
        select(
            DayStatistic.hour,
            func.sum(DayStatistic.entered).label("enters"),
            func.sum(DayStatistic.exited).label("exits")
        ).where(DayStatistic.date == date)
         .group_by(DayStatistic.hour)
         .order_by(DayStatistic.hour)
    )
    records = res.all()
    if not records:
        raise BotException("Нет данных за сегодня.")

    hours = [r[0] for r in records]
    enters = [r.enters for r in records]
    exits = [r.exits for r in records]

    plt.figure(figsize=(8,4))
    plt.plot(hours, enters, label="Въезд", marker="o")
    plt.plot(hours, exits, label="Выезд", marker="x")
    plt.xlabel("Час")
    plt.ylabel("Количество")
    plt.title(f"Статистика за {date.strftime('%d.%m.%Y')}")
    plt.legend()
    plt.grid(True)

    buf = io.BytesIO()
    plt.tight_layout()
    plt.savefig(buf, format="png")
    buf.seek(0)
    plt.close()

    output_dir = os.path.join(os.getcwd(), "graphs")
    os.makedirs(output_dir, exist_ok=True)

    path = os.path.join(output_dir, f"day_stats_{date}.png")
    with open(path, "wb") as f:
        f.write(buf.read())

    return path
