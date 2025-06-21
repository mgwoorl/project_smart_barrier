import asyncio
import os
from aiogram import Bot, Dispatcher
from aiogram.filters import CommandStart, Command
from aiogram.types import Message, ReplyKeyboardRemove
from sqlalchemy.ext.asyncio import AsyncSession
import logging
from src.bot.exceptions import BotException
from src.bot.logic import _add_new_user
from src.database import engine

logger = logging.getLogger(__name__)


TELEGRAM_BOT_TOKEN = os.environ.get("BOT_TOKEN")

if not TELEGRAM_BOT_TOKEN:
    raise ValueError("BOT_TOKEN is not set in environment variables.")

bot = Bot(token=TELEGRAM_BOT_TOKEN)
dp = Dispatcher()


@dp.message(CommandStart())
async def handle_start(message: Message):
    instruction = "Привет, я ботик"
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


if __name__ == "__main__":
    asyncio.run(main())
