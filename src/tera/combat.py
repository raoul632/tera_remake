"""Resolution de combat, portee depuis les fonctions desassemblees
0x8315 / 0x8215 / 0x8391 / 0x82E5 / 0x842A de TERA.COM.

Voir docs/TERA_notes_moteur.md section 13 et 17, et
data/source code/TERAUTIL.INC, pour le detail complet des formules et
leur provenance. Le detail par action (`resolve_action`) reproduit les
9 branches confirmees de C_842A -- avant cette version, cette fonction
ajoutait le bonus d'equipement dans la puissance AVANT le jet, ce qui
ne correspond pas a l'original (le bonus s'applique APRES, differemment
par action).
"""
from __future__ import annotations
import random

from .models import Character, Monster
from .data_loader import get_objects

# Seuils fideles a l'original -- a NE PAS changer sans raison, ils
# encodent l'equilibrage teste par les auteurs en 1986.
SUCCESS_THRESHOLD = 44
DAMAGE_OFFSET = 35
RANDOM_MULT_RANGE = (3, 17)  # RANDOM(15) + 3

# Numerotation des stats du personnage (Sav.f_Stats[perso][1..9], section 13).
STAT_BY_INDEX = {
    1: "force", 2: "intelligence", 3: "chance", 4: "endurance",
    5: "volonte", 6: "vitalite", 7: "agilite", 8: "charme", 9: "habilete",
}

# Association verbe de combat -> action_type interne de C_842A (1-9).
# Chaque C_8315(competence, stat, perso) confirme dans le desassemblage
# passe TOUJOURS le meme indice pour stat et competence -- Character.skills
# est donc indexe par NOM DE STAT (la maitrise de cette stat en combat),
# pas par nom de verbe.
#
# action_type 7 (Endurance+Agilite, objet fixe "armure spatiale") n'a
# aucun verbe correspondant parmi les 8 de VERBES 26-33 une fois les 7
# autres assignes (invoquer utilise aussi le type 5, en dehors du menu de
# combat, pour le test de l'autel de soin) -- laisse accessible via
# resolve_action(7, ...) mais volontairement absent de ACTION_TYPES pour
# ne pas inventer une association de verbe non confirmee.
ACTION_TYPES = {
    "lutter": 1,
    "tirer": 2,
    "attaque_mentale": 3,
    "dissiper": 4,
    "invoquer": 5,
    "psalmodier": 6,
    "jeter_un_sort": 8,
    "crier": 9,
}

# Objet requis fixe par action_type (0x0F/0x12/0x15 dans le desassemblage).
FIXED_ITEMS = {1: 15, 2: 18, 7: 21}

# Actions dont l'objet requis est lu dynamiquement dans l'original
# (D_75EA/EB/EC/ED -- tres probablement "l'objet actuellement selectionne
# par le joueur pour cette action", role exact non confirme). Le CLI doit
# demander au joueur quel objet il utilise pour ces actions-la.
DYNAMIC_ITEM_ACTIONS = {"attaque_mentale", "dissiper", "invoquer", "psalmodier"}

# Actions sans verification d'equipement du tout dans l'original.
NO_ITEM_ACTIONS = {"jeter_un_sort", "crier"}


def skill_modifier(char: Character, stat: str) -> int:
    """Port de C_8215 : -2 si la maitrise de `stat` n'est pas acquise, sinon son niveau."""
    level = char.skills.get(stat, 0)
    return level if level > 0 else -2


def equipment_bonus(char: Character, party: list[Character], required_item: str | None) -> int:
    """Port de C_8391/C_8270 : bonus degressif 3/2/1/0.

    3 si le personnage a lui-meme l'objet requis, 2/1 si un compagnon
    proche dans l'ordre du groupe l'a, 0 si personne (ou si aucun objet
    n'est requis).
    """
    if required_item is None:
        return 0
    idx = party.index(char) if char in party else 0
    for offset, bonus in ((0, 3), (1, 2), (2, 1)):
        i = idx - offset
        if 0 <= i < len(party) and required_item in party[i].equipment:
            return bonus
    return 0


def _fixed_item_name(action_type: int) -> str | None:
    obj_id = FIXED_ITEMS.get(action_type)
    if obj_id is None:
        return None
    objects = get_objects()
    return objects[obj_id - 1] if obj_id - 1 < len(objects) else None


def _base_roll(char: Character, stat_idx: int, skill_idx: int, rng: random.Random) -> int:
    """Port de C_8315(competence, stat, perso) -- SANS equipement (voir
    section 17 : le bonus d'equipement n'est jamais dans cette fonction,
    contrairement a l'ancienne hypothese)."""
    stat_value = char.stats.get(STAT_BY_INDEX[stat_idx], 0)
    power = (
        stat_value
        + skill_modifier(char, STAT_BY_INDEX[skill_idx])
        + char.skills.get("bonus_fixe", 0)
    )
    roll = power * rng.randint(*RANDOM_MULT_RANGE)
    return roll - DAMAGE_OFFSET if roll > SUCCESS_THRESHOLD else 0


