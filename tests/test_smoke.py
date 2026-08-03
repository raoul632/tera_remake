"""Test de fumee : verifie que le moteur de base tourne sans erreur."""
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

import tempfile

from tera.models import Character, SaveGame
from tera.worldgen import generate_dungeon, generate_alfol_base, generate_meduz, CHEST_MEDITER, CHEST_INVOQUER, CHEST_BUREAU_MEDIAS
from tera.amarande import generate_mountains, generate_towns, generate_secret_words, try_move
from tera.combat import attack_power, monster_resistance, spawn_monster_hp
from tera.data_loader import get_monsters, get_objects
from tera.save import save_to_file, load_from_file


def test_worldgen():
    worlds = generate_dungeon(num_floors=3, difficulty=5, rng=random.Random(42))
    assert len(worlds) == 3
    assert len(worlds[0].cells) == 100


def test_combat():
    hero = Character(
        name="Thelia", gender="F",
        stats={"force": 10, "habilete": 8, "volonte": 6},
        skills={"force": 3},  # maitrise de la Force (action "lutter" -> action_type 1)
    )
    dmg = attack_power(hero, "lutter", party=[hero], rng=random.Random(1))
    assert dmg >= 0


def test_data_loader():
    monsters = get_monsters()
    objects = get_objects()
    assert len(monsters) == 22
    assert len(objects) == 25
    print("Premier monstre :", monsters[0])


def test_monster_hp():
    monsters = get_monsters()
    hp = spawn_monster_hp(monsters[0], rng=random.Random(7))
    assert hp >= monsters[0].hp_base


def test_save_roundtrip():
    worlds = generate_dungeon(num_floors=2, difficulty=5, rng=random.Random(3))
    hero = Character(name="Test", gender="H", stats={"force": 10}, hp=15, mp=5, max_hp=15, max_mp=5, in_party=True)
    save = SaveGame(characters=[hero], worlds=worlds, position=0, world_id=0)
    with tempfile.TemporaryDirectory() as tmpdir:
        path = f"{tmpdir}/save.json"
        save_to_file(save, path)
        loaded = load_from_file(path)
    assert loaded.characters[0].name == "Test"
    assert len(loaded.worlds) == 2
    assert loaded.worlds[0].cells[0].walls == worlds[0].cells[0].walls


def test_amarande():
    rng = random.Random(1)
    pos_h, pos_v = generate_mountains(rng)
    assert len(pos_h) == 9
    assert len(pos_v) == 25
    towns = generate_towns(rng)
    assert len(towns) == len(set(towns)) == 20
    words = generate_secret_words(rng)
    assert len(words) == 4

    pos, moved = try_move(1450, "N", pos_h, pos_v)
    assert moved
    assert pos == 1451


def test_alfol_base_and_meduz():
    rng = random.Random(2)
    alfol_base = generate_alfol_base(difficulty=5, rng=rng)
    assert sum(1 for c in alfol_base.cells if c.chest == CHEST_BUREAU_MEDIAS) == 1

    meduz = generate_meduz(rng=rng)
    assert sum(1 for c in meduz.cells if c.chest == CHEST_MEDITER) == 1
    assert sum(1 for c in meduz.cells if c.chest == CHEST_INVOQUER) == 1


if __name__ == "__main__":
    test_worldgen()
    test_combat()
    test_data_loader()
    test_monster_hp()
    test_save_roundtrip()
    test_amarande()
    test_alfol_base_and_meduz()
    print("Tous les tests de fumee sont passes.")
