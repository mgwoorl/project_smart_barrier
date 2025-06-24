import asyncio
import os
import hashlib
import logging
from aiogram import Bot, Dispatcher, F
from aiogram.types import Message, ReplyKeyboardMarkup, KeyboardButton
from aiogram.filters import CommandStart
from sqlalchemy.ext.asyncio import AsyncSession

from aiogram.fsm.state import State, StatesGroup
from aiogram.fsm.context import FSMContext
from aiogram.fsm.storage.memory import MemoryStorage

from src.bot.exceptions import BotException
from src.bot.logic import (
    _add_new_user,
    _change_user_status,
    _remove_admin_rights,
    _set_wanna_entrance_open,
    _set_wanna_exit_open
)
from src.database import engine

logger = logging.getLogger(__name__)
TELEGRAM_BOT_TOKEN = os.environ.get("BOT_TOKEN")
if not TELEGRAM_BOT_TOKEN:
    raise ValueError("BOT_TOKEN is not set in environment variables.")

bot = Bot(token=TELEGRAM_BOT_TOKEN)
dp = Dispatcher(storage=MemoryStorage())

user_keyboard = ReplyKeyboardMarkup(
    keyboard=[
        [KeyboardButton(text="📋 Мой Chat ID")],
        [KeyboardButton(text="🔓 Открыть въезд"), KeyboardButton(text="🚪 Открыть выезд")],
        [KeyboardButton(text="🛡 Стать админом")],
    ],
    resize_keyboard=True
)

admin_keyboard = ReplyKeyboardMarkup(
    keyboard=[
        [KeyboardButton(text="📋 Мой Chat ID")],
        [KeyboardButton(text="🔓 Открыть въезд"), KeyboardButton(text="🚪 Открыть выезд")],
        [KeyboardButton(text="➕ Зарегистрировать пользователя")],
        [KeyboardButton(text="❌ Удалить админа")],
    ],
    resize_keyboard=True
)

@dp.message(CommandStart())
async def handle_start(message: Message):
    username = message.from_user.username
    instruction = f"👋 Привет, {username}!\nЯ — Бот Барьер 🤖\n\nВот список доступных команд:\n🔹 /register [chat_id] — регистрация пользователя (доступно только для админов)\n🔹 /chat — узнать свой уникальный айдишник\n\n"
    await message.answer(instruction)


@dp.message(Command("chat"))
async def handle_chat_id(message: Message):
    await message.answer(str(message.chat.id))


@dp.message(Command("register"))
async def register_command(message: Message):
    try:
        parts = message.text.split(" ", 1)
        if len(parts) != 2:
            await message.reply("Неверный формат команды. Используйте: /register [chat id пользователь]")
            return

        user_chat_id = parts[1]

        async with AsyncSession(engine) as db:
            try:
                await _add_new_user(user_chat_id, message.chat.id, db)
                await message.reply(f"Пользователь {user_chat_id} успешно добавлен.")
            except BotException as exception:
                await message.reply(exception.detail)
                return
            except Exception as e:
                logger.exception(f"Ошибка работы бота: {e}")
                await message.reply("Неизвестная ошибка. Начните сначала или попробуйте использовать команду позже")
                return
    except Exception as e:
        logger.exception(f"Ошибка работы бота: {e}")
        await message.reply("Произошла ошибка при обработке команды. Проверьте формат и повторите попытку.")


async def main():
    try:
        print("Bot started")
        await dp.start_polling(bot)
    except KeyboardInterrupt:
        print("Bot stopped")
    finally:
        await bot.session.close()


ADMIN_PASSWORD_HASH = hashlib.sha256("admin228".encode()).hexdigest()

@dp.message(Command("ImAdmin"))
async def im_admin_command(message: Message):
    try:
        parts = message.text.split(" ", 1)
        if len(parts) != 2:
            await message.reply("Неверный формат команды. Используйте: /ImAdmin [пароль]")
            return

        input_password = parts[1]
        input_password_hash = hashlib.sha256(input_password.encode()).hexdigest()

        if input_password_hash != ADMIN_PASSWORD_HASH:
            await message.reply("Неверный пароль.")
            return

        async with AsyncSession(engine) as db:
            try:
                await _change_user_status(message.chat.id, db)
                await message.reply("Теперь вы администратор.")
            except BotException as be:
                await message.reply(be.detail)
            except Exception as e:
                logger.exception("Ошибка при установке администратора.")
                await message.reply("Произошла ошибка. Попробуйте снова позже.")
    except Exception as e:
        logger.exception("Ошибка в команде ImAdmin.")
        await message.reply("Произошла ошибка.")


if __name__ == "__main__":
    asyncio.run(main())

