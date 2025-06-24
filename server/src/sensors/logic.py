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

async def reset_gate_flags(data: ResetGateStatusModel, db: AsyncSession):
    system = await db.get(System, 1)
    if not system:
        raise SensorException("System row not found", status_code=404)

    if data.reset_entrance:
        system.isWannaEntranceOpen = False
    if data.reset_exit:
        system.isWannaExitOpen = False

    await db.commit()
    await db.refresh(system)
    return system

async def get_status(db: AsyncSession):
    result = await db.execute(select(System))
    return result.scalars().all()
