import asyncio
import os
from aiogram import Bot, Dispatcher
from aiogram.filters import CommandStart, Command
from aiogram.types import Message, ReplyKeyboardRemove
from sqlalchemy.ext.asyncio import AsyncSession
import logging
from src.database import engine

logger = logging.getLogger(__name__)


TELEGRAM_BOT_TOKEN = os.environ.get("BOT_TOKEN")

if not TELEGRAM_BOT_TOKEN:
    raise ValueError("BOT_TOKEN is not set in environment variables.")

bot = Bot(token=TELEGRAM_BOT_TOKEN)
dp = Dispatcher()


@dp.message(CommandStart())
async def handle_start(message: Message):
    instruction = "Привет. Я ботик"
    markup = ReplyKeyboardRemove()
    await message.answer(instruction, reply_markup=markup)


@dp.message(Command("link"))
async def link_command(message: Message):
    pass


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
