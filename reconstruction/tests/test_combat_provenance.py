"""Vecteurs de regression derives des preuves BIN/PAS, pas du Python ancien.

Chaque test indique la regle qu'il protege ; les valeurs de reference
proviennent de la specification relue, sans execution du binaire original.
"""
import itertools
import os
import pytest
# Run via tools/test_python_reference.py: imports the patched copy inside
# reconstruction/build/python_reference, never the historical source tree.
if os.environ.get("TERA_RECONSTRUCTION_PYTHON_AUDIT") != "1":
    pytest.skip("Use reconstruction/tools/test_python_reference.py for the isolated audit",
                allow_module_level=True)

from tera.combat import (
    ACTION_TYPES, STAT_BY_INDEX, generate_helper_items, helper_item_name,
    resolve_action, vitality_roll,
    equipment_bonus,
)
from tera.data_loader import get_objects
from tera.game import _party_turn, resolve_altar
from tera.models import Character, SaveGame
from tera.save import save_to_file, load_from_file


class Draws:
    """Fournir des tirages imposes et enregistrer leur ordre de consommation.

    Intention : distinguer une bonne formule d'une formule qui utilise le
    mauvais tirage. Une demande de tirage supplementaire epuise l'iterateur
    et fait echouer le test ; aucun aleatoire implicite ne masque l'erreur.
    """
    def __init__(self, values):
        """Preparer la suite de valeurs attendues et l'historique vide."""
        self.values = iter(values)
        self.used = []

    def randint(self, lo, hi):
        """Consommer une valeur imposee, verifiee dans les bornes inclusives."""
        value = next(self.values)
        assert lo <= value <= hi
        self.used.append(value)
        return value

    def randrange(self, stop):
        """Adapter le meme tirage impose aux bornes [0, stop[ du Pascal RANDOM."""
        return self.randint(0, stop - 1)


def actor():
    """Creer le personnage du vecteur BIN : stats=10, competences=3.

    Les quatre objets couvrent les branches testees avec bonus 3 et
    tiennent dans l'inventaire original limite a dix emplacements.
    """
    return Character("Test", "H", stats={s: 10 for s in STAT_BY_INDEX.values()},
                     skills={s: 3 for s in STAT_BY_INDEX.values()},
                     equipment=[get_objects()[i - 1] for i in (3, 15, 18, 21)],
                     in_party=True)


def test_menu_binary_correspondence():
    """Proteger la table touche/verbe/type reconstruite depuis C_8790."""
    assert ACTION_TYPES == {"lutter": 1, "tirer": 2, "attaque_mentale": 3,
                            "dissiper": 4, "psalmodier": 5, "crier": 6,
                            "jeter_un_sort": 9}


def test_hand_derived_branch_vectors_and_draw_order():
    """Verifier les neuf resultats et le nombre exact de tirages par branche."""
    # stats=10, skills=3 => base power=16; 16*5-35=45.
    # Type 6: (20+3+3)*7 DIV 2-35=56; type 7: 10*7-35=35;
    # type 8: (20+3)*7-35=126. Equipment multiplier uses bonus 3.
    expected = [180, 202, 180, 180, 180, 224, 87, 126, 45]
    hero = actor()
    for action, value in enumerate(expected, 1):
        draws = [7] if action in (6, 7, 8) else [7, 5]
        rng = Draws(draws)
        assert resolve_action(action, hero, [hero], get_objects()[2], rng) == value
        assert rng.used == draws


def test_fixed_item_cannot_be_replaced_by_selected_item():
    """Tirer exige sa famille d'armes ; une aide magique ne doit pas la remplacer."""
    hero = actor()
    hero.equipment = [get_objects()[2]]  # gemme, sans desintegrateur
    assert resolve_action(2, hero, [hero], get_objects()[2], Draws([7, 5])) == 0


