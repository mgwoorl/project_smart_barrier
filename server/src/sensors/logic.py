from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession
from src.sensors.models import Sensor, System
from src.sensors.schemes import ESPDataModel, ResetGateStatusModel
from src.sensors.exceptions import SensorException


async def upsert_sensor_data(data: ESPDataModel, db: AsyncSession):
    result = await db.execute(select(Sensor).filter(Sensor.id == data.id))
    sensor = result.scalar_one_or_none()

    if sensor:
        sensor.co2 = data.co2
        sensor.free_places = data.free_places
        sensor.distance_entrance = data.distance_entrance
        sensor.distance_exit = data.distance_exit
    else:
        sensor = Sensor(
            id=data.id,
            co2=data.co2,
            free_places=data.free_places,
            distance_entrance=data.distance_entrance,
            distance_exit=data.distance_exit
        )
        db.add(sensor)

    await db.commit()
    await db.refresh(sensor)
    return sensor
