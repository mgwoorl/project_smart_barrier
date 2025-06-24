from pydantic import BaseModel

class ESPDataModel(BaseModel):
    id: int  # ESP ID — будет primary key для обновления
    distance_exit: int | None = None
    distance_entrance: int | None = None
    free_places: int
    co2: int

class SimpleResponseModel(BaseModel):
    detail: str

from pydantic import BaseModel
from typing import Optional

class ResetGateStatusModel(BaseModel):
    reset_entrance: Optional[bool] = False
    reset_exit: Optional[bool] = False
