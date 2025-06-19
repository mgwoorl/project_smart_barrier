from pydantic import BaseModel, Field, field_validator
from typing import Dict, Optional


class UserRegisterRequestModel(BaseModel):
    name: str
    password: str
    chat_id: str
