"""Serialisation JSON de SaveGame (remplace le systeme disquette/FCB
3 lettres de l'original, section 9 des notes moteur -- voir blueprint
section 6, "Sauvegarde: JSON/YAML au lieu de FCB DOS + disquette").
"""
from __future__ import annotations
import json
from dataclasses import asdict
from pathlib import Path

from .models import Character, MapCell, SaveGame, World


def save_to_file(save: SaveGame, path: str | Path) -> None:
    data = {
        "characters": [asdict(c) for c in save.characters],
        "worlds": {
            str(world_id): {
                "id": world.id,
                "cells": [asdict(cell) for cell in world.cells],
                "pos_arioch": world.pos_arioch,
            }
            for world_id, world in save.worlds.items()
        },
        "position": save.position,
        "world_id": save.world_id,
        "money": save.money,
        "energy": save.energy,
        "difficulty": save.difficulty,
        "days": save.days,
        "tasks_done": save.tasks_done,
        "tasks_counter": save.tasks_counter,
        "secret_words": save.secret_words,
        "hint_strings": save.hint_strings,
        "keys": save.keys,
        "has_map": save.has_map,
        "pos_arioch": save.pos_arioch,
        "pos_h_mountains": save.pos_h_mountains,
        "pos_v_mountains": save.pos_v_mountains,
        "loc_towns": save.loc_towns,
        "amarande_position": save.amarande_position,
        "alfol_surface_position": save.alfol_surface_position,
        "meditated": save.meditated,
        "location": save.location,
    }
    Path(path).write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")


def load_from_file(path: str | Path) -> SaveGame:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    characters = [Character(**c) for c in data["characters"]]
    worlds = {}
    for world_id, w in data["worlds"].items():
        cells = [MapCell(**cell) for cell in w["cells"]]
        worlds[int(world_id)] = World(id=w["id"], cells=cells, pos_arioch=w.get("pos_arioch"))
    return SaveGame(
        characters=characters,
        worlds=worlds,
        position=data["position"],
        world_id=data["world_id"],
        money=data["money"],
        energy=data["energy"],
        difficulty=data["difficulty"],
        days=data["days"],
        tasks_done=data["tasks_done"],
        tasks_counter=data["tasks_counter"],
        secret_words=data["secret_words"],
        hint_strings=data["hint_strings"],
        keys=data["keys"],
        has_map=data["has_map"],
        pos_arioch=data.get("pos_arioch"),
        pos_h_mountains=data.get("pos_h_mountains", []),
        pos_v_mountains=data.get("pos_v_mountains", []),
        loc_towns=data.get("loc_towns", []),
        amarande_position=data.get("amarande_position", 1000),
        alfol_surface_position=data.get("alfol_surface_position", 0),
        meditated=data.get("meditated", False),
        location=data.get("location", "amarande"),
    )
