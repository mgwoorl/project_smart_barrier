from fastapi import APIRouter, Depends, status
from sqlalchemy.ext.asyncio import AsyncSession
from starlette.responses import JSONResponse
import logging

from src.database import get_db_session
from src.sensors.schemes import ESPDataModel, SimpleResponseModel
from src.sensors.logic import upsert_sensor_data, get_status
from src.sensors.exceptions import SensorException

sensor_router = APIRouter(prefix="/sensors")
logger = logging.getLogger(__name__)

@sensor_router.post("/data", response_model=SimpleResponseModel)
async def receive_esp_data(data: ESPDataModel, db: AsyncSession = Depends(get_db_session)):
    try:
        updated = await upsert_sensor_data(data, db)
        return SimpleResponseModel(detail=f"Sensor {updated.id} data updated")
    except SensorException as se:
        return JSONResponse(
            status_code=se.status_code,
            content={"detail": se.detail}
        )
    except Exception as e:
        logger.exception(f"Unexpected error while receiving ESP data: {e}")
        return JSONResponse(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            content={"detail": "Internal server error"}
        )