def negotiation_roll(char: Character, rng: random.Random | None = None) -> int:
    """Port de `C_8315(perso,8,8)` tel qu'utilise par le commerce de
    Kronopolis (`OVL_00BC.PAS::__Buying`/`__Selling`, section 7 des
    notes moteur) : le MEME jet de base que le combat, applique au
    Charme (indice 8) pour la stat ET la competence -- pas juste la
    stat Charme brute comme une hypothese precedente le supposait."""
    rng = rng or random
    return _base_roll(char, 8, 8, rng)


def resolve_action(
    action_type: int, char: Character, party: list[Character],
    required_item: str | None, rng: random.Random | None = None,
) -> int:
    """Port complet de C_842A -- une branche par action_type (1-9).

    Voir docs/TERA_notes_moteur.md section 17 et
    data/source code/TERAUTIL.INC pour le detail de chaque branche.
    `required_item` : nom d'objet fourni par l'appelant pour les actions
    a objet dynamique (3,4,5,6) ; ignore pour les actions a objet fixe
    (1,2,7, qui utilisent leur objet fixe quoi qu'il arrive) et pour
    celles sans equipement (8,9).
    """
    rng = rng or random

    if action_type in FIXED_ITEMS and required_item is None:
        required_item = _fixed_item_name(action_type)

    if action_type == 1:
        base = _base_roll(char, 1, 1, rng)
        equip = equipment_bonus(char, party, required_item)
        return base * (1 + equip)
    if action_type == 2:
        base = _base_roll(char, 9, 9, rng)
        equip = equipment_bonus(char, party, required_item)
        return (base * equip * 3) // 2
    if action_type == 3:
        base = _base_roll(char, 5, 5, rng)
        equip = equipment_bonus(char, party, required_item)
        return base * (1 + equip)
    if action_type == 4:
        base = _base_roll(char, 3, 3, rng)
        equip = equipment_bonus(char, party, required_item)
        return base * (1 + equip)
    if action_type == 5:
        base = _base_roll(char, 6, 6, rng)
        equip = equipment_bonus(char, party, required_item)
        return base * (1 + equip)
    if action_type == 6:
        stat_sum = char.stats.get("chance", 0) + char.stats.get("vitalite", 0)
        jet = (
            (stat_sum + skill_modifier(char, "vitalite") + skill_modifier(char, "chance"))
            * rng.randint(*RANDOM_MULT_RANGE)
        ) // 2
        base = jet - DAMAGE_OFFSET if jet > SUCCESS_THRESHOLD else 0
        equip = equipment_bonus(char, party, required_item)
        return base * (1 + equip)
    if action_type == 7:
        stat_sum = (char.stats.get("endurance", 0) + char.stats.get("agilite", 0)) // 2
        jet = stat_sum * rng.randint(*RANDOM_MULT_RANGE)
        base = jet - DAMAGE_OFFSET if jet > SUCCESS_THRESHOLD else 0
        equip = equipment_bonus(char, party, required_item)
        return (base * (2 + equip)) // 2
    if action_type == 8:
        stat_sum = char.stats.get("intelligence", 0) + char.stats.get("volonte", 0)
        jet = (stat_sum + skill_modifier(char, "volonte")) * rng.randint(*RANDOM_MULT_RANGE)
        return jet - DAMAGE_OFFSET if jet > SUCCESS_THRESHOLD else 0
    if action_type == 9:
        return _base_roll(char, 2, 2, rng)
    raise ValueError(f"action_type inconnu : {action_type}")


def attack_power(char: Character, action: str, party: list[Character],
                  required_item: str | None = None, rng: random.Random | None = None) -> int:
    """Point d'entree par nom de verbe (tirer/lutter/...). Voir `resolve_action`
    pour le detail par action_type et `ACTION_TYPES` pour la correspondance.
    """
    rng = rng or random
    return resolve_action(ACTION_TYPES[action], char, party, required_item, rng)


def monster_resistance(monster: Monster, item_used: str | None) -> int:
    """Port de la fonction 0x8D20 : resistance/faiblesse du monstre.

    Si l'objet utilise correspond a la faiblesse declaree du monstre,
    diviseur favorable (colonne resist_a / 5) ; sinon diviseur normal
    (colonne resist_b / 7). Le detail exact des branches reste NON
    RESOLU (bloque par un pointeur de portee imbriquee non retrace, voir
    docs/TERA_notes_moteur.md section 17) -- ceci reste une approximation
    documentee, pas une reconstruction confirmee.
    """
    if monster.weakness_item_id is not None and item_used == str(monster.weakness_item_id):
        return monster.resist_a // 5
    return monster.resist_b // 7


def spawn_monster_hp(monster: Monster, rng: random.Random | None = None) -> int:
    """Port de la generation des PV a l'invocation (fonction 0x95D9)."""
    rng = rng or random
    return monster.hp_base + rng.randint(0, max(0, monster.hp_variance - 1))


def initiative_check(party: list[Character], monster: Monster, rng: random.Random | None = None) -> bool:
    """Retourne True si le groupe agit en premier.

    Port de la comparaison Agilite moyenne du groupe vs
    (VALMON[type][5] + RANDOM(5) - 2).
    """
    rng = rng or random
    present = [c for c in party if c.in_party and c.state != "HS"]
    if not present:
        return False
    avg_agility = sum(c.stats.get("agilite", 0) for c in present) / len(present)
    monster_init = monster.initiative + rng.randint(0, 4) - 2
    return avg_agility > monster_init
