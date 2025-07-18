from sqlalchemy import Column, Integer, String, DateTime, Boolean, ForeignKey
from sqlalchemy.orm import relationship
from sqlalchemy.sql import func
from src.database import BaseDBModel
from sqlalchemy import Date, Time

class User(BaseDBModel):
    __tablename__ = "users"

    id = Column(Integer, primary_key=True, autoincrement=True)
    chat_id = Column(String(255), nullable=False)
    isAdmin = Column(Boolean, nullable=False)
    deleted_at = Column(DateTime, nullable=True)

class Sensor(BaseDBModel):
    __tablename__ = "sensors"

    id = Column(Integer, primary_key=True, autoincrement=True)
    distance_exit = Column(Integer, nullable=True)         
    distance_entrance = Column(Integer, nullable=True)    
    free_places = Column(Integer, nullable=True)           
    co2 = Column(Integer, nullable=True)

class System(BaseDBModel):
    __tablename__ = "system"

    id = Column(Integer, primary_key=True, autoincrement=True)
    isEntranceBlock = Column(Boolean, nullable=False)       
    isWannaEntranceOpen = Column(Boolean, nullable=False)
    isWannaExitOpen = Column(Boolean, nullable=False)

class DayStatistic(BaseDBModel):
    __tablename__ = "day_statistics"

    id = Column(Integer, primary_key=True, autoincrement=True)
    date = Column(Date, nullable=False)
    hour = Column(Integer, nullable=False)  # от 0 до 23 включительно
    entered = Column(Integer, default=0)
    exited = Column(Integer, default=0)
