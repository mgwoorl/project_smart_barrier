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

class AdminStates(StatesGroup):
    waiting_for_register_id = State()
    waiting_for_remove_admin_id = State()

ADMIN_PASSWORD_HASH = hashlib.sha256("admin228".encode()).hexdigest()

@dp.message(CommandStart())
async def handle_start(message: Message):
    await message.answer("👋 Привет! Я Бот Барьер. Выберите действие ниже 👇", reply_markup=user_keyboard)


@dp.message(F.text == "📋 Мой Chat ID")
async def handle_chat_id(message: Message):
    await message.answer(f"Ваш chat ID: `{message.chat.id}`", parse_mode="Markdown")


@dp.message(F.text == "🔓 Открыть въезд")
async def handle_open_entrance(message: Message):
    async with AsyncSession(engine) as db:
        try:
            await _set_wanna_entrance_open(message.chat.id, db)
            await message.answer("Въезд открыт ✅")
        except BotException as e:
            await message.answer(e.detail)
        except Exception as e:
            logger.exception("Ошибка открытия въезда")
            await message.answer("Произошла ошибка. Попробуйте позже.")


@dp.message(F.text == "🚪 Открыть выезд")
async def handle_open_exit(message: Message):
    async with AsyncSession(engine) as db:
        try:
            await _set_wanna_exit_open(message.chat.id, db)
            await message.answer("Выезд открыт ✅")
        except BotException as e:
            await message.answer(e.detail)
        except Exception as e:
            logger.exception("Ошибка открытия выезда")
            await message.answer("Произошла ошибка. Попробуйте позже.")


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


@dp.message(F.text == "🛡 Стать админом")
async def handle_become_admin(message: Message):
    await message.answer("Введите пароль администратора:")


@dp.message(F.text.regexp("^admin228$"))
async def handle_admin_password(message: Message):
    async with AsyncSession(engine) as db:
        try:
            await _change_user_status(message.chat.id, db)
            await message.answer("Вы стали администратором ✅", reply_markup=admin_keyboard)
        except BotException as e:
            await message.answer(e.detail)
        except Exception as e:
            logger.exception("Ошибка назначения администратора")
            await message.answer("Произошла ошибка. Попробуйте позже.")


if __name__ == "__main__":
    asyncio.run(main())