def test_equipment_three_item_hierarchy_without_party_sharing():
    """Le meilleur des trois IDs gagne chez l'acteur, dans dix cases seulement."""
    hero, companion = actor(), actor()
    hero.equipment = []
    objects = get_objects()
    assert equipment_bonus(hero, [companion, hero], objects[14]) == 0
    for item_id, expected in ((13, 1), (14, 2), (15, 3)):
        hero.equipment = [objects[item_id - 1]]
        assert equipment_bonus(hero, [hero], objects[14]) == expected
    hero.equipment = [objects[12], objects[13], objects[14]]
    assert equipment_bonus(hero, [hero], objects[14]) == 3
    for required in (3, 6, 9, 12, 15, 18, 21):
        for offset in range(3):
            hero.equipment = [objects[required - offset - 1]]
            assert equipment_bonus(hero, [hero], objects[required - 1]) == 3 - offset
    hero.equipment = [""] * 10 + [objects[14]]
    assert equipment_bonus(hero, [hero], objects[14]) == 0


def test_systeme_d_and_threshold():
    """Lire systeme D, ignorer bonus_fixe invente et distinguer jets 44/45."""
    hero = actor()
    hero.stats["vitalite"] = 8
    hero.skills.update(vitalite=0, agilite=5, bonus_fixe=200)
    assert vitality_roll(hero, Draws([4])) == 0  # (8-2+5)*4=44
    hero.stats["vitalite"] = 11
    hero.skills.update(vitalite=1, agilite=3)
    assert vitality_roll(hero, Draws([3])) == 10  # (11+1+3)*3=45


def test_signed16_result_before_division():
    """Conserver le debordement IMUL avant IDIV, meme si le resultat est negatif."""
    hero = actor()
    hero.stats["habilete"] = 255
    hero.skills.update(habilete=255, agilite=255)
    assert resolve_action(2, hero, [hero], None, Draws([7, 17])) == -7171


def test_all_helper_assignments_and_rejected_draws():
    """Couvrir les 24 attributions et les tirages rejetes, sans remplacer par shuffle."""
    assignments = set()
    for permutation in itertools.permutations(range(4)):
        rng = Draws(permutation)
        items = generate_helper_items(rng)
        assignments.add(tuple(items))
        assert set(items) == {3, 6, 9, 12}
        assert rng.used == list(permutation)
    assert len(assignments) == 24
    rng = Draws([0, 0, 1, 1, 0, 2, 0, 2, 1, 3])
    assert generate_helper_items(rng) == [3, 12, 9, 6]
    assert len(rng.used) == 10
    assert helper_item_name("attaque_mentale", [3, 12, 9, 6]) == get_objects()[5]
    assert helper_item_name("dissiper", [3, 12, 9, 6]) == get_objects()[2]
    assert helper_item_name("psalmodier", [3, 12, 9, 6]) == get_objects()[8]
    assert helper_item_name("crier", [3, 12, 9, 6]) == get_objects()[11]


def test_helper_save_persistence_and_old_save(tmp_path):
    """Sauver les aides connues sans en inventer pour une sauvegarde ancienne."""
    import json
    path = tmp_path / "save.json"
    save_to_file(SaveGame(helper_items=[3, 12, 9, 6]), path)
    assert load_from_file(path).helper_items == [3, 12, 9, 6]
    data = json.loads(path.read_text(encoding="utf-8"))
    del data["helper_items"]
    path.write_text(json.dumps(data), encoding="utf-8")
    assert load_from_file(path).helper_items == []  # aucune invention retrospective


def test_altar_direct_base_roll_consumes_only_one_draw():
    """L'autel appelle C_8315 directement, sans le tirage d'entree de C_842A."""
    hero = actor()
    hero.hp = 1
    rng = Draws([5])
    resolve_altar([hero], rng)
    assert hero.hp == hero.max_hp
    assert rng.used == [5]


def test_party_turn_uses_assigned_helper_without_item_prompt():
    """Utiliser l'aide attribuee a la partie, sans proposer un choix d'objet invente."""
    from tera.data_loader import get_monsters
    hero = actor()
    prompts = []
    def ask(prompt):
        """Imposer psalmodier et relever toute question supplementaire inattendue."""
        prompts.append(prompt)
        return "psalmodier"
    hp, fled = _party_turn([hero], get_monsters()[0], 1, Draws([7, 5]),
                          ask=ask, log=lambda _: None, helper_items=[3, 12, 9, 6])
    assert hp < 1 and not fled
    assert len(prompts) == 1
