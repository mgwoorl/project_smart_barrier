from sqlalchemy import Column, Integer, String, DateTime, Boolean, ForeignKey
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
from src.database import BaseDBModel


class User(BaseDBModel):
    __tablename__ = "users"

    id = Column(Integer, primary_key=True, autoincrement=True)
    name = Column(String(255), nullable=False)
    chat_id = Column(String(255), nullable=False)
    password = Column(String(255), nullable=False)
    # name = Column(String(255), nullable=True)
    # last_activity = Column(DateTime, nullable=True)
    # created_at = Column(DateTime, nullable=False, server_default=func.now())
    # deleted_at = Column(DateTime, nullable=True)
    # ports = relationship("Port", back_populates="device")
    # chat_ids = relationship("DeviceChatIDs", back_populates="device")


class Car(BaseDBModel):
    __tablename__ = "cars"

    id = Column(Integer, primary_key=True, autoincrement=True)
    name = Column(String(255), nullable=False)
    speed = Column(Integer, nullable=False)