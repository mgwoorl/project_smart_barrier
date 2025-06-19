from pathlib import Path


class ConfigApp:
    name = "src.app:app"
    port = 5050
    host = "0.0.0.0"
    port_vue = 5173
    origins = [
        "http://localhost",
        f"http://localhost:{port_vue}",
    ]


class Config:
    app = ConfigApp()