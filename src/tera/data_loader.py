"""Chargement des donnees de jeu extraites des fichiers originaux
(VERBES, OBJETS, NOMON, NOMPERS, VALMON) -- voir data/game_data.json.
"""
from __future__ import annotations
import json
from pathlib import Path
from functools import lru_cache

from .models import Monster

DATA_PATH = Path(__file__).resolve().parent.parent.parent / "data" / "game_data.json"


@lru_cache(maxsize=1)
def load_game_data() -> dict:
    with open(DATA_PATH, encoding="utf-8") as f:
        return json.load(f)


def get_monsters() -> list[Monster]:
    data = load_game_data()
    return [
        Monster(
            id=m["id"],
            name=m["name"],
            hp_base=m["hp_base"],
            resist_a=m["resist_a"],
            resist_b=m["resist_b"],
            weakness_item_id=m["weakness_item_id"],
            initiative=m["initiative"],
            hp_variance=m["hp_variance"],
        )
        for m in data["monsters"]
    ]


def get_objects() -> list[str]:
    return load_game_data()["objets"]


def get_npc_names() -> list[str]:
    return load_game_data()["nompers"]


def get_verbs() -> list[str]:
    return load_game_data()["verbes"]


def get_messages(variant: int = 1) -> list[str]:
    """Textes de resultat d'action (MESSAGE/MESSAGE2/MESSAGE3, section 12.1
    et 17 des notes moteur) -- 3 variantes tirees en alternance pour eviter
    la repetition a chaque tour de combat. `variant` = 1, 2 ou 3."""
    key = "message" if variant == 1 else f"message{variant}"
    return load_game_data()[key]
