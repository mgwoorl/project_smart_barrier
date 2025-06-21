from sqlalchemy import Column, Integer, String, DateTime, Boolean, ForeignKey
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
from src.database import BaseDBModel


class User(BaseDBModel):
    __tablename__ = "users"

    id = Column(Integer, primary_key=True, autoincrement=True)
    chat_id = Column(String(255), nullable=False)
    isAdmin = Column(Boolean, nullable=False)
    deleted_at = Column(DateTime, nullable=True)

